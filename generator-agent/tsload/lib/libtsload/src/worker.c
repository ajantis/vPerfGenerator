/*
 * worker.c
 *
 *  Created on: 04.12.2012
 *      Author: myaut
 */

#define LOG_SOURCE "worker"
#include <log.h>

#include <threads.h>
#include <threadpool.h>

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
