/*
 * threadpool.h
 *
 *  Created on: Nov 19, 2012
 *      Author: myaut
 */

#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include <threads.h>
#include <pthread.h>

#define TPNAMELEN		64
#define TPMAXTHREADS 	128

#define CONTROL_TID		-1
#define WORKER_TID		0

typedef struct thread_pool {
	unsigned tp_num_threads;
	char tp_name[TPNAMELEN];

	uint64_t tp_quantum;			/**< Dispatcher's quantum duration (in ns)*/
	uint64_t tp_time;				/**< Last time dispatcher had woken up (in ns)*/

	thread_t  tp_ctl_thread;		/**< Dispatcher thread*/
	thread_t* tp_work_threads;		/**< Worker threads*/

	pthread_mutex_t	tp_mutex;
	pthread_cond_t  tp_cv;			/**< Mutex and cond. var used to wake up
										 worker threads when next quantum starts */
} thread_pool_t;

thread_pool_t* tp_create(unsigned num_threads, const char* name, uint64_t quantum);

void create_default_tp();
void destroy_default_tp();

#endif /* THREADPOOL_H_ */
