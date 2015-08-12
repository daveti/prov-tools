/*
 * app_ipc.c
 * Application IPC implementation
 * NOTE: we are using Unix domain socket with datagram without connection and Non-blocking!
 * and this implementation is NOT thread-safe!
 * Feb 18, 2015
 * root@davejingtian.org
 * http://davejingtian.org
 */
#include "app_ipc.h"
#include "provmon_proto.h"

/* Global vars */
static int sfd;
static struct sockaddr_un addr;
static struct sockaddr_un client_addr;
static struct msghdr msgh;
static struct msghdr msgh_ack;
static struct iovec iov;
static struct iovec iov_ack;
static union {
	struct cmsghdr cmh;
	char   control[CMSG_SPACE(sizeof(struct ucred))];
	/* Space large enough to hold a ucred structure */
} control_un;
static struct provmsg_imagemagick_convert data;
static struct provmsg_imagemagick_convert ack;
static char app_proc_path[APP_PATH_LEN_MAX];
static char app_bin_name[APP_PATH_LEN_MAX];
static int app_ipc_start;
static int app_debug = 1;


/* Internal routines */
static int app_ipc_retrieve_pid(void)
{
	struct cmsghdr *cmhp;
	struct ucred *ucredp;

	if (app_debug)
		printf("app_ipc: into [%s]\n", __func__);

	/* Extract credentials information from received ancillary data */
	cmhp = CMSG_FIRSTHDR(&msgh);
	if (cmhp == NULL || cmhp->cmsg_len != CMSG_LEN(sizeof(struct ucred))) {
		printf("Error: bad cmsg header / message length\n");
		return -1;
    	}
	if (cmhp->cmsg_level != SOL_SOCKET) {
		printf("Error: cmsg_level != SOL_SOCKET\n");
		return -1;
	}
	if (cmhp->cmsg_type != SCM_CREDENTIALS) {
		printf("Error: cmsg_type != SCM_CREDENTIALS\n");
		return -1;
	}

	ucredp = (struct ucred *) CMSG_DATA(cmhp);
	if (app_debug)
		printf("Received credentials pid=[%ld], uid=[%ld], gid=[%ld]\n",
			(long) ucredp->pid, (long) ucredp->uid, (long) ucredp->gid);

	return (int)ucredp->pid;
}

static int app_ipc_get_bin_name(int pid)
{
	ssize_t r;

	if (app_debug)
		printf("app_ipc: into [%s], pid [%d]\n", __func__, pid);

	/* Construct the /proc/pid/exe path */
	memset(app_proc_path, 0x0, APP_PATH_LEN_MAX);
	snprintf(app_proc_path, APP_PATH_LEN_MAX, "%s%d%s",
		APP_PID_PATH_PREFIX, pid, APP_PID_PATH_SUFFIX);
	if (app_debug)
		printf("Debug: app_proc_path=[%s]\n", app_proc_path);

	/* Read the /proc symlink */
	memset(app_bin_name, 0x0, APP_PATH_LEN_MAX);
	r = readlink(app_proc_path, app_bin_name, APP_PATH_LEN_MAX-1);
	if (r == -1) {
		printf("Error: readlink failed [%s]\n", strerror(errno));
		return -1;
	}
	if (app_debug)
		printf("Debug: app_bin_name=[%s]\n", app_bin_name);

	return 0;
}

static int app_ipc_send_ack(void)
{
	ssize_t ns;

	if (app_debug)
		printf("app_ipc: into [%s]\n", __func__);

	/* Reinit the msgh_ack */
	msgh_ack.msg_name = (void *)&client_addr;
	msgh_ack.msg_namelen = sizeof(client_addr);
	msgh_ack.msg_control = NULL;
	msgh_ack.msg_controllen = 0;
	if (app_debug) 
		printf("Debug: msg.name [%p], msg.namelen [%d], path [%s]\n",
			msgh_ack.msg_name, msgh_ack.msg_namelen, client_addr.sun_path);

	/* Send the ACK */
	ns = sendmsg(sfd, &msgh_ack, 0);
	if (ns == -1) {
		printf("Error: sendmsg failed [%s]\n", strerror(errno));
		return -1;
	}
	if (app_debug) 
		printf("Debug: sendmsg() returned %ld\n", (long) ns);
	
	return 0;
}

/* Return 0 if success otherwise non-0 */
static int app_ipc_ima_check(void)
{
	FILE *fp;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	int count = 0;
	int ret;
	char *found;

	if (app_debug)
		printf("app_ipc: into [%s]\n", __func__);

	/* Open the IMA measurement */
	fp = fopen(APP_IMA_PATH, "r");
	if (!fp) {
		printf("fopen failed for file [%s]\n", APP_IMA_PATH);
		return -1;
	}

	/* Go thru each line */
	while ((read = getline(&line, &len, fp)) != -1) {
		if (app_debug)
			printf("Debug: [%zu]:[%s]\n", read, line);
		/* Hunt for the binary */
		found = strcasestr(line, app_bin_name);
		if (found) {
			count++;
			if (app_debug)
				printf("Debug: found a measurement for binary [%s], count [%d]\n",
					app_bin_name, count);
		}
		/* Check for early return */
		if (count > 1) {
			if (app_debug)
				printf("Debug: got [%d] measurements for binary [%s] - return\n",
					count, app_bin_name);
			ret = 1;
			goto OUT;
		}
	}

	ret = 0;

OUT:
	fclose(fp);
	if (line)
		free(line);

	return ret;
}


/* APIs */
int app_ipc_process(int stop, gzFile out)
{
	int ret;
	int pid;
	char *log;
	size_t nw;
	ssize_t nr;

	if (app_debug)
		printf("app_ipc: into [%s], stop [%d], out [%p]\n",
			__func__, stop, out);

	if (stop) {
		printf("Warning: app_ipc_process is stopped\n");
		return 0;
	}

	if (!app_ipc_start) {
		printf("Warning: app ipc is NOT started\n");
		return 0;
	}

	/* Try to recv the provenance record */
	nr = recvmsg(sfd, &msgh, MSG_DONTWAIT);
	if (nr <= 0) {
		if (app_debug)
			printf("recvmsg failed - [%s]\n", strerror(errno));
		/* If nothing happens, that is fine */
		return 0;
	}
	if (app_debug)
		printf("Debug: dump the provmsg\n"
			"input_len=[%d]\n"
			"output_len=[%d]\n"
			"input=[%s]\n"
			"output=[%s]\n",
			data.input_len, data.output_len, data.input, data.output);

	/* Get the PID of the remote client */
	pid = app_ipc_retrieve_pid();
	if (pid == -1) {
		printf("Error: app_ipc_retrieve_pid failed\n");
		return -1;
	}

	/* Get the binary name from the PID */
	ret = app_ipc_get_bin_name(pid);
	if (ret) {
		printf("Error: app_ipc_get_bin_name failed\n");
		return -1;
	}

	/* Now we can release the application */
	ret = app_ipc_send_ack();
	if (ret) {
		printf("Error: app_ipc_send_ack failed\n");
		/* However, we should not return */
	}

	/* Check the IMA */
	ret = app_ipc_ima_check();
	if (ret) {
		printf("Warning: app_ipc_ima_check failed\n");
		/* TODO: we probably should do something more here
		 * given that we know the application is compromised
		 * in a certain way...
		 * daveti
		 */
		return -1;
	}

	/* Commit the application provenance logging */
	log = (char *)&data;
	nw = sizeof(data);
	if (gzwrite(out, log, nw) < nw) {
		printf("Error: gzwrite failed\n");
		return -1;
	}
	
	return 0;
}

int app_ipc_init(void)
{
	int optval;

	if (app_debug)
		printf("app_ipc: into [%s]\n", __func__);

	/* Create socket bound to well-known address */
	if (remove(APP_SOCK_PATH) == -1 && errno != ENOENT) {
		printf("Error: remove failed\n");
		return -1;
	}

	/* Build the UNIX socket using UDP */
	if (strlen(APP_SOCK_PATH) >= sizeof(addr.sun_path)-1) {
		printf("Error: path length exceeds the max\n");
		return -1;
	}

	memset(&addr, 0x0, sizeof(struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, APP_SOCK_PATH, sizeof(addr.sun_path)-1);

	sfd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (sfd == -1) {
		printf("Error: socket failed [%s]\n", strerror(errno));
		return -1;
	}

	/* Bind the socket with our address */
	if (bind(sfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) == -1) {
		printf("Error: bind failed [%s]\n", strerror(errno));
		close(sfd);
		return -1;
	}

	/* We must set the SO_PASSCRED socket option in order to receive creditials */
	optval = 1;
	if (setsockopt(sfd, SOL_SOCKET, SO_PASSCRED, &optval, sizeof(optval)) == -1) {
		printf("Error: setsockopt failed [%s]\n", strerror(errno));
		close(sfd);
		return -1;
   	}

	/* Set 'control_un' to describe ancillary data that we want to receive */
	control_un.cmh.cmsg_len = CMSG_LEN(sizeof(struct ucred));
	control_un.cmh.cmsg_level = SOL_SOCKET;
	control_un.cmh.cmsg_type = SCM_CREDENTIALS;

	/* Set 'msgh' fields to describe 'control_un' */
	msgh.msg_control = control_un.control;
	msgh.msg_controllen = sizeof(control_un.control);

	/* Set fields of 'msgh' to point to buffer used to receive (real)
	data read by recvmsg() */
	msgh.msg_iov = &iov;
	msgh.msg_iovlen = 1;
	iov.iov_base = &data;
	iov.iov_len = sizeof(data);
	memset(&data, 0x0, sizeof(data));

	/* Save the client address for sendmsg */
	memset(&client_addr, 0x0, sizeof(client_addr));
	msgh.msg_name = (void *)&client_addr;
	msgh.msg_namelen = sizeof(client_addr);

	/* Init the ACK msg which will be used soon */
	memset(&ack, 0x0, sizeof(ack));
	ack.input_len = APP_DATA_ACK_LEN;
	ack.output_len = APP_DATA_ACK_LEN;
	sprintf(ack.input, "%s", APP_DATA_ACK_NAME);
	sprintf(ack.output, "%s", APP_DATA_ACK_NAME);
	msgh_ack.msg_iov = &iov_ack;
	msgh_ack.msg_iovlen = 1;
	iov_ack.iov_base = &ack;
	iov_ack.iov_len = sizeof(ack);

	/* Mark the flag and return */
	app_ipc_start = 1;
	return 0;
}

void app_ipc_exit(void)
{
	if (app_debug)
		printf("app_ipc: into [%s]\n", __func__);

	if (app_ipc_start)
		close(sfd);
}
