#define LOG_SOURCE "client"
#include <log.h>

#include <defs.h>
#include <threads.h>

#include <client.h>

#include <libjson.h>

#include <stdlib.h>

#include <netdb.h>
#include <sys/socket.h>

#define CLNT_CHUNK	1024

JSONNODE* json_clnt_hello_msg();
JSONNODE* json_clnt_msg_format(const char* command, JSONNODE* msg);

int clnt_sockfd = -1;

int clnt_port = 9090;
char clnt_host[CLNTHOSTLEN] = "localhost";

/* Message unique identifier */
static unsigned clnt_msg_id = 0;

/* Finish flag for clnt_recv_thread*/
int clnt_finished = FALSE;

thread_t t_client_receive;

void* clnt_recv_thread(void* arg) {
	THREAD_ENTRY(arg, void, unused);

	struct timeval tv;
	struct timeval tv_zero;
	fd_set read_fd;

	void *buffer, *bufptr;
	size_t buflen;
	size_t recvlen;

	FD_ZERO(&read_fd);
	FD_SET(clnt_sockfd, &read_fd);

	tv.tv_sec = 0;
	tv.tv_usec = CLNT_RECV_TIMEOUT;

	tv_zero.tv_sec = 0;
	tv_zero.tv_usec = 0;

	while(!clnt_finished) {
		if(select(1, &read_fd, NULL, NULL, &tv) == -1)
			continue;

		recvlen = 0;
		buflen = CLNT_CHUNK_SIZE;
		bufptr = buffer = malloc(buflen);

		while(1) {\
			/*Receive chunk of data*/
			recvlen += recv(clnt_sockfd, bufptr, CLNT_CHUNK_SIZE, MSG_NOSIGNAL);

			/*No more data left in socket, break*/
			if(select(1, &read_fd, NULL, NULL, &tv_zero) == -1)
				break;

			buflen += CLNT_CHUNK_SIZE;
			bufptr += CLNT_CHUNK_SIZE;
			buffer = realloc(buffer, buflen);
		}

		/*Process data in buffer*/
		logmsg(LOG_TRACE, "Received %lu bytes from socket", recvlen);

		free(buffer);
	}

	THREAD_EXIT(arg);
}

int clnt_send(JSONNODE* node) {
	int ret = 0;
	char* json_msg = json_write(node);
	size_t len = strlen(json_msg);

	ret = send(clnt_sockfd, json_msg, len, MSG_NOSIGNAL);

	json_free(json_msg);
	json_delete(node);

	return ret;
}

int clnt_invoke(const char* command, JSONNODE* msg_node) {
	JSONNODE* node = json_clnt_msg_format(command, msg_node);
	int ret = 0;

	ret = clnt_send(node);

	return ret;
}

int clnt_init() {
	struct hostent*	he;
	struct sockaddr_in sa;

	if((he = gethostbyname(clnt_host)) == NULL) {
		logmsg(LOG_CRIT, "Failed to resolve host %s", clnt_host);
		return CLNT_ERR_RESOLVE;
	}

	clnt_sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if(clnt_sockfd == -1) {
		logmsg(LOG_CRIT, "Failed to open socket");
		return CLNT_ERR_SOCKET;
	}

	sa.sin_family = AF_INET;
	sa.sin_port = htons(clnt_port);
	sa.sin_addr = *((struct in_addr*) he->h_addr);

	memset(sa.sin_zero, 0, sizeof(sa.sin_zero));

	if(connect(clnt_sockfd, (struct sockaddr*) & sa,
			sizeof(struct sockaddr)) == -1) {
		logmsg(LOG_CRIT, "Failed to connect to %s:%d", clnt_host, clnt_port);
		return CLNT_ERR_CONNECT;
	}

	t_init(&t_client_receive, NULL, 0, clnt_recv_thread);

	return clnt_invoke("hello", json_clnt_hello_msg());
}

int clnt_fini() {
	clnt_finished = TRUE;

	shutdown(clnt_sockfd, 2);

	return CLNT_OK;
}

JSONNODE* json_clnt_hello_msg() {
	return json_new(JSON_NODE);
}

JSONNODE* json_clnt_msg_format(const char* command, JSONNODE* msg_node) {
	JSONNODE* node = json_new(JSON_NODE);
	JSONNODE* cmd_node = json_new_a(node, command);

	json_set_name(cmd_node, "cmd");
	json_set_name(msg_node, "msg");

	json_push_back(node, cmd_node);
	json_push_back(node, msg_node);

	return node;
}
