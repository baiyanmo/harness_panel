#include "beep.h"

/*
函数功能：蜂鸣器的初始化函数
（将PA6配置为推挽输出模式）
*/
void BEEP_Config(void)
{
	GPIO_InitTypeDef GPIO_BEEP = {0};//用于配置GPIO口的结构体变量 
	//开时钟,第一个参数代表要开哪个模块的时钟，第二个参数代表使能（开）还是失能（关）
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	//配置GPIO口（需要定义一个结构体变量，然后给结构体变量中的成员赋值）
	GPIO_BEEP.GPIO_Mode = GPIO_Mode_Out_PP;//模式配置为推挽输出
	GPIO_BEEP.GPIO_Pin = GPIO_Pin_6;//管脚号(当要将同一个端口下的多个管脚配置为同一种工作模式是可以利用|同时选中多个管脚一起配置)
	GPIO_BEEP.GPIO_Speed = GPIO_Speed_50MHz;//GPI口的输出速率（一般情况下直接选最大的即可）
	//初始化GPIO口,第一个参数端口号，第二参数配置好的结构体变量的首地址
	GPIO_Init(GPIOA,&GPIO_BEEP);
	//关闭蜂鸣器
	BEEP_OFF;
}
