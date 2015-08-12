/*
 * Provenance collector daemon.  Simply funnels the relay file to port 16152 on
 * the given host.
 *
 * Unlike uprovd.c, this function records uncompressed provenance to the log
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>

#include <sys/stat.h>
#include <fcntl.h>

#include "plog.h"

int stopping;

void handle(int signo)
{
	stopping = 1;
}

int main(int argc, char *argv[])
{
	int rv = 0;
	int delay = -1;
	const char *inpath;
	int infd;
	char *endp;

	/* Parse arguments - should be two or three */
	if (argc < 2 || argc > 3)
		goto err_usage;

	inpath = argv[1];

	if(argc == 2) {
	  errno = 0;
	  delay = strtod(argv[2], &endp);
	  if (errno || *endp || delay < 0) {
	    fprintf(stderr, "invalid delay: `%s'\n", argv[2]);
	    return 1;
	  }
	}

	if (inpath[0] != '/') {
		fprintf(stderr, "Input file must be given as absolute path\n");
		return 1;
	}

	/* Try to open provenance relay */
	infd = open(inpath, O_RDONLY);
	if (infd < 0) {
		perror("open read");
		return 1;
	}

	/* Daemonize if not a one-shot */
	if (delay >= 0 && daemon(0, 0)) {
		perror("daemon");
		rv = 1;
		goto out_close_relay;
	}

	/* Shut down gracefully when given the chance */
	signal(SIGHUP, handle);
	signal(SIGINT, handle);
	signal(SIGQUIT, handle);
	signal(SIGTERM, handle);

	/* Start doing real work */
	for (;;) {
		/* Write everything currently in relay to log */
	        rv = plog_process_raw(infd, defhandlers, NUM_PROVMSG_TYPES);

		if (rv)
			goto out_close_relay;

		/* Exit condition */
		if (stopping || delay < 0)
			break;
		if (TEMP_FAILURE_RETRY(close(infd))) {
			perror("close");
			rv = 1;
			goto out_close_log;
		}
		usleep(delay);
		infd = TEMP_FAILURE_RETRY(open(inpath, O_RDONLY));
		if (infd < 0) {
			perror("open temp failure retry");
			rv = 1;
			goto out_close_log;
		}
	}

out_close_relay:
	/* Close relay */
	if (TEMP_FAILURE_RETRY(close(infd))) {
		perror("close");
		rv = 1;
	}
out_close_log:

	/* Close log file */
	return rv;

err_usage:
	fprintf(stderr, "usage: spade_provd <infile> [<delay in us>]\n"
			"   If no delay is given, operates in \"one-shot\" mode\n");
	return 1;
}
