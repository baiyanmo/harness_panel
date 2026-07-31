#include "dht11.h"
#include "delay.h"
#include "stdio.h"
#include "lcd.h"
#include "wenduin.h"
#include "shiduin.h"

u8 dht11_data[5] = {0};
u8 dht11_showbuf[50] = {0};
/*
函数功能：DHT11的初始化函数
函数参数：
	填DHT11_OUT--将PB0配置为推挽输出
	填DHT11_IN--将PB0配置为浮空输入
*/
void DHT11_Config(u8 mode)
{
	GPIO_InitTypeDef DHT11 = {0};
	//开时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	//配置GPIO口
	DHT11.GPIO_Pin = GPIO_Pin_0;//管脚号PB0
	DHT11.GPIO_Speed = GPIO_Speed_50MHz;//输出速率，选最大值
	if(mode == DHT11_OUT)
		DHT11.GPIO_Mode = GPIO_Mode_Out_PP;//推挽输出
	if(mode == DHT11_IN)
		DHT11.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
	//初始化GPIO口
	GPIO_Init(GPIOB,&DHT11);
}

/*
函数功能：获取温湿度数据
*/
void GET_DHT11Data(void)
{
	u8 i = 0;
	//STM32向DHT11发送起始信号
	DHT11_Config(DHT11_OUT);
	GPIO_ResetBits(GPIOB,GPIO_Pin_0);
	Delay_ms(20);
	GPIO_SetBits(GPIOB,GPIO_Pin_0);
	//将PB0切换为浮空输入模式，接收DHT11发送过来的信号和数据
	DHT11_Config(DHT11_IN);
	//等待低电平（等待响应信号）
	while(DHT11_INPUT == 1);
	//等待高电平（等待准备信号）
	while(DHT11_INPUT == 0);
	//开始接收数据
	for(i = 0;i<40;i++)//循环40次来接收DHT11发送过来的40位数据
	{
		//等待低电平
		while(DHT11_INPUT == 1);
		//等待高电平
		while(DHT11_INPUT == 0);
		//延时30us,后判断这位数据是0还是1
		Delay_us(30);
		if(DHT11_INPUT == 0)//数据0
			dht11_data[i/8]  &= ~(1<<(7-i%8));
		if(DHT11_INPUT == 1)//数据1
			dht11_data[i/8] |= (1<<(7-i%8));
	}
	//校验 
	//dht11_data[0]:湿度整数 dht11_data[1]:湿度小数 
	//dht11_data[2]:温度整数 dht11_data[3]:温度小数
	//dht11_data[4]:校验位
	u8 cheak = dht11_data[0]+dht11_data[1]+dht11_data[2]+dht11_data[3];
	if(dht11_data[4] == cheak)//校验通过
	{
		//在LCD屏上显示数据
		LCD_ShowPicture(0,124,16,16,gImage_wenduin);
		LCD_ShowChinese(22,124,"室内温度",BLACK,WHITE,16,0);
		sprintf(dht11_showbuf,":%2d",dht11_data[2]);
		LCD_ShowString(86,124,dht11_showbuf,BLACK,WHITE,16,0);
		LCD_ShowChinese(110,124,"℃",BLACK,WHITE,16,0);
		
		LCD_ShowPicture(0,144,16,16,gImage_shiduin);
		LCD_ShowChinese(22,144,"室内湿度",BLACK,WHITE,16,0);
		sprintf(dht11_showbuf,":%2d%%",dht11_data[0]);
		LCD_ShowString(86,144,dht11_showbuf,BLACK,WHITE,16,0);
	}
}

