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

/* Events */

void event_init(thread_event_t* event, const char* namefmt, ...) {
	va_list va;

	plat_event_init(&event->te_impl);

	va_start(va, namefmt);
	vsnprintf(event->te_name, TEVENTNAMELEN, namefmt, va);
	va_end(va);
}

void event_wait_unlock(thread_event_t* event, thread_mutex_t* mutex) {
#ifdef TS_LOCK_DEBUG
	thread_t* t = t_self();

	if(t != NULL) {
		t->t_block_time = tm_get_time();
		t->t_state = TS_WAITING;
		t->t_block_event = event;
	}
#endif

	plat_event_wait_unlock(&event->te_impl, &mutex->tm_impl);

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
	plat_event_notify_one(&event->te_impl);
}

void event_notify_all(thread_event_t* event) {
	plat_event_notify_all(&event->te_impl);
}

void event_destroy(thread_event_t* event) {
	plat_event_destroy(&event->te_impl);
}

/* Mutexes */

static void __mutex_init(thread_mutex_t* mutex,
						 int recursive,
					     const char* namefmt, va_list va) {
	plat_mutex_init(&mutex->tm_impl, recursive);

	vsnprintf(mutex->tm_name, TMUTEXNAMELEN, namefmt, va);
}

void mutex_init(thread_mutex_t* mutex, const char* namefmt, ...) {
	va_list va;

	va_start(va, namefmt);
	__mutex_init(mutex, B_FALSE, namefmt, va);
	va_end(va);
}

void rmutex_init(thread_mutex_t* mutex, const char* namefmt, ...) {
	va_list va;

	va_start(va, namefmt);
	__mutex_init(mutex, B_TRUE, namefmt, va);
	va_end(va);
}


void mutex_lock(thread_mutex_t* mutex) {
#ifdef TS_LOCK_DEBUG
	thread_t* t = NULL;

	t = t_self();

	if(t != NULL) {
		t->t_block_time = tm_get_time();
		t->t_state = TS_LOCKED;
		t->t_block_mutex = mutex;
	}
#endif

	plat_mutex_lock(&mutex->tm_impl);

#ifdef TS_LOCK_DEBUG
	if(t != NULL) {
		t->t_state = TS_RUNNABLE;
		t->t_block_mutex = NULL;
	}
#endif
}

void mutex_unlock(thread_mutex_t* mutex) {
	plat_mutex_unlock(&mutex->tm_impl);
}


void mutex_destroy(thread_mutex_t* mutex) {
	plat_mutex_destroy(&mutex->tm_impl);
}

/* Keys */

void tkey_init(thread_key_t* key,
			   const char* namefmt, ...) {
	va_list va;

	plat_tkey_init(&key->tk_impl);

	va_start(va, namefmt);
	vsnprintf(key->tk_name, TKEYNAMELEN, namefmt, va);
	va_end(va);
}

void tkey_destroy(thread_key_t* key) {
	plat_tkey_destroy(&key->tk_impl);
}

void tkey_set(thread_key_t* key, void* value) {
	plat_tkey_set(&key->tk_impl, value);
}

void* tkey_get(thread_key_t* key) {
	return plat_tkey_get(&key->tk_impl);
}
