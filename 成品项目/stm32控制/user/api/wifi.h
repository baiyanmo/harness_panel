#ifndef _WIFI_H_
#define _WIFI_H_

#include "stm32f10x.h"

typedef struct{
	u8 RX_buff[1024];
	u16 RX_count;
	u8 WIFI_RecFlag;
}WIFI_DataTypedef;

extern WIFI_DataTypedef WIFI_Data;

void usart2_config(uint32_t brr);
void wifi_SendStr(char *p);
void wifi_SendArray(u8 Arr[],u16 DataLenth);
#endif