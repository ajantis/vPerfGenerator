/*
 * threadpool.c
 *
 *  Created on: 10.11.2012
 *      Author: myaut
 */


#define LOG_SOURCE "tpool"
#include <log.h>

#include <threads.h>
#include <threadpool.h>
#include <defs.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

thread_pool_t* default_pool = NULL;

/*Thread code*/
void* control_thread(void* arg) {
	THREAD_ENTRY(arg, thread_pool_t, tp);

	logmsg(LOG_DEBUG, "Started control thread (tpool: %s)", tp->tp_name);

THREAD_END:
	THREAD_FINISH(arg);
}

void* worker_thread(void* arg) {
	THREAD_ENTRY(arg, thread_pool_t, tp);

	logmsg(LOG_DEBUG, "Started worker thread #%d (tpool: %s)", thread->t_id, tp->tp_name);

THREAD_END:
	THREAD_FINISH(arg);
}

thread_pool_t* tp_create(unsigned num_threads, const char* name, uint64_t quantum) {
	thread_pool_t* tp = NULL;
	int tid;

	char control_name[TNAMELEN];
	char worker_name[TNAMELEN];
	char event_name[TEVENTNAMELEN];

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

	snprintf(event_name, TEVENTNAMELEN, "tp-%s-event", name);
    event_init(&tp->tp_event, event_name);

	/*Create threads*/
    snprintf(control_name, TNAMELEN, "tp-%s-%s", name, "ctl");
	t_init(&tp->tp_ctl_thread, (void*) tp, control_name, control_thread);
	tp->tp_ctl_thread.t_local_id = CONTROL_TID;

	for(tid = 0; tid < num_threads; ++tid) {
		snprintf(worker_name, TNAMELEN, "tp-%s-%d", name, tid);
		t_init(tp->tp_work_threads + tid, (void*) tp, worker_name, worker_thread);
		tp->tp_ctl_thread.t_local_id = WORKER_TID + tid;
	}

	logmsg(LOG_INFO, "Created thread pool %s with %d threads", name, num_threads);

	return tp;
}

void create_default_tp() {
	default_pool = tp_create(4, "[DEFAULT]", 50 * MS);
}
