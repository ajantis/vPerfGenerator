/*
 * uname.c
 *
 *  Created on: Dec 10, 2012
 *      Author: myaut
 */

#include <defs.h>
#include <uname.h>

#include <windows.h>

PLATAPIDECL(hi_get_nodename) const char nodename[64];

/*TODO: implement uname for MS Windows*/

/* Returns operating system name and version */
PLATAPI const char* hi_get_os_name() {
	return "Windows";
}

PLATAPI const char* hi_get_os_release() {
	return "3.11";
}

/* Returns nodename and domain name of current host*/
PLATAPI const char* hi_get_nodename() {
	GetComputerName(nodename, 64);

	return nodename;
}

PLATAPI const char* hi_get_domainname() {
	return "";
}

/* Returns machine architecture */
PLATAPI const char* hi_get_mach() {
	return "?";
}

