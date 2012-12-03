/*
 * threads.h
 *
 *  Created on: 10.11.2012
 *      Author: myaut
 */

#ifndef THREADS_H_
#define THREADS_H_

#include <stdint.h>

#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>

#define TEVENTNAMELEN	16
#define TMUTEXNAMELEN	16
#define TNAMELEN		32

#define THASHSHIFT		4
#define THASHSIZE		(1 << THASHSHIFT)
#define THAHSMASK		(THASHSIZE - 1)

/**
 * Threads engine. Working with pthread
 */

/**
 * THREAD_ENTRY macro should be inserted into beginning of thread function
 *
 * declares thread_t* thread and argtype_t* arg_name (private argument passed to t_init)
 *
 * i.e.:
 * void* worker_thread(void* arg) {
 *    THREAD_ENTRY(arg, workload_t, wl)
 *    ...
 * */
#define THREAD_ENTRY(arg, arg_type, arg_name) 			\
	thread_t* thread = t_post_init((thread_t*) arg);	\
	arg_type* arg_name = (arg_type*) thread->t_arg;

/*
 * THREAD_END and THREAD_FINISH are used as thread's epilogue:
 *
 * THREAD_END:
 *     De-initialization code goes here
 * 	   THREAD_FINISH(arg);
 * }
 * */
#define THREAD_END 		_l_thread_exit
#define THREAD_FINISH(arg)			\
	thread->t_state = TS_DEAD;		\
	t_exit(thread);					\
	return arg;

/*
 * THREAD_EXIT - prematurely exit from thread (works only in main function of thread)
 * */
#define THREAD_EXIT()				\
	goto _l_thread_exit

/*
 * Thread states:
 *
 *       |
 *     t_init              +----------------------------------------------+
 *       |                 |                                              |
 * TS_INITIALIZED --> TS_RUNNABLE --+--event_wait--> TS_WAITING ----------+
 *                         |        \                                     |
 *                       t_exit      \--mutex_lock--> TS_LOCKED ----------+
 *                         |
 *                         v
 *                      TS_DEAD
 * */
typedef enum {
	TS_INITIALIZED,
	TS_RUNNABLE,
	TS_WAITING,
	TS_LOCKED,
	TS_DEAD
} thread_state_t;

typedef struct {
	char			te_name[TEVENTNAMELEN];

	pthread_mutex_t	te_mutex;
	pthread_cond_t  te_cv;

} thread_event_t;

typedef struct {
	char 			tm_name[TMUTEXNAMELEN];

	pthread_mutex_t	tm_mutex;

#ifdef TS_LOCK_DEBUG
	/*See t_self() comment*/
	int 			tm_is_locked;
#endif
} thread_mutex_t;

typedef struct thread {
	thread_state_t	t_state;

	int				t_id;
	int				t_local_id;

	/*FIXME: only for linux*/
	pid_t			t_system_id;

	char 			t_name[TNAMELEN];

	pthread_attr_t 	t_attr;
	pthread_t		t_thread;

	/* When thread finishes, it notifies parent thread thru t_cv*/
	thread_event_t*	t_event;

	void*			t_arg;

#ifdef TS_LOCK_DEBUG
	struct timeval	t_block_time;
	thread_event_t*	t_block_event;
	thread_mutex_t* t_block_mutex;
#endif

	struct thread*	t_next;			/*< Next thread in global thread list*/
	struct thread*	t_pool_next;	/*< Next thread in pool*/
} thread_t;

void mutex_init(thread_mutex_t* mutex, const char* name);
void mutex_lock(thread_mutex_t* mutex);
void mutex_unlock(thread_mutex_t* mutex);
void mutex_destroy(thread_mutex_t* mutex);

void event_init(thread_event_t* event, const char* name);
void event_wait_unlock(thread_event_t* event, thread_mutex_t* mutex);
void event_wait(thread_event_t* event);
void event_notify_one(thread_event_t* event);
void event_notify_all(thread_event_t* event);
void event_destroy(thread_event_t* event);

thread_t* t_self();

void t_init(thread_t* thread, void* arg, const char* name, void* (*start)(void*));
thread_t* t_post_init(thread_t* t);
void t_exit(thread_t* t);
void t_destroy(thread_t* thread);

void t_join(thread_t* thread, thread_event_t* event);

int threads_init(void);
void threads_fini(void);

void t_dump_threads();

#endif /* THREADPOOL_H_ */
