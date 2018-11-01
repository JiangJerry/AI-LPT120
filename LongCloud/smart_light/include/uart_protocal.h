#ifndef __UART_PROTOCAL_H__
#define __UART_PROTOCAL_H__

#define MAX_BUFF_LEN  255
#define MAX_FRAME_LEN 0xF0




typedef struct 
{
	unsigned char buffer[MAX_BUFF_LEN+1];
	int  r_index;
	int  w_index;
} uart_buffer_t;


#endif

typedef struct 
{
	unsigned char cmd;
	int (*action)(unsigned char src,unsigned char *payload,int len);
} uart_cmd_action_t;

enum UARTCMD
{
	AT_CMD=0x01,
	AT_CMD_RSP,
	RESET_WIFI_CMD=0x04,
	DEVICE_ID_CMD,
	SMARTLINK_CMD,
	MCU_VER_CMD,
	MCU_FW_UPGRADE_CMD,	
	ONOFF_CMD=0x20,
	BRIGHTNESS_CMD,
	ERROR_REPORT_CMD,
	ONOFF_REPORT_CMD,
	BRIGHTNESS_REPORT_CMD,
	ONOFF_OWNER_CMD=0x26,
	COLOR_CMD,
	COLORTEMP_CMD,
	COLOR_REPORT_CMD,
	COLORTEMP_REPORT_CMD,
	RESPONSE_CMD=0xF0,
};

#define BLUETOOTH_ID 0x01
#define WIFI_MODULE_ID 0x02
#define CONTROL_MCU_ID 0x03


int Get_Uart_bufferlen(uart_buffer_t *Q);
void init_uart_buffer(void);
void Push_data(uart_buffer_t *Q,unsigned char *data,int len);
int Pop_data(uart_buffer_t *Q,int start,unsigned char *buff,int len,int flag);



int USER_FUNC uart0_recv_callback(uint32_t event,char *data,uint32_t len,uint32_t buf_len);
int USER_FUNC uart1_recv_callback(uint32_t event,char *data,uint32_t len,uint32_t buf_len);
int MakeUartPacket(unsigned char *packet,int *len,unsigned char src,unsigned char dest,unsigned char cmd,char *payload,int plen);
void Set_Uart_Port(void);
void Turn_Light_Onoff(unsigned char on);
void Set_Light_Brightness(unsigned char brightness);
extern void SetLightColorTemp(unsigned char ColorTemp);//增加串口协议
extern void SetLightColor(unsigned char Color);
void Get_Mac_Addr(void);
void Get_MCUFW_Ver(void);
int Mcu_upgarde_cmd(void);

extern unsigned char g_light_onoff;
extern unsigned char g_light_brightness;
extern int qlcloud_is_connected;
extern unsigned int GLightColor;
extern unsigned char GLightColorTemp;
