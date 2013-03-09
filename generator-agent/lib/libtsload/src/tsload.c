/*
 * init.c
 *
 *  Created on: Dec 3, 2012
 *      Author: myaut
 */

#define LOG_SOURCE ""
#include <log.h>

#include <defs.h>
#include <threads.h>
#include <mempool.h>
#include <workload.h>
#include <modules.h>
#include <wltype.h>
#include <threadpool.h>
#include <tstime.h>
#include <uname.h>

#include <tsinit.h>
#include <tsload.h>

#include <libjson.h>

#include <stdlib.h>

tsload_error_msg_func tsload_error_msg = NULL;
tsload_workload_status_func tsload_workload_status = NULL;
tsload_requests_report_func tsload_requests_report = NULL;

extern ts_time_t tp_min_quantum;

struct subsystem subsys[] = {
	SUBSYSTEM("log", log_init, log_fini),
	SUBSYSTEM("mempool", mempool_init, mempool_fini),
	SUBSYSTEM("threads", threads_init, threads_fini),
	SUBSYSTEM("wl_type", wlt_init, wlt_fini),
	SUBSYSTEM("modules", mod_init, mod_fini),
	SUBSYSTEM("workload", wl_init, wl_fini),
	SUBSYSTEM("threadpool", tp_init, tp_fini)
};

JSONNODE* tsload_get_workload_types(void) {
	return json_wl_type_format_all();
}

int tsload_configure_workload(const char* wl_name, JSONNODE* wl_params) {
	workload_t* wl = json_workload_proc(wl_name, wl_params);

	if(wl == NULL) {
		tsload_error_msg(TSE_INTERNAL_ERROR, "Error in tsload_configure_workload!");
		return TSLOAD_ERROR;
	}

	wl_config(wl);

	return TSLOAD_OK;
}

int tsload_provide_step(const char* wl_name, long step_id, unsigned num_rqs, int* pstatus) {
	workload_t* wl = wl_search(wl_name);
	int ret;

	if(wl == NULL) {
		tsload_error_msg(TSE_INVALID_DATA, "Not found workload %s", wl_name);
		return TSLOAD_ERROR;
	}

	ret = wl_provide_step(wl, step_id, num_rqs);

	switch(ret) {
	case WL_STEP_INVALID:
		tsload_error_msg(TSE_INVALID_DATA, "Step %ld is not correct, last step was %ld!", step_id, wl->wl_last_step);
		return TSLOAD_ERROR;
	case WL_STEP_QUEUE_FULL:
	case WL_STEP_OK:
		*pstatus = ret;
		return TSLOAD_OK;
	}
}

int tsload_start_workload(const char* wl_name, ts_time_t start_time) {
	workload_t* wl = wl_search(wl_name);

	if(wl == NULL) {
		tsload_error_msg(TSE_INVALID_DATA, "Not found workload %s", wl_name);
		return TSLOAD_ERROR;
	}

	if(wl->wl_is_configured == B_FALSE) {
		tsload_error_msg(TSE_INVALID_STATE, "Workload %s not yet configured", wl_name);
		return TSLOAD_ERROR;
	}

	wl->wl_start_time = start_time;
	return TSLOAD_OK;
}

int tsload_unconfigure_workload(const char* wl_name) {
	workload_t* wl = wl_search(wl_name);

	if(wl == NULL) {
		tsload_error_msg(TSE_INVALID_DATA, "Not found workload %s", wl_name);
		return TSLOAD_ERROR;
	}

	if(wl->wl_status == WLS_CONFIGURING) {
		tsload_error_msg(TSE_INVALID_STATE, "Workload %s is currently configuring", wl_name);
		return TSLOAD_ERROR;
	}

	if(wl->wl_status == WLS_RUNNING) {
		/* XXX: does this cause race condition if we are finish workload at the same time? */
		tp_detach(wl->wl_tp, wl);
	}

	wl_unconfig(wl);
	wl_destroy(wl);
	return TSLOAD_OK;
}

int tsload_create_threadpool(unsigned num_threads, const char* tp_name, ts_time_t quantum) {
	thread_pool_t* tp = NULL;

	if(num_threads > TPMAXTHREADS || num_threads == 0) {
		tsload_error_msg(TSE_INVALID_DATA, "Too much or zero threads requested (%u, max: %u)", num_threads,
							TPMAXTHREADS);
		return TSLOAD_ERROR;
	}

	if(quantum < tp_min_quantum) {
		tsload_error_msg(TSE_INVALID_DATA, "Too small quantum requested (%lld, max: %lld)", quantum,
							tp_min_quantum);
		return TSLOAD_ERROR;
	}

	tp = tp_search(tp_name);

	if(tp != NULL) {
		tsload_error_msg(TSE_INVALID_DATA, "Thread pool '%s' already exists", tp_name);
		return TSLOAD_ERROR;
	}

	tp = tp_create(num_threads, tp_name, quantum);

	if(tp == NULL) {
		tsload_error_msg(TSE_INTERNAL_ERROR, "Internal error occured while creating threadpool");
		return TSLOAD_ERROR;
	}

	return TSLOAD_OK;
}

JSONNODE* tsload_get_threadpools(void) {
	return json_tp_format_all();
}

int tsload_destroy_threadpool(const char* tp_name) {
	thread_pool_t* tp = tp_search(tp_name);

	if(tp == NULL) {
		tsload_error_msg(TSE_INVALID_DATA, "Thread pool '%s' not exists", tp_name);
		return TSLOAD_ERROR;
	}

	if(tp->tp_wl_count > 0) {
		tsload_error_msg(TSE_INVALID_STATE, "Thread pool '%s' has workloads running on it", tp_name);
		return TSLOAD_ERROR;
	}

	tp_destroy(tp);
	return TSLOAD_OK;
}

int tsload_init(struct subsystem* xsubsys, unsigned xs_count) {
	struct subsystem** subsys_list = NULL;
	unsigned s_count = sizeof(subsys) / sizeof(struct subsystem);
	int si, xsi;

	/* Mempool is not yet initialized, so use libc; freed by ts_finish */
	subsys_list = malloc((s_count + xs_count) * sizeof(struct subsystem*));

	/* Initialize [0:s_count] subsystems with tsload's subsystems
	 * and [s_count:s_count+xs_count] with extended subsystems */
	for(si = 0; si < s_count; ++si)
		subsys_list[si] = &subsys[si];
	for(xsi = 0; xsi < xs_count; ++si, ++xsi)
		subsys_list[si] = &xsubsys[xsi];

	return ts_init(subsys_list, s_count + xs_count);
}

int tsload_start(const char* basename) {
	logmsg(LOG_INFO, "Started %s on %s...", basename, hi_get_nodename());
	logmsg(LOG_INFO, "Clock resolution is %llu", tm_get_clock_res());

	/*Wait until universe collapses or we receive a signal :)*/
	t_eternal_wait();

	/*NOTREACHED*/
	return 0;
}

