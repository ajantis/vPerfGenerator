/*
 * load.c
 *
 *  Created on: Nov 19, 2012
 *      Author: myaut
 */

#define LOG_SOURCE "load"
#include <log.h>

#include <list.h>
#include <workload.h>

#include <commands.h>

#include <libjson.h>

#include <stdio.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>

char wl_filename[MAXFNLEN];
char step_filename[MAXFNLEN];

JSONNODE* wl_node = NULL;

/**
 * Read JSON representation of workloads from file
 *
 * @note: Uses mmap because it is easier than reading entire file into
 * large memory buffer
 *
 * @param fd: file descriptor
 * @param length: length of file
 *
 * @return LOAD_OK if everything was OK or LOAD_ERR_ constant
 */
int wl_read(int fd, size_t length) {
	int ret = 0;
	char* wl_map = (char*) mmap(
				NULL, length, PROT_READ, MAP_SHARED, fd, 0);

	if(wl_map == NULL) {
		logmsg(LOG_CRIT, "Failed to do mmapping of file %s", wl_filename);
		return LOAD_ERR_MMAP_FAIL;
	}

	wl_node = json_parse(wl_map);

	if(wl_node == NULL) {
		logmsg(LOG_CRIT, "JSON parse error for file %s", wl_filename);
		ret = LOAD_ERR_BAD_JSON;
	}

	munmap(wl_map, length);

	return ret;
}

/**
 * Open file wl_filename and read workloads from that
 */
int wl_init() {
	int ret = LOAD_OK;
	int fd = open(wl_filename, O_RDONLY);
	size_t length = 0;

	if(fd < 0) {
		logmsg(LOG_CRIT, "Couldn't open workload file %s", wl_filename);
		return LOAD_ERR_OPEN_FAIL;
	}

	length = lseek(fd, 0, SEEK_END);

	ret = wl_read(fd, length);

	close(fd);

	return ret;
}

/**
 * Load system with requests
 */
int do_load() {
	int err = LOAD_OK;
	workload_t* wl = NULL;
	list_head_t wl_list;

	list_head_init(&wl_list, "wl_list");

	if((err = wl_init()) != LOAD_OK)
		return err;

	json_workload_proc_all(wl_node, &wl_list);

	if(list_empty(&wl_list))
		return LOAD_ERR_JSON_PROC;

	wl_config(wl);

	/*TODO: load code goes here*/

	wl_unconfig(wl);

	return LOAD_OK;
}
