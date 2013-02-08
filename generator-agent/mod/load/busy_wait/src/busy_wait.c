/*
 * dummy.c
 *
 *  Created on: 06.11.2012
 *      Author: myaut
 */

#define LOG_SOURCE "busy_wait"
#include <log.h>

#include <mempool.h>
#include <defs.h>
#include <workload.h>
#include <modules.h>
#include <modapi.h>

#include <busy_wait.h>

#include <stdlib.h>
#include <string.h>

DECLARE_MODAPI_VERSION(MOD_API_VERSION);
DECLARE_MOD_NAME("busy_wait");
DECLARE_MOD_TYPE(MOD_TSLOAD);

MODEXPORT wlp_descr_t mod_params[] = {
	{ WLP_INTEGER,
		WLP_NO_RANGE(),
		"num_cycles",
		"Number of cycles to be waited (not CPU cycles)",
		offsetof(struct busy_wait_workload, num_cycles) },
	{ WLP_NULL }
};

MODEXPORT size_t mod_params_size = sizeof(struct busy_wait_workload);

module_t* self = NULL;

MODEXPORT int mod_config(module_t* mod) {
	self = mod;

	return 0;
}

MODEXPORT int mod_unconfig(module_t* mod) {
	return 0;
}

MODEXPORT int mod_workload_config(workload_t* wl) {
	return 0;
}

MODEXPORT int mod_workload_unconfig(workload_t* wl) {
	return 0;
}

MODEXPORT int mod_run_request(request_t* rq) {
	struct busy_wait_workload* bww =
			(struct busy_wait_workload*) rq->rq_workload->wl_params;

	volatile wlp_integer_t i;

	for(i = 0; i < bww->num_cycles; ++i);

	return 0;
}
