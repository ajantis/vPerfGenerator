/*
 * tutil.c
 *
 *  Created on: 25.11.2012
 *      Author: myaut
 */

#include <threads.h>
#include <defs.h>

#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include <errno.h>

void event_init(thread_event_t* event, const char* namefmt, ...) {
	va_list va;

	pthread_mutex_init(&event->te_mutex, NULL);
	pthread_cond_init (&event->te_cv, NULL);

	va_start(va, namefmt);
	vsnprintf(event->te_name, TEVENTNAMELEN, namefmt, va);
	va_end(va);
}

void event_wait_unlock(thread_event_t* event, thread_mutex_t* mutex) {
#ifdef TS_LOCK_DEBUG
	thread_t* t = t_self();

	if(t != NULL) {
		gettimeofday(&t->t_block_time, NULL);
		t->t_state = TS_WAITING;
		t->t_block_event = event;
	}
#endif

	while(pthread_mutex_lock(&event->te_mutex) == EINVAL);

	if(mutex) {
		mutex_unlock(mutex);
	}

	pthread_cond_wait(&event->te_cv, &event->te_mutex);
	pthread_mutex_unlock(&event->te_mutex);

#ifdef TS_LOCK_DEBUG
	if(t != NULL) {
		t->t_state = TS_RUNNABLE;
		t->t_block_event = NULL;
	}
#endif
}

void event_wait(thread_event_t* event) {
	event_wait_unlock(event, NULL);
}

void event_notify_one(thread_event_t* event) {
	while(pthread_mutex_lock(&event->te_mutex) == EINVAL);
    pthread_cond_signal(&event->te_cv);
    pthread_mutex_unlock(&event->te_mutex);
}

void event_notify_all(thread_event_t* event) {
	while(pthread_mutex_lock(&event->te_mutex) == EINVAL);

    pthread_cond_broadcast(&event->te_cv);
    pthread_mutex_unlock(&event->te_mutex);
}

void event_destroy(thread_event_t* event) {
	pthread_mutex_destroy(&event->te_mutex);
	pthread_cond_destroy(&event->te_cv);
}

static void __mutex_init(thread_mutex_t* mutex,
						 pthread_mutexattr_t* attr,
					     const char* namefmt, va_list va) {
	pthread_mutex_init(&mutex->tm_mutex, attr);

	vsnprintf(mutex->tm_name, TMUTEXNAMELEN, namefmt, va);
}

void mutex_init(thread_mutex_t* mutex, const char* namefmt, ...) {
	va_list va;

	va_start(va, namefmt);
	__mutex_init(mutex, NULL, namefmt, va);
	va_end(va);


}

void rmutex_init(thread_mutex_t* mutex, const char* namefmt, ...) {
	va_list va;
	pthread_mutexattr_t mta;

	pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_RECURSIVE);

	va_start(va, namefmt);
	__mutex_init(mutex, &mta, namefmt, va);
	va_end(va);
}


void mutex_lock(thread_mutex_t* mutex) {
#ifdef TS_LOCK_DEBUG
	thread_t* t = NULL;

	t = t_self();

	if(t != NULL) {
		gettimeofday(&t->t_block_time, NULL);
		t->t_state = TS_LOCKED;
		t->t_block_mutex = mutex;
	}
#endif

	while(pthread_mutex_lock(&mutex->tm_mutex) == EINVAL);

#ifdef TS_LOCK_DEBUG
	if(t != NULL) {
		t->t_state = TS_RUNNABLE;
		t->t_block_mutex = NULL;
	}
#endif
}

void mutex_unlock(thread_mutex_t* mutex) {
	pthread_mutex_unlock(&mutex->tm_mutex);
}


void mutex_destroy(thread_mutex_t* mutex) {
	pthread_mutex_destroy(&mutex->tm_mutex);
}
