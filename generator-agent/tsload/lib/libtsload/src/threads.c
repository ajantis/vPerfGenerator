/*
 * threads.c
 *
 *  Created on: 10.11.2012
 *      Author: myaut
 */

#define LOG_SOURCE "thread"
#include <log.h>

#include <threads.h>
#include <workload.h>

/*Helpers for pthreads*/
void pt_init(pthread_mutex_t* mtx, pthread_cond_t* cv) {
	pthread_mutex_init(mtx, NULL);
	pthread_cond_init (cv, NULL);
}

void pt_wait(pthread_mutex_t* mtx, pthread_cond_t* cv) {
	pthread_mutex_lock(mtx);
	pthread_cond_wait(cv, mtx);
	pthread_mutex_unlock(mtx);
}

void pt_notify_one(pthread_mutex_t* mtx, pthread_cond_t* cv) {
    pthread_mutex_lock(mtx);
    pthread_cond_signal(cv);
    pthread_mutex_unlock(mtx);
}

void pt_notify_all(pthread_mutex_t* mtx, pthread_cond_t* cv) {
    pthread_mutex_lock(mtx);
    pthread_cond_broadcast(cv);
    pthread_mutex_unlock(mtx);
}


void t_init(thread_t* thread, void* arg, int tid, void* (*start)(void*)) {
	thread->t_id = tid;

	pthread_attr_init(&thread->t_attr);
	pthread_attr_setdetachstate(&thread->t_attr, PTHREAD_CREATE_JOINABLE);

	pthread_create(&thread->t_thread,
			       &thread->t_attr,
			       start, (void*) thread);

	thread->t_mutex = NULL;
	thread->t_cv = NULL;

	thread->t_arg = arg;
}

int t_create_id() {
	static int tid = 0;

	return ++tid;
}
