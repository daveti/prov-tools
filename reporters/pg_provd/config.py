
from multiprocessing import Pool
from optparse import OptionParser
import pdb
import pickle
import psycopg2
import psycopg2.extras
import pprint
from random import randrange
import socket
import sys
import time
import threading

PAGE_COLOR = "#FFFFFF"

#Node Formatting
DEFAULT_FONTSIZE = 40
DEFAULT_FONTCOLOR ='black'
AGENT_FONTSIZE = DEFAULT_FONTSIZE
AGENT_FONTCOLOR = DEFAULT_FONTCOLOR
AGENT_COLOR = '#F596B1'
AGENT_SHAPE = 'octagon'
PROCESS_FONTSIZE = DEFAULT_FONTSIZE
PROCESS_FONTCOLOR = DEFAULT_FONTCOLOR
PROCESS_COLOR = '#8DD5DE'
PROCESS_SHAPE = 'rectangle'
FILE_FONTSIZE = DEFAULT_FONTSIZE
FILE_FONTCOLOR = DEFAULT_FONTCOLOR
FILE_COLOR = '#FFF976'
FILE_SHAPE = 'ellipse'
MESSAGE_FONTSIZE = DEFAULT_FONTSIZE
MESSAGE_FONTCOLOR = DEFAULT_FONTCOLOR
MESSAGE_COLOR = '#91D449'
MESSAGE_SHAPE = 'note'
SOCK_FONTSIZE = MESSAGE_FONTSIZE
SOCK_FONTCOLOR = MESSAGE_FONTCOLOR
SOCK_COLOR = MESSAGE_COLOR
SOCK_SHAPE = MESSAGE_SHAPE

#old colors
INODE_COLOR = '#0B8830'
PROC_COLOR = '#AA08E8'
CRED_COLOR = 'dodgerblue3'

#Edge (Activity) Formatting
EDGE_DEFAULT_COLOR = '#000000'
EDGE_DEFAULT_FONTCOLOR = '#000000'
INODE_P_FONTSIZE = 40

INODE_READ_COLOR = '#000000'
INODE_WRITE_COLOR = '#FFD900'
INODE_CREATE_COLOR = '#00C9FF' 
FILE_READ_COLOR = INODE_READ_COLOR
FILE_WRITE_COLOR = INODE_WRITE_COLOR

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
	if key not in benchmarks:
		return -1
	return benchmarks[key]

######################################################
#     CONFIGURE COMMAND LINE ARGUMENTS
######################################################
parser = OptionParser()
parser.set_usage("python query.py -f FILE [ -i INODE | -e EXEC ]\n\t(You must provide FILE and a query target!)")

parser.add_option("-f","--file", dest="filename",default=False,
		  help="write the provenance graph to FILE", metavar="FILE")
parser.add_option("-d","--dag", action="store_true", dest="dag",default=False,
		  help="create directed acyclic version of provenance graph by versioning all file read/writes with a unique event id.")
parser.add_option("-v","--verbose", action="store_true", dest="verbose",default=False,
		  help="enabled debug messages.")
parser.add_option("-i","--inode", dest="inode",default=False,
		  help="specify INODE for which to build a provenance graph", metavar="INODE")
parser.add_option("-e","--exec", dest="exec_inode",default=False,
		  help="specify inode for an EXEC'd process for which to build a provenance graph", metavar="EXEC")
parser.add_option("-b","--bench", dest="bench",default=False,
		  help="specify a number of iterations for benchmarking (builds random inode's provenance graph)", metavar="BENCH")
(options, args) = parser.parse_args()

if not options.filename:
	parser.print_usage()
	sys.exit(0)

######################################################
#             DATABASE CONFIGURATION
######################################################
conn_string = "dbname='provenance' user='postgres' password='badbanana'"
debug("Connecting to database\n\t->%s\n" % (conn_string))
# get a connection, if a connect cannot be made an exception will be raised here
#conn = psycopg2.connect(conn_string)

# conn.cursor will return a cursor object, you can use this query to perform queries
# note that in this example we pass a cursor_factory argument that will
# dictionary cursor so COLUMNS will be returned as a dictionary so we
# can access columns by their name instead of index.
#cursor = conn.cursor(cursor_factory=psycopg2.extras.DictCursor)

######################################################
#     LOAD PRECOMPILED DENTRY MAP
######################################################
debug("Loading pickle file of dentries... ")
bench_start('pickle')
pkl_file = open('dentries.pkl','rb')
link_map = pickle.load(pkl_file)
pkl_file.close()
debug(" finished (%f sec)\n" % (bench_end('pickle')))




if __name__ == "__main__":
	parser.print_usage()
	sys.exit(0)


