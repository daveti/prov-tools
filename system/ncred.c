#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "provmon_proto.h"

#include "plog.h"

int creds, max_creds, setids;

void print_line(void)
{
	fprintf(stderr, "\r%5d %5d %5d\033[K", creds, max_creds, setids);
}

int handle_boot(struct provmsg *msg, void *arg)
{
	printf("boot\n");
	return 0;
}

int handle_setid(struct provmsg *msg, void *arg)
{
	setids++;
	print_line();
	return 0;
}

int handle_credfork(struct provmsg *msg, void *arg)
{
	if (++creds > max_creds)
		max_creds = creds;
	print_line();
	return 0;
}

int handle_credfree(struct provmsg *msg, void *arg)
{
	if (--creds < 0) {
		fprintf(stderr, "\nbelow zero!");
		return 1;
	}
	print_line();
	return 0;
}

int main(int argc, char *argv[])
{
	static plog_handler_t *handlers[NUM_PROVMSG_TYPES] = {
		[PROVMSG_BOOT] = handle_boot,
		[PROVMSG_CREDFORK] = handle_credfork,
		[PROVMSG_CREDFREE] = handle_credfree,
		[PROVMSG_SETID] = handle_setid,
	};
	int rv;

	rv = plog_process(stdin, handlers, NUM_PROVMSG_TYPES, NULL);
	printf("\n");
	if (rv)
		fprintf(stderr, "%s\n", strerror(rv));
	return 0;
}
