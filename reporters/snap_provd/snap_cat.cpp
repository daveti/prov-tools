#include "plog.hpp"

static inline void usage() {
	fprintf(stderr,
		"Usage: snap_cat\n"
		"Reads a binary provenance log from standard input and translates\n");
}

int main(int argc, char *argv[]){

	int i, rv;

	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-') {
			usage();
			return 1;
		}
		switch (argv[i][1]) {
			case 'h':
				usage();
				return 0;
		}
	}

	/* Main process for logging provenance records */
	rv = plog_process(stdin);
	if (rv)
		fprintf(stderr, "Plog: %s\n", strerror(rv));

	return rv;
}
