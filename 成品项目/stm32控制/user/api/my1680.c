#include "my1680.h"

// 协调器接收缓冲区
u8 ZigBee_RX_buff[64];
u8 ZigBee_RX_count = 0;
u8 ZigBee_RecFlag = 0;

/*
功能：UART4初始化，用于连接ZigBee协调器
PC10--UART4_TX   PC11--UART4_RX
*/
void MY1680_Config(void)
{
	GPIO_InitTypeDef GPIO_UART4 = {0};
	USART_InitTypeDef UART_4 = {0};
	NVIC_InitTypeDef NVIC_UART4 = {0};

	// 使能时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE);

	// 配置PC10(TX) - 复用推挽输出
	GPIO_UART4.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_UART4.GPIO_Pin = GPIO_Pin_10;
	GPIO_UART4.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOC, &GPIO_UART4);

	// 配置PC11(RX) - 浮空输入
	GPIO_UART4.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_UART4.GPIO_Pin = GPIO_Pin_11;
	GPIO_Init(GPIOC, &GPIO_UART4);

	// 配置UART4
	UART_4.USART_BaudRate = 115200;
	UART_4.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	UART_4.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
	UART_4.USART_Parity = USART_Parity_No;
	UART_4.USART_StopBits = USART_StopBits_1;
	UART_4.USART_WordLength = USART_WordLength_8b;

	// 初始化UART4
	USART_Init(UART4, &UART_4);

	// NVIC配置
	NVIC_UART4.NVIC_IRQChannel = UART4_IRQn;
	NVIC_UART4.NVIC_IRQChannelCmd = ENABLE;
	NVIC_UART4.NVIC_IRQChannelPreemptionPriority = 3;
	NVIC_UART4.NVIC_IRQChannelSubPriority = 3;
	NVIC_Init(&NVIC_UART4);

	// 使能接收中断和空闲中断
	USART_ITConfig(UART4, USART_IT_RXNE, ENABLE);
	USART_ITConfig(UART4, USART_IT_IDLE, ENABLE);

	// 使能UART4
	USART_Cmd(UART4, ENABLE);
}

/*
功能：UART4发送一个字节
*/
void UART4_SendData(u8 data)
{
	while(USART_GetFlagStatus(UART4, USART_FLAG_TC) == RESET);
	USART_SendData(UART4, data);
}

/*
功能：UART4中断服务函数 - 接收协调器数据
*/
void UART4_IRQHandler(void)
{
	u8 data = 0;

	// 接收中断
	if(USART_GetITStatus(UART4, USART_IT_RXNE) == SET)
	{
		data = UART4->DR;
		ZigBee_RX_buff[ZigBee_RX_count++] = data;

		if(ZigBee_RX_count >= 64)
			ZigBee_RX_count = 0;
	}

	// 空闲中断 - 一帧数据接收完成
	if(USART_GetITStatus(UART4, USART_IT_IDLE) == SET)
	{
		UART4->SR;
		UART4->DR;
		ZigBee_RecFlag = 1;
	}
}

/*
功能：播放指定目录下的曲目
参数：
	H: 目录的高位
	L: 目录的低位
*/
void Player_41(u8 H, u8 L)
{
	UART4_SendData(0x7E);
	UART4_SendData(0x05);
	UART4_SendData(0x41);
	UART4_SendData(H);
	UART4_SendData(L);
	u8 cheak = 0x05 ^ 0x41 ^ H ^ L;
	UART4_SendData(cheak);
	UART4_SendData(0xEF);
}

/*
功能：播放指定文件夹中的曲目
参数：
	H: 文件夹号
	L: 曲目号
*/
void Player_42(u8 H, u8 L)
{
	UART4_SendData(0x7E);
	UART4_SendData(0x05);
	UART4_SendData(0x42);
	UART4_SendData(H);
	UART4_SendData(L);
	u8 cheak = 0x05 ^ 0x42 ^ H ^ L;
	UART4_SendData(cheak);
	UART4_SendData(0xEF);
}

/*
功能：my1680继续播放
*/
void Player_15()
{
	UART4_SendData(0x7E);
	UART4_SendData(0x03);
	UART4_SendData(0x15);
	UART4_SendData(0x16);
	UART4_SendData(0xEF);
}

/*
功能：暂停my1680播放
*/
void Player_16()
{
	UART4_SendData(0x7E);
	UART4_SendData(0x03);
	UART4_SendData(0x16);
	UART4_SendData(0x15);
	UART4_SendData(0xEF);
}

/*
功能：播放数字（小于1000）
注意：需要自己控制延时
*/
void Player_num(int num)
{
	u8 i, j, k;
	if(num < 0)
	{
		Player_42(00, 24);  // 播放负号
		num = -num;
	}
	if(num < 20)
	{
		Player_42(00, num);
		return;
	}
	i = num / 100;
	j = (num % 100) / 10;
	k = num % 10;

	// 播放百位
	if(i != 0)
	{
		Player_42(00, i);
		Player_42(00, 20);  // 播放百
	}
	// 播放十位
	if(j != 0)
	{
		Player_42(00, j);
		Player_42(00, 10);  // 播放十
	}
	// 播放个位
	if(k != 0)
	{
		if(j == 0)
		{
			Player_42(00, 0);  // 先播0
		}
		Player_42(00, k);
	}
}
