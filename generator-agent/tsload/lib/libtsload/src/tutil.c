/*
 * tutil.c
 *
 *  Created on: 25.11.2012
 *      Author: myaut
 */

#include <threads.h>
#include <defs.h>

#include <string.h>

void event_init(thread_event_t* event, const char* name) {
	pthread_mutex_init(&event->te_mutex, NULL);
	pthread_cond_init (&event->te_cv, NULL);

	strncpy(event->te_name, name, TEVENTNAMELEN);
}

void event_wait(thread_event_t* event) {
	thread_t* t = t_self();

	if(t != NULL) {
		gettimeofday(&t->t_block_time, NULL);
		t->t_state = TS_WAITING;
		t->t_block_event = event;
	}

	pthread_mutex_lock(&event->te_mutex);
	pthread_cond_wait(&event->te_cv, &event->te_mutex);
	pthread_mutex_unlock(&event->te_mutex);

	if(t != NULL) {
		t->t_state = TS_RUNNABLE;
		t->t_block_event = NULL;
	}
}

void event_notify_one(thread_event_t* event) {
	pthread_mutex_lock(&event->te_mutex);
    pthread_cond_signal(&event->te_cv);
    pthread_mutex_unlock(&event->te_mutex);
}

void event_notify_all(thread_event_t* event) {
    pthread_mutex_lock(&event->te_mutex);
    pthread_cond_broadcast(&event->te_cv);
    pthread_mutex_unlock(&event->te_mutex);
}

void event_notify_thread(thread_t* t) {
	if(t->t_block_event) {
		event_notify_all(t->t_block_event);
	}
}

void mutex_init(thread_mutex_t* mutex, const char* name) {
	pthread_mutex_init(&mutex->tm_mutex, NULL);

	strncpy(mutex->tm_name, name, TEVENTNAMELEN);

	mutex->tm_is_locked = FALSE;
}

void mutex_lock(thread_mutex_t* mutex) {
	thread_t* t = NULL;

	mutex->tm_is_locked = TRUE;

	t = t_self();

	if(t != NULL) {
		gettimeofday(&t->t_block_time, NULL);
		t->t_state = TS_LOCKED;
		t->t_block_mutex = mutex;
	}

	pthread_mutex_lock(&mutex->tm_mutex);

	if(t != NULL) {
		t->t_state = TS_RUNNABLE;
		t->t_block_mutex = NULL;
	}
}

void mutex_unlock(thread_mutex_t* mutex) {
	pthread_mutex_unlock(&mutex->tm_mutex);

	mutex->tm_is_locked = FALSE;
}
