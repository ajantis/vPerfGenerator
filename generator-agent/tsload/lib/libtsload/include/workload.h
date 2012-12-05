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
#include <syncqueue.h>
#include <modtsload.h>
#include <tstime.h>

#define WLHASHSIZE	8
#define WLHASHMASK	(WLHASHSIZE - 1)
#define WLNAMELEN	64

/**
 * Workloads
 * */

/* Keep up to 16 steps in queue */
#define WLSTELSIZE	16
#define WLSTEPMASK	(WLSTELSIZE - 1)

#define RQF_STARTED		0x1
#define RQF_SUCCESS		0x2
#define RQF_ONTIME		0x4

struct workload;

typedef struct request {
	long rq_step;
	int rq_id;

	int rq_thread_id;

	ts_time_t rq_start_tv;
	ts_time_t rq_end_tv;

	struct timeval rq_time;

	int rq_flags;

	struct workload* rq_workload;
	struct request* rq_next;		/* Next request in chain */
} request_t;

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

	thread_t		 wl_cfg_thread;		/**< Thread responsible for configuration*/

	int 			 wl_is_configured;

	int				 wl_current_rq;

	/* Requests queue */
	mutex_t			 wl_rq_mutex;		/**< Mutex that protects wl_requests*/

	long			 wl_current_step;	/**< Id of current step iteration*/
	long			 wl_last_step;		/**< Latest defined step*/

	int		 wl_requests[WLSTELSIZE];	/**< Simple request queue*/
	/* End of requests queue*/

	squeue_t		 wl_steps;			/**< Number of requests per each step*/

	struct workload* wl_next;			/**<next in workload chain*/
	struct workload* wl_tp_next;		/**<next in thread pool wl list*/
	struct workload* wl_hm_next;		/**<next in workload hashmap*/
} workload_t;

void wl_notify(workload_t* wl, wl_status_t status, int done, char* format, ...) ;

void wl_config(workload_t* wl);
void wl_unconfig(workload_t* wl);

void wl_next_step(workload_t* wl);
request_t* wl_create_request(workload_t* wl, int thread_id);

void rq_start(request_t* rq);
void rq_end(request_t* rq);

int wl_init(void);
void wl_fini(void);

#ifndef NO_JSON
#include <libjson.h>

workload_t* json_workload_proc_all(JSONNODE* node);
workload_t* json_workload_proc(JSONNODE* node);
#endif

#endif /* WORKLOAD_H_ */
