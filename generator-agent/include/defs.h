/*
 * defs.h
 *
 *  Created on: 08.11.2012
 *      Author: myaut
 *
 *  Contains useful definitions
 */

#ifndef DEFS_H_
#define DEFS_H_

#include <stddef.h>

#ifndef PLAT_SOLARIS
typedef enum { B_FALSE, B_TRUE } boolean_t;
#else
/* In Solaris boolean defined in <sys/types.h> */
#include <sys/types.h>
#endif

#define SZ_KB	(1024ll)
#define SZ_MB 	(1024ll * SZ_KB)
#define SZ_GB	(1024ll * SZ_MB)
#define SZ_TB	(1024ll * SZ_GB)

/*All time periods measured in ns*/
#define T_NS 		1ll
#define T_US		(1000ll * T_NS)
#define T_MS		(1000ll * T_US)
#define T_SEC		(1000ll * T_MS)

/* Platform API */
#define PLATAPI
#define PLATAPIDECL(...)

/* Checkformat provides warnings for incorrect printf format */
#ifdef __GNUC__
#	define CHECKFORMAT(func, arg1, arg2) __attribute__ ((format (func, arg1, arg2)))
#else
#	define CHECKFORMAT(func, arg1, arg2)
#endif

/* Initialize structure member
 * VisualC doesn't support designated initializers (C99), so
 * it cannot use form .member = value */
#ifdef __GNUC__
#	define SM_INIT(member, value)   member = value
#else
#	define SM_INIT(member, value)	value
#endif

#ifdef _MSC_VER
#	define STATIC_INLINE static __inline
#else
#	define STATIC_INLINE static inline
#endif

#define container_of(ptr, type, member) ((type*) ((char*)ptr - offsetof(type, member)))

#define uninitialized_var(x) x = x

/* likely and unlikely conditional optimizations */
#ifdef __GNUC__
#  define likely(x)		(__builtin_expect(!!(x), 1))
#  define unlikely(x)	(__builtin_expect(!!(x), 0))
#else
#  define likely(x)		x
#  define unlikely(x)	x
# endif

/* On Windows we should define special markings for exported functions
 * and variables from shared libraries.
 *
 * Use LIBEXPORT in header for functions
 * Use LIBIMPORT for shared data in executable code*/
#ifdef _MSC_VER
#define LIBEXPORT			__declspec(dllexport)
#define LIBIMPORT			__declspec(dllimport)
#else
#define LIBEXPORT
#define LIBIMPORT			extern
#endif

/* For GNU C we need to use attributes with packed structures
 * For VC++ it is defined using #pragma pack(...). GNU C supports for compability */
#ifdef __GNUC__
#  define PACKED_STRUCT		__attribute__((packed))
#else
#  define PACKED_STRUCT
# endif

/* Define min and max macroses.
 * VS has it's own min/max implementation, so use it */
#ifndef _MSC_VER
#	define min(a, b) ((a) < (b)? (a) : (b))
#	define max(a, b) ((a) > (b)? (a) : (b))
#else
#	include <stdlib.h>
#endif

#include <config.h>

#endif /* DEFS_H_ */
