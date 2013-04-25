/*
 * mutex.c
 *
 *  Created on: Apr 25, 2013
 *      Author: myaut
 */

#include <defs.h>
#include <threads.h>
#include <tstime.h>
#include <atomic.h>

#include <assert.h>

#define TESTS			8

#define NUM_THREADS		4
#define AWAKE_DELAY 	(10 * T_MS)

/**
 * Number of awaken threads
 * */
atomic_t awake = NUM_THREADS;
boolean_t working = B_TRUE;

thread_event_t ev;

/**
 *      1           2                1
 * -----|-----------|----------------|-->		main()
 * ---|----|----------------------|----->		test_event-x()
 *    1    2                      1
 *
 * main()
 * 1. Notifies test_event thread
 * 2. Checks if awake == number of awaken threads
 *
 * test_event-x()
 * 1. Sleeps on event
 * 2. Awakes on event because main() sent notification
 *    awake++
 * */

thread_result_t test_event(thread_arg_t arg) {
	THREAD_ENTRY(arg, void, unused);
	int step = 0;

	while(working) {
		atomic_dec(&awake);
		event_wait(&ev);

		atomic_inc(&awake);
		tm_sleep_milli(AWAKE_DELAY);
	}

THREAD_END:
	THREAD_FINISH(arg);
}

void test_event_notify_one() {
	int test;

	for(test = 0; test < TESTS; ++test) {
		event_notify_one(&ev);
		/* Wait until thread wakes up*/
		tm_sleep_milli(AWAKE_DELAY / 2);

		assert(atomic_read(&awake) == 1);
		tm_sleep_milli(AWAKE_DELAY);
	}
}

void test_event_notify_all() {
	int test;

	for(test = 0; test < TESTS; ++test) {
		event_notify_all(&ev);
		tm_sleep_milli(AWAKE_DELAY / 2);

		assert(atomic_read(&awake) == NUM_THREADS);
		tm_sleep_milli(AWAKE_DELAY);
	}
}

int test_main() {
	thread_t threads[NUM_THREADS];
	int tid;

	threads_init();

	event_init(&ev, "event");

	for(tid = 0; tid < NUM_THREADS; ++tid) {
		t_init(&threads[tid], NULL, test_event,
					"tevent-%d", tid);
	}

	/* Wait until threads initialize themselves */
	tm_sleep_milli(2 * AWAKE_DELAY);

	test_event_notify_one();
	test_event_notify_all();

	/* Stop threads */
	working = B_FALSE;
	event_notify_all(&ev);

	for(tid = 0; tid < NUM_THREADS; ++tid) {
		t_join(&threads[tid]);
		t_destroy(&threads[tid]);
	}

	event_destroy(&ev);
	threads_fini();

	return 0;
}
