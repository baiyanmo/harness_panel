#ifndef _MY1680_H_
#define _MY1680_H_

#include "stm32f10x.h"

// 协调器接收缓冲区(在main.c中定义)
extern u8 ZigBee_RX_buff[64];
extern u8 ZigBee_RX_count;
extern u8 ZigBee_RecFlag;

void MY1680_Config(void);
void UART4_SendData(u8 data);
void Player_41(u8 H, u8 L);
void Player_42(u8 H, u8 L);
void Player_15();
void Player_16();
void Player_num(int num);

#endif
