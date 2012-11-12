/*
 * dummy.c
 *
 *  Created on: 06.11.2012
 *      Author: myaut
 */

#define LOG_SOURCE "dummy"
#include <log.h>

#include <defs.h>
#include <wlparam.h>
#include <modules.h>
#include <modapi.h>

#include <dummy.h>

int mod_api_version = MODAPI_VERSION;

char mod_name[MODNAMELEN] = "dummy";

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
	{ WLP_NULL }
};

int mod_config(module_t* mod) {
	logmsg(LOG_INFO, "Dummy module is loaded");


	return 0;
}
