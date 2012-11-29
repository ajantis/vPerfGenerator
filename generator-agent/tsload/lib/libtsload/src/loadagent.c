/*
 * loadagent.c
 *
 *  Created on: 30.11.2012
 *      Author: myaut
 */

#define LOG_SOURCE "agent"
#include <log.h>

#include <agent.h>
#include <modtsload.h>
#include <wlparam.h>

JSONNODE* agent_get_modules_info(void* argv) {
	JSONNODE* mod_info = json_modules_info();
	JSONNODE* node = json_new(JSON_NODE);

	json_set_name(mod_info, "modules");
	json_push_back(node, mod_info);

	return agent_response_msg(node);
}

JSONNODE* agent_configure_workloads(void* argv) {
	return agent_error_msg("Not implemented yet");
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

void agent_init(void) {
	agent_register_methods(loadagent_table);
}
