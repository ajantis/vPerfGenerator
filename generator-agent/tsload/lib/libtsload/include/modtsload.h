/*
 * modapi.h
 *
 *  Created on: 06.11.2012
 *      Author: myaut
 */

#ifndef MODTSLOAD_H_
#define MODTSLOAD_H_

#include <modules.h>
#include <wlparam.h>

struct module;
struct workload;

typedef int (* mod_wl_config_func)(struct workload* wl);

typedef struct {
	wlp_descr_t* mod_params;
	size_t mod_params_size;	/**< sizeof structure where parameter values are stored*/

	mod_wl_config_func mod_wl_config;
	mod_wl_config_func mod_wl_unconfig;
} tsload_module_t;

int tsload_mod_helper(module_t* mod);

#ifndef NO_JSON
#include <libjson.h>

JSONNODE* json_mod_params_name(const char* name);
JSONNODE* json_modules_info();
#endif

#endif /* MODAPI_H_ */
