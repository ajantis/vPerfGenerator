/*
 * workload.c
 *
 *  Created on: Nov 19, 2012
 *      Author: myaut
 */

#define LOG_SOURCE "workload"
#include <log.h>

#include <modules.h>
#include <workload.h>

#include <libjson.h>

workload_t* wl_create(const char* name, module_t* mod) {
	workload_t* wl = (workload_t*) malloc(sizeof(workload_t));

	if(wl == NULL)
		return NULL;

	strncpy(wl->wl_name, name, WLNAMELEN);
	wl->wl_mod = mod;

	wl->wl_tp = NULL;
	wl->wl_params = malloc(mod->mod_params_size);

	return wl;
}

void wl_free(workload_t* wl) {
	free(wl->wl_params);

	free(wl);
}

workload_t* json_workload_proc_all(JSONNODE* node) {
	JSONNODE_ITERATOR iter = json_begin(node),
			          end = json_end(node);

	while(iter != end) {
		json_workload_proc(*iter);

		++iter;
	}

	return NULL;
}

workload_t* json_workload_proc(JSONNODE* node) {
	JSONNODE_ITERATOR i_mod = json_find(node, "module");
	JSONNODE_ITERATOR i_params = json_find(node, "params");

	workload_t* wl = NULL;
	module_t* mod = NULL;
	char* wl_name = json_name(node);
	char* mod_name = NULL;

	if(strlen(wl_name) == 0) {
		logmsg(LOG_WARN, "Failed to parse workload, no name is defined");

		return NULL;
	}

	logmsg(LOG_INFO, "Parsing workload %s", wl_name);

	if(!i_mod || !i_params) {
		logmsg(LOG_WARN, "Failed to parse workload, missing parameter %s",
				(!i_mod) ? "module" :
				(!i_params) ? "params" : "");
		return NULL;
	}

	if(json_type(*i_mod) != JSON_STRING) {
		logmsg(LOG_WARN, "Expected that module is JSON_STRING");
		return NULL;
	}

	if(json_type(*i_params) != JSON_NODE) {
		logmsg(LOG_WARN, "Expected that params is JSON_NODE");
		return NULL;
	}

	mod_name = json_as_string(*i_mod);
	mod = mod_search(mod_name);

	if(mod == NULL) {
		logmsg(LOG_WARN, "Invalid module name %s", mod_name);
		return NULL;
	}

	wl = wl_create(wl_name, mod);

	json_wlparam_proc_all(*i_params, mod->mod_params, wl->wl_params);

	return wl;
}

