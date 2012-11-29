/*
 * agent.h
 *
 *  Created on: 29.11.2012
 *      Author: myaut
 */

#ifndef AGENT_H_
#define AGENT_H_

#include <libjson.h>

#define AGENTMAXARGC	16

typedef void* (*agent_proc_func_t)(JSONNODE* arg);
typedef JSONNODE* (*agent_method_func_t)(void* argv);

typedef struct {
	char* ad_name;

	struct {
		char* ad_arg_name;
		agent_proc_func_t ad_arg_proc;
	} ad_args[AGENTMAXARGC];

	agent_method_func_t ad_method;
} agent_dispatch_t;

#define ADT_ARGUMENT(name, proc) { .ad_arg_name = name, .ad_arg_proc = proc }
#define ADT_LAST_ARG()  { .ad_arg_name = NULL }
#define ADT_LAST_METHOD() { .ad_name = NULL }

JSONNODE* agent_process_command(char* command, JSONNODE* msg);
JSONNODE* agent_response_msg(JSONNODE* response);
JSONNODE* agent_error_msg(const char* format, ...);
void agent_register_methods(agent_dispatch_t* table);

int agent_hello();

#endif /* AGENT_H_ */
