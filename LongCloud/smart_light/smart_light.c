#include "hsf.h"
#include "process.h"
#include "smart_light.h"
#include "clock.h"
#include "uart_protocal.h"
#include "qlcloud_interface.h"

int dev_light_bright(int brightness)
{
    //pls control dev according to the brightness,range: 0-100

	Set_Light_Brightness(brightness);

    return 0;
}

int dev_light_control(qly_u8 dp_switch)
{
    //pls control dev switch
    // dp_switch == 1 ==>on
    // dp_switch == 0 ==>off
    Turn_Light_Onoff(dp_switch);
    
	return 0;
}

int DevLightColor(int Color)
{
	SetLightColor(Color);
	return 0;
}

int DevLightColorTemp(int ColorTemp)
{
	SetLightColorTemp(ColorTemp);
	return 0;
}
//debug test
//#define PRODUCTID  1000187812
#define PRODUCTID  1003873988
//const char PRODUCTKEY[] ={0x69,0x69,0x21,0xbb,0x30,0x6d,0x85,0x22,0x64,0x33,0x6c,0x51,0x29,0x5a,0x6e,0xb9};
const char PRODUCTKEY[] ={0x51,0x33,0x99,0x7a,0x76,0x61,0x63,0x81,0x28,0x22,0x38,0x21,0x4f,0x67,0x6f,0x6d};

char MCU_VERSION[5]=  "01.10";	//云平台会显示此版本号

void qly_init(void)
{
	qlcloud_wifi_version_set("02.06");	//云端显示并且此版本号做为升级依据
    qlcloud_initialization(PRODUCTID, PRODUCTKEY, MCU_VERSION, 4096, 4096, 0);
    qlcloud_ota_option_set(121,MAX_MCUFW_BUFF_SIZE);	//修改一次传递4K数据
}

PROCESS(smart_light_process, "smart_light_process");
extern void mydebug(char *data);
PROCESS_THREAD(smart_light_process, ev, data)
{
	PROCESS_BEGIN();
    //demo timer
    static struct etimer timer_sleep;
	static unsigned char Flag = 0;
	etimer_set(&timer_sleep, 3 * CLOCK_SECOND);
	hfgpio_configure_fpin(HFM_GPIO8,HFM_IO_OUTPUT_1|HFM_IO_OUTPUT_0);
	while(1)
	{
		PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER ||ev ==PROCESS_EVENT_CONTINUE);
        if(ev == PROCESS_EVENT_TIMER)
        {
            //demo printf
            u_printf("..\n");
            etimer_set(&timer_sleep, 3 * CLOCK_SECOND);
        }
		//mydebug("Hello world!\n");	//串口0，就是下载用的那个串口打印调试
		if(Flag == 1)
		{
			Flag = 0;
			hfgpio_set_out_high(HFM_GPIO8);
		}
		else
		{
			Flag = 1;
			hfgpio_set_out_low(HFM_GPIO8);
		}
	}

	PROCESS_END();
}


void smart_light_start(void)
{
    process_start(&smart_light_process, NULL);
}

