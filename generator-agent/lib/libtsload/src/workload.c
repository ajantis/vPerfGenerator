/*
 * workload.c
 *
 *  Created on: Nov 19, 2012
 *      Author: myaut
 */

#define LOG_SOURCE "workload"
#include <log.h>

#include <defs.h>
#include <list.h>
#include <hashmap.h>
#include <mempool.h>
#include <modules.h>
#include <threads.h>
#include <threadpool.h>
#include <workload.h>
#include <modtsload.h>
#include <loadagent.h>
#include <tstime.h>

#include <libjson.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

squeue_t	wl_requests;

thread_t	t_wl_requests;

DECLARE_HASH_MAP(workload_hash_map, workload_t, WLHASHSIZE, wl_name, wl_hm_next,
	{
		char* p = (char*) key;
		unsigned hash = 0;

		while(*p != 0)
			hash += *p++;

		return hash & WLHASHMASK;
	},
	{
		return strcmp(key1, key2);
	}
)

/**
 * wl_create - create new workload: allocate memory and initialize fields
 *
 * @return NULL if malloc had failed or new workload object
 */
workload_t* wl_create(const char* name, module_t* mod, thread_pool_t* tp) {
	workload_t* wl = (workload_t*) mp_malloc(sizeof(workload_t));
	tsload_module_t* tmod = NULL;

	assert(mod->mod_helper != NULL);

	tmod = (tsload_module_t*) mod->mod_helper;

	if(wl == NULL)
		return NULL;

	strncpy(wl->wl_name, name, WLNAMELEN);
	wl->wl_mod = mod;
	wl->wl_ts_mod = tmod;

	wl->wl_tp = tp;

	wl->wl_current_rq = 0;
	wl->wl_current_step = -1;
	wl->wl_last_step = -1;

	wl->wl_hm_next = NULL;

	list_node_init(&wl->wl_chain);
	list_node_init(&wl->wl_tp_node);

	wl->wl_params = mp_malloc(tmod->mod_params_size);

	wl->wl_is_configured = FALSE;

	wl->wl_is_started = FALSE;
	wl->wl_start_time = TS_TIME_MAX;

	mutex_init(&wl->wl_rq_mutex, "wl-%s-rq", name);

	hash_map_insert(&workload_hash_map, wl);

	tp_attach(tp, wl);

	return wl;
}

/**
 * wl_destroy - free memory for single workload_t object
 * */
void wl_destroy(workload_t* wl) {
	hash_map_remove(&workload_hash_map, wl);

	list_del(&wl->wl_chain);

	tp_detach(wl->wl_tp, wl);

	mutex_destroy(&wl->wl_rq_mutex);

	mp_free(wl->wl_params);
	mp_free(wl);
}

/**
 * wl_destroy_all - free memory for chain of workload objects
 * */
void wl_destroy_all(list_head_t* wl_head) {
	workload_t *wl, *wl_tmp;

	list_for_each_entry_safe(wl, wl_tmp, wl_head, wl_chain) {
		wl_destroy(wl);
	}
}

workload_t* wl_search(const char* name) {
	return hash_map_find(&workload_hash_map, name);
}

/**
 * Notify server of workload configuring progress.
 *
 * done is presented in percent. If status is WLS_FAIL - it would be set to -1,
 * if status is WLS_SUCCESS or WLS_FINISHED - it would be set to 100
 *
 * @param wl configuring workload
 * @param status configuration/execution status
 * @param done configuration progress (in percent)
 * @param format message format string
 */
void wl_notify(workload_t* wl, wl_status_t status, int done, char* format, ...) {
	char config_msg[512];
	va_list args;

	switch(status) {
	case WLS_CONFIGURING:
		/* 146% and -5% are awkward */
		if(done < 0) done = 0;
		if(done > 100) done = 100;
	break;
	case WLS_FAIL:
		done = -1;
	break;
	case WLS_SUCCESS:
	case WLS_FINISHED:
		done = 100;
	break;
	default:
		assert(status == WLS_CONFIGURING);
	break;
	}

	va_start(args, format);
	vsnprintf(config_msg, 512, format, args);
	va_end(args);

	logmsg((status == WLS_FAIL)? LOG_WARN : LOG_INFO,
			"Workload %s status: %s", wl->wl_name, config_msg);

	agent_workload_status(wl->wl_name, status, done, config_msg);
}

void* wl_config_thread(void* arg) {
	THREAD_ENTRY(arg, workload_t, wl);
	int ret;

	logmsg(LOG_INFO, "Started configuring of workload %s", wl->wl_name);

	ret = wl->wl_ts_mod->mod_wl_config(wl);

	if(ret != 0) {
		logmsg(LOG_WARN, "Failed to configure workload %s", wl->wl_name);

		THREAD_EXIT();
	}

	wl->wl_is_configured = TRUE;

	wl_notify(wl, WLS_SUCCESS, 100, "Successfully configured workload %s", wl->wl_name);



THREAD_END:
	THREAD_FINISH(arg);
}

void wl_config(workload_t* wl) {
	t_init(&wl->wl_cfg_thread, (void*) wl, wl_config_thread,
			"wl-cfg-%s", wl->wl_name);
}

void wl_unconfig(workload_t* wl) {
	wl->wl_ts_mod->mod_wl_unconfig(wl);
}

int wl_is_started(workload_t* wl) {
	if(wl->wl_is_started)
		return TRUE;

	if(tm_get_time() >= wl->wl_start_time) {
		logmsg("Starting workload %s...", wl->wl_name);

		wl->wl_is_started = TRUE;
		return TRUE;
	}

	return FALSE;
}

/**
 * Provide number of requests for current step
 *
 * Responds to agent:
 * 	- Error AE_INVALID_DATA if step_id is incorrect
 * 	- Response { "success": true | false, "reason": "Reason" }
 *
 * @param wl workload
 * @param step_id step id to be provided
 * @param num_rqs number
 *
 * @return 0 if steps are saved, -1 if incorrect step provided or wl_requests is full
 * */
int wl_provide_step(workload_t* wl, long step_id, unsigned num_rqs) {
	int ret = WL_STEP_OK;
	long step_off = step_id & WLSTEPQMASK;

	mutex_lock(&wl->wl_rq_mutex);

	if((wl->wl_last_step - wl->wl_current_step) == WLSTEPQSIZE) {
		ret = WL_STEP_QUEUE_FULL;
		goto done;
	}

	if(step_id != (wl->wl_last_step + 1)) {
		/*Provided incorrect step*/
		agent_error_msg(AE_INVALID_DATA, "Step %ld is not correct, last step was %ld!", step_id, wl->wl_last_step);

		ret = WL_STEP_INVALID;
		goto done;
	}

	wl->wl_last_step = step_id;
	wl->wl_rqs_per_step[step_off] = num_rqs;

done:
	mutex_unlock(&wl->wl_rq_mutex);

	return ret;
}

/**
 * Proceed to next step, and return number of requests in it
 *
 * @param wl processing workload
 * @param p_num_rqs variable where number of requests is saved. Cannot be NULL
 *
 * @return 0 if operation successful or -1 if not steps on queue
 * */
int wl_advance_step(workload_step_t* step) {
	int ret = 0;
	long step_off;

	workload_t* wl = NULL;

	assert(step != NULL);

	wl = step->wls_workload;

	mutex_lock(&wl->wl_rq_mutex);

	wl->wl_current_step++;
	wl->wl_current_rq = 0;

	if(wl->wl_current_step > wl->wl_last_step) {
		/* No steps on queue */
		logmsg(LOG_WARN, "No steps on queue %s, step #%d!", wl->wl_name, wl->wl_current_step);

		ret = -1;
		goto done;
	}

	step_off = wl->wl_current_step  & WLSTEPQMASK;
	step->wls_rq_count = wl->wl_rqs_per_step[step_off];

done:
	mutex_unlock(&wl->wl_rq_mutex);

	return ret;
}

/**
 * Create request structure, append it to requests queue, initialize
 *
 * @param wl workload for request
 * @param thread_id id of thread that will execute thread
 * */
request_t* wl_create_request(workload_t* wl, int thread_id) {
	request_t* rq = (request_t*) mp_malloc(sizeof(request_t));

	rq->rq_step = wl->wl_current_step;
	rq->rq_id = wl->wl_current_rq++;

	rq->rq_thread_id = thread_id;

	rq->rq_flags = 0;

	rq->rq_workload = wl;

	rq->rq_start_time = 0;
	rq->rq_end_time = 0;

	list_node_init(&rq->rq_node);

	return rq;
}

/**
 * Run request for execution
 */
void wl_run_request(request_t* rq) {
	int ret;
	workload_t* wl = rq->rq_workload;

	rq->rq_flags |= RQF_STARTED;

	rq->rq_start_time = tm_get_time();
	ret = wl->wl_ts_mod->mod_run_request(rq);
	rq->rq_end_time = tm_get_time();

	/* FIXME: on-time condition*/
	if(rq->rq_end_time < (wl->wl_tp->tp_time + wl->wl_tp->tp_quantum))
		rq->rq_flags |= RQF_ONTIME;

	if(ret == 0)
		rq->rq_flags |= RQF_SUCCESS;

	rq->rq_flags |= RQF_FINISHED;
}

void wl_rq_chain_push(list_head_t* rq_chain) {
	squeue_push(&wl_requests, rq_chain);
}

void wl_rq_chain_destroy(void *p_rq_chain) {
	list_head_t* rq_chain = (list_head_t*) p_rq_chain;
	request_t *rq, *rq_tmp;

	list_for_each_entry_safe(rq, rq_tmp, rq_chain, rq_node) {
		wl_request_free(rq);
	}

	mp_free(p_rq_chain);
}

void* wl_requests_thread(void* arg) {
	THREAD_ENTRY(arg, void, unused);
	list_head_t* rq_chain;

	JSONNODE* j_rq_chain;

	request_t *rq, *rq_tmp;

	while(TRUE) {
		rq_chain =  (list_head_t*) squeue_pop(&wl_requests);

		if(rq_chain == NULL)
			THREAD_EXIT();

		j_rq_chain = json_request_format_all(rq_chain);
		agent_requests_report(j_rq_chain);

		wl_rq_chain_destroy(rq_chain);
	}

THREAD_END:
	THREAD_FINISH(arg);
}

void wl_request_free(request_t* rq) {
	mp_free(rq);
}

JSONNODE* json_request_format_all(list_head_t* rq_list) {
	JSONNODE* jrq;
	JSONNODE* j_rq_list = json_new(JSON_ARRAY);

	request_t* rq;

	list_for_each_entry(rq, rq_list, rq_node) {
		jrq = json_new(JSON_NODE);

		json_push_back(jrq, json_new_i("step", rq->rq_step));
		json_push_back(jrq, json_new_i("request", rq->rq_id));
		json_push_back(jrq, json_new_i("thread", rq->rq_thread_id));

		json_push_back(jrq, json_new_i("start", rq->rq_start_time));
		json_push_back(jrq, json_new_i("end", rq->rq_end_time));

		json_push_back(jrq, json_new_i("flags", rq->rq_flags));

		json_push_back(j_rq_list, jrq);
	}

	return j_rq_list;
}

void json_workload_proc_all(JSONNODE* node, list_head_t* wl_list) {
	JSONNODE_ITERATOR iter = json_begin(node),
			          end = json_end(node);
	workload_t* wl;
	char* wl_name;

	while(iter != end) {
		wl_name = json_name(*iter);

		wl = json_workload_proc(wl_name, *iter);
		++iter;

		json_free(wl_name);

		/* json_workload_proc failed to process workload,
		 * free all workloads that are already parsed and return NULL*/
		if(wl == NULL) {
			logmsg(LOG_WARN, "Failed to process workloads from JSON, discarding all of them");
			wl_destroy_all(wl_list);

			return;
		}

		/* Insert workload into workload chain */
		list_add_tail(&wl->wl_chain, wl_list);
	}
}


#define JSON_GET_VALIDATE_PARAM(iter, name, req_type)	\
	({											\
		iter = json_find(node, name);			\
		if(iter == i_end) {						\
			agent_error_msg(AE_MESSAGE_FORMAT,	\
					"Failed to parse workload, missing parameter %s", name);	\
			goto fail;							\
		}										\
		if(json_type(*iter) != req_type) {		\
			agent_error_msg(AE_MESSAGE_FORMAT,	\
					"Expected that " name " is " #req_type );	\
			goto fail;							\
		}										\
	})

#define SEARCH_OBJ(iter, type, search) 		\
	({										\
		type* obj = NULL;					\
		char* name = json_as_string(*iter);	\
		obj = search(name);					\
		json_free(name);					\
		if(obj == NULL) {					\
			agent_error_msg(AE_INVALID_DATA,				\
					"Invalid " #type " name %s", name);		\
			goto fail;						\
		}									\
		obj;								\
	});


/**
 * Process incoming workload in format
 * {
 * 		"module": "..."
 * 		"threadpool": "...",
 * 		"params": {}
 * }
 * and create workload. Called from agent context,
 * so returns NULL in case of error and invokes agent_error_msg
 * */
workload_t* json_workload_proc(const char* wl_name, JSONNODE* node) {
	JSONNODE_ITERATOR i_mod = NULL;
	JSONNODE_ITERATOR i_tp = NULL;
	JSONNODE_ITERATOR i_params = NULL;
	JSONNODE_ITERATOR i_end = json_end(node);

	workload_t* wl = NULL;
	module_t* mod = NULL;
	tsload_module_t* tmod = NULL;
	thread_pool_t* tp = NULL;

	int ret;

	/* Get threadpool and module from incoming message
	 * and find corresponding objects*/
	JSON_GET_VALIDATE_PARAM(i_mod, "module", JSON_STRING);
	JSON_GET_VALIDATE_PARAM(i_tp, "threadpool", JSON_STRING);
	JSON_GET_VALIDATE_PARAM(i_params, "params", JSON_NODE);

	mod = SEARCH_OBJ(i_mod, module_t, mod_search);
	tp = SEARCH_OBJ(i_tp, thread_pool_t, tp_search);

	/* Save tmod interface */
	assert(mod->mod_helper != NULL);
	tmod = (tsload_module_t*) mod->mod_helper;

	/* Get workload's name */
	logmsg(LOG_DEBUG, "Parsing workload %s", wl_name);

	if(strlen(wl_name) == 0) {
		agent_error_msg(AE_MESSAGE_FORMAT,"Failed to parse workload, no name was defined");
		goto fail;
	}

	if(wl_search(wl_name) != NULL) {
		agent_error_msg(AE_INVALID_DATA, "Workload %s already exists %s", wl_name);
		goto fail;
	}

	/* Create workload */
	wl = wl_create(wl_name, mod, tp);

	/* Process params from i_params to wl_params, where mod_params contains
	 * parameters descriptions (see wlparam) */
	ret = json_wlparam_proc_all(*i_params, tmod->mod_params, wl->wl_params);

	if(ret != WLPARAM_JSON_OK) {
		goto fail;
	}

	return wl;

fail:
	if(wl)
		wl_destroy(wl);

	return NULL;
}

int wl_init(void) {
	hash_map_init(&workload_hash_map, "workload_hash_map");

	squeue_init(&wl_requests, "wl-requests");
	t_init(&t_wl_requests, NULL, wl_requests_thread, "wl_requests");

	return 0;
}

void wl_fini(void) {
	squeue_destroy(&wl_requests, wl_rq_chain_destroy);
	t_destroy(&t_wl_requests);

	hash_map_destroy(&workload_hash_map);
}