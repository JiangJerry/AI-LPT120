#include <hsf.h>
#include "qlcloud_interface.h"
#include "smart_light.h"
#include "uart_protocal.h"
#include "xmodem.h"

int qlcloud_is_connected = 0;

//  第一部分 [系统函数]  /////////////////////////////////////////////////////

/*
 * 当设备连接状态发生改变时，sdk自动调用此函数
 */
void qlcloud_status_cb( DEV_STATUS_T dev_status, qly_u32 timestamp )
{
    u_printf("device state:%d, ts:%d\r\n", dev_status, timestamp);

    if(dev_status == 0)//连上云端成功
    {
		uint32_t seq;
        qlcloud_is_connected = 1;
        //After connecting to the cloud, upload the device status. dp_switch,brightness and time_cfg
	    dp_up_add_int(DP_ID_BRIGHTNESS,g_light_brightness);	//注册云数据点索引和数据存储变量
	    dp_up_add_int(DP_ID_DP_SWITCH,g_light_onoff);
	    dp_up_add_int(DP_ID_COLOR,GLightColor);
	    dp_up_add_int(DP_ID_COLOR_TEMP,GLightColorTemp);
	    qlcloud_upload_dps(&seq);
	}
    else
    {
        qlcloud_is_connected = 0;
    }
}

//  第一部分结束  ////////////////////////////////////////////////////////////


/*##########################################################################*/


//  第二部分 [传输数据]  /////////////////////////////////////////////////////

/*
 * 上传数据成功后，sdk自动调用下面这个回调函数
 */
void qlcloud_upload_dps_cb ( qly_u32 data_seq )
{
    u_printf("qlcloud_upload_dps_cb:%d.\r\n", data_seq);
}

//  各下发数据点处理函数，用户请按需自行实现  /////////////////////////////////

void dp_down_handle_switch ( qly_u8* in_data, qly_u32 in_len )
{
    qly_u8 dp_switch  = bytes_to_bool(in_data);

    u_printf("dp_switch:%d\r\n", dp_switch);

    //control dev
    dev_light_control(dp_switch);

    //upload status
//    uint32_t seq;
//    dp_up_add_bool(DP_ID_DP_SWITCH,dp_switch);
//    qlcloud_upload_dps(&seq);
}

void dp_down_handle_brightness( qly_u8* in_data, qly_u32 in_len )
{

    int brightness = bytes_to_int(in_data);
    u_printf("brightness:%d\r\n", brightness);
    //control dev
    dev_light_bright(brightness);

    //upload brightness
//    uint32_t seq;
//    dp_up_add_int(DP_ID_BRIGHTNESS,brightness);
//    qlcloud_upload_dps(&seq);
}
void dp_down_handle_color( qly_u8* in_data, qly_u32 in_len )//处理颜色命令
{
    int Color = bytes_to_int(in_data);
    u_printf("Color:%d\r\n", Color);

    DevLightColor(Color);
}
void dp_down_handle_color_temp( qly_u8* in_data, qly_u32 in_len )//处理色温
{

    int ColorTemp = bytes_to_int(in_data);
    u_printf("ColorTemp:%d\r\n", ColorTemp);

    DevLightColorTemp(ColorTemp);
}

//  下发数据点处理函数结束  ////////////////////////////////////////////////////


//  下发数据点数据结构定义  ////////////////////////////////////////////////////

/*****************************************************************************
结构名称 : 下发数据点结构体数组
功能描述 : 用户请将需要处理的下发数据点，按数据点ID、数据点类型、
           数据点对应处理函数填写下面这个数组，并将处理函数实现
*****************************************************************************/
qlcloud_download_dps_t qlcloud_down_dps[] =
{   /*  数据点ID     , 数据点类型  ,   处理函数               */
    { DP_ID_DP_SWITCH,  DP_TYPE_BOOL,   dp_down_handle_switch     },
    { DP_ID_BRIGHTNESS, DP_TYPE_INT,    dp_down_handle_brightness },
	{ DP_ID_COLOR,  	DP_TYPE_INT,    dp_down_handle_color     },	//20181015追加
    { DP_ID_COLOR_TEMP, DP_TYPE_INT,    dp_down_handle_color_temp }	

};
/*****************************************************************************
结构名称 : DOWN_DPS_CNT整数
功能描述 : 下发数据点结构体数组中数据点数量，用户请勿更改
*****************************************************************************/
qly_u32 DOWN_DPS_CNT = sizeof(qlcloud_down_dps)/sizeof(qlcloud_download_dps_t);

//  下发数据点数据结构定义结束  /////////////////////////////////////////////////


//  第二部分结束  ////////////////////////////////////////////////////////////




/*##########################################################################*/


//  第三部分 [固件升级]  /////////////////////////////////////////////////////

#define MCU_START_ADDR 4096
unsigned char *g_mcufw_ptr=NULL;
unsigned char *g_cur_ptr=NULL;
qly_u32 g_v_count=0;
qly_u32 g_mcufw_size=0;


qly_s32 qlcloud_ota_chunk_cb ( qly_u8  chunk_is_last, qly_u32 chunk_offset, qly_u32 chunk_size, const qly_s8*  chunk )
{
	qly_s32 i;
	int ret;
	u_printf("get chunk offset = %d, size = %d\r\n", chunk_offset, chunk_size);

    //pls save or transport chunk

	//收到固件升级块后，先存储在WIFI Flash空间。
	//总空间120K
	//0~4095 存放总长度  
	//4096~ 固件  116K 最大长度
	//int HSF_API hfuflash_erase_page(uint32_t addr,int pages)
	//int HSF_API hfuflash_read(uint32_t addr,char *data,int len)
	//int HSF_API hfuflash_write(uint32_t addr,char *data,int len)
	
	if(g_mcufw_ptr == NULL)
	{
		g_mcufw_ptr = hfmem_malloc(MAX_MCUFW_BUFF_SIZE+1);
		if(g_mcufw_ptr == NULL)
		{
			hfuart_send(HFUART1, "1111", 4,1000);	
			return ACK_ERR;
		}		
		g_cur_ptr = g_mcufw_ptr;
		g_v_count=0;
	}

	for(i=0;i<chunk_size;i++)
	{
		*g_cur_ptr++=*chunk++;
		if((g_cur_ptr - g_mcufw_ptr) == 4096)
		{
			ret = hfuflash_erase_page(MCU_START_ADDR+g_v_count*4096,1);
			if(ret != HF_SUCCESS)
			{
				hfuart_send(HFUART1, "2222", 4,1000);	
				hfuart_send(HFUART1, &ret, 4,1000);	
			
				return ACK_ERR;
			}
			ret = hfuflash_write(MCU_START_ADDR+g_v_count*4096,g_mcufw_ptr,4096);
			if(ret != 4096)
			{
				hfuart_send(HFUART1, "3333", 4,1000);	
				hfuart_send(HFUART1, &ret, 4,1000);	
			
				return ACK_ERR;	
			}
			g_v_count++;
			g_cur_ptr = g_mcufw_ptr;			
		}
	}

	if(chunk_is_last)
	{
		if((g_cur_ptr-g_mcufw_ptr)>0)
		{
			ret = hfuflash_erase_page(MCU_START_ADDR+g_v_count*4096,1);
			if(ret != HF_SUCCESS)
			{
				hfuart_send(HFUART1, "4444", 4,1000);	
				hfuart_send(HFUART1, &ret, 4,1000);	
			
				return ACK_ERR;
			}
			ret = hfuflash_write(MCU_START_ADDR+g_v_count*4096,g_mcufw_ptr,g_cur_ptr-g_mcufw_ptr);
			
			if(ret != g_cur_ptr-g_mcufw_ptr)
			{
				hfuart_send(HFUART1, "5555", 4,1000);	
				hfuart_send(HFUART1, &ret, 4,1000);	
			
				return ACK_ERR;	
			}
		}
		hfmem_free(g_mcufw_ptr);
		g_cur_ptr=NULL;
		g_v_count=0;
		g_mcufw_size = chunk_offset+chunk_size;
		
		hfuart_send(HFUART1, &g_mcufw_size, 4,1000);
		ret = hfuflash_erase_page(0,1);
		if(ret != HF_SUCCESS)
		{
			hfuart_send(HFUART1, "6666", 4,1000);	
			hfuart_send(HFUART1, &ret, 4,1000); 
		
			return ACK_ERR;
		}
		ret = hfuflash_write(0,&g_mcufw_size,4);
		if(ret != 4)		//write size
		{
			hfuart_send(HFUART1, "7777", 4,1000);	
			hfuart_send(HFUART1, &ret, 4,1000); 
		
			return ACK_ERR;
		}
		
	}
	
    /*
     *   处理收到的固件数据块，请用户按需自行实现
    */
    return ACK_OK;
}

void qlcloud_ota_upgrade_cb ( void )
{
    u_printf("get ota cmd success,reboot right now\r\n");

    /*
     *  处理收到的升级指令，需重启，运行新版本固件
    */

	//First Send Upgrade CMD
	Mcu_upgarde_cmd();
	
}

//  第三部分结束  ////////////////////////////////////////////////////////////


/*##########################################################################*/


//  第四部分 [高级功能]  /////////////////////////////////////////////////////

void qlcloud_tx_data_cb ( qly_u32 data_seq )
{
    u_printf("qlcloud_tx_data_cb :%d.\r\n", data_seq);
}

void qlcloud_rx_data_cb ( qly_u32 data_timestamp, qly_u8* data, qly_u32 data_len )
{
    u_printf("qlcloud_rx_data_cb :%d.\r\ndata[%d]:%s\r\n", data_timestamp, data_len, data);
}

//  第四部分结束  ////////////////////////////////////////////////////////////

//end qlcloud_interface.c

