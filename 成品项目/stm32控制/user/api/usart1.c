#include "usart1.h"

/*
函数功能：串口一的初始化函数
函数参数：串口一的波特率
PA9--USRAT1_TX   PA10--USRAT1_RX
*/
void USART1_Config(uint32_t brr)
{
	GPIO_InitTypeDef GPIO_USART1 = {0};//用于配置GPIO口的结构体变量 
	USART_InitTypeDef USART_1 = {0};//用于配置串口一的结构体变量
	//开时钟(GPIOA、USART1)
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_USART1,ENABLE);
	//配置并初始化PA9（复用推挽）
	GPIO_USART1.GPIO_Mode = GPIO_Mode_AF_PP;//模式配置为复用推挽
	GPIO_USART1.GPIO_Pin = GPIO_Pin_9;//9号管脚号
	GPIO_USART1.GPIO_Speed = GPIO_Speed_50MHz;//GPIO口的输出速率
	GPIO_Init(GPIOA,&GPIO_USART1);
	//配置并初始化PA10（复用推挽）
	GPIO_USART1.GPIO_Mode = GPIO_Mode_IN_FLOATING;//模式配置为浮空输入
	GPIO_USART1.GPIO_Pin = GPIO_Pin_10;//10号管脚号
	GPIO_Init(GPIOA,&GPIO_USART1);
	//配置串口
	USART_1.USART_BaudRate = brr;//波特率
	USART_1.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//使用软件（代码）控制串口的发送和接受，所以不使用硬件流控
	USART_1.USART_Mode = USART_Mode_Tx|USART_Mode_Rx;//接收和发送功能同时打开
	USART_1.USART_Parity = USART_Parity_No;//不使用奇偶校验
	USART_1.USART_StopBits = USART_StopBits_1;//1位停止位
	USART_1.USART_WordLength = USART_WordLength_8b;//8位数据位
	//初始化串口
	USART_Init(USART1,&USART_1);
	//使能串口
	USART_Cmd(USART1,ENABLE);
	
	//串口中断的配置
	USART_ITConfig(USART1,USART_IT_RXNE,ENABLE);//打开接收完成中断
	NVIC_SetPriority(USART1_IRQn,0);//设置串口1中断的优先级
	NVIC_EnableIRQ(USART1_IRQn);//使能串口的中断
}

/*
printf函数的重定向
printf函数的原功能：将信息原样打印在标准输出设备上
功能重定向为：将信息通过串口一向外发送
printf函数是通过调用fputc这个函数来实现功能的
咱们只需要将fputc这个函数的功能，由向标准输出设备写入，改变为通过串口一向外发送即可
*/
#include "stdio.h"
int fputc(int ch,FILE *fp)
{
	//等待上一帧数据发送完成
	while(USART_GetFlagStatus(USART1,USART_FLAG_TXE) == RESET);
	//通过串口一向外发送数据
	USART_SendData(USART1,ch);
	return ch;
}
#include "beep.h"
//串口1的中断服务函数
void USART1_IRQHandler(void)
{
	uint8_t data;
	//判断中断是否为接收完成中断
	while(USART_GetITStatus(USART1,USART_IT_RXNE) == SET)
	{
		//接收数据
		data = USART_ReceiveData(USART1);
		//使用电脑上的串口助手通过单片机的串口1下发数据控制蜂鸣器的开关
		//串口助手下发1开蜂鸣器；下发2关蜂鸣器
		switch(data)
		{
			case '1':BEEP_ON;break;
			case '2':BEEP_OFF;break;
		}
	}
}


