/*
 * info.c
 *
 *  Created on: Nov 19, 2012
 *      Author: myaut
 */

#include <modules.h>

#include <libjson.h>

#include <stdio.h>

int do_info() {
	JSONNODE* node = json_modules_info();
	char* info = json_write_formatted(node);

	fputs(info, stdout);
	fputc('\n', stdout);

	json_free(info);

	return 0;
}

