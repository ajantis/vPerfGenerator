/*
 * modules.h
 *
 *  Created on: 06.11.2012
 *      Author: myaut
 */

#ifndef MODULES_H_
#define MODULES_H_

#include <modapi.h>

#define MODPATHLEN	256
#define MODSTATUSMSG 512

/*
 * Module states
 *
 *                       +----------> CONFIG_ERROR
 * 						 |
 * UNITIALIZED ---> UNCONFIGURED ---> READY ---> REQUEST_ERROR
 *                                     ^                |
 *                                     +----------------+
 */

typedef enum mod_status {
	MOD_UNITIALIZED,
	MOD_UNCONFIGURED,
	MOD_READY,
	MOD_CONFIG_ERROR,
	MOD_REQUEST_ERROR
} mod_status_t;

typedef struct module {
	char mod_path[MODPATHLEN];
	void* mod_library;		/**< Handle provided by dlopen() */

	char* mod_name;

	mod_status_t mod_status;
	char* mod_status_msg;

	mod_config_func mod_config;
	mod_config_func mod_unconfig;

	void* mod_helper;

	void* mod_private;		/**< Allocated when module is configured*/

	struct module* mod_next;
} module_t;

int load_modules();
void unload_modules(void);

module_t* mod_search(const char* name);
int mod_error(module_t* mod, char* fmtstr, ...);

void* mod_load_symbol(module_t* mod, const char* name);

#define MOD_LOAD_SYMBOL(type, mod, name, flag) 				\
		(type) ({ void* ret = mod_load_symbol(mod, name); 	\
			if(ret == NULL)	{			 					\
				logmsg(LOG_WARN, "Required parameter(s) %s is undefined", name); \
				flag = TRUE;			 					\
			}							 					\
			ret; })

void set_mod_helper(int type, int (*helper)(module_t* mod));

#endif /* MODULES_H_ */
