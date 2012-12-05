/*
 * atomic.h
 *
 *  Created on: 05.12.2012
 *      Author: myaut
 */

#ifndef ATOMIC_H_
#define ATOMIC_H_

typedef long atomic_t;

static long atomic_read(atomic_t* atom) {
	return __sync_fetch_and_add(atom, 0);
}

static atomic_set(atomic_t* atom, long value) {
	 (void) __sync_lock_test_and_set(atom, value);
}

static long atomic_inc_ret(atomic_t* atom) {
	return __sync_fetch_and_add(atom, 1);
}

#endif /* ATOMIC_H_ */
