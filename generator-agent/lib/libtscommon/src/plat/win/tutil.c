/*
 * plat.c
 *
 *  Created on: 20.12.2012
 *      Author: myaut
 */

#include <defs.h>

#include <plat/win/threads.h>

/* Mutexes */

PLATAPI void plat_mutex_init(plat_thread_mutex_t* mutex, int recursive) {

}

PLATAPI void plat_mutex_lock(plat_thread_mutex_t* mutex) {

}

PLATAPI void plat_mutex_unlock(plat_thread_mutex_t* mutex) {

}

PLATAPI void plat_mutex_destroy(plat_thread_mutex_t* mutex) {

}

/* Events */

PLATAPI void plat_event_init(plat_thread_event_t* event) {

}

PLATAPI void plat_event_wait_unlock(plat_thread_event_t* event, plat_thread_mutex_t* mutex) {

}

PLATAPI void plat_event_notify_one(plat_thread_event_t* event) {

}

PLATAPI void plat_event_notify_all(plat_thread_event_t* event) {

}

PLATAPI void plat_event_destroy(plat_thread_event_t* event) {

}

/* Keys */
PLATAPI void plat_tkey_init(plat_thread_key_t* key, void (*destructor)(void* key)) {

}

PLATAPI void plat_tkey_destroy(plat_thread_key_t* key) {

}

PLATAPI void plat_tkey_set(plat_thread_key_t* key, void* value) {

}

PLATAPI void* plat_tkey_get(plat_thread_key_t* key) {

}
