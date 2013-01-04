/*
 * config.h
 *
 *  Created on: 03.01.2013
 *      Author: myaut
 */

#ifndef CONFIG_H_
#define CONFIG_H_

/*
 * Mempool configuration options
 * ----------------------------- */

/* MEMPOOL_TRACE - allow additional tracing for mempool allocator
 * (controlled by mp_trace_bitmaps and mp_trace_allocator flags) */
/* #define MEMPOOL_TRACE */

/* MEMPOOL_USE_VALGRIND - add valgrind macroses to mempool allocator
 */
/* #define MEMPOOL_USE_VALGRIND */

/* MEMPOOL_USE_LIBC_HEAP - use libc's heap for mp_malloc/mp_realloc/mp_free
 */
/* #define MEMPOOL_USE_LIBC_HEAP */

#endif /* CONFIG_H_ */
