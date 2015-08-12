/*
 * testlibpq.c
 *
 *      Test the C version of libpq, the PostgreSQL frontend library.
 */
#include <stdio.h>
#include <stdlib.h>
#include "libpq-fe.h"
#include "pg_handlers.h"

#define DROP 1
#define CREATE 1

int main(int argc, char **argv) {
  const char *conninfo;
  PGconn     *conn;
  PGresult   *res;

  conninfo = "dbname = provenance";

  /* Make a connection to the database */
  conn = PQconnectdb(conninfo);

  /* Check to see that the backend connection was successfully made */
  if (PQstatus(conn) != CONNECTION_OK)
    {
      fprintf(stderr, "Connection to database failed: %s",
	      PQerrorMessage(conn));
      exit_nicely(conn);
    }

  /*
   * Our test case here involves using a cursor, for which we must be inside
   * a transaction block.  We could do the whole thing with a single
   * PQexec() of "select * from pg_database", but that's too trivial to make
   * a good example.
   */

  /* Start a transaction block */
  res = PQexec(conn, "BEGIN");
  if (PQresultStatus(res) != PGRES_COMMAND_OK)
    handle_err(res,conn,"BEGIN",PQerrorMessage(conn));
  PQclear(res);  

  if(DROP){
    res = PQexec(conn, "DROP TABLE main,boot,credfork,credfree,setid,exec,inode_p,file_p,mmap,link,unlink,setattr,inode_alloc,inode_dealloc,socksend,sockrecv,sockalias,mqsend,mqrecv,shmat,readlink;");  
    PQclear(res);
  }

  /*CREATE TABLE main (
       // absolute sequencen number for event ordering
       eventid bigserial PRIMARY KEY,
       //provenance id assignned in kernel 
       provid bigint  NOT NULL,
       // what type of event is this? acceptable values are enumerated above 
       event_type smallint NOT NULL,*/
  if(CREATE){
    res = PQexec(conn, "CREATE TABLE main (eventid serial PRIMARY KEY,provid bigint NOT NULL,event_type smallint NOT NULL);");
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
      handle_err(res,conn,"CREATE main",PQerrorMessage(conn));
    PQclear(res);
  }
  
  /* CREATE TABLE boot (
  // unique event id that you can use to look up the event in another table
  eventid bigint PRIMARY_KEY,
  // uuid of the booted partition
  uuid_be uuid NOT NULL); */
  if(CREATE){
    res = PQexec(conn, "CREATE TABLE boot (eventid bigint PRIMARY KEY,uuid_be uuid NOT NULL);");
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
      handle_err(res,conn,"CREATE boot",PQerrorMessage(conn));
    PQclear(res);
  }
  
  /* CREATE TABLE credfork (
  // unique event id that you can use to look up the event in another table
  eventid bigint primary key,
  // provenance id assignned in kernel
  provid bigint NOT NULL, <--- this is redundant from main table, but may prove useful for quickly assembling a provid hitlist.
  // new forked provenance id
  newid bigint NOT NULL); */
  if(CREATE){
    res = PQexec(conn, "CREATE TABLE credfork (eventid bigint primary key,provid bigint NOT NULL, newid bigint NOT NULL);");
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
      handle_err(res,conn,"CREATE credfork",PQerrorMessage(conn));
    PQclear(res);
  }
  
  /* CREATE TABLE credfree (
      // unique event id that you can use to look up the event in another table
      eventid bigint primary key,
      // freed provenance credential
      provid bigint NOT NULL); */
  if(CREATE){
    res = PQexec(conn, "CREATE TABLE credfree (eventid bigint primary key,provid bigint NOT NULL);");
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
      handle_err(res,conn,"CREATE credfree",PQerrorMessage(conn));
    PQclear(res);    
  }

  /* CREATE TABLE setid (
      // unique event id that you can use to look up the event in another table
      eventid bigint primary key,
      // user ids
      uid integer NOT NULL,
      euid integer NOT NULL,
      gid integer NOT NULL,
      egid integer NOT NULL,
      suid integer NOT NULL,
      fsuid integer NOT NULL,
      sgid integer NOT NULL,
      fsgid integer NOT NULL); */
  if(CREATE){
    res = PQexec(conn, "CREATE TABLE setid (eventid bigint primary key,uid integer NOT NULL,euid integer NOT NULL,gid integer NOT NULL,egid integer NOT NULL,suid integer NOT NULL,fsuid integer NOT NULL,sgid integer NOT NULL,fsgid integer NOT NULL);");
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
      handle_err(res,conn,"CREATE setid",PQerrorMessage(conn));
    PQclear(res);
  }
  
  /* CREATE TABLE exec (
      // unique event id that you can use to look up the event in another table
      eventid bigint primary key,
      uuid_be uuid NOT NULL,
      inode bigint NOT NULL,
      version bigint NOT NULL,
      argc integer NOT NULL,
      argv text[] NOT NULL
      ); */
  if(CREATE){
    res = PQexec(conn, "CREATE TABLE exec (eventid bigint primary key,uuid_be uuid NOT NULL,inode bigint NOT NULL,version bigint NOT NULL,argc integer NOT NULL,argv text[] NOT NULL);");
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
      handle_err(res,conn,"CREATE exec",PQerrorMessage(conn));
    PQclear(res);
  }
  
  /* CREATE TABLE inode_p (
      // unique event id that you can use to look up the event in another table
      eventid bigint primary key,
      uuid_be uuid NOT NULL,
      inode bigint NOT NULL,
      version bigint NOT NULL,
      mask  integer NOT NULL      
      ); */
  if(CREATE){
    res = PQexec(conn, "CREATE TABLE inode_p (eventid bigint primary key,uuid_be uuid NOT NULL,inode bigint NOT NULL,version bigint NOT NULL,mask integer NOT NULL);");
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
      handle_err(res,conn,"CREATE inode_p",PQerrorMessage(conn));
    PQclear(res);
  }
  
  /* CREATE TABLE file_p (
      // unique event id that you can use to look up the event in another table
      eventid bigint primary key,
      uuid_be uuid NOT NULL,
      inode bigint NOT NULL,
      version bigint NOT NULL,
      mask integer NOT NULL      
      ); */
  if(CREATE){
    res = PQexec(conn, "CREATE TABLE file_p (eventid bigint primary key,uuid_be uuid NOT NULL,inode bigint NOT NULL,version bigint NOT NULL,mask integer NOT NULL);");
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
      handle_err(res,conn,"CREATE file_p",PQerrorMessage(conn));
    PQclear(res);
  }

  /* CREATE TABLE mmap (
      // unique event id that you can use to look up the event in another table
      eventid bigint primary key,
      uuid_be uuid NOT NULL,
      inode bigint NOT NULL,
      version bigint NOT NULL,
      prot bigint NOT NULL,
      flags bigint NOT NULL
      ); */
  if(CREATE){
    res = PQexec(conn, "CREATE TABLE mmap (eventid bigint primary key,uuid_be uuid NOT NULL,inode bigint NOT NULL,version bigint NOT NULL,prot bigint NOT NULL, flags bigint NOT NULL);");
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
      handle_err(res,conn,"CREATE mmap",PQerrorMessage(conn));
    PQclear(res);
  }
  
  /* CREATE TABLE link (
      // unique event id that you can use to look up the event in another table
      eventid bigint primary key,
      uuid_be uuid NOT NULL,
      inode_dir bigint,
      inode bigint,
      fname text); */
  if(CREATE){
    res = PQexec(conn, "CREATE TABLE link (eventid bigint primary key,uuid_be uuid NOT NULL,inode_dir bigint,inode bigint,fname text);");
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
      handle_err(res,conn,"CREATE link",PQerrorMessage(conn));
    PQclear(res);
  }
  
  /* CREATE TABLE unlink (
      // unique event id that you can use to look up the event in another table
      eventid bigint primary key,
      uuid_be uuid NOT NULL,
      inode_dir bigint NOT NULL,      
      fname text NOT NULL); */
  if(CREATE){
    res = PQexec(conn, "CREATE TABLE unlink (eventid bigint primary key,uuid_be uuid NOT NULL,inode_dir bigint NOT NULL,fname text NOT NULL);");
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
      handle_err(res,conn,"CREATE unlink",PQerrorMessage(conn));
    PQclear(res);
  }

  /* CREATE TABLE setattr (
      // unique event id that you can use to look up the event in another table
      eventid bigint primary key,
      uuid_be uuid NOT NULL,
      inode bigint NOT NULL,      
      uid integer NOT NULL,
      gid integer NOT NULL,
      mode integer NOT NULL); */
  if(CREATE){
    res = PQexec(conn, "CREATE TABLE setattr (eventid bigint primary key,uuid_be uuid NOT NULL,inode bigint NOT NULL,uid integer NOT NULL,gid integer NOT NULL,mode integer NOT NULL);");
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
      handle_err(res,conn,"CREATE setattr",PQerrorMessage(conn));
    PQclear(res);
  }

  /* CREATE TABLE inode_alloc (
      // unique event id that you can use to look up the event in another table
      eventid bigint primary key,
      uuid_be uuid NOT NULL,
      inode bigint NOT NULL); */
  if(CREATE){
    res = PQexec(conn, "CREATE TABLE inode_alloc (eventid bigint primary key,uuid_be uuid NOT NULL,inode bigint NOT NULL);");
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
      handle_err(res,conn,"CREATE inode_alloc",PQerrorMessage(conn));
    PQclear(res);
  }

  /* CREATE TABLE inode_dealloc (
      // unique event id that you can use to look up the event in another table
      eventid bigint primary key,
      uuid_be uuid NOT NULL,
      inode bigint NOT NULL); */
  if(CREATE){
    res = PQexec(conn, "CREATE TABLE inode_dealloc (eventid bigint primary key,uuid_be uuid NOT NULL,inode bigint NOT NULL);");
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
      handle_err(res,conn,"CREATE inode_dealloc",PQerrorMessage(conn));
    PQclear(res);
  }
  
  /* CREATE TABLE socksend (
      // unique event id that you can use to look up the event in another table
      eventid bigint primary key,
      low_sockid bigint NOT NULL,
      high_sockid integer NOT NULL
      family smallint,
      protocol smallint,
      address character(108),
      port integer
      ); */
  if(CREATE){
    res = PQexec(conn, "CREATE TABLE socksend (eventid bigint primary key,low_sockid bigint NOT NULL,high_sockid integer NOT NULL,family smallint,protocol smallint,address character(108),port integer);");
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
      handle_err(res,conn,"CREATE socksend",PQerrorMessage(conn));
    PQclear(res);
  }
  
  /* CREATE TABLE sockrecv (
      // unique event id that you can use to look up the event in another table
      eventid bigint primary key,
      uuid_be uuid NOT NULL,
      low_sockid bigint NOT NULL,
      high_sockid integer NOT NULL,
      family smallint NOT NULL,
      protocol smallint NOT NULL,
      address characters(108),
      port integer
      ); */
  if(CREATE){
    res = PQexec(conn, "CREATE TABLE sockrecv (eventid bigint primary key,uuid_be uuid NOT NULL,low_sockid bigint NOT NULL,high_sockid integer NOT NULL,family smallint NOT NULL,protocol smallint NOT NULL,address character(108),port integer);");
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
      handle_err(res,conn,"CREATE sockrecv",PQerrorMessage(conn));
    PQclear(res);  
  }

  /* CREATE TABLE sockalias (
      // unique event id that you can use to look up the event in another table
      eventid bigint primary key,
      uuid_be_sock uuid NOT NULL,
      low_sockid integer NOT NULL,
      high_sockid smallint NOT NULL),
      uuid_be_alias uuid NOT NULL,
      low_aliasid integer NOT NULL,
      high_aliasid smallint NOT NULL); */
  if(CREATE){
    res = PQexec(conn, "CREATE TABLE sockalias (eventid bigint primary key,uuid_be_sock uuid NOT NULL,low_sockid integer NOT NULL,high_sockid smallint NOT NULL,uuid_be_alias uuid NOT NULL,low_aliasid integer NOT NULL,high_aliasid smallint NOT NULL);");
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
      handle_err(res,conn,"CREATE sockalias",PQerrorMessage(conn));
    PQclear(res);  
  }

  /* CREATE TABLE mqsend (
      // unique event id that you can use to look up the event in another table
      eventid bigint primary key,
      msgid integer NOT NULL); */
  if(CREATE){
    res = PQexec(conn, "CREATE TABLE mqsend (eventid bigint primary key, msgid integer NOT NULL);");
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
      handle_err(res,conn,"CREATE mqsend",PQerrorMessage(conn));
    PQclear(res);  
  }

  /* CREATE TABLE mqrecv (
      // unique event id that you can use to look up the event in another table
      eventid bigint primary key,
      msgid integer NOT NULL); */
  if(CREATE){
    res = PQexec(conn, "CREATE TABLE mqrecv (eventid bigint primary key, msgid integer NOT NULL);");
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
      handle_err(res,conn,"CREATE mqrecv",PQerrorMessage(conn));
    PQclear(res);  
  }

  /* CREATE TABLE shmat (
      // unique event id that you can use to look up the event in another table
      eventid bigint primary key,
      shmid integer NOT NULL
      flags integer NOT NULL); */
  if(CREATE){
    res = PQexec(conn, "CREATE TABLE shmat (eventid bigint primary key,shmid integer NOT NULL,flags integer NOT NULL);");
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
      handle_err(res,conn,"CREATE shmat",PQerrorMessage(conn));
    PQclear(res);  
  }

  /* CREATE TABLE readlink (
      // unique event id that you can use to look up the event in another table
      eventid bigint primary key,
      uuid_be uuid NOT NULL,
      inode bigint NOT NULL); */
  if(CREATE){
    res = PQexec(conn, "CREATE TABLE readlink (eventid bigint primary key,uuid_be uuid NOT NULL,inode bigint NOT NULL);");
    if (PQresultStatus(res) != PGRES_COMMAND_OK)
      handle_err(res,conn,"CREATE readlink",PQerrorMessage(conn));
    PQclear(res);  
  }

  
  /*

  res = PQexec(conn, "insert into tbl1(i) values(4) returning i;");
  if (PQresultStatus(res) != PGRES_TUPLES_OK)
    handle_err(res,conn,"insert",PQerrorMessage(conn));	
  printf("result = %s\n",PQgetvalue(res,0,0));
  PQclear(res);

  */
  

  /* end the transaction */
  res = PQexec(conn, "END");
  PQclear(res);

  /* close the connection to the database and cleanup */
  PQfinish(conn);

  return 0;    
}
