#ifndef _QLY_DEMO_H_
#define _QLY_DEMO_H_

#include "qlcloud_interface.h"

int dev_light_bright(int brightness);
int dev_light_control(qly_u8 dp_switch);
int DevLightColor(int Color);			//增加声明
int DevLightColorTemp(int ColorTemp);
void qly_init(void);
void smart_light_start(void);
#endif