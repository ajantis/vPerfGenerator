/*
 * loadagent.c
 *
 *  Created on: 30.11.2012
 *      Author: myaut
 */

#define LOG_SOURCE "agent"
#include <log.h>

#include <loadagent.h>
#include <defs.h>
#include <client.h>
#include <agent.h>

#define TSLOAD_IMPORT LIBIMPORT
#include <tsload.h>

/* = Agent's handlers = */
void agent_get_modules_info(JSONNODE* argv[]) {
	JSONNODE* response = json_new(JSON_NODE);

	JSONNODE* mod_info = tsload_get_modules_info();

	json_set_name(mod_info, "modules");
	json_push_back(response, mod_info);

	return agent_response_msg(response);
}

void agent_configure_workload(JSONNODE* argv[]) {
	char* wl_name = json_as_string(argv[0]);
	JSONNODE* wl_params = (JSONNODE*) argv[1];

	tsload_configure_workload(wl_name, wl_params);

	json_free(wl_name);

	/* Send response if no error encountered */
	if(!clnt_proc_error()) {
		agent_response_msg(json_new(JSON_NODE));
	}
}

void agent_start_workload(JSONNODE* argv[]) {
	/* While ts_time_t may hold time until 26th century, json_as_int
	 * provides signed values, so it will work until Sat Apr 12 03:47:16 2262 */
	ts_time_t start_time = (ts_time_t) json_as_int(argv[1]);
	char* wl_name = json_as_string(argv[0]);

	tsload_start_workload(wl_name, start_time);

	json_free(wl_name);

	/* Send response if no error encountered */
	if(!clnt_proc_error()) {
		agent_response_msg(json_new(JSON_NODE));
	}
}

void agent_provide_step(JSONNODE* argv[]) {
	int ret = 0;

	char* wl_name = json_as_string(argv[0]);
	long step_id = json_as_int(argv[1]);
	unsigned num_rqs = json_as_int(argv[2]);

	JSONNODE* response = NULL;

	/* Call tsload function */
	ret = tsload_provide_step(wl_name, step_id, num_rqs);

	json_free(wl_name);

	/* Send response if no error encountered */
	if(!clnt_proc_error()) {
		response = json_new(JSON_NODE);
		json_push_back(response, json_new_i("result", ret));
		agent_response_msg(response);
	}
}

/* = TSServer's methods = */
void agent_workload_status(const char* wl_name, int status, int progress, const char* config_msg) {
	JSONNODE* message = json_new(JSON_NODE);
	JSONNODE* response;

	json_push_back(message, json_new_a("workload", wl_name));
	json_push_back(message, json_new_i("status", status));
	json_push_back(message, json_new_i("progress", progress));
	json_push_back(message, json_new_a("message", config_msg));

	clnt_invoke("workload_status", message, &response);

	json_delete(response);
}

void agent_requests_report(list_head_t* rq_list) {
	JSONNODE *msg = json_new(JSON_NODE), *arg0 = json_new(JSON_NODE);
	JSONNODE *response;

	JSONNODE *j_rq_list = json_request_format_all(rq_list);

	json_set_name(arg0, "requests");
	json_push_back(msg, arg0);

	json_set_name(j_rq_list, "requests");
	json_push_back(arg0, j_rq_list);

	clnt_invoke("requests_report", msg, &response);

	json_delete(response);
}

static agent_dispatch_t loadagent_table[] = {
	AGENT_METHOD("get_modules_info",
		ADT_ARGS(),
		agent_get_modules_info),
	AGENT_METHOD("configure_workload",
		ADT_ARGS(
			ADT_ARGUMENT("workload_name", JSON_STRING),
			ADT_ARGUMENT("workload_params", JSON_NODE),
		),
		agent_configure_workload),
	AGENT_METHOD("start_workload",
		ADT_ARGS(
			ADT_ARGUMENT("workload_name", JSON_STRING),
			ADT_ARGUMENT("start_time", JSON_NUMBER),
		),
		agent_start_workload),
	AGENT_METHOD("provide_step",
		ADT_ARGS(
			ADT_ARGUMENT("workload_name", JSON_STRING),
			ADT_ARGUMENT("step_id", JSON_NUMBER),
			ADT_ARGUMENT("num_requests", JSON_NUMBER),
		),
		agent_provide_step),

	ADT_LAST_METHOD()
};

int agent_init(void) {
	agent_register_methods(loadagent_table);

	tsload_error_msg = agent_error_msg;
	tsload_workload_status = agent_workload_status;
	tsload_requests_report = agent_requests_report;

	return clnt_init();
}

void agent_fini(void) {
	clnt_fini();
}
