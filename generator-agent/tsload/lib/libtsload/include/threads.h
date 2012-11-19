/*
 * threads.h
 *
 *  Created on: 10.11.2012
 *      Author: myaut
 */

#ifndef THREADS_H_
#define THREADS_H_

#include <stdint.h>
#include <pthread.h>

#define THREAD_ENTRY(arg, arg_type, arg_name) 		\
	thread_t* thread = (thread_t*) arg;				\
	arg_type* arg_name = (arg_type*) thread->t_arg;	\

#define THREAD_EXIT(arg)			\
	t_notify(thread);				\
	return arg;

typedef struct {
	int					t_id;

	pthread_attr_t 		t_attr;
	pthread_t		    t_thread;

	/* When thread finishes, it notifies parent thread thru t_cv*/
	pthread_mutex_t*	t_mutex;
	pthread_cond_t*  	t_cv;

	void*				t_arg;
} thread_t;

void pt_init(pthread_mutex_t* mtx, pthread_cond_t* cv);
void pt_wait(pthread_mutex_t* mtx, pthread_cond_t* cv);
void pt_notify_one(pthread_mutex_t* mtx, pthread_cond_t* cv);
void pt_notify_all(pthread_mutex_t* mtx, pthread_cond_t* cv);

void t_init(thread_t* thread, void* arg, int tid, void* (*start)(void*));

static void t_attach(thread_t* thread, pthread_mutex_t* mtx, pthread_cond_t* cv) {
	thread->t_mutex = mtx;
	thread->t_cv = cv;
}

static void t_join(thread_t* thread) {
	if(thread->t_mutex && thread->t_cv)
		pt_wait(thread->t_mutex, thread->t_cv);
}

static void t_notify(thread_t* thread) {
	if(thread->t_mutex && thread->t_cv)
		pt_notify_all(thread->t_mutex, thread->t_cv);
}

int t_create_id();

#endif /* THREADPOOL_H_ */
