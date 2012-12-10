/*
 * modapi.h
 *
 *  Created on: 26.11.2012
 *      Author: myaut
 */

#ifndef MODAPI_H_
#define MODAPI_H_

#define MODNAMELEN			    32

#define MOD_API_VERSION		0x0001

#define MOD_TSLOAD		1
#define MOD_MONITOR		2

#define DECLARE_MODAPI_VERSION(version) \
		int mod_api_version = version
#define DECLARE_MOD_TYPE(type) \
		int mod_type = type
#define DECLARE_MOD_NAME(name)	\
	char mod_name[MODNAMELEN] = name

struct module;
typedef int (* mod_config_func)(struct module* mod);

#endif /* MODAPI_H_ */
