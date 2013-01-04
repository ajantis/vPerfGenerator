/*
 * init.c
 *
 *  Created on: Dec 3, 2012
 *      Author: myaut
 */

#define LOG_SOURCE ""
#include <log.h>

#include <defs.h>
#include <threads.h>
#include <mempool.h>
#include <workload.h>
#include <modules.h>
#include <loadagent.h>
#include <threadpool.h>

#include <tsinit.h>
#include <tsload.h>

struct subsystem subsys[] = {
	SUBSYSTEM("log", log_init, log_fini),
	SUBSYSTEM("mempool", mempool_init, mempool_fini),
	SUBSYSTEM("threads", threads_init, threads_fini),
	SUBSYSTEM("modules", load_modules, unload_modules),
	SUBSYSTEM("workload", wl_init, wl_fini),
	SUBSYSTEM("threadpool", tp_init, tp_fini),
	SUBSYSTEM("agent", agent_init, agent_fini)
};


int tsload_start(const char* basename) {
	init(subsys, sizeof(subsys) / sizeof(struct subsystem));

	logmsg(LOG_INFO, "Started %s...", basename);

	/*Wait until universe collapses or we receive a signal :)*/
	t_eternal_wait();

	/*NOTREACHED*/
	return 0;
}

