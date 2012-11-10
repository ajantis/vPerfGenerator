/*
 * threadpool.c
 *
 *  Created on: 10.11.2012
 *      Author: myaut
 */


#define LOG_SOURCE "tpool"
#include <log.h>

#include <threads.h>
#include <defs.h>

#include <string.h>
#include <stdlib.h>

#include <pthread.h>

thread_pool_t* default_pool = NULL;

/*From threads.c*/
void* control_thread(void*);
void* worker_thread(void*);

void t_init(thread_t* thread, thread_pool_t* tp, int tid, void* (*start)()) {
	thread->t_id = tid;
	thread->t_pool = tp;

	pthread_attr_init(&thread->t_attr);
	pthread_attr_setdetachstate(&thread->t_attr, PTHREAD_CREATE_JOINABLE);

	pthread_create(&thread->t_thread,
			       &thread->t_attr,
			       start, (void*) thread);
}

thread_pool_t* tp_create(unsigned num_threads, const char* name, uint64_t quantum) {
	thread_pool_t* tp = NULL;
	int tid;

	if(num_threads > TPMAXTHREADS) {
		logmsg(LOG_WARN, "Failed to create thread_pool %s: no more %d threads allowed (%d requested)",
				name, TPMAXTHREADS, num_threads);

		return NULL;
	}

	tp = (thread_pool_t*) malloc(sizeof(thread_pool_t));

	strncpy(tp->tp_name, name, TPNAMELEN);

	tp->tp_num_threads = num_threads;
	tp->tp_work_threads = malloc(num_threads * sizeof(thread_t));

	tp->tp_time	   = 0ll;	   /*Time is set by control thread*/
	tp->tp_quantum = quantum;

	pthread_mutex_init(&tp->tp_mutex, NULL);
	pthread_cond_init (&tp->tp_cv, NULL);

	/*Create threads*/
	t_init(&tp->tp_ctl_thread, tp, CONTROL_TID, control_thread);

	for(tid = 0; tid < num_threads; ++tid)
		t_init(tp->tp_work_threads + tid, tp, tid + WORKER_TID, worker_thread);

	logmsg(LOG_INFO, "Created thread pool %s with %d threads", name, num_threads);

	return tp;
}

void create_default_tp() {
	default_pool = tp_create(4, "[DEFAULT]", 50 * MS);
}
