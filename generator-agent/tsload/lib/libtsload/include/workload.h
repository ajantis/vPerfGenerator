/*
 * workload.h
 *
 *  Created on: Nov 19, 2012
 *      Author: myaut
 */

#ifndef WORKLOAD_H_
#define WORKLOAD_H_

#include <threadpool.h>
#include <wlparam.h>
#include <modules.h>

#define WLNAMELEN	64

/**
 * workloads
 * */

typedef struct workload {
	char 			wl_name[WLNAMELEN];
	module_t*		wl_mod;
	thread_pool_t*	wl_tp;
	void*			wl_params;
	struct workload* wl_next;
} workload_t;

static void wl_config(workload_t* wl) {
	wl->wl_mod->mod_wl_config(wl);
}

static void wl_unconfig(workload_t* wl) {
	wl->wl_mod->mod_wl_unconfig(wl);
}

#ifndef NO_JSON
#include <libjson.h>

workload_t* json_workload_proc_all(JSONNODE* node);
workload_t* json_workload_proc(JSONNODE* node);
#endif

#endif /* WORKLOAD_H_ */
