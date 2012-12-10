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

typedef void (*agent_method_func_t)(JSONNODE* argv[]);

typedef struct {
	char* ad_name;

	struct {
		char* ada_name;
		char ada_type;
	} ad_args[AGENTMAXARGC];

	agent_method_func_t ad_method;
} agent_dispatch_t;

#define ADT_ARGUMENT(name, type) { .ada_name = name, .ada_type = type }
#define ADT_LAST_ARG()  { .ada_name = NULL, .ada_type = JSON_NULL }
#define ADT_LAST_METHOD() { .ad_name = NULL }

typedef enum agent_errcode {
	AE_COMMAND_NOT_FOUND = 100,
	AE_MESSAGE_FORMAT	 = 101,
	AE_INVALID_DATA		 = 102,
	AE_INTERNAL_ERROR	 = 200
} agent_errcode_t;

void agent_process_command(char* command, JSONNODE* msg);
void agent_response_msg(JSONNODE* response);
void agent_error_msg(agent_errcode_t code, const char* format, ...);
void agent_register_methods(agent_dispatch_t* table);

int agent_hello();

#endif /* AGENT_H_ */
