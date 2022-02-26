#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "socket_client.h"
#include <errno.h>
#include "mainwindow.h"
#include "protocol.h"
#include "config.h"


/* C++ include C */
#ifdef __cplusplus
extern "C" {
#endif
#include "ringbuffer.h"
#ifdef __cplusplus
}
#endif

uint8_t proto_recv_buf[PROTO_PACK_MAX_LEN] = {0};
uint8_t proto_ack_buf[PROTO_PACK_MAX_LEN] = {0};

struct clientInfo client_info = {0};

int client_0x03_heartbeat(struct clientInfo *client, uint8_t *data, int len, uint8_t *ack_data, int size, int *ack_len)
{
	printf("%s enter ++\n", __FUNCTION__);
	return 0;
}

int socket_send_data(void *arg, uint8_t *data, int len)
{
	struct clientInfo *client = (struct clientInfo *)arg;
	int total = 0;
	int ret;

	if(data==NULL || client->state==STATE_CLOSE)
		return -1;

	// lock
	pthread_mutex_lock(&client->send_mutex);
	do{
		ret = send(client->fd, data +total, len -total, MSG_NOSIGNAL);
		if(ret < 0)
		{
			if(errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN)
			{
				usleep(1000);
				continue;
			}
			else
			{
				client->state = STATE_CLOSE;
				perror("socket send failed");
				printf("ret: %d, errno = %d\n", ret, errno);
				break;
			}
		}
		total += ret;
	}while(total < len);
	// unlock
	pthread_mutex_unlock(&client->send_mutex);

	return total;
}

int socket_recv_data(struct clientInfo *client)
{
	uint8_t tmpBuf[128];
	int len, space;
	int ret = 0;

	if(client == NULL)
		return -1;

	space = ringbuf_space(&client->recv_ringbuf);

	memset(tmpBuf, 0, sizeof(tmpBuf));
	len = recv(client->fd, tmpBuf, (int)sizeof(tmpBuf)>space ? space:(int)sizeof(tmpBuf), 0);
	if(len > 0)
	{
		ret = ringbuf_write(&client->recv_ringbuf, tmpBuf, len);
	}
	else if(len < 0)
	{
		if(errno != EINTR && errno != EWOULDBLOCK && errno != EAGAIN)
		{
			client->state = STATE_CLOSE;
			perror("socket recv failed");
			printf("ret: %d, errno = %d\n", len, errno);
		}
	}

	return ret;
}


static int socket_param_init(struct clientInfo *client, char *srv_ip, int srv_port)
{
	int flags = 0;
    int ret;

	printf("server ip: %s, port: %d\n", srv_ip, srv_port);

	client->fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(client->fd < 0)
	{
		printf("open socket failed!");
        return -1;
	}

	pthread_mutex_init(&client->send_mutex, NULL);
	client->state = STATE_DISCONNECT;

	flags = fcntl(client->fd, F_GETFL, 0);
	fcntl(client->fd, F_SETFL, flags | O_NONBLOCK);

	client->srv_addr.sin_family = AF_INET;
	inet_pton(AF_INET, srv_ip, &client->srv_addr.sin_addr);
	client->srv_addr.sin_port = htons(srv_port);

	ret = ringbuf_init(&client->recv_ringbuf, CLI_RECVBUF_SIZE);
	if(ret != 0)
	{
        return -2;
	}

    return ret;
}

static void socket_param_deinit(struct clientInfo *client)
{
	ringbuf_deinit(&client->recv_ringbuf);
	close(client->fd);
	printf("%s: close socket.\n", __FUNCTION__);
}

static int proto_packet_handle(struct clientInfo *client, uint8_t *pack, uint32_t len)
{
	uint8_t seq = 0, cmd = 0;
	uint8_t *data = NULL;
	int data_len = 0;
    int ack_len = 0;
    int ret;

    ret = proto_packet_analy(pack, len, &seq, &cmd, &data_len, &data);
	if(ret != 0)
		return -1;

	printf("ptoto cmd: 0x%02x, seq: %d, pack_len: %d, data_len: %d\n", cmd, seq, len, data_len);

    switch(cmd)
    {
        case 0x01:
            break;

        case 0x02:
            break;

        case 0x03:
			ret = client_0x03_heartbeat(client, data, data_len, proto_ack_buf, sizeof(proto_ack_buf), &ack_len);
            break;
        
        default:
            break;
    }
	
	/* send ack data */
	if(ret==0 && ack_len>0)
	{
		proto_makeup_packet(seq, cmd, ack_len, proto_ack_buf, proto_recv_buf, PROTO_PACK_MAX_LEN, &data_len);
		socket_send_data(client, proto_recv_buf, data_len);
	}

    return 0;
}

int client_proto_handel(struct clientInfo *client)
{
    int proto_len;
	int recv_ret;
	int det_ret;

	// 接收数据
	recv_ret = socket_recv_data(client);

    // 检测协议包数据
    det_ret = proto_packet_detect(&client->recv_ringbuf, &client->proto_detect, proto_recv_buf, sizeof(proto_recv_buf), &proto_len);
    if(det_ret == 0)
    {
        //处理协议包数据
        proto_packet_handle(client, proto_recv_buf, proto_len);
    }

	if(recv_ret<=0 && det_ret!=0)
	{
		usleep(30*1000);
	}

    return 0;
}


void *socket_client_task(void *arg)
{
    struct clientInfo *client = &client_info;
	char svr_str[32] = {0};
	char *ip, *port;
	time_t heartbeat_time = 0;
	time_t init_time = 0;
	time_t curTime;
    int ret;

    printf("%s enter +_+\n", __FUNCTION__);

	strcpy(svr_str, (char *)arg);
	// get ip and port
	ip = strtok((char *)svr_str, ":");
	port = strtok(NULL, ":");
	if(ip==NULL || port==NULL)
	{
		printf("ERROR: ip or port input illegal!\n");
		goto EXIT;
	}

    ret = socket_param_init(client, ip, atoi(port));
    if(ret != 0)
    {
        printf("connect server failed!\n");
        goto EXIT;
    }

	proto_init(socket_send_data, client);

	init_time = time(NULL);
    while(client->state != STATE_CLOSE)
    {
		curTime = time(NULL);
		switch (client->state)
		{
			case STATE_DISCONNECT:
				ret = connect(client->fd, (struct sockaddr *)&client->srv_addr, sizeof(client->srv_addr));
				if(ret == 0)
				{
					client->state = STATE_CONNECTED;
					printf("connect server success.\n");
				}
				// connect timeout - 连接超时(超过3秒未连接成功)
				if(abs(curTime - init_time) >= 3)
				{
					printf("connect server timeout.\n");
					socket_client_stop();
				}
				break;

			case STATE_CONNECTED:
				if(abs(curTime - heartbeat_time) >= HEARTBEAT_INTERVAL_S)
				{
					proto_0x03_sendHeartBeat();
					printf("heartbear ...\n");
					heartbeat_time = curTime;
				}
				break;

			case STATE_LOGIN:
				break;
			
			default:
				break;
		}

		if(client->state == STATE_CONNECTED)
		{
			client_proto_handel(client);
		}

		usleep(200*1000);

    }

EXIT:
	socket_param_deinit(client);
	mainwin_set_disconnect();
    printf("%s exit -_-\n", __FUNCTION__);

    return NULL;
}

/* satrt socket client - 启动socket套接字客户端连接通信*/
int socket_client_start(char *ip_port_str)
{
	static char svr_str[32] = {0};
	pthread_t tid;
	int ret;

	strcpy(svr_str, ip_port_str);
	ret = pthread_create(&tid, NULL, socket_client_task, svr_str);
	if(ret != 0)
	{
		return -1;
	}

	return 0;
}

/* stop socket client - 启动socket套接字连接通信*/
void socket_client_stop(void)
{
    struct clientInfo *client = &client_info;
	client->state = STATE_CLOSE;
	printf("stop socket client.\n");
}
