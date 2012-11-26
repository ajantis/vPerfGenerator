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

#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/time.h>

/* Thread hash map. Maps pthread to our thread */
static thread_t* t_hash_heads[THASHSIZE];

static unsigned th_hash_tid(pthread_t tid) {
	unsigned hash = 0;

	while(tid != 0) {
		hash += tid & THAHSMASK;
		tid = tid >> THASHSHIFT;
	}

	return hash % THASHSIZE;
}

static unsigned th_hash_thread(void* object) {
	thread_t* t = (thread_t*) object;
	return th_hash_tid(t->t_thread);
}

static unsigned th_hash_key(void* key) {
	pthread_t* tid = (pthread_t*) key;
	return th_hash_tid(*tid);
}

static void* th_next(void* obj) {
	thread_t* t = (thread_t*) obj;

	return t->t_next;
}

static void th_set_next(void* obj, void* next) {
	thread_t* t = (thread_t*) obj;
	thread_t* t_hext = (thread_t*) next;

	t->t_next = t_hext;
}

static int th_compare(void* obj, void* key) {
	thread_t* t = (thread_t*) obj;
	pthread_t* tid = (pthread_t*) key;

	if(pthread_equal(t->t_thread, *tid))
		return 0;

	return 1;
}

static hashmap_t thread_hash_map = {
	.hm_size = THASHSIZE,
	.hm_heads = (void**) t_hash_heads,

	.hm_hash_object = th_hash_thread,
	.hm_hash_key = th_hash_key,
	.hm_next = th_next,
	.hm_set_next = th_set_next,
	.hm_compare = th_compare
};

/*End of thread_hash_map declaration*/

/**
 * returns pointer to current thread
 *
 * may return NULL in two cases:
 *    if current thread is not under management of threads (pthreads in particular)
 *    called when thread_hash_map.hm_mutex is already held
 *
 * Used to monitor mutex/event deadlock and starvation (see tutil.c)
 *  */
thread_t* t_self() {
	pthread_t tid = pthread_self();

	/* XXX: oh, that's really awkward */

	/* Sometimes mutex_lock called from context of thread routines,
	 * i.e. when logging in t_dump_thread or hash_map_find (see below :) )
	 *
	 * If this happen, hash_map_find tries to lock hm->hm_mutex
	 * and this causes deadlock. So return NULL if hm->hm_mutex is locking*/

	if(thread_hash_map.hm_mutex.tm_is_locked)
		return NULL;

	return hash_map_find(&thread_hash_map, &tid);
}

/*
 * Generate next thread id
 * */
int t_assign_id() {
	static int tid = 0;

	return ++tid;
}

/*
 * Initialize and run thread
 * */
void t_init(thread_t* thread, void* arg, const char* name, void* (*start)(void*)) {
	thread->t_id = t_assign_id();
	thread->t_local_id = 0;

	strncpy(thread->t_name, name, TNAMELEN);

	pthread_attr_init(&thread->t_attr);
	pthread_attr_setdetachstate(&thread->t_attr, PTHREAD_CREATE_JOINABLE);

	thread->t_event = NULL;
	thread->t_arg = arg;

	thread->t_state = TS_INITIALIZED;

	pthread_create(&thread->t_thread,
			       &thread->t_attr,
			       start, (void*) thread);
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
 * Attach event to thread. When thread will finish it notifies us thru this event
 * if thread state is TS_DEAD, does nothing
 * */
void t_attach(thread_t* thread, thread_event_t* event) {
	if(thread->t_state != TS_DEAD)
		thread->t_event = event;
}

/*
 * Wait until thread finishes (should be called with t_attach)
 * */
void t_join(thread_t* thread) {
	if(thread->t_event)
		event_wait(thread->t_event);
}

/*
 * Gives how many time thread spent on event/mutex
 *
 * @param tv - time interwal
 * */
void t_get_wait_time(thread_t* t, struct timeval* tv) {
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

static void t_dump_thread(void* object) {
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
}

void t_dump_threads() {
	logmsg(LOG_DEBUG, "%5s %5s %32s %5s %s", "TID", "STID", "NAME", "STATE", "BLOCK");

	hash_map_walk(&thread_hash_map, t_dump_thread);
}

int threads_init(void) {
	hash_map_init(&thread_hash_map, "thread_hash_map");

	return 0;
}
