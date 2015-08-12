#ifndef GRAPH_COMPONENTS_H
#define GRAPH_COMPONENTS_H

#include "linux-mman.h"
#include "linux-fs.h"
#include "linux-shm.h" 

#include "provmon_proto.h"

/* Old PG stuff. delete after transition is complete */
#define CMDLEN 2048
#define MAXENVLEN (CMDLEN-256)

/* Enumerators for graph types (used in place of strcmp's) */
enum {
        SNAP_TYPE_ACTIVITY,
        SNAP_TYPE_ENTITY,
        SNAP_TYPE_AGENT,
	SNAP_TYPE_SELINUX,
	NUM_SNAP_TYPES,
};

enum {
        SNAP_SUBTYPE_BOOT,
        SNAP_SUBTYPE_FORK,
        SNAP_SUBTYPE_INODE,
        SNAP_SUBTYPE_PACKET,
        SNAP_SUBTYPE_USER,
        SNAP_SUBTYPE_GROUP,
	SNAP_TYPE_SELINUX_SUBJECT,
	SNAP_TYPE_SELINUX_OBJECT,
	NUM_SNAP_SUBTYPES,
};

enum {
        SNAP_RELATION_WASFORKEDFROM,
        SNAP_RELATION_USED,
        SNAP_RELATION_WASGENERATEDBY,
        SNAP_RELATION_WASCONTROLLEDBY,
        SNAP_RELATION_ACTEDONBEHALFOF,
        SNAP_RELATION_WASDERIVEDFROM,
	SNAP_RELATION_SELINUX_MAY_READ,
	SNAP_RELATION_SELINUX_MAY_WRITE,	
	NUM_SNAP_RELATIONSS,
};


  /* Key for InodeVersionMap */
  struct InodeVersion {
    uint64_t inode;
    uint64_t version;

    bool operator<(const InodeVersion& iv) const
    {
      if (inode < iv.inode) return true;
      if (inode > iv.inode) return false;
      if (version < iv.version) return true;
      if (version > iv.version) return false;
      return false;
    }
  } __attribute__((packed));

  /* Key for EdgeMap */
  struct EdgePair {
    int SrcId;
    int DstId;

    bool operator<(const EdgePair& e) const
    {
      if (SrcId < e.SrcId) return true;
      if (SrcId > e.SrcId) return false;
      if (DstId < e.DstId) return true;
      if (DstId > e.DstId) return false;
      return false;
    }
  } __attribute__((packed));

  /* Key for UserMap */
  struct User {
    uint32_t uid;
    uint32_t suid;
    uint32_t euid;
    uint32_t fsuid;
    
    bool operator<(const User& u) const
    {
      if (uid < u.uid) return true;
      if (uid > u.uid) return false;
      if (suid < u.suid) return true;
      if (suid > u.suid) return false;
      if (euid < u.euid) return true;
      if (euid > u.euid) return false;
      if (fsuid < u.fsuid) return true;
      if (fsuid > u.fsuid) return false;
      return false;
    }
  } __attribute__((packed));

  struct Group {
    uint32_t gid;
    uint32_t sgid;
    uint32_t egid;
    uint32_t fsgid;
    
    bool operator<(const Group& g) const
    {
      if (gid < g.gid) return true;
      if (gid > g.gid) return false;
      if (sgid < g.sgid) return true;
      if (sgid > g.sgid) return false;
      if (egid < g.egid) return true;
      if (egid > g.egid) return false;
      if (fsgid < g.fsgid) return true;
      if (fsgid > g.fsgid) return false;
      return false;
    }
  } __attribute__((packed));

  /* Key for CredIpPortMap */
  struct CredIpPort {
    unsigned long cred_id;
    uint16_t port;
    char * ip_address;

    bool operator<(const CredIpPort & cip) const
    {
      if (cred_id < cip.cred_id) return true;
      if (cred_id > cip.cred_id) return false;
      if (port < cip.port) return true;
      if (port > cip.port) return false;
      if (strcmp(ip_address,cip.ip_address) < 0) return true;
      if (strcmp(ip_address,cip.ip_address) > 0) return false;
      return false;
    }
  } __attribute__((packed));

#endif
