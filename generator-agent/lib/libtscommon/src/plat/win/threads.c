/*
 * threads.c
 *
 *  Created on: 20.12.2012
 *      Author: myaut
 */

#include <defs.h>
#include <plat/win/threads.h>

#include <threads.h>

#include <windows.h>

PLATAPI void plat_thread_init(plat_thread_t* thread, void* arg,
		  	  	  	  	  	  void* (*start)(void*)) {


}

PLATAPI void plat_thread_destroy(plat_thread_t* thread) {

}

PLATAPI unsigned long plat_gettid() {
	return 0;
}

PLATAPI void t_eternal_wait(void) {
	Sleep(INFINITE);
}
