/*
 * client.c
 *
 *  Created on: 22.12.2012
 *      Author: myaut
 */

#define LOG_SOURCE "client"
#include <log.h>

#include <defs.h>
#include <client.h>

#include <stdlib.h>

#include <poll.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>

int clnt_sockfd = -1;

struct sockaddr_in clnt_sa;

PLATAPI int clnt_connect(const char* clnt_host, const int clnt_port) {
	struct hostent*	he;

	if((he = gethostbyname(clnt_host)) == NULL) {
		logmsg(LOG_CRIT, "Failed to resolve host %s", clnt_host);
		return CLNT_ERR_RESOLVE;
	}

	clnt_sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if(clnt_sockfd == -1) {
		logmsg(LOG_CRIT, "Failed to open socket");
		return CLNT_ERR_SOCKET;
	}

	clnt_sa.sin_family = AF_INET;
	clnt_sa.sin_port = htons(clnt_port);
	clnt_sa.sin_addr = *((struct in_addr*) he->h_addr);

	memset(clnt_sa.sin_zero, 0, sizeof(clnt_sa.sin_zero));

	if(connect(clnt_sockfd, (struct sockaddr*) & clnt_sa,
			   sizeof(struct sockaddr)) == -1) {
		logmsg(LOG_CRIT, "Failed to connect to %s:%d, retrying in %ds", clnt_host, clnt_port, CLNT_RETRY_TIMEOUT);
		return CLNT_ERR_CONNECT;
	}

	logmsg(LOG_INFO, "Connected to %s:%d", clnt_host, clnt_port);

	return CLNT_OK;
}

PLATAPI int clnt_disconnect() {
	shutdown(clnt_sockfd, SHUT_RDWR);
	close(clnt_sockfd);

	return CLNT_OK;
}

PLATAPI int clnt_poll(long timeout) {
	struct pollfd sock_poll = {
		.fd = clnt_sockfd,
		.events = POLLIN | POLLNVAL | POLLHUP,
		.revents = 0
	};

	poll(&sock_poll, 1, timeout);

	if(sock_poll.revents & POLLNVAL)
		return CLNT_POLL_FAILURE;

	if(sock_poll.revents & POLLHUP)
		return CLNT_POLL_DISCONNECT;

	if(sock_poll.revents & POLLIN)
		return CLNT_POLL_NEW_DATA;

	return CLNT_POLL_OK;
}

PLATAPI int clnt_sock_send(void* data, size_t len) {
	return send(clnt_sockfd, data, len, MSG_NOSIGNAL);
}

PLATAPI int clnt_sock_recv(void* data, size_t len) {
	return recv(clnt_sockfd, data, len, MSG_NOSIGNAL);
}
