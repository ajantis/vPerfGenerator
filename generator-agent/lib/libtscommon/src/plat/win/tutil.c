/*
 * plat.c
 *
 *  Created on: 20.12.2012
 *      Author: myaut
 */

#include <defs.h>

#include <plat/win/threads.h>

#include <assert.h>

/* Mutexes */

PLATAPI void plat_mutex_init(plat_thread_mutex_t* mutex, boolean_t recursive) {
	InitializeCriticalSection(&mutex->tm_crit_section);

	/* Recursive mutexes are not supported*/
	assert(!recursive);
}

PLATAPI void plat_mutex_lock(plat_thread_mutex_t* mutex) {
	EnterCriticalSection(&mutex->tm_crit_section);
}

PLATAPI void plat_mutex_unlock(plat_thread_mutex_t* mutex) {
	LeaveCriticalSection(&mutex->tm_crit_section);
}

PLATAPI void plat_mutex_destroy(plat_thread_mutex_t* mutex) {
	DeleteCriticalSection(&mutex->tm_crit_section);
}

/* Events */

PLATAPI void plat_event_init(plat_thread_event_t* event) {
	InitializeCriticalSection(&event->te_crit_section);
	InitializeConditionVariable(&event->te_cond_var);
}

PLATAPI void plat_event_wait_unlock(plat_thread_event_t* event, plat_thread_mutex_t* mutex) {
	EnterCriticalSection(&event->te_crit_section);

	if(mutex) {
		plat_mutex_unlock(mutex);
	}

	SleepConditionVariableCS(&event->te_cond_var, &event->te_crit_section, INFINITE);

	LeaveCriticalSection(&event->te_crit_section);
}

PLATAPI void plat_event_notify_one(plat_thread_event_t* event) {
	EnterCriticalSection(&event->te_crit_section);
	WakeConditionVariable(&event->te_cond_var);
	LeaveCriticalSection(&event->te_crit_section);
}

PLATAPI void plat_event_notify_all(plat_thread_event_t* event) {
	EnterCriticalSection(&event->te_crit_section);
	WakeAllConditionVariable(&event->te_cond_var);
	LeaveCriticalSection(&event->te_crit_section);
}

PLATAPI void plat_event_destroy(plat_thread_event_t* event) {
	DeleteCriticalSection(&event->te_crit_section);
}

/* Keys */
PLATAPI void plat_tkey_init(plat_thread_key_t* key) {
	key->tk_tls = TlsAlloc();

	assert(key->tk_tls != TLS_OUT_OF_INDEXES);
}

PLATAPI void plat_tkey_destroy(plat_thread_key_t* key) {
	TlsFree(key->tk_tls);
}

PLATAPI void plat_tkey_set(plat_thread_key_t* key, void* value) {
	TlsSetValue(key->tk_tls, value);
}

PLATAPI void* plat_tkey_get(plat_thread_key_t* key) {
	return TlsGetValue(key->tk_tls);
}
