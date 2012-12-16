/*
 * loadagent.h
 *
 *  Created on: 30.11.2012
 *      Author: myaut
 */

#ifndef LOADAGENT_H_
#define LOADAGENT_H_

#include <agent.h>

void agent_workload_status(const char* wl_name, int status, int done, const char* config_msg);
void agent_requests_report(JSONNODE* j_rq_list);

int agent_init(void);
void agent_fini(void);

#endif /* LOADAGENT_H_ */