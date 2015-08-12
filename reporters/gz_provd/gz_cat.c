#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>

#include "linux-mman.h"
#include "linux-fs.h"
#include "linux-shm.h"
#include "provmon_proto.h"
#include "plog.h"

#define __STR(x) #x
#define _STR(x) __STR(x)

struct int_list {
	struct int_list *next;
	uint32_t data;
};

struct args {
	struct int_list *creds;
	struct int_list *extracreds;
	int recurse;
};

static inline void usage()
{
	fprintf(stderr,
		"Usage: pcat [-c hexid]\n"
		"Reads a binary provenance log from standard input and prints\n"
		"a human-readable representation to standard output.\n\n"
		"Examples:\n"
		"   pcat < plog > plog.out\n"
		"   tail -fc+0 live_plog | pcat -c 4a0 -t socksend | less\n\n"
		"Options:\n"
		"   -c HEXID   select records with credential id 0xHEXID\n"
		"   -f         follow forks (twice for forks and execs)\n"
		"   -t type    select records of a certain type\n");
}

static int additem(struct int_list **list, uint32_t item)
{
	struct int_list *newitem = malloc(sizeof(*newitem));
	if (!newitem) {
		perror("malloc");
		return 1;
	}
	newitem->data = item;
	newitem->next = *list;
	*list = newitem;
	return 0;
}

static int removeitem(struct int_list **list, uint32_t item)
{
	struct int_list *cur = *list, *prev = NULL;
	for (; cur; cur = cur->next) {
		if (cur->data == item)
			break;
		prev = cur;
	}
	if (!cur)
		return 1;
	if (!prev)
		*list = cur->next;
	else
		prev->next = cur->next;
	return 0;
}

static int hasitem(struct int_list *list, uint32_t item)
{
	for (; list; list = list->next) {
		if (list->data == item)
			return 1;
	}
	return 0;
}

static void freelist(struct int_list *list)
{
	struct int_list *next;
	for (; list; list = next) {
		next = list->next;
		free(list);
	}
}

static void print_uuid_be(uuid_be *uuid)
{
	int i;
	for (i = 0; i < 16; i++) {
		if (i == 4 || i == 6 || i == 8)
			printf("-");
		printf("%02x", uuid->b[i]);
	}
}

static void print_sb_inode(struct sb_inode *sbi)
{
	print_uuid_be(&sbi->sb_uuid);
	printf(":%lu", (unsigned long)sbi->ino);
}

static void print_sockid(struct sockid *s)
{
	printf("%04hx-%08x", s->high, s->low);
}

static void print_host_sockid(struct host_sockid *hs)
{
	print_uuid_be(&hs->host);
	printf(":");
	print_sockid(&hs->sock);
}

static int want_cred(struct args *args, int cred)
{
	/* Returns true if we are interested in this cred */
	/* Cred	Extra
	 * 0	0	True
	 * 0	Y/N	(Assert)
	 * Y	0/N/Y	True
	 * N	0/N	False
	 * N	Y	True
	 */
	if (!args->creds)
		return 1;
	if (hasitem(args->creds, cred))
		return 1;
	return hasitem(args->extracreds, cred);	
}

static int handle_boot(struct provmsg *msg, void *arg)
{
	struct provmsg_boot *m = (struct provmsg_boot *) msg;
	struct args *args = (struct args *) arg;
	if (!want_cred(args, msg->cred_id))
		return 0;
	printf("[%x] boot ", m->msg.cred_id);
	print_uuid_be(&m->uuid);
	printf("\n");
	return 0;
}

static int def_handle_credfork(struct provmsg *msg, void *arg)
{
	struct provmsg_credfork *m = (struct provmsg_credfork *) msg;
	struct args *args = (struct args *) arg;
	if (!want_cred(args, msg->cred_id))
		return 0;
	if (args->creds && args->recurse >= 1)
		additem(&args->extracreds, m->forked_cred);
	return 0;
}

static int handle_credfork(struct provmsg *msg, void *arg)
{
	struct provmsg_credfork *m = (struct provmsg_credfork *) msg;
	struct args *args = (struct args *) arg;
	if (!want_cred(args, msg->cred_id))
		return 0;
	if (args->creds && args->recurse >= 1)
		additem(&args->extracreds, m->forked_cred);
	printf("[%x] credfork %x\n", m->msg.cred_id, m->forked_cred);
	return 0;
}

static int def_handle_credfree(struct provmsg *msg, void *arg)
{
	struct args *args = (struct args *) arg;
	if (!want_cred(args, msg->cred_id))
		return 0;
	if (args->creds)
		removeitem(&args->extracreds, msg->cred_id);
	return 0;
}

static int handle_credfree(struct provmsg *msg, void *arg)
{
	struct provmsg_credfree *m = (struct provmsg_credfree *) msg;
	struct args *args = (struct args *) arg;
	if (!want_cred(args, msg->cred_id))
		return 0;
	if (args->creds)
		removeitem(&args->extracreds, msg->cred_id);
	printf("[%x] credfree\n", m->msg.cred_id);
	return 0;
}

static int handle_setid(struct provmsg *msg, void *arg)
{
	struct provmsg_setid *m = (struct provmsg_setid *) msg;
	struct args *args = (struct args *) arg;
	if (!want_cred(args, msg->cred_id))
		return 0;
	printf("[%x] ", m->msg.cred_id);
	printf("setid r%u:%u, s%u:%u, e%u:%u, fs%u:%u\n",
			m->uid, m->gid, m->suid, m->sgid,
			m->euid, m->egid, m->fsuid, m->fsgid);
	return 0;
}

static int def_handle_exec(struct provmsg *msg, void *arg)
{
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

static int handle_exec(struct provmsg *msg, void *arg)
{
	struct provmsg_exec *m = (struct provmsg_exec *) msg;
	struct args *args = (struct args *) arg;
	if (args->creds && !hasitem(args->creds, msg->cred_id)) {
		if (args->recurse < 2) {
			removeitem(&args->extracreds, msg->cred_id);
			return 0;
		} else if (!hasitem(args->extracreds, msg->cred_id)) {
			return 0;
		}
	}
	printf("[%x] exec ", m->msg.cred_id);
	print_sb_inode(&m->inode);
	printf(" ");

	fwrite(m->argv_envp, msg_getlen(msg) - sizeof(*m), 1, stdout);
	printf("\n");
	return 0;
}
static int handle_inode_p(struct provmsg *msg, void *arg)
{
	struct provmsg_inode_p *m = (struct provmsg_inode_p *) msg;
	struct args *args = (struct args *) arg;
	if (!want_cred(args, msg->cred_id))
		return 0;
	printf("[%x] iperm ", m->msg.cred_id);
	if (m->mask == 0)
		printf("access ");
	else {
		if (m->mask & MAY_READ)
			printf("R");
		if (m->mask & MAY_WRITE)
			printf("W");
		if (m->mask & MAY_APPEND)
			printf("+");
		if (m->mask & MAY_EXEC)
			printf("X");
		if (m->mask & MAY_ACCESS)
			printf("a");
		if (m->mask & MAY_OPEN)
			printf("o");
		printf(" ");
	}
	print_sb_inode(&m->inode);
	printf(":%lu",m->inode_version);
	//printf(" %.*s",m->path_len,m->path);
	printf("\n");
	return 0;
}

static int handle_mmap(struct provmsg *msg, void *arg)
{
	struct provmsg_mmap *m = (struct provmsg_mmap *) msg;
	struct args *args = (struct args *) arg;
	if (!want_cred(args, msg->cred_id))
		return 0;
	printf("[%x] mmap ", m->msg.cred_id);
	if (m->prot == 0)
		printf("NONE ");
	else {
		if (m->prot & PROT_READ)
			printf("R");
		if (m->prot & PROT_WRITE)
			printf("W");
		if (m->prot & PROT_EXEC)
			printf("X");
		printf(" ");
	}
	printf("fl=%lu ", (unsigned long)m->flags);
	print_sb_inode(&m->inode);
	printf("\n");
	return 0;
}

static int handle_file_p(struct provmsg *msg, void *arg)
{
	struct provmsg_file_p *m = (struct provmsg_file_p *) msg;
	struct args *args = (struct args *) arg;
	if (!want_cred(args, msg->cred_id))
		return 0;
	printf("[%x] fperm ", m->msg.cred_id);
	if (m->mask == 0)
		printf("access ");
	else {
		if (m->mask & MAY_READ)
			printf("R");
		if (m->mask & MAY_WRITE)
			printf("W");
		if (m->mask & MAY_APPEND)
			printf("+");
		if (m->mask & MAY_EXEC)
			printf("X");
		if (m->mask & MAY_ACCESS)
			printf("a");
		if (m->mask & MAY_OPEN)
			printf("o");
		printf(" ");
	}
	print_sb_inode(&m->inode);
	printf(":%lu",m->inode_version);
	printf("\n");
	return 0;
}

static int handle_link(struct provmsg *msg, void *arg)
{
	struct provmsg_link *m = (struct provmsg_link *) msg;
	struct args *args = (struct args *) arg;
	int len;
	if (!want_cred(args, msg->cred_id))
		return 0;
	len = msg_getlen(msg) - sizeof(*m);
	printf("[%x] ", m->msg.cred_id);
	if (len == 0) {
		printf("root_inode ");
		print_sb_inode(&m->inode);
		printf("\n");
	} else {
		printf("link ");
		print_sb_inode(&m->inode);
		printf(" to %lu:%.*s\n", (unsigned long)m->dir, len, m->fname);
	}
	return 0;
}

static int handle_unlink(struct provmsg *msg, void *arg)
{
	struct provmsg_unlink *m = (struct provmsg_unlink *) msg;
	struct args *args = (struct args *) arg;
	int len;
	if (!want_cred(args, msg->cred_id))
		return 0;
	len = msg_getlen(msg) - sizeof(*m);
	printf("[%x] unlink ", m->msg.cred_id);
	print_sb_inode(&m->dir);
	printf(":%.*s\n", len, m->fname);
	return 0;
}

static int handle_setattr(struct provmsg *msg, void *arg)
{
	struct provmsg_setattr *m = (struct provmsg_setattr *) msg;
	struct args *args = (struct args *) arg;
	if (!want_cred(args, msg->cred_id))
		return 0;
	printf("[%x] setattr ", m->msg.cred_id);
	print_sb_inode(&m->inode);
	printf(" %u:%u m=%o\n", m->uid, m->gid, m->mode);
	return 0;
}

static int handle_inode_alloc(struct provmsg *msg, void *arg)
{
	struct provmsg_inode_alloc *m = (struct provmsg_inode_alloc *) msg;
	struct args *args = (struct args *) arg;
	if (!want_cred(args, msg->cred_id))
		return 0;
	printf("[%x] ialloc ", m->msg.cred_id);
	print_sb_inode(&m->inode);
	printf("\n");
	return 0;
}

static int handle_inode_dealloc(struct provmsg *msg, void *arg)
{
	struct provmsg_inode_dealloc *m = (struct provmsg_inode_dealloc *) msg;
	struct args *args = (struct args *) arg;
	if (!want_cred(args, msg->cred_id))
		return 0;
	printf("[%x] idealloc ", m->msg.cred_id);
	print_sb_inode(&m->inode);
	printf("\n");
	return 0;
}

static int handle_socksend(struct provmsg *msg, void *arg)
{
	struct provmsg_socksend *m = (struct provmsg_socksend *) msg;
	struct args *args = (struct args *) arg;
	if (!want_cred(args, msg->cred_id))
		return 0;
	printf("[%x] socksend ", m->msg.cred_id);
	print_sockid(&m->peer);
	printf("\n");
	return 0;
}

static int handle_sockrecv(struct provmsg *msg, void *arg)
{
	struct provmsg_sockrecv *m = (struct provmsg_sockrecv *) msg;
	struct args *args = (struct args *) arg;
	if (!want_cred(args, msg->cred_id))
		return 0;
	printf("[%x] sockrecv ", m->msg.cred_id);
	print_host_sockid(&m->sock);
	printf("\n");
	return 0;
}

static int handle_sockalias(struct provmsg *msg, void *arg)
{
	struct provmsg_sockalias *m = (struct provmsg_sockalias *) msg;
	struct args *args = (struct args *) arg;
	if (!want_cred(args, msg->cred_id))
		return 0;
	printf("[%x] sockalias ", m->msg.cred_id);
	print_host_sockid(&m->sock);
	printf(" <= ");
	print_host_sockid(&m->alias);
	printf("\n");
	return 0;
}

static int handle_mqsend(struct provmsg *msg, void *arg)
{
	struct provmsg_mqsend *m = (struct provmsg_mqsend *) msg;
	struct args *args = (struct args *) arg;
	if (!want_cred(args, msg->cred_id))
		return 0;
	printf("[%x] mqsend %u\n", m->msg.cred_id, m->msgid);
	return 0;
}

static int handle_mqrecv(struct provmsg *msg, void *arg)
{
	struct provmsg_mqrecv *m = (struct provmsg_mqrecv *) msg;
	struct args *args = (struct args *) arg;
	if (!want_cred(args, msg->cred_id))
		return 0;
	printf("[%x] mqrecv %u\n", m->msg.cred_id, m->msgid);
	return 0;
}

static int handle_shmat(struct provmsg *msg, void *arg)
{
	struct provmsg_shmat *m = (struct provmsg_shmat *) msg;
	struct args *args = (struct args *) arg;
	if (!want_cred(args, msg->cred_id))
		return 0;
	printf("[%x] shmat %u%s\n", m->msg.cred_id, m->shmid,
			(m->flags & SHM_RDONLY) ? " (RO)" : "");
	return 0;
}

static int handle_readlink(struct provmsg *msg, void *arg)
{
	struct provmsg_readlink *m = (struct provmsg_readlink *) msg;
	struct args *args = (struct args *) arg;
	if (!want_cred(args, msg->cred_id))
		return 0;
	printf("[%x] readlink ", m->msg.cred_id);
	print_sb_inode(&m->inode);
	printf("\n");
	return 0;
}

static int32_t type_from_name(char *name)
{
	if (!strcmp("boot", name))
		return PROVMSG_BOOT;
	else if (!strcmp("credfork", name))
		return PROVMSG_CREDFORK;
	else if (!strcmp("credfree", name))
		return PROVMSG_CREDFREE;
	else if (!strcmp("setid", name))
		return PROVMSG_SETID;
	else if (!strcmp("exec", name))
		return PROVMSG_EXEC;
	else if (!strcmp("fperm", name))
		return PROVMSG_FILE_P;
	else if (!strcmp("mmap", name))
		return PROVMSG_MMAP;
	else if (!strcmp("iperm", name))
		return PROVMSG_INODE_P;
	else if (!strcmp("ialloc", name))
		return PROVMSG_INODE_ALLOC;
	else if (!strcmp("idealloc", name))
		return PROVMSG_INODE_DEALLOC;
	else if (!strcmp("setattr", name))
		return PROVMSG_SETATTR;
	else if (!strcmp("link", name))
		return PROVMSG_LINK;
	else if (!strcmp("unlink", name))
		return PROVMSG_UNLINK;
	else if (!strcmp("mqsend", name))
		return PROVMSG_MQSEND;
	else if (!strcmp("mqrecv", name))
		return PROVMSG_MQRECV;
	else if (!strcmp("shmat", name))
		return PROVMSG_SHMAT;
	else if (!strcmp("readlink", name))
		return PROVMSG_READLINK;
	else if (!strcmp("socksend", name))
		return PROVMSG_SOCKSEND;
	else if (!strcmp("sockrecv", name))
		return PROVMSG_SOCKRECV;
	else if (!strcmp("sockalias", name))
		return PROVMSG_SOCKALIAS;
	else
		return -1;
}

int main(int argc, char *argv[])
{
	static plog_handler_t *defhandlers[NUM_PROVMSG_TYPES] = {
		[PROVMSG_BOOT] = handle_boot,
		[PROVMSG_CREDFORK] = handle_credfork,
		[PROVMSG_CREDFREE] = handle_credfree,
		[PROVMSG_SETID] = handle_setid,
		[PROVMSG_EXEC] = handle_exec,
		[PROVMSG_FILE_P] = handle_file_p,
		[PROVMSG_MMAP] = handle_mmap,
		[PROVMSG_INODE_P] = handle_inode_p,
		[PROVMSG_INODE_ALLOC] = handle_inode_alloc,
		[PROVMSG_INODE_DEALLOC] = handle_inode_dealloc,
		[PROVMSG_SETATTR] = handle_setattr,
		[PROVMSG_LINK] = handle_link,
		[PROVMSG_UNLINK] = handle_unlink,
		[PROVMSG_MQSEND] = handle_mqsend,
		[PROVMSG_MQRECV] = handle_mqrecv,
		[PROVMSG_SHMAT] = handle_shmat,
		[PROVMSG_READLINK] = handle_readlink,
		[PROVMSG_SOCKSEND] = handle_socksend,
		[PROVMSG_SOCKRECV] = handle_sockrecv,
		[PROVMSG_SOCKALIAS] = handle_sockalias,
	};
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

	rv = plog_process(stdin, handlers, NUM_PROVMSG_TYPES, &args);
	if (rv)
		fprintf(stderr, "%s\n", strerror(rv));
out:
	freelist(args.creds);
	return rv;
}
