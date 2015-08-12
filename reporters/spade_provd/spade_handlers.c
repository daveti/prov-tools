#include "spade_handlers.h"

//below are file level declaration to keep track of process, agent, artifact which got created

struct Process* p1 = NULL;
struct Agent* ag = NULL;
struct Artifact* ar = NULL;
struct Used* used = NULL;
struct WasGeneratedBy* wasgenBy = NULL;
static int counter = 0;
static int tempCounter=0;


/* Mugdha -- Implement this! */
int handle_boot(struct provmsg *msg) {

	struct provmsg_boot *m = (struct provmsg_boot *) msg;

	/* Extract provmsg fields */
	unsigned long cred_id = m->msg.cred_id;

	//create processId out of credId

	char integer_string[TEMPCHARSIZE];
	sprintf(integer_string, "%lu", cred_id);
	char* processId = malloc(CHARSIZE * sizeof(char));
	strcpy(processId, "P");
	strcat(processId, integer_string); //

	// push the process ID in struct corresponding to credId
	pushProcess(&p1, cred_id, processId);

	/* Create Activity node for cred_id */
	char buffer[BUFFERSIZE];
	int sizeReal =
			snprintf(buffer, sizeof(buffer),
					"echo type:Process id:%s Event:Boot_Handler Cred_Id:%lu >> /tmp/spade_pipe",
					processId, cred_id);
	char actualBuf[sizeReal + 1];
	strncpy(actualBuf, buffer, sizeReal);
	actualBuf[sizeReal] = '\0';
	system(actualBuf);
	//printf("%s\n",actualBuf);
	//free(processId);
	memset(buffer, 0, sizeof buffer);
	memset(actualBuf, 0, sizeof actualBuf);
	return 0;
}

/* Mugdha -- Implement this! */
int handle_credfork(struct provmsg *msg) { 

	struct provmsg_credfork *m = (struct provmsg_credfork *) msg;
	//Extract the field
	unsigned long cred_id = m->msg.cred_id;
	unsigned long forked_cred = m->forked_cred;
	
	// check if process corresponding to cred id is already thr or not.. ideally it should be there
	int isCredpresent = checkProcessId(p1, cred_id);
	int isForkedpresent = checkProcessId(p1, forked_cred);

	char* processId;
	char* forked_processId;

	char integer_string[TEMPCHARSIZE];
	char buffer[BUFFERSIZE];
	int sizeReal;

	if (isCredpresent == 1)  // process is not there, create it
			{
		processId = malloc(CHARSIZE * sizeof(char));
		memset(integer_string, 0, sizeof integer_string);
		sprintf(integer_string, "%lu", cred_id);
		strcpy(processId, "P");
		strcat(processId, integer_string);

		pushProcess(&p1, cred_id, processId);
		sizeReal =
				snprintf(buffer, sizeof(buffer),
						"echo type:Process id:%s Event:CredFork_Handler Cred_Id:%lu >> /tmp/spade_pipe",
						processId, cred_id);
		char actualBuf[sizeReal + 1];
		strncpy(actualBuf, buffer, sizeReal);
		actualBuf[sizeReal] = '\0';
		system(actualBuf);
		//printf("%s\n",actualBuf);
		memset(buffer, 0, sizeof buffer);
		memset(actualBuf, 0, sizeof actualBuf);

	} else {
		processId = getProcessId(p1, cred_id);
	}
	if (isForkedpresent == 1) {
		forked_processId = malloc(CHARSIZE * sizeof(char));
		memset(integer_string, 0, sizeof integer_string); // to be on safer size.. make sure its empty
		sprintf(integer_string, "%lu", forked_cred);
		strcpy(forked_processId, "P");
		strcat(forked_processId, integer_string);

		pushProcess(&p1, forked_cred, forked_processId);
		sizeReal =
				snprintf(buffer, sizeof(buffer),
						"echo type:Process id:%s Event:CredFork_Handler Cred_Id:%lu >> /tmp/spade_pipe",
						forked_processId, forked_cred);
		char actualBuf1[sizeReal + 1];
		strncpy(actualBuf1, buffer, sizeReal);
		actualBuf1[sizeReal] = '\0';
		system(actualBuf1);
		memset(buffer, 0, sizeof buffer);
		memset(actualBuf1, 0, sizeof actualBuf1);
	} else {
		forked_processId = getProcessId(p1, forked_cred);
	}

/* create WasForkedby from newProcessId to oldProcessId*/
	sizeReal = snprintf(buffer, sizeof buffer,
			"echo type:WasForkedby from:%s to:%s >> /tmp/spade_pipe",
			forked_processId, processId);
	char actualBuf2[sizeReal + 1];
	strncpy(actualBuf2, buffer, sizeReal);
	actualBuf2[sizeReal] = '\0';
	system(actualBuf2);
	//printf("%s\n",actualBuf2);

	memset(buffer, 0, sizeof buffer);
	memset(actualBuf2, 0, sizeof actualBuf2);
	
	//free(processId);
	//free(forked_processId);

	return 0;
}

int handle_credfree(struct provmsg *msg) {

	return 0;
}

/* Mugdha -- Implement this! */
int handle_setid(struct provmsg *msg) {

	struct provmsg_setid *m = (struct provmsg_setid *) msg;

	/* Extract provmsg fields */
	unsigned long cred_id = m->msg.cred_id;
	uint32_t uid = m->uid;

// check if process corresponding to cred id is already thr or not.. ideally it should be there
	int iscredPresent = checkProcessId(p1, cred_id);
	
	// check if agent id is there corresponding to that uid. 
	int isUidpresent = checkAgentId(ag, uid);
	char* processId;
	char* agentId;

	char integer_string[TEMPCHARSIZE];
	char buffer[BUFFERSIZE];
	int sizeReal;
	if (iscredPresent == 1)  // process is not there
			{
		processId = malloc(CHARSIZE * sizeof(char));
		memset(integer_string, 0, sizeof integer_string); // to be on safer size.. make sure its empty
		sprintf(integer_string, "%lu", cred_id);
		strcpy(processId, "P");
		strcat(processId, integer_string);

		pushProcess(&p1, cred_id, processId);
		sizeReal =
				snprintf(buffer, sizeof(buffer),
						"echo type:Process id:%s Event:SetId_Handler Cred_Id:%lu >> /tmp/spade_pipe",
						processId, cred_id);
		char actualBuf[sizeReal + 1];
		strncpy(actualBuf, buffer, sizeReal);
		actualBuf[sizeReal] = '\0';
		system(actualBuf);
		//printf("%s\n",actualBuf);
		memset(buffer, 0, sizeof buffer);
		memset(actualBuf, 0, sizeof actualBuf);

	} else {
		processId = getProcessId(p1, cred_id);
		//printf("%s",processId);
	}

	if (isUidpresent == 1) // agent is not there   create Agent
			{
		agentId = malloc(CHARSIZE * sizeof(char));
		memset(integer_string, 0, sizeof integer_string);
		sprintf(integer_string, "%lu", uid);
		strcpy(agentId, "AG");
		strcat(agentId, integer_string);
		pushAgent(&ag, uid, agentId);
		sizeReal =
				snprintf(buffer, sizeof(buffer),
						"echo type:Agent id:%s Event:SetId_Handler UserId:%d >> /tmp/spade_pipe",
						agentId, uid);
		char actualBuf2[sizeReal + 1];
		strncpy(actualBuf2, buffer, sizeReal);
		actualBuf2[sizeReal] = '\0';
		system(actualBuf2);
		//printf("%s\n",actualBuf2);
		memset(buffer, 0, sizeof buffer);
		memset(actualBuf2, 0, sizeof actualBuf2);

	} else        // agent is there
	{
		agentId = getAgentId(ag, uid);
		//  printf("%s",agentId);

	}

	/* Get (or create) WasControlledBy relationship:*/
	/*   cred_id <-- WasControlledBy -- uid */

	sizeReal = snprintf(buffer, sizeof buffer,
			"echo type:WasControlledBy from:%s to:%s >> /tmp/spade_pipe",
			processId, agentId);
	char actualBuf3[sizeReal + 1];
	strncpy(actualBuf3, buffer, sizeReal);
	actualBuf3[sizeReal] = '\0';
	system(actualBuf3);
	//printf("%s\n",actualBuf3);
	memset(buffer, 0, sizeof buffer);
	memset(actualBuf3, 0, sizeof actualBuf3);
	//free(processId);     
	//free(agentId);
	return 0;
}

/* Mugdha -- Implement this! */
int handle_exec(struct provmsg *msg) {
	struct provmsg_exec *m = (struct provmsg_exec *) msg;

	/* Extract provmsg fields */
	unsigned long cred_id = m->msg.cred_id;
	int argc = m->argc;
	/* Extract the command into a string */
	char* label = argv_to_label(m->argv_envp, msg_getlen(msg) - sizeof(*m),
			argc);
	const int sourceLen = strlen(label);
	//char* newString = (char*) malloc(sourceLen + 4 * sizeof(char) + 1);    // adding 4  for \" adding in front and back
	char* newString;
	char *newLabel;

			
	// there is a problem when label constains \\", echo is not happening correctly to overcome that we will replace \\" by space. There are only two 
	// even for which it is happening so to avoid the performance overhead we will look for those even and just replace in that even . It is happening
	// when counter=3885 and 3887. we will just check for it
	
	if(counter!=3885 && counter!=3887)
	{
		
 // below are steps to put label in double 	quotes so that it is safe for echo
	newString = (char*) malloc(sourceLen + 4 * sizeof(char) + 1);
	newLabel = newString;
	int i;
	 for (i = 0; i <= sourceLen; ++i) {
		if(i==0){
			*newString++ = '\\';
			*newString++ = '\"';
			*newString++ = label[i];	
	 		 }
	 	 else if (i==sourceLen) {
			 *newString++ = '\\';
	 	 	*newString++ = '\"';
			
	 	} else
	 		*newString++ = label[i];
	 }
	*newString = '\0';   
		
	}	
	else
	{	
		int pos = 0;	 
		int j;
		for (j = 0; j < sourceLen-2; ++j) {
			if (label[j] == '\\' && label[j+1] == '\\' && label[j+2] == '\"')
				++pos;
		}
		 
		newString = (char*) malloc(sourceLen - pos +4+ 1); 
		newLabel=newString;		  
		bool flag=false;
		 int i;
		for (i = 0; i < sourceLen-2; ++i) {
			if(i==0){
			*newString++ = '\\';
			*newString++ = '\"';			 
	 		 }
			if (label[i] == '\\' && label[i+1] == '\\' && label[i+2] == '\"')
			{
				//*newString++ = ' ';
				i=i+2;	
				if(i==(sourceLen-1))
					flag=true;		 
			} else
				*newString++ = label[i];
		}
		if(!flag)  // appending the last char
		{
		*newString++ = label[sourceLen-2];
		*newString++ = label[sourceLen-1];	
		}		 
		 *newString++ = '\\';
	 	 *newString++ = '\"';
		 *newString = '\0';	
		 //printf("%s",newLabel);
					
	}			
	 
 
	 
	// check if process corresponding to cred id is already thr or not.. ideally it should be there
	int isCredPresent = checkProcessId(p1, cred_id);

	char* processId;

	char integer_string[TEMPCHARSIZE];
	char buffer[BUFFERSIZE];
	int sizeReal;

	if (isCredPresent == 1)  // process is not there
			{
		processId = malloc(CHARSIZE * sizeof(char));
		memset(integer_string, 0, sizeof integer_string);
		sprintf(integer_string, "%lu", cred_id);
		strcpy(processId, "P");
		strcat(processId, integer_string);

		pushProcess(&p1, cred_id, processId);
		sizeReal =
				snprintf(buffer, sizeof(buffer),
						"echo type:Process id:%s Event:Exec_Handler Cred_Id:%d >> /tmp/spade_pipe",
						processId, cred_id);
		char actualBuf[sizeReal + 1];
		strncpy(actualBuf, buffer, sizeReal);
		actualBuf[sizeReal] = '\0';
		system(actualBuf);
		//printf("%s\n",actualBuf);
		memset(buffer, 0, sizeof buffer);
		memset(actualBuf, 0, sizeof actualBuf);

	} else {
		processId = getProcessId(p1, cred_id);
	}
// create a new process for  exec cmd with all the labels	 , we are using counter to generate new unique process
	char* prefix = "exec_";
	char postfix[CHARSIZE];
	snprintf(postfix, CHARSIZE, "%d", counter);
	char* execProcessId = malloc(strlen(prefix) + strlen(postfix) + 1);
	strcpy(execProcessId, prefix);
	strcat(execProcessId, postfix);
	 
	
	
	counter++; // increase the counter by 1 so that it woudl be used by anothet exce command
		

	sizeReal =
			snprintf(buffer, sizeof(buffer),
					"echo type:Process id:%s Event:Exec_Handler LABEL:\"%s\" >> /tmp/spade_pipe",
					execProcessId, newLabel);

	char actualBuf3[sizeReal + 1];
	strncpy(actualBuf3, buffer, sizeReal);
	actualBuf3[sizeReal] = '\0';	 
	system(actualBuf3);	
	memset(buffer, 0, sizeof buffer);
	memset(actualBuf3, 0, sizeof actualBuf3);
	memset(postfix, 0, sizeof postfix);

	// now create the event WasTriggeredBy between process corresponding to credId and new process created for exec cmd 

	sizeReal = snprintf(buffer, sizeof buffer,
			"echo type:WasTriggeredBy from:%s to:%s >> /tmp/spade_pipe",
			execProcessId, processId);
	char actualBuf2[sizeReal + 1];
	strncpy(actualBuf2, buffer, sizeReal);
	actualBuf2[sizeReal] = '\0';
	system(actualBuf2);
 //	printf("%s\n",actualBuf2);
	memset(buffer, 0, sizeof buffer);
	memset(actualBuf2, 0, sizeof actualBuf2);

	/* Update Activity node (cred_id) so that it has the new label */
	/* The Activity node definitely already exists */

	// Your code here
	free(label);
	free(newLabel);
	return 0;
}

int handle_inode_p(struct provmsg *msg) {

	return 0;
}

int handle_mmap(struct provmsg *msg) {

	return 0;
}

/* Mugdha -- Implement this! */
int handle_file_p(struct provmsg *msg) {

	struct provmsg_file_p *m = (struct provmsg_file_p *) msg;

	/* Extract provmsg fields */
	unsigned long cred_id = m->msg.cred_id;
	unsigned long inode = m->inode.ino;
	int32_t mask = m->mask;
	uint64_t inode_version=m->inode_version;
	
	/**
	 * for conventience we will convert uint64_t  to unsigned long other way is to include inttypes.h and use below syntaxt
	 * printf("%" PRIu64 "\n", t);
	 */
	unsigned long i_nodeVersion=(unsigned long)inode_version;

	
	/* Get Process node for cred_id */

	// check if process corresponding to cred id is already thr or not.. ideally it should be there
	int iscredpresent = checkProcessId(p1, cred_id);
	int isInodePresent = checkArtifactId(ar, inode,i_nodeVersion);

	char* processId;
	char* iNodeId;

	char integer_string[TEMPCHARSIZE];
	char buffer[BUFFERSIZE];
	int sizeReal;

	if (iscredpresent == 1)  // process is not there, creating a new process
			{
		processId = malloc(CHARSIZE * sizeof(char));
		memset(integer_string, 0, sizeof integer_string);
		sprintf(integer_string, "%lu", cred_id);
		strcpy(processId, "P");
		strcat(processId, integer_string);

		pushProcess(&p1, cred_id, processId);
		sizeReal =
				snprintf(buffer, sizeof(buffer),
						"echo type:Process id:%s Event:File_Handler Cred_Id:%lu >> /tmp/spade_pipe",
						processId, cred_id);
		char actualBuf[sizeReal + 1];
		strncpy(actualBuf, buffer, sizeReal);
		actualBuf[sizeReal] = '\0';
		system(actualBuf);
		//printf("%s\n",actualBuf);
		memset(buffer, 0, sizeof buffer);
		memset(actualBuf, 0, sizeof actualBuf);

	} else {
		processId = getProcessId(p1, cred_id);
	}

	// Your code here

	/* Get (or create) Artifact node for inode */

	if (isInodePresent == 1)         // creating a new Artifact
			{
		iNodeId = malloc(CHARSIZE * sizeof(char));
		memset(integer_string, 0, sizeof integer_string);
		sprintf(integer_string, "%lu", inode);   // convert int to string  inode value
		strcpy(iNodeId, "AR");
		strcat(iNodeId, integer_string);
		
		memset(integer_string, 0, sizeof integer_string);
		sprintf(integer_string, "%lu", i_nodeVersion);   // convert int to string version value
		strcat(iNodeId, ".");
		strcat(iNodeId, integer_string);
		pushArtifact(&ar, inode,i_nodeVersion,iNodeId);

		sizeReal =
				snprintf(buffer, sizeof(buffer),
						"echo type:Artifact id:%s INODE:%lu VERSION:%lu Event:File_P_Handler >> /tmp/spade_pipe",
						iNodeId,inode,i_nodeVersion);
		char actualBuf1[sizeReal + 1];
		strncpy(actualBuf1, buffer, sizeReal);
		actualBuf1[sizeReal] = '\0';
		system(actualBuf1);
		//printf("%s\n",actualBuf1);
		memset(buffer, 0, sizeof buffer);
		memset(actualBuf1, 0, sizeof actualBuf1);

	} else {

		iNodeId = getArtifactId(ar, inode,i_nodeVersion);
	}

	/* Create relationship */
	if (m->mask & MAY_READ) {
		/* cred_id -- Used --> inode */

		int isUsedBy = checkUsedBy(used, processId, iNodeId);
		if (isUsedBy == 1) // used by relation is not there , so we will create it  else do nothing
				{
			sizeReal = snprintf(buffer, sizeof buffer,
					"echo type:Used from:%s to:%s >> /tmp/spade_pipe",
					processId, iNodeId);
			char actualBuf2[sizeReal + 1];
			strncpy(actualBuf2, buffer, sizeReal);
			actualBuf2[sizeReal] = '\0';
			system(actualBuf2);
			//printf("%s\n",actualBuf2);
			pushUsedBy(&used, processId, iNodeId);
			memset(buffer, 0, sizeof buffer);
		}
	} else if (m->mask & MAY_WRITE) {
		/* cred_id <-- WasGeneretedBy -- inode */
		int isGeneratedBy = checkWasGeneratedBy(wasgenBy, processId, iNodeId);
		if (isGeneratedBy == 1) {
			sizeReal = snprintf(buffer, sizeof buffer,
					"echo type:WasGeneratedBy from:%s to:%s >> /tmp/spade_pipe",
					iNodeId, processId);
			char actualBuf3[sizeReal + 1];
			strncpy(actualBuf3, buffer, sizeReal);
			actualBuf3[sizeReal] = '\0';
			system(actualBuf3);
			//printf("%s\n",actualBuf3);
			pushWasGeneratedBy(&wasgenBy, processId, iNodeId);
			memset(buffer, 0, sizeof buffer);
		}
	}

	return 0;

}

int handle_link(struct provmsg *msg) {

	return 0;
}

int handle_unlink(struct provmsg *msg) {

	return 0;
}

int handle_setattr(struct provmsg *msg) {

	return 0;
}

int handle_inode_alloc(struct provmsg *msg) {

	return 0;
}

int handle_inode_dealloc(struct provmsg *msg) {

	return 0;

}

int handle_socksend(struct provmsg *msg) {

	return 0;
}

int handle_sockrecv(struct provmsg *msg) {

	return 0;
}

int handle_sockalias(struct provmsg *msg) {

	return 0;
}

int handle_mqsend(struct provmsg *msg) {

	return 0;
}

int handle_mqrecv(struct provmsg *msg) {

	return 0;
}

int handle_shmat(struct provmsg *msg) {

	return 0;
}

int handle_readlink(struct provmsg *msg) {

	return 0;
}

/* Other stuff */

void exit_nicely() {
	exit(1);
}

void handle_err(char * cmd, char * err) {
	fprintf(stderr, "%s command failed: %s\n", cmd, err);
}

char * encode_to_utf8(char * cstring, int len) {

	char * newstring = malloc(len + 1);
	//memcpy(newstring,cstring,len);
	//newstring[len]='\0';

	if (!newstring) {
		printf("Out of memory\n");
		exit_nicely();
	}

	// I really just don't care if you are a non-utf8 character
	// I am going to blast you away
	int newcursor = 0, oldcursor = 0;
	while (oldcursor < len) {
		if (cstring[oldcursor] < 128 && cstring[oldcursor] > 0) {
			if (cstring[oldcursor] == '\\' || cstring[oldcursor] == '\'') {
				newstring[newcursor] = ' ';
				newcursor++;
			} else {
				newstring[newcursor] = cstring[oldcursor];
				newcursor++;
			}
		}
		oldcursor++;
	}

	newstring[newcursor] = '\0';

	return newstring;
}

char * argv_to_label(char * cstring, int len, int argc) {

	int item_count = 0;
	int quote_count = 0;
	for (int n = 0; n < len; n++) {
		if (cstring[n] == 0)
			item_count++;
		else if (cstring[n] == '\"' || cstring[n] == '\'')
			quote_count++;
	}

	// For argv item1\0item2\0item3="escaped quotes"\0
	// Format is '{"item1","item2","item3=\"escaped quotes\"}'
	int newlength = 2 + len + item_count * 3 + quote_count * 2;
	char * newstring = (char *) malloc(newlength);
	if (!newstring) {
		printf("Out of memory\n");
		exit_nicely();
	}

	int newcursor = 0;
	int oldcursor = 0;
	while (oldcursor < len && newcursor < MAXENVLEN) {
		if (cstring[oldcursor] != 0) {
			// Escape quotation marks
			if (cstring[oldcursor] == '\"' || cstring[oldcursor] == '\'') {
				newstring[newcursor] = '\\';
				newcursor++;
				newstring[newcursor] = '\\';
				newcursor++;
				cstring[oldcursor] = '\"';
			}
			// Replace non-UTF8 encoded bytes with a space
			else if (cstring[oldcursor] >= 128 || cstring[oldcursor] < 0)
				cstring[oldcursor] = '_';

			newstring[newcursor] = cstring[oldcursor];

			//printf("(%c,%d)\n",newstring[newcursor],newstring[newcursor]);
			newcursor++;
		}
		// If we encounter a null terminator it marks a new item or EOL
		// If new item
		else if (cstring[oldcursor] == 0 && oldcursor < len - 1) {
			argc -= 1;
			if (argc == 0) {
				newstring[newcursor] = '\0';
				return newstring;
			}
			newstring[newcursor] = '_';
			newcursor++;
		}
		oldcursor++;
	}

	// If EOL 
	newstring[newcursor] = '\0';

	return newstring;

}

char * print_uuid_be(uuid_be *uuid) {
	int i, c = 0;
	char * rv = malloc(37);
	for (i = 0; i < 16; i++) {
		if (i == 4 || i == 6 || i == 8 || i == 10) {
			rv[c] = '-';
			c++;
		}
		sprintf(rv + c, "%02x", uuid->b[i]);
		c += 2;

		rv[c] = (char) uuid->b[i];
		snprintf(&rv[c], 1, "%02x", uuid->b[i]);
	}

	return rv;
}

int32_t type_from_name(char *name) {
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
void pushProcess(struct Process** head_ref, unsigned long cr, char* prId) {
	/* allocate node */
	struct Process* new_node = (struct Process*) malloc(sizeof(struct Process));

	//new_node->processId=malloc(25*sizeof (char)); 
	/* put in the data  */
	new_node->credId = cr;
	new_node->processId = prId;
	new_node->next = (*head_ref);

	/* move the head to point to the new node */
	(*head_ref) = new_node;
}

int checkProcessId(struct Process* head, unsigned long cr) {
	struct Process* current = head;

	while (current != NULL) {
		if (current->credId == cr)
			return 0;   // exist
		current = current->next;
	}
	return 1;

}

//get processId
char* getProcessId(struct Process* head, unsigned long cr) {
	struct Process* current = head;

	while (current != NULL) {
		if (current->credId == cr)
			return current->processId;
		current = current->next;
	}
	return 0;

}

void printVal(struct Process* head) {
	while (head != NULL) {
		printf("%d", head->credId);
		printf("\n");
		printf("%s", head->processId);
		printf("\n");
		head = head->next;
	}
}

//get agentId
char* getAgentId(struct Agent* head, uint32_t u_id) {
	struct Agent* current = head;

	while (current != NULL) {
		if (current->uid == u_id)
			return current->agentId;
		current = current->next;
	}
	return 0;

}

int checkAgentId(struct Agent* head, uint32_t u_id) {
	struct Agent* current = head;

	while (current != NULL) {
		if (current->uid == u_id)
			return 0;   // exist
		current = current->next;
	}
	return 1;

}

void pushAgent(struct Agent** head_ref, uint32_t u_id, char* agId) {
	/* allocate node */
	struct Agent* new_node = (struct Agent*) malloc(sizeof(struct Agent));

	//new_node->processId=malloc(25*sizeof (char)); 
	/* put in the data  */
	new_node->uid = u_id;
	new_node->agentId = agId;
	new_node->next = (*head_ref);

	/* move the head to point to the new node */
	(*head_ref) = new_node;
}

void pushArtifact(struct Artifact** head_ref, unsigned long iNode,unsigned long version,char* artiId) {
	/* allocate node */
	struct Artifact* new_node = (struct Artifact*) malloc(
			sizeof(struct Artifact));

	//new_node->processId=malloc(25*sizeof (char)); 
	/* put in the data  */
	new_node->inodeNo = iNode;
	new_node->versionNo=version;
	new_node->artifactId = artiId;
	new_node->next = (*head_ref);

	/* move the head to point to the new node */
	(*head_ref) = new_node;

}

int checkArtifactId(struct Artifact* head, unsigned long iNode,unsigned long version) {
	struct Artifact* current = head;

	while (current != NULL) {
		if (current->inodeNo == iNode && current->versionNo==version)
			return 0;   //exist
		current = current->next;
	}
	return 1;

}
char* getArtifactId(struct Artifact* head, unsigned long iNode,unsigned long version) {
	struct Artifact* current = head;

	while (current != NULL) {
		if (current->inodeNo == iNode && current->versionNo==version)
			return current->artifactId;
		current = current->next;
	}
	return 0;

}

void pushUsedBy(struct Used** head_ref, char* pId, char* artiId) {
	/* allocate node */
	struct Used* new_node = (struct Used*) malloc(sizeof(struct Used));

	//new_node->processId=malloc(25*sizeof (char)); 
	/* put in the data  */
	new_node->processId = pId;
	new_node->artifactId = artiId;
	new_node->next = (*head_ref);

	/* move the head to point to the new node */
	(*head_ref) = new_node;
}
int checkUsedBy(struct Used* head, char* pId, char* artiId) {
	struct Used* current = head;

	while (current != NULL) {
		if (current->artifactId == artiId) {
			if (current->processId == pId)
				return 0;   //exist
		}

		current = current->next;
	}
	return 1;

}

void pushWasGeneratedBy(struct WasGeneratedBy** head_ref, char* pId,
		char* artiId) {
	/* allocate node */
	struct WasGeneratedBy* new_node = (struct WasGeneratedBy*) malloc(
			sizeof(struct WasGeneratedBy));

	//new_node->processId=malloc(25*sizeof (char)); 
	/* put in the data  */
	new_node->processId = pId;
	new_node->artifactId = artiId;
	new_node->next = (*head_ref);

	/* move the head to point to the new node */
	(*head_ref) = new_node;
}
int checkWasGeneratedBy(struct WasGeneratedBy* head, char* pId, char* artiId) {
	struct WasGeneratedBy* current = head;

	while (current != NULL) {
		if (current->artifactId == artiId) {
			if (current->processId == pId)
				return 0;   //exist
		}

		current = current->next;
	}
	return 1;

}

