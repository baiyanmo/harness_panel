#ifndef	_BEEP_H_
#define	_BEEP_H_

#include "stm32f10x.h"

#define BEEP_OFF	GPIO_ResetBits(GPIOA,GPIO_Pin_6)
#define BEEP_ON		GPIO_SetBits(GPIOA,GPIO_Pin_6)

void BEEP_Config(void);//º¯ÊýµÄÉùÃ÷

#endif
