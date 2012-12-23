/*
 * threads.h
 *
 *  Created on: 10.11.2012
 *      Author: myaut
 */

#ifndef THREADS_H_
#define THREADS_H_

#include <stdint.h>

#include <plat/threads.h>

#include <defs.h>
#include <tstime.h>

#define TEVENTNAMELEN	32
#define TMUTEXNAMELEN	32
#define TKEYNAMELEN 	32
#define TNAMELEN		48

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
	arg_type* arg_name = (arg_type*) thread->t_arg

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
	plat_thread_event_t te_impl;

	char			te_name[TEVENTNAMELEN];
} thread_event_t;

typedef struct {
	plat_thread_mutex_t tm_impl;

	char 			tm_name[TMUTEXNAMELEN];
	int				tm_is_recursive;
} thread_mutex_t;

typedef struct {
	plat_thread_key_t tk_impl;

	char			tk_name[TKEYNAMELEN];
} thread_key_t;

#define THREAD_EVENT_INITIALIZER 							\
	{ SM_INIT(.te_impl, PLAT_THREAD_EVENT_INITIALIZER),		\
	  SM_INIT(.te_name, "\0") }

#define THREAD_MUTEX_INITIALIZER 							\
	{ SM_INIT(.tm_impl, PLAT_THREAD_MUTEX_INITIALIZER),		\
	  SM_INIT(.tm_name, "\0"),								\
	  SM_INIT(.tm_is_recursive, B_FALSE) }

#define THREAD_KEY_INITIALIZER 								\
	{ SM_INIT(.tk_impl, PLAT_THREAD_KEY_INITIALIZER),		\
	  SM_INIT(.tk_name, "\0") }

typedef unsigned 	thread_id_t;

typedef struct thread {
	plat_thread_t	t_impl;

	thread_state_t	t_state;

	thread_id_t  	t_id;
	int				t_local_id;

	unsigned long   t_system_id;

	char 			t_name[TNAMELEN];

	/* When thread finishes, it notifies parent thread thru t_cv*/
	thread_event_t*	t_event;

	void*			t_arg;

#ifdef TS_LOCK_DEBUG
	ts_time_t		t_block_time;
	thread_event_t*	t_block_event;
	thread_mutex_t* t_block_mutex;
#endif

	struct thread*	t_next;			/*< Next thread in global thread list*/
	struct thread*	t_pool_next;	/*< Next thread in pool*/
} thread_t;

LIBEXPORT void mutex_init(thread_mutex_t* mutex, const char* namefmt, ...);
LIBEXPORT void rmutex_init(thread_mutex_t* mutex, const char* namefmt, ...);
LIBEXPORT void mutex_lock(thread_mutex_t* mutex);
LIBEXPORT void mutex_unlock(thread_mutex_t* mutex);
LIBEXPORT void mutex_destroy(thread_mutex_t* mutex);

LIBEXPORT void event_init(thread_event_t* event, const char* namefmt, ...);
LIBEXPORT void event_wait_unlock(thread_event_t* event, thread_mutex_t* mutex);
LIBEXPORT void event_wait(thread_event_t* event);
LIBEXPORT void event_notify_one(thread_event_t* event);
LIBEXPORT void event_notify_all(thread_event_t* event);
LIBEXPORT void event_destroy(thread_event_t* event);

LIBEXPORT void tkey_init(thread_key_t* key, void (*destructor)(void* key),
			   	   	   	 const char* namefmt, ...);
LIBEXPORT void tkey_destroy(thread_key_t* key);
LIBEXPORT void tkey_set(thread_key_t* key, void* value);
LIBEXPORT void* tkey_get(thread_key_t* key);

LIBEXPORT thread_t* t_self();

LIBEXPORT void t_init(thread_t* thread, void* arg,
					  void* (*start)(void*),
		              const char* namefmt, ...);
LIBEXPORT thread_t* t_post_init(thread_t* t);
LIBEXPORT void t_exit(thread_t* t);
LIBEXPORT void t_destroy(thread_t* thread);

LIBEXPORT void t_join(thread_t* thread, thread_event_t* event);

LIBEXPORT int threads_init(void);
LIBEXPORT void threads_fini(void);

LIBEXPORT PLATAPI void t_eternal_wait(void);

/* Platform-dependent functions */

PLATAPI void plat_thread_init(plat_thread_t* thread, void* arg,
							  void* (*start)(void*));
PLATAPI void plat_thread_destroy(plat_thread_t* thread);
PLATAPI unsigned long plat_gettid();

PLATAPI void plat_mutex_init(plat_thread_mutex_t* mutex, int recursive);
PLATAPI void plat_mutex_lock(plat_thread_mutex_t* mutex);
PLATAPI void plat_mutex_unlock(plat_thread_mutex_t* mutex);
PLATAPI void plat_mutex_destroy(plat_thread_mutex_t* mutex);

PLATAPI void plat_event_init(plat_thread_event_t* event);
PLATAPI void plat_event_wait_unlock(plat_thread_event_t* event, plat_thread_mutex_t* mutex);
PLATAPI void plat_event_notify_one(plat_thread_event_t* event);
PLATAPI void plat_event_notify_all(plat_thread_event_t* event);
PLATAPI void plat_event_destroy(plat_thread_event_t* event);

PLATAPI void plat_tkey_init(plat_thread_key_t* key, void (*destructor)(void* key));
PLATAPI void plat_tkey_destroy(plat_thread_key_t* key);
PLATAPI void plat_tkey_set(plat_thread_key_t* key, void* value);
PLATAPI void* plat_tkey_get(plat_thread_key_t* key);

#endif /* THREADPOOL_H_ */
