/*
 * threadpool.h
 *
 *  Created on: Nov 19, 2012
 *      Author: myaut
 */

#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include <list.h>
#include <tstime.h>
#include <threads.h>
#include <syncqueue.h>

#define TPNAMELEN		64
#define TPMAXTHREADS 	64

#define DEFAULT_TP_NAME	"[DEFAULT]"

#define CONTROL_TID		-1
#define WORKER_TID		0

struct request;
struct workload;
struct thread_pool;
struct workload_step;

typedef struct tp_worker {
	struct thread_pool* w_tp;

	thread_t w_thread;

	thread_mutex_t w_rq_mutex;

	list_head_t w_requests;
} tp_worker_t;

typedef struct thread_pool {
	unsigned tp_num_threads;
	char tp_name[TPNAMELEN];

	int tp_is_dead;

	ts_time_t tp_quantum;			/**< Dispatcher's quantum duration (in ns)*/
	ts_time_t tp_time;				/**< Last time dispatcher had woken up (in ns)*/

	thread_t  tp_ctl_thread;		/**< Dispatcher thread*/
	tp_worker_t* tp_workers;		/**< Worker threads*/

	thread_event_t tp_event;		/**< Used to wake up worker threads when next quantum starts */

	thread_mutex_t tp_mutex;		/**< Protects workload list*/

	squeue_t tp_requests;

	list_head_t	   tp_wl_head;
	int tp_wl_count;
	int tp_wl_changed;
} thread_pool_t;

thread_pool_t* tp_create(unsigned num_threads, const char* name, uint64_t quantum);

thread_pool_t* tp_search(const char* name);

void tp_attach(thread_pool_t* tp, struct workload* wl);
void tp_detach(thread_pool_t* tp, struct workload* wl);

void tp_distribute_requests(struct workload_step* step, thread_pool_t* tp);

int tp_init(void);
void tp_fini(void);

#endif /* THREADPOOL_H_ */
