#include "main.h"
#include "stm32f10x_usart.h"

// LED状态 0=关 1=开
u8 led_state = 0;

int main()
{
	JTAG_SWD_Config();
	LED_Config();
	USART1_Config(9600);             // 调试用
	usart2_config(9600);          // ESP8266 WiFi
	USART3_Init();                  // ZigBee协调器（sensor.c中，9600，收发）

	printf("===== Smart Hotel System Start =====\r\n");

	while(1)
	{
		// 检测ZigBee数据
		USART3_Updata();

		// 检测WiFi数据
		if(WIFI_Data.WIFI_RecFlag == 1)
		{
			WIFI_Data.RX_buff[WIFI_Data.RX_count] = '\0';
			printf("[WiFi] [%s]\r\n", WIFI_Data.RX_buff);

			// 去掉尾部的\r\n
			{
				int len = strlen((char *)WIFI_Data.RX_buff);
				while(len > 0 && (WIFI_Data.RX_buff[len-1] == '\r' || WIFI_Data.RX_buff[len-1] == '\n'))
				{
					WIFI_Data.RX_buff[--len] = '\0';
				}
			}

			// 颜色映射：云端命令 → 协调器短命令
			char *zigbee_cmd = "off";  // 默认关闭
			if(strstr((char*)WIFI_Data.RX_buff, "red") != NULL ||
			   strstr((char*)WIFI_Data.RX_buff, "RED") != NULL)
			{
				zigbee_cmd = "on1";
				LED1_ON; LED2_OFF; LED3_OFF;
			}
			else if(strstr((char*)WIFI_Data.RX_buff, "green") != NULL ||
			        strstr((char*)WIFI_Data.RX_buff, "GREEN") != NULL)
			{
				zigbee_cmd = "on2";
				LED1_OFF; LED2_ON; LED3_OFF;
			}
			else if(strstr((char*)WIFI_Data.RX_buff, "blue") != NULL ||
			        strstr((char*)WIFI_Data.RX_buff, "BLUE") != NULL)
			{
				zigbee_cmd = "on3";
				LED1_OFF; LED2_OFF; LED3_ON;
			}
			else if(strstr((char*)WIFI_Data.RX_buff, "off") != NULL ||
			        strstr((char*)WIFI_Data.RX_buff, "OFF") != NULL)
			{
				zigbee_cmd = "off";
				LED1_OFF; LED2_OFF; LED3_OFF;
			}

			printf("[ZigBee] %s\r\n", zigbee_cmd);
			ZigBee_SendStr(zigbee_cmd);

			// ★ 清零接收缓冲区，否则下一条消息会追加到上一条后面
			WIFI_Data.WIFI_RecFlag = 0;
			WIFI_Data.RX_count = 0;
		}
	}
}
