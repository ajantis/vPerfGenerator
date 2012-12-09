/*
 * wlparam.c
 *
 *  Created on: Nov 12, 2012
 *      Author: myaut
 */

#define LOG_SOURCE "wlparam"
#include <log.h>

#include <modules.h>
#include <wlparam.h>
#include <agent.h>

#include <libjson.h>

#include <stdlib.h>
#include <string.h>

#define STRSETCHUNK		256

static JSONNODE* json_wlparam_strset_format(wlp_descr_t* wlp) {
	JSONNODE* node = json_new(JSON_ARRAY);
	JSONNODE* el;
	int i;
	char* str;

	json_set_name(node, "strset");

	for(i = 0; i < wlp->range.ss_num; ++i) {
		str = wlp->range.ss_strings[i];

		el = json_new(JSON_STRING);
		json_set_a(el, str);

		json_push_back(node, el);
	}

	return node;
}

JSONNODE* json_wlparam_format(wlp_descr_t* wlp) {
	JSONNODE* wlp_node = json_new(JSON_NODE);

	json_set_name(wlp_node, wlp->name);

	switch(wlp->type) {
	case WLP_BOOL:
		json_push_back(wlp_node, json_new_a("type", "bool"));
		break;
	case WLP_INTEGER:
		json_push_back(wlp_node, json_new_a("type", "integer"));
		json_push_back(wlp_node, json_new_i("min", wlp->range.i_min));
		json_push_back(wlp_node, json_new_i("max", wlp->range.i_max));
		break;
	case WLP_FLOAT:
		json_push_back(wlp_node, json_new_a("type", "float"));
		json_push_back(wlp_node, json_new_f("min", wlp->range.d_min));
		json_push_back(wlp_node, json_new_f("max", wlp->range.d_max));
		break;
	case WLP_SIZE:
		json_push_back(wlp_node, json_new_a("type", "size"));
		json_push_back(wlp_node, json_new_i("min", wlp->range.sz_min));
		json_push_back(wlp_node, json_new_i("max", wlp->range.sz_max));
		break;
	case WLP_RAW_STRING:
		json_push_back(wlp_node, json_new_a("type", "string"));
		json_push_back(wlp_node, json_new_i("len", wlp->range.str_length));
		break;
	case WLP_STRING_SET:
		json_push_back(wlp_node, json_new_a("type", "strset"));
		json_push_back(wlp_node, json_wlparam_strset_format(wlp));

		break;
	}

	json_push_back(wlp_node, json_new_a("description", wlp->description));

	return wlp_node;
}

JSONNODE* json_wlparam_format_all(wlp_descr_t* wlp) {
	JSONNODE* node = json_new(JSON_NODE);
	JSONNODE* wlp_node = NULL;

	json_set_name(node, "params");

	while(wlp->type != WLP_NULL) {
		wlp_node = json_wlparam_format(wlp);

		json_push_back(node, wlp_node);
		wlp++;
	}

	return node;
}

int json_wlparam_string_proc(JSONNODE* node, wlp_descr_t* wlp, void* param) {
	wlp_string_t* str = (wlp_string_t*) param;
	char* js = NULL;

	if(json_type(node) != JSON_STRING)
		return WLPARAM_JSON_WRONG_TYPE;

	js = json_as_string(node);

	if(strlen(js) > wlp->range.str_length)
		return WLPARAM_JSON_OUTSIDE_RANGE;

	strcpy(param, js);

	json_free(js);

	return WLPARAM_JSON_OK;
}


int json_wlparam_strset_proc(JSONNODE* node, wlp_descr_t* wlp, void* param) {
	int i;
	wlp_strset_t* ss = (wlp_strset_t*) param;
	char* js = NULL;

	if(json_type(node) != JSON_STRING)
		return WLPARAM_JSON_WRONG_TYPE;

	js = json_as_string(node);

	for(i = 0; i < wlp->range.ss_num; ++i) {
		if(strcmp(wlp->range.ss_strings[i], js) == 0) {
			*ss = i;

			json_free(js);

			return WLPARAM_JSON_OK;
		}
	}

	/* No match found - wrong value provided*/
	json_free(js);

	*ss = -1;
	return WLPARAM_JSON_OUTSIDE_RANGE;
}

#define WLPARAM_RANGE_CHECK(min, max) 							\
		if(*p < wlp->range.min || *p > wlp->range.max)			\
					return WLPARAM_JSON_OUTSIDE_RANGE;

#define WLPARAM_NO_RANGE_CHECK

#define WLPARAM_PROC_SIMPLE(type, json_node_type, 	\
		json_as_func, range_check) 					\
	{												\
		type* p = (type*) param;					\
		if(json_type(node) != json_node_type)		\
			return WLPARAM_JSON_WRONG_TYPE;			\
		*p = json_as_func(node);					\
		range_check;								\
	}

int json_wlparam_proc(JSONNODE* node, wlp_descr_t* wlp, void* param) {
	switch(wlp->type) {
	case WLP_BOOL:
		WLPARAM_PROC_SIMPLE(wlp_bool_t, JSON_BOOL,
				json_as_bool, WLPARAM_NO_RANGE_CHECK);
		break;
	case WLP_INTEGER:
		WLPARAM_PROC_SIMPLE(wlp_integer_t, JSON_NUMBER,
				json_as_int, WLPARAM_RANGE_CHECK(i_min, i_max));
		break;
	case WLP_FLOAT:
		WLPARAM_PROC_SIMPLE(wlp_float_t, JSON_NUMBER,
				json_as_float, WLPARAM_RANGE_CHECK(d_min, d_max));
		break;
	case WLP_SIZE:
		WLPARAM_PROC_SIMPLE(wlp_size_t, JSON_NUMBER,
				json_as_int, WLPARAM_RANGE_CHECK(sz_min, sz_max));
		break;
	case WLP_RAW_STRING:
		return json_wlparam_string_proc(node, wlp, param);
	case WLP_STRING_SET:
		return json_wlparam_strset_proc(node, wlp, param);
	}

	return WLPARAM_JSON_OK;
}

int json_wlparam_proc_all(JSONNODE* node, wlp_descr_t* wlp, void* params) {
	int ret;

	while(wlp->type != WLP_NULL) {
		JSONNODE_ITERATOR i_param = json_find(node, wlp->name),
						  i_end = json_end(node);

		if(i_param == i_end) {
			agent_error_msg(AE_INVALID_DATA, "Workload parameter %s not specified", wlp->name);
			return WLPARAM_JSON_NOT_FOUND;
		}

		ret = json_wlparam_proc(*i_param, wlp, params + wlp->off);

		if(ret == WLPARAM_JSON_WRONG_TYPE) {
			agent_error_msg(AE_INVALID_DATA, "Workload parameter %s has wrong type", wlp->name);
			return ret;
		}

		if(ret == WLPARAM_JSON_OUTSIDE_RANGE) {
			agent_error_msg(AE_INVALID_DATA, "Workload parameter %s outside defined range", wlp->name);
			return ret;
		}


		wlp++;
	}

	return WLPARAM_JSON_OK;
}
