/*
 * tsinit.c
 *
 *  Created on: 30.12.2012
 *      Author: myaut
 */

#include <tsinit.h>

#include <windows.h>

typedef NTSTATUS (NTAPI *NtDelayExecution_func)(BOOLEAN Alertable, PLARGE_INTEGER DelayInterval);
NtDelayExecution_func NtDelayExecution;

int plat_read_ntdll(void) {
	HMODULE hModule = LoadLibrary("ntdll.dll");
	int ret = 0;

	if(!hModule) {
		return 1;
	}

	NtDelayExecution = (NtDelayExecution_func) GetProcAddress(hModule, "NtDelayExecution");

	if(!NtDelayExecution) {
		ret = 1;
	}

	FreeLibrary(hModule);

	return ret;
}

PLATAPI int plat_init(void) {
	/* Suppress messageboxes with failures */
	SetErrorMode(SEM_FAILCRITICALERRORS);

	return plat_read_ntdll();
}
