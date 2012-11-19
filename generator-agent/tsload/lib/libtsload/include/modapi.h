/*
 * modapi.h
 *
 *  Created on: 06.11.2012
 *      Author: myaut
 */

#ifndef MODAPI_H_
#define MODAPI_H_

#define MODNAMELEN			   32
#define MODAPI_VERSION		0x0001

struct module;
struct workload;

typedef int (* mod_config_func)(struct module* mod);
typedef int (* mod_workload_config_func)(struct workload* wl);

#define DECLARE_MODAPI_VERSION() \
		int mod_api_version = MOD_API_VERSION
#define DECLARE_MOD_NAME(name)	\
	char mod_name[MODNAMELEN] = name


#endif /* MODAPI_H_ */
