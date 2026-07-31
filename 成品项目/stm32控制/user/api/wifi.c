#include "wifi.h"

WIFI_DataTypedef WIFI_Data = {0};

/*
�������ܣ�WIFIģ��ĳ�ʼ�����������ò���ʼ������2��
����������
	brr�����ڶ��Ĳ�����
PA2--USART2_TX(��������)   PA3--USART2_RX(��������)
*/
void usart2_config(uint32_t brr)
{
	GPIO_InitTypeDef GPIO_USART2 = {0};//����GPIO�ڵĽṹ�����
	USART_InitTypeDef USART_2 = {0};//���ô��ڵĽṹ�����
	NVIC_InitTypeDef NVIC_USART2 = {0};//����NVIC�Ľṹ�����
	//��ʱ��
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2,ENABLE);
	//���ò���ʼ��PA2
	GPIO_USART2.GPIO_Mode = GPIO_Mode_AF_PP;//��������
	GPIO_USART2.GPIO_Pin = GPIO_Pin_2;//ѡ��ܽ�PA2
	GPIO_USART2.GPIO_Speed = GPIO_Speed_50MHz;//������ʣ�һ��ֱ�Ӹ����
	GPIO_Init(GPIOA,&GPIO_USART2);
	//���ò���ʼ��PA3
	GPIO_USART2.GPIO_Mode = GPIO_Mode_IN_FLOATING;//��������
	GPIO_USART2.GPIO_Pin = GPIO_Pin_3;//ѡ��ܽ�PA2
	GPIO_Init(GPIOA,&GPIO_USART2);
	//���ô���
	USART_2.USART_BaudRate = brr;//������
	USART_2.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��ʹ��Ӳ������
	USART_2.USART_Mode = USART_Mode_Tx|USART_Mode_Rx;//���ܷ���ģʽͬʱ��
	USART_2.USART_Parity = USART_Parity_No;//��ʹ����żУ��
	USART_2.USART_StopBits = USART_StopBits_1;//1λֹͣλ
	USART_2.USART_WordLength = USART_WordLength_8b;//8λ����λ
	//��ʼ������
	USART_Init(USART2,&USART_2);
	//ʹ�ܴ���
	USART_Cmd(USART2,ENABLE);
	//NVIC����
	NVIC_USART2.NVIC_IRQChannel = USART2_IRQn;//Ҫ�����ĸ�ģ����ж�
	NVIC_USART2.NVIC_IRQChannelCmd = ENABLE;//ʹ��:ENABLE �� ʧ��:DISABLE
	NVIC_USART2.NVIC_IRQChannelPreemptionPriority = 1;//ռ�����ȼ�
	NVIC_USART2.NVIC_IRQChannelSubPriority = 1;//�������ȼ�
	//NVIC��ʼ��
	NVIC_Init(&NVIC_USART2);
	//����2�ж�����
	USART_ITConfig(USART2,USART_IT_RXNE,ENABLE);//�򿪽����ж�
	USART_ITConfig(USART2,USART_IT_IDLE,ENABLE);//�򿪿����ж�
}

/*
�������ܣ�����2���ⷢ�͵��ֽ�
����������
	data��Ҫ���͵�����
*/
void wifi_SendData(u8 data)
{
	//�ȴ���һ֡���ݷ������
	while(USART_GetFlagStatus(USART2,USART_FLAG_TC) == RESET);
	//��������
	USART_SendData(USART2,data);
}

/*
�������ܣ�����2���ⷢ���ַ���
����������
	p������Ҫ�����ַ����Ŀռ���׵�ַ
*/
void wifi_SendStr(char *p)
{
	/*�����ַ������ַ����е��ַ�
	����д�õķ��͵����ֽڵĺ���
	һ��һ���ķ��ͳ�ȥ*/
	while(*p !='\0')
	{
		wifi_SendData(*p);
		p++;
	}
}

/*
�������ܣ�����2���ⷢ�͹̶���С����
����������
Arr:Ҫ���͵�������
DataLenth������Ĵ�С����λΪ�ֽ�
*/
void wifi_SendArray(u8 Arr[],u16 DataLenth)
{
	u16 i = 0;
	/*�����������齫�����е�����Ԫ��
	����д�õķ��͵����ֽڵĺ���
	һ��һ���ķ��ͳ�ȥ*/
	while(DataLenth--)
	{
		wifi_SendData(Arr[i++]);
	}
}

/*
�������ܣ����ڵ��жϷ�����
���жϷ������������ݵĽ��ܴ���������wifiģ��Ļ�ִ��
*/
void USART2_IRQHandler(void)
{
	u8 data = 0;//���ڵ����ֽڵĽ���
	//�ж��ж������Ƿ�Ϊ�����ж�
	if(USART_GetITStatus(USART2,USART_IT_RXNE) == SET)
	{
		//����ж�
		USART_ClearITPendingBit(USART2,USART_IT_RXNE);
		data = USART2->DR;//���ڶ��Ľ���
		USART1->DR = data;//����һ�ķ���
		
		WIFI_Data.RX_buff[WIFI_Data.RX_count++] = data;
		if(WIFI_Data.RX_count >= 1024)
			WIFI_Data.RX_count = 0;
	}
	//�ж��ж������Ƿ�Ϊ�����ж�
	if(USART_GetITStatus(USART2,USART_IT_IDLE) == SET)
	{
		/*
		���ڵĿ����ж��޷�ʹ�ÿ⺯�����
		USART_ClearITPendingBit(USART2,USART_IT_IDLE);
		*/
		//��������ж�
		USART2->SR;
		USART2->DR;
		WIFI_Data.WIFI_RecFlag = 1;//�������
	}
}





