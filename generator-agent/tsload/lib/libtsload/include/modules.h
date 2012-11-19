/*
 * modules.h
 *
 *  Created on: 06.11.2012
 *      Author: myaut
 */

#ifndef MODULES_H_
#define MODULES_H_

#include <wlparam.h>
#include <modapi.h>

#define MODPATHLEN	256

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

	wlp_descr_t* mod_params;
	size_t mod_params_size;	/**< sizeof structure where parameter values are stored*/

	/*Methods*/
	mod_config_func mod_config;

	void* mod_private;		/**< Allocated when module is configured*/

	struct module* mod_next;
} module_t;

int load_modules();

module_t* mod_search(const char* name);

#ifndef NO_JSON
#include <libjson.h>

JSONNODE* json_mod_params(const char* name);
JSONNODE* json_modules_info();
#endif

#endif /* MODULES_H_ */
