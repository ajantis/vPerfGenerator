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

	json_push_back(info_node, json_new_a("hostName", "localhost"));

	json_set_name(info_node, "info");
	json_push_back(node, info_node);

	return node;
}

void agent_response_msg(JSONNODE* response) {
	JSONNODE* node = json_new(JSON_NODE);

	assert(response != NULL);

	json_set_name(response, "response");
	json_push_back(node, response);

	clnt_add_response(RT_RESPONSE, node);
}

void agent_error_msg(agent_errcode_t code, const char* format, ...) {
	JSONNODE* node = json_new(JSON_NODE);
	char error_msg[256];
	va_list args;

	clnt_proc_msg_t* msg = clnt_proc_get_msg();

	va_start(args, format);
	vsnprintf(error_msg, 256, format, args);
	va_end(args);

	json_push_back(node, json_new_i("code", code));
	json_push_back(node, json_new_a("error", error_msg));

	logmsg(LOG_WARN, "Error while processing command %s msg #%u: %s",
			msg->m_command, msg->m_msg_id, error_msg);

	clnt_add_response(RT_ERROR, node);
}

void agent_process_command(char* command, JSONNODE* n_msg) {
	agent_dispatch_t* method = agent_dispatch_table;
	int i = 0, num_args = 0;

	char* arg_name;
	agent_proc_func_t arg_proc;

	JSONNODE* n_arg;
	void* argv[AGENTMAXARGC];
	JSONNODE* jargv[AGENTMAXARGC];

	assert(agent_dispatch_table != NULL);

	while(method->ad_name) {
		if(strcmp(method->ad_name, command) != 0) {
			++method;
			continue;
		}

		/* Process arguments. Incoming arguments are JSON_NODE with
		 * subnodes which names are matching arguments names.
		 *
		 * Finds n_arg for each argument and saves result to jargv,
		 * than process it if needed and saves result to argv. Argv
		 * passed to method.
		 *
		 * arg_proc may allocate memory, so first we check arguments
		 * than we process them */
		for(i = 0; i < AGENTMAXARGC; ++i) {
			arg_name = method->ad_args[i].ad_arg_name;

			if(arg_name == NULL) {
				num_args = i + 1;
				break;
			}

			n_arg = json_get(n_msg, arg_name);

			if(n_arg == NULL) {
				agent_error_msg(AE_MESSAGE_FORMAT, "Missing argument %s!", arg_name);

				return;
			}

			jargv[i] = n_arg;
		}

		for(i = 0; i < num_args; ++i) {
			arg_proc = method->ad_args[i].ad_arg_proc;
			argv[i] = (void*) arg_proc ? arg_proc(jargv[i]) : jargv[i];
		}

		method->ad_method(argv);
		return;
	}

	agent_error_msg(AE_COMMAND_NOT_FOUND, "Not found command %s!", command);
}

int agent_hello() {
	int ret;
	JSONNODE* response;

	ret = clnt_invoke("hello", agent_hello_msg(), &response);

	/*Process agent ID*/

	return ret;
}
