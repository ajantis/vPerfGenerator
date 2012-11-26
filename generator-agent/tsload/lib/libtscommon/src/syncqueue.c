/*
 * syncqueue.c
 *
 *  Created on: 27.11.2012
 *      Author: myaut
 */

#include <threads.h>
#include <syncqueue.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Initialize synchronized queue
 *
 * @param sq - queue
 * @param name - name of queue (for debugging purposes)
 */
void squeue_init(squeue_t* sq, const char* name) {
	char sq_event_name[TEVENTNAMELEN];
	char sq_mutex_name[TMUTEXNAMELEN];

	snprintf(sq_event_name, TEVENTNAMELEN, "%s.event", name);
	snprintf(sq_mutex_name, TMUTEXNAMELEN, "%s.mutex", name);

	mutex_init(&sq->sq_mutex, sq_mutex_name);
	event_init(&sq->sq_event, sq_event_name);

	strncpy(sq->sq_name, name, SQUEUENAMELEN);

	sq->sq_head = NULL;
	sq->sq_tail = NULL;
}

/**
 * Extract element from synchronized queue.
 * If no elements on queue, blocks until squeue_push will add an element.
 *
 * May be called from multiple threads simultainously, thread selected
 * in event_notify_one wins.
 *
 * @param sq queue
 *
 * @return element
 */
void* squeue_pop(squeue_t* sq) {
	squeue_el_t* el = NULL;
	void *object = NULL;

retry:
	mutex_lock(&sq->sq_mutex);

	/* Try to extract element from head*/
	el = sq->sq_head;

	if(el == NULL) {
		event_wait_unlock(&sq->sq_event, &sq->sq_mutex);
		goto retry;
	}

	/* Move backward */
	sq->sq_head = el->s_next;

	/* Only one element left and it is head,
	 * clear tail pointer*/
	if(sq->sq_tail == sq->sq_head)
		sq->sq_tail = NULL;

	mutex_unlock(&sq->sq_mutex);

	object = el->s_data;
	free(el);

	return object;
}

/**
 * Add an element to synchronized queue.
 *
 * @param object element
 */
void squeue_push(squeue_t* sq, void* object) {
	squeue_el_t* el = malloc(sizeof(squeue_el_t));
	el->s_data = object;

	mutex_lock(&sq->sq_mutex);

	if(sq->sq_head == NULL) {
		/* Empty queue, notify pop*/
		sq->sq_head = el;

		event_notify_one(&sq->sq_event);
	}
	else if(sq->sq_tail == NULL) {
		/* Only sq_head present, insert el as tail and link head with tail */
		sq->sq_tail = el;
		sq->sq_head->s_next = el;
	}
	else {
		/* Add element and move tail forward */
		sq->sq_tail->s_next = el;
		sq->sq_tail = el;
	}

	mutex_unlock(&sq->sq_mutex);
}
