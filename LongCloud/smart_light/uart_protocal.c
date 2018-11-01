/***************************************************************************
uart_protocal.c
xiaodong 
1.initial 2018/2/2
****************************************************************************/
#include "hsf.h"
#include "hfsys.h"
#include "smart_light.h"
#include "uart_protocal.h"
#include "xmodem.h"
#if 1
void mydebug(char *data)
{
	hfuart_send(HFUART0, data, strlen(data),1000);	
	hfuart_send(HFUART0, "\r\n", 2,1000);	
}
#else
#define mydebug 
#endif

uart_buffer_t g_uart0;
uart_buffer_t g_uart1;

unsigned char g_light_onoff=0;
unsigned char g_light_brightness=100;
unsigned int GLightColor = 0;
unsigned char GLightColorTemp = 0;
unsigned char g_mac_addr[12];
void Set_Uart_Port(void)
{
	char rsp[255];
	int ret=255;
	if(hfat_send_cmd("AT+UART=9600,8,1,NONE,NFC,0\r\n",strlen("AT+UART=9600,8,1,NONE,NFC,0\r\n"),rsp,ret) == HF_SUCCESS)
	{
		//Set Bluetooth Uart baud to 9600
		//HF_Debug(DEBUG_WARN,"Set uart1 fail!\n");
		//hfuart_send(HFUART1, rsp, strlen(rsp),1000);	
		return;
	}
}

void Get_Mac_Addr(void)
{
	char rsp[255];
	int ret=255,i;
	if(hfat_send_cmd("AT+WSMAC\r\n",strlen("AT+WSMAC\r\n"),rsp,ret) == HF_SUCCESS)
	{
		//+OK=<MAC ADDR> CR LF CR LF
		for(i=0;i<12;i++)
			g_mac_addr[i]=rsp[4+i];
		
		//hfuart_send(HFUART0, g_mac_addr, 12,1000);	
		return;
	}

}

void Get_MCUFW_Ver(void)
{
	unsigned char packet[MAX_FRAME_LEN+1];
	int p_len;
	if(MakeUartPacket(&packet,&p_len,WIFI_MODULE_ID,CONTROL_MCU_ID,MCU_VER_CMD,NULL,0))
	{
		hfuart_send(HFUART1, packet, p_len,1000);	
	}	
	
	return 1;	
}


int Mcu_upgarde_cmd(void)
{
	unsigned char packet[MAX_FRAME_LEN+1];
	int p_len;
	if(MakeUartPacket(&packet,&p_len,WIFI_MODULE_ID,CONTROL_MCU_ID,MCU_FW_UPGRADE_CMD,NULL,0))
	{
		hfuart_send(HFUART1, packet, p_len,1000);	
	}	
	
	return 1;	
}



int Atcmd_action(unsigned char src,unsigned char *payload,int len)
{
	char rsp[255];
	unsigned char packet[MAX_FRAME_LEN+1];
	int p_len;
	hfuart_handle_t handle;
	
	//hfuart_send(HFUART0, payload, len,1000);	//Send back AT cmd to debug
	if(src == BLUETOOTH_ID ) handle =HFUART0;	
	if(src == CONTROL_MCU_ID ) handle =HFUART1;	
	if(hfat_send_cmd(payload,len,rsp,255) == HF_SUCCESS )
	{
		//Send Rsp Back

		if(MakeUartPacket(&packet,&p_len,0x02,src,0x02,rsp,strlen(rsp)))
		{
			hfuart_send(handle, packet, p_len,1000);	
		}
	}
	else
	{
		//Send F0 Error
		rsp[0]=0x01;
		if(MakeUartPacket(&packet,&p_len,0x02,src,0xF0,rsp,1))
		{
			hfuart_send(handle, packet, p_len,1000);	
		}		
	}
	
	
	return 1;
}

int Reset_action(unsigned char src,unsigned char *payload,int len)
{
	//hfuart_send(HFUART0, payload, len,1000);	
	
	return 1;
}

int DeviceID_action(unsigned char src,unsigned char *payload,int len)
{
	//hfuart_send(HFUART0, payload, len,1000);	
	unsigned char packet[MAX_FRAME_LEN+1];
	int p_len;
	if(MakeUartPacket(&packet,&p_len,0x02,src,DEVICE_ID_CMD,g_mac_addr,12))
	{
		hfuart_send(HFUART0, packet, p_len,1000);	
	}	
	
	return 1;
}

int SmartLink_action(unsigned char src,unsigned char *payload,int len)
{
	char rsp[255];
	unsigned char packet[MAX_FRAME_LEN+1];
	int p_len;
	hfuart_handle_t handle;
	unsigned char on;

	on = *payload;
	
	//hfuart_send(HFUART0, payload, len,1000);	//Send back AT cmd to debug
	if(src == BLUETOOTH_ID ) handle =HFUART0;	
	if(src == CONTROL_MCU_ID ) handle =HFUART1;	

	if(on)
	{
		if(hfsmtlk_start() == HF_SUCCESS)
		{
			rsp[0]=0x00;
		}
		else
		{
				//Send F0 Error
				rsp[0]=0x01;
	
		}
	}
	else
	{
		if(hfsmtlk_stop() == HF_SUCCESS)
		{
			rsp[0]=0x00;
		}
		else
		{
			//Send F0 Error
			rsp[0]=0x01;
		
		}		
	}
	if(MakeUartPacket(&packet,&p_len,0x02,src,0xF0,rsp,1))
	{
		hfuart_send(handle, packet, p_len,1000);	
	}	

	return 1;
}


int Onoff_action(unsigned char src,unsigned char *payload,int len)
{
	//hfuart_send(HFUART0, payload, len,1000);	
	return 1;
}

int Brightness_action(unsigned char src,unsigned char *payload,int len)
{
	//hfuart_send(HFUART0, payload, len,1000);	
	return 1;
}

int Error_report_action(unsigned char src,unsigned char *payload,int len)
{
	return 1;
}

int Onoff_report_action(unsigned char src,unsigned char *payload,int len)
{
	//
	uint32_t seq;
	unsigned char v_hw_onoff;
	if(payload !=NULL)
	{
		g_light_onoff = *payload;
		payload++;
		v_hw_onoff = *payload;
		//upload data
		if(qlcloud_is_connected)
		{
		    dp_up_add_int(DP_ID_DP_SWITCH,g_light_onoff);
		    qlcloud_upload_dps(&seq);	

			if(v_hw_onoff == 0x00)
			{
			    dp_up_add_int(DP_ID_ONOFF_OWNER,g_light_onoff);
			    qlcloud_upload_dps(&seq);				
			}
		}
	}
	
	return 1;
}

int Brightness_report_action(unsigned char src,unsigned char *payload,int len)
{
	uint32_t seq;
	if(payload !=NULL)
	{
		g_light_brightness = *payload;
		//upload data
		if(qlcloud_is_connected)
		{
		    dp_up_add_int(DP_ID_BRIGHTNESS,g_light_brightness);
		    qlcloud_upload_dps(&seq);
		}
	}
	return 1;
}
int ColorTempReportAction(unsigned char src,unsigned char *payload,int len)
{
	uint32_t seq;
	if(payload !=NULL)
	{
		GLightColorTemp = *payload;
		//upload data
		if(qlcloud_is_connected)
		{
		    dp_up_add_int(DP_ID_COLOR_TEMP,GLightColorTemp);
		    qlcloud_upload_dps(&seq);
		}
	}
	return 1;
}

int ColorReportAction(unsigned char src,unsigned char *payload,int len)
{
	uint32_t seq;
	if(payload !=NULL)
	{
		GLightColor = *payload;
		//upload data
		if(qlcloud_is_connected)
		{
		    dp_up_add_int(DP_ID_COLOR,GLightColor);
		    qlcloud_upload_dps(&seq);
		}
	}
	return 1;
}
int Onoff_Owner_action(unsigned char src,unsigned char *payload,int len)
{
	uint32_t seq;
	unsigned char v_owner;
	if(payload !=NULL)
	{
		v_owner = *payload;
		//upload data
		if(qlcloud_is_connected)
		{
		    dp_up_add_int(DP_ID_ONOFF_OWNER,v_owner);
		    qlcloud_upload_dps(&seq);
		}
	}
	return 1;
}
extern char MCU_VERSION[5];
int Mcu_ver_action(unsigned char src,unsigned char *payload,int len)
{
	int i;
	if(payload == NULL) return 0;
	if(len !=5 ) return 0;
	for(i=0;i<len;i++)
	{
		MCU_VERSION[i]=*payload++;
	}
	
	return 1;
}


int Mcu_upgarde_action(unsigned char src,unsigned char *payload,int len)
{

	mcu_upgrade_start();
	return 1;	
}


//串口0 连接Bluetooth
uart_cmd_action_t uart0_action[]=
{
	{AT_CMD,Atcmd_action},
	{RESET_WIFI_CMD,Reset_action},
	{DEVICE_ID_CMD,DeviceID_action},
	{SMARTLINK_CMD,SmartLink_action},
	{ONOFF_CMD,Onoff_action},
	{BRIGHTNESS_CMD,Brightness_action},
	{0xFF,NULL}
};

//串口1 连接Control MCU
uart_cmd_action_t uart1_action[]=
{
	{AT_CMD,Atcmd_action},
	{RESET_WIFI_CMD,Reset_action},
	{SMARTLINK_CMD,SmartLink_action},	
	{ERROR_REPORT_CMD,Error_report_action},
	{ONOFF_REPORT_CMD,Onoff_report_action},
	{BRIGHTNESS_REPORT_CMD,Brightness_report_action},
	{ONOFF_OWNER_CMD,Onoff_Owner_action},
	{MCU_VER_CMD,Mcu_ver_action},
	{MCU_FW_UPGRADE_CMD,Mcu_upgarde_action},
	{COLOR_REPORT_CMD,ColorReportAction},
	{COLORTEMP_REPORT_CMD,ColorTempReportAction},	
	{0xFF,NULL}
};


int MakeUartPacket(unsigned char *packet,int *len,unsigned char src,unsigned char dest,unsigned char cmd,char *payload,int plen)
{
	int pk_len=0,i;
	unsigned char checksum=0;
	
	if(packet == NULL) return 0;
	//header
	*packet++=0xf1;
	*packet++=0xf2;
	*packet++ = plen + 8;  //length

	checksum += plen+8;
	*packet++ = src;
	*packet++ = dest;
	*packet++ = cmd;
	checksum +=src;
	checksum +=dest;
	checksum +=cmd;

	
	for(i=0;i<plen;i++)
	{
		*packet++ = *payload;
		checksum += *payload++;
	}
	*packet++=checksum;
	*packet = 0xF9;

	*len = plen + 8;
	return 1;
	
}

void Frame_process(uart_buffer_t *Q,unsigned char *data,int len)
{
	unsigned char cmd,i,src;
	src = data[3];
	if (data[4]==CONTROL_MCU_ID ) //to Control
	{
		hfuart_send(HFUART1, data, len,1000);	
	}
	else if(data[4]== WIFI_MODULE_ID) //to WiFi
	{
		cmd = data[5];
		if(Q == &g_uart0)
		{
			//from bt
			for(i=0;;i++)
			{
				if(uart0_action[i].cmd == 0xFF) return;
				if(uart0_action[i].cmd == cmd )
				{
					if(uart0_action[i].action)
						uart0_action[i].action(src,&data[6],len-8);
					return;
				}
			}
			
		}
		else if(Q == &g_uart1)
		{
				//from control mcu
			for(i=0;;i++)
			{
				if(uart1_action[i].cmd == 0xFF) return;
				if(uart1_action[i].cmd == cmd )
				{
					if(uart1_action[i].action)
						uart1_action[i].action(src,&data[6],len-8);
					return;
				}			
			}

		}
	}
	else if(data[4]==BLUETOOTH_ID)
	{
		hfuart_send(HFUART0, data, len,1000);		
	}

}

/**************************************************************
int GetUartFrame(uart_buffer_t *Q,int *start,int *len)
从uart数据里面找一个完整的Frame
Q uart Q
start Frame的起始位置
len Frame的长度
return 值
0  没找到
1  找到
HEADER(2Bytes)	LENGTH(1BYTE)	SRC(1BYTE)	DEST(1BYTE)	CMD(1BYTE)	PAYLOAD	CHECKSUM(1BYTE)	END
HEADER 0xF1 0xF2
LENGTH: 整个数据包的长度包含HEADER 
0x04 -0xEF 之间
END 0xF9

***************************************************************/

int ParaseUartFrame(uart_buffer_t *Q)
{
	int i,frame_len=0;
	int header=0,start;
	unsigned char checksum=0;
	unsigned char data[MAX_FRAME_LEN];
	if(Q == NULL) return 0;

	if(Q->r_index == Q->w_index ) return 0;
	
	i=Q->r_index;

	for(;;)
	{
		if(i == MAX_BUFF_LEN )
		{
					if(Q->buffer[i]==0xF1 && Q->buffer[0]==0xF2)
					{
						start = i;
						header = 1;
						i=0;
					}
		}	
		else
		{
			if(Q->buffer[i]==0xF1 && Q->buffer[i+1]==0xF2)
			{
				start = i;
				i++;
				header = 1;
			}
		}
		if( i== Q->w_index)
		{
			return 0;
		}		
		i++;
		if(i> MAX_BUFF_LEN ) i=0;

		if( i== Q->w_index)
		{
			return 0;
		}

		if( header == 1)
		{
			frame_len = Q->buffer[i];

			if(frame_len >= 0xF0 || frame_len < 0x08) 
			{
				header = 0;
				continue;
			}

			if(frame_len > Get_Uart_bufferlen(Q))
			{
				return 0;
			}
			break;
		}
	}

	//
	if (Pop_data(Q, start,data,frame_len,1)==0 ) return 0;
	if(data[frame_len -1] != 0xF9) // 错误
	{	
		mydebug("error tail");
		Pop_data(Q, start,data,2,0);	//丢掉这个错误的头
		return 1;
	}
	for(i=2;i<frame_len-2;i++)
	{	
		checksum += data[i];
	}
	if( checksum == data[i]) //checksum ok
	{
		if(Pop_data(Q, start,data,frame_len,0))
		{
			//
			Frame_process(Q,data,frame_len);	
			return 1;
		}
	}
	else
	{
		//checksum error;
		//丢掉这个包
		mydebug("error checksum");
		
		Pop_data(Q, start,data,frame_len,0);
		return 1;
	}

	return 0;
}
/**************************************************************
void init_uart_buffer(void)

***************************************************************/

void init_uart_buffer(void)
{
	memset(g_uart0.buffer,0,sizeof(g_uart0.buffer));
	memset(g_uart1.buffer,0,sizeof(g_uart1.buffer));
	
	g_uart0.r_index=0;
	g_uart0.w_index=0;
	g_uart1.r_index=0;
	g_uart1.w_index=0;
}
/**************************************************************
int Get_Uart_bufferlen(uart_buffer_t *Q)

***************************************************************/
int Get_Uart_bufferlen(uart_buffer_t *Q)
{
	int len;
	if(Q==NULL) return 0;
	if(Q->r_index>Q->w_index)
	{
		len = MAX_BUFF_LEN - Q->r_index + 1  + Q->w_index;
	}
	else
	{
		len = Q->w_index -Q->r_index;
	}
	return len;
}


/**************************************************************
uart_buffer_t *Q  
char *data     
uint32 len the length of data
***************************************************************/
void Push_data(uart_buffer_t *Q,unsigned char *data,int len)
{
	int i;
	if(Q == NULL) return;
	for(i=0;i<len;i++)
	{	
		if(Q->w_index >= MAX_BUFF_LEN)
		{
			Q->buffer[Q->w_index]=*data++;
			Q->w_index = 0;
		}
		else
		{
			Q->buffer[Q->w_index++]=*data++;
		}
		if(Q->w_index == Q->r_index) 
		{
			Q->r_index +=1;//r=w Q is NULL r<>w Q has data
			if(Q->r_index > MAX_BUFF_LEN)
				Q->r_index = 0;
		}
	}
}

/********************************************************************
uart_buffer_t *Q  
int start 起始位置
char *buff     
uint32 len the length of data
int flag
	flag 0 remove the data
	flag 1 keep the data
*********************************************************************/
int Pop_data(uart_buffer_t *Q,int start,unsigned char *buff,int len,int flag)
{
	int i,v_r_index,v_len;

	if(Q == NULL ) return 0;
	if(buff ==NULL) return 0;

	if(Q->w_index == Q->r_index ) return 0;
	if(Q->w_index > Q->r_index)
	{
		if(start< Q->r_index || start >= Q->w_index) return 0;
		v_len = Q->w_index- start;
	}
	else
	{
		if(start >= Q->w_index && start <Q->r_index) return 0;
		if( start > Q->w_index )
			v_len = MAX_BUFF_LEN - start + 1 + Q->w_index;
		else
			v_len = Q->w_index -start;
	}
	
	if(v_len < len ) return 0;
	
	v_r_index = start;
	
	for(i=0;i<len;i++)
	{
		*buff++ = Q->buffer[v_r_index++];
		if(v_r_index > MAX_BUFF_LEN)
		{
			v_r_index = 0;
			
		}
	}
	if(flag==0) Q->r_index = v_r_index;
	return 1;


}

void Turn_Light_Onoff(unsigned char on)
{
	unsigned char packet[MAX_FRAME_LEN+1];
	int p_len;
	unsigned char payload[1];
	payload[0]=0x01;

	if(on)
	{
		//Turn on		
		payload[0]=0x01;
	}
	else
	{
		//Turn off
		payload[0]=0x00;
	}
	MakeUartPacket(&packet,&p_len,WIFI_MODULE_ID,CONTROL_MCU_ID,ONOFF_CMD,payload,1);
	hfuart_send(HFUART1, packet, p_len,1000);	
}

void Set_Light_Brightness(unsigned char brightness)
{
	unsigned char packet[MAX_FRAME_LEN+1];
	int p_len;
	unsigned char payload[1];

	if(brightness>100) brightness = 100;
	
	payload[0]=brightness;

	MakeUartPacket(&packet,&p_len,WIFI_MODULE_ID,CONTROL_MCU_ID,BRIGHTNESS_CMD,payload,1);
	hfuart_send(HFUART1, packet, p_len,1000);
}

void SetLightColor(unsigned char Color)
{
	unsigned char packet[MAX_FRAME_LEN+1];
	int p_len;
	unsigned char payload[1];

	if(Color>100) Color = 100;
	
	payload[0]=Color;

	MakeUartPacket(&packet,&p_len,WIFI_MODULE_ID,CONTROL_MCU_ID,COLOR_CMD,payload,1);
	hfuart_send(HFUART1, packet, p_len,1000);	
}

void SetLightColorTemp(unsigned char ColorTemp)
{
	unsigned char packet[MAX_FRAME_LEN+1];
	int p_len;
	unsigned char payload[1];

//	if(ColorTemp>100) ColorTemp = 100;	//0-100 暖光 101-200 冷光
	
	payload[0] = ColorTemp;

	MakeUartPacket(&packet,&p_len,WIFI_MODULE_ID,CONTROL_MCU_ID,COLORTEMP_CMD,payload,1);
	hfuart_send(HFUART1, packet, p_len,1000);	
}




int USER_FUNC uart0_recv_callback(uint32_t event,char *data,uint32_t len,uint32_t buf_len)
{
/*	int check=1;
	Push_data(&g_uart0,data, len);
	while(check)
	{
		check = ParaseUartFrame(&g_uart0);
	}*/
	return 0;
}

int USER_FUNC uart1_recv_callback(uint32_t event,char *data,uint32_t len,uint32_t buf_len)
{
	//hfuart_send(HFUART1, data, len,1000);	
	int check=1;

	Push_data(&g_uart1,data, len);
	while(check)
	{
		check = ParaseUartFrame(&g_uart1);
	}

	return 0;
}


