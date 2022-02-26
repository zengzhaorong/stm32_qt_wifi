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
typedef int (*send_func_t)(uint8_t *data, int len);


typedef struct
{
	char head;
	char verify;
	char tail;
	int len;
	int pack_len;
} proto_detect_t;



int proto_server_handel(struct ringbuffer *ringbuf);

int proto_init(send_func_t send);
void proto_deinit(void);


#endif	// _PROTOCOL_H_
