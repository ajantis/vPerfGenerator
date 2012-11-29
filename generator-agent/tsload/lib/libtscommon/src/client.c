#define LOG_SOURCE "client"
#include <log.h>

#include <agent.h>
#include <mempool.h>
#include <defs.h>
#include <threads.h>
#include <hashmap.h>
#include <syncqueue.h>
#include <client.h>

#include <libjson.h>

#include <stdlib.h>

#include <poll.h>
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
thread_t t_client_process;

/* Protects sending from multiple threads simultaneously*/
static thread_mutex_t send_mutex;

static thread_event_t conn_event;

squeue_t proc_queue;

/*
 * Hash table helpers
 * */
static clnt_msg_handler_t* handlers[CLNTMHTABLESIZE];

static unsigned cmh_hash_handler(void* object) {
	clnt_msg_handler_t* hdl = (clnt_msg_handler_t*) object;
	return hdl->mh_msg_id & CLNTMHTABLEMASK;
}

static unsigned cmh_hash_key(void* key) {
	unsigned* msg_id = (unsigned*) key;
	return *msg_id & CLNTMHTABLESIZE;
}

static void* cmh_next(void* obj) {
	clnt_msg_handler_t* hdl = (clnt_msg_handler_t*) obj;

	return hdl->mh_next;
}

static void cmh_set_next(void* obj, void* next) {
	clnt_msg_handler_t* hdl = (clnt_msg_handler_t*) obj;
	clnt_msg_handler_t* h_hext = (clnt_msg_handler_t*) next;

	hdl->mh_next = h_hext;
}

static int cmh_compare(void* obj, void* key) {
	clnt_msg_handler_t* hdl = (clnt_msg_handler_t*) obj;
	unsigned* msg_id = (unsigned*) key;

	return ((int) *msg_id) - hdl->mh_msg_id;
}

static hashmap_t hdl_hashmap = {
	.hm_size = CLNTMHTABLESIZE,
	.hm_heads = (void**) handlers,

	.hm_hash_object = cmh_hash_handler,
	.hm_hash_key = cmh_hash_key,
	.hm_next = cmh_next,
	.hm_set_next = cmh_set_next,
	.hm_compare = cmh_compare
};

int clnt_send(JSONNODE* node);

static clnt_msg_handler_t* clnt_create_msg() {
	unsigned msg_id = __sync_fetch_and_add(&clnt_msg_id, 1);
	clnt_msg_handler_t* hdl = mp_malloc(sizeof(clnt_msg_handler_t));

	event_init(&hdl->mh_event, "clnt_msg_handler");

	hdl->mh_next = NULL;
	hdl->mh_msg_id = msg_id;
	hdl->mh_response = NULL;

	hash_map_insert(&hdl_hashmap, hdl);

	return hdl;
}

static void clnt_delete_handler(clnt_msg_handler_t* hdl) {
	hash_map_remove(&hdl_hashmap, hdl);

	mp_free(hdl);
}

int clnt_process_response(unsigned msg_id, JSONNODE* response, clnt_response_type_t rt) {
	clnt_msg_handler_t* hdl = NULL;

	hdl = (clnt_msg_handler_t*) hash_map_find(&hdl_hashmap, &msg_id);

	if(hdl == NULL) {
		logmsg(LOG_WARN, "Failed to process incoming message: unknown message id %u", msg_id);
		return -1;
	}

	/*Notify sender thread that we got a response*/
	hdl->mh_response_type = rt;
	hdl->mh_response = response;

	event_notify_one(&hdl->mh_event);

	return 0;
}

int clnt_process_command(unsigned msg_id, JSONNODE* command, JSONNODE* msg) {
	char* cmd;
	JSONNODE* ret;

	if(json_type(command) != JSON_STRING)
		return -1;

	cmd = json_as_string(command);

	logmsg(LOG_TRACE, "Invoked command %s", cmd);

	ret = agent_process_command(cmd, msg);

	if(ret == NULL) {
		logmsg(LOG_WARN, "Processed command %s returned NULL!", cmd);
		return -1;
	}

	json_push_back(ret, json_new_i("id", msg_id));

	clnt_send(ret);
	json_delete(ret);

	return 0;
}

/* Process incoming message from buffer
 *
 * - Parses buffer into JSON
 * - Finds 'id' field in JSON
 * - Searches handler with appropriate id
 * - Unblocks sender, and sets response JSON
 *
 * @return 0 if processing successful and -1 otherwise*/
int clnt_process_msg(JSONNODE* msg) {
	JSONNODE_ITERATOR i_id, i_response, i_command, i_msg, i_error, i_end;
	unsigned msg_id;
	int ret;

	if(msg == NULL) {
		logmsg(LOG_WARN, "Failed to process incoming message: not JSON");
		goto fail;
	}

	i_end = json_end(msg);
	i_id = json_find(msg, "id");

	if(i_id == i_end || json_type(*i_id) != JSON_NUMBER) {
		logmsg(LOG_WARN, "Failed to process incoming message: unknown 'id'");
		goto fail;
	}

	msg_id = json_as_int(*i_id);

	/* Determine, which type of message we received and
	 * go to subroutine*/
	if((i_response = json_find(msg, "response")) != i_end) {
		ret = clnt_process_response(msg_id, *i_response, RT_RESPONSE);
	} else if(
			(i_command = json_find(msg, "cmd")) != i_end &&
			(i_msg = json_find(msg, "msg")) != i_end) {
		ret = clnt_process_command(msg_id, *i_command, *i_msg);
	}
	else if((i_error = json_find(msg, "error")) != i_end) {
		ret = clnt_process_response(msg_id, *i_error, RT_ERROR);
	}
	else {
		logmsg(LOG_WARN, "Failed to process incoming message: invalid format");
		goto fail;
	}

	json_delete(msg);

	if(ret == -1)
		goto fail;

	return 0;

fail:
	if(msg)
		json_delete(msg);
	return -1;
}

void* clnt_proc_thread(void* arg) {
	THREAD_ENTRY(arg, void, unused);
	JSONNODE* msg;

	while(!clnt_finished) {
		msg = (JSONNODE*) squeue_pop(&proc_queue);

		clnt_process_msg(msg);
	}

	THREAD_END:
		THREAD_FINISH(arg);
}

void clnt_on_disconnect() {
	clnt_connected = FALSE;
	/*TODO: cleanup all client resources*/

	shutdown(clnt_sockfd, SHUT_RDWR);
	close(clnt_sockfd);

	/*Notify connect thread*/
	event_notify_all(&conn_event);
}

void clnt_recv_parse(void* buffer, size_t recvlen) {
	JSONNODE* node;
	char* msg = (char*) buffer;

	size_t off = 0, len = 0;

	while(off < recvlen) {
		/*Process data in buffer*/
		logmsg(LOG_TRACE, "Processing %lu bytes off: %lu", len, off);

		len = strlen(msg) + 1;
		node = json_parse(msg);

		logmsg(LOG_TRACE, "msg: %s", msg);

		if(node) {
			squeue_push(&proc_queue, node);
		}
		else {
			logmsg(LOG_WARN, "Failure during receive: not a valid JSON");
		}

		off += len;
		msg += len;
	}
}

/**
 * clnt_recv_thread - thread that receives data from socket,
 * parses it and sends to clnt_process_thread
 */
void* clnt_recv_thread(void* arg) {
	THREAD_ENTRY(arg, void, unused);

	struct pollfd sock_poll = {
		.fd = clnt_sockfd,
		.events = POLLIN | POLLNVAL | POLLHUP,
		.revents = 0
	};

	void *buffer, *bufptr;
	size_t buflen;
	size_t recvlen;

	size_t remaining;

	int ret;

	while(!clnt_finished && clnt_connected) {
		poll(&sock_poll, 1, CLNT_RECV_TIMEOUT);

		/*XXX: poll return events on disconnect are platform-specific*/
		if(!(sock_poll.revents & POLLIN))
			continue;

		if(sock_poll.revents & POLLNVAL) {
			logmsg(LOG_CRIT, "Invalid socket state fd: %d", clnt_sockfd);
			break;
		}

		if(sock_poll.revents & POLLHUP) {
			logmsg(LOG_CRIT, "Disconnected %d", clnt_sockfd);
			break;
		}

		recvlen = 0;
		remaining = buflen = CLNT_CHUNK_SIZE;
		bufptr = buffer = mp_malloc(buflen);
		memset(buffer, 0, buflen);

		/*Receive data from socket by chunks of data*/
		while(1) {
			/*Receive chunk of data*/
			ret = recv(clnt_sockfd, bufptr, remaining, MSG_NOSIGNAL);

			if(ret <= 0) {
				logmsg(LOG_WARN, "recv() was failed, disconnecting");

				THREAD_EXIT();
			}

			bufptr += ret;
			recvlen += ret;
			remaining -= ret;

			poll(&sock_poll, 1, 0);
			/*No more data left in socket, break*/
			if(!(sock_poll.revents & POLLIN))
				break;

			/* We had filled buffer, reallocate it */
			if(remaining == 0) {
				buflen += CLNT_CHUNK_SIZE;
				buffer = mp_realloc(buffer, buflen);

				remaining += CLNT_CHUNK_SIZE;

				memset(bufptr, 0, CLNT_CHUNK_SIZE);
			}
		}

		logmsg(LOG_TRACE, "Received buffer @%p len: %lu", buffer, recvlen);
		clnt_recv_parse(buffer, recvlen);

		mp_free(buffer);
		buffer = NULL;
	}

THREAD_END:
	clnt_on_disconnect();
	mp_free(buffer);

	THREAD_FINISH(arg);
}

/**
 * Send JSON message to remote server
 *
 * @param node - message
 *
 * @return number of sent bytes or -1 on error
 */
int clnt_send(JSONNODE* node) {
	char* json_msg = NULL;
	size_t len = 0;

	int ret;

	if(!clnt_connected) {
		logmsg(LOG_CRIT, "Failed to send message, client not connected");

		return -1;
	}

	json_msg = json_write(node);
	len = strlen(json_msg) + 1;

	mutex_lock(&send_mutex);
	ret = send(clnt_sockfd, json_msg, len, MSG_NOSIGNAL);
	mutex_unlock(&send_mutex);

	json_free(json_msg);

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
	event_wait(&hdl->mh_event);

	*p_response = hdl->mh_response;

	/*Delete handler because it is not needed anymore*/
	clnt_delete_handler(hdl);

	return 0;
}

int clnt_connect() {
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

void* clnt_connect_thread(void* arg) {
	THREAD_ENTRY(arg, void, unused);

	while(!clnt_finished) {
		if(!clnt_connected) {
			if(clnt_connect() != CLNT_OK) {
				sleep(CLNT_RETRY_TIMEOUT);
				continue;
			}

			clnt_connected = TRUE;

			/*Restart receive thread*/
			t_init(&t_client_receive, NULL, "clnt_recv_thread", clnt_recv_thread);

			agent_hello();
		}

		/*Wait until socket will be disconnected*/
		event_wait(&conn_event);
	}

THREAD_END:
	THREAD_FINISH(arg);
}


int clnt_init() {
	hash_map_init(&hdl_hashmap, "hdl_hashmap");
	mutex_init(&send_mutex, "send_mutex");
	event_init(&conn_event, "conn_event");

	squeue_init(&proc_queue, "proc_queue");

	t_init(&t_client_connect, NULL, "clnt_conn_thread", clnt_connect_thread);
	t_init(&t_client_process, NULL, "clnt_proc_thread", clnt_proc_thread);

	return 0;
}

int clnt_fini() {
	clnt_finished = TRUE;
	clnt_connected = FALSE;

	shutdown(clnt_sockfd, SHUT_RDWR);
	close(clnt_sockfd);

	return CLNT_OK;
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

