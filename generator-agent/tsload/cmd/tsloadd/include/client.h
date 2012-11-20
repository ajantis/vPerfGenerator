/*
 * client.h
 *
 *  Created on: 19.11.2012
 *      Author: myaut
 */

#ifndef CLIENT_H_
#define CLIENT_H_

/*Receive timeout (in us)*/
#define CLNT_RECV_TIMEOUT	100

#define CLNT_CHUNK_SIZE		2048

#define CLNTHOSTLEN	32

#define CLNT_OK				0
#define CLNT_ERR_RESOLVE	-1
#define CLNT_ERR_SOCKET		-2
#define CLNT_ERR_CONNECT	-3

int clnt_init();
int clnt_fini();

#endif /* CLIENT_H_ */
