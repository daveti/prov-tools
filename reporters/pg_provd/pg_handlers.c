#include "pg_handlers.h"

void exit_nicely(PGconn *conn) {
  PQfinish(conn);
  exit(1);
}

void handle_err(PGresult * res, PGconn *conn,
		       char * cmd, char * err){
  fprintf(stderr, "%s command failed: %s\n", cmd,err);
  PQclear(res);
  exit_nicely(conn);
}

char * encode_to_utf8(char * cstring, int len){

  char * newstring = malloc(len+1);
  //memcpy(newstring,cstring,len);
  //newstring[len]='\0';

  if(!newstring){
    printf("Out of memory\n");
    exit_nicely(conn);
  }

  // I really just don't care if you are a non-utf8 character
  // I am going to blast you away
  int newcursor=0,oldcursor=0;
  while(oldcursor < len){
    if (cstring[oldcursor] < 128 
	&& cstring[oldcursor] > 0){
      if(cstring[oldcursor] == '\\' 
	 || cstring[oldcursor] == '\''){
	newstring[newcursor] = ' '; newcursor++;
      }
      else{
	newstring[newcursor] = cstring[oldcursor]; newcursor++;
      }
    }
    oldcursor++;
  }

  newstring[newcursor] = '\0';

  return newstring;
}

char * argv_to_postgres_arr(char * cstring, int len){

  int item_count = 0;
  int quote_count = 0;
  for (int n = 0; n < len; n++){
    if(cstring[n] == 0)
      item_count++;
    else if(cstring[n] == '\"' || cstring[n] == '\'')
      quote_count++;
  }
  
  // For argv item1\0item2\0item3="escaped quotes"\0
  // Format is '{"item1","item2","item3=\"escaped quotes\"}'
  int newlength = 2 + len + item_count * 3 + quote_count * 2;
  char * newstring = malloc(newlength);
  if(!newstring){
    printf("Out of memory\n");
    exit_nicely(conn);
  }

  newstring[0] = '{';
  newstring[1] = '\"';

  int newcursor = 2;
  int oldcursor = 0;
  while(oldcursor < len && newcursor < MAXENVLEN){
    if(cstring[oldcursor] != 0){
      // Escape quotation marks
      if(cstring[oldcursor] == '\"' || cstring[oldcursor] == '\''){
	newstring[newcursor] = '\\'; newcursor++;
	newstring[newcursor] = '\\'; newcursor++;
	cstring[oldcursor] = '\"';
      }
      // Replace non-UTF8 encoded bytes with a space
      else if (cstring[oldcursor] >= 128 || cstring[oldcursor] < 0)
	cstring[oldcursor] = ' ';

      newstring[newcursor] = cstring[oldcursor];

      //printf("(%c,%d)\n",newstring[newcursor],newstring[newcursor]);
      newcursor++;
    }
    // If we encounter a null terminator it marks a new item or EOL
    // If new item
    else if(cstring[oldcursor] == 0
	&& oldcursor < len-1) {
	newstring[newcursor] = '\"'; newcursor ++;
	newstring[newcursor] = ','; newcursor ++;
	newstring[newcursor] = '\"'; newcursor ++;
    }
    oldcursor++;
  }

  // If EOL 
  newstring[newcursor] = '\"'; newcursor ++;
  newstring[newcursor] = '}'; newcursor ++;
  newstring[newcursor] = '\0';
  
  return newstring;  

}

int additem(struct int_list **list, uint32_t item)
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

int removeitem(struct int_list **list, uint32_t item)
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

int hasitem(struct int_list *list, uint32_t item)
{
	for (; list; list = list->next) {
		if (list->data == item)
			return 1;
	}
	return 0;
}

void freelist(struct int_list *list)
{
	struct int_list *next;
	for (; list; list = next) {
		next = list->next;
		free(list);
	}
}

char * print_uuid_be(uuid_be *uuid)
{
  int i,c=0;
  char * rv = malloc(37);
  for (i = 0; i < 16; i++) {
    if (i == 4 || i == 6 || i == 8 || i == 10){
      rv[c] = '-';
	    c++;
    }
    sprintf(rv+c,"%02x", uuid->b[i]);	  
    c+=2;
    
    rv[c] = (char)uuid->b[i];
    snprintf(&rv[c],1,"%02x", uuid->b[i]);
  }
  
  return rv;
}

int want_cred(struct args *args, int cred)
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

int32_t type_from_name(char *name)
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


unsigned long insert_into_main(uint32_t provid, int event_type) {

  /* Create an entry for this record in main table */
  snprintf(cmd,CMDLEN,"insert into main(provid,event_type) values(%d,%d) returning eventid;",
	   provid, event_type);
  
  /* Execute command */
  res = PQexec(conn, cmd);
  
  /* Check for error */
  if (PQresultStatus(res) != PGRES_TUPLES_OK)
    handle_err(res,conn,"insert main",PQerrorMessage(conn)); 

  /* Get the event id for the newly created record */
  unsigned long rv = atol(PQgetvalue(res,0,0));
  
  /* Clear the result of the first command */
  PQclear(res);
  
  /* Return the event id */
  return rv;
}

int handle_boot(struct provmsg *msg, void *arg) {

  struct provmsg_boot *m = (struct provmsg_boot *) msg;
  struct args *args = (struct args *) arg;

  if (!want_cred(args, msg->cred_id))
    return 0;

  /* Extract the event id for the insertion into the main table */
  unsigned long eventid = insert_into_main(m->msg.cred_id,PROVMSG_BOOT);
  
  /* Extract the uuid into a string */
  char * uuid = print_uuid_be(&m->uuid);
  
  /* Create an entry in the event-specific table */
  
  snprintf(cmd,CMDLEN,"insert into boot(eventid,uuid_be) values(%lu,'{%s}') returning eventid;",
	   eventid,uuid);

  /* Free the string created by print_uuid_be */
  free(uuid);

  res = PQexec(conn, cmd);
  if (PQresultStatus(res) != PGRES_TUPLES_OK)
    handle_err(res,conn,"insert boot",PQerrorMessage(conn)); 
  
  /* Clear the result of the second command */
  PQclear(res);
    
  return 0;
}

int handle_credfork(struct provmsg *msg, void *arg) {

  struct provmsg_credfork *m = (struct provmsg_credfork *) msg;
  struct args *args = (struct args *) arg;

  if (!want_cred(args, msg->cred_id))
    return 0;

  if (args->creds && args->recurse >= 1)
    additem(&args->extracreds, m->forked_cred);

  /* Extract the event id for the insertion into the main table */
  unsigned long eventid = insert_into_main(m->msg.cred_id,PROVMSG_CREDFORK);
  
  /* Create an entry in the event-specific table */  
  snprintf(cmd,CMDLEN,"insert into credfork(eventid,provid,newid) values(%lu,%lu,%lu) returning eventid;",
	   eventid,(unsigned long)m->msg.cred_id,(unsigned long)m->forked_cred);

  res = PQexec(conn, cmd);
  if (PQresultStatus(res) != PGRES_TUPLES_OK)
    handle_err(res,conn,"insert credfork",PQerrorMessage(conn)); 
  
  /* Clear the result of the second command */
  PQclear(res);

  return 0;
}

int handle_credfree(struct provmsg *msg, void *arg) {
  struct provmsg_credfree *m = (struct provmsg_credfree *) msg;
  struct args *args = (struct args *) arg;

  if (!want_cred(args, msg->cred_id))
    return 0;

  if (args->creds)
    removeitem(&args->extracreds, msg->cred_id);

  /* Extract the event id for the insertion into the main table */
  unsigned long eventid = insert_into_main(m->msg.cred_id,PROVMSG_CREDFREE);
  
  /* Create an entry in the event-specific table */  
  snprintf(cmd,CMDLEN,"insert into credfree(eventid,provid) values(%lu,%lu) returning eventid;",
	   eventid,(unsigned long)m->msg.cred_id);

  res = PQexec(conn, cmd);
  if (PQresultStatus(res) != PGRES_TUPLES_OK)
    handle_err(res,conn,"insert credfree",PQerrorMessage(conn)); 
  
  /* Clear the result of the second command */
  PQclear(res);
  
  return 0;
}

int handle_setid(struct provmsg *msg, void *arg) {

  struct provmsg_setid *m = (struct provmsg_setid *) msg;
  struct args *args = (struct args *) arg;

  if (!want_cred(args, msg->cred_id))
    return 0;

  /* Extract the event id for the insertion into the main table */
  unsigned long eventid = insert_into_main(m->msg.cred_id,PROVMSG_SETID);
  
  /* Create an entry in the event-specific table */  
  snprintf(cmd,CMDLEN,"insert into setid(eventid,uid,euid,gid,egid,suid,fsuid,sgid,fsgid) values(%lu,%d,%d,%d,%d,%d,%d,%d,%d) returning eventid;",
	   eventid,m->uid, m->gid, m->suid, m->sgid,
	   m->euid, m->egid, m->fsuid, m->fsgid);

  res = PQexec(conn, cmd);
  if (PQresultStatus(res) != PGRES_TUPLES_OK)
    handle_err(res,conn,"insert setid",PQerrorMessage(conn)); 
  
  /* Clear the result of the second command */
  PQclear(res);
  
  return 0;
}


int handle_exec(struct provmsg *msg, void *arg)
{
  struct provmsg_exec *m = (struct provmsg_exec *) msg;
  struct args *args = (struct args *) arg;

  if (args->creds && !hasitem(args->creds, msg->cred_id)) {
    if (args->recurse < 2) {
      removeitem(&args->extracreds, msg->cred_id);
      return 0;
    } 
    else if (!hasitem(args->extracreds, msg->cred_id)) {
      return 0;
    }
  }

  /* Extract the event id for the insertion into the main table */
  unsigned long eventid = insert_into_main(m->msg.cred_id,PROVMSG_EXEC);

  /* Extract the uuid into a string */
  char * uuid = print_uuid_be(&m->inode.sb_uuid);
  
  /* Need to make sure arguments are UTF8 friendly */
  char * argv = argv_to_postgres_arr(m->argv_envp, msg_getlen(msg) - sizeof(*m));

  unsigned long version = m->inode_version;

  /* Create an entry in the event-specific table */  
  snprintf(cmd,CMDLEN,"insert into exec(eventid,uuid_be,inode,version,argc,argv) values(%lu,'{%s}',%lu,%lu,%d,E'%s') returning eventid;",
	   eventid,uuid,(unsigned long)m->inode.ino,version,m->argc,argv);
  
  /* Free the string things */
  free(uuid);
  free(argv);

  res = PQexec(conn, cmd);
  if (PQresultStatus(res) != PGRES_TUPLES_OK){
    printf("ERROR %s\n",cmd);
    handle_err(res,conn,"insert exec",PQerrorMessage(conn)); 
  }

  /* Clear the result of the second command */
  PQclear(res);
  
  return 0;
}

int handle_inode_p(struct provmsg *msg, void *arg) {

  struct provmsg_inode_p *m = (struct provmsg_inode_p *) msg;
  struct args *args = (struct args *) arg;
  if (!want_cred(args, msg->cred_id))
    return 0;
  
  /* Extract the event id for the insertion into the main table */
  unsigned long eventid = insert_into_main(m->msg.cred_id,PROVMSG_INODE_P);
  
  /* Extract the uuid into a string */
  char * uuid = print_uuid_be(&m->inode.sb_uuid);
  
  unsigned long version = m->inode_version;

  /* Create an entry in the event-specific table */
  snprintf(cmd,CMDLEN,"insert into inode_p(eventid,uuid_be,inode,version,mask) values(%lu,'{%s}',%lu,%lu,%d) returning eventid;",
	   eventid,uuid,(unsigned long)m->inode.ino,version,m->mask);

  /* Free the string created by print_uuid_be */
  free(uuid);
  
  res = PQexec(conn, cmd);
  if (PQresultStatus(res) != PGRES_TUPLES_OK)
    handle_err(res,conn,"insert inode_p",PQerrorMessage(conn)); 
  
  /* Clear the result of the second command */
  PQclear(res);
  
  return 0;
}

int handle_mmap(struct provmsg *msg, void *arg) {

  struct provmsg_mmap *m = (struct provmsg_mmap *) msg;
  struct args *args = (struct args *) arg;
  if (!want_cred(args, msg->cred_id))
    return 0;
  
  /* Extract the event id for the insertion into the main table */
  unsigned long eventid = insert_into_main(m->msg.cred_id,PROVMSG_MMAP);
  
  /* Extract the uuid into a string */
  char * uuid = print_uuid_be(&m->inode.sb_uuid);
  
  unsigned long version = m->inode_version;

  /* Create an entry in the event-specific table */
  snprintf(cmd,CMDLEN,"insert into mmap(eventid,uuid_be,inode,version,prot,flags) values(%lu,'{%s}',%lu,%lu,%lu,%lu) returning eventid;",
	   eventid,
	   uuid,
	   (unsigned long)m->inode.ino,
	   version,
	   (unsigned long)m->prot,
	   (unsigned long)m->flags);
  
  /* Free the string created by print_uuid_be */
  free(uuid);
  
  res = PQexec(conn, cmd);
  if (PQresultStatus(res) != PGRES_TUPLES_OK)
    handle_err(res,conn,"insert mmap",PQerrorMessage(conn)); 
  
  /* Clear the result of the second command */
  PQclear(res);
  
  return 0;
}

int handle_file_p(struct provmsg *msg, void *arg)
{
  struct provmsg_file_p *m = (struct provmsg_file_p *) msg;
  struct args *args = (struct args *) arg;
  if (!want_cred(args, msg->cred_id))
    return 0;
  
  /* Extract the event id for the insertion into the main table */
  unsigned long eventid = insert_into_main(m->msg.cred_id,PROVMSG_FILE_P);
  
  /* Extract the uuid into a string */
  char * uuid = print_uuid_be(&m->inode.sb_uuid);
  
  unsigned long version = m->inode_version;

  if(version > 9223372036854775807)
    version = 0;
  
  /* Create an entry in the event-specific table */
  snprintf(cmd,CMDLEN,"insert into file_p(eventid,uuid_be,inode,version,mask) values(%lu,'{%s}',%lu,%lu,%d) returning eventid;",
	   eventid,uuid,(unsigned long)m->inode.ino,version,m->mask);

  /* Free the string created by print_uuid_be */
  free(uuid);
  
  res = PQexec(conn, cmd);
  if (PQresultStatus(res) != PGRES_TUPLES_OK){
    handle_err(res,conn,"insert file_p",PQerrorMessage(conn)); 
    printf("The command was -- %s\n",cmd);
  }
  /* Clear the result of the second command */
  PQclear(res);
  
  return 0;
}

int handle_link(struct provmsg *msg, void *arg)
{
  struct provmsg_link *m = (struct provmsg_link *) msg;
  struct args *args = (struct args *) arg;
  int len;
  if (!want_cred(args, msg->cred_id))
    return 0;
  
  len = msg_getlen(msg) - sizeof(*m);
  
  /* Extract the event id for the insertion into the main table */
  unsigned long eventid = insert_into_main(m->msg.cred_id,PROVMSG_LINK);
  
  /* Extract the uuid into a string */
  char * uuid = print_uuid_be(&m->inode.sb_uuid);

  /* Create an entry in the event-specific table */	
  if (len == 0) {
    /* We are linking the root directory */
    /* inode_dir == -1 means that we are linking the root directory */
    snprintf(cmd,CMDLEN,"insert into link(eventid,uuid_be,inode_dir,inode,fname) values(%lu,'{%s}',%d,%lu,'%s') returning eventid;",
	     eventid,uuid,-1,
	     (unsigned long)m->inode.ino,"root_inode");
  }
  else{
    /* Need to make filename play nice with postgres */
    char * fname = encode_to_utf8(m->fname,len);
    snprintf(cmd,CMDLEN,"insert into link(eventid,uuid_be,inode_dir,inode,fname) values(%lu,'{%s}',%lu,%lu,'%s') returning eventid;",
	     eventid,uuid,(unsigned long)m->dir,
	     (unsigned long)m->inode.ino,fname);
    free(fname);
  }
  
  /* Free the string created by print_uuid_be */
  free(uuid);
  
  res = PQexec(conn, cmd);
  if (PQresultStatus(res) != PGRES_TUPLES_OK){
    printf("%s\n",cmd);
    handle_err(res,conn,"insert link",PQerrorMessage(conn)); 
  }
 
 /* Clear the result of the second command */
  PQclear(res);
  
  return 0;
}

int handle_unlink(struct provmsg *msg, void *arg)
{
  struct provmsg_unlink *m = (struct provmsg_unlink *) msg;
  struct args *args = (struct args *) arg;

  int len;
  if (!want_cred(args, msg->cred_id))
    return 0;
  
  len = msg_getlen(msg) - sizeof(*m);

  /* Extract the event id for the insertion into the main table */
  unsigned long eventid = insert_into_main(m->msg.cred_id,PROVMSG_UNLINK);
  
  /* Extract the uuid into a string */
  char * uuid = print_uuid_be(&m->dir.sb_uuid);
  
  /* Need to mke filename play nice with postgres */
  char * fname = encode_to_utf8(m->fname,len);
  
  /* Create an entry in the event-specific table */
  snprintf(cmd,CMDLEN,"insert into unlink(eventid,uuid_be,inode_dir,fname) values(%lu,'{%s}',%lu,'%s') returning eventid;",
	   eventid,uuid,(unsigned long)m->dir.ino,fname);
  
  /* Free the string created by print_uuid_be */
  free(uuid);
  free(fname);
  
  res = PQexec(conn, cmd);
  if (PQresultStatus(res) != PGRES_TUPLES_OK)
    handle_err(res,conn,"insert unlink",PQerrorMessage(conn)); 
  
  /* Clear the result of the second command */
  PQclear(res);
  
  return 0;
}

int handle_setattr(struct provmsg *msg, void *arg)
{
  struct provmsg_setattr *m = (struct provmsg_setattr *) msg;
  struct args *args = (struct args *) arg;
  if (!want_cred(args, msg->cred_id))
    return 0;
  
  /* Extract the event id for the insertion into the main table */
  unsigned long eventid = insert_into_main(m->msg.cred_id,PROVMSG_SETATTR);
  
  /* Extract the uuid into a string */
  char * uuid = print_uuid_be(&m->inode.sb_uuid);
  
  /* Create an entry in the event-specific table */
  snprintf(cmd,CMDLEN,"insert into setattr(eventid,uuid_be,inode,uid,gid,mode) values(%lu,'{%s}',%lu,%u,%u,%o) returning eventid;",
	   eventid,uuid,(unsigned long)m->inode.ino,m->uid,m->gid,m->mode);
  
  res = PQexec(conn, cmd);
  if (PQresultStatus(res) != PGRES_TUPLES_OK)
    handle_err(res,conn,"insert setattr",PQerrorMessage(conn)); 
  
  /* Clear the result of the second command */
  PQclear(res);
  
  /* Free the string created by print_uuid_be */
  free(uuid);
  
  return 0;
}

int handle_inode_alloc(struct provmsg *msg, void *arg)
{
  struct provmsg_inode_alloc *m = (struct provmsg_inode_alloc *) msg;
  struct args *args = (struct args *) arg;
  if (!want_cred(args, msg->cred_id))
    return 0;
  
  /* Extract the event id for the insertion into the main table */
  unsigned long eventid = insert_into_main(m->msg.cred_id,PROVMSG_INODE_ALLOC);
  
  /* Extract the uuid into a string */
  char * uuid = print_uuid_be(&m->inode.sb_uuid);
  
  /* Create an entry in the event-specific table */
  snprintf(cmd,CMDLEN,"insert into inode_alloc(eventid,uuid_be,inode) values(%lu,'{%s}',%lu) returning eventid;",
	   eventid,uuid,(unsigned long)m->inode.ino);
  
  res = PQexec(conn, cmd);
  if (PQresultStatus(res) != PGRES_TUPLES_OK)
    handle_err(res,conn,"insert inode_alloc",PQerrorMessage(conn)); 
  
  /* Clear the result of the second command */
  PQclear(res);
  
  /* Free the string created by print_uuid_be */
  free(uuid);
  
  return 0;
}

int handle_inode_dealloc(struct provmsg *msg, void *arg)
{

  struct provmsg_inode_dealloc *m = (struct provmsg_inode_dealloc *) msg;
  struct args *args = (struct args *) arg;
  if (!want_cred(args, msg->cred_id))
    return 0;
  
  /* Extract the event id for the insertion into the main table */
  unsigned long eventid = insert_into_main(m->msg.cred_id,PROVMSG_INODE_DEALLOC);
  
  /* Extract the uuid into a string */
  char * uuid = print_uuid_be(&m->inode.sb_uuid);
  
  /* Create an entry in the event-specific table */
  snprintf(cmd,CMDLEN,"insert into inode_dealloc(eventid,uuid_be,inode) values(%lu,'{%s}',%lu) returning eventid;",
	   eventid,uuid,(unsigned long)m->inode.ino);
  
  res = PQexec(conn, cmd);
  if (PQresultStatus(res) != PGRES_TUPLES_OK)
    handle_err(res,conn,"insert inode_dealloc",PQerrorMessage(conn)); 
  
  /* Clear the result of the second command */
  PQclear(res);
  
  /* Free the string created by print_uuid_be */
  free(uuid);
  
  return 0;

}

int handle_socksend(struct provmsg *msg, void *arg)
{

  struct provmsg_socksend *m = (struct provmsg_socksend *) msg;
  struct args *args = (struct args *) arg;
  struct sockaddr * addr = (struct sockaddr *) &m->addr; 
  if (!want_cred(args, msg->cred_id))
    return 0;
  
  /* Extract the event id for the insertion into the main table */
  unsigned long eventid = insert_into_main(m->msg.cred_id,PROVMSG_SOCKSEND);
  
  /* Create an entry in the event-specific table */

  /* Case 1: Address is type IPv4 */
  if(addr->sa_family == AF_INET && m->addr_len > 0){
    snprintf(cmd,CMDLEN,"insert into socksend(eventid,low_sockid,high_sockid,family,protocol,address,port) values(%lu,%u,%u,%u,%u,'%s',%u) returning eventid;",
	     eventid,
	     m->peer.low,
	     m->peer.high,
	     m->family,
	     m->protocol,
	     inet_ntoa(((struct sockaddr_in *)addr)->sin_addr),
	     ((struct sockaddr_in *)addr)->sin_port);   
  }
  /* Case 2: Address is type IPv6 */
  else if(addr->sa_family == AF_INET6 && m->addr_len > 0){
    char straddr[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6,
	      &((struct sockaddr_in6 *)addr)->sin6_addr,
	      straddr,
	      sizeof(straddr));

    snprintf(cmd,CMDLEN,"insert into socksend(eventid,low_sockid,high_sockid,family,protocol,address,port) values(%lu,%u,%u,%u,%u,'%s',%u) returning eventid;",
	     eventid,
	     m->peer.low,
	     m->peer.high,
	     m->family,
	     m->protocol,
	     straddr,
	     ((struct sockaddr_in *)addr)->sin_port);   
  }
  /* Case 3: No sockaddr struct was embedded in the message */
  else
    snprintf(cmd,CMDLEN,"insert into socksend(eventid,low_sockid,high_sockid,family,protocol) values(%lu,%u,%u,%u,%u) returning eventid;",
	     eventid,
	     m->peer.low,
	     m->peer.high,
	     m->family,
	     m->protocol);

  res = PQexec(conn, cmd);
  if (PQresultStatus(res) != PGRES_TUPLES_OK){
    printf("ERROR: %s\n",cmd);
    handle_err(res,conn,"insert socksend",PQerrorMessage(conn)); 
  }
  /* Clear the result of the second command */
  PQclear(res);
  
  return 0;
}

int handle_sockrecv(struct provmsg *msg, void *arg)
{

  struct provmsg_sockrecv *m = (struct provmsg_sockrecv *) msg;
  struct args *args = (struct args *) arg;
  struct sockaddr * addr = (struct sockaddr *) &m->addr; 

  if (!want_cred(args, msg->cred_id))
    return 0;

  /* Extract the event id for the insertion into the main table */
  unsigned long eventid = insert_into_main(m->msg.cred_id,PROVMSG_SOCKRECV);
  
  /* Extract the uuid into a string */
  char * uuid = print_uuid_be(&m->sock.host);

  /* Create an entry in the event-specific table */

  /* Case 1: Address is type IPv4 */
  if(addr->sa_family == AF_INET && m->addr_len > 0)
    snprintf(cmd,CMDLEN,"insert into sockrecv(eventid,uuid_be,low_sockid,high_sockid,family,protocol,address,port) values(%lu,'{%s}',%u,%u,%u,%u,'%s',%u) returning eventid;",
	     eventid,
	     uuid,
	     m->sock.sock.low,
	     m->sock.sock.high,
	     m->family,
	     m->protocol,
	     inet_ntoa(((struct sockaddr_in *)addr)->sin_addr),
	     ((struct sockaddr_in *)addr)->sin_port);   
  /* Case 2: Address is type IPv6 */
  else if(addr->sa_family == AF_INET6 && m->addr_len > 0){
    char straddr[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6,
	      &((struct sockaddr_in6 *)addr)->sin6_addr,
	      straddr,
	      sizeof(straddr));

    snprintf(cmd,CMDLEN,"insert into sockrecv(eventid,uuid_be,low_sockid,high_sockid,family,protocol,address,port) values(%lu,'{%s}',%u,%u,%u,%u,'%s',%u) returning eventid;",
	     eventid,
	     uuid,
	     m->sock.sock.low,
	     m->sock.sock.high,
	     m->family,
	     m->protocol,
	     straddr,
	     ((struct sockaddr_in *)addr)->sin_port);   
  }
  /* Case 3: Address is type AF_UNIX */
  else if(addr->sa_family == AF_UNIX && m->addr_len > 0)
    snprintf(cmd,CMDLEN,"insert into sockrecv(eventid,uuid_be,low_sockid,high_sockid,family,protocol,address) values(%lu,'{%s}',%u,%u,%u,%u,'%s') returning eventid;",
	     eventid,
	     uuid,
	     m->sock.sock.low,
	     m->sock.sock.high,
	     m->family,
	     m->protocol,
	     ((struct sockaddr_un *)addr)->sun_path);
  /* Case 4: No sockaddr struct was embedded in the message */
  else
    snprintf(cmd,CMDLEN,"insert into sockrecv(eventid,uuid_be,low_sockid,high_sockid,family,protocol) values(%lu,'{%s}',%u,%u,%u,%u) returning eventid;",
	     eventid,
	     uuid,
	     m->sock.sock.low,
	     m->sock.sock.high,
	     m->family,
	     m->protocol);

  res = PQexec(conn, cmd);
  if (PQresultStatus(res) != PGRES_TUPLES_OK){
    printf("%s\n",cmd);
    handle_err(res,conn,"insert sockrecv",PQerrorMessage(conn)); 
  }

  /* Free the string created by print_uuid_be */
  free(uuid);  

  /* Clear the result of the second command */
  PQclear(res);

  return 0;
}

int handle_sockalias(struct provmsg *msg, void *arg)
{
  struct provmsg_sockalias *m = (struct provmsg_sockalias *) msg;
  struct args *args = (struct args *) arg;
  if (!want_cred(args, msg->cred_id))
    return 0;

  /* Extract the event id for the insertion into the main table */
  unsigned long eventid = insert_into_main(m->msg.cred_id,PROVMSG_SOCKALIAS);
  
  /* Extract the uuids into a string */
  char * uuid_sock = print_uuid_be(&m->sock.host);
  char * uuid_alias = print_uuid_be(&m->alias.host);

  /* Create an entry in the event-specific table */
  snprintf(cmd,CMDLEN,"insert into sockalias(eventid,uuid_be_sock,low_sockid,high_sockid,uuid_be_alias,low_aliasid,high_aliasid) values(%lu,'{%s}',%d,%d,'{%s}',%d,%d) returning eventid;",
	   eventid,uuid_sock,m->sock.sock.low,m->sock.sock.high,
	   uuid_alias,m->alias.sock.low,m->alias.sock.high);

  res = PQexec(conn, cmd);
  if (PQresultStatus(res) != PGRES_TUPLES_OK)
    handle_err(res,conn,"insert sockalias",PQerrorMessage(conn)); 
  
  /* Free the strings created by print_uuid_be */
  free(uuid_sock);
  free(uuid_alias);


  /* Clear the result of the second command */
  PQclear(res);

  return 0;
}

int handle_mqsend(struct provmsg *msg, void *arg)
{

  struct provmsg_mqsend *m = (struct provmsg_mqsend *) msg;
  struct args *args = (struct args *) arg;
  if (!want_cred(args, msg->cred_id))
    return 0;
  
  /* Extract the event id for the insertion into the main table */
  unsigned long eventid = insert_into_main(m->msg.cred_id,PROVMSG_MQSEND);

  /* Create an entry in the event-specific table */
  snprintf(cmd,CMDLEN,"insert into mqsend(eventid,msgid) values(%lu,%u) returning eventid;",
	   eventid,m->msgid);

  res = PQexec(conn, cmd);
  if (PQresultStatus(res) != PGRES_TUPLES_OK)
    handle_err(res,conn,"insert mqsend",PQerrorMessage(conn)); 
  
  /* Clear the result of the second command */
  PQclear(res);

  return 0;
}

int handle_mqrecv(struct provmsg *msg, void *arg)
{
  struct provmsg_mqrecv *m = (struct provmsg_mqrecv *) msg;
  struct args *args = (struct args *) arg;
  if (!want_cred(args, msg->cred_id))
    return 0;

  /* Extract the event id for the insertion into the main table */
  unsigned long eventid = insert_into_main(m->msg.cred_id,PROVMSG_MQRECV);

  /* Create an entry in the event-specific table */
  snprintf(cmd,CMDLEN,"insert into mqrecv(eventid,msgid) values(%lu,%u) returning eventid;",
	   eventid,m->msgid);

  res = PQexec(conn, cmd);
  if (PQresultStatus(res) != PGRES_TUPLES_OK)
    handle_err(res,conn,"insert mqrecv",PQerrorMessage(conn)); 
  
  /* Clear the result of the second command */
  PQclear(res);

  return 0;
}

int handle_shmat(struct provmsg *msg, void *arg)
{
  struct provmsg_shmat *m = (struct provmsg_shmat *) msg;
  struct args *args = (struct args *) arg;
  if (!want_cred(args, msg->cred_id))
    return 0;
  
  /* Extract the event id for the insertion into the main table */
  unsigned long eventid = insert_into_main(m->msg.cred_id,PROVMSG_SHMAT);

  /* Create an entry in the event-specific table */
  snprintf(cmd,CMDLEN,"insert into shmat(eventid,shmid,flags) values(%lu,%u,%u) returning eventid;",
	   eventid,m->shmid,m->flags);

  res = PQexec(conn, cmd);
  if (PQresultStatus(res) != PGRES_TUPLES_OK)
    handle_err(res,conn,"insert shmat",PQerrorMessage(conn)); 
  
  /* Clear the result of the second command */
  PQclear(res);

  return 0;
}

int handle_readlink(struct provmsg *msg, void *arg)
{

  struct provmsg_readlink *m = (struct provmsg_readlink *) msg;
  struct args *args = (struct args *) arg;
  if (!want_cred(args, msg->cred_id))
    return 0;

  /* Extract the event id for the insertion into the main table */
  unsigned long eventid = insert_into_main(m->msg.cred_id,PROVMSG_READLINK);

  /* Extract the uuid into a string */
  char * uuid = print_uuid_be(&m->inode.sb_uuid);
  
  /* Create an entry in the event-specific table */
  snprintf(cmd,CMDLEN,"insert into readlink(eventid,uuid_be,inode) values(%lu,'{%s}',%lu) returning eventid;",
	   eventid,uuid,(unsigned long)m->inode.ino);
  
  /* Free the string created by print_uuid_be */
  free(uuid);
  
  res = PQexec(conn, cmd);
  if (PQresultStatus(res) != PGRES_TUPLES_OK)
    handle_err(res,conn,"insert readlink",PQerrorMessage(conn)); 
  
  /* Clear the result of the second command */
  PQclear(res);

  return 0;
}

