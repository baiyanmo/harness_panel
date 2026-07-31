#include "key.h"
#include "delay.h"

/*
函数功能：按键的初始化函数
（将PA0\PB8配置为推挽浮空输入）
*/
void KEY_Config(void)
{
	GPIO_InitTypeDef GPIO_KEY = {0};//用于配置GPIO口的结构体变量 
	//开时钟,第一个参数代表要开哪个模块的时钟，第二个参数代表使能（开）还是失能（关）
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB,ENABLE);
	//配置GPIO口（需要定义一个结构体变量，然后给结构体变量中的成员赋值）
	GPIO_KEY.GPIO_Mode = GPIO_Mode_IN_FLOATING;//模式配置为浮空输入
	GPIO_KEY.GPIO_Pin = GPIO_Pin_0;//管脚号(当要将同一个端口下的多个管脚配置为同一种工作模式是可以利用|同时选中多个管脚一起配置)
	//初始化GPIO口,第一个参数端口号，第二参数配置好的结构体变量的首地址
	GPIO_Init(GPIOA,&GPIO_KEY);
	
	GPIO_KEY.GPIO_Pin = GPIO_Pin_8;
	GPIO_Init(GPIOB,&GPIO_KEY);
}

/*
函数功能：检测哪个按键按下了
函数返回值：KEY1按下返回1；KEY2按下返回2
*/
u8 KEY_Scan(void)
{
	static uint8_t count1 = 0;
	static uint8_t count2 = 0;
	//KEY1的检测
	if(GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_0) == 1)
	{
		count1++;
	}
	else
	{
		if(count1 > 20)
		{
			count1 = 0;
			return 3;
		}
		else if(count1 > 3&&count1 <= 20)
		{
			count1 = 0;
			return 1;
		}
	}
	//KEY2的检测
	if(GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_8) == 0)
	{
		count2++;
	}
	else
	{
		if(count2 > 20)
		{
			count2 = 0;
			return 4;
		}
		else if(count2 > 3&&count2 <= 20)
		{
			count2 = 0;
			return 2;
		}
	}
	return 0;
}







