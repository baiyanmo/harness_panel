#ifndef	_DHT11_H_
#define _DHT11_H_

#include "stm32f10x.h"

#define DHT11_OUT 1
#define DHT11_IN 	0
#define DHT11_INPUT GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_0)

void DHT11_Config(u8 mode);
void GET_DHT11Data(void);

#endif