/*
 * config.c
 *
 *  Created on: Nov 19, 2012
 *      Author: myaut
 */

#include <log.h>
#include <modules.h>

#include <cfgfile.h>

#include <stdio.h>
#include <string.h>
#include <ctype.h>

extern char log_filename[];
extern char mod_search_path[];

int cfg_option(char* name, char* value) {
	if(strcmp(name, "MODPATH") == 0) {
		strncpy(mod_search_path, value, MODPATHLEN);
	}
	else if(strcmp(name, "LOGFILE") == 0) {
		strncpy(log_filename, value, LOGFNMAXLEN);
	}
	else {
		return CFG_ERR_INVALID_OPTION;
	}

	return CFG_OK;
}

int cfg_read(FILE* cfg_file) {
	char line[CONFLINELEN];
	char* eq_pos = NULL;		/*Position of '=' sign*/
	size_t len;
	int line_num = 0;

	while(!feof(cfg_file)) {
		fgets(line, CONFLINELEN, cfg_file);
		len = strlen(line);

		/*Strip line*/
		while(isspace(line[--len]) && len != 0);
		line[len + 1] = 0;

		if(line[0] == '#')
			continue;

		eq_pos = strchr(line, '=');

		/*Ignore line without =*/
		if(!eq_pos) {
			fprintf(stderr, "Not found '=' on line", line_num);
			return CFG_ERR_MISSING_EQ;
		}

		/*Split line into two substrings*/
		*eq_pos = '\0';

		if(cfg_option(line, eq_pos + 1) != 0) {
			fprintf(stderr, "Invalid option %s on line %d", line, line_num);
			return CFG_ERR_INVALID_OPTION;
		}

		line_num++;
	}

	return CFG_OK;
}

int cfg_init(const char* cfg_file_name) {
	int ret = CFG_OK;

	FILE* cfg_file = fopen(cfg_file_name, "r");

	if(!cfg_file) {
		fprintf(stderr, "Failed to open file '%s'\n", cfg_file_name);
		return CFG_ERR_NOFILE;
	}

	ret = cfg_read(cfg_file);

	fclose(cfg_file);

	return ret;
}
