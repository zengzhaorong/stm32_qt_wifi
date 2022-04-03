#include <stdio.h>
#include <string.h>
#include "led.h"
#include "delay.h"
#include "key.h"
#include "sys.h"
#include "usart.h"
#include "usart3.h"
#include "ringbuffer.h"
#include "cJSON.h"
#include "wifi_esp8266.h"
#include "protocol.h"
#include "config.h"


extern unsigned char ring_buf[512];
extern struct ringbuffer wifi_ringbuf;


int main(void)
{

	delay_init();	    	 //延时函数初始化	  
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //设置NVIC中断分组2:2位抢占优先级，2位响应优先级
	uart_init(115200);	 //串口初始化为115200

	wifi_init();
	proto_init(wifi_send_data);

	wifi_config_AP_mode(WIFI_AP_SSID, WIFI_AP_PWD);

	wifi_config_tcp_server(TCP_SERVER_IP, TCP_SERVER_PORT);

	wifi_set_AT(AT_GET_LOCAL_IP, strlen(AT_GET_LOCAL_IP), NULL, 0);

	while(1)
	{

		// 未连接上，wifi模块处理接收的数据
		if(wifi_get_tcp_state() != TCP_STATE_CONNECTED)
		{
			wifi_data_handle();
		}
		else	// 已连接，协议模块处理数据
		{
			proto_server_handel(&wifi_ringbuf);
		}

	}	 
}

