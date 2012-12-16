/*
 * modules.c
 *
 *  Created on: 24.10.2012
 *      Author: myaut
 */

#define LOG_SOURCE "modules"
#include <log.h>

#include <mempool.h>
#include <defs.h>
#include <modules.h>
#include <libjson.h>

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <sys/types.h>
#include <dirent.h>
#include <dlfcn.h>

#define INFOCHUNKLEN	2048

char mod_search_path[MODPATHLEN];

/*First module in modules linked list*/
module_t* first_module = NULL;

int mod_type = -1;
int (*mod_load_helper)(module_t* mod) = NULL;

module_t* mod_load(const char* path_name);
void mod_destroy(module_t* mod);

int load_modules() {
	DIR* dir = opendir(mod_search_path);
	struct dirent* d = NULL;
	char path[MODPATHLEN];

	if(!dir) {
		logmsg(LOG_CRIT, "Failed to open directory %s", mod_search_path);
		logerror();

		return -1;
	}

	while((d = readdir(dir)) != NULL) {
		if(d->d_name[0] == '.' || d->d_type != DT_REG)
			continue;

		/* May be optimized (do strcpy once, move pointer to end of
		 * path, then do strcpy again and again)*/
		strcpy(path, mod_search_path);
		strncat(path, d->d_name, MODPATHLEN);

		mod_load(path);
	}

	closedir(dir);
	return 0;
}

void unload_modules(void) {
	module_t* mod = first_module;
	module_t* next;

	while(mod != NULL) {
		next = mod->mod_next;
		mod_destroy(mod);
		mod = next;
	}
}

module_t* mod_create() {
	module_t* mod = (module_t*) mp_malloc(sizeof(module_t));

	mod->mod_library = NULL;
	mod->mod_helper = NULL;
	mod->mod_status = MOD_UNITIALIZED;

	mod->mod_next = NULL;

	return mod;
}

/*Add module to modules linked list*/
void mod_add(module_t* mod) {
	if(!first_module) {
		first_module = mod;
	}
	else {
		/*Search for list tail*/
		module_t* mi = first_module;

		while(mi->mod_next)
			mi = mi->mod_next;

		mi->mod_next = mod;
	}
}

void mod_destroy(module_t* mod) {
	/*XXX: doesn't unlink module from linked list because this function is called during cleanup*/
	/*TODO: cleanup other fields like mod_private*/

	if(mod->mod_status != MOD_UNITIALIZED) {
		if(dlclose(mod->mod_library) != 0) {
			logmsg(LOG_WARN, "Failed to free dl-handle of module %s. Reason: %s", mod->mod_name, dlerror());
		}

		if(mod->mod_helper)
			mp_free(mod->mod_helper);

		logmsg(LOG_INFO, "Destroying module %s", mod->mod_name);
	}

	mp_free(mod);
}

module_t* mod_search(const char* name) {
	module_t* mod = first_module;

	while(mod != NULL) {
		if(strcmp(mod->mod_name, name) == 0)
			break;

		mod = mod->mod_next;
	}

	return mod;
}

void* mod_load_symbol(module_t* mod, const char* name) {
	return dlsym(mod->mod_library, name);
}

module_t* mod_load(const char* path_name) {
	module_t* mod = mod_create();
	int* api_version;
	int* type;

	int flag = FALSE;

	assert(mod_load_helper != NULL);
	assert(mod != NULL);

	mod->mod_library = dlopen(path_name, RTLD_NOW | RTLD_LOCAL);

	logmsg(LOG_INFO, "Loading module %s ...", path_name);

	if(!mod->mod_library) {
		logmsg(LOG_WARN, "DL reason: %s", dlerror());
		logerror();

		goto fail;
	}

	/*Load module api version and name*/

	api_version = MOD_LOAD_SYMBOL(int*, mod, "mod_api_version", flag);
	type = MOD_LOAD_SYMBOL(int*, mod, "mod_type", flag);
	mod->mod_name = MOD_LOAD_SYMBOL(char*, mod, "mod_name", flag);

	if(flag)
		goto fail;

	if(*type != mod_type) {
		logmsg(LOG_INFO, "Ignoring module - wrong type %d", *type);

		goto fail;
	}

	if(*api_version != MOD_API_VERSION) {
		logmsg(LOG_WARN, "Wrong api version %d", *api_version);

		goto fail;
	}

	if(mod_search(mod->mod_name) != NULL) {
		logmsg(LOG_WARN, "Module %s already exists", mod->mod_name);

		goto fail;
	}

	mod->mod_config = MOD_LOAD_SYMBOL(mod_config_func, mod, "mod_config", flag);
	mod->mod_unconfig = MOD_LOAD_SYMBOL(mod_config_func, mod, "mod_unconfig", flag);

	/*Call helper*/
	mod->mod_status = MOD_UNCONFIGURED;

	if(mod_load_helper(mod) == 1)
		goto fail;

	logmsg(LOG_INFO, "Loaded module %s (path: %s)", mod->mod_name, path_name);

	mod->mod_status = MOD_READY;
	mod_add(mod);

	return mod;

fail:
	if(mod->mod_library)
		dlclose(mod->mod_library);

	logmsg(LOG_WARN, "Failed to load module %s!", path_name);

	mod_destroy(mod);

	return NULL;
}

int mod_error(module_t* mod, char* fmtstr, ...) {
	char status[512];
	va_list args;

	mod->mod_status = MOD_CONFIG_ERROR;

	va_start(args, fmtstr);
	vsnprintf(status, 512, fmtstr, args);
	va_end(args);

	logmsg(LOG_WARN, "Error in module %s: %s", mod->mod_name, status);

	mod->mod_status_msg = strdup(status);

	return 0;
}

void set_mod_helper(int type, int (*helper)(module_t* mod)) {
	mod_type = type;
	mod_load_helper = helper;
}
