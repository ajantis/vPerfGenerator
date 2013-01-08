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

struct subsystem subsys[] = {
	SUBSYSTEM("log", log_init, log_fini),
	SUBSYSTEM("mempool", mempool_init, mempool_fini),
	SUBSYSTEM("threads", threads_init, threads_fini),
	SUBSYSTEM("modules", mod_init, mod_fini),
	SUBSYSTEM("workload", wl_init, wl_fini),
	SUBSYSTEM("threadpool", tp_init, tp_fini)
};

JSONNODE* tsload_get_modules_info(void) {
	return json_modules_info();
}

void tsload_configure_workload(const char* wl_name, JSONNODE* wl_params) {
	workload_t* wl = json_workload_proc(wl_name, wl_params);

	if(wl == NULL) {
		return tsload_error_msg(TSE_INTERNAL_ERROR, "Error in agent_configure_workload!");
	}

	wl_config(wl);
}

int tsload_provide_step(const char* wl_name, long step_id, unsigned num_rqs) {
	workload_t* wl = wl_search(wl_name);

	if(wl == NULL) {
		tsload_error_msg(TSE_INVALID_DATA, "Not found workload %s", wl_name);
		return;
	}

	return wl_provide_step(wl, step_id, num_rqs);
}

void tsload_start_workload(const char* wl_name, ts_time_t start_time) {
	workload_t* wl = wl_search(wl_name);

	if(wl == NULL) {
		tsload_error_msg(TSE_INVALID_DATA, "Not found workload %s", wl_name);
		return;
	}

	if(wl->wl_is_configured == B_FALSE) {
		tsload_error_msg(TSE_INVALID_STATE, "Workload %s not yet configured", wl_name);
		return;
	}

	wl->wl_start_time = start_time;
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

