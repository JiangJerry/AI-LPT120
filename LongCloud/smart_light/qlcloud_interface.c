#include <hsf.h>
#include "qlcloud_interface.h"
#include "smart_light.h"
#include "uart_protocal.h"
#include "xmodem.h"

int qlcloud_is_connected = 0;

//  ��һ���� [ϵͳ����]  /////////////////////////////////////////////////////

/*
 * ���豸����״̬�����ı�ʱ��sdk�Զ����ô˺���
 */
void qlcloud_status_cb( DEV_STATUS_T dev_status, qly_u32 timestamp )
{
    u_printf("device state:%d, ts:%d\r\n", dev_status, timestamp);

    if(dev_status == 0)//�����ƶ˳ɹ�
    {
		uint32_t seq;
        qlcloud_is_connected = 1;
        //After connecting to the cloud, upload the device status. dp_switch,brightness and time_cfg
	    dp_up_add_int(DP_ID_BRIGHTNESS,g_light_brightness);	//ע�������ݵ����������ݴ洢����
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

//  ��һ���ֽ���  ////////////////////////////////////////////////////////////


/*##########################################################################*/


//  �ڶ����� [��������]  /////////////////////////////////////////////////////

/*
 * �ϴ����ݳɹ���sdk�Զ�������������ص�����
 */
void qlcloud_upload_dps_cb ( qly_u32 data_seq )
{
    u_printf("qlcloud_upload_dps_cb:%d.\r\n", data_seq);
}

//  ���·����ݵ㴦�������û��밴������ʵ��  /////////////////////////////////

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
void dp_down_handle_color( qly_u8* in_data, qly_u32 in_len )//������ɫ����
{
    int Color = bytes_to_int(in_data);
    u_printf("Color:%d\r\n", Color);

    DevLightColor(Color);
}
void dp_down_handle_color_temp( qly_u8* in_data, qly_u32 in_len )//����ɫ��
{

    int ColorTemp = bytes_to_int(in_data);
    u_printf("ColorTemp:%d\r\n", ColorTemp);

    DevLightColorTemp(ColorTemp);
}

//  �·����ݵ㴦��������  ////////////////////////////////////////////////////


//  �·����ݵ����ݽṹ����  ////////////////////////////////////////////////////

/*****************************************************************************
�ṹ���� : �·����ݵ�ṹ������
�������� : �û��뽫��Ҫ������·����ݵ㣬�����ݵ�ID�����ݵ����͡�
           ���ݵ��Ӧ��������д����������飬����������ʵ��
*****************************************************************************/
qlcloud_download_dps_t qlcloud_down_dps[] =
{   /*  ���ݵ�ID     , ���ݵ�����  ,   ������               */
    { DP_ID_DP_SWITCH,  DP_TYPE_BOOL,   dp_down_handle_switch     },
    { DP_ID_BRIGHTNESS, DP_TYPE_INT,    dp_down_handle_brightness },
	{ DP_ID_COLOR,  	DP_TYPE_INT,    dp_down_handle_color     },	//20181015׷��
    { DP_ID_COLOR_TEMP, DP_TYPE_INT,    dp_down_handle_color_temp }	

};
/*****************************************************************************
�ṹ���� : DOWN_DPS_CNT����
�������� : �·����ݵ�ṹ�����������ݵ��������û��������
*****************************************************************************/
qly_u32 DOWN_DPS_CNT = sizeof(qlcloud_down_dps)/sizeof(qlcloud_download_dps_t);

//  �·����ݵ����ݽṹ�������  /////////////////////////////////////////////////


//  �ڶ����ֽ���  ////////////////////////////////////////////////////////////




/*##########################################################################*/


//  �������� [�̼�����]  /////////////////////////////////////////////////////

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

	//�յ��̼���������ȴ洢��WIFI Flash�ռ䡣
	//�ܿռ�120K
	//0~4095 ����ܳ���  
	//4096~ �̼�  116K ��󳤶�
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
     *   �����յ��Ĺ̼����ݿ飬���û���������ʵ��
    */
    return ACK_OK;
}

void qlcloud_ota_upgrade_cb ( void )
{
    u_printf("get ota cmd success,reboot right now\r\n");

    /*
     *  �����յ�������ָ��������������°汾�̼�
    */

	//First Send Upgrade CMD
	Mcu_upgarde_cmd();
	
}

//  �������ֽ���  ////////////////////////////////////////////////////////////


/*##########################################################################*/


//  ���Ĳ��� [�߼�����]  /////////////////////////////////////////////////////

void qlcloud_tx_data_cb ( qly_u32 data_seq )
{
    u_printf("qlcloud_tx_data_cb :%d.\r\n", data_seq);
}

void qlcloud_rx_data_cb ( qly_u32 data_timestamp, qly_u8* data, qly_u32 data_len )
{
    u_printf("qlcloud_rx_data_cb :%d.\r\ndata[%d]:%s\r\n", data_timestamp, data_len, data);
}

//  ���Ĳ��ֽ���  ////////////////////////////////////////////////////////////

//end qlcloud_interface.c

