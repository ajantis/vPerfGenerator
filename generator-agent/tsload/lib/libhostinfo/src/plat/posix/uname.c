/*
 * uname.c
 *
 *  Created on: Dec 10, 2012
 *      Author: myaut
 */

#include <defs.h>

#include <sys/utsname.h>

int uname_read = FALSE;
struct utsname uname_data;

static inline struct utsname* hi_get_uname() {
	if(!uname_read)
		uname(&uname_data);

	return &uname_data;
}

/* Returns operating system name and version */
const char* hi_get_os_name() {
	/*FIXME: Linux distibution names*/
	return hi_get_uname()->sysname;
}

const char* hi_get_os_release() {
	return hi_get_uname()->release;
}

/* Returns nodename and domain name of current host*/
const char* hi_get_nodename() {
	return hi_get_uname()->nodename;
}

const char* hi_get_domainname() {
	/*FIXME: solaris/bsd implementation*/
	return hi_get_uname()->__domainname;
}

/* Returns machine architecture */
const char* hi_get_mach() {
	return hi_get_uname()->machine;
}

