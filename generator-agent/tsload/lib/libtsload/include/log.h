/*
 * log.h
 *
 *  Created on: 24.10.2012
 *      Author: myaut
 */

#ifndef LOG_H_
#define LOG_H_

#define LOGFNMAXLEN 128

#define LOGMAXSIZE 2 * 1024 * 1024

#define LOG_CRIT	0
#define LOG_WARN	1
#define LOG_INFO	2

#define LOG_DEBUG	3
#define LOG_TRACE	4

int log_init();
void log_fini();
int logmsg(int severity, char* source, char* format, ...);

#endif /* LOG_H_ */
