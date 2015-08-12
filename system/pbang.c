#define _XOPEN_SOURCE 500
#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <stdlib.h>
#include <sys/mount.h>
#include <limits.h>

#include <sys/stat.h>
#include <fcntl.h>

#include <libgen.h>
#undef basename
#include <ftw.h>
#include <uuid/uuid.h>
#include <attr/xattr.h>

#include <zlib.h>

#include "bitvec.h"
#include "provmon_proto.h"

/*
 * I hate it as much as you do, but we need globals here so that the ftw
 * callback can get information that was initialized in the main function.
 * In other words, ftw ftl.
 */
gzFile loggz;
struct bitvec inodes;
uuid_be uuid;

/*
 * Prints a happy little usage message.
 */
static inline void usage()
{
	fprintf(stderr,
		"Usage: pbang DEV FSTYPE\n"
		"Generates a provenance Big Bang for the filesystem on DEV\n"
		"of type FSTYPE and stores the log in /prov-bang.log.gz.\n");
}

/*
 * All the heavy lifting happens here.  Checks to see if this inode is a hard
 * link to one we've already encountered, and if not, issues inode_alloc and
 * setattr messages.  Then, regardless of the check, sends an inode_link message
 * to "register" the filename.
 */
static int callback(const char *fpath, const struct stat *sb, int typeflag,
		struct FTW *ftwbuf)
{
	struct provmsg_inode_alloc allocmsg;
	struct provmsg_setattr attrmsg;
	struct provmsg_link linkmsg;
	char *parent_path;
	const char *fname;
	struct stat parent;
	int fname_len;

	/* Set up early so we can copy larger chunks to other messages */
	linkmsg.inode.sb_uuid = uuid;
	linkmsg.inode.ino = sb->st_ino;

	if (!bitvec_test(&inodes, sb->st_ino)) {
		bitvec_set(&inodes, sb->st_ino);

		/* Write allocation info */
		msg_initlen(&allocmsg.msg, sizeof(allocmsg));
		allocmsg.msg.type = PROVMSG_INODE_ALLOC;
		allocmsg.msg.cred_id = 0;
		allocmsg.inode = linkmsg.inode;
		if (gzwrite(loggz, &allocmsg, sizeof allocmsg) < sizeof allocmsg) {
			perror("gzwrite");
			return 1;
		}

		/* Write attribute info */
		msg_initlen(&attrmsg.msg, sizeof(attrmsg));
		attrmsg.msg.type = PROVMSG_SETATTR;
		attrmsg.msg.cred_id = 0;
		attrmsg.inode = linkmsg.inode;
		attrmsg.mode = sb->st_mode;
		attrmsg.uid = sb->st_uid;
		attrmsg.gid = sb->st_gid;
		if (gzwrite(loggz, &attrmsg, sizeof(attrmsg)) < sizeof(attrmsg)) {
			perror("gzwrite");
			return 1;
		}
	}

	/* Send link (i.e. filename) message */
	linkmsg.msg.type = PROVMSG_LINK;
	linkmsg.msg.cred_id = 0;
	/* .inode field was set up earlier */
	if (ftwbuf->level > 0) {
		fname = strndup(fpath, PATH_MAX);
		parent_path = strndup(fpath, PATH_MAX);
		if (stat(dirname(parent_path), &parent)) {
			perror("stat");
			return 1;
		}
		linkmsg.dir = parent.st_ino;
		fname = basename(fname);
		fname_len = strlen(fname);
	} else {
		/* Stick with convention that root dir is "its own parent" */
		linkmsg.dir = linkmsg.inode.ino;
		fname_len = 0;
	}
	msg_initlen(&linkmsg.msg, sizeof(linkmsg) + fname_len);
	if (gzwrite(loggz, &linkmsg, sizeof(linkmsg)) < sizeof(linkmsg)) {
		perror("gzwrite");
		return 1;
	}
	if (fname_len > 0 && gzwrite(loggz, fname, fname_len) < fname_len) {
		perror("gzwrite");
		return 1;
	}

	return 0;
}

/*
 * Main function.  Just checks the arguments, initializes the environment, and
 * sets everything in motion by calling nftw.  The callback function does the
 * fun stuff.
 */
int main(int argc, char *argv[])
{
	int rv = 0;
	static char template[] = "/tmp/tmp.XXXXXX";
	char *tmpdir;
	char logpath[256];
	int logfd;
	char *xattr = NULL;
	unsigned len = 255;

	if (argc != 3) {
		usage();
		return 1;
	}

	if (geteuid()) {
		fprintf(stderr, "pbang: must be run as root\n");
		return 1;
	}

	/* Create random UUID */
	uuid_generate_random(uuid.b);

	/* Create a mountpoint to use */
	tmpdir = mkdtemp(template);
	if (tmpdir == NULL) {
		perror("mkdtemp");
		return 1;
	}

	/* Mount the device */
	if (mount(argv[1], tmpdir, argv[2], MS_NOSUID | MS_NODEV | MS_NOEXEC |
				MS_NOATIME, "")) {
		perror("mount");
		rv = 1;
		goto out_rmdir;
	}
	fprintf(stderr, "pbang: temporary mount at %s\n", tmpdir);

	/* Check for a label */
	xattr = malloc(len+1);
	if(!xattr)
	  goto out_umount;

	rv = getxattr(tmpdir, "security.provenance", xattr, len);
	if (rv > 0) {
	  //fprintf(stderr, "pbang: provenance label already exists: %s. overwriting.\n",xattr);
	  fprintf(stdout, "pbang: old provenance label is ");
	  for (size_t i = 0; i < rv; i ++) {
	    fprintf(stdout, "%x", xattr[i]);
	  }
	  fprintf(stdout, ".\n");
		//fprintf(stderr, "pbang: provenance label already "
		//		"exists, remove to continue\n");
		//rv = 1;
		//goto out_umount;
	} else if (rv < 0 && errno != ENOATTR) {
		perror("getxattr");
		rv = 1;
		goto out_xattr;
	}


	/* Open logfile, whether it exists or not */
	strcpy(logpath, tmpdir);
	strcat(logpath, "/prov-bang.log.gz");
	logfd = open(logpath, O_WRONLY | O_CREAT, 0600);
	//logfd = open(logpath, O_WRONLY | O_CREAT | O_EXCL, 0600);
	if (logfd < 0) {
		rv = 1;
		fprintf(stdout, "pbang: logfd < 0\n");
		goto out_xattr;
	}
	loggz = gzdopen(logfd, "wb");
	if (!loggz) {
		rv = 1;
		close(logfd);
		goto out_xattr;
	}

	/* Set up inode bitmap */
	if (bitvec_init(&inodes)) {
		rv = 1;
		goto out_close;
	}

	/* Walk the file tree under our mountpoint */
	rv = nftw(tmpdir, callback, 10, FTW_MOUNT | FTW_PHYS);
	if (rv < 0) {
		fprintf(stderr, "nftw: internal failure\n");
		rv = 1;
		goto out_bitvec;
	}

	/* Write uuid to xattr. This is our "commit" */
	//if (setxattr(tmpdir, "security.provenance", &uuid, sizeof uuid, XATTR_CREATE)) {
	if (setxattr(tmpdir, "security.provenance", &uuid, sizeof uuid, 0)) {
	  perror("setxattr");
	  rv = 1;
	}

	len = getxattr(tmpdir, "security.provenance", xattr, 255);
	fprintf(stdout, "pbang: new provenance label is ");
	for (size_t i = 0; i < len; i ++) {
	  fprintf(stdout, "%x", xattr[i]);
	}
	fprintf(stdout, ".\n");
	

out_bitvec:
	/* Clean up, clean up */
	bitvec_destroy(&inodes);

out_close:
	/* Everybody, everywhere! */
	if (gzclose(loggz)) {
		perror("fclose");
		rv = 1;
	}

out_xattr:
	free(xattr);

out_umount:
	/* Clean up, clean up */
	if (umount(tmpdir)) {
		perror("umount");
		rv = 1;
	}

out_rmdir:
	/* Everybody do your share! */
	if (rmdir(tmpdir)) {
		perror("rmdir");
		rv = 1;
	}
	return rv;
}
