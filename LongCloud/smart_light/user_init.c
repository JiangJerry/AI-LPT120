#include "hsf.h"
#include "hfsys.h"
#include "smart_light.h"

int sys_event_callback( uint32_t event_id,void * param)
{
    static uint8_t is_setup_wifi_first_run = 0;
	switch(event_id)
	{
		case HFE_WIFI_STA_CONNECTED:
			printf("HFE_WIFI_STA_CONNECTED\n");
			break;
		case HFE_WIFI_STA_DISCONNECTED:
			printf("HFE_WIFI_STA_DISCONNECTED\n");
			break;
		case HFE_CONFIG_RELOAD:
			printf("HFE_CONFIG_RELOAD\n");
			break;
		case HFE_DHCP_OK:
			printf("HFE_DHCP_OK\n");
            if(is_setup_wifi_first_run == 0)
            {
                is_setup_wifi_first_run = 1;
                qly_init();	//连接上wifi后进入青莲云初始化
            }
			break;
		case HFE_SMTLK_OK:
			printf("HFE_SMTLK_OK\n");
			break;
	}
	return 0;
}

void user_init(void)
{
    u_printf("pls user_init\n");
    smart_light_start();

	hfsys_register_system_event(sys_event_callback);
}

