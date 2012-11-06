/*
 * dummy.c
 *
 *  Created on: 06.11.2012
 *      Author: myaut
 */

#define LOG_SOURCE "dummy"
#include <log.h>

#include <modules.h>
#include <modapi.h>

int mod_api_version = MODAPI_VERSION;

char mod_name[MODNAMELEN] = "dummy";

int mod_config(module_t* mod) {
	logmsg(LOG_INFO, "Dummy module is loaded");

	return 0;
}
