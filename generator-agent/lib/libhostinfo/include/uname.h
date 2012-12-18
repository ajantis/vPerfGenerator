/*
 * uname.h
 *
 *  Created on: Dec 10, 2012
 *      Author: myaut
 */

#ifndef UNAME_H_
#define UNAME_H_

#include <defs.h>

/* Returns operating system name and version */
PLATAPI const char* hi_get_os_name();
PLATAPI const char* hi_get_os_release();

/* Returns nodename and domain name of current host*/
PLATAPI const char* hi_get_nodename();
PLATAPI const char* hi_get_domainname();

/* Returns machine architecture */
PLATAPI const char* hi_get_mach();

#endif /* UNAME_H_ */
