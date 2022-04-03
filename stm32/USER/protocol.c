#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "wifi_esp8266.h"
#include "protocol.h"
#include "type.h"


uint8_t proto_recv_buf[PROTO_PACK_MAX_LEN] = {0};
uint8_t proto_ack_buf[PROTO_PACK_MAX_LEN] = {0};
proto_detect_t proto_detect;

//协议发送函数指针
send_func_t proto_send_data = NULL;

/*
 * buffer按十六进制输出
 */
void hex_dump(const char *buf, int num)
{
    int i = 0;
    for (; i < num; i++) 
    {
        printf("%02X ", buf[i]);
        if ((i+1)%8 == 0) 
            printf("\n");
    }
    printf("\n");
}


static int server_0x03_heartbeat(uint8_t *data, int len, uint8_t *ack_buf, int ack_size, int *ack_len)
{
	int tmplen = 0;
	int seq = 0;
	int ret = 0;

	if(data==NULL || len<=0)
		return -1;

	memcpy(&seq, data +tmplen, 4);
	tmplen += 4;

	printf("%s: seq %d\r\n", __FUNCTION__, seq);

	/* ---------------- ack ---------------- */

	tmplen = 0;

	ret = 0;
	memcpy(ack_buf +tmplen, &ret, 4);
	tmplen += 4;

	*ack_len = tmplen;

    return 0;
}

int proto_makeup_packet(uint8_t seq, uint8_t cmd, int len, uint8_t *data, uint8_t *outbuf, int size, int *outlen)
{
	uint8_t *packBuf = outbuf;
	int packLen = 0;

	if((len!=0 && data==NULL) || outbuf==NULL || outlen==NULL)
		return -1;

	/* if outbuf is not enough */
	if(PROTO_DATA_OFFSET +len +1 > size)
	{
		printf("ERROR: %s: outbuf size [%d:%d] is not enough !!!\n", __FUNCTION__, size, PROTO_DATA_OFFSET +len +1);
		return -2;
	}

	packBuf[PROTO_HEAD_OFFSET] = PROTO_HEAD;
	packLen += 1;
	
	memcpy(packBuf +PROTO_VERIFY_OFFSET, PROTO_VERIFY, 4);
	packLen += 4;

	packBuf[PROTO_SEQ_OFFSET] = seq;
	packLen += 1;
	
	packBuf[PROTO_CMD_OFFSET] = cmd;
	packLen += 1;

	memcpy(packBuf +PROTO_LEN_OFFSET, &len, 4);
	packLen += 4;

	memcpy(packBuf +PROTO_DATA_OFFSET, data, len);
	packLen += len;

	packBuf[PROTO_DATA_OFFSET +len] = PROTO_TAIL;
	packLen += 1;

	*outlen = packLen;

	return 0;
}

static int proto_packet_detect(struct ringbuffer *ringbuf, proto_detect_t *detect, uint8_t *proto_data, int size, int *proto_len)
{
	char buf[64];
	int len;
	char veri_buf[] = PROTO_VERIFY;
	int tmp_protoLen;
	uint8_t byte;

	if(ringbuf==NULL || proto_data==NULL || proto_len==NULL || size<PROTO_PACK_MIN_LEN)
		return -1;

	tmp_protoLen = detect->len;

	/* get and check protocol head */
	if(!detect->head)
	{
		while(ringbuf_datalen(ringbuf) > 0)
		{
			ringbuf_read(ringbuf, &byte, 1);
			if(byte == PROTO_HEAD)
			{
				proto_data[0] = byte;
				tmp_protoLen = 1;
				detect->head = 1;
				//printf("********* detect head\n");
				break;
			}
			else
				printf("%c", byte);
		}
	}

	/* get and check verify code */
	if(detect->head && !detect->verify)
	{
		while(ringbuf_datalen(ringbuf) > 0)
		{
			ringbuf_read(ringbuf, &byte, 1);
			if(byte == veri_buf[tmp_protoLen-1])
			{
				proto_data[tmp_protoLen] = byte;
				tmp_protoLen ++;
				if(tmp_protoLen == 1+strlen(PROTO_VERIFY))
				{
					detect->verify = 1;
					//printf("********* detect verify\n");
					break;
				}
			}
			else
			{
				if(byte == PROTO_HEAD)
				{
					proto_data[0] = byte;
					tmp_protoLen = 1;
					detect->head = 1;
				}
				else
				{
					tmp_protoLen = 0;
					detect->head = 0;
				}
			}
		}
	}

	/* get other protocol data */
	if(detect->head && detect->verify)
	{
		while(ringbuf_datalen(ringbuf) > 0)
		{
			if(tmp_protoLen < PROTO_DATA_OFFSET)	// read data_len
			{
				len = ringbuf_read(ringbuf, buf, sizeof(buf) < PROTO_DATA_OFFSET -tmp_protoLen ? \
													sizeof(buf) : PROTO_DATA_OFFSET -tmp_protoLen);
				if(len > 0)
				{
					memcpy(proto_data +tmp_protoLen, buf, len);
					tmp_protoLen += len;
				}
				if(tmp_protoLen >= PROTO_DATA_OFFSET)
				{
					memcpy(&len, proto_data +PROTO_LEN_OFFSET, 4);
					detect->pack_len = PROTO_DATA_OFFSET +len +1;
					if(detect->pack_len > size)
					{
						printf("ERROR: %s: pack len[%d] > buf size[%d]\n", __FUNCTION__, detect->pack_len, size);
						memset(detect, 0, sizeof(proto_detect_t));
					}
				}
			}
			else	// read data
			{
				len = ringbuf_read(ringbuf, buf, sizeof(buf) < detect->pack_len -tmp_protoLen ? \
													sizeof(buf) : detect->pack_len -tmp_protoLen);
				if(len > 0)
				{
					memcpy(proto_data +tmp_protoLen, buf, len);
					tmp_protoLen += len;
					if(tmp_protoLen == detect->pack_len)
					{
						if(proto_data[tmp_protoLen-1] != PROTO_TAIL)
						{
							printf("%s : packet data error, no detect tail!\n", __FUNCTION__);
							memset(detect, 0, sizeof(proto_detect_t));
							tmp_protoLen = 0;
							break;
						}
						*proto_len = tmp_protoLen;
						memset(detect, 0, sizeof(proto_detect_t));
						//printf("%s : get complete protocol packet, len: %d\n", __FUNCTION__, *proto_len);
						return 0;
					}
				}
			}
		}
	}

	detect->len = tmp_protoLen;

	return -1;
}

static int proto_packet_analy(uint8_t *pack, int pack_len, uint8_t *seq, uint8_t *cmd, int *len, uint8_t **data)
{
	if(pack==NULL || seq==NULL || cmd==NULL || len==NULL || data==NULL)
		return -1;

	if(pack_len < PROTO_PACK_MIN_LEN)
		return -2;

	//hex_dump(pack, pack_len);

	*seq = pack[PROTO_SEQ_OFFSET];

	*cmd = pack[PROTO_CMD_OFFSET];

	memcpy(len, pack +PROTO_LEN_OFFSET, 4);

	if(*len +PROTO_PACK_MIN_LEN != pack_len)
	{
		return -1;
	}

	if(*len > 0)
		*data = pack + PROTO_DATA_OFFSET;

	return 0;
}

static int proto_packet_handle(uint8_t *pack, uint32_t len)
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
			ret = server_0x03_heartbeat(data, data_len, proto_ack_buf, sizeof(proto_ack_buf), &ack_len);
            break;
        
        default:
            break;
    }
	
	/* send ack data */
	if(ret==0 && ack_len>0)
	{
		proto_makeup_packet(seq, cmd, ack_len, proto_ack_buf, proto_recv_buf, PROTO_PACK_MAX_LEN, &data_len);
		proto_send_data(proto_recv_buf, data_len);
	}


    return 0;
}

int proto_server_handel(struct ringbuffer *ringbuf)
{
    int proto_len;
    int ret = -1;

    if(ringbuf == NULL)
        return -1;

    // 检测协议包数据
    ret = proto_packet_detect(ringbuf, &proto_detect, proto_recv_buf, sizeof(proto_recv_buf), &proto_len);
    if(ret == 0)
    {
        //处理协议包数据
        proto_packet_handle(proto_recv_buf, proto_len);
    }

    return 0;
}


int proto_init(send_func_t send)
{

    memset(&proto_detect, 0, sizeof(proto_detect_t));

    proto_send_data = send;

    return 0;
}

void proto_deinit(void)
{
    
}
