#define LOG_SOURCE "client"
#include <log.h>

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

int clnt_send_command(const char* command, JSONNODE* msg_node) {
	JSONNODE* node =  json_clnt_msg_format(command, msg_node);
	int ret = 0;
	char* json_msg = json_write(node);
	size_t len = strlen(json_msg);

	ret = send(clnt_sockfd, json_msg, len, MSG_NOSIGNAL);

	json_free(json_msg);
	json_delete(node);

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

	return clnt_send_command("hello", json_clnt_hello_msg());
}

int clnt_fini() {
	shutdown(clnt_sockfd, 2);

	return CLNT_OK;
}

JSONNODE* json_clnt_hello_msg() {
	return json_new(JSON_NULL);
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
