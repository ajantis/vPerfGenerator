/*
 * threads.h
 *
 *  Created on: 20.12.2012
 *      Author: myaut
 */

#ifndef PLAT_POSIX_THREADS_H_
#define PLAT_POSIX_THREADS_H_

#include <pthread.h>

typedef struct {
	pthread_mutex_t	te_mutex;
	pthread_cond_t  te_cv;
} plat_thread_event_t;

typedef struct {
	pthread_mutex_t	tm_mutex;
} plat_thread_mutex_t;

typedef struct {
	pthread_attr_t 	t_attr;
	pthread_t		t_thread;
} plat_thread_t;

typedef struct {
	pthread_key_t 	tk_key;
} plat_thread_key_t;

#endif /* PLAT_POSIX_THREADS_H_ */
