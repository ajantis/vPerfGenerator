/*
 * tstime.h
 *
 *  Created on: 06.12.2012
 *      Author: myaut
 */

#ifndef TS_TIME_H_
#define TS_TIME_H_

#include <defs.h>
#include <stdint.h>
#include <limits.h>

/**
 * 64-bit integer is enough to keep time up to XXIIIth century
 * with nanosecond precision. I hope, that we will release patch until then.
 * */
typedef int64_t ts_time_t;

#define TS_TIME_MAX 	ULLONG_MAX

/**
 * Returns time since Epoch.
 * Guarantees millisecond precision */
LIBEXPORT PLATAPI ts_time_t tm_get_time();

/**
 * Returns clock (monotonic time since some unspecified starting point)
 * Guarantees 100-nanosecond precision (Windows) or better (Unix) */
LIBEXPORT PLATAPI ts_time_t tm_get_clock();

/**
 * Returns clock's resolution */
LIBEXPORT PLATAPI ts_time_t tm_get_clock_res();

LIBEXPORT ts_time_t tm_diff(ts_time_t a, ts_time_t b);

/**
 * Sleep with millisecond precision * */
LIBEXPORT PLATAPI void tm_sleep_milli(ts_time_t t);

/**
 * Sleep with nanosecond precision */
LIBEXPORT PLATAPI void tm_sleep_nano(ts_time_t t);

LIBEXPORT ts_time_t tm_ceil_diff(ts_time_t tm, ts_time_t precision);


#endif /* TS_TIME_H_ */
