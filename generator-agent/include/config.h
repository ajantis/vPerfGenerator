/*
 * config.h
 *
 *  Created on: 03.01.2013
 *      Author: myaut
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include <genconfig.h>

/*
 * Mempool configuration options
 * ----------------------------- */

/*! MEMPOOL_TRACE - allow additional tracing for mempool allocator
 * (controlled by mp_trace_bitmaps and mp_trace_allocator flags) */
#undef MEMPOOL_TRACE

/*! MEMPOOL_USE_VALGRIND - add valgrind macroses to mempool allocator
 */
#ifdef HAVE_VALGRIND_VALGRIND_H
#define MEMPOOL_USE_VALGRIND
#endif

#endif /* CONFIG_H_ */
