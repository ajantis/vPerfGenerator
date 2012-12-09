#define LOG_SOURCE "client"
#include <log.h>

#include <agent.h>
#include <mempool.h>
#include <defs.h>
#include <threads.h>
#include <hashmap.h>
#include <syncqueue.h>
#include <client.h>
#include <atomic.h>

#include <libjson.h>

#include <stdlib.h>
#include <assert.h>

#include <poll.h>
#include <unistd.h>
#include <pthread.h>
#include <netdb.h>
#include <sys/socket.h>

#define CLNT_CHUNK	1024

JSONNODE* json_clnt_command_format(const char* command, JSONNODE* msg_node, unsigned msg_id);
void clnt_proc_set_msg(clnt_proc_msg_t* msg);

/* Globals */

int clnt_sockfd = -1;

int clnt_port = 9090;
char clnt_host[CLNTHOSTLEN] = "localhost";

struct sockaddr_in clnt_sa;

/* Message unique identifier */
static atomic_t clnt_msg_id = 0;

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
DECLARE_HASH_MAP(hdl_hashmap, clnt_msg_handler_t, CLNTMHTABLESIZE, mh_msg_id, mh_next,
	{
		unsigned* msg_id = (unsigned*) key;
		return *msg_id & CLNTMHTABLESIZE;
	},
	{
		unsigned* msg_id1 = (unsigned*) key1;
		unsigned* msg_id2 = (unsigned*) key2;

		return ((int) *msg_id1) - *msg_id2;
	}
);

int clnt_send(JSONNODE* node);

static clnt_msg_handler_t* clnt_create_msg() {
	unsigned msg_id = (unsigned) atomic_inc_ret(&clnt_msg_id);
	clnt_msg_handler_t* hdl = mp_malloc(sizeof(clnt_msg_handler_t));

	event_init(&hdl->mh_event, "clnt_msg_handler");

	hdl->mh_next = NULL;
	hdl->mh_msg_id = msg_id;
	hdl->mh_response = NULL;
	hdl->mh_response_type = RT_NOTHING;
	hdl->mh_error_code = 0;

	hash_map_insert(&hdl_hashmap, hdl);

	return hdl;
}

static void clnt_delete_handler(clnt_msg_handler_t* hdl) {
	hash_map_remove(&hdl_hashmap, hdl);

	mp_free(hdl);
}

int clnt_process_response(unsigned msg_id, JSONNODE* response, clnt_response_type_t rt, JSONNODE* n_code) {
	clnt_msg_handler_t* hdl = NULL;
	int error_code;

	hdl = (clnt_msg_handler_t*) hash_map_find(&hdl_hashmap, &msg_id);

	if(hdl == NULL) {
		logmsg(LOG_WARN, "Failed to process incoming message: unknown message id %u", msg_id);
		json_delete(response);
		return -1;
	}

	if(n_code != NULL) {
		hdl->mh_error_code = json_as_int(n_code);
	}

	/*Notify sender thread that we got a response*/
	hdl->mh_response_type = rt;
	hdl->mh_response = json_copy(response);

	event_notify_one(&hdl->mh_event);

	return 0;
}

/**
 * Process incoming command
 *
 * @return 0 if all OK or -1 if message was incorrect
 */
int clnt_process_command(unsigned msg_id, JSONNODE* n_cmd, JSONNODE* n_msg) {
	clnt_proc_msg_t* msg;

	if(json_type(n_cmd) != JSON_STRING ||
	   json_type(n_msg) != JSON_NODE)
		return -1;

	/* Create proc message */
	msg = (clnt_proc_msg_t*) mp_malloc(sizeof(clnt_proc_msg_t));

	msg->m_msg_id = msg_id;
	msg->m_command = json_as_string(n_cmd);
	msg->m_response_type = RT_NOTHING;
	msg->m_response = NULL;

	clnt_proc_set_msg(msg);

	/* Call agent's method */

	logmsg(LOG_TRACE, "Invoked command %s message #%d", msg->m_command, msg_id);

	agent_process_command(msg->m_command, n_msg);

	/* Process response */

	assert(msg->m_response != NULL);

	json_push_back(msg->m_response, json_new_i("id", msg_id));
	clnt_send(msg->m_response);

	/* Clean up */

	json_delete(msg->m_response);
	json_free(msg->m_command);

	clnt_proc_set_msg(NULL);
	mp_free(msg);

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
	JSONNODE_ITERATOR i_id, i_response, i_command, i_msg, i_error, i_code, i_end;
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
		ret = clnt_process_response(msg_id, *i_response, RT_RESPONSE, NULL);
	} else if(
			(i_command = json_find(msg, "cmd")) != i_end &&
			(i_msg = json_find(msg, "msg")) != i_end) {
		ret = clnt_process_command(msg_id, *i_command, *i_msg);
	}
	else if((i_code = json_find(msg, "code")) != i_end &&
			(i_error = json_find(msg, "error")) != i_end) {
		ret = clnt_process_response(msg_id, *i_error, RT_ERROR, *i_code);
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

		if(msg == NULL)
			THREAD_EXIT();

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

		/*FIXME: poll return events on disconnect are platform-specific*/
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

/*
 * Should be called from clnt_proc_thread
 *
 * @return message that is currently processed
 * */
clnt_proc_msg_t* clnt_proc_get_msg(void) {
	assert(t_self() == &t_client_process);

	return (clnt_proc_msg_t*) t_client_process.t_arg;
}

void clnt_proc_set_msg(clnt_proc_msg_t* msg) {
	assert(t_self() == &t_client_process);

	t_client_process.t_arg = (void*) msg;
}

/* Adds response to command that is processed
 * Called by agent and saves response message (error or message)
 * to clnt_proc_thread.t_arg
 *
 * @param node response or error
 *
 * @see agent_response_msg, @see agent_error_msg
 * */
void clnt_add_response(clnt_response_type_t type, JSONNODE* node) {
	clnt_proc_msg_t* msg = clnt_proc_get_msg();

	assert(node != NULL);
	assert(msg != NULL);

	if(msg->m_response != NULL) {
		/* Already has a response, discard this one.
		 *
		 * This may happen if callee already reports error,
		 * than caller reports an internal error just to
		 * be sure */
		json_delete(node);
		return;
	}

	msg->m_response_type = type;
	msg->m_response = node;
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
	json_delete(node);

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
			t_init(&t_client_receive, NULL, clnt_recv_thread, "clnt_recv_thread");

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

	t_init(&t_client_connect, NULL, clnt_connect_thread, "clnt_conn_thread");
	t_init(&t_client_process, NULL, clnt_proc_thread, "clnt_proc_thread");

	return 0;
}

void clnt_fini(void) {
	clnt_finished = TRUE;
	clnt_connected = FALSE;

	/*This will finish conn_thread*/
	shutdown(clnt_sockfd, SHUT_RDWR);
	close(clnt_sockfd);

	/*Delete all remaining messages and finish proc_thread*/
	squeue_destroy(&proc_queue, json_delete);

	/*Finish connect thread*/
	event_notify_all(&conn_event);

	t_destroy(&t_client_process);
	t_destroy(&t_client_receive);
	t_destroy(&t_client_connect);

	event_destroy(&conn_event);
	mutex_destroy(&send_mutex);
	hash_map_destroy(&hdl_hashmap);
}

JSONNODE* json_clnt_command_format(const char* command, JSONNODE* msg_node, unsigned msg_id) {
	JSONNODE* node = json_new(JSON_NODE);

	json_push_back(node, json_new_i("id", msg_id));
	json_push_back(node, json_new_a("cmd", command));

	json_set_name(msg_node, "msg");
	json_push_back(node, msg_node);

	return node;
}

