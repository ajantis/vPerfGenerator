/*
 * threadpool.h
 *
 *  Created on: Nov 19, 2012
 *      Author: myaut
 */

#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include <threads.h>

#define TPNAMELEN		64
#define TPMAXTHREADS 	128

#define DEFAULT_TP_NAME	"[DEFAULT]"

#define CONTROL_TID		-1
#define WORKER_TID		0

struct workload;

typedef struct thread_pool {
	unsigned tp_num_threads;
	char tp_name[TPNAMELEN];

	int tp_is_dead;

	uint64_t tp_quantum;			/**< Dispatcher's quantum duration (in ns)*/
	uint64_t tp_time;				/**< Last time dispatcher had woken up (in ns)*/

	thread_t  tp_ctl_thread;		/**< Dispatcher thread*/
	thread_t* tp_work_threads;		/**< Worker threads*/

	thread_event_t tp_event;		/**< Used to wake up worker threads when next quantum starts */

	thread_mutex_t tp_mutex;		/**< Protects workload list*/
	struct workload* tp_wl_head;
	struct workload* tp_wl_tail;
} thread_pool_t;

thread_pool_t* tp_create(unsigned num_threads, const char* name, uint64_t quantum);

thread_pool_t* tp_search(const char* name);

void tp_attach(thread_pool_t* tp, struct workload* wl);
void tp_detach(thread_pool_t* tp, struct workload* wl);

int tp_init(void);
void tp_fini(void);

#endif /* THREADPOOL_H_ */
