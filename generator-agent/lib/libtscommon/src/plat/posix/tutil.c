/*
 * plat.c
 *
 *  Created on: 20.12.2012
 *      Author: myaut
 */

#include <defs.h>

#include <errno.h>
#include <plat/posix/threads.h>

#include <assert.h>

/* Mutexes */

PLATAPI void plat_mutex_init(plat_thread_mutex_t* mutex, int recursive) {
	pthread_mutexattr_t mta;

	if(recursive)
		pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_RECURSIVE);
	else
		pthread_mutexattr_settype(&mta, 0);

	pthread_mutex_init(&mutex->tm_mutex, &mta);
}

PLATAPI void plat_mutex_lock(plat_thread_mutex_t* mutex) {
	while(pthread_mutex_lock(&mutex->tm_mutex) == EINVAL);
}

PLATAPI void plat_mutex_unlock(plat_thread_mutex_t* mutex) {
	pthread_mutex_unlock(&mutex->tm_mutex);
}

PLATAPI void plat_mutex_destroy(plat_thread_mutex_t* mutex) {
	pthread_mutex_destroy(&mutex->tm_mutex);
}

/* Events */

PLATAPI void plat_event_init(plat_thread_event_t* event) {
	pthread_mutex_init(&event->te_mutex, NULL);
	pthread_cond_init (&event->te_cv, NULL);
}

PLATAPI void plat_event_wait_unlock(plat_thread_event_t* event, plat_thread_mutex_t* mutex) {
	while(pthread_mutex_lock(&event->te_mutex) == EINVAL);

	if(mutex) {
		plat_mutex_unlock(mutex);
	}

	pthread_cond_wait(&event->te_cv, &event->te_mutex);
	pthread_mutex_unlock(&event->te_mutex);
}

PLATAPI void plat_event_notify_one(plat_thread_event_t* event) {
	while(pthread_mutex_lock(&event->te_mutex) == EINVAL);
    pthread_cond_signal(&event->te_cv);
    pthread_mutex_unlock(&event->te_mutex);
}

PLATAPI void plat_event_notify_all(plat_thread_event_t* event) {
	while(pthread_mutex_lock(&event->te_mutex) == EINVAL);

    pthread_cond_broadcast(&event->te_cv);
    pthread_mutex_unlock(&event->te_mutex);
}

PLATAPI void plat_event_destroy(plat_thread_event_t* event) {
	pthread_mutex_destroy(&event->te_mutex);
	pthread_cond_destroy(&event->te_cv);
}

/* Keys */
PLATAPI void plat_tkey_init(plat_thread_key_t* key) {
	pthread_key_create(&key->tk_key, NULL);
}

PLATAPI void plat_tkey_destroy(plat_thread_key_t* key) {
	pthread_key_delete(key->tk_key);
}

PLATAPI void plat_tkey_set(plat_thread_key_t* key, void* value) {
	pthread_setspecific(key->tk_key, value);
}

PLATAPI void* plat_tkey_get(plat_thread_key_t* key) {
	return pthread_getspecific(key->tk_key);
}
