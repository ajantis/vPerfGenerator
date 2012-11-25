/*
 * main.c
 *
 *  Created on: Nov 19, 2012
 *      Author: myaut
 */

#define LOG_SOURCE "run-tsload"
#include <log.h>

#include <modules.h>

#include <commands.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>

extern char wl_filename[];
extern char step_filename[];

extern char log_filename[];
extern int log_debug;
extern int log_trace;

extern char mod_search_path[];

enum {
	CMD_NONE,
	CMD_INFO,
	CMD_LOAD
} command;

void env_usage() {
	fprintf(stderr, "environment options: \n");
	fprintf(stderr, "TS_MODPATH - path to loadable modules\n");
	fprintf(stderr, "TS_LOGFILE - log destination ('-' for stdout)\n");

	exit(1);
}

void usage() {
	fprintf(stderr, "command line: \n");
	fprintf(stderr, "\trun-tsload [-d|-t] -w <workload-file> -s <steps-file> \n\t\truns loader\n");
	fprintf(stderr, "\trun-tsload [-d|-t] -i \n\t\tget information on modules (in json)\n");
	fprintf(stderr, "\trun-tsload [-d|-t] -h \n\t\tyou are here\n");

	exit(1);
}

void read_environ() {
	char* env_mod_path = getenv("TS_MODPATH");
	char* env_log_filename = getenv("TS_LOGFILE");

	if(!env_mod_path || !env_log_filename) {
		env_usage();
	}

	strncpy(mod_search_path, env_mod_path, MODPATHLEN);
	strncpy(log_filename, env_log_filename, LOGFNMAXLEN);
}

void parse_options(int argc, char* argv[]) {
	int wflag = 0;
	int sflag = 0;
	int iflag = 0;
	int ok = 1;

	int c;

	while((c = getopt(argc, argv, "w:s:dtih")) != -1) {
		switch(c) {
		case 'h':
			usage();
			break;
		case 'w':
			wflag = 1;
			strncpy(wl_filename, optarg, MODPATHLEN);
			break;
		case 's':
			sflag = 1;
			strncpy(step_filename, optarg, MODPATHLEN);
			break;
		case 'i':
			iflag = 1;
			break;
		case 't':
			log_trace = 1;
			/*FALLTHROUGH*/
		case 'd':
			log_debug = 1;
			break;
		case '?':
			if(optopt == 'l' || optopt == 'm')
				fprintf(stderr, "-%c option requires an argument\n", optopt);
			else
				fprintf(stderr, "Unknown option `-%c'.\n", optopt);
			ok = 0;
			break;
		}

		if(!ok)
			break;
	}

	if(!ok) {
		usage();
	}

	if(wflag && sflag)
		command = CMD_LOAD;
	else if(iflag)
		command = CMD_INFO;
	else
		usage();
}

int main(int argc, char* argv[]) {
	int err = 0;

	command = CMD_NONE;

	read_environ();
	parse_options(argc, argv);

	if((err = threads_init()) != 0)
		return err;

	if((err = log_init()) != 0) {
		return err;
	}

	logmsg(LOG_INFO, "Started run-tsload");

	if((err = load_modules()) != 0) {
		fprintf(stderr, "Load modules error %d\n", err);
		return err;
	}

	logmsg(LOG_DEBUG, "run-tsload command: %d", command);

	if(command == CMD_INFO) {
		err = do_info();
	}
	else if(command == CMD_LOAD) {
		err = do_load();
	}

	if(err != 0) {
		fprintf(stderr, "Command execution was not OK, see log for details\n");
	}

	log_fini();

	return 0;
}


