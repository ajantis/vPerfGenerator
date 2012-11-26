#define LOG_SOURCE "modules"
#include <log.h>

#include <modules.h>
#include <modtsload.h>
#include <wlparam.h>

#include <libjson.h>

#include <stdlib.h>

extern module_t* first_module;

int tsload_mod_helper(module_t* mod) {
	tsload_module_t* tmod = malloc(sizeof(tsload_module_t));
	size_t* params_size;

	int flag = FALSE;

	tmod->mod_params = MOD_LOAD_SYMBOL(wlp_descr_t*, mod, "mod_params", flag);
	params_size =  MOD_LOAD_SYMBOL(size_t*, mod, "mod_params_size", flag);

	tmod->mod_wl_config = MOD_LOAD_SYMBOL(mod_wl_config_func, mod, "mod_workload_config", flag);
	tmod->mod_wl_unconfig = MOD_LOAD_SYMBOL(mod_wl_config_func, mod, "mod_workload_unconfig", flag);

	if(flag) {
		free(tmod);

		return 1;
	}

	tmod->mod_params_size = *params_size;
	mod->mod_helper = tmod;

	return 0;
}

JSONNODE* json_mod_params(module_t* mod) {
	tsload_module_t* tmod = NULL;

	if(!mod || !mod->mod_helper)
		return NULL;

	tmod = (tsload_module_t*) mod->mod_helper;

	return json_wlparam_format_all(tmod->mod_params);
}

JSONNODE* json_mod_params_name(const char* name) {
	return json_mod_params(mod_search(name));
}

JSONNODE* json_modules_info() {
	JSONNODE* node = json_new(JSON_NODE);
	JSONNODE* mod_node = NULL;
	module_t* mod = first_module;

	while(mod != NULL) {
		mod_node = json_new(JSON_NODE);
		json_set_name(mod_node, mod->mod_name);

		json_push_back(mod_node, json_new_a("path", mod->mod_path));
		json_push_back(mod_node, json_mod_params(mod));

		json_push_back(node, mod_node);

		mod = mod->mod_next;
	}

	return node;
}
