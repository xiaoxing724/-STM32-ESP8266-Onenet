/*************** STM32F103C8T6 **********************
 * 文件名  : dht11.c
 * 功能    : DHT11 温湿度传感器驱动
 * 接口    : PA0 (DATA)
 ********************************************************/

#include "dht11.h"
#include "delay.h"

#define DT GPIO_Pin_0
      
// 复位 DHT11
void DHT11_Rst(void)	   
{                 
	DHT11_IO_OUT(); 	// 配置为输出模式
	DHT11_DQ_OUT(0); 	// 拉低数据线
	DelayXms(20);    	// 保持低电平至少 18ms
	DHT11_DQ_OUT(1); 	// 释放数据线
	DelayUs(30);     	// 主机拉高 20~40us
}

// 等待 DHT11 响应
// 返回值: 0=检测到设备, 1=未检测到设备
u8 DHT11_Check(void) 	   
{   
	u8 retry=0;
	DHT11_IO_IN(); // 配置为输入模式	 
    while (DHT11_DQ_IN&&retry<100) // DHT11 先拉低 40~80us
	{
		retry++;
		DelayUs(1);
	};	 
	if(retry>=100)return 1;
	else retry=0;
    while (!DHT11_DQ_IN&&retry<100) // 随后再拉高 40~80us
	{
		retry++;
		DelayUs(1);
	};
	if(retry>=100)return 1;	    
	return 0;
}

// 从 DHT11 读取 1 bit
// 返回值: 0 或 1
u8 DHT11_Read_Bit(void) 			 
{
 	u8 retry=0;
	while(DHT11_DQ_IN&&retry<100) // 等待低电平起始
	{
		retry++;
		DelayUs(1);
	}
	retry=0;
	while(!DHT11_DQ_IN&&retry<100) // 等待进入高电平
	{
		retry++;
		DelayUs(1);
	}
	DelayUs(40); // 在高电平中点采样
	if(DHT11_DQ_IN)return 1;
	else return 0;		   
}

// 从 DHT11 读取 1 byte
// 返回值: 读取到的 8 位数据
u8 DHT11_Read_Byte(void)    
{        
	u8 i,dat;
	dat=0;
	for (i=0;i<8;i++) 
	{
		dat<<=1; 
		dat|=DHT11_Read_Bit();
	}						    
	return dat;
}

// 从 DHT11 读取一帧数据
// temp: 温度整数值
// humi: 湿度整数值
// 返回值: 0=成功, 1=失败
u8 DHT11_Read_Data(u8 *temp,u8 *humi)    
{        
 	u8 buf[5];
	u8 i;
	DHT11_Rst();
	if(DHT11_Check()==0)
	{
		for(i=0;i<5;i++) // 读取 40 bit (5 字节)
		{
			buf[i]=DHT11_Read_Byte();
		}
		if((buf[0]+buf[1]+buf[2]+buf[3])==buf[4])
		{
			*humi=buf[0];
			*temp=buf[2];
		}
	}
	else return 1;
	return 0;	    
}

// 初始化 DHT11 IO，并检测设备是否存在
// 返回值: 0=存在, 1=不存在
u8 DHT11_Init(void)
{	 
 	GPIO_InitTypeDef  GPIO_InitStructure;	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	 // 使能 GPIOA 时钟
	GPIO_InitStructure.GPIO_Pin = DT;				 // PA0 引脚配置
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 // 推挽输出
 	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);				 // 初始化 IO
	GPIO_SetBits(GPIOA,DT);						 // PA0 输出高
			    
	DHT11_Rst();  // 复位 DHT11
	return DHT11_Check(); // 等待 DHT11 响应
} 

