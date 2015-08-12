#ifndef PLOG_H
#define PLOG_H

#include <stdio.h>
#include <sys/types.h>

#include "provmon_proto.h"
#include "spade_handlers.h"

typedef int plog_handler_t(struct provmsg *);

static plog_handler_t *defhandlers[NUM_PROVMSG_TYPES] = {

  [PROVMSG_BOOT] = handle_boot,
  [PROVMSG_CREDFORK] = handle_credfork,
  /*
  [PROVMSG_CREDFREE] = handle_credfree,
  */
  [PROVMSG_SETID] = handle_setid,
  [PROVMSG_EXEC] = handle_exec,
  [PROVMSG_FILE_P] = handle_file_p,
  /*
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
  */
};

int plog_process(FILE *log, plog_handler_t *fns[], int nfns);
int plog_process_raw(int log, plog_handler_t *fns[], int nfns);

#endif
