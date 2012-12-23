/*
 * tstime.c
 *
 *  Created on: 22.12.2012
 *      Author: myaut
 */

#include <defs.h>
#include <tstime.h>

#include <windows.h>

/*FIXME: should be ns-precise time source*/
PLATAPI ts_time_t tm_get_time() {
	return GetTickCount() * T_MS;
}

PLATAPI void tm_sleep(ts_time_t t) {
	Sleep(t / T_MS);
}
