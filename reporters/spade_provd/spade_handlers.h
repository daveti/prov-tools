#ifndef SPADE_HANDLERS_H
#define SPADE_HANDLERS_H

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
#include "inttypes.h"
#include <stdbool.h>
 

#define CMDLEN 2048
#define MAXENVLEN (CMDLEN-256)
 
#define CHARSIZE 50
#define TEMPCHARSIZE 40
#define BUFFERSIZE 1400


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

/* Other stuff */
void exit_nicely();
char * encode_to_utf8(char * cstring, int len);
char * argv_to_label(char * cstring, int len, int argc);
char * print_uuid_be(uuid_be *uuid);
int32_t type_from_name(char *name);


/* Link list node for process . this list is to check if process is already created or not*/
struct Process
{
    unsigned long credId;  
    char* processId;   
    struct Process* next;
};
/* Link list node for Agent . this list is to check if Agent is already existing or not*/
 struct Agent
{
     uint32_t uid;
    char* agentId;   
    struct Agent* next;
};
/* Link list node for artifcat . this list is to check if artifact is already created or not*/
struct Artifact
{
    unsigned long inodeNo;
    unsigned long versionNo;  
    char* artifactId;   
    struct Artifact* next
};

/*this struct is to check if a artifact is already used by a process or not */
struct Used
{
    char* processId;
    char* artifactId;
    struct Used* next;
};

/*this struct is to check if a artifact is already generated by a process or not */
struct WasGeneratedBy
{
    char* processId;
    char* artifactId;
    struct WasGeneratedBy* next;
};


void pushProcess(struct Process** head_ref, unsigned long cr,char* prId);
int checkProcessId(struct Process* head,unsigned long cr);
char* getProcessId(struct Process* head,unsigned long cr);
void printVal(struct Process* head);

void pushAgent(struct Agent** head_ref, uint32_t u_id,char* agId);
int checkAgentId(struct Agent* head,uint32_t u_id);
char* getAgentId(struct Agent* head,uint32_t u_id);

void pushArtifact(struct Artifact** head_ref, unsigned long iNode,unsigned long version,char* artiId);
int checkArtifactId(struct Artifact* head,unsigned long iNode,unsigned long version);
char* getArtifactId(struct Artifact* head, unsigned long iNode,unsigned long version);


void pushUsedBy(struct Used** head_ref,char* pId,char* artiId);
int checkUsedBy(struct Used* head,char* pId,char* artiId);

void pushWasGeneratedBy(struct WasGeneratedBy** head_ref,char* pId,char* artiId);
int checkWasGeneratedBy(struct WasGeneratedBy* head,char* pId,char* artiId);

 

#endif
