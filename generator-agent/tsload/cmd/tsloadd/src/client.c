#define LOG_SOURCE "client"
#include <log.h>

#include <defs.h>
#include <threads.h>

#include <client.h>

#include <libjson.h>

#include <stdlib.h>

#include <unistd.h>
#include <pthread.h>
#include <netdb.h>
#include <sys/socket.h>

#define CLNT_CHUNK	1024

JSONNODE* json_clnt_hello_msg();
JSONNODE* json_clnt_command_format(const char* command, JSONNODE* msg_node, unsigned msg_id);

int clnt_sockfd = -1;

int clnt_port = 9090;
char clnt_host[CLNTHOSTLEN] = "localhost";

struct sockaddr_in clnt_sa;

/* Message unique identifier */
static unsigned clnt_msg_id = 0;


/* Threads and it's flags */
int clnt_connected = FALSE;
int clnt_finished = FALSE;

thread_t t_client_receive;
thread_t t_client_connect;

/* Hash table of invoke handlers */
static pthread_mutex_t hdl_mutex;
static clnt_msg_handler_t* handlers[CLNTMHTABLESIZE];

/* Protects sending from multiple threads simultaneously*/
static pthread_mutex_t send_mutex;

static pthread_mutex_t conn_mutex;
static pthread_cond_t conn_cv;

/*
 * Insert message handler into table
 * */
static void clnt_insert_handler(clnt_msg_handler_t* hdl) {
	clnt_msg_handler_t** head = handlers + (hdl->mh_msg_id & CLNTMHTABLESIZE);
	clnt_msg_handler_t* iter;

	pthread_mutex_lock(&hdl_mutex);
	if(*head == NULL) {
		*head = hdl;
	}
	else {
		iter = *head;

		while(iter->mh_next != NULL) {
			iter = iter->mh_next;
		}

		iter->mh_next = hdl;
	}
	pthread_mutex_unlock(&hdl_mutex);
}

static void clnt_remove_handler(clnt_msg_handler_t* hdl) {
	clnt_msg_handler_t** head = handlers + (hdl->mh_msg_id & CLNTMHTABLESIZE);
	clnt_msg_handler_t* iter;

	pthread_mutex_lock(&hdl_mutex);

	if((*head)->mh_msg_id == hdl->mh_msg_id) {
			*head = NULL;
	}
	else {
		iter = *head;

		while(iter->mh_next != NULL) {
			iter = iter->mh_next;
		}

		iter->mh_next = hdl->mh_next;
	}

	pthread_mutex_unlock(&hdl_mutex);
}

static clnt_msg_handler_t* clnt_find_handler(unsigned msg_id) {
	clnt_msg_handler_t* hdl = handlers[msg_id & CLNTMHTABLESIZE];

	pthread_mutex_lock(&hdl_mutex);

	while(hdl != NULL) {
		if(hdl->mh_msg_id == msg_id)
			break;

		hdl = hdl->mh_next;
	}

	pthread_mutex_unlock(&hdl_mutex);

	return hdl;
}

static clnt_msg_handler_t* clnt_create_msg() {
	unsigned msg_id = __sync_fetch_and_add(&clnt_msg_id, 1);
	clnt_msg_handler_t* hdl = malloc(sizeof(clnt_msg_handler_t));

	pt_init(&hdl->mh_mutex, &hdl->mh_cv);

	hdl->mh_next = NULL;
	hdl->mh_msg_id = msg_id;
	hdl->mh_response = NULL;

	clnt_insert_handler(hdl);

	return hdl;
}

static void clnt_delete_handler(clnt_msg_handler_t* hdl) {
	clnt_remove_handler(hdl);

	free(hdl);
}

int clnt_process_response(unsigned msg_id, JSONNODE* response, clnt_response_type_t rt) {
	clnt_msg_handler_t* hdl = NULL;

	hdl = clnt_find_handler(msg_id);

	if(hdl == NULL) {
		logmsg(LOG_WARN, "Failed to process incoming message: unknown message id %u", msg_id);
		return -1;
	}

	/*Notify sender thread that we got a response*/
	hdl->mh_response_type = rt;
	hdl->mh_response = response;
	pt_notify_one(&hdl->mh_mutex, &hdl->mh_cv);

	return 0;
}

int clnt_process_command(unsigned msg_id, JSONNODE* command, JSONNODE* msg) {
	return 0;
}

/* Process incoming message from buffer
 *
 * XXX: What if we'll receive two JSONs in one buffer???
 *
 * - Parses buffer into JSON
 * - Finds 'id' field in JSON
 * - Searches handler with appropriate id
 * - Unblocks sender, and sets response JSON
 *
 * @return 0 if processing successful and -1 otherwise*/
int clnt_process_msg(void* buffer) {
	JSONNODE* msg = json_parse((char*) buffer);
	JSONNODE_ITERATOR i_id, i_response, i_command, i_msg, i_error;
	unsigned msg_id;
	int ret;

	if(msg == NULL) {
		logmsg(LOG_WARN, "Failed to process incoming message: not JSON");
		goto fail;
	}

	i_id = json_find(msg, "id");

	if(i_id == NULL || json_type(*i_id) != JSON_NUMBER) {
		logmsg(LOG_WARN, "Failed to process incoming message: unknown 'id'");
		goto fail;
	}

	msg_id = json_as_int(*i_id);

	/* Determine, which type of message we received and
	 * go to subroutine*/
	if((i_response = json_find(msg, "response")) != NULL) {
		ret = clnt_process_response(msg_id, *i_response, RT_RESPONSE);
	} else if(
			(i_command = json_find(msg, "command")) != NULL &&
			(i_msg = json_find(msg, "msg")) != NULL) {
		ret = clnt_process_command(msg_id, *i_command, *i_msg);
	}
	else if((i_error = json_find(msg, "error")) != NULL) {
		ret = clnt_process_response(msg_id, *i_error, RT_ERROR);
	}
	else {
		logmsg(LOG_WARN, "Failed to process incoming message: invalid format");
		goto fail;
	}

	if(ret == -1)
		goto fail;

	return 0;

fail:
	if(msg)
		json_delete(msg);
	return -1;
}

void clnt_on_disconnect() {
	clnt_connected = FALSE;
	/*TODO: cleanup all client resources*/

	/*Notify connect thread*/
	pt_notify_one(&conn_mutex, &conn_cv);
}

void* clnt_recv_thread(void* arg) {
	THREAD_ENTRY(arg, void, unused);

	struct timeval tv;
	struct timeval tv_zero;
	fd_set read_fd;

	void *buffer, *bufptr;
	size_t buflen;
	size_t recvlen;

	int ret;

	FD_ZERO(&read_fd);
	FD_SET(clnt_sockfd, &read_fd);

	tv.tv_sec = 0;

	tv_zero.tv_sec = 0;
	tv_zero.tv_usec = 0;

	while(!clnt_finished && clnt_connected) {
		tv.tv_usec = CLNT_RECV_TIMEOUT;

		if(select(1, &read_fd, NULL, NULL, &tv) != 1)
			continue;

		recvlen = 0;
		buflen = CLNT_CHUNK_SIZE;
		bufptr = buffer = malloc(buflen);

		while(1) {
			/*Receive chunk of data*/
			ret += recv(clnt_sockfd, bufptr, CLNT_CHUNK_SIZE, MSG_NOSIGNAL);

			if(ret == -1 || ret == 0) {
				logmsg(LOG_CRIT, "Failure during receive: disconnecting");
				clnt_on_disconnect();

				free(buffer);

				THREAD_EXIT();
			}

			recvlen += ret;

			/*No more data left in socket, break*/
			if(select(1, &read_fd, NULL, NULL, &tv_zero) != 1)
				break;

			buflen += CLNT_CHUNK_SIZE;
			bufptr += CLNT_CHUNK_SIZE;
			buffer = realloc(buffer, buflen);
		}

		/*Process data in buffer*/
		clnt_process_msg(buffer);

		logmsg(LOG_TRACE, "Received %lu bytes from socket", recvlen);

		free(buffer);
	}

	THREAD_FINISH(arg);
}

void* clnt_connect_thread(void* arg) {
	THREAD_ENTRY(arg, void, unused);

	while(!clnt_finished) {
		if(!clnt_connected) {
			if(connect(clnt_sockfd, (struct sockaddr*) & clnt_sa,
						sizeof(struct sockaddr)) == -1) {
				logmsg(LOG_CRIT, "Failed to connect to %s:%d, retrying in ", clnt_host, clnt_port);

				sleep(CLNT_RETRY_TIMEOUT);
				continue;
			}

			/*Restart receive thread*/
			t_init(&t_client_receive, NULL, 0, clnt_recv_thread);

			clnt_connected = TRUE;
		}

		/*Wait until socket will be disconnected*/
		pt_wait(&conn_mutex, &conn_cv);
	}

	THREAD_FINISH(arg);
}

int clnt_send(JSONNODE* node) {
	char* json_msg = NULL;
	size_t len = 0;

	int ret;

	json_msg = json_write(node);
	len = strlen(json_msg);

	pthread_mutex_lock(&send_mutex);
	ret = send(clnt_sockfd, json_msg, len, MSG_NOSIGNAL);
	pthread_mutex_unlock(&send_mutex);

	json_free(json_msg);
	json_delete(node);

	return ret;
}

/*
 * Invoke command on remote server
 *
 * @param command - command name
 * @param msg_node - JSON representation of command'params
 * @param p_response - pointer where response is written
 *
 * NOTE: deletes msg_node in any case
 * */
int clnt_invoke(const char* command, JSONNODE* msg_node, JSONNODE** p_response) {
	clnt_msg_handler_t* hdl = clnt_create_msg();
	JSONNODE* node = json_clnt_command_format(command, msg_node, hdl->mh_msg_id);

	if(hdl == NULL) {
		json_delete(msg_node);
		return -1;
	}

	clnt_send(node);

	/*Wait until response will arrive*/
	pt_wait(&hdl->mh_mutex, &hdl->mh_cv);
	*p_response = hdl->mh_response;

	/*Delete handler because it is not needed anymore*/
	clnt_delete_handler(hdl);

	return 0;
}

int clnt_hello() {
	int ret;
	JSONNODE* response;

	ret = clnt_invoke("hello", json_clnt_hello_msg(), &response);

	/*Process agent ID*/

	return ret;
}

int clnt_init() {
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

	pthread_mutex_init(&hdl_mutex, NULL);
	pthread_mutex_init(&send_mutex, NULL);

	pt_init(&conn_mutex, &conn_cv);

	t_init(&t_client_connect, NULL, 0, clnt_connect_thread);

	return clnt_hello();
}

int clnt_fini() {
	clnt_finished = TRUE;

	shutdown(clnt_sockfd, 2);

	return CLNT_OK;
}

JSONNODE* json_clnt_hello_msg() {
	JSONNODE* node = json_new(JSON_NODE);

	JSONNODE* info_node = json_new(JSON_NODE);
	json_set_name(info_node, "info");

	json_push_back(info_node, json_new_a("hostName", "localhost"));

	json_push_back(node, info_node);

	return node;
}

JSONNODE* json_clnt_command_format(const char* command, JSONNODE* msg_node, unsigned msg_id) {
	JSONNODE* node = json_new(JSON_NODE);
	JSONNODE* cmd_node = json_new_a(node, command);

	json_push_back(node, json_new_i("id", msg_id));
	json_push_back(node, json_new_a("cmd", command));

	json_set_name(msg_node, "msg");
	json_push_back(node, msg_node);

	return node;
}

