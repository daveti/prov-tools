from config import *

#AGENT_LOCK contols agent_nodes, cred_to_node_num, node_num_to_cred, uid_to_node_num, node_num_to_uid
agent_lock = threading.RLock()
agent_nodes = {}
cred_to_node_num = {}
node_num_to_cred = {}
uid_to_node_num = {}
node_num_to_uid = {}

#FILE_LOCK controls file_nodes, inode_to_node_num, and node_num_to_inode
file_lock = threading.RLock()
file_nodes = {}
inode_to_node_num = {}
node_num_to_inode = {}

#PROCESS_LOCK controls process_ndoes, node_num_to_process_id, and process_id_to_node_num
process_lock = threading.RLock()
process_nodes = {}
node_num_to_process_id = {}
process_id_to_node_num = {}

#EDGE_LOCK controlls edge_num_to_edge, edge_to_edge_num, dst_to_edge_nums, and src_to_edge_nums
edge_lock = threading.RLock()
edge_num_to_edge = {}
edge_to_edge_num = {}
dst_to_edge_nums = {}
src_to_edge_nums = {}

#NUM_LOCK controls node_num and edge_num
num_lock = threading.RLock()
node_num = 0
edge_num = 0

# Lists that manage the growth and termination of the graph
#SCAN_LOCK controls scanned_creds and scanned_inodes
scan_lock = threading.RLock()
scanned_creds = []
scanned_inodes = []

def add_agent_node(label,provid):

    global node_num,agent_nodes,cred_to_node_num,node_num_to_cred

    with agent_lock:
        if provid not in agent_nodes:
            with num_lock:
                node_assignment = node_num
                node_num += 1

            agent_nodes[provid] = {'label':label,
                                   'edges':0,
                                   'num':node_assignment,
                                   'fontsize':AGENT_FONTSIZE,
                                   'fontcolor':AGENT_FONTCOLOR,
                                   'color':AGENT_COLOR,
                                   'shape':AGENT_SHAPE,
                                   'type':'cred',
                                   'cred':provid}

	    node_num_to_cred[node_assignment] = provid
	    cred_to_node_num[provid] = node_assignment

        return agent_nodes[provid]['num']

def add_sock_node(label,ip):
    global node_num,file_nodes,inode_to_node_num,node_num_to_inode
    
    with file_lock:
        if ip not in file_nodes:
            with num_lock:
                node_assignment = node_num
                node_num += 1

            file_nodes[ip] = {'label':label,
                              'edges':0,
                              'num':node_assignment,
                              'fontsize':SOCK_FONTSIZE,
                              'fontcolor':SOCK_FONTCOLOR,
                              'color':SOCK_COLOR,
                              'shape':SOCK_SHAPE,
                              'type':'sock',
                              'inode':ip}
            inode_to_node_num[ip] = node_assignment
            node_num_to_inode[node_assignment] = ip
    
        return file_nodes[ip]['num']	    

def add_file_node(label,inode):

    global node_num,file_nodes,inode_to_node_num,node_num_to_inode

    with file_lock:
        if inode not in file_nodes:
            with num_lock:
                node_assignment = node_num
                node_num += 1

            file_nodes[inode] = {'label':label,
                                 'edges':0,
                                 'num':node_assignment,
                                 'fontsize':FILE_FONTSIZE,
                                 'fontcolor':FILE_FONTCOLOR,
                                 'color':FILE_COLOR,
                                 'shape':FILE_SHAPE,
                                 'type':'inode',
                                 'inode':inode}
            inode_to_node_num[inode] = node_assignment
            node_num_to_inode[node_assignment] = inode

        return file_nodes[inode]['num']

def add_process_node(provid,label):

    global node_num,process_nodes,inode_to_node_num,node_num_to_inode

    with process_lock:
        if not provid in process_nodes:
            with num_lock:
                node_assignment = node_num
                node_num += 1

            process_nodes[provid] =  {'label':label + " - " + str(provid),
                                      'edges':0,
                                      'num':node_assignment,
                                      'fontsize':PROCESS_FONTSIZE,
                                      'fontcolor':PROCESS_FONTCOLOR,
                                      'color':PROCESS_COLOR,
                                      'shape':PROCESS_SHAPE,
                                      'type':'proc',
                                      'cred':provid}
				          
	    process_id_to_node_num[provid] = node_assignment
	    node_num_to_process_id[node_assignment] = provid
    
        return process_nodes[provid]['num']

def add_sock_edges(sock_node_num,other_node_num):

	global agent_nodes,file_nodes,edge_num,edge_num_to_edge,src_to_edge_nums,dst_to_edge_nums


        with agent_lock:
            with file_lock:
                with process_lock:
                    with edge_lock:
                        the_ip = node_num_to_inode[sock_node_num]
            
                        if not (other_node_num,sock_node_num) in edge_to_edge_num:                

                            with num_lock:
                                edge_assignment = edge_num
                                edge_num += 1

                            new_edge = { 'src': other_node_num,
                                         'dst': sock_node_num,
                                         'label' : 'Messaged',
                                         'num': edge_assignment,
                                         'fontsize': DEFAULT_FONTSIZE,
                                         'fontcolor': DEFAULT_FONTCOLOR,
                                         'color': 'orange'}
                            
                            edge_num_to_edge[edge_assignment] = new_edge
                            edge_to_edge_num[(other_node_num,sock_node_num)] = edge_assignment
			
                            if sock_node_num in dst_to_edge_nums:
                                dst_to_edge_nums[sock_node_num].append(edge_assignment)
                            else:
                                dst_to_edge_nums[sock_node_num] = [edge_assignment]
                                
                            if other_node_num in src_to_edge_nums:
                                src_to_edge_nums[other_node_num].append(edge_assignment)
                            else:
                                src_to_edge_nums[other_node_num] = [edge_assignment]

                            #Increase Edge Count        
                            file_nodes[the_ip]['edges'] += 1
                            if other_node_num in node_num_to_cred:
                                agent_nodes[node_num_to_cred[other_node_num]]['edges'] += 1
                            else:
                                process_nodes[node_num_to_process_id[other_node_num]]['edges'] += 1

def add_file_edges(file_node_num,other_node_num,mask):
	
	global agent_nodes,file_nodes,edge_num,edge_num_to_edge,src_to_edge_nums,dst_to_edge_nums

	#Determine edge label by inspecting masks
	if not isinstance(mask,int):
		return

        with agent_lock:
            with file_lock:
                with process_lock:
                    with edge_lock:
                        the_inode = node_num_to_inode[file_node_num]

                        if(testBit(mask,1) == MAY.WRITE
                           or testBit(mask,3) == MAY.APPEND):

                            if not (file_node_num,other_node_num) in edge_to_edge_num:

                                with num_lock:
                                    edge_assignment = edge_num
                                    edge_num += 1

                                new_edge = { 'src': file_node_num,
                                             'dst': other_node_num,
                                             'label' : 'WasWrittenBy',#'WasGeneratedBy',
                                             'num': edge_assignment,
                                             'fontsize': DEFAULT_FONTSIZE,
                                             'fontcolor': DEFAULT_FONTCOLOR,
                                             'color': 'red'}

                                edge_num_to_edge[edge_assignment] = new_edge
                                edge_to_edge_num[(file_node_num,other_node_num)] = edge_assignment

                                if file_node_num in src_to_edge_nums:
                                    src_to_edge_nums[file_node_num].append(edge_assignment)
                                else:
                                    src_to_edge_nums[file_node_num] = [edge_assignment]

                                if other_node_num in dst_to_edge_nums:
                                    dst_to_edge_nums[other_node_num].append(edge_assignment)
                                else:
                                    dst_to_edge_nums[other_node_num] = [edge_assignment]

                                #Increase Edge Count        
                                file_nodes[node_num_to_inode[file_node_num]]['edges'] += 1
                                if other_node_num in node_num_to_cred:
                                    agent_nodes[node_num_to_cred[other_node_num]]['edges'] += 1
                                else:
                                    process_nodes[node_num_to_process_id[other_node_num]]['edges'] += 1
			
                        if(testBit(mask,2) == MAY.READ):
                            if not (other_node_num,file_node_num) in edge_to_edge_num:

                                with num_lock:
                                    edge_assignment = edge_num
                                    edge_num += 1

                                new_edge = { 'src': other_node_num,
                                             'dst': file_node_num,
                                             'label' : 'Read',#'Used',
                                             'num': edge_assignment,
                                             'fontsize': DEFAULT_FONTSIZE,
                                             'fontcolor': DEFAULT_FONTCOLOR,
                                             'color': 'green'}
                                
                                edge_num_to_edge[edge_assignment] = new_edge
                                edge_to_edge_num[(other_node_num,file_node_num)] = edge_assignment
                    
                                if file_node_num in dst_to_edge_nums:
                                    dst_to_edge_nums[file_node_num].append(edge_assignment)
                                else:
                                    dst_to_edge_nums[file_node_num] = [edge_assignment]
                        
                                if other_node_num in src_to_edge_nums:
                                    src_to_edge_nums[other_node_num].append(edge_assignment)
                                else:
                                    src_to_edge_nums[other_node_num] = [edge_assignment]
                                    
                                #Increase Edge Count        
                                file_nodes[the_inode]['edges'] += 1
                                if other_node_num in node_num_to_cred:
                                    agent_nodes[node_num_to_cred[other_node_num]]['edges'] += 1
                                else:
                                    process_nodes[node_num_to_process_id[other_node_num]]['edges'] += 1

                        if(testBit(mask,1) != MAY.WRITE
                           and testBit(mask,3) != MAY.APPEND
                           and  testBit(mask,2) != MAY.READ):
                            print "Lost relation: mask=%d" % (mask)

def add_process_edges(process_node_num,agent_node_num):

	global edge_num, edge_num_to_edge, edge_to_edge_num, process_nodes, agent_nodes

        with agent_lock:
            with process_lock:
                with edge_lock:
                    if not (process_node_num,agent_node_num) in edge_to_edge_num:

                        with num_lock:
                            edge_assignment = edge_num
                            edge_num += 1
                            
                        new_edge = { 'src': process_node_num,
                                     'dst': agent_node_num,
                                     'label' : 'WasControlledBy',
                                     'num': edge_assignment,
                                     'fontsize': DEFAULT_FONTSIZE,
                                     'fontcolor': DEFAULT_FONTCOLOR,
                                     'color': 'purple'}
                        
                        edge_num_to_edge[edge_assignment] = new_edge
                        edge_to_edge_num[(process_node_num,agent_node_num)] = edge_assignment
                        
                        if process_node_num in src_to_edge_nums:
                            src_to_edge_nums[process_node_num].append(edge_assignment)
                        else:
                            src_to_edge_nums[process_node_num] = [edge_assignment]
                    
                        if agent_node_num in dst_to_edge_nums:
                            dst_to_edge_nums[agent_node_num].append(edge_assignment)
                        else:
                            dst_to_edge_nums[agent_node_num] = [edge_assignment]	
                            
		        #Increase edge count
                        process_nodes[node_num_to_process_id[process_node_num]]['edges'] += 1
                        agent_nodes[node_num_to_cred[agent_node_num]]['edges'] += 1


def harvest_agent_edges():
	global edge_num_to_edge, agent_nodes 

        with agent_lock:
            with edge_lock:
                for a in agent_nodes.values():
                    curr_cred = a['cred']
                    curr_node_num = cred_to_node_num[curr_cred]

                    if curr_node_num not in node_num_to_uid:
                        uid = a['label']
                        if not uid in uid_to_node_num:
                            print "Skipping",uid
                            continue

                        true_node_num = uid_to_node_num[a['label']]
                        true_cred = node_num_to_cred[true_node_num]
                        true_agent = agent_nodes[true_cred]
			
                        if curr_node_num in dst_to_edge_nums:
                            edges = dst_to_edge_nums[curr_node_num] 
                            del dst_to_edge_nums[curr_node_num]

                            for edge_num in edges:				
                                edge_num_to_edge[edge_num]['dst'] = true_node_num

                                agent_nodes[curr_cred]['edges'] -= 1
                                agent_nodes[true_cred]['edges'] += 1
                                
                            if true_node_num in dst_to_edge_nums:
                                dst_to_edge_nums[true_node_num] += edges
                            else:
                                dst_to_edge_nums[true_node_num] = edges
                                
                        if curr_node_num in src_to_edge_nums:
                            edges = src_to_edge_nums[curr_node_num] 
                            del src_to_edge_nums[curr_node_num]

                            for edge_num in edges:				
                                edge_num_to_edge[edge_num]['src'] = true_node_num
                                agent_nodes[curr_cred]['edges'] -= 1
                                agent_nodes[true_cred]['edges'] += 1
                                
                            if true_node_num in src_to_edge_nums:
                                src_to_edge_nums[true_node_num] += edges
                            else:
                                src_to_edge_nums[true_node_num] = edges
                                        

def harvest_process_edges():
	global edge_num_to_edge, process_nodes, agent_nodes, dst_to_edge_nums, src_to_edge_nums


        with agent_lock:
            with process_lock:
                with edge_lock:
                    for p in process_nodes.keys():
                        process = process_nodes[p]
                        process_node_num = process_id_to_node_num[p]

                        cred = process['cred']
                        cred_node_num = cred_to_node_num[cred]
                        agent = agent_nodes[cred]

                        if cred_node_num in dst_to_edge_nums:
                            edges = dst_to_edge_nums[cred_node_num]
                            new_dst_list = []
                            for edge_num in edges:				
                                if not edge_num_to_edge[edge_num]['label'] == "WasControlledBy":
                                    edge_num_to_edge[edge_num]['dst'] = process_node_num
                                    agent_nodes[cred]['edges'] -= 1
                                    process_nodes[p]['edges'] += 1
                                    if process_node_num in dst_to_edge_nums:
                                        dst_to_edge_nums[process_node_num].append(edge_num)
                                    else:
                                        dst_to_edge_nums[process_node_num] = [edge_num]
                                else:
                                    new_dst_list.append(edge_num)
                            dst_to_edge_nums[cred_node_num] = new_dst_list

                        if cred_node_num in src_to_edge_nums:
                            edges = src_to_edge_nums[cred_node_num]
                            new_src_list = []
                            for edge_num in edges:				
                                if not edge_num_to_edge[edge_num]['label'] == "WasControlledBy":
                                    edge_num_to_edge[edge_num]['src'] = process_node_num
                                    agent_nodes[cred]['edges'] -= 1
                                    process_nodes[p]['edges'] += 1
                                    if process_node_num in src_to_edge_nums:
                                        src_to_edge_nums[process_node_num].append(edge_num)
                                    else:
                                        src_to_edge_nums[process_node_num] = [edge_num]
                                else:
                                    new_src_list.append(edge_num)
                            src_to_edge_nums[cred_node_num] = new_src_list
	
			
def label_inodes():
	global file_nodes

	with file_lock:
            for inode in file_nodes.keys():
		if options.dag:
                    eventid = file_nodes[inode]['label'].split(":")[0]
		else:
                    eventid=""

		label = ""
		curr_inode = inode		
		while curr_inode in link_map:
                    (inode_dir,fname) = [link_map[curr_inode]['inode_dir'], link_map[curr_inode]['fname']]
			
                    if fname == "root_inode":
                        label = "/" + label
                        break
                    elif label == "":
                        label = fname
                    else:
                        label = fname + "/" + label

                    curr_inode = inode_dir

                if label != "" and not options.dag:
                    file_nodes[inode]['label'] = label
		elif label != "":
                    file_nodes[inode]['label'] = eventid + ":" + label
		#debug("\tInode %d is now labeled %s" % (inode,file_nodes[inode]['label']))
			       



def out_to_dot():

    f = open(options.filename,'w')

    f.write("digraph test {\n")
    f.write("   graph[size=\"11,8.5\",dpi=900,center=\"true\",nodesep=\"0.5\",ranksep=\"1\"]\n")
    f.write("   margin=\"0\"\n")
    f.write("   center=true\n")
    f.write("   pagedir=\"TL\"\n")
    f.write("   bgcolor=\"%s\"" % (PAGE_COLOR))
    f.write("   orientation=\"portrait\"\n")

    f.write("\n\n")

    with agent_lock:
        with file_lock:
            with process_lock:
                with num_lock:
                    with edge_lock:
        
                        for n in range(0,node_num):
                            node = {}
                            if n in node_num_to_inode :#and file_nodes[node_num_to_inode[n]]['edges'] > 0:
                                node = file_nodes[node_num_to_inode[n]]
                            elif n in node_num_to_process_id:#  and process_nodes[node_num_to_process_id[n]]['edges'] > 0:
                                node = process_nodes[node_num_to_process_id[n]]
                            elif n in node_num_to_cred:# and agent_nodes[node_num_to_cred[n]]['edges'] > 0:
                                node = agent_nodes[node_num_to_cred[n]]
                            else:
                                continue
            
                            f.write("   node%d[label=\"%s\",shape=%s,style=filled,fillcolor=\"%s\",fontcolor=%s,fontsize=\"%d\"];\n" 
                                    % (node['num'], 
                                       node['label'],
                                       node['shape'],
                                       node['color'],
                                       node['fontcolor'],
                                       node['fontsize']))
                            
                        for e in range(0,edge_num):
                            edge = edge_num_to_edge[e]
                            
                            f.write("   node%d -> node%d [ label=\"%s\",fontsize=\"%d\",color=\"%s\",weight=1,penwidth=4,arrowsize=4,fontcolor=\"%s\"];\n " % (
                                    edge['src'],
                                    edge['dst'],
                                    edge['label'],
                                    edge['fontsize'],
                                    edge['color'],
                                    edge['fontcolor']))
                            

    f.write("\n}\n")
    f.close()
