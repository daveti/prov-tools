#ifndef PLOG_H
#define PLOG_H

#include <stdio.h>
#include <sys/types.h>

#include "provmon_proto.h"

typedef int plog_handler_t(struct provmsg *, void *);

int plog_process(FILE *log, plog_handler_t *fns[], int nfns, void *arg);

#endif
