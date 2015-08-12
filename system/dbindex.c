#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <db.h>

#include "provmon_proto.h"

static void usage(void)
{
	printf("Usage: dbindex <dbfile>\n"
	       "Reindexes the given Berkeley DB file.\n");
}

static int get_cred_id(DB *dbi, const DBT *key, const DBT *data, DBT *result)
{
	struct provmsg *msg = data->data;
	result->data = &msg->cred_id;
	result->size = sizeof msg->cred_id;
	return 0;
}

int main(int argc, char *argv[])
{
	int rv;
	DB *db, *dbi;
	char *dbfile, *idxfile;
	int n;

	if (argc < 2) {
		printf("dbfile argument is required\n\n");
		usage();
		return 1;
	}

	n = strlen(argv[1]);
	dbfile = malloc(n + 3);
	if (!dbfile) {
		fprintf(stderr, "failed to allocate string\n");
		return 1;
	}
	idxfile = malloc(n + 8);
	if (!idxfile) {
		fprintf(stderr, "failed to allocate string\n");
		free(dbfile);
		return 1;
	}
	strcpy(dbfile, argv[1]);
	strcat(dbfile, ".db");
	strcpy(idxfile, argv[1]);
	strcat(idxfile, "_cred.db");

	rv = db_create(&db, NULL, 0);
	if (rv) {
		fprintf(stderr, "db_create: %s\n", strerror(rv));
		goto out_free;
	}
	rv = db->open(db, NULL, dbfile, NULL, DB_UNKNOWN, DB_RDONLY, 0644);
	if (rv) {
		db->err(db, rv, NULL);
		goto out_free;
	}

	rv = db_create(&dbi, NULL, 0);
	if (rv) {
		fprintf(stderr, "db_create: %s\n", strerror(rv));
		goto out_close_db;
	}
	rv = dbi->open(dbi, NULL, idxfile, NULL, DB_HASH,
			DB_CREATE | DB_EXCL, 0644);
	if (rv) {
		dbi->err(dbi, rv, NULL);
		goto out_close_db;
	}

	rv = db->associate(db, NULL, dbi, get_cred_id,
			DB_CREATE | DB_IMMUTABLE_KEY);
	if (rv)
		db->err(db, rv, NULL);

	dbi->close(dbi, 0);
out_close_db:
	db->close(db, 0);
out_free:
	free(idxfile);
	free(dbfile);
	return rv ? 1 : 0;
}
