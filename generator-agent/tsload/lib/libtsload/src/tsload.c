/*
 * init.c
 *
 *  Created on: Dec 3, 2012
 *      Author: myaut
 */

#define LOG_SOURCE ""
#include <log.h>

#include <threads.h>
#include <mempool.h>
#include <workload.h>
#include <modules.h>
#include <loadagent.h>
#include <threadpool.h>

#include <stdlib.h>
#include <stdio.h>

#include <sys/select.h>

typedef enum {
	SS_UNINITIALIZED,
	SS_OK,
	SS_ERROR
} subsystem_state_t;

struct subsystem {
	char* s_name;

	int s_state;
	int s_error_code;

	int (*s_init)(void);
	void (*s_fini)(void);
};

#define SUBSYSTEM(name, init, fini) \
	{								\
		.s_name = name,				\
		.s_error_code = 0,			\
		.s_state = SS_UNINITIALIZED,\
		.s_init = init,				\
		.s_fini = fini				\
	}

struct subsystem subsys[] = {
	SUBSYSTEM("mempool", mempool_init, mempool_fini),
	SUBSYSTEM("threads", threads_init, threads_fini),
	SUBSYSTEM("log", log_init, log_fini),
	SUBSYSTEM("modules", load_modules, unload_modules),
	SUBSYSTEM("workload", wl_init, wl_fini),
	SUBSYSTEM("threadpool", tp_init, tp_fini),
	SUBSYSTEM("agent", agent_init, agent_fini)
};

int init(void) {
	int i = 0;
	int err = 0;
	size_t s_count = sizeof(subsys) / sizeof(struct subsystem);

	for(i = 0; i < s_count; ++i) {
		err = subsys[i].s_init();

		if(err != 0) {
			subsys[i].s_state = SS_ERROR;
			fprintf(stderr, "Failure initializing %s, exiting", subsys[i].s_name);
			exit(err);
		}

		subsys[i].s_error_code = err;
	}
}

void finish(void) {
	int i = 0;
	size_t s_count = sizeof(subsys) / sizeof(struct subsystem);

	for(i = s_count - 1; i >= 0; --i) {
		if(subsys[i].s_state == SS_OK) {
			subsys[i].s_fini();
		}
	}
}

int tsload_start(const char* basename) {
	atexit(finish);
	init();

	logmsg("Started %s...", basename);

	/*Wait until universe collapses or we receive a signal :)*/
	select(0, NULL, NULL, NULL, NULL);
}

