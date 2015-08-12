#ifndef PG_HANDLERS_H
#define PG_HANDLERS_H

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include "libpq-fe.h"
#include "provmon_proto.h"

struct int_list {
	struct int_list *next;
	uint32_t data;
};

struct args {
	struct int_list *creds;
	struct int_list *extracreds;
	int recurse;
};

#define CONNINFO "dbname = provenance"
#define CMDLEN 2048
#define MAXENVLEN (CMDLEN-256)

PGconn     *conn;
PGresult   *res;
char       cmd[CMDLEN];	


void exit_nicely(PGconn *conn);
void handle_err(PGresult * res, PGconn *conn,
		char * cmd, char * err);

char * encode_to_utf8(char * cstring, int len);
char * argv_to_postgres_arr(char * cstring, int len);
int additem(struct int_list **list, uint32_t item);
int removeitem(struct int_list **list, uint32_t item);
int hasitem(struct int_list *list, uint32_t item);
void freelist(struct int_list *list);
char * print_uuid_be(uuid_be *uuid);
int want_cred(struct args *args, int cred);
int32_t type_from_name(char *name);

unsigned long insert_into_main(uint32_t provid, int event_type);

int handle_boot(struct provmsg *msg, void *arg);
int handle_credfork(struct provmsg *msg, void *arg);
int handle_credfree(struct provmsg *msg, void *arg);
int handle_setid(struct provmsg *msg, void *arg);
int handle_exec(struct provmsg *msg, void *arg);
int handle_inode_p(struct provmsg *msg, void *arg);
int handle_mmap(struct provmsg *msg, void *arg);
int handle_file_p(struct provmsg *msg, void *arg);
int handle_link(struct provmsg *msg, void *arg);
int handle_unlink(struct provmsg *msg, void *arg);
int handle_setattr(struct provmsg *msg, void *arg);
int handle_inode_alloc(struct provmsg *msg, void *arg);
int handle_inode_dealloc(struct provmsg *msg, void *arg);
int handle_socksend(struct provmsg *msg, void *arg);
int handle_sockrecv(struct provmsg *msg, void *arg);
int handle_sockalias(struct provmsg *msg, void *arg);
int handle_mqsend(struct provmsg *msg, void *arg);
int handle_mqrecv(struct provmsg *msg, void *arg);
int handle_shmat(struct provmsg *msg, void *arg);
int handle_readlink(struct provmsg *msg, void * arg);

#endif
