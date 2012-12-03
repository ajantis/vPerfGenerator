/*
 * loadagent.c
 *
 *  Created on: 30.11.2012
 *      Author: myaut
 */

#define LOG_SOURCE "agent"
#include <log.h>

#include <client.h>
#include <agent.h>
#include <modtsload.h>
#include <wlparam.h>
#include <workload.h>

JSONNODE* agent_get_modules_info(void* argv[]) {
	JSONNODE* mod_info = json_modules_info();
	JSONNODE* node = json_new(JSON_NODE);

	json_set_name(mod_info, "modules");
	json_push_back(node, mod_info);

	return agent_response_msg(node);
}

JSONNODE* agent_configure_workloads(void* argv[]) {
	workload_t* wl, *iter;
	JSONNODE* wl_head = argv[0];
	JSONNODE_ITERATOR i_list = json_find(wl_head, "workloads"),
					  i_end = json_end(wl_head);

	if(i_list == i_end) {
		return agent_error_msg("Not found workloads");
	}

	wl = json_workload_proc_all(*i_list);

	if(wl == NULL)
		return agent_error_msg("workload_error!");

	for(iter = wl; iter != NULL; iter = iter->wl_next)
		wl_config(iter);

	return agent_response_msg(json_new(JSON_NODE));
}

void agent_workload_status(const char* wl_name, const char* status_msg, int done, const char* config_msg) {
	JSONNODE* status = json_new(JSON_NODE), *node = json_new(JSON_NODE);
	JSONNODE* response;

	json_push_back(status, json_new_a("workload", wl_name));
	json_push_back(status, json_new_a("status", status_msg));
	json_push_back(status, json_new_i("done", done));
	json_push_back(status, json_new_a("message", config_msg));

	json_set_name(status, "status");
	json_push_back(node, status);

	clnt_invoke("workload_status", node, &response);

	json_delete(response);
}

static agent_dispatch_t loadagent_table[] = {
	{
		.ad_name = "get_modules_info",
		.ad_args = {
			ADT_LAST_ARG()
		},
		.ad_method = agent_get_modules_info
	},
	{
		.ad_name = "configure_workloads",
		{
			ADT_ARGUMENT("workloads", NULL)
		},
		.ad_method = agent_configure_workloads
	},
	ADT_LAST_METHOD()
};

int agent_init(void) {
	agent_register_methods(loadagent_table);

	return clnt_init();
}

void agent_fini(void) {
	clnt_fini();
}
