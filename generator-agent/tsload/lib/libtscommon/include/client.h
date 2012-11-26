/*
 * client.h
 *
 *  Created on: 19.11.2012
 *      Author: myaut
 */

#ifndef CLIENT_H_
#define CLIENT_H_

#include <libjson.h>

#include <threads.h>

/*Receive timeout (in us)*/
#define CLNT_RECV_TIMEOUT	10000

#define CLNT_CHUNK_SIZE		2048

#define CLNTHOSTLEN	32

#define CLNT_OK				0
#define CLNT_ERR_RESOLVE	-1
#define CLNT_ERR_SOCKET		-2
#define CLNT_ERR_CONNECT	-3

typedef enum {
	RT_RESPONSE,
	RT_ERROR,

	RT_TIMEOUT		/*Response was not received in timeout*/
} clnt_response_type_t;

typedef struct clnt_msg_handler {
	thread_event_t mh_event;

	unsigned mh_msg_id;

	struct clnt_msg_handler* mh_next;

	clnt_response_type_t mh_response_type;
	JSONNODE* mh_response;
} clnt_msg_handler_t;

#define CLNTMHTABLESIZE 16
#define CLNTMHTABLEMASK (CLNTMHTABLESIZE - 1)

#define CLNT_RETRY_TIMEOUT 3

int clnt_init();
int clnt_fini();

#endif /* CLIENT_H_ */
