#ifndef _UART5_H_
#define _UART5_H_

#include "stm32f10x.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "wifi.h"

#define int8 char
#define uint8 uint8_t
#define uint16 uint16_t

struct tagSensor_Nodex //��ʪ�ȼ��
{
	uint8 t_val;//�¶�
	uint8 h_val;//ʪ��
	uint8 mode;//
};

//�û��޸�����Ĵ���
extern struct tagSensor_Nodex node_val;//����Э�������͹����Ľڵ�����

#define SENSOR_MAX_RX_BUFF_LEN 256
struct SENSOR_MESSAGE
{
	uint8_t rx_buff[SENSOR_MAX_RX_BUFF_LEN];//������յ�������
	uint32_t rx_count;                     //������յ����ݵĸ���
	uint8_t rx_over_flag;                 //������ɱ�־ 0-δ������� 1-�������
};

extern struct SENSOR_MESSAGE sensor_message;

//void Bafayun_Data_Anlyze(void);
void USART3_Init(void);
void USART3_Updata(void);
void ZigBee_SendByte(uint8_t data);
void ZigBee_SendStr(char *str);
#endif
