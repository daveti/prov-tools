#include "neo_handlers.h"

void exit_nicely(P) {
  exit(1);
}

void handle_err(char * cmd, char * err){
  fprintf(stderr, "%s command failed: %s\n", cmd,err);
}

char * encode_to_utf8(char * cstring, int len){

  char * newstring = malloc(len+1);
  //memcpy(newstring,cstring,len);
  //newstring[len]='\0';

  if(!newstring){
    printf("Out of memory\n");
    exit_nicely();
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

char * argv_to_label(char * cstring, int len, int argc){

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
    exit_nicely();
  }

  int newcursor = 0;
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
	argc -= 1;
	if(argc == 0){
	  newstring[newcursor] = '\0';
	  return newstring;
	}
	newstring[newcursor] = ' '; newcursor ++;
    }
    oldcursor++;
  }

  // If EOL 
  newstring[newcursor] = '\0';
  
  return newstring;  
  
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


int handle_boot(struct provmsg *msg) {

  struct provmsg_boot *m = (struct provmsg_boot *) msg;

  json_t * obj;
  char * text;

  /* Extract the uuid into a string */
  unsigned long cred_id = m->msg.cred_id;
  char * uuid = print_uuid_be(&m->uuid);

  /* Create Activity node for Child */
  snprintf(cypher_req, MAXREQLEN, "CREATE (n:Activity {id : \"%lu\", uuid : \"%s\"})",
	   cred_id,
	   uuid);

  obj = json_pack("{s:[{s:s}]}",
		  "statements",
		  "statement", 
		  cypher_req);

  text = post(NEO_URL,json_dumps(obj,0));

  free(text);
  free(obj);
  free(uuid);

  return 0;
}

int handle_credfork(struct provmsg *msg) {

  struct provmsg_credfork *m = (struct provmsg_credfork *) msg;

  unsigned long cred_id = m->msg.cred_id;
  unsigned long forked_cred = m->forked_cred;

  json_t * obj;
  char * text;

  /* Create Activity node for Child */
  snprintf(cypher_req, MAXREQLEN, "CREATE (n:Activity {id : \"%lu\", parentid : \"%lu\"})",
	   forked_cred, cred_id);


  obj = json_pack("{s:[{s:s}]}",
		  "statements",
		  "statement", 
		  cypher_req);

  text = post(NEO_URL,json_dumps(obj,0));

  free(text);
  free(obj);

  /* Create relationship */
  snprintf(cypher_req, MAXREQLEN, 
	   "MATCH (a:Activity), (b:Activity) "
	   "WHERE a.id = \"%lu\" AND b.id = \"%lu\" "
	   "CREATE (a)-[r:WasForkedFrom]->(b) ",
	   forked_cred, cred_id);
 
  obj = json_pack("{s:[{s:s}]}",
		  "statements",
		  "statement", 
		  cypher_req);
  
  text = post(NEO_URL,json_dumps(obj,0));

  free(text);
  free(obj);
  
  return 0;
}


int handle_credfree(struct provmsg *msg) {
  
  return 0;
}

int handle_setid(struct provmsg *msg) {
  
  return 0;
}


int handle_exec(struct provmsg *msg)
{
  struct provmsg_exec *m = (struct provmsg_exec *) msg;

  json_t * obj;
  char * text;

  /* Extract the uuid into a string */
  char * uuid = print_uuid_be(&m->inode.sb_uuid);
  int argc = m->argc;
  char * argv =  argv_to_label(m->argv_envp, msg_getlen(msg) - sizeof(*m),argc);

  unsigned long version = m->inode_version;
  unsigned long cred_id = m->msg.cred_id;

  /* Update Activity node */
  snprintf(cypher_req, MAXREQLEN, "MATCH (n:Activity {id : \"%lu\"}) SET n.label = n.label + '%s'",
	   cred_id, argv);

  obj = json_pack("{s:[{s:s}]}",
		  "statements",
		  "statement", 
		  cypher_req);

  text = post(NEO_URL,json_dumps(obj,0));

  free(text);
  free(obj);
  free(uuid);
  free(argv);
  
  return 0;
}

int handle_inode_p(struct provmsg *msg) {
  
  return 0;
}

int handle_mmap(struct provmsg *msg) {
  
  return 0;
}

int handle_file_p(struct provmsg *msg)
{

  struct provmsg_file_p *m = (struct provmsg_file_p *) msg;

  json_t * obj;
  char * text;

  if (!(m->mask & MAY_READ) &&
      !(m->mask & MAY_WRITE) &&
      !(m->mask & MAY_APPEND) )
    return 0;

  /* Extract provmsg fields */
  char * uuid = print_uuid_be(&m->inode.sb_uuid);
  unsigned long inode = m->inode.ino;
  unsigned long version = m->inode_version;
  int mask = m->mask;
  int cred = m->msg.cred_id;

  /* Create InodeVersion node */
  snprintf(cypher_req, MAXREQLEN, "CREATE (n:InodeVersion {id : \"%lu:%lu\", uuid : \"%s\", inode : %lu, version : %lu, mask : %d})",
	   inode,
	   version,
	   uuid,
	   inode,
	   version,
	   mask);


  obj = json_pack("{s:[{s:s}]}",
		  "statements",
		  "statement", 
		  cypher_req);

  text = post(NEO_URL,json_dumps(obj,0));

  free(text);
  free(obj);

  /* Create Activity node */
  snprintf(cypher_req, MAXREQLEN, "CREATE (n:Activity {id : \"%d\"})",
	   cred);

  obj = json_pack("{s:[{s:s}]}",
		  "statements",
		  "statement", 
		  cypher_req);

  text = post(NEO_URL,json_dumps(obj,0));

  free(text);
  free(obj);

  /* Create relationship */
  int create_relationship = 0;
  if (m->mask & MAY_READ){
    snprintf(cypher_req, MAXREQLEN, 
	     "MATCH (a:InodeVersion), (b:Activity) "
	     "WHERE a.id = \"%lu:%lu\" AND b.id = \"%d\" "
	     "CREATE (b)-[r:Used]->(a) "
	     "RETURN r;",
	     inode,
	     version,
	     cred);;
    create_relationship = 1;
  }
  else if ( m->mask & MAY_WRITE || m->mask & MAY_APPEND ) {
    snprintf(cypher_req, MAXREQLEN, 
	     "MATCH (a:InodeVersion), (b:Activity) "
	     "WHERE a.id = \"%lu:%lu\" AND b.id = \"%d\" "
	     "CREATE (a)-[r:WasGeneratedBy]->(b) ",
	     //"RETURN r;",
	     inode,
	     version,
	     cred);
    create_relationship = 1;
  }

  if ( create_relationship ) {
    obj = json_pack("{s:[{s:s}]}",
		    "statements",
		    "statement", 
		    cypher_req);
    
    text = post(NEO_URL,json_dumps(obj,0));
    free(text);
    free(obj);
  }

  free(uuid);

  return 0;

}

int handle_link(struct provmsg *msg)
{
  
  return 0;
}

int handle_unlink(struct provmsg *msg)
{
  
  return 0;
}

int handle_setattr(struct provmsg *msg)
{
  
  return 0;
}

int handle_inode_alloc(struct provmsg *msg)
{
  
  return 0;
}

int handle_inode_dealloc(struct provmsg *msg)
{
  
  return 0;

}

int handle_socksend(struct provmsg *msg)
{
  
  return 0;
}

int handle_sockrecv(struct provmsg *msg)
{

  return 0;
}

int handle_sockalias(struct provmsg *msg)
{

  return 0;
}

int handle_mqsend(struct provmsg *msg)
{

  return 0;
}

int handle_mqrecv(struct provmsg *msg)
{

  return 0;
}

int handle_shmat(struct provmsg *msg)
{

  return 0;
}

int handle_readlink(struct provmsg *msg)
{

  return 0;
}

