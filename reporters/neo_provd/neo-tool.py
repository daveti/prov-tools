from query_handlers_postgres import *

def meanstdv(x): 
    n, mean, std = len(x), 0, 0 
    for a in x: 
        mean = mean + a 
    mean = mean / float(n) 
    for a in x: 
        std = std + (a - mean)**2 
    if n > 1:
        std = sqrt(std / float(n-1)) 
    else:
        std = 0
    return mean, std 

def scan_pg_cred(cred,target_inode,target_version):
    global graph_db

    bench_start(cred)
    events_processed = 0

    #Peek at the execs. 
    execs = get_execs(cred,cursor)        

    # If there isn't an exec, this is a kernel activity,
    # and we are going to quit.
    if len(execs) == 0:
        bench_end(cred)
        return

    ############################################
    #          ACTIVITIES
    # 1) Handle Boot Records
    # 2) Handle Credfork Records
    # 3) Label Activity Nodes with Exec Records
    ############################################
    activities = graph_db.get_or_create_index(neo4j.Node,'Activity')

    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    # 1) Handle Boot Records
    boots = get_boots(cred,cursor)
    events_processed += len(boots)

    current_activity = False
    for b in boots:
        (provid,uuid_be) = (boots[b]['provid'],boots[b]['uuid_be'])
        
        current_activity = activities.get_or_create('Activity',
                                                    provid,
                                                    {'name':provid,
                                                     'provid':provid,
                                                     'uuid_be':uuid_be})
        
    #If not created by a boot record, get/create the current activity
    if not current_activity:
        current_activity = activities.get_or_create('Activity',
                                                    cred,{})

    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    # 2) Handle Credfork Records
    credforks = get_credforks(cred,cursor)
    events_processed += len(credforks)
    
    for c in credforks:
        (provid,parentid) = (credforks[c]['provid'],credforks[c]['parentid'])

        # Get or create new node
        new_activity = activities.get_or_create('Activity',
                                                provid,
                                                {'name':provid,
                                                 'parentid':parentid})
        new_activity.add_labels("Activity")        
        new_activity.get_or_create_path( "WasForkedFrom", current_activity)

    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    # 3) Label Activity Nodes with Exec Records        
    # (We pulled these ahead of time)
    events_processed += len(execs)

    cmd = ""
    for e in execs:
        argc = int(execs[e]['argc'])

        if cmd != "":
            cmd += '\n'
        cmd += ' '.join(execs[e]['argv'][:argc])

    current_activity.set_properties({'name':cmd,
                                     'provid':cred})


    ############################################
    #            AGENTS
    # 4) Handle setid Records
    ############################################
    agents = graph_db.get_or_create_index(neo4j.Node,'Agent')

    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    # 4) Handle setid Records
    setids =  get_setids(cred,cursor)
    events_processed += len(setids)

    uid = ""
    for key in setids:
        uid = setids[key]['euid']
        break
    current_agent = agents.get_or_create('Agent',
                                         uid,
                                         {'name':'u'+uid})
    current_agent.add_labels("Agent")    
    current_activity.get_or_create_path( "WasAssociatedWith", current_agent )

    ############################################
    #            ENTITIES
    # 5) Handle file_p records
    ############################################
    entity_files = graph_db.get_or_create_index(neo4j.Node,'File')

    #~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    # 4) Handle file_p records
    (inputs,outputs) = get_files(cred,cursor)
    events_processed += len(inputs) + len(outputs)

    (input_nodes,output_nodes) = ([],[])
    for i in inputs:		
        (inode,version,uuid_be) = (inputs[i]['inode'],inputs[i]['version'],inputs[i]['uuid_be'])
        label = label_inode(int(inode))

        input_node = entity_files.get_or_create('File',
                                                "%s:%s" % (inode,version),
                                                {'name':label,
                                                 'inode':inode,
                                                 'version':version,
                                                 'uuid_be':0})
        input_node.add_labels("File")
        input_nodes.append(input_node)
        current_activity.get_or_create_path("Used", input_node)

    for i in outputs:
        inode = outputs[i]['inode']
        version = outputs[i]['version']

        if(options.inode 
           and inode  != target_inode
           and version != target_version):
            continue

        label = label_inode(int(inode))

        output_node = entity_files.get_or_create('File',
                                                 "%s:%s" % (inode,version),
                                                 {'name':label,
                                                  'inode':inode,
                                                  'version':version})
        output_node.add_labels("File")
        output_node.get_or_create_path("WasGeneratedBy", current_activity)

        #Enable for the "WasDerivedFrom" Relation
        #This creates A LOT of new edges!
        if True:
            for input_node in input_nodes:
                output_node.get_or_create_path("WasDerivedFrom", input_node)

        
        '''
    if 0:
        (net_inputs,net_outputs) = get_network(cred,cursor)
        net_index = graph_db.get_or_create_index(neo4j.Node,'Network')

        for i in net_inputs:
            label = net_inputs[i]['address']

            packet = file_index.get_or_create('Network',label,{})
            packet.set_properties({'name':label,})
            packet.add_labels("Network")
            proc.get_or_create_path("ReceivedFrom", packet)
        
        for i in net_outputs:
            label = net_outputs[i]['address']

            packet = file_index.get_or_create('Network',label,{})
            packet.set_properties({'name':label,})
            packet.add_labels("Network")
            proc.get_or_create_path("SendTo", packet)
            '''

    debug("%4d evt\t" % events_processed)
    bench_end(cred)

def scan_inode(inode,version):

    global inodes_in_graph, included_inodes

    inodes_in_graph["%s:%s"%(inode,version)] = (inode,version)

    while len(inodes_in_graph) > 0:

        print len(inodes_in_graph)

        key,(inode,version) = inodes_in_graph.popitem()
        print "(%s,%s)"%(inode,version)
        if "%s:%s"%(inode,version) in included_inodes:
            continue
        else:
            cmd = ("SELECT main.provid FROM file_p "
                   + "INNER JOIN main on (file_p.eventid = main.eventid) "
                   + "WHERE file_p.inode=%s AND file_p.version=%s AND file_p.mask=%d"
                   % (inode,version,MAY.WRITE))
            
            cursor.execute(cmd)
            result = cursor.fetchall()
            included_inodes["%s:%s"%(inode,version)] = True
            
            for record in result:
                provid = record[0]
                scan_pg_cred(provid,inode,version)
            
def scan_all_pg_creds():

    global cursor

    cmd = "SELECT DISTINCT provid FROM main WHERE provid >= %s ORDER BY provid;" % (options.all)
    cursor.execute(cmd)
    result = cursor.fetchall()

    (elapsed_time,creds_scanned) = (0.,0.)
    for record in result:

        cred = str(record[0])

        debug("Cred %s...\t" % (cred))

        scan_pg_cred(cred,False,False)

        creds_scanned += 1
        result = bench_get(cred)
        duration = result['elapsed']
        elapsed_time += duration 
        debug("%f sec \tAVG=%f sec\n" % (duration,float(elapsed_time/creds_scanned)))

    debug("Cred scanning finished (%f sec)\n" % (elapsed_time))

def scan_all_gz_creds():

    raw_input = open(options.logfile,'r')

    for row in raw_input:
        row=row.rstrip()

        m = re.match(r"\[([^\[\]]*)\]\s(.*)",row)

        if not m:
            print "Failed to match",row                                                                 
            continue

        print "%s (%s)" % (m.group(1),row)

        provid = m.group(1)
        row = m.group(2)

def benchmark():
    
    targets = []
    source_file = open('../../logs/sort-unique-inodes.txt','r')
    for row in source_file:
        row = row.rstrip()
        (inode,version) = row.split(",")
        targets.append((inode,version))
    source_file.close()

    query = neo4j.CypherQuery(graph_db, "")
    times = []
    max_duration = .0
    min_duration = 10000
    (countdown,i) = (1000,0)
    node_counts = []
    while i < countdown:

        inode,version = targets[randrange(0,len(targets))]

        debug("%s:%s\t" % (inode,version))
        
        bench_start('b'+str(i))
        
        try:
            query = neo4j.CypherQuery(graph_db, "START a=node:File(File='%s:%s') MATCH path=a-[r:WasDerivedFrom*]->b RETURN b;" % (inode,version))

            node_count = 0
            for record in query.stream():
                node_count += 1
                continue
                '''
                print "\n\n"
                print "New Record of length %d" % len(record)
                for i in range(0,len(record)):
                print "\t%s" % str(record[i])
                print "\n\n"
                '''

            # BENCHMARKING STUFF
            elapsed = bench_end('b'+str(i))                
            
            if node_count == 0:
                continue

            times.append(elapsed)
            node_counts.append(node_count)

            average, stdev = meanstdv(times)
            node_avg, node_stdev = meanstdv(node_counts)

            if elapsed > max_duration:
                max_duration = elapsed
            
            if elapsed < min_duration:
                min_duration = elapsed

            #Results for this query
            debug("%0.3fs/$0.3dcnt\t" %
                  (elapsed,
                   node_count))
            
            #Total Results Thus Far

            #  Average time elapsed
            debug("AVG=%0.3fs [%0.3f,%0.3f]\t" %
                  (average,
                   average - 2 * stdev,
                   average + 2 * stdev))

            #  Min/Max time elapsed
            debug("MIN=%0.3f MAX=%0.3f " % 
                  (min_duration,
                   max_duration))

            #  Average number of nodes reeturned
            debug("CNT=%3d [%3.0f,%3.0f]" % 
                  (node_avg,
                   node_avg - 2 * node_stdev,
                   node_avg + 2 * node_stdev))
                  
            i += 1
        except:
            bench_end('b'+str(i))                
            raise




def main():

    if options.bench:
        benchmark()
    elif options.postgres:
        if options.all:
            scan_all_pg_creds()
        elif options.cred:
            scan_pg_cred(options.cred,False,False)
        elif options.inode:
            scan_inode(options.inode,options.version)

if __name__ == "__main__":
    main()

