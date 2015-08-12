/*
 * Provenance collector daemon.  Simply funnels the relay file to port 16152 on
 * the given host.
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

#include <zlib.h>

//#include "provmon_proto.h"

/* daveti: for app ipc */
#include "app_ipc.h"

int stopping;

void handle(int signo)
{
	stopping = 1;
}

static int funnel_data(int infd, gzFile outfile)
{
	char buf[BUFSIZ];
	size_t n;

	while (!stopping && (n = TEMP_FAILURE_RETRY(read(infd, buf, sizeof buf))) > 0) {
		if (gzwrite(outfile, buf, n) < n) {
			perror("gzwrite");
			return 1;
		}
	}
	if (n < 0) {
		perror("read");
		return 1;
	}
	return 0;
}

int main(int argc, char *argv[])
{
	int rv = 0;
	int delay = -1;
	const char *inpath, *outpath;
	const char *errstr;
	gzFile outfile;
	int infd;
	char *endp;

	/* Parse arguments - should be two or three */
	if (argc < 3 || argc > 4)
		goto err_usage;
	inpath = argv[1];
	outpath = argv[2];
	if (argc == 4) {
		errno = 0;
		delay = strtod(argv[3], &endp);
		if (errno || *endp || delay < 0) {
			fprintf(stderr, "invalid delay: `%s'\n", argv[3]);
			return 1;
		}
	}
	if (inpath[0] != '/' || outpath[0] != '/') {
		fprintf(stderr, "Input and output files must be given as absolute paths\n");
		return 1;
	}

	/* Try to open provenance relay */
	infd = open(inpath, O_RDONLY);
	if (infd < 0) {
		perror("open");
		return 1;
	}

	/* Try to open output file */
	umask(S_IWGRP | S_IWOTH);
	outfile = gzopen(outpath, "wb");
	if (!outfile) {
		perror("gzopen");
		if (TEMP_FAILURE_RETRY(close(infd)))
			perror("close");
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

	/* daveti: init the app ipc */
	rv = app_ipc_init();
	if (rv) {
		perror("app_ipc_init");
		/* We should NOT impact the kernel relay */
	}

	/* Start doing real work */
	for (;;) {
		/* Write everything currently in relay to log */
		rv = funnel_data(infd, outfile);
		if (rv)
			goto out_close_relay;

		/* DAVETI: Add something here to check for new app prov */
		// Check for new message
		// Recover PID of proc that sent the message
		// Use PID to get binary name
		// Check IMA to see if digest for that binary is valid
		// If so...
		//   Pipe message into zip file like funnel_data
		// Either way, send ACK
		rv = app_ipc_process(stopping, outfile);
		if (rv) {
			perror("app_ipc_process");
			/* We should NOT impact the kernel relay */
		}

		/* Exit condition */
		if (stopping || delay < 0)
			break;
		if (TEMP_FAILURE_RETRY(close(infd))) {
			perror("close");
			rv = 1;
			goto out_close_log;
		}
		if (gzflush(outfile, Z_SYNC_FLUSH) < 0) {
			errstr = gzerror(outfile, &rv);
			if (rv == Z_ERRNO)
				perror("gzflush");
			else
				fprintf(stderr, "gzflush: %s\n", errstr);
			rv = 1;
			goto out_close_log;
		}
		usleep(delay);
		infd = TEMP_FAILURE_RETRY(open(inpath, O_RDONLY));
		if (infd < 0) {
			perror("open");
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
#if ZLIB_VERNUM >= 0x1240
	if (gzclose_w(outfile) == Z_ERRNO) {
		perror("gzclose_w");
#else
	if (gzclose(outfile) == Z_ERRNO) {
		perror("gzclose");
#endif
		rv = 1;
	}

	/* daveti: app ipc close */
	app_ipc_exit();

	return rv;

err_usage:
	fprintf(stderr, "usage: uprovd <infile> <outfile>.gz [<delay in us>]\n"
			"   If no delay is given, operates in \"one-shot\" mode\n");
	return 1;
}
