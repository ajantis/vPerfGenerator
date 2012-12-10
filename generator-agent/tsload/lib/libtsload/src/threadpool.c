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

static void tp_create_worker(thread_pool_t* tp, int tid) {
	tp_worker_t* worker = tp->tp_workers + tid;

	mutex_init(&worker->w_rq_mutex, "work-%s-%d", tp->tp_name, tid);

	list_head_init(&worker->w_requests, "rqs-%s-%d", tp->tp_name, tid);

	worker->w_tp = tp;

	t_init(&worker->w_thread, (void*) worker, worker_thread,
			"work-%s-%d", tp->tp_name, tid);
	worker->w_thread.t_local_id = WORKER_TID + tid;
}

static void tp_destroy_worker(thread_pool_t* tp, int tid) {
	tp_worker_t* worker = tp->tp_workers + tid;

	t_destroy(&worker->w_thread + tid);

	mutex_destroy(&worker->w_rq_mutex);
}

thread_pool_t* tp_create(unsigned num_threads, const char* name, uint64_t quantum) {
	thread_pool_t* tp = NULL;
	int tid;

	if(num_threads > TPMAXTHREADS) {
		logmsg(LOG_WARN, "Failed to create thread_pool %s: maximum %d threads allowed (%d requested)",
				name, TPMAXTHREADS, num_threads);

		return NULL;
	}

	tp = (thread_pool_t*) mp_malloc(sizeof(thread_pool_t));

	strncpy(tp->tp_name, name, TPNAMELEN);

	tp->tp_num_threads = num_threads;
	tp->tp_workers = mp_malloc(num_threads * sizeof(tp_worker_t));

	tp->tp_time	   = 0ll;	   /*Time is set by control thread*/
	tp->tp_quantum = quantum;

	tp->tp_is_dead = FALSE;
	tp->tp_wl_changed = FALSE;

	tp->tp_wl_count = 0;

	/*Initialize objects*/
	list_head_init(&tp->tp_wl_head, "tp-%s", name);
    event_init(&tp->tp_event, "tp-%s", name);
    mutex_init(&tp->tp_mutex, "tp-%s", name);

	/*Create threads*/
	t_init(&tp->tp_ctl_thread, (void*) tp, control_thread,
			"tp-ctl-%s", name);
	tp->tp_ctl_thread.t_local_id = CONTROL_TID;

	for(tid = 0; tid < num_threads; ++tid) {
		tp_create_worker(tp, tid);
	}

	logmsg(LOG_INFO, "Created thread pool %s with %d threads", name, num_threads);

	return tp;
}

void tp_destroy(thread_pool_t* tp) {
	int tid;

	tp->tp_is_dead = TRUE;

	/* Notify workers that we are done */
	event_notify_all(&tp->tp_event);

	for(tid = 0; tid < tp->tp_num_threads; ++tid) {
		tp_destroy_worker(tp, tid);
	}

	t_destroy(&tp->tp_ctl_thread);

	event_destroy(&tp->tp_event);
	mutex_destroy(&tp->tp_mutex);

	mp_free(tp->tp_workers);
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

	list_add_tail(&wl->wl_tp_node, &tp->tp_wl_head);
	++tp->tp_wl_count;
	tp->tp_wl_changed = TRUE;

	mutex_unlock(&tp->tp_mutex);
}

void tp_detach_nolock(thread_pool_t* tp, struct workload* wl) {
	list_del(&wl->wl_tp_node);
	--tp->tp_wl_count;
	tp->tp_wl_changed = TRUE;
}

/**
 * Remove workload from thread pool list
 */
void tp_detach(thread_pool_t* tp, struct workload* wl) {
	assert(wl->wl_tp == tp);

	mutex_lock(&tp->tp_mutex);

	tp_detach_nolock(tp, wl);

	mutex_unlock(&tp->tp_mutex);
}

/**
 * Helper for tp_distribute_requests
 *
 * Distributes num requests across array elements randomly
 *
 * @param def default number of requests per array item
 * @param num extra number of requests
 * @param len length of array
 * @param array pointer to array
 * */
static void distribute_requests(int def, int num, int len, int* array) {
	int i = 0, bit = 0;
	long rand_value = rand();

	/*Initialize array*/
	for(i = 0; i < len; ++i) {
		array[i] = def;
	}

	while(num > 0) {
		if(rand_value && (1 << bit)) {
			/* We found one, attach request*/
			--num;
			array[bit % len]++;
		}

		/* We exhausted rand_value, regenerate it*/
		if((1 << bit) >= RAND_MAX) {
			bit = 0;
			rand_value = rand();
		}

		bit++;
	}
}

/**
 * Distribute requests across threadpool's workers
 *
 * I.e. we have to distribute 11 requests across 4 workers
 * First of all, it attaches 2 rqs (10 / 4 = 2 for integer division)
 * to each worker then distributes 3 rqs randomly
 *
 * @note Called with w_rq_mutex held for all workers*/
void tp_distribute_requests(workload_step_t* step, thread_pool_t* tp) {
	unsigned rq_per_worker = step->wls_rq_count / tp->tp_num_threads;
	unsigned extra_rqs = step->wls_rq_count % tp->tp_num_threads;

	int* num_rqs = (int*) mp_malloc(tp->tp_num_threads * sizeof(int));
	request_t* rq;

	int tid;

	distribute_requests(rq_per_worker, extra_rqs, tp->tp_num_threads, num_rqs);

	for(tid = 0; tid < tp->tp_num_threads; ++tid) {
		while(num_rqs[tid] >= 0) {
			rq = wl_create_request(step->wls_workload, tid);

			list_add_tail(&rq->rq_node, &tp->tp_workers[tid].w_requests);

			--num_rqs[tid];
		}
	}

	mp_free(num_rqs);
}

int tp_init(void) {
	/*FIXME: default pool should have threads number num_of_phys_cores*/
	default_pool = tp_create(4, DEFAULT_TP_NAME, 250 * MS);

	return 0;
}

void tp_fini(void) {
	tp_destroy(default_pool);
}
