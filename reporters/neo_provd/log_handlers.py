def handle_PROVMSG_CREDFORK(provid,record):
    components = record.split(' ')
    credfork = str(components[1])
    return credfork

def handle_PROVMSG_EXEC(provid,record):
    return str_provid(provid) + ' ' + str_uid(provid) + " " + record

def handle_PROVMSG_FILE_P(provid,record):
    retval = ''
    components = record.split(' ')

    access_type = components[1]
    uuid = components[2].split(':')[0]
    inode = components[2].split(':')[1]
    
    filepath = fullpath(uuid,inode,'')

    #if filepath == '':
    #    print "Bates: No filepath for ",inode

    retval = str_provid(provid) + ' ' + str_uid(provid) + " fperm " + access_type + ' ' + uuid + ' ' + inode + ':' + filepath
    return retval

def handle_PROVMSG_SETID(provid,record):

    global uid,gid,suid,sgid,euid,egid,fsuid,fsgid

    m = re.match(r"setid r(\d+):(\d+), s(\d+):(\d+), e(\d+):(\d+), fs(\d+):(\d+)",row)        
    
    uid = str(m.group(1))
    gid = str(m.group(2))
    suid = str(m.group(3))
    sgid = str(m.group(4))
    euid = str(m.group(5))
    egid = str(m.group(6))
    fsuid = str(m.group(7))
    fsgid = str(m.group(8))

    creds[provid] = { 'uid':uid,
                      'gid':gid,
                      'suid':suid,
                      'sgid':sgid,
                      'euid':euid,
                      'egid':egid,
                      'fsuid':fsuid,
                      'fsgid':fsgid }

    retval = (str_provid(provid) + ' ' + str_uid(provid) + ' setid r' + uid + ':' + gid + ', s' + suid + ':' + sgid 
              + ', e' + euid + ':' + egid + ', fs' + fsuid + ':' + fsgid)

    return retval

def handle(provid,record):

    if record.startswith("credfork"):
        return handle_PROVMSG_CREDFORK(provid,record)
    elif record.startswith("setid"):
        return handle_PROVMSG_SETID(provid,record)
    elif record.startswith("exec"):
        return handle_PROVMSG_EXEC(provid,record)
    elif record.startswith("fperm"):
        return handle_PROVMSG_FILE_P(provid,record)
    elif(record.startswith("link")
         or record.startswith("root_inode")):
        return handle_PROVMSG_LINK(provid,record)

        '''
	if record.startswith("boot"):
		return handle_PROVMSG_BOOT(provid,record)
	elif record.startswith("credfree"):
		return handle_PROVMSG_CREDFREE(provid,record)
	elif record.startswith("mmap"):
		return handle_PROVMSG_MMAP(provid,record)
	elif record.startswith("iperm"):
		return handle_PROVMSG_INODE_P(provid,record)
	elif record.startswith("ialloc"):
		return handle_PROVMSG_INODE_ALLOC(provid,record)
	elif record.startswith("idealloc"):
		return handle_PROVMSG_INODE_DEALLOC(provid,record)
	elif record.startswith("setattr"):
		return handle_PROVMSG_SETATTR(provid,record)
	elif record.startswith("unlink"):
		return handle_PROVMSG_UNLINK(provid,record)
	elif record.startswith("mqsend"):
		return handle_PROVMSG_MQSEND(provid,record)
	elif record.startswith("mqrecv"):
		return handle_PROVMSG_MQRECV(provid,record)
	elif record.startswith("shmat"):
		return handle_PROVMSG_SHMAT(provid,record)
	elif record.startswith("readlink"):
		return handle_PROVMSG_READLINK(provid,record)
	elif record.startswith("socksend"):
		return handle_PROVMSG_SOCKSEND(provid,record)
	elif record.startswith("sockrecv"):
		return handle_PROVMSG_SOCKRECV(provid,record)
	elif record.startswith("sockalias"):
		return handle_PROVMSG_SOCKALIAS(provid,record)
	else:
            return '' 
            '''
