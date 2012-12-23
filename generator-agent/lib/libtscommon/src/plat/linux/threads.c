/*
 * threads.c
 *
 *  Created on: 23.12.2012
 *      Author: myaut
 */

#include <defs.h>

#include <sys/syscall.h>

PLATAPI unsigned long plat_gettid() {
	return syscall(__NR_gettid);
}
