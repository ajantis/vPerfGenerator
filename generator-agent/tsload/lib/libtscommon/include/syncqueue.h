/*
 * syncqueue.h
 *
 *  Created on: 27.11.2012
 *      Author: myaut
 */

#ifndef SYNCQUEUE_H_
#define SYNCQUEUE_H_

#include <threads.h>

#define SQUEUENAMELEN		24

typedef struct squeue_el {
	struct squeue_el* s_next;

	void* s_data;
} squeue_el_t;

typedef struct squeue {
	thread_mutex_t sq_mutex;
	thread_event_t sq_event;

	squeue_el_t* sq_head;
	squeue_el_t* sq_tail;

	char sq_name[SQUEUENAMELEN];

	int sq_is_destroyed;
} squeue_t;

void squeue_init(squeue_t* sq, const char* namefmt, ...);
void squeue_push(squeue_t* sq, void* object);
void* squeue_pop(squeue_t* sq);
void squeue_destroy(squeue_t* sq, void (*el_free)(void* obj));

#endif /* SYNCQUEUE_H_ */
