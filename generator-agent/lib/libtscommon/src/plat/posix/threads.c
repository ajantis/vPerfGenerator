/*
 * threads.c
 *
 *  Created on: 20.12.2012
 *      Author: myaut
 */

#include <defs.h>
#include <plat/posix/threads.h>

#include <threads.h>

PLATAPI void plat_thread_init(plat_thread_t* thread, void* arg,
		  	  	  	  	  	  void* (*start)(void*)) {

	pthread_attr_init(&thread->t_attr);
	pthread_attr_setdetachstate(&thread->t_attr, PTHREAD_CREATE_JOINABLE);

	pthread_create(&thread->t_thread,
			       &thread->t_attr,
			       start, arg);
}

PLATAPI void plat_thread_destroy(plat_thread_t* thread) {
	pthread_attr_destroy(&thread->t_attr);
	pthread_detach(thread->t_thread);
}

PLATAPI unsigned long plat_gettid() {
	return (unsigned long) pthread_self();
}

PLATAPI void t_eternal_wait(void) {
	select(0, NULL, NULL, NULL, NULL);
}

