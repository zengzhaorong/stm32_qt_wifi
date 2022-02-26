#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "protocol.h"
#include "type.h"


uint8_t proto_send_buf[PROTO_PACK_MAX_LEN] = {0};


proto_data_t proto_data = {0};

int proto_0x03_sendHeartBeat(void)
{
	static int hb_time = 0;
	uint8_t data_buf[32];
	int data_len = 0;
	int pack_len = 0;

	memcpy(data_buf, &hb_time, 4);
	data_len += 4;

	hb_time ++;

	proto_makeup_packet(0, 0x03, data_len, data_buf, proto_send_buf, PROTO_PACK_MAX_LEN, &pack_len);

	proto_data.send(proto_data.arg, proto_send_buf, pack_len);
	
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

int proto_packet_analy(uint8_t *pack, int pack_len, uint8_t *seq, uint8_t *cmd, int *len, uint8_t **data)
{

	if(pack==NULL || seq==NULL || cmd==NULL || len==NULL || data==NULL)
		return -1;

	if(pack_len < PROTO_PACK_MIN_LEN)
		return -2;

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


int proto_packet_detect(struct ringbuffer *ringbuf, proto_detect_t *detect, uint8_t *proto_data, int size, int *proto_len)
{
	char buf[64];
	int len;
	char veri_buf[] = PROTO_VERIFY;
	int tmp_protoLen;
	uint8_t byte;

	if(ringbuf==NULL || proto_data==NULL || proto_len==NULL || size<PROTO_PACK_MIN_LEN)
		return -1;

	tmp_protoLen = *proto_len;

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
						printf("ERROR: %s: pack len[%d] > buf size[%d]\n", __FUNCTION__, size, detect->pack_len);
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

	*proto_len = tmp_protoLen;

	return -1;
}


int proto_init(send_func_t send, void *arg)
{

	memset(&proto_data, 0, sizeof(proto_data_t));

	proto_data.send = send;
	proto_data.arg = arg;

    return 0;
}

void proto_deinit(void)
{
    
}
