/*
 * dummy.c
 *
 *  Created on: 06.11.2012
 *      Author: myaut
 */

#define LOG_SOURCE "dummy"
#include <log.h>

#include <mempool.h>
#include <defs.h>
#include <workload.h>
#include <modules.h>
#include <modapi.h>

#include <dummy.h>

#include <plat/posixdecl.h>

#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <fcntl.h>

#include <sys/ioctl.h>
#include <linux/fs.h>

DECLARE_MODAPI_VERSION(MOD_API_VERSION);
DECLARE_MOD_NAME("dummy");
DECLARE_MOD_TYPE(MOD_TSLOAD);

static char* tests[] =  {"read", "write"};

MODEXPORT wlp_descr_t mod_params[] = {
	{ WLP_SIZE, WLPF_OPTIONAL,
		WLP_SIZE_RANGE(1, 1 * SZ_TB),
		WLP_SIZE_DEFAULT(0),
		"filesize",
		"Size of file which would be benchmarked",
		offsetof(struct dummy_workload, file_size) },
	{ WLP_SIZE, WLPF_NO_FLAGS,
		WLP_SIZE_RANGE(1, 16 * SZ_MB),
		WLP_NO_DEFAULT(),
		"blocksize",
		"Size of block which would be used",
		offsetof(struct dummy_workload, block_size)},
	{ WLP_RAW_STRING, WLPF_NO_FLAGS,
		WLP_STRING_LENGTH(512),
		WLP_NO_DEFAULT(),
		"path",
		"Path to file",
		offsetof(struct dummy_workload, path)
	},
	{ WLP_STRING_SET, WLPF_NO_FLAGS,
		WLP_STRING_SET_RANGE(tests),
		WLP_NO_DEFAULT(),
		"test",
		"Benchmark name (read, write)",
		offsetof(struct dummy_workload, test)
	},
	{ WLP_BOOL, WLPF_NO_FLAGS,
		WLP_NO_RANGE(),
		WLP_NO_DEFAULT(),
		"sparse",
		"Allow sparse creation of data",
		offsetof(struct dummy_workload, sparse)
	},
	{ WLP_BOOL, WLPF_NO_FLAGS,
		WLP_NO_RANGE(),
		WLP_NO_DEFAULT(),
		"sync",
		"Use synchronious I/O",
		offsetof(struct dummy_workload, sync)
	},
	{ WLP_NULL }
};

MODEXPORT size_t mod_params_size = sizeof(struct dummy_workload);

module_t* self = NULL;

int dummy_write_file(workload_t* wl, int fd, struct dummy_workload* dummy) {
	size_t i = 0, last = dummy->file_size / dummy->block_size;
	float one_percent = ((float) last) / 100;
	int last_notify = 0, done = 0;

	for(i = 0; i < last; ++i) {
		if(write(fd, dummy->block, dummy->block_size) == -1) {
			wl_notify(wl, WLS_FAIL, 0,
					"Failed to write: %s", strerror(errno));
			return -1;
		}

		done = (int) (i / one_percent);

		if(done > last_notify) {
			wl_notify(wl, WLS_CONFIGURING, done,
					"Written %lu bytes", i * dummy->block_size);
			last_notify = done;
		}
	}

	return 0;
}

int dummy_create_file(workload_t* wl, struct dummy_workload* dummy) {
	int fd = open(dummy->path, O_WRONLY | O_CREAT, 0660);

	if(fd < 0) {
		wl_notify(wl, WLS_FAIL, 0,
					"Couldn't open file %s for writing: %s", dummy->path, strerror(errno));
		return -1;
	}

	if(dummy->sparse) {
		lseek(fd, dummy->file_size, SEEK_SET);
	}
	else {
		dummy_write_file(wl, fd, dummy);
	}

	close(fd);
}

wlp_size_t dummy_get_file_size(const char* path, struct stat s) {
	/*TODO: Error checking, multi-platform*/

	if((s.st_mode & S_IFMT) == S_IFREG) {
		return 512 * s.st_blocks;
	}
	else if((s.st_mode & S_IFMT) == S_IFBLK) {
		uint64_t file_size;
		int fd;

		fd = open(path, O_RDONLY);
		ioctl(fd, BLKGETSIZE64, &file_size);
		close(fd);

		return file_size;
	}

	/*Wrong file*/
}

MODEXPORT int mod_config(module_t* mod) {
	logmsg(LOG_INFO, "Dummy module is loaded");

	self = mod;

	return 0;
}

MODEXPORT int mod_unconfig(module_t* mod) {
	return 0;
}

MODEXPORT int mod_workload_config(workload_t* wl) {
	struct dummy_workload* dummy = (struct dummy_workload*) wl->wl_params;
	int fd = 0;

	struct stat s;

	logmsg(LOG_INFO, "Creating file %s with size %lld", dummy->path, dummy->file_size);

	if(dummy->file_size != 0) {
		dummy_create_file(wl, dummy);
	}

	if(stat(dummy->path, &s) == -1) {
		wl_notify(wl, WLS_FAIL, 0,
					"Couldn't stat target file %s: %s", dummy->path, strerror(errno));
		return -1;
	}

	dummy->file_size = dummy_get_file_size(dummy->path, s);
	dummy->block = mp_malloc(dummy->block_size);
	dummy->fd = open(dummy->path,
				(dummy->test == DUMMY_WRITE) ? O_WRONLY : O_RDONLY |
				(dummy->sync)? O_DSYNC : 0,
				0660);

	return 0;
}

MODEXPORT int mod_workload_unconfig(workload_t* wl) {
	struct dummy_workload* dummy = (struct dummy_workload*) wl->wl_params;

	close(dummy->fd);

	if(dummy->file_size != 0) {
		/*We had created file - remove it now*/
		remove(dummy->path);
	}

	mp_free(dummy->block);

	return 0;
}

MODEXPORT int mod_run_request(request_t* rq) {
	struct dummy_workload* dummy = (struct dummy_workload*) rq->rq_workload->wl_params;
	int ret;

	unsigned block = rand() % (dummy->file_size / dummy->block_size);

	/* Select random block */
	lseek(dummy->fd, block * dummy->block_size, SEEK_SET);

	/* FIXME: read/writes to same buffer: need request context */
	switch(dummy->test) {
	case DUMMY_READ:
		ret = read(dummy->fd, dummy->block, dummy->block_size);
		break;
	case DUMMY_WRITE:
		ret = write(dummy->fd, dummy->block, dummy->block_size);
		break;
	}

	return (ret == dummy->block_size)? 0 : -1;
}
