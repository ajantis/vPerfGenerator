/*
 * agent.c
 *
 *  Created on: 29.11.2012
 *      Author: myaut
 */

#define LOG_SOURCE "agent"
#include <log.h>

#include <agent.h>
#include <client.h>

#include <libjson.h>

#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

static agent_dispatch_t* agent_dispatch_table = NULL;

void agent_register_methods(agent_dispatch_t* table) {
	agent_dispatch_table = table;
}

JSONNODE* agent_hello_msg() {
	JSONNODE* node = json_new(JSON_NODE);

	JSONNODE* info_node = json_new(JSON_NODE);
	json_set_name(info_node, "info");

	json_push_back(info_node, json_new_a("hostName", "localhost"));

	json_push_back(node, info_node);

	return node;
}

JSONNODE* agent_response_msg(JSONNODE* response) {
	JSONNODE* node = json_new(JSON_NODE);

	assert(response != NULL);

	json_set_name(response, "response");
	json_push_back(node, response);

	return node;
}

JSONNODE* agent_error_msg(const char* format, ...) {
	JSONNODE* node = json_new(JSON_NODE);
	char error_msg[256];
	va_list args;

	va_start(args, format);
	vsnprintf(error_msg, 256, format, args);
	va_end(args);

	json_push_back(node, json_new_a("error", error_msg));

	return node;
}

JSONNODE* agent_process_command(char* command, JSONNODE* msg) {
	agent_dispatch_t* method = agent_dispatch_table;
	int i = 0, num_args = 0;

	char* arg_name;
	agent_proc_func_t arg_proc;

	JSONNODE_ITERATOR i_arg, i_end;
	void* argv[AGENTMAXARGC];
	JSONNODE* jargv[AGENTMAXARGC];

	assert(agent_dispatch_table != NULL);

	i_end = json_end(msg);

	while(method->ad_name) {
		if(strcmp(method->ad_name, command) != 0) {
			++method;
			continue;
		}

		/* Process arguments
		 *
		 * arg_proc may allocate memory, so first we check arguments
		 * than we process them */
		for(i = 0; i < AGENTMAXARGC; ++i) {
			arg_name = method->ad_args[i].ad_arg_name;

			if(arg_name == NULL) {
				num_args = i + 1;
				break;
			}

			i_arg = json_find(msg, arg_name);

			if(i_arg == i_end) {
				logmsg(LOG_WARN, "Invoked command %s, but argument %s is missing!", command, arg_name);
				return agent_error_msg("Missing argument %s!", arg_name);
			}

			jargv[i] = *i_arg;
		}

		for(i = 0; i < num_args; ++i) {
			arg_proc = method->ad_args[i].ad_arg_proc;
			argv[i] = (void*) arg_proc ? arg_proc(jargv[i]) : jargv[i];
		}

		return method->ad_method(argv);
	}

	logmsg(LOG_WARN, "Invoked unknown command %s!", command);
	return agent_error_msg("Not found command %s!", command);
}

int agent_hello() {
	int ret;
	JSONNODE* response;

	ret = clnt_invoke("hello", agent_hello_msg(), &response);

	/*Process agent ID*/

	return ret;
}
