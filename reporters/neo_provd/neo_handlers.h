#ifndef NEO_HANDLERS_H
#define NEO_HANDLERS_H

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <curl/curl.h>
#include <string.h>
#include <jansson.h>

#include "linux-mman.h"
#include "linux-fs.h"
#include "linux-shm.h" 
#include "provmon_proto.h"
#include "curl_handlers.h"

/* Neo4j Stuff */
#define NEO_URL "http://localhost:7474/db/data/transaction/commit"
#define MAXREQLEN 2048
char       cypher_req[MAXREQLEN];
#define CMDLEN 2048
#define MAXENVLEN (CMDLEN-256)

/* Other stuff */
void exit_nicely();

char * encode_to_utf8(char * cstring, int len);
char * argv_to_postgres_arr(char * cstring, int len);
char * print_uuid_be(uuid_be *uuid);
int32_t type_from_name(char *name);

int handle_boot(struct provmsg *msg);
int handle_credfork(struct provmsg *msg);
int handle_credfree(struct provmsg *msg);
int handle_setid(struct provmsg *msg);
int handle_exec(struct provmsg *msg);
int handle_inode_p(struct provmsg *msg);
int handle_mmap(struct provmsg *msg);
int handle_file_p(struct provmsg *msg);
int handle_link(struct provmsg *msg);
int handle_unlink(struct provmsg *msg);
int handle_setattr(struct provmsg *msg);
int handle_inode_alloc(struct provmsg *msg);
int handle_inode_dealloc(struct provmsg *msg);
int handle_socksend(struct provmsg *msg);
int handle_sockrecv(struct provmsg *msg);
int handle_sockalias(struct provmsg *msg);
int handle_mqsend(struct provmsg *msg);
int handle_mqrecv(struct provmsg *msg);
int handle_shmat(struct provmsg *msg);
int handle_readlink(struct provmsg *msg);

#endif
