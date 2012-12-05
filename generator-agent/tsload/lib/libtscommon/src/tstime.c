/*
 * time.c
 *
 *  Created on: 06.12.2012
 *      Author: myaut
 */

#include <defs.h>
#include <tstime.h>

#include <sys/time.h>

/*FIXME: should be ns-precise time source*/

ts_time_t tm_get_time() {
	struct timeval tv;
	gettimeofday(&tv, NULL);

	return tv->tv_usec * US + tv->tv_sec * SEC;
}

ts_time_t tm_diff(ts_time_t a, ts_time_t b) {
	return b - a;
}

void tm_sleep(ts_time_t t) {
	usleep(t / US);
}
