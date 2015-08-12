#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <jansson.h>
#include "linux-mman.h"
#include "linux-fs.h"
#include "linux-shm.h"

#include "plog.h"

#define __STR(x) #x
#define _STR(x) __STR(x)

static inline void usage() {
	fprintf(stderr,
		"Usage: spade_cat > <binary provenance log>\n"
		"Reads a binary provenance log from standard input and translates\n"
		"it into the Spade system\n");

}


int main(int argc, char *argv[])
{
	int i, rv;

	/* Main process for logging provenance records */
	rv = plog_process(stdin, defhandlers, NUM_PROVMSG_TYPES);
	if (rv)
		fprintf(stderr, "Plog: %s\n", strerror(rv));

out:
	return rv;
}


