/*
 * uname.h
 *
 *  Created on: Dec 10, 2012
 *      Author: myaut
 */

#ifndef UNAME_H_
#define UNAME_H_

/* Returns operating system name and version */
const char* hi_get_os_name();
const char* hi_get_os_release();

/* Returns nodename and domain name of current host*/
const char* hi_get_nodename();
const char* hi_get_domainname();

/* Returns machine architecture */
const char* hi_get_mach();

#endif /* UNAME_H_ */
