/*
 * threads.c
 *
 *  Created on: 10.11.2012
 *      Author: myaut
 */

#define LOG_SOURCE "thread"
#include <log.h>

#include <threads.h>

void* control_thread(void* arg) {
	thread_t* t = (thread_t*) arg;
	thread_pool_t* tp = t->t_pool;

	logmsg(LOG_DEBUG, "Started control thread (tpool: %s)", tp->tp_name);

	return arg;
}

void* worker_thread(void* arg) {
	thread_t* t = (thread_t*) arg;
	thread_pool_t* tp = t->t_pool;

	logmsg(LOG_DEBUG, "Started worker thread #%d (tpool: %s)", t->t_id, tp->tp_name);

	return arg;
}
