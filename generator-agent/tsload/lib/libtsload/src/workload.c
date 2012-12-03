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
workload_t* wl_create(const char* name, module_t* mod) {
	workload_t* wl = (workload_t*) mp_malloc(sizeof(workload_t));
	tsload_module_t* tmod = NULL;

	assert(mod->mod_helper != NULL);

	tmod = (tsload_module_t*) mod->mod_helper;

	if(wl == NULL)
		return NULL;

	strncpy(wl->wl_name, name, WLNAMELEN);
	wl->wl_mod = mod;
	wl->wl_ts_mod = tmod;

	wl->wl_tp = NULL;
	wl->wl_next = NULL;

	wl->wl_params = mp_malloc(tmod->mod_params_size);

	wl->wl_is_configured = FALSE;

	return wl;
}

/**
 * wl_free - free memory for single workload_t object
 * */
void wl_free(workload_t* wl) {
	mp_free(wl->wl_params);

	mp_free(wl);
}

/**
 * wl_free - free memory for chain of workload objects
 * */
void wl_free_all(workload_t* wl_head) {
	workload_t* wl = wl_head;

	while(wl) {
		wl_head = wl;
		wl = wl->wl_next;

		wl_free(wl_head);
	}
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
			wl_free_all(wl_head);

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

workload_t* json_workload_proc(JSONNODE* node) {
	JSONNODE_ITERATOR i_mod = json_find(node, "module");
	JSONNODE_ITERATOR i_params = json_find(node, "params");
	JSONNODE_ITERATOR i_end = json_end(node);

	workload_t* wl = NULL;
	module_t* mod = NULL;

	tsload_module_t* tmod = NULL;

	char* wl_name = json_name(node);
	char* mod_name = NULL;

	wlp_descr_t* bad_param;

	int ret;

	if(strlen(wl_name) == 0) {
		logmsg(LOG_WARN, "Failed to parse workload, no name is defined");
		goto fail;
	}

	logmsg(LOG_DEBUG, "Parsing workload %s", wl_name);

	if(i_mod == i_end || i_params == i_end) {
		logmsg(LOG_WARN, "Failed to parse workload, missing parameter %s",
				(i_mod == i_end) ? "module" :
				(i_params == i_end) ? "params" : "");
		goto fail;
	}

	if(json_type(*i_mod) != JSON_STRING) {
		logmsg(LOG_WARN, "Expected that module is JSON_STRING");
		goto fail;
	}

	if(json_type(*i_params) != JSON_NODE) {
		logmsg(LOG_WARN, "Expected that params is JSON_NODE");
		goto fail;
	}

	mod_name = json_as_string(*i_mod);
	mod = mod_search(mod_name);
	json_free(mod_name);

	if(mod == NULL) {
		logmsg(LOG_WARN, "Invalid module name %s", mod_name);
		goto fail;
	}

	assert(mod->mod_helper != NULL);
	tmod = (tsload_module_t*) mod->mod_helper;

	wl = wl_create(wl_name, mod);

	ret = json_wlparam_proc_all(*i_params, tmod->mod_params, wl->wl_params, &bad_param);

	if(ret != WLPARAM_JSON_OK) {
		/*FIXME: error handling*/
		goto fail;
	}

	json_free(wl_name);

	return wl;

fail:
	json_free(wl_name);

	if(wl)
		wl_free(wl);

	return NULL;
}
