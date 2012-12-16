/*
 * util.h
 *
 *  Created on: 09.12.2012
 *      Author: myaut
 */

#ifndef UTIL_H_
#define UTIL_H_

/* Contains various utilities */

#define container_of(ptr, type, member) ({					\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (char *)__mptr - offsetof(type,member) );})

#define uninitialized_var(x) x = x

# ifndef likely
#  define likely(x)	(__builtin_expect(!!(x), 1))
# endif
# ifndef unlikely
#  define unlikely(x)	(__builtin_expect(!!(x), 0))
# endif

#endif /* UTIL_H_ */
