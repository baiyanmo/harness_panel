#include "delay.h"

void SysTick_Handler(void)
{

}

/*
�������ܣ�΢�뼶����ʱ
����������Ҫ��ʱ����΢�� 
*/
void Delay_us(uint32_t time)
{
	while(time--)
		delay_1us();
}

/*
�������ܣ����뼶����ʱ
����������Ҫ��ʱ���ٺ���
*/
void Delay_ms(uint32_t time)
{
	uint64_t ms = time*1000;
	while(ms--)
	{
		delay_1us();
	}
}




