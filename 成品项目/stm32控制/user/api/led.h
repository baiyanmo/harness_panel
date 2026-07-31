#ifndef	_LED_H_
#define	_LED_H_

#include "stm32f10x.h"

#define LED1_ON		GPIO_ResetBits(GPIOC,GPIO_Pin_0)
#define LED1_OFF	GPIO_SetBits(GPIOC,GPIO_Pin_0)
#define LED2_ON		GPIO_ResetBits(GPIOC,GPIO_Pin_1)
#define LED2_OFF	GPIO_SetBits(GPIOC,GPIO_Pin_1)
#define LED3_ON		GPIO_ResetBits(GPIOC,GPIO_Pin_2)
#define LED3_OFF	GPIO_SetBits(GPIOC,GPIO_Pin_2)

void LED_Config(void);//º¯ÊýÉùÃ÷

#endif
