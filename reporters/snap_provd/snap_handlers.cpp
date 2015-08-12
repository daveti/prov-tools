#include "snap_handlers.hpp"

void exit_nicely() {
  exit(1);
}

void handle_err(char * cmd, char * err){
  fprintf(stderr, "%s command failed: %s\n", cmd,err);
  exit_nicely();
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
  char * newstring = (char *)malloc(newlength);
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
  char * rv = (char *)malloc(37);
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

ProvenanceGraph::ProvenanceGraph(){

  Graph = TNEANet::New();

}

int ProvenanceGraph::handle(int type, struct provmsg *msg) {
  
  if(type == PROVMSG_BOOT)
    return handle_boot(msg);
  else if(type == PROVMSG_CREDFORK)
    return handle_credfork(msg);
  else if(type == PROVMSG_CREDFREE)
    return handle_credfree(msg);
  else if(type == PROVMSG_EXEC)
    return handle_exec(msg);
  else if(type == PROVMSG_FILE_P)
    return handle_file_p(msg);
  else if(type == PROVMSG_SETID)
    return handle_setid(msg);
  else if(type == PROVMSG_SOCKSEND)
    return handle_socksend(msg);
  else if(type == PROVMSG_SOCKRECV)
    return handle_sockrecv(msg);
  return -1;
}

int ProvenanceGraph::GetEdgeAttr(TNEANet::TEdgeI Edge, char const * key){

  /*
  TStrV attrs,values;
  Edge.GetStrAttrNames(attrs);
  Edge.GetStrAttrVal(values);
  for(int i=0; i < attrs.Len(); i++){
    if(attrs[i] == key)
      return values[i];
  }
  */
  return Graph->GetIntAttrDatE(Edge.GetId(),TStr(key));
}

TStr ProvenanceGraph::GetNodeAttr(TNEANet::TNodeI Node, char const * key){

  TStrV attrs,values;
  Node.GetStrAttrNames(attrs);
  Node.GetStrAttrVal(values);
  for(int i=0; i < attrs.Len(); i++){
    if(attrs[i] == key)
      return values[i];
  }
  return TStr("");
}

void ProvenanceGraph::DumpNode(int TgtId){

  TNEANet::TEdgeI Edge;
  int EdgeId;
  TStrV attrs, values;
  TIntV valueI;
  TNEANet::TNodeI TargetNode = Graph->GetNI(TgtId);
      
  printf("Node %d has %d incoming edges and %d outgoing edges\n",
	 TgtId, TargetNode.GetInDeg(),TargetNode.GetOutDeg());

  TargetNode.GetAttrNames(attrs); 
  TargetNode.GetAttrVal(values); 
  for(int i=0; i < attrs.Len(); i++){
    printf("\t%s: %s\n",attrs[i].CStr(),values[i].CStr());
  }
      
  for(int i=0; i<TargetNode.GetInDeg(); i++){
    EdgeId = TargetNode.GetInEId(i);
    printf("\tEID=%d\n",EdgeId);
    Edge = Graph->GetEI(EdgeId);
    printf("\t\t%d -> %d\n",Edge.GetSrcNId(),Edge.GetDstNId());
    
    Edge.GetAttrNames(attrs); 
    Edge.GetIntAttrVal(valueI); 
    for(int i=0; i < attrs.Len(); i++){
      printf("\t\t\t%s: %d\n",attrs[i].CStr(),valueI[i].Val);
    }
  }
  
  for(int i=0; i<TargetNode.GetOutDeg(); i++){
    EdgeId = TargetNode.GetOutEId(i);
    printf("\tEID=%d\n",EdgeId);
    Edge = Graph->GetEI(EdgeId);
    printf("\t%d -> %d\n",Edge.GetSrcNId(),Edge.GetDstNId());
    
    Edge.GetAttrNames(attrs); 
    Edge.GetIntAttrVal(valueI); 
    for(int i=0; i < attrs.Len(); i++){
      printf("\t\t\t%s: %d\n",attrs[i].CStr(),valueI[i].Val);
    }
  }

  return;


  
}
 
std::map<int,int> ProvenanceGraph::RecurseAncestors(int TgtId) {

  TNEANet::TNodeI tgt = Graph->GetNI(TgtId);
  TNEANet::TEdgeI Edge;
  TInt EdgeId;
  TStrV attrs,values;
  std::map<int,int> ancestors;

  /* Check to see if this node already has a cache entry */
  if (ANCESTOR_CACHE && AncestorCache.find(TgtId) != AncestorCache.end() ) { 
    /* We have already traversed this subgraph.
       Copy results into current ancestor list and return */
    for (std::map<int,int>::iterator it=AncestorCache[TgtId].begin();
	 it!=AncestorCache[TgtId].end(); ++it) 
	  ancestors[it->first] = 0;
  }
  /* Add self to ancestor list if an inode and recurse "WasGeneratedBy" */  
  else if ( SNAP_SUBTYPE_INODE == Graph->GetIntAttrDatN(TgtId,"subtype") ) {
    ancestors[TgtId] = 0;  

    for(int i=0; i < tgt.GetOutDeg(); i++) {
      EdgeId = tgt.GetOutEId(i);
      Edge = Graph->GetEI(EdgeId);
      
      if( GetEdgeAttr(Edge,"rel") == SNAP_RELATION_WASGENERATEDBY ) {
	std::map<int,int> subancestors = RecurseAncestors(Edge.GetDstNId());

	for (std::map<int,int>::iterator it=subancestors.begin();
	     it!=subancestors.end(); ++it) 
	  ancestors[it->first] = 0;
      }
    }

    if(ANCESTOR_CACHE)
      AncestorCache[TgtId] = ancestors;
  }
  /* If a fork, recurse "Used" edge */
  else if( SNAP_SUBTYPE_FORK == Graph->GetIntAttrDatN(TgtId,"subtype") ) {

    for(int i=0; i < tgt.GetOutDeg(); i++) {
      EdgeId = tgt.GetOutEId(i);      
      Edge = Graph->GetEI(EdgeId);

      if( GetEdgeAttr(Edge,"rel") == SNAP_RELATION_USED ) {
	std::map<int,int> subancestors = RecurseAncestors(Edge.GetDstNId());	  

	for (std::map<int,int>::iterator it=subancestors.begin();
	     it!=subancestors.end(); ++it) 
	  ancestors[it->first] = 0;
      }
    }
  }

  return ancestors;

}

int ProvenanceGraph::GetAncestors(uint64_t inode, uint64_t version){

  struct InodeVersion IvKey;
  IvKey.inode = inode;
  IvKey.version = version;

  int TgtId = get_or_create_node(SNAP_TYPE_ENTITY,SNAP_SUBTYPE_INODE, (void *) &IvKey);

  std::map<int,int> ancestors = RecurseAncestors(TgtId);

  return 0;
}

int ProvenanceGraph::get_or_create_node(int type, int subtype, void * key){

  TInt NodeId = -1;
  switch (type) {
    
  case SNAP_TYPE_ACTIVITY:
    if ( CredMap.find(*(TInt*)key) != CredMap.end() )
      NodeId = CredMap[*(TInt*)key];      
    else{
      NodeId = Graph->AddNode(Graph->GetMxNId());
      Graph->AddIntAttrDatN(NodeId,*(TInt*)key,"id");
      Graph->AddIntAttrDatN(NodeId,SNAP_TYPE_ACTIVITY,"type");
      Graph->AddIntAttrDatN(NodeId,subtype,"subtype");
      CredMap[*(TInt*)key] = NodeId;
    }
    break;
    
  case SNAP_TYPE_ENTITY:
  
    switch(subtype){
      
    case SNAP_SUBTYPE_INODE:
      InodeVersion iv = *(InodeVersion*)key;

      if ( InodeVersionMap.find(iv) != InodeVersionMap.end() )
	NodeId = InodeVersionMap[iv];      
      else {
	NodeId = Graph->AddNode(Graph->GetMxNId());
	Graph->AddFltAttrDatN(NodeId,iv.inode,"inode");
	Graph->AddFltAttrDatN(NodeId,iv.version,"version");
	Graph->AddIntAttrDatN(NodeId,SNAP_TYPE_ENTITY,"type");
	Graph->AddIntAttrDatN(NodeId,SNAP_SUBTYPE_INODE,"subtype");
	InodeVersionMap[*(InodeVersion*)key] = NodeId;
      }
      break;

    }

  case SNAP_TYPE_AGENT:
    
    switch(subtype){
      
    case SNAP_SUBTYPE_USER:
      User user = *(User*)key;
      if ( UserMap.find(user) != UserMap.end() )
	NodeId = UserMap[user];      
      else {
	NodeId = Graph->AddNode(Graph->GetMxNId());
	Graph->AddIntAttrDatN(NodeId,user.uid,"uid");
	Graph->AddIntAttrDatN(NodeId,user.suid,"suid");
	Graph->AddIntAttrDatN(NodeId,user.euid,"euid");
	Graph->AddIntAttrDatN(NodeId,user.fsuid,"fsuid");
	Graph->AddIntAttrDatN(NodeId,SNAP_TYPE_AGENT,"type");
	Graph->AddIntAttrDatN(NodeId,SNAP_SUBTYPE_USER,"subtype");
	UserMap[*(User*)key] = NodeId;
      }
      break;
    }
    

  }

  return NodeId;

}

int ProvenanceGraph::get_edge(int SrcId, int DstId) {

  EdgePair e;
  e.SrcId = SrcId;
  e.DstId = DstId;

  std::map<EdgePair,TInt>::iterator it = EdgeMap.find(e);
  if ( EdgeMap.find(e) == EdgeMap.end() )
    return -1;
  else
    return it->second;

}

int ProvenanceGraph::create_edge(int SrcId, int DstId, int rel) {

  int EdgeId = Graph->AddEdge(SrcId,DstId,Graph->GetMxEId());
  Graph->AddIntAttrDatE(EdgeId,rel,"rel");

  EdgePair e;
  e.SrcId = SrcId;
  e.DstId = DstId;
  EdgeMap[e] = EdgeId;

  return EdgeId;

}

int ProvenanceGraph::get_or_create_edge(int SrcId, int DstId, int rel) {

  /* Check to see if the edge already exists */
  /* Only create new if edge does not already exist */ 
  if( SrcId == DstId )
    return -1;

  int EdgeId = get_edge(SrcId,DstId);

  /* Only create new if edge does not already exist */
  if(EdgeId < 0) 
    EdgeId = create_edge(SrcId,DstId,rel);

  return EdgeId;

}

int ProvenanceGraph::handle_boot(struct provmsg *msg){

  struct provmsg_boot *m = (struct provmsg_boot *) msg;
 
  /* Extract the uuid into a string */
  //char * uuid = print_uuid_be(&m->uuid);

  /* Create new Cred Node */
  int BootId = get_or_create_node(SNAP_TYPE_ACTIVITY,SNAP_SUBTYPE_BOOT,(void *) &m->msg.cred_id);
  //Graph->AddStrAttrDatN(BootId,uuid,"uuid");

  CredMap[m->msg.cred_id] = BootId;

  return 0;
}

int ProvenanceGraph::handle_credfork(struct provmsg *msg) {

  struct provmsg_credfork *m = (struct provmsg_credfork *) msg;

  int NewForkId, OldForkId, EdgeId;

  /* Get parent Cred Node */
  OldForkId = get_or_create_node(SNAP_TYPE_ACTIVITY,SNAP_SUBTYPE_BOOT,(void *) &m->msg.cred_id);

  /* Create new child Cred Node */
  NewForkId = get_or_create_node(SNAP_TYPE_ACTIVITY,SNAP_SUBTYPE_FORK,(void *) &m->forked_cred);
  Graph->AddIntAttrDatN(NewForkId,m->msg.cred_id,"parentid");

  EdgeId = get_or_create_edge(NewForkId,OldForkId,SNAP_RELATION_WASFORKEDFROM);

  return 0;

}

int ProvenanceGraph::handle_exec(struct provmsg *msg){

  struct provmsg_exec *m = (struct provmsg_exec *) msg;

  /* Extract the uuid into a string */
  //char * uuid = print_uuid_be(&m->inode.sb_uuid);
  char * new_label =  argv_to_label(m->argv_envp, msg_getlen(msg) - sizeof(*m),m->argc);

  TInt ForkId = CredMap[m->msg.cred_id];

  /* Get previous Id, if any */
  TStr label = TStr("");//Graph->GetStrAttrDatN(ForkId,"label");

  /* Update label if one already exists */

  if(strlen(label.CStr()) > 0)
    label = label + "\n" + new_label;

  Graph->AddStrAttrDatN(ForkId,label,"label");

  /* Note: Right now this actually overwrites old information
     if multiple processes are launched in the same fork */
  Graph->AddIntAttrDatN(ForkId,m->inode.ino,"inode");
  Graph->AddIntAttrDatN(ForkId,m->inode_version,"version");
  //Graph->AddStrAttrDatN(ForkId,uuid,"uuid");

  //free(uuid);
  free(new_label);

  return 0;

}

int ProvenanceGraph::handle_file_p(struct provmsg *msg){

  struct provmsg_file_p *m = (struct provmsg_file_p *) msg;
  TInt InodeVersionId;

  /* Basically just ignoring "May Execute" */
  if (!(m->mask & MAY_READ) &&
      !(m->mask & MAY_WRITE) &&
      !(m->mask & MAY_APPEND) )
    return 0;

  /* Extract the uuid into a string */
  //char * uuid = print_uuid_be(&m->inode.sb_uuid);

  /* Lookup the InodeVersion node */
  struct InodeVersion iv;
  iv.inode = m->inode.ino;
  iv.version = m->inode_version;

  InodeVersionId = get_or_create_node(SNAP_TYPE_ENTITY,
				      SNAP_SUBTYPE_INODE,
				      (InodeVersion *)&iv);

  //Graph->AddStrAttrDatN(InodeVersionId,uuid,"uuid");

  /* Get the Cred node  */
  TInt CredId = CredMap[m->msg.cred_id];

  TInt EdgeId;
  /* If a write/append */
  if ( (m->mask & MAY_WRITE) ||
       (m->mask & MAY_APPEND) ) {
    EdgeId = get_or_create_edge(InodeVersionId,CredId,SNAP_RELATION_WASGENERATEDBY);

    /* Pre-compute ancestors for the new node */
    if(ANCESTOR_CACHE)
      RecurseAncestors(InodeVersionId);

  }

  /* If a read */
  /* Check to see if there already exists a write node between these two nodes */
  /* If so, do not create this read relationship because doing so would create */
  /* a cycle. If you have written to the file your knowledge of its contents */
  /* is implied */
  if ( m->mask & MAY_READ && get_edge(InodeVersionId,CredId) < 0 ){
    EdgeId = get_or_create_edge(CredId,InodeVersionId,SNAP_RELATION_USED);
  }
  return 0;
}

int ProvenanceGraph::handle_credfree(struct provmsg *msg){

  struct provmsg_credfork *m = (struct provmsg_credfork *) msg;

  /* I guess if we see a credfree it is safe to remove it from the map! */
  CredMap.erase (m->msg.cred_id);

  return 0;

}

int ProvenanceGraph::handle_setid(struct provmsg *msg){

  struct provmsg_setid *m = (struct provmsg_setid *) msg;

  TInt CredId = get_or_create_node(SNAP_TYPE_ACTIVITY,SNAP_SUBTYPE_FORK,(TInt *)&m->msg.cred_id);

  /* Create user / group keys */
  struct User user;
  user.uid = m->uid;
  user.suid = m->suid;
  user.euid = m->euid;
  user.fsuid = m->fsuid;

  struct Group group;
  group.gid = m->gid;
  group.sgid = m->sgid;
  group.egid = m->egid;
  group.fsgid = m->fsgid;
  
  /* If user does not already exist, create */
  TInt UserId = get_or_create_node(SNAP_TYPE_AGENT,
				   SNAP_SUBTYPE_USER,
				   (User *)&user);

  /* If group does not already exist, create */
  TInt GroupId;
  if ( GroupMap.find(group) == GroupMap.end() ){
    GroupId = Graph->AddNode(Graph->GetMxNId());
    
    Graph->AddIntAttrDatN(GroupId,m->uid,"uid");
    Graph->AddIntAttrDatN(GroupId,m->suid,"suid");
    Graph->AddIntAttrDatN(GroupId,m->euid,"euid");
    Graph->AddIntAttrDatN(GroupId,m->fsuid,"fsuid");
    Graph->AddIntAttrDatN(GroupId,SNAP_TYPE_AGENT,"type");
    Graph->AddIntAttrDatN(GroupId,SNAP_SUBTYPE_GROUP,"subtype");

    GroupMap[group] = GroupId;
  }
  else
    GroupId = GroupMap[group];

  TInt UserEdgeId,GroupEdgeId;
  /* If setId was previously called for the cred, we have to conservatively
     assume that both ID's are responsbile. So adding multiple User Nodes for
     a single cred is ok */
  UserEdgeId = get_or_create_edge(CredId,UserId,SNAP_RELATION_WASCONTROLLEDBY);

  /* If setId was previously called for the cred, we have to conservatively
     assume that both ID's are responsbile. So adding multiple Group Nodes for
     a single cred is ok */

  /* .... But only create this relationship if it doesn't already exist */
  GroupEdgeId = get_or_create_edge(UserId,GroupId,SNAP_RELATION_ACTEDONBEHALFOF);

  return 0;

}

int ProvenanceGraph::handle_socksend(struct provmsg *msg){

  struct provmsg_socksend *m = (struct provmsg_socksend *) msg;
  struct sockaddr * addr = (struct sockaddr *) &m->addr; 

  struct CredIpPort cip;

  /* Extract the uuid into a string */
  //char * uuid = print_uuid_be(&m->inode.sb_uuid);

  /* Case 1: Address is type IPv4 */
  if(addr->sa_family == AF_INET && m->addr_len > 0){
    cip.cred_id = m->msg.cred_id;
    cip.port = ((struct sockaddr_in *)addr)->sin_port;
    cip.ip_address = inet_ntoa(((struct sockaddr_in *)addr)->sin_addr);

    TInt CredIpPortId;
    if ( SockSendMap.find(cip) == SockSendMap.end() ){
      CredIpPortId = Graph->AddNode(Graph->GetMxNId());
      Graph->AddStrAttrDatN(CredIpPortId,cip.ip_address,"ip_address");
      Graph->AddFltAttrDatN(CredIpPortId,cip.port,"port");
      Graph->AddIntAttrDatN(CredIpPortId,m->protocol,"protocol");
      Graph->AddIntAttrDatN(CredIpPortId,m->protocol,"family");
      Graph->AddIntAttrDatN(CredIpPortId,SNAP_TYPE_ENTITY,"type");
      Graph->AddIntAttrDatN(CredIpPortId,SNAP_SUBTYPE_PACKET,"subtype");

      /* Update map if new CredIpPort Node was created */
      SockSendMap[cip] = CredIpPortId;
    }
    else
      CredIpPortId = SockSendMap[cip];

    /* Get the Cred node */
    TInt CredId = CredMap[m->msg.cred_id];
    
    TInt EdgeId = get_or_create_edge(CredIpPortId,CredId,SNAP_RELATION_WASGENERATEDBY);

  }
  /* Case 2: Address is type IPv6 */
  else if(addr->sa_family == AF_INET6 && m->addr_len > 0){
  }

  return 0;
}

/* DOES NOT WORK RIGHT NOW */
int ProvenanceGraph::handle_sockrecv(struct provmsg *msg){

  struct provmsg_sockrecv *m = (struct provmsg_sockrecv *) msg;
  struct sockaddr * addr = (struct sockaddr *) &m->addr; 

  struct CredIpPort cip;

  /* Extract the uuid into a string */
  //char * uuid = print_uuid_be(&m->inode.sb_uuid);

  /* Case 1: Address is type IPv4 */
  if(addr->sa_family == AF_INET && m->addr_len > 0){
    cip.cred_id = m->msg.cred_id;
    cip.port = ((struct sockaddr_in *)addr)->sin_port;
    cip.ip_address = inet_ntoa(((struct sockaddr_in *)addr)->sin_addr);

    TInt CredIpPortId;
    if ( SockRecvMap.find(cip) == SockRecvMap.end() ){
      CredIpPortId = Graph->AddNode(Graph->GetMxNId());
      Graph->AddStrAttrDatN(CredIpPortId,cip.ip_address,"ip_address");
      Graph->AddFltAttrDatN(CredIpPortId,cip.port,"port");
      Graph->AddIntAttrDatN(CredIpPortId,m->protocol,"protocol");
      Graph->AddIntAttrDatN(CredIpPortId,m->protocol,"family");
      Graph->AddIntAttrDatN(CredIpPortId,SNAP_TYPE_ENTITY,"type");
      Graph->AddIntAttrDatN(CredIpPortId,SNAP_SUBTYPE_PACKET,"subtype");
      
      /* Update map if new CredIpPort Node was created */
      SockRecvMap[cip] = CredIpPortId;
    }
    else
      CredIpPortId = SockRecvMap[cip];

    /* Get the Cred node */
    TInt CredId = CredMap[m->msg.cred_id];

    TInt EdgeId = get_or_create_edge(CredId,CredIpPortId,SNAP_RELATION_USED);


  }
  /* Case 2: Address is type IPv6 */
  else if(addr->sa_family == AF_INET6 && m->addr_len > 0){
  }

  return 0;
}

int ProvenanceGraph::handle_inode_p(struct provmsg *msg){return 0;}
int ProvenanceGraph::handle_mmap(struct provmsg *msg){return 0;}
int ProvenanceGraph::handle_link(struct provmsg *msg){return 0;}
int ProvenanceGraph::handle_unlink(struct provmsg *msg){return 0;}
int ProvenanceGraph::handle_setattr(struct provmsg *msg){return 0;}
int ProvenanceGraph::handle_inode_alloc(struct provmsg *msg){return 0;}
int ProvenanceGraph::handle_inode_dealloc(struct provmsg *msg){return 0;}
int ProvenanceGraph::handle_sockalias(struct provmsg *msg){return 0;}
int ProvenanceGraph::handle_mqsend(struct provmsg *msg){return 0;}
int ProvenanceGraph::handle_mqrecv(struct provmsg *msg){return 0;}
int ProvenanceGraph::handle_shmat(struct provmsg *msg){return 0;}
int ProvenanceGraph::handle_readlink(struct provmsg *msg){return 0;}





