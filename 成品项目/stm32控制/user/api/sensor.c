#include "sensor.h"

struct SENSOR_MESSAGE sensor_message;
uint8_t buzzerstate;
extern uint8_t ledstate;
//�û��޸�����Ĵ���
struct tagSensor_Nodex node_val;//����Э�������͹����Ľڵ�����

void USART3_Init(void)
{
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC|RCC_APB2Periph_GPIOD|RCC_APB2Periph_AFIO, ENABLE); //remapʱ��|RCC_APB2Periph_AFIO //����GPIOBʱ��
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);//����Ҫ�ֿ��� //USART3ʱ��  ����APB1
       
	//GPIO_PartialRemap_USART3 ������ӳ��  GPIOC_10  GPIOC_11
       
  GPIO_PinRemapConfig(GPIO_PartialRemap_USART3,ENABLE);  	
	//1������IO  PC10		PC11
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//�ٶ�
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;		//�����������
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;					//����
	GPIO_Init(GPIOC, &GPIO_InitStructure);						//��ʼ��
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//��������
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;						 //����
	GPIO_Init(GPIOC, &GPIO_InitStructure);							 //��ʼ��
	//2������USART3
	USART_InitTypeDef USART_InitStruct;
	USART_InitStruct.USART_BaudRate = 9600;							//������
	USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStruct.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;		//������ ������ʹ��
	USART_InitStruct.USART_Parity = USART_Parity_No;		//������żУ��
	USART_InitStruct.USART_StopBits = USART_StopBits_1;	//1��ֹͣλ
	USART_InitStruct.USART_WordLength = USART_WordLength_8b;//8������λ
	USART_Init(USART3, &USART_InitStruct);

	//�򿪴��ڵĽ����ж�
	//NVIC����
	NVIC_InitTypeDef	NVIC_InitStructure={0};
	NVIC_InitStructure.NVIC_IRQChannel =USART3_IRQn ;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;//��ռ���ȼ�
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;//�����ȼ�
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
		//�����ж�����
	USART_ITConfig(USART3,USART_IT_RXNE,ENABLE);    //�����ж�
	USART_ITConfig(USART3,USART_IT_IDLE,ENABLE);   //�����ж�
	
	
	//3.ʹ�ܴ���
	USART_Cmd(USART3,ENABLE);
}

//usart3�жϷ�����
void USART3_IRQHandler(void)
{
	u8 data = 0;
	if(USART_GetITStatus(USART3,USART_IT_RXNE) == SET)//�ж��Ƿ��Ǵ��ڽ����ж�
	{
		USART_ClearITPendingBit(USART3,USART_IT_RXNE);//����жϱ�־λ
		sensor_message.rx_buff[sensor_message.rx_count++] = USART3->DR;		
		if(sensor_message.rx_count>256)
		{
			sensor_message.rx_count = 0;
		}
	}
	if(USART_GetITStatus(USART3,USART_IT_IDLE) == SET)//�ж��Ƿ��Ǵ��ڿ����ж�
	{
		data = USART3->SR;
		data = USART3->DR;
		sensor_message.rx_over_flag = 1;//�������
		printf("sensor_message.rx_count = %d\r\n",sensor_message.rx_count);
	}
}

//�û��޸�����Ĵ���
#include "stdio.h"
#include "led.h"
char lcd_buff[256]="\0";
char sensor_sendbuff[256]="\0";

void USART3_Updata(void)
{
	if(sensor_message.rx_over_flag == 1 && sensor_message.rx_count > 0){

		// 加结束符，当作字符串处理
		sensor_message.rx_buff[sensor_message.rx_count] = '\0';
		char *cmd = (char *)sensor_message.rx_buff;

		printf("[ZigBee] %s\r\n", cmd);

		// 收到 LED 状态，转发给 WiFi
		wifi_SendStr(cmd);

		sensor_message.rx_over_flag = 0;
		sensor_message.rx_count = 0;
	}
}

//USART3发送一个字节
void ZigBee_SendByte(uint8_t data)
{
    while(USART_GetFlagStatus(USART3, USART_FLAG_TC) == RESET);
    USART_SendData(USART3, data);
}

//USART3发送字符串
void ZigBee_SendStr(char *str)
{
    while(*str != '\0')
    {
        ZigBee_SendByte(*str);
        str++;
    }
}

