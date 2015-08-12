#!/bin/python

from query_handlers import *

found_network = False

def _scan_cred(cred):
    global scanned_inodes

    #Postgres will throw an error if too many connections are open, so wait
    conn = False
    while not conn:
        try:
            conn = psycopg2.connect(conn_string)
        except:
            continue

    cursor = conn.cursor(cursor_factory=psycopg2.extras.DictCursor)

    agent_node_num = handle_setid(cred,cursor)
    proc_node_num = handle_exec(cred,agent_node_num,cursor)
    (files_read,files_written) = handle_file_p(cred,agent_node_num,proc_node_num,cursor)
    sock_result = handle_sockrecv(cred,agent_node_num,proc_node_num,cursor)

    # Crawl graph from files read
    process_inodes = []
    for inode in files_read:
        to_scan = False
        with scan_lock:
            to_scan = inode not in scanned_inodes

        if to_scan:
            process_inodes.append(inode)

    creds = []
    for inode in process_inodes:
        with scan_lock:
            scanned_inodes.append(inode)

        new_creds = get_creds_by_inode("file_p",inode,[str(MAY.WRITE),
                                                       str(MAY.APPEND),
                                                       str(MAY.WRITE+MAY.READ),
                                                       str(MAY.WRITE+MAY.READ+MAY.APPEND)],cursor)        
        for new_cred in new_creds:
            if new_cred not in creds:
                creds.append(new_cred)

    #Close connection because we are about to fork
    cursor.close()
    conn.close()

    return creds

######################################################
#     MULTITHREAD CONFIGURATION
######################################################
NUM_THREADS = 20
pool = Pool(processes=NUM_THREADS)

def scan_cred(cred):

    global scanned_creds,scanned_inodes

    with scan_lock:
        scanned_creds.append(cred)

    new_creds = _scan_cred(cred)


    threads = []
    while len(new_creds) > 0:
        new_cred = new_creds.pop()

        to_scan = False
        with scan_lock:
            to_scan = new_cred not in scanned_creds

        if to_scan:
            with scan_lock:
                scanned_creds.append(new_cred)
            threads.append(pool.apply_async(_scan_cred,[new_cred]))

        if len(threads) == NUM_THREADS:
            for t in range(0,NUM_THREADS):
                thread = threads.pop(0)
                result = thread.get(timeout=100)
                new_creds += result
        
def clear_all():

	global agent_nodes,cred_to_node_num,node_num_to_cred,uid_to_node_num,node_num_to_uid,file_nodes,inode_to_node_num,node_num_to_inode,process_nodes,node_num_to_process_id,process_id_to_node_num,edge_num_to_edge,edge_to_edge_num,dst_to_edge_nums,src_to_edge_nums,node_num,edge_num,scanned_creds,scanned_inodes
	
        
	agent_nodes = {}
	cred_to_node_num = {}
	node_num_to_cred = {}
	uid_to_node_num = {}
	node_num_to_uid = {}
	
	file_nodes = {}
	inode_to_node_num = {}
	node_num_to_inode = {}
	
	process_nodes = {}
	node_num_to_process_id = {}
	process_id_to_node_num = {}
	
	edge_num_to_edge = {}
	edge_to_edge_num = {}
	dst_to_edge_nums = {}
	src_to_edge_nums = {}
	node_num = 0
	edge_num = 0
	
	scanned_creds = []
	scanned_inodes = []

def query(creds):
    debug("\tScanning nodes... ");
    bench_start('scan_nodes')
    for cred in creds:
        scan_cred(cred)
    debug("finished (%f sec)\n" % (bench_end('scan_nodes')))

    debug("\tLabeling inodes... ");
    bench_start('label_inodes')
    label_inodes()
    debug(" finished (%f sec)\n" % (bench_end('label_inodes')))

    #debug("Harvesting Agent Edges\n");
    #harvest_agent_edges()
    #debug("Harvesting Process Edges\n");
    #harvest_process_edges()
    debug("\tWriting to output file... ");
    bench_start('write_out')
    if options.inode:
	    target_inode = int(options.inode)
	    file_nodes[target_inode]['fontsize'] *= 2
	    file_nodes[target_inode]['color'] = '#00FF00'
    elif options.exec_inode:
            target_provids = creds
	    for provid in target_provids:
		    process_nodes[provid]['fontsize'] *= 4
		    process_nodes[provid]['color'] = '#00FF00'

    out_to_dot()
    debug("finished (%f sec)\n" % (bench_end('write_out')))

def main():

    global scanned_inodes

    conn = psycopg2.connect(conn_string)
    cursor = conn.cursor(cursor_factory=psycopg2.extras.DictCursor)

    creds = []    
    ######################################################
    #     FIND STARTING NODES FOR PROVENANCE GRAPH
    ######################################################
    if options.inode:
        creds = get_creds_by_inode("file_p",options.inode,[],cursor)
        scanned_inodes.append(options.inode)
        query(creds)
    elif options.exec_inode:
        creds = get_creds_by_inode("exec",options.exec_inode,[],cursor)
        scanned_inodes.append(options.exec_inode)
        query(creds)
    elif options.bench:
        sum_time = 0.0
        iterations = int(options.bench)
        for i in range(0,iterations):
            clear_all()
            debug("\n\n\tSelecting new random inode...")
            bench_start('rand_'+str(i))
            random_inode = get_random_inode(cursor)
            debug("%f seconds\n\n" % (bench_end('rand_'+str(i))))

            debug("Iteration %d...\n" % i)
            bench_start(str(i))

            debug("\tPulling creds for new inode... ")
            bench_start('pull_'+str(i))
            creds = get_creds_by_inode("file_p",random_inode,[],cursor)
            scanned_inodes.append(random_inode)
            debug(" finished (%f sec)\n" % (bench_end('pull_'+str(i))))

            query(creds)            
            print "Iteration %d COMPLETED in %f seconds" % (i,bench_end(str(i)))
            print "%d agents, %d files, %d processes" % (len(agent_nodes),len(file_nodes),len(process_nodes))
            sum_time += float(bench_end(str(i)))

        average = sum_time / float(iterations)
        debug("\n\n")

        print "~~~~~~~~~BENCHMARKING COMPLETE~~~~~~~~~"
        print "Average graph construction time = %f" % average
        print "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"

    else:
	    parser.print_usage()
	    sys.exit(0)

if __name__ == "__main__":
    main()





