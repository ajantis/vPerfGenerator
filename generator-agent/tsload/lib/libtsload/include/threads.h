/*
 * threadpool.h
 *
 *  Created on: 10.11.2012
 *      Author: myaut
 */

#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#define TPNAMELEN		64
#define TPMAXTHREADS 	128

#define CONTROL_TID		-1
#define WORKER_TID		0

#include <stdint.h>
#include <pthread.h>

struct thread_pool;

typedef struct {
	int					t_id;	   /*ID of thread: -1 for dispatcher, 0..N for worker*/

	struct thread_pool* t_pool;

	pthread_attr_t 		t_attr;
	pthread_t		    t_thread;
} thread_t;

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
