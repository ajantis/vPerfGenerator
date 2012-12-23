/*
 * tstime.c
 *
 *  Created on: 22.12.2012
 *      Author: myaut
 */

#include <defs.h>
#include <tstime.h>

#include <unistd.h>
#include <sys/time.h>

/*FIXME: should be ns-precise time source*/
PLATAPI ts_time_t tm_get_time() {
	struct timeval tv;
	gettimeofday(&tv, NULL);

	return tv.tv_usec * T_US + tv.tv_sec * T_SEC;
}

PLATAPI void tm_sleep(ts_time_t t) {
	usleep(t / T_US);
}
