#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "plog.h"

int plog_process(FILE *log, plog_handler_t *fns[], int nfns, void *arg)
{
	struct provmsg header;
	struct provmsg *msg;
	int sz;
	int rv = 0;

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
		if (header.type < nfns && fns[header.type])
			rv = fns[header.type](msg, arg);
		free(msg);

		if (rv)
			return rv;
	}
	return ferror(log) ? EIO : 0;
}
