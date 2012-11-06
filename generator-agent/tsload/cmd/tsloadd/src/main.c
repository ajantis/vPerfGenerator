/*
 * main.c
 *
 * Main file for ts-loader daemon
 */


#define LOG_SOURCE "tsloadd"
#include <log.h>

#include <modules.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>

extern char log_filename[];
extern int log_debug;
extern int log_trace;

extern char mod_search_path[];

void usage() {
	fprintf(stderr, "command line: \n");
	fprintf(stderr, "\ttsloadd -m <mod-path> -l <log-file> [-d|-t]\n");

	exit(1);
}

void parse_options(int argc, char* argv[]) {
	int lflag = 0;
	int mflag = 0;
	int ok = 1;

	int c;

	while((c = getopt(argc, argv, "l:m:dt")) != -1) {
		switch(c) {
		case 'm':
			mflag = 1;
			strncpy(mod_search_path, optarg, MODPATHLEN);
			break;
		case 'l':
			lflag = 1;
			strncpy(log_filename, optarg, LOGFNMAXLEN);
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

	if(!ok || !lflag) {
		usage();
		exit(1);
	}
}

int main(int argc, char* argv[]) {
	int err = 0;

	parse_options(argc, argv);

	if((err = log_init()) != 0)
		return err;

	logmsg(LOG_INFO, "Started tsloadd");

	if((err = load_modules()) != 0)
		return err;

	log_fini();

	return 0;
}
