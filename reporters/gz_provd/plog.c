#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include "plog.h"

int plog_process(FILE *log, plog_handler_t *fns[], int nfns, void *arg)
{
	struct provmsg header;
	struct provmsg *msg;
	int sz;
	int rv = 0;

	struct timeval start, end;
	float num_events=0;
	gettimeofday(&start,NULL);
	while (fread(&header, sizeof header, 1, log) == 1) {

		sz = msg_getlen(&header);
		if (sz < sizeof(header)){
			return EINVAL;
		}

		msg = malloc(sz);
		if (!msg){
			return ENOMEM;
		}

		sz -= sizeof(*msg);

		/* Fill in the entire message */
		*msg = header;
		if (sz > 0 && fread(msg + 1, sz, 1, log) < 1) {
			free(msg);
			return ferror(log) ? EIO : EINVAL;
		}

		/* Process message */
		if (header.type < nfns && fns[header.type]){
			rv = fns[header.type](msg, arg);
			num_events++;
		}
		free(msg);

		if (rv)
			return rv;
	}

	//out:	 
	gettimeofday(&end,NULL);
	/* Calculate elapsed time */
	unsigned long elapsed_time = (1000000 * end.tv_sec + end.tv_usec) - (1000000 * start.tv_sec + start.tv_usec);
	printf("Elapsed time is %lu microseconds\n",elapsed_time);
	printf("Number of events is %f\n",num_events);
	printf("Events per second is %f\n", num_events / (elapsed_time/1000000.0));

	/* Calculate elapsed time */


	return ferror(log) ? EIO : 0;
}
