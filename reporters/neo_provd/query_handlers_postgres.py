from config import *

def get_boots(cred,cursor):

    return_value = {}

    #Only relevant to first cred
    if cred != '0':
        return return_value

    
    cmd = "SELECT main.provid,boot.uuid_be FROM main INNER JOIN boot ON (main.eventid = boot.eventid) WHERE main.provid=%s order by main.eventid" % (str(cred))
    cursor.execute(cmd)
    result = cursor.fetchall()

    for row in result:
        (provid,uuid_be) = row 
        return_value[provid] = {'provid':provid,
                                'uuid_be':uuid_be}

    return return_value


def get_credforks(cred,cursor):

    return_value = {}

    cmd = "SELECT credfork.newid,credfork.provid FROM main INNER JOIN credfork ON (main.eventid = credfork.eventid) WHERE main.provid=%s order by main.eventid" % (str(cred))
    cursor.execute(cmd)
    result = cursor.fetchall()

    for row in result:
        (newid,provid) = row
        return_value[newid] = {'provid':newid,
                               'parentid':provid}

    return return_value

def get_execs(cred,cursor):

    return_value = {}

    cmd = "SELECT main.eventid,exec.uuid_be,exec.inode,exec.argc,exec.argv FROM main INNER JOIN exec ON (main.eventid = exec.eventid) WHERE provid=%s order by main.eventid" % (str(cred))
    cursor.execute(cmd)
    result = cursor.fetchall()

    for row in result:
        (eventid,uuid,inode,argc,argv) = row
        return_value[eventid] = {'uuid':str(uuid),
                                 'inode':str(inode),
                                 'argc':argc,
                                 'argv':argv}
    
    return return_value

def get_setids(a,cursor):       

	cred = provid = a
        return_value = {}

	while True:
            cmd = "SELECT main.eventid,setid.uid,setid.euid,setid.gid,setid.egid,setid.suid,setid.fsuid,setid.sgid,setid.fsgid FROM main INNER JOIN setid ON (main.eventid = setid.eventid) WHERE provid=%s ORDER BY main.eventid DESC" % (str(provid))
            cursor.execute(cmd)
            result = cursor.fetchall()

            #Only entered if we found the setid event
            for row in result:
                (eventid,uid,euid,gid,egid,suid,fsuid,sgid,fsgid) = row
                
                return_value[eventid] = {'uid':str(uid),
                                         'euid':str(euid),
                                         'gid':str(gid),
                                         'egid':str(egid),
                                         'suid':str(suid),
                                         'fsuid':str(fsuid),
                                         'sgid':str(sgid),
                                         'fsgid':str(fsgid)}
                
                return return_value


            cmd = "SELECT provid FROM credfork WHERE newid=%s limit 1" % (provid)
            cursor.execute(cmd)
            result = cursor.fetchall()

            if len(result) > 0:
                provid = result[0][0]    
            else:
                #We are back to the original credential.
                #Return original cred
                return_value[eventid] = {'uid':str(0),
                                         'euid':str(0),
                                         'gid':str(0),
                                         'egid':str(0),
                                         'suid':str(0),
                                         'fsuid':str(0),
                                         'sgid':str(0),
                                         'fsgid':str(0)}
                break

        return return_value

def get_files(cred,cursor):

    (return_value_reads,return_value_writes) = ({},{})

    '''
    I think that my cypher queries are never returning due to cycles within an individual cred
    where a process writes a file and then reads it *afterwards*:

       -- written by ->
    F                    P
       <-     read   --

    This inevitably creates a cycle because the same inode/version is being listed as both an
    input and an output to the process. This is only a problem within this 2 node cycle scenario, 
    because the versioning takes care of it between creds. So I added this "included_inode-
    _versions" thing down here...

      Case 1 -- If we add an inode/version to the queue for WRITE edge creation, and it shows up 
      for READ as well, we will add a previous version of that inode to the queue instead
      (The first version that we find that we didn't also write in this cred)

      Case 2 -- If we version<0, we will not add the inode as an input.

    This is a conservative solution that is sure to capture the output's dependencies.
    '''

    included_inode_versions = {}

    cmd = "SELECT main.eventid,file_p.uuid_be,file_p.inode,file_p.version,file_p.mask FROM main INNER JOIN file_p ON (main.eventid = file_p.eventid) WHERE provid=%s and mask IN (%d,%d) order by main.eventid" % (str(cred), MAY.WRITE, MAY.APPEND)
    cursor.execute(cmd)
    result = cursor.fetchall()

    #In the first pass we search for WRITE/APPEND and ignore READ
    for row in result:
	    (eventid,uuid,inode,version,mask) = row

	    if(testBit(mask,1) == MAY.WRITE
               or testBit(mask,3) == MAY.APPEND):
		    return_value_writes[(inode,version)] = {'uuid':str(uuid),
                                                            'inode':str(inode),
                                                            'version':version,
                                                            'mask':mask,
                                                            'uuid_be':uuid}
                    #Prevent a 2 node cycle (read/write) with this dict
                    included_inode_versions[(inode,version)] = True

    cmd = "SELECT main.eventid,file_p.uuid_be,file_p.inode,file_p.version,file_p.mask FROM main INNER JOIN file_p ON (main.eventid = file_p.eventid) WHERE provid=%s and mask IN (%d) order by main.eventid" % (str(cred), MAY.READ)
    cursor.execute(cmd)
    result = cursor.fetchall()

    #In the second pass we search for READ and ignore WRITE/APPEND
    for row in result:
	    (eventid,uuid,inode,version,mask) = row
    
	    if testBit(mask,2) == MAY.READ:
                    #Prevent a 2 node cycle (read/write) with this dict

                    while (inode,version) in included_inode_versions and version > 0:
                        version -= 1

		    return_value_reads[(inode,version)] = {'uuid':str(uuid),
                                                           'inode':str(inode),
                                                           'version':version,
                                                           'mask':mask,
                                                           'uuid_be':uuid}

		    
    return (return_value_reads,return_value_writes)

def get_network(cred,cursor):

    (return_value_recvs, return_value_sends) = ({},{})

    # SOCK RECVS
    cmd = "SELECT main.eventid,low_sockid,high_sockid,address,port,protocol FROM main INNER JOIN sockrecv ON (main.eventid = sockrecv.eventid) WHERE provid=%s AND sockrecv.family=%d order by main.eventid" % (str(cred), socket.AF_INET)
    cursor.execute(cmd)
    result = cursor.fetchall()

    for record in result:
        (eventid,low_sockid,high_sockid,address,port,protocol) = record



        if not address or len(address) > 1:
            continue
            #(For handling unix sockets)
            #address = str(high_sockid) + ":" + str(low_sockid)
            #port = 0
        else:
            return_value_recvs[eventid] = {'address':address,
                                           'port':port,
                                           'protocol':protocol}
            
    # SOCK SENDS
    #cmd = "SELECT main.eventid,low_sockid,high_sockid,address,port,protocol FROM main INNER JOIN socksend ON (main.eventid = socksend.eventid) WHERE provid=%s AND family=%d order by main.eventid" % (str(cred),socket.AF_INET)
    cmd = "SELECT main.eventid,low_sockid,high_sockid,address,port,protocol FROM main INNER JOIN socksend ON (main.eventid = socksend.eventid) WHERE provid=%s and socksend.family=%d order by main.eventid" % (str(cred), socket.AF_INET)
    cursor.execute(cmd)
    result = cursor.fetchall()

    for record in result:
        (eventid,low_sockid,high_sockid,address,port,protocol) = record

        if not address or len(address) <= 1:
            continue
            #address = str(high_sockid) + ":" + str(low_sockid)
            #port = 0
        else:
            return_value_recvs[eventid] = {'address':address,
                                           'port':port,
                                           'protocol':protocol}        

    return (return_value_recvs,return_value_sends)


def get_or_create_node( node_type, node_key, node_name ):

    global included_inodes, included_users, included_procs

    if node_type == 'inode':
        if node_key not in included_inodes:
            included_inodes[node_key], = graph_db.create(node(name=node_name))
            included_inodes[node_key].add_labels("File")
        return included_inodes[node_key]
    elif node_type == 'user':
        if node_key not in included_users:
            included_users[node_key], = graph_db.create(node(name=node_name))
            included_users[node_key].add_labels("Agent")
        return included_users[node_key]
    elif node_type == 'proc':
        if node_key not in included_procs:
            included_procs[node_key], = graph_db.create(node(name=node_name))
            included_procs[node_key].add_labels("Process")
        return included_procs[node_key]
    elif node_type == 'network':
        if node_key not in included_network:
            included_network[node_key], = graph_db.create(node(name=node_name))
            included_network[node_key].add_labels("Network")
        return included_network[node_key]




