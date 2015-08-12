#!/bin/python

from dot_handlers import *

#Although these edges will ultimately go FILE <-> PROCESS,
# For the time being they go FILE <-> CRED.				      
def get_creds_by_inode(tablename,inode,masks,cursor):

    #Find references to target inode
    if len(masks) == 0:
        cmd = "SELECT eventid FROM %s WHERE inode=%s" % (tablename,inode,)
    else:
        mask = ','.join(masks)
        cmd = "SELECT eventid FROM %s WHERE inode=%s AND mask IN(%s)" % (tablename,inode,mask)

    cursor.execute(cmd)
    results = cursor.fetchall()

    eventids = []
    for row in results:
        eventid = row[0]
        eventids.append(eventid)

    #Find references to target cred        
    creds = []
    for eventid in eventids:
        cmd = ("SELECT provid FROM main WHERE eventid=%lu limit 1;" % (eventid))
        cursor.execute(cmd)
        new_cred = cursor.fetchall()[0][0]

	if new_cred not in creds:
            with scan_lock:
                if new_cred not in scanned_creds:
                    creds.append(new_cred)

    return creds

def get_agent_label(a,cursor):

	cred = provid = a

	while True:
		cmd = "SELECT provid FROM credfork WHERE newid=%lu limit 1" % (provid)
                cursor.execute(cmd)
                result = cursor.fetchall()

                try:
                    provid = result[0][0]
                except:
                    print ("ERROR -- result len = %d for cmd %s" % (len(result),cmd))
                    return

                cmd = "SELECT eventid FROM main WHERE provid=%lu AND event_type=%d order by eventid desc limit 1" % (provid,PROVMSG.SETID)
                cursor.execute(cmd)
                result = cursor.fetchall()

		if len(result) > 0:
			eventid = result[0][0]

			cmd = "SELECT uid FROM setid WHERE eventid=%lu limit 1" % (eventid)
                        cursor.execute(cmd)
                        record = cursor.fetchall()[0]

			uid = record[0]		
			return uid

		#We are back to the original credential.
		#Return original cred
		if provid == 0:
			print "Failed to find uid for %d" %(cred)
			return cred


def handle_setid(cred,cursor):       
    global node_num_to_uid,uid_to_node_num,node_num_to_cred,cred_to_node_num

    #######################################################
    #               HANDLE SETID
    ######################################################
    #debug("Handling SETID Message for %s\n" % (str(cred)))
    cmd = "SELECT setid.uid FROM main  INNER JOIN setid ON (main.eventid = setid.eventid) WHERE provid=%lu order by main.eventid desc" % (cred)
    cursor.execute(cmd)
    result = cursor.fetchall()

    #debug("Searching for uid for cred %d... " % (int(cred)))

    #If this condition fails then there was no SETID event
    if len(result) > 0:
	    uid = str(result[0][0])
    #So recover it from cred's predacessors
    else:
	    uid = str(get_agent_label(cred,cursor))

    #debug("it is %s.\n" % (uid))

    if not uid in uid_to_node_num:
        #debug("\t%s not in uid_to_node_num, creating new node... " % uid)
        agent_node_num = add_agent_node(uid,cred)
        node_num_to_uid[agent_node_num] = uid
        uid_to_node_num[uid] = agent_node_num
        node_num_to_cred[agent_node_num] = cred
    else:
        #debug("\t%s already in uid_to_node_num (len %d), looking up... " % (uid,len(uid_to_node_num)))
        agent_node_num = uid_to_node_num[uid]

    cred_to_node_num[cred] = agent_node_num

    #debug("returning node num %d\n" % agent_node_num)
    return agent_node_num


def handle_exec(cred,agent_node_num,cursor):

    #######################################################
    #               HANDLE EXEC
    ######################################################
    proc_node_num = -1
    #debug("Handling EXEC Message for %d\n" % (cred))
    cmd = "SELECT exec.inode, exec.argc, exec.argv, main.eventid FROM main INNER JOIN exec ON (main.eventid = exec.eventid) WHERE provid=%lu order by main.eventid" % (cred)
    cursor.execute(cmd)
    result = cursor.fetchall()

    if len(result) > 0:
	    exec_label = ""
	    for record in result:
		    #Includes cmdline args
		    #cmd = " ".join(record[2][:record[1]]
		    #Doesn't include cmdline args				   
                try:
		    cmd = record[2][0]
                except:
                    print "ERROR -- for cmd %s record is %s" % (cmd,str(record))
                    return

                if exec_label != "":
                    exec_label += "\n"
                exec_label += cmd

	    proc_node_num = add_process_node(cred,exec_label)
	    add_process_edges(proc_node_num,agent_node_num)
    
    return proc_node_num

def handle_file_p(cred, agent_node_num, proc_node_num,cursor):
    #######################################################
    #               HANDLE FILE_P
    ######################################################
    files_read = []
    files_written = []

    #debug("Handling FILE_P Message for %d\n" % (cred))
    cmd = "SELECT file_p.inode,file_p.mask,file_p.eventid FROM main INNER JOIN file_p ON (main.eventid = file_p.eventid) WHERE provid=%lu order by main.eventid" % (cred)
    cursor.execute(cmd)
    result = cursor.fetchall()

    if len(result) > 0:
	    for record in result:
		    (inode,mask,eventid) = record

		    if((testBit(mask,1) == MAY.WRITE
			or testBit(mask,3) == MAY.APPEND)
		       and inode not in files_written):
			    files_written.append(inode)
		      
		    if(testBit(mask,2) == MAY.READ
		       and inode not in files_read):
			    files_read.append(inode)
		      
                    #Add file node if necessary
		    if options.dag:
			    inode_node_num = add_file_node(str(eventid) + ":" + str(inode),inode)		    
		    else:
			    inode_node_num = add_file_node(str(inode),inode)		    
		 
		    #Add edge from file to agent and/or process
		    if proc_node_num < 0:
			    add_file_edges(inode_node_num,agent_node_num,mask)
		    else:
			    add_file_edges(inode_node_num,proc_node_num,mask)

    return (files_read,files_written)

def handle_sockrecv(cred, proc_node_num, agent_node_num,cursor):
    #######################################################
    #               HANDLE SOCKRECV
    ######################################################
    #debug("Handling SOCKRECV Message for %d\n" % (cred))
    cmd = "SELECT address,port FROM main INNER JOIN sockrecv ON (main.eventid = sockrecv.eventid) WHERE provid=%lu AND family=%d order by main.eventid" % (cred,socket.AF_INET)
    cursor.execute(cmd)
    result = cursor.fetchall()

    if len(result) > 0:
        for record in result:
            (address,port) = record
            sock_node_num = add_sock_node(address,address)

            if proc_node_num < 0:
                add_sock_edges(sock_node_num,agent_node_num)
            else:
                add_sock_edges(sock_node_num,proc_node_num)

    return len(result)

def get_random_inode(cursor):

    cursor.execute("SELECT count(eventid) FROM file_p;")
    num_eventids = cursor.fetchall()[0][0]

    random_position = randrange(num_eventids)
    
    cursor.execute("SELECT inode FROM file_p LIMIT 1 OFFSET %d;" % (random_position))
    random_inode = cursor.fetchall()[0][0]

    return random_inode


