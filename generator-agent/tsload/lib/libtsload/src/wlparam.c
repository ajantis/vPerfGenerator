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

#include <libjson.h>
#include <stdlib.h>

#define STRSETCHUNK		256

JSONNODE* json_gen_wlp_strset(wlp_descr_t* wlp) {
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

JSONNODE* json_wlparam_descr_format(wlp_descr_t* wlp) {
	JSONNODE* wlp_node = json_new(JSON_NODE);

	json_set_name(wlp_node, wlp->name);

	switch(wlp->type) {
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
		json_push_back(wlp_node, json_gen_wlp_strset(wlp));

		break;
	}

	json_push_back(wlp_node, json_new_a("description", wlp->description));

	return wlp_node;
}

JSONNODE* json_wlparam_format(wlp_descr_t* wlp) {
	JSONNODE* node = json_new(JSON_NODE);
	JSONNODE* wlp_node = NULL;
	json_char* json;

	json_set_name(node, "params");

	while(wlp->type != WLP_NULL) {
		wlp_node = json_wlparam_descr_format(wlp);

		json_push_back(node, wlp_node);
		wlp++;
	}

	return node;
}

int json_wlparam_proc(JSONNODE* node, wlp_descr_t* wlp, void* param) {
	switch(wlp->type) {
	case WLP_INTEGER:
	{
		wlp_integer_t* l = (wlp_integer_t*) param;

		if(json_type(node) != JSON_NUMBER)
			return WLPARAM_JSON_WRONG_TYPE;

		*l = json_as_int(node);

		if(*l < wlp->range.i_min || *l > wlp->range.i_max)
			return WLPARAM_JSON_OUTSIDE_RANGE;
	}
		break;
	case WLP_FLOAT:
	{
		wlp_float_t* f = (wlp_float_t*) param;

		if(json_type(node) != JSON_NUMBER)
			return WLPARAM_JSON_WRONG_TYPE;

		*f = json_as_float(node);

		if(*f < wlp->range.d_min || *f > wlp->range.d_max)
			return WLPARAM_JSON_OUTSIDE_RANGE;
	}
		break;
	case WLP_SIZE:
	{
		wlp_size_t* sz = (wlp_size_t*) param;

		if(json_type(node) != JSON_NUMBER)
			return WLPARAM_JSON_WRONG_TYPE;

		*sz = json_as_int(node);

		if(*sz < wlp->range.sz_min || *sz > wlp->range.sz_max)
			return WLPARAM_JSON_OUTSIDE_RANGE;
	}
		break;
	case WLP_RAW_STRING:
	{
		wlp_string_t* str = (wlp_size_t*) param;
		char* js = NULL;

		if(json_type(node) != JSON_STRING)
			return WLPARAM_JSON_WRONG_TYPE;

		js = json_as_string(node);

		if(strlen(js) > wlp->range.str_length)
			return WLPARAM_JSON_OUTSIDE_RANGE;

		strcpy(param, js);
	}
		break;
	case WLP_STRING_SET:
		/*NOTIMPLEMENTED*/

		break;

	}

	return WLPARAM_JSON_OK;
}

int json_wlparam_proc_all(JSONNODE* node, wlp_descr_t* wlp, void* params) {
	while(wlp->type != WLP_NULL) {
		JSONNODE_ITERATOR i_param = json_find(node, wlp->name);

		if(i_param == NULL) {
			logmsg(LOG_WARN, "Coulnd't find workload parameter %s", wlp->name);
			return -1;
		}

		json_wlparam_proc(*i_param, wlp, params + wlp->off);

		wlp++;
	}
}
