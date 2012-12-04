/*
 * workload.c
 *
 *  Created on: Nov 19, 2012
 *      Author: myaut
 */

#define LOG_SOURCE "workload"
#include <log.h>

#include <defs.h>
#include <hashmap.h>
#include <mempool.h>
#include <modules.h>
#include <threads.h>
#include <threadpool.h>
#include <workload.h>
#include <modtsload.h>
#include <loadagent.h>

#include <libjson.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

DECLARE_HASH_MAP(workload_hash_map, workload_t, WLHASHSIZE, wl_name, wl_hm_next,
	{
		char* p = (char*) key;
		unsigned hash;

		while(*p != 0)
			hash += *p;

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

	char sq_steps_name[SQUEUENAMELEN];
	char sq_requests_name[SQUEUENAMELEN];

	assert(mod->mod_helper != NULL);

	tmod = (tsload_module_t*) mod->mod_helper;

	if(wl == NULL)
		return NULL;

	strncpy(wl->wl_name, name, WLNAMELEN);
	wl->wl_mod = mod;
	wl->wl_ts_mod = tmod;

	wl->wl_tp = tp;

	wl->wl_next = NULL;
	wl->wl_tp_next = NULL;
	wl->wl_hm_next = NULL;

	wl->wl_params = mp_malloc(tmod->mod_params_size);

	wl->wl_is_configured = FALSE;

	snprintf(sq_steps_name, SQUEUENAMELEN, "wl-%s-steps", name);
	snprintf(sq_requests_name, SQUEUENAMELEN, "wl-%s-rqs", name);

	squeue_init(&wl->wl_steps, sq_steps_name);
	squeue_init(&wl->wl_requests, sq_requests_name);

	hash_map_insert(&workload_hash_map, wl);

	return wl;
}

/**
 * wl_destroy - free memory for single workload_t object
 * */
void wl_destroy(workload_t* wl) {
	hash_map_remove(&workload_hash_map, wl);

	squeue_destroy(&wl->wl_steps, mp_free);
	squeue_destroy(&wl->wl_requests, mp_free);

	mp_free(wl->wl_params);

	mp_free(wl);
}

/**
 * wl_destroy_all - free memory for chain of workload objects
 * */
void wl_destroy_all(workload_t* wl_head) {
	workload_t* wl = wl_head;

	while(wl) {
		wl_head = wl;
		wl = wl->wl_next;

		wl_destroy(wl_head);
	}
}

workload_t* wl_search(const char* name) {
	return hash_map_find(&workload_hash_map, name);
}

/**
 * Notify server of workload configuring progress.
 *
 * @param wl configuring workload
 * @param status configuration status
 * @param done configuration progress (in percent)
 * @param format message format string
 */
void wl_notify(workload_t* wl, wl_status_t status, int done, char* format, ...) {
	const char* status_msg[] = {"configuring", "success", "fail"};

	char config_msg[512];
	va_list args;

	/* 146% and -5% are awkward */
	if(done < 0)
		done = 0;
	if(done > 100)
		done = 100;

	va_start(args, format);
	vsnprintf(config_msg, 512, format, args);
	va_end(args);

	agent_workload_status(wl->wl_name, status_msg[status], done, config_msg);
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

THREAD_END:
	THREAD_FINISH(arg);
}

void wl_config(workload_t* wl) {
	char t_name[TNAMELEN];
	snprintf(t_name, TNAMELEN, "wl-cfg-%s", wl->wl_name);

	t_init(&wl->wl_cfg_thread, (void*) wl, t_name, wl_config_thread);
}

void wl_unconfig(workload_t* wl) {
	wl->wl_ts_mod->mod_wl_unconfig(wl);
}

workload_t* json_workload_proc_all(JSONNODE* node) {
	JSONNODE_ITERATOR iter = json_begin(node),
			          end = json_end(node);
	workload_t* wl, *wl_head = NULL, *wl_last = NULL;

	while(iter != end) {
		wl = json_workload_proc(*iter);
		++iter;

		/* json_workload_proc failed to process workload,
		 * free all workloads that are already parsed and return NULL*/
		if(wl == NULL) {
			logmsg(LOG_WARN, "Failed to process workloads from JSON, discarding all of them");
			wl_destroy_all(wl_head);

			return NULL;
		}

		/* Insert workload into workload chain */
		if(wl_head == NULL) {
			wl_last = wl_head = wl;
		}
		else {
			wl_last->wl_next = wl;
			wl_last = wl;
		}
	}

	return wl_head;
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
workload_t* json_workload_proc(JSONNODE* node) {
	JSONNODE_ITERATOR i_mod = NULL;
	JSONNODE_ITERATOR i_tp = NULL;
	JSONNODE_ITERATOR i_params = NULL;
	JSONNODE_ITERATOR i_end = json_end(node);

	workload_t* wl = NULL;
	module_t* mod = NULL;
	tsload_module_t* tmod = NULL;
	thread_pool_t* tp = NULL;

	char* wl_name = NULL;

	int ret;

	logmsg(LOG_DEBUG, "Parsing workload %s", wl_name);

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
	wl_name = json_name(node);

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
	if(wl_name)
		json_free(wl_name);

	if(wl)
		wl_destroy(wl);

	return NULL;
}

int wl_init(void) {
	hash_map_init(&workload_hash_map, "workload_hash_map");

	return 0;
}

void wl_fini(void) {
	hash_map_destroy(&workload_hash_map);
}
