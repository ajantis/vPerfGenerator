/*
 * loadagent.c
 *
 *  Created on: 30.11.2012
 *      Author: myaut
 */

#define LOG_SOURCE "agent"
#include <log.h>

#include <list.h>
#include <client.h>
#include <agent.h>
#include <modtsload.h>
#include <wlparam.h>
#include <workload.h>

/* = Agent's handlers = */
void agent_get_modules_info(JSONNODE* argv[]) {
	JSONNODE* mod_info = json_modules_info();
	JSONNODE* node = json_new(JSON_NODE);

	json_set_name(mod_info, "modules");
	json_push_back(node, mod_info);

	return agent_response_msg(node);
}

void agent_configure_workloads(JSONNODE* argv[]) {
	workload_t* wl;
	JSONNODE* j_wl_head = argv[0];
	list_head_t wl_list;
	JSONNODE_ITERATOR i_list = json_find(j_wl_head, "workloads"),
					  i_end = json_end(j_wl_head);

	if(i_list == i_end) {
		return agent_error_msg(AE_INVALID_DATA, "Not found workloads");
	}

	list_head_init(&wl_list, "wl_list");

	json_workload_proc_all(*i_list, &wl_list);

	if(list_empty(&wl_list)) {
		return agent_error_msg(AE_INTERNAL_ERROR, "workload_error!");
	}

	list_for_each_entry(wl, &wl_list, wl_chain) {
		wl_config(wl);
	}

	/*Response */
	agent_response_msg(json_new(JSON_NODE));
}

void agent_start_workload(JSONNODE* argv[]) {
	/* While ts_time_t may hold time until 26th century, json_as_int
	 * provides signed values, so it will work until Sat Apr 12 03:47:16 2262 */
	ts_time_t start_time = (ts_time_t) json_as_int(argv[1]);

	char* wl_name = json_as_string(argv[0]);
	workload_t* wl = wl_search(wl_name);

	if(wl == NULL) {
		agent_error_msg(AE_INVALID_DATA, "Not found workload %s", wl_name);
		json_free(wl_name);
		return;
	}

	if(wl->wl_is_configured == FALSE) {
		agent_error_msg(AE_INVALID_DATA, "Workload %s not yet configured", wl_name);
		json_free(wl_name);
		return;
	}

	wl->wl_start_time = start_time;

	json_free(wl_name);

	/*Response */
	agent_response_msg(json_new(JSON_NODE));
}

void agent_provide_step(JSONNODE* argv[]) {
	int ret = 0;

	char* wl_name = json_as_string(argv[0]);
	long step_id = json_as_int(argv[1]);
	unsigned num_rqs = json_as_int(argv[2]);

	workload_t* wl = wl_search(wl_name);

	JSONNODE* response = json_new(JSON_NODE);

	if(wl == NULL) {
		agent_error_msg(AE_INVALID_DATA, "Not found workload %s", wl_name);
		json_free(wl_name);
		return;
	}

	ret = wl_provide_step(wl, step_id, num_rqs);

	json_push_back(response, json_new_i("result", ret));

	json_free(wl_name);

	agent_response_msg(response);
}

/* = TSServer's methods = */
void agent_workload_status(const char* wl_name, int status, int done, const char* config_msg) {
	JSONNODE* msg = json_new(JSON_NODE), *arg0 = json_new(JSON_NODE);
	JSONNODE* response;

	json_push_back(arg0, json_new_a("workload", wl_name));
	json_push_back(arg0, json_new_i("status", status));
	json_push_back(arg0, json_new_i("done", done));
	json_push_back(arg0, json_new_a("message", config_msg));

	json_set_name(arg0, "status");
	json_push_back(msg, arg0);

	clnt_invoke("workload_status", msg, &response);

	json_delete(response);
}

void agent_requests_report(JSONNODE* j_rq_list) {
	JSONNODE *msg = json_new(JSON_NODE), *arg0 = json_new(JSON_NODE);
	JSONNODE *response;

	json_set_name(arg0, "requests");
	json_set_name(j_rq_list, "requests");

	json_push_back(arg0, j_rq_list);
	json_push_back(msg, arg0);

	clnt_invoke("requests_report", msg, &response);

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
			ADT_ARGUMENT("workloads", JSON_NODE),
			ADT_LAST_ARG()
		},
		.ad_method = agent_configure_workloads
	},
	{
		.ad_name = "start_workload",
		{
			ADT_ARGUMENT("workload_name", JSON_STRING),
			ADT_ARGUMENT("start_time", JSON_NUMBER),
			ADT_LAST_ARG()
		},
		.ad_method = agent_start_workload
	},
	{
	.ad_name = "provide_step",
		{
			ADT_ARGUMENT("workload_name", JSON_STRING),
			ADT_ARGUMENT("step_id", JSON_NUMBER),
			ADT_ARGUMENT("num_requests", JSON_NUMBER),
			ADT_LAST_ARG()
		},
		.ad_method = agent_provide_step
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
