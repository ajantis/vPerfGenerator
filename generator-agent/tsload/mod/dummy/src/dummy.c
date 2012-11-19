/*
 * dummy.c
 *
 *  Created on: 06.11.2012
 *      Author: myaut
 */

#define LOG_SOURCE "dummy"
#include <log.h>

#include <defs.h>
#include <workload.h>
#include <modules.h>
#include <modapi.h>

#include <dummy.h>

#include <stdlib.h>

#include <fcntl.h>
#include <unistd.h>

int mod_api_version = MODAPI_VERSION;

char mod_name[MODNAMELEN] = "dummy";
static char* tests[] =  {"read", "write"};

wlp_descr_t mod_params[] = {
	{ WLP_SIZE,
		WLP_SIZE_RANGE(1, 1 * TBYTE),
		"filesize",
		"Size of file which would be benchmarked",
		offsetof(struct dummy_workload, file_size) },
	{ WLP_SIZE,
		WLP_SIZE_RANGE(1, 16 * MBYTE),
		"blocksize",
		"Size of block which would be used",
		offsetof(struct dummy_workload, block_size)},
	{ WLP_RAW_STRING,
		WLP_STRING_LENGTH(512),
		"path",
		"Path to file",
		offsetof(struct dummy_workload, path)
	},
	{ WLP_STRING_SET,
		WLP_STRING_SET_RANGE(tests),
		"test",
		"Benchmark name (read, write)",
		offsetof(struct dummy_workload, test)
	},
	{ WLP_BOOL,
		WLP_NO_RANGE(),
		"sparse",
		"Allow sparse creation of data",
		offsetof(struct dummy_workload, sparse)
	},
	{ WLP_NULL }
};

size_t mod_params_size = sizeof(struct dummy_workload);

module_t* self = NULL;

int dummy_create_file(int fd, struct dummy_workload* dummy) {
	long i = 0, last = dummy->file_size / dummy->block_size;

	for(i = 0; i < last; ++i) {
		write(fd, dummy->block, dummy->block_size);
	}

	return 0;
}

int mod_config(module_t* mod) {
	logmsg(LOG_INFO, "Dummy module is loaded");

	self = mod;

	return 0;
}

int mod_unconfig(module_t* mod) {
	return 0;
}

int mod_workload_config(workload_t* wl) {
	struct dummy_workload* dummy = (struct dummy_workload*) wl->wl_params;
	int fd = 0;

	logmsg(LOG_INFO, "Creating file %s with size %ld", dummy->path, dummy->file_size);

	fd = open(dummy->path, O_WRONLY | O_CREAT, 0660);

	if(fd < 0) {
		mod_error(self, "Couldn't open file %s for writing", dummy->path);
		return -1;
	}

	dummy->block = malloc(dummy->block_size);
	if(dummy->sparse) {
		lseek(fd, dummy->file_size, SEEK_SET);
	}
	else {
		dummy_create_file(fd, dummy);
	}

	return 0;
}

int mod_workload_unconfig(workload_t* wl) {
	struct dummy_workload* dummy = (struct dummy_workload*) wl->wl_params;

	close(dummy->fd);

	return 0;
}
