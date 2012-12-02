/*
 * workload.h
 *
 *  Created on: Nov 19, 2012
 *      Author: myaut
 */

#ifndef WORKLOAD_H_
#define WORKLOAD_H_

#include <threads.h>
#include <threadpool.h>
#include <wlparam.h>
#include <modules.h>
#include <modtsload.h>

#define WLHASHSIZE	8
#define WLHASHMASK	(WLHASHSIZE - 1)
#define WLNAMELEN	64

/**
 * workloads
 * */

typedef enum {
	WLS_CONFIGURING,
	WLS_SUCCESS,
	WLS_FAIL
} wl_status_t;

typedef struct workload {
	char 			 wl_name[WLNAMELEN];

	module_t*		 wl_mod;
	tsload_module_t* wl_ts_mod;

	thread_pool_t*	 wl_tp;
	void*			 wl_params;

	thread_t		 wl_cfg_thread;

	int 			 wl_is_configured;

	struct workload* wl_next;			/**<next in workload chain*/
	struct workload* wl_hm_next;		/**<next in workload hashmap*/
} workload_t;

void wl_notify(workload_t* wl, wl_status_t status, int done, char* format, ...) ;

void wl_config(workload_t* wl);
void wl_unconfig(workload_t* wl);

#ifndef NO_JSON
#include <libjson.h>

workload_t* json_workload_proc_all(JSONNODE* node);
workload_t* json_workload_proc(JSONNODE* node);
#endif

#endif /* WORKLOAD_H_ */
