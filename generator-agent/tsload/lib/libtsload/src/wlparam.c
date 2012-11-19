/*
 * wlparam.c
 *
 *  Created on: Nov 12, 2012
 *      Author: myaut
 */

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

JSONNODE* json_gen_wlp_descr(wlp_descr_t* wlp) {
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

json_char* json_gen_wlp(wlp_descr_t* wlp) {
	JSONNODE* node = json_new(JSON_NODE);
	JSONNODE* wlp_node = NULL;
	json_char* json;

	while(wlp->type != WLP_NULL) {
		wlp_node = json_gen_wlp_descr(wlp);

		json_push_back(node, wlp_node);
		wlp++;
	}

	json = json_write(node);
	json_delete(node);

	return json;
}
