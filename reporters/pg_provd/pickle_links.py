#!/bin/python

import pickle
import psycopg2
import psycopg2.extras


######################################################
#             DATABASE CONFIGURATION
######################################################
conn_string = "dbname='provenance' user='adam'"
print "Connecting to database\n\t->%s" % (conn_string)
# get a connection, if a connect cannot be made an exception will be raised here
conn = psycopg2.connect(conn_string)

# conn.cursor will return a cursor object, you can use this query to perform queries
# note that in this example we pass a cursor_factory argument that will
# dictionary cursor so COLUMNS will be returned as a dictionary so we
# can access columns by their name instead of index.
cursor = conn.cursor(cursor_factory=psycopg2.extras.DictCursor)

#Find largest event id
cursor.execute("SELECT max(eventid) FROM link;")
max_eventid  = int(cursor.fetchall()[0][0])
cursor.execute("SELECT max(eventid) FROM unlink;")
max_eventid_unlink = int(cursor.fetchall()[0][0])
if max_eventid_unlink > max_eventid:
    max_eventid = max_eventid_unlink

#id2cm map inodes_dir_to_contents, neccessary to handle unlinks
id2cm = {}

#Read all links and unlinks into memory
link_map = {}
cursor.execute("SELECT eventid,inode,inode_dir,fname FROM link order by eventid")
for row in cursor:
    (eventid,inode,inode_dir,fname) = row
    link_map[eventid] = { 'inode':inode, 'inode_dir':inode_dir, 'fname':fname }
    id2cm[inode_dir] = {}

unlink_map = {}
cursor.execute("SELECT eventid,inode_dir,fname FROM link order by eventid")
for row in cursor:
    (eventid,inode_dir,fname) = row
    unlink_map[eventid] = { 'inode_dir':inode_dir, 'fname':fname }

dentry_map = {}

for e in range(0,max_eventid):
    if e in link_map:
        (inode,inode_dir,fname) = [link_map[e]['inode'], link_map[e]['inode_dir'], link_map[e]['fname']]
        dentry_map[inode] = { 'inode_dir':inode_dir, 'fname':fname }

        if fname in id2cm[inode_dir]:
            id2cm[inode_dir][fname].append(inode)
        else:
            id2cm[inode_dir][fname] = [inode]

    elif e in unlink_map:
        (inode_dir,fname) = [unlink_map[e]['inode_dir'],unlink_map[e]['fname']]
        if fname not in id2cm[inode_dir]:
            print "UNLINK WITHOUT LINK FIRST for %s in dir %d" % (fname,inode_dir) 
        else:
            inodes = id2cm[inode_dir][fname]
            for inode in inodes:
                del dentry_map[inode]
            del id2cm[inode_dir][fname]

output = open('dentries.pkl','wb')
pickle.dump(dentry_map,output)
output.close()
