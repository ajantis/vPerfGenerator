/*
 * log.c
 *
 *  Created on: 02.01.2013
 *      Author: myaut
 */

#include <defs.h>
#include <log.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <execinfo.h>

PLATAPI int plat_get_callers(char* callers, size_t size) {
	int nptrs, j;
	void *buffer[LOG_MAX_BACKTRACE + 2];
	char **strings;

	char* fname;
	int ret;

	nptrs = backtrace(buffer, LOG_MAX_BACKTRACE);
	strings = backtrace_symbols(buffer, nptrs);

	for(j = 2; j < nptrs; j++) {
		fname = strchr(strings[j], '(');
		fname = (fname == NULL) ? strings[j] : fname;

		ret = snprintf(callers, size, "\n\t\t%s", fname);

		if(ret < 0 || ret >= size)
			break;

		size -= ret;
		callers += ret;
	}

	free(strings);

	return 0;
}
