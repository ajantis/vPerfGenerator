/*
 * tstime.h
 *
 *  Created on: 06.12.2012
 *      Author: myaut
 */

#ifndef TS_TIME_H_
#define TS_TIME_H_

#include <stdint.h>
#include <limits.h>

/**
 * 64-bit integer is enough to keep time until Mon Jul 22 03:34:33 2554 MSK
 * with nanosecond precision. I hope, that we will release patch until then.
 * */
typedef uint64_t ts_time_t;

#define TS_TIME_MAX 	ULLONG_MAX

ts_time_t tm_get_time();
ts_time_t tm_diff(ts_time_t a, ts_time_t b);

void tm_sleep(ts_time_t t);

ts_time_t tm_ceil_diff(ts_time_t tm, ts_time_t precision);


#endif /* TS_TIME_H_ */
