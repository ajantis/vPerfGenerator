/*
 * threadpool.c
 *
 *  Created on: 10.11.2012
 *      Author: myaut
 */


#define LOG_SOURCE "tpool"
#include <log.h>

#include <mempool.h>
#include <threads.h>
#include <threadpool.h>
#include <defs.h>
#include <workload.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

thread_pool_t* default_pool = NULL;

void* worker_thread(void* arg);
void* control_thread(void* arg);

thread_pool_t* tp_create(unsigned num_threads, const char* name, uint64_t quantum) {
	thread_pool_t* tp = NULL;
	int tid;

	char control_name[TNAMELEN];
	char worker_name[TNAMELEN];
	char event_name[TEVENTNAMELEN];
	char mutex_name[TMUTEXNAMELEN];

	if(num_threads > TPMAXTHREADS) {
		logmsg(LOG_WARN, "Failed to create thread_pool %s: maximum %d threads allowed (%d requested)",
				name, TPMAXTHREADS, num_threads);

		return NULL;
	}

	tp = (thread_pool_t*) mp_malloc(sizeof(thread_pool_t));

	strncpy(tp->tp_name, name, TPNAMELEN);

	tp->tp_num_threads = num_threads;
	tp->tp_work_threads = mp_malloc(num_threads * sizeof(thread_t));

	tp->tp_time	   = 0ll;	   /*Time is set by control thread*/
	tp->tp_quantum = quantum;

	tp->tp_is_dead = FALSE;

	snprintf(event_name, TEVENTNAMELEN, "tp-%s-event", name);
    event_init(&tp->tp_event, event_name);

	/*Create threads*/
    snprintf(control_name, TNAMELEN, "tp-%s-ctl", name);
	t_init(&tp->tp_ctl_thread, (void*) tp, control_name, control_thread);
	tp->tp_ctl_thread.t_local_id = CONTROL_TID;

	for(tid = 0; tid < num_threads; ++tid) {
		snprintf(worker_name, TNAMELEN, "tp-%s-%d", name, tid);
		t_init(tp->tp_work_threads + tid, (void*) tp, worker_name, worker_thread);
		tp->tp_ctl_thread.t_local_id = WORKER_TID + tid;
	}

	snprintf(mutex_name, TMUTEXNAMELEN, "tp-%s-mutex", name);
	mutex_init(&tp->tp_mutex, mutex_name);

	logmsg(LOG_INFO, "Created thread pool %s with %d threads", name, num_threads);

	return tp;
}

void tp_destroy(thread_pool_t* tp) {
	int tid;

	tp->tp_is_dead = TRUE;

	/* Notify workers that we are done */
	event_notify_all(&tp->tp_event);

	for(tid = 0; tid < tp->tp_num_threads; ++tid) {
		t_destroy(tp->tp_work_threads + tid);
	}

	t_destroy(&tp->tp_ctl_thread);

	mp_free(tp);
}

thread_pool_t* tp_search(const char* name) {
	if(strcmp(name, DEFAULT_TP_NAME) == 0)
		return default_pool;

	return NULL;
}

/**
 * Insert workload into thread pool's list
 */
void tp_attach(thread_pool_t* tp, struct workload* wl) {
	assert(wl->wl_tp == tp);

	mutex_lock(&tp->tp_mutex);

	/* FIXME: implement list routines */
	if(tp->tp_wl_head == NULL) {
		tp->tp_wl_head = wl;
	}
	else {
		tp->tp_wl_tail->wl_tp_next = wl;
		tp->tp_wl_tail = wl;
	}

	mutex_unlock(&tp->tp_mutex);
}

/**
 * Remove workload from thread pool list
 */
void tp_detach(thread_pool_t* tp, struct workload* wl) {
	struct workload* prev = NULL;

	assert(wl->wl_tp == tp);

	mutex_lock(&tp->tp_mutex);

	/* FIXME: implement list routines */
	if(tp->tp_wl_head == wl) {
		tp->tp_wl_head = wl->wl_tp_next;
	}
	else {
		prev = tp->tp_wl_head;

		while(prev && prev->wl_tp_next != wl)
			prev = prev->wl_tp_next;

		assert(prev != NULL);

		prev->wl_tp_next = wl->wl_tp_next;

		if(tp->tp_wl_tail == wl) {
			tp->tp_wl_tail = prev;
		}
	}

	mutex_unlock(&tp->tp_mutex);
}

int tp_init(void) {
	default_pool = tp_create(4, DEFAULT_TP_NAME, 50 * MS);
	assert(default_pool == NULL);

	return 0;
}

void tp_fini(void) {
	tp_destroy(default_pool);
}
