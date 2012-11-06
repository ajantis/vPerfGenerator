/*
 * log.h
 *
 *  Created on: 24.10.2012
 *      Author: myaut
 */

#ifndef LOG_H_
#define LOG_H_

#ifndef LOG_SOURCE
#define LOG_SOURCE ""
#endif

#define LOGFNMAXLEN 128

#define LOGMAXSIZE 2 * 1024 * 1024

#define LOG_CRIT	0
#define LOG_WARN	1
#define LOG_INFO	2

#define LOG_DEBUG	3
#define LOG_TRACE	4

int log_init();
void log_fini();

int logerror();
int logmsg_src(int severity, char* source, char* format, ...);

#define logmsg(severity, ...) \
	logmsg_src(severity, LOG_SOURCE, __VA_ARGS__)

#endif /* LOG_H_ */
