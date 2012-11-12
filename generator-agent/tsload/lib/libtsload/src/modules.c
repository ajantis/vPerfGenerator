/*
 * modules.c
 *
 *  Created on: 24.10.2012
 *      Author: myaut
 */

#define LOG_SOURCE "modules"
#include <log.h>

#include <modules.h>

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>

#include <dlfcn.h>

char mod_search_path[MODPATHLEN];

/*First module in modules linked list*/
static module_t* first_module = NULL;

module_t* mod_load(char* path_name);

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

module_t* mod_create() {
	module_t* mod = (module_t*) malloc(sizeof(module_t));

	mod->mod_library = NULL;
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

		logmsg(LOG_INFO, "Destroying module %s", mod->mod_name);
	}

	free(mod);
}

module_t* mod_load(char* path_name) {
	module_t* mod = mod_create();
	int* api_version;

	mod->mod_library = dlopen(path_name, RTLD_NOW | RTLD_LOCAL);

	if(!mod->mod_library) {
		logmsg(LOG_WARN, "Failed to load module %s. Reason: %s", path_name, dlerror());
		logerror();

		goto fail;
	}

	/*Load module api version and name*/

	api_version = (int*) dlsym(mod->mod_library, "mod_api_version");
	mod->mod_name = (char*) dlsym(mod->mod_library, "mod_name");
	mod->mod_params = (wlp_descr_t*) dlsym(mod->mod_library, "mod_params");

	if(!api_version || !mod->mod_name || !mod->mod_params) {
		logmsg(LOG_WARN, "Failed to load module %s. API version, name or params are undefined.", path_name);

		goto fail;
	}

	if(*api_version != MODAPI_VERSION) {
		logmsg(LOG_WARN, "Failed to load module %s. Wrong api version %d", path_name, *api_version);

		goto fail;
	}

	/*Load methods*/
	mod->mod_config = dlsym(mod->mod_library, "mod_config");

	logmsg(LOG_DEBUG, "Module json: %s", json_gen_wlp(mod->mod_params));

	if(!mod->mod_config) {
		logmsg(LOG_WARN, "Failed to load module %s. One of method was not found", mod->mod_name);

		goto fail;
	}

	/*Finish loading*/
	mod->mod_config(mod);

	logmsg(LOG_INFO, "Loaded module %s (path: %s)", mod->mod_name, path_name);

	mod->mod_status = MOD_UNCONFIGURED;
	mod_add(mod);

	return mod;

fail:
	if(mod->mod_library)
		dlclose(mod->mod_library);

	mod_destroy(mod);

	return NULL;
}
