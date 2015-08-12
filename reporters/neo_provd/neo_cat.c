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
		"Usage: neo_cat\n"
		"Reads a binary provenance log from standard input and translates\n"
		"it into the neo4j db at localhost:7474"
		"   neo_cat < plog\n");
}


int main(int argc, char *argv[])
{
	int i, rv;

	curl_global_init(CURL_GLOBAL_ALL);
	/* Initialize cUrl */
	curl = curl_easy_init();
	if(!curl){
	  fprintf(stderr, "neo_cat: %s\n", "Failed to initialized curl object");
	  goto out;
	}

	/* Main process for logging provenance records */
	rv = plog_process(stdin, defhandlers, NUM_PROVMSG_TYPES);
	if (rv)
		fprintf(stderr, "Plog: %s\n", strerror(rv));

	/* always cleanup */ 
	curl_easy_cleanup(curl);
out:
	return rv;
}


