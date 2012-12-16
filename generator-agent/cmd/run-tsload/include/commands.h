/*
 * commands.h
 *
 *  Created on: Nov 19, 2012
 *      Author: myaut
 */

#ifndef COMMANDS_H_
#define COMMANDS_H_

#define LOAD_OK				0
#define LOAD_ERR_OPEN_FAIL	-1
#define LOAD_ERR_MMAP_FAIL	-2
#define LOAD_ERR_BAD_JSON	-3
#define LOAD_ERR_JSON_PROC	-4

#define MAXFNLEN	512

int do_info();
int do_load();


#endif /* COMMANDS_H_ */
