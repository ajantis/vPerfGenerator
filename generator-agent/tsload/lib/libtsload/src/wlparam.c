/*
 * wlparam.c
 *
 *  Created on: Nov 12, 2012
 *      Author: myaut
 */

#include <wlparam.h>

#include <libjson.h>

#define STRSETCHUNK		256

char* json_gen_wlp_strset(wlp_descr_t* wlp) {
	size_t len = STRSETCHUNK, alen = 0;
	char* str = (char*) malloc(len);
	char* temp, *astr;	/*for realloc*/
	int i;

	str[0] = 0;

	for(i = 0; i < wlp->range.ss_num; ++i) {
		astr = wlp->range.ss_strings[i];
		alen = strlen(astr);

		/*Reserve bytes for delim and NULL-terminator*/
		if(alen > (len - 2)) {
			/*reallocate str*/
			while(alen > len)
				len += STRSETCHUNK;

			temp = (char*) malloc(len);
			strcpy(temp, str);
			free(str);
			str = temp;
		}

		strcat(str, astr);
	}

	return str;
}

JSONNODE* json_gen_wlp_descr(wlp_descr_t* wlp) {
	JSONNODE* wlp_node = json_new(JSON_NODE);
	char* string_set;
	int i;

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

		string_set = json_gen_wlp_strset(wlp);
		json_push_back(wlp_node, json_new_a("strset", string_set));
		free(string_set);

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
