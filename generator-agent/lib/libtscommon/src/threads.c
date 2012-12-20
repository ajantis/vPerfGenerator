/*
 * threads.c
 *
 *  Created on: 10.11.2012
 *      Author: myaut
 */

#define LOG_SOURCE "thread"
#include <log.h>

#include <threads.h>
#include <hashmap.h>
#include <defs.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/time.h>

#include <assert.h>

/* Thread hash map. Maps pthread to our thread */
DECLARE_HASH_MAP(thread_hash_map, thread_t, THASHSIZE, t_id, t_next,
	/*hash*/ {
		int id = * (int*) key;

		return id % THASHSIZE;
	},
	/*compare*/ {
		thread_id_t tid1 = * (int*) key1;
		thread_id_t tid2 = * (int*) key2;

		if(tid1 == tid2)
			return FALSE;

		return TRUE;
	});

/*End of thread_hash_map declaration*/

thread_key_t thread_key;

static void thread_key_destructor(void* key) {
	/* threads are freed by their spawners so simply
	 * do nothing*/
}

/**
 * returns pointer to current thread
 *
 * Used to monitor mutex/event deadlock and starvation (see tutil.c)
 *  */
thread_t* t_self() {
	return (thread_t*) tkey_get(&thread_key);
}

/*
 * Generate next thread id
 * */
thread_id_t t_assign_id() {
	static thread_id_t tid = 0;

	return ++tid;
}

/*
 * Initialize and run thread
 * */
void t_init(thread_t* thread, void* arg,
		void* (*start)(void*),
		const char* namefmt, ...) {
	va_list va;

	thread->t_id = t_assign_id();
	thread->t_local_id = 0;

	va_start(va, namefmt);
	vsnprintf(thread->t_name, TNAMELEN, namefmt, va);
	va_end(va);

	thread->t_event = NULL;
	thread->t_arg = arg;

	thread->t_state = TS_INITIALIZED;

	thread->t_next = NULL;
	thread->t_pool_next = NULL;

	thread->t_system_id = 0;

	logmsg(LOG_DEBUG, "Created thread #%d '%s'", thread->t_id, thread->t_name);

	plat_thread_init(&thread->t_impl, (void*) thread, start);
}

/*
 * Post-initialize thread. Called from context of thread: inserts it to
 * hash-map and sets system's thread id
 *
 * @param t - current thread
 * @return t (for THREAD_ENTRY)
 * */
thread_t* t_post_init(thread_t* t) {
	t->t_system_id = syscall(__NR_gettid);
	t->t_state = TS_RUNNABLE;

	hash_map_insert(&thread_hash_map, t);

	tkey_set(&thread_key, (void*) t);

	return t;
}

/*
 * Exit thread. Called from context of thread: notifies thread which is joined to t
 * and remove thread from hash_map
 * */
void t_exit(thread_t* t) {
	t->t_state = TS_DEAD;

	hash_map_remove(&thread_hash_map, t);

	if(t->t_event)
		event_notify_all(t->t_event);
}


/*
 * Wait until thread finishes (should be called with t_attach)
 * */
void t_join(thread_t* thread, thread_event_t* event) {
	if(thread->t_state != TS_DEAD) {
		thread->t_event = event;
		event_wait(thread->t_event);
	}
}

/* Destroy dead thread
 *
 * @note blocks until thread exits from itself!
 * */
void t_destroy(thread_t* thread) {
	if(thread->t_state != TS_DEAD) {
		/*XXX: shouldn't this cause race condition with t_exit?*/
		thread_event_t event;
		event_init(&event, "(t_destroy)");

		t_join(thread, &event);
	}

	hash_map_remove(&thread_hash_map, thread);

	plat_thread_destroy(&thread->t_impl);
}

/*
 * Gives how many time thread spent on event/mutex
 *
 * @param tv - time interwal
 * */
#if 0
void t_get_wait_time(thread_t* t, struct timeval* tv) {
	/*FIXME: Use tstime*/
	gettimeofday(tv, NULL);

	if (t->t_block_time.tv_usec < tv->tv_usec) {
		int nsec = (tv->tv_usec - t->t_block_time.tv_usec) / 1000000 + 1;
		tv->tv_usec -= 1000000 * nsec;
		tv->tv_sec += nsec;
	}
	if (t->t_block_time.tv_usec - tv->tv_usec > 1000000) {
		int nsec = (tv->tv_usec - t->t_block_time.tv_usec) / 1000000;
		tv->tv_usec += 1000000 * nsec;
		tv->tv_sec -= nsec;
	}
}

static int t_dump_thread(void* object, void* arg) {
	const char* t_state_name[] = {
			"INIT",
			"RUNA",
			"WAIT",
			"LOCK",
			"DEAD"
	};
	char t_wait_state[64];
	thread_t* t = (thread_t*) object;
	struct timeval t_wait_tv;

	if(t->t_state == TS_WAITING ||
	   t->t_state == TS_LOCKED) {
		char* w_name = (t->t_state == TS_WAITING)
					  	? t->t_block_event->te_name
						: t->t_block_mutex->tm_name;
		char* w_state = (t->t_state == TS_WAITING)
						? "Waiting"
						: "Locked";

		t_get_wait_time(t, &t_wait_tv);

		snprintf(t_wait_state, 64, "(%s, %s, %ld.%06ld)",
					w_state, w_name,
					t_wait_tv.tv_sec,
					t_wait_tv.tv_usec);
	}
	else {
		strcpy(t_wait_state, "(not-blocked)");
	}

	logmsg(LOG_DEBUG, "%5d %5d %32s %5s %s",
			t->t_id, t->t_system_id,
			t->t_name, t_state_name[t->t_state],
			t_wait_state);

	return HM_WALKER_CONTINUE;
}

void t_dump_threads() {
	logmsg(LOG_DEBUG, "%5s %5s %32s %5s %s", "TID", "STID", "NAME", "STATE", "BLOCK");

	(void) hash_map_walk(&thread_hash_map, t_dump_thread, NULL);
}
#endif

int threads_init(void) {
	tkey_init(&thread_key, thread_key_destructor, "thread_key");

	hash_map_init(&thread_hash_map, "thread_hash_map");

	return 0;
}

void threads_fini(void) {
	hash_map_destroy(&thread_hash_map);

	tkey_destroy(&thread_key);
}
