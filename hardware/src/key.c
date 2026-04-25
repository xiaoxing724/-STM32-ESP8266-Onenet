#include "stm32f10x.h"

//硬件驱动
#include "key.h"
#include "delay.h"
#include "led.h"
#include "Motor.h"

/*
************************************************************
*	函数名称：	Key_Init
*
*	函数功能：	key初始化
*
*	入口参数：	无
*
*	返回参数：	无
*
*	说明：	
************************************************************
*/
void Key_Init(void)
{

	GPIO_InitTypeDef GPIO_Initstructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);		//打开GPIOB的时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	
	GPIO_Initstructure.GPIO_Mode = GPIO_Mode_IPU;			 //设置为输出上拉输入模式
	GPIO_Initstructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_11;						//将初始化的Pin脚设置为1和11
	GPIO_Initstructure.GPIO_Speed = GPIO_Speed_50MHz;				//可承载的最大频率
	
	GPIO_Init(GPIOB, &GPIO_Initstructure);							//初始化GPIOB
	
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource1);
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource11);
	
	EXTI_InitStructure.EXTI_Line = EXTI_Line1 | EXTI_Line11;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_Init(&EXTI_InitStructure);
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
	NVIC_Init(&NVIC_InitStructure);
	
	NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
	NVIC_Init(&NVIC_InitStructure);
}

void EXTI1_IRQHandler(void)
{
	DelayXms(10);
	if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_1) == 0)
	{
		if(led_info.Led_Status == LED_ON)Led_Set(LED_OFF);
		else Led_Set(LED_ON);
	}
	EXTI_ClearITPendingBit(EXTI_Line1);
}

void EXTI15_10_IRQHandler(void)
{
	DelayXms(10);
	if(GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_11) == 0)
	{
		fan_speed++;
		if(fan_speed > FAN_GEAR_MAX)
		{
			fan_speed = FAN_SPEED_MIN;
		}
		Motor_SetSpeed(fan_speed);
	}
	EXTI_ClearITPendingBit(EXTI_Line11);
}
