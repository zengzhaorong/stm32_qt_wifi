#ifndef _SOCKET_CLIENT_H_
#define _SOCKET_CLIENT_H_

#include <netinet/in.h>
#include "type.h"

/* C++ include C */
#ifdef __cplusplus
extern "C" {
#endif
#include "ringbuffer.h"
#include "protocol.h"
#ifdef __cplusplus
}
#endif


#define CLI_SENDBUF_SIZE    1024
#define CLI_RECVBUF_SIZE	(CLI_SENDBUF_SIZE*2)


#define HEARTBEAT_INTERVAL_S		10


typedef enum
{
	STATE_DISCONNECT,	//断开连接
	STATE_CONNECTED,	//连接成功
	STATE_LOGIN,		//登陆成功
	STATE_CLOSE,		//关闭
}client_state_e;



struct clientInfo
{
	int fd;
	pthread_mutex_t	send_mutex;
	struct sockaddr_in 	srv_addr;		// server ip addr
	struct ringbuffer recv_ringbuf;			// socket receive data ring buffer
	proto_detect_t proto_detect;
	client_state_e state;		// client state
};


int socket_client_start(char *ip_port_str);

void socket_client_stop(void);


#endif	// _SOCKET_CLIENT_H_
