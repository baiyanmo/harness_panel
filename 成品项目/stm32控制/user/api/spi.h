#ifndef __SPI_H
#define __SPI_H			  	 
#include "delay.h"
#include "stm32f10x_spi.h"



#define LCD_SPI                       SPI2
#define LCD_SPI_CLK                   RCC_APB1Periph_SPI2
#define LCD_SPI_CLK_CMD							 	RCC_APB1PeriphClockCmd

#define LCD_SPI_SCK_PIN               GPIO_Pin_13                 
#define LCD_SPI_SCK_GPIO_PORT         GPIOB                      
#define LCD_SPI_SCK_GPIO_CLK          RCC_APB2Periph_GPIOB

#define LCD_SPI_MOSI_PIN              GPIO_Pin_15                 
#define LCD_SPI_MOSI_GPIO_PORT        GPIOB                   
#define LCD_SPI_MOSI_GPIO_CLK         RCC_APB2Periph_GPIOB

#define LCD_CS_PIN                    GPIO_Pin_10              
#define LCD_CS_GPIO_PORT              GPIOB                   
#define LCD_CS_GPIO_CLK               RCC_APB2Periph_GPIOB


#define LCD_RES_PIN                   GPIO_Pin_12              
#define LCD_RES_GPIO_PORT             GPIOB                  
#define LCD_RES_GPIO_CLK              RCC_APB2Periph_GPIOB

#define LCD_DC_PIN                    GPIO_Pin_11            
#define LCD_DC_GPIO_PORT              GPIOB                  
#define LCD_DC_GPIO_CLK               RCC_APB2Periph_GPIOB

#define LCD_BL_PIN                    GPIO_Pin_6             
#define LCD_BL_GPIO_PORT              GPIOC                  
#define LCD_BL_GPIO_CLK               RCC_APB2Periph_GPIOC





//片选控制
#define LCD_CS_CMD(x)		(x)?(GPIO_SetBits(LCD_CS_GPIO_PORT, LCD_CS_PIN)):(GPIO_ResetBits(LCD_CS_GPIO_PORT, LCD_CS_PIN))
//数据和命令引脚控制
#define LCD_DC_CMD(x)		(x)?(GPIO_SetBits(LCD_DC_GPIO_PORT, LCD_DC_PIN)):(GPIO_ResetBits(LCD_DC_GPIO_PORT, LCD_DC_PIN))
//复位引脚控制
#define LCD_RES_CMD(x)	(x)?(GPIO_SetBits(LCD_RES_GPIO_PORT, LCD_RES_PIN)):(GPIO_ResetBits(LCD_RES_GPIO_PORT, LCD_RES_PIN))
//背光控制
#define LCD_BL_CMD(x)	(x)?(GPIO_SetBits(LCD_BL_GPIO_PORT, LCD_BL_PIN)):(GPIO_ResetBits(LCD_BL_GPIO_PORT, LCD_BL_PIN))







void SPI2_Init(void);
void SPI2_SetSpeed(uint8_t SPI_BaudRatePrescaler);
uint8_t SPI2_ReadWriteByte(uint8_t TxData);




#endif




