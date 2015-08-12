#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "bitvec.h"
#include "provmon_proto.h"
#include "plog.h"

int handle_credfork(struct provmsg *msg, void *arg)
{
	struct provmsg_credfork *m = (struct provmsg_credfork *) msg;
	struct bitvec *alive = (struct bitvec *) arg;
	bitvec_set(alive, m->forked_cred);
	return 0;
}

int handle_credfree(struct provmsg *msg, void *arg)
{
	struct bitvec *alive = (struct bitvec *) arg;
	if (!bitvec_test(alive, msg->cred_id))
		printf("%x freed before fork!\n", msg->cred_id);
	else
		bitvec_clear(alive, msg->cred_id);
	return 0;
}

int main(int argc, char *argv[])
{
	static plog_handler_t *handlers[NUM_PROVMSG_TYPES] = {
		[PROVMSG_CREDFORK] = handle_credfork,
		[PROVMSG_CREDFREE] = handle_credfree,
	};
	struct bitvec alive;
	int rv;
	int i, j, ofs;
	unsigned char c;

	if (bitvec_init(&alive)) {
		fprintf(stderr, "Failed to initialize bit vector\n");
		return 1;
	}
	rv = plog_process(stdin, handlers, NUM_PROVMSG_TYPES, &alive);
	ofs = 0;
	for (i = 0; i < alive.size; i++) {
		c = alive.array[i];
		for (j = 0; j < 8; j++) {
			if (c & 1)
				printf("%x, ", ofs);
			c >>= 1;
			ofs++;
		}
	}
	printf("\n");
	bitvec_destroy(&alive);

	if (rv)
		fprintf(stderr, strerror(rv));
	return 0;
}
