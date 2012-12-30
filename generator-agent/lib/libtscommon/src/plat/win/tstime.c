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
	FILETIME time;
	ts_time_t now;

	/*See http://support.microsoft.com/kb/167296 */

	GetSystemTimeAsFileTime(&time);

	now = ((LONGLONG) time.dwHighDateTime) << 32 | time.dwLowDateTime;
	now = (now - 116444736000000000) * 100;

	return now;
}

PLATAPI void tm_sleep(ts_time_t t) {
	Sleep(t / T_MS);
}
