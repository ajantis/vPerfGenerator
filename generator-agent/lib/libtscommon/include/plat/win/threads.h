/*
 * threads.h
 *
 *  Created on: 20.12.2012
 *      Author: myaut
 */

#ifndef PLAT_POSIX_THREADS_H_
#define PLAT_POSIX_THREADS_H_

#include <windows.h>

typedef struct {
	HANDLE hEvent;
} plat_thread_event_t;

typedef struct {
	HANDLE hMutex;
} plat_thread_mutex_t;

typedef struct {
	int _dummy;
} plat_thread_t;

typedef struct {
	int _dummy;
} plat_thread_key_t;

#define PLAT_THREAD_EVENT_INITIALIZER 						\
	{ SM_INIT(.hEvent, ((HANDLE) ERROR_INVALID_HANDLE))  }

#define PLAT_THREAD_MUTEX_INITIALIZER 						\
	{ SM_INIT(.hMutex, ((HANDLE) ERROR_INVALID_HANDLE)) }

#define PLAT_THREAD_KEY_INITIALIZER 					\
	{ SM_INIT(.dummy, 0) }

#endif /* PLAT_POSIX_THREADS_H_ */
