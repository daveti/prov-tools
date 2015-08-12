#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>

#include "linux-mman.h"
#include "linux-fs.h"
#include "linux-shm.h"

#include "plog.h"


#define __STR(x) #x
#define _STR(x) __STR(x)

static inline void usage() {
	fprintf(stderr,
		"Usage: pg_cat [-c hexid]\n"
		"Reads a binary provenance log from standard input and translates\n"
		"it into the postgres provenance db"
		"   pg_cat < plog\n");
}

static int def_handle_credfork(struct provmsg *msg, void *arg) {

  struct provmsg_credfork *m = (struct provmsg_credfork *) msg;
  struct args *args = (struct args *) arg;

  if (!want_cred(args, msg->cred_id))
    return 0;

  if (args->creds && args->recurse >= 1)
    additem(&args->extracreds, m->forked_cred);
  
  return 0;
}

static int def_handle_credfree(struct provmsg *msg, void *arg) {

  struct args *args = (struct args *) arg;
  
  if (!want_cred(args, msg->cred_id))
    return 0;
  
  if (args->creds)
    removeitem(&args->extracreds, msg->cred_id);
  
  return 0;
}

static int def_handle_exec(struct provmsg *msg, void *arg) {

  struct args *args = (struct args *) arg;
  if (args->creds && !hasitem(args->creds, msg->cred_id)) {
    if (args->recurse < 2) {
      removeitem(&args->extracreds, msg->cred_id);
      return 0;
    } else if (!hasitem(args->extracreds, msg->cred_id)) {
      return 0;
    }
  }
  
  return 0;
}

int main(int argc, char *argv[])
{
	static plog_handler_t *handlers[NUM_PROVMSG_TYPES];

	int i, rv;
	struct args args = {
		.creds = NULL,
		.recurse = 0,
	};
	int32_t arg;
	struct int_list *types = NULL, *n;

	for (i = 1; i < argc; i++) {
		if (argv[i][0] != '-') {
			usage();
			return 1;
		}
		switch (argv[i][1]) {
			case 'h':
				usage();
				return 0;
			case 'c':
				if (++i == argc) {
					usage();
					rv = 1;
					goto out;
				}
				arg = strtoul(argv[i], NULL, 16);
				if (arg < 0) {
					usage();
					rv = 1;
					goto out;
				}
				if (additem(&args.creds, arg)) {
					rv = 1;
					goto out;
				}
				break;
			case 't':
				if (++i == argc) {
					usage();
					rv = 1;
					goto out;
				}
				arg = type_from_name(argv[i]);
				if (arg < 0) {
					usage();
					rv = 1;
					goto out;
				}
				if (additem(&types, arg)) {
					rv = 1;
					goto out;
				}
				break;
			case 'f':
				args.recurse++;
				break;
		}
	}

	if (!types)
		for (i = 0; i < NUM_PROVMSG_TYPES; i++)
			handlers[i] = defhandlers[i];
	else {
		handlers[PROVMSG_CREDFORK] = def_handle_credfork;
		handlers[PROVMSG_CREDFREE] = def_handle_credfree;
		handlers[PROVMSG_EXEC] = def_handle_exec;
		for (; types; types = n) {
			n = types->next;
			handlers[types->data] = defhandlers[types->data];
			free(types);
		}
	}

	/* Make a connection to the database */
	conn = PQconnectdb(CONNINFO);
	
	/* Check to see that the backend connection was successfully made */
	if (PQstatus(conn) != CONNECTION_OK){
	    fprintf(stderr, "Connection to database failed: %s",
		    PQerrorMessage(conn));
	  }

	/* Start a transaction block */
	res = PQexec(conn, "BEGIN");
	if (PQresultStatus(res) != PGRES_COMMAND_OK)
	  handle_err(res,conn,"BEGIN",PQerrorMessage(conn));
	PQclear(res);  

	/* Main process for logging provenance records */
	rv = plog_process(stdin, handlers, NUM_PROVMSG_TYPES, &args);
	if (rv)
		fprintf(stderr, "Plog: %s\n", strerror(rv));
out:
	freelist(args.creds);
	
	/* end the transaction */
	res = PQexec(conn, "END");
	PQclear(res);

	/* close the connection to the database and cleanup */
	PQfinish(conn);
	
	return rv;
}


