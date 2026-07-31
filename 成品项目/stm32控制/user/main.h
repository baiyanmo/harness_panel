#ifndef _MAIN_H_
#define _MAIN_H_

#include "stm32f10x.h"
#include "stdio.h"
#include "string.h"
#include "delay.h"
#include "led.h"
#include "wifi.h"
#include "usart1.h"
#include "sensor.h"

typedef struct{
	char name[20];
	char date[20];
	char text_day[20];
	char code_day[20];
	char text_night[20];
	char code_night[20];
	char high[20];
	char low[20];
	char humidity[20];
}Weather_DataTypedef;

//�ر�JTAG���ܣ���SWD����
inline void JTAG_SWD_Config(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);
	
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable,ENABLE);
}

#endif
