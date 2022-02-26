#ifndef _PROTOCOL_H_
#define _PROTOCOL_H_

#include "ringbuffer.h"
#include "type.h"


#define PROTO_HEAD		0xFF
#define PROTO_TAIL		0xFE
#define PROTO_VERIFY	"ABCD"

#define PROTO_HEAD_OFFSET		0
#define PROTO_VERIFY_OFFSET		(PROTO_HEAD_OFFSET +1)
#define PROTO_SEQ_OFFSET		(PROTO_VERIFY_OFFSET +4)
#define PROTO_CMD_OFFSET		(PROTO_SEQ_OFFSET +1)
#define PROTO_LEN_OFFSET		(PROTO_CMD_OFFSET +1)
#define PROTO_DATA_OFFSET		(PROTO_LEN_OFFSET +4)

#define PROTO_PACK_MAX_LEN      256
#define PROTO_PACK_MIN_LEN		(PROTO_DATA_OFFSET +1)

// 发送数据函数，返回值：实际发送的长度，-1出错
typedef int (*send_func_t)(void *arg, uint8_t *data, int len);


typedef struct
{
	char head;
	char verify;
	char tail;
	int len;
	int pack_len;
} proto_detect_t;

typedef struct
{
	send_func_t send;	//协议发送函数指针
	void *arg;
} proto_data_t;


int proto_0x03_sendHeartBeat(void);

int proto_makeup_packet(uint8_t seq, uint8_t cmd, int len, uint8_t *data, uint8_t *outbuf, int size, int *outlen);
int proto_packet_analy(uint8_t *pack, int pack_len, uint8_t *seq, uint8_t *cmd, int *len, uint8_t **data);
int proto_packet_detect(struct ringbuffer *ringbuf, proto_detect_t *detect, uint8_t *proto_data, int size, int *proto_len);

int proto_init(send_func_t send, void *arg);
void proto_deinit(void);


#endif	// _PROTOCOL_H_
