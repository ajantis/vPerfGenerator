/*
 * log.c
 *
 *  Created on: 24.10.2012
 *      Author: myaut
 */

#include <log.h>

#include <libjson.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>

char log_filename[LOGFNMAXLEN];

/* By default, debug and tracing are disabled
 * */
int log_debug 	= 0;
int log_trace	= 0;

FILE* log_file;

pthread_mutex_t	log_mutex;

const char* log_severity[] =
	{"CRIT", "WARN", "INFO", "_DBG", "_TRC" };

/*Used by libjson*/
static void json_error_callback(const char* msg) {
	logmsg_src(LOG_DEBUG, "JSON", msg);
}

/* Rotate tsload logs
 *
 * if logfile size is greater than LOGMAXSIZE (2M) by default,
 * it moves logfile to .old file*/
int log_rotate() {
	char old_log_filename[LOGFNMAXLEN + 4];
	struct stat log_stat;

	if(stat(log_filename, &log_stat) == -1) {
		fprintf(stderr, "log_rotate -> stat error: %s\n", strerror(errno));

		return errno;
	}

	if(log_stat.st_size < LOGMAXSIZE)
		return 0;

	strcpy(old_log_filename, log_filename);
	strcat(old_log_filename, ".old");

	if(access(old_log_filename, F_OK) != -1)
		remove(old_log_filename);

	rename(log_filename, old_log_filename);

	return 0;
}

int log_init() {
	int ret;

	pthread_mutex_init(&log_mutex, NULL);

	if(strcmp(log_filename, "-") == 0) {
		log_file = stdout;
		return 0;
	}

	if((ret = log_rotate()) != 0)
		return ret;
	log_file = fopen(log_filename, "a");

	json_register_debug_callback(json_error_callback);

	return 0;
}

void log_fini() {
	fclose(log_file);
}

void log_gettime(char* buf, int sz) {
	time_t rawtime;
	struct tm* timeinfo;

	time ( &rawtime );
	timeinfo = localtime ( &rawtime );

	strftime(buf, sz, "%c", timeinfo);
}

/**
 * Log message to default logging location
 * No need to add \n to format string
 *
 * @param severity - level of logging (LOG_CRIT, LOG_WARN, LOG_INFO, LOG_DEBUG, LOG_TRACING)
 * @param source - origin of message
 * @param format - printf format
 *
 * @return number of written symbols or -1 if message was discarded due to severity
 */
int logmsg_src(int severity, const char* source, const char* format, ...)
	{
	char time[64];
	va_list args;
	int ret = 0;

	if((severity == LOG_DEBUG && log_debug == 0) ||
	   (severity == LOG_TRACE && log_trace == 0) ||
	    severity > LOG_TRACE || severity < 0)
			return -1;


	log_gettime(time, 64);

	pthread_mutex_lock(&log_mutex);

	ret = fprintf(log_file, "%s [%s:%s] ", time, source, log_severity[severity]);

	va_start(args, format);
	ret += vfprintf(log_file, format, args);
	va_end(args);

	fputc('\n', log_file);
	fflush(log_file);

	pthread_mutex_unlock(&log_mutex);

	return ret + 1;	/*+1 for \n*/
}

/* Log error provided by errno
 * */
int logerror() {
	return logmsg_src(LOG_CRIT, "error", "Error: %s", strerror(errno));
}
