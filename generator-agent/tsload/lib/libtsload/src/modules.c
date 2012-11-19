/*
 * modules.c
 *
 *  Created on: 24.10.2012
 *      Author: myaut
 */

#define LOG_SOURCE "modules"
#include <log.h>

#include <modules.h>
#include <libjson.h>

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>

#include <dlfcn.h>

#define INFOCHUNKLEN	2048

char mod_search_path[MODPATHLEN];

/*First module in modules linked list*/
static module_t* first_module = NULL;

module_t* mod_load(const char* path_name);

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

module_t* mod_search(const char* name) {
	module_t* mod = first_module;

	while(mod != NULL) {
		if(strcmp(mod->mod_name, name) == 0)
			break;

		mod = mod->mod_next;
	}

	return mod;
}

module_t* mod_load(const char* path_name) {
	module_t* mod = mod_create();
	int* api_version;
	size_t* params_size;

	mod->mod_library = dlopen(path_name, RTLD_NOW | RTLD_LOCAL);

	logmsg(LOG_INFO, "Loading module %s ...", path_name);

	if(!mod->mod_library) {
		logmsg(LOG_WARN, "DL reason: %s", dlerror());
		logerror();

		goto fail;
	}

	/*Load module api version and name*/

	api_version = (int*) dlsym(mod->mod_library, "mod_api_version");
	mod->mod_name = (char*) dlsym(mod->mod_library, "mod_name");
	mod->mod_params = (wlp_descr_t*) dlsym(mod->mod_library, "mod_params");
	params_size = (size_t*) dlsym(mod->mod_library, "mod_params_size");

	if(!api_version || !mod->mod_name || !mod->mod_params || !params_size) {
		logmsg(LOG_WARN, "Required parameter(s) %s %s %s %s is undefined",
				(!api_version)? "mod_api_version" : "",
				(!mod->mod_name)? "mod_name" : "",
				(!mod->mod_params)? "mod_params" : "",
				(!params_size)? "mod_params_size" : "" );

		goto fail;
	}

	if(*api_version != MODAPI_VERSION) {
		logmsg(LOG_WARN, "Wrong api version %d", *api_version);

		goto fail;
	}

	if(mod_search(mod->mod_name) != NULL) {
		logmsg(LOG_WARN, "Module %s already exists", mod->mod_name);
	}

	mod->mod_params_size = *params_size;

	/*Load methods*/
	mod->mod_config = dlsym(mod->mod_library, "mod_config");
	mod->mod_unconfig = dlsym(mod->mod_library, "mod_unconfig");
	mod->mod_wl_config = dlsym(mod->mod_library, "mod_workload_config");
	mod->mod_wl_unconfig = dlsym(mod->mod_library, "mod_workload_unconfig");

	if(!mod->mod_config || !mod->mod_wl_config || !mod->mod_wl_unconfig) {
		logmsg(LOG_WARN, "Method(s) %s %s %s was not found",
				(!mod->mod_config)? "mod_config" : "",
				(!mod->mod_wl_config)? "mod_workload_config" : "",
				(!mod->mod_wl_unconfig)? "mod_workload_unconfig" : "");

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

	logmsg(LOG_WARN, "Failed to load module %s !", path_name);

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

JSONNODE* json_mod_params(const char* name) {
	module_t* mod = mod_search(name);

	if(!mod)
		return NULL;

	return json_wlparam_format(mod->mod_params);
}

JSONNODE* json_modules_info() {
	JSONNODE* node = json_new(JSON_NODE);
	JSONNODE* mod_node = NULL;
	module_t* mod = first_module;

	while(mod != NULL) {
		mod_node = json_new(JSON_NODE);
		json_set_name(mod_node, mod->mod_name);

		json_push_back(mod_node, json_new_a("path", mod->mod_path));
		json_push_back(mod_node, json_wlparam_format(mod->mod_params));

		json_push_back(node, mod_node);

		mod = mod->mod_next;
	}

	return node;
}
