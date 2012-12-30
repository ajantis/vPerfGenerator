/*
 * tsinit.c
 *
 *  Created on: 30.12.2012
 *      Author: myaut
 */

#include <tsinit.h>

#include <windows.h>

PLATAPI int plat_init(void) {
	/* Suppress messageboxes with failures */
	SetErrorMode(SEM_FAILCRITICALERRORS);

	return 0;
}
