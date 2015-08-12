from math import sqrt 
from optparse import OptionParser,OptionGroup
import pickle
import psycopg2
import psycopg2.extras
from py2neo import *
import pprint
from random import randrange
import re
import socket
import sys
import time
import sys

#ENUMERATIONS
class MAY():
	EXEC = 1
	WRITE = 2
	READ = 4
	APPEND = 8
	ACCESS = 16
	OPEN = 32

class PROVMSG():
	BOOT = 0
	CREDFORK = 1
	CREDFREE = 2
	SETID = 3
	EXEC = 4
	FILE_P = 5
	MMAP = 6
	INODE_P = 7
	INODE_ALLOC = 8
	INODE_DEALLOC = 9
	SETATTR = 10
	LINK = 11
	UNLINK = 12
	MQSEND = 13
	MQRECV = 14
	SHMAT = 15
	READLINK = 16
	SOCKSEND = 17
	SOCKRECV = 18
	SOCKALIAS = 19
	NUM_TYPES = 20

def debug(msg):
	if options.verbose:
		sys.stdout.write(msg)
		sys.stdout.flush()

def testBit(int_type, offset):
        mask = 1 << offset
	return(int_type & mask)

benchmarks = {}
def bench_start(key):
	global benchmarks
	if key not in benchmarks:
		benchmarks[key] = {}
	benchmarks[key]['start'] = time.time()
	return benchmarks[key]

def bench_end(key):
	global benchmarks
	if key not in benchmarks:
		return -1
	benchmarks[key]['end'] = time.time()
	benchmarks[key]['elapsed'] = benchmarks[key]['end'] - benchmarks[key]['start']
	return benchmarks[key]['elapsed'] 

def bench_get(key):
	global benchmarks
	if key not in benchmarks:
		return -1
	return benchmarks[key]

######################################################
#     CONFIGURE COMMAND LINE ARGUMENTS
######################################################
def set_logfile(option, opt, value, parser):
    parser.values.logfile = value
    parser.values.logfile_set_explicitly = True

parser = OptionParser()
parser.set_usage("python insert2neo.py [-b | -p | -l FILE]...")

parser.add_option("-d","--debug", dest="verbose",action="store_true",default=False,
		  help="Output debugging information.")

group1 = OptionGroup(parser, "Top-Level Options",
		     "Choose whether to insert from Postgres or from a Logfile")

group1.add_option("-b","--bench", dest="bench",action="store_true",default=False,
		  help="QUERY: Benchmark tests by building graphs for random inodes")

group1.add_option("-l","--logfile", dest="logfile", #action='callback', callback=set_logfile,
		  help="INSERTION: Use the specified log file as input", 
		  metavar="LOGFILE", default=False)

group1.add_option("-p","--postgres", dest="postgres",action="store_true",default=False,
		  help="INSERTION: Use postgres provenance database as input to neo4j")

parser.add_option_group(group1)

group2 = OptionGroup(parser, "Postgres Options ONLY",
		     "Use these options only after selecting the -p flag")

group2.add_option("-A","--all", dest="all",default=False, metavar="CRED",
		  help="Scans *ALL* credentials starting at CRED in the PostGres DB and encodes them as a provenance graph in neo4j")

group2.add_option("-c","--cred", dest="cred",default=False,
		  help="Specify a credential for which to encode into neo4j", metavar="CRED")


group2.add_option("-i","--inode", dest="inode",default=False,
		  help="Specify a inode for which to encode into a neo4j provenance graph", metavar="INODE")

group2.add_option("-v","--version", dest="version",default=False,
		  help="Specify a inode version for which to encode into a neo4j provenance graph", metavar="VER")

parser.add_option_group(group2)

(options, args) = parser.parse_args()

if((options.bench and (options.logfile or options.postgres))
   or (options.logfile and options.postgres)):
	parser.error('options -b, -l, and -p are mutually exclusive')
elif not options.bench and not options.logfile and not options.postgres:
	parser.error('either -b, -l, or -p is required')

if options.postgres:
	if not options.all and not options.cred and not options.inode:
		parser.error('option -p requires option -A, -c, or -i')
	if options.all:
		if options.cred or options.inode:
			parser.error('options -A, -c, -i are mutually exclusive')
	elif options.cred:
		if options.inode:
			parser.error('options -A, -c, -i are mutually exclusive')

	if options.inode:
		if not options.version:
			parser.error('option -i requires option -v')

######################################################
#             DATABASE CONFIGURATION
######################################################
conn_string = "dbname='provenance' user='adam' "
debug("Connecting to database\n\t->%s\n" % (conn_string))
# get a connection, if a connect cannot be made an exception will be raised here
conn = psycopg2.connect(conn_string)

# conn.cursor will return a cursor object, you can use this query to perform queries
# note that in this example we pass a cursor_factory argument that will
# dictionary cursor so COLUMNS will be returned as a dictionary so we
# can access columns by their name instead of index.
cursor = conn.cursor(cursor_factory=psycopg2.extras.DictCursor)


######################################################
#             NEO4J CONFIGURATION
######################################################
graph_db = neo4j.GraphDatabaseService("http://localhost:7474/db/data/")
index = graph_db.get_or_create_index(neo4j.Node, "inode_index")

#Used in in inode scanner
included_inodes = {}
inodes_in_graph = {}

######################################################
#     LOAD PRECOMPILED DENTRY MAP
######################################################
link_map = {}
'''
debug("Loading pickle file of dentries... ")
bench_start('pickle')
pkl_file = open('../dentries.pkl','rb')
link_map = pickle.load(pkl_file)
pkl_file.close()
debug(" finished (%f sec)\n" % (bench_end('pickle')))
'''

if __name__ == "__main__":
	parser.print_usage()
	sys.exit(0)

def label_inode(inode):
	#Set to True for full path name
	label = ""

	if False:
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
	else:
		if inode in link_map:
			label = link_map[inode]['fname']
		
	if label != "":
		return label
	else:
		return inode

