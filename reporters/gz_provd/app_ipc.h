/*
 * app_ipc.h
 * Header file for app_ipc.c
 * Different provd should only need to include this header file
 * to support different applications.
 * Feb 17, 2015
 * root@davejingtian.org
 * http://davejingtian.org
 */
#define _GNU_SOURCE             /* To get SCM_CREDENTIALS definition from
                                   <sys/sockets.h> */
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>  /* Type definitions used by many programs */
#include <stdio.h>      /* Standard I/O functions */
#include <stdlib.h>     /* Prototypes of commonly used library functions,
                           plus EXIT_SUCCESS and EXIT_FAILURE constants */
#include <unistd.h>     /* Prototypes for many system calls */
#include <errno.h>      /* Declares errno and defines error constants */
#include <string.h>     /* Commonly used string-handling functions */
#include <zlib.h>

#define APP_SOCK_PATH		"./app_ipc_sock"
#define APP_PID_PATH_PREFIX	"/proc/"
#define APP_PID_PATH_SUFFIX	"/exe"
#define APP_IMA_PATH		"/sys/kernel/security/ima/ascii_runtime_measurements"
#define APP_PATH_LEN_MAX	1024
#define APP_DATA_ACK_LEN	3
#define APP_DATA_ACK_NAME	"ACK"

/* APIs */
int app_ipc_init(void);
void app_ipc_exit(void);
int app_ipc_process(int stop, gzFile out);
