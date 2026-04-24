#include "stm32f10x.h"

//硬件驱动
#include "led.h"


LED_INFO led_info = {0};


/*
************************************************************
*	函数名称：	Led_Init
*
*	函数功能：	led初始化
*
*	入口参数：	无
*
*	返回参数：	无
*
*	说明：		
************************************************************
*/
void Led_Init(void)
{

	GPIO_InitTypeDef gpio_initstruct;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);		//打开GPIOB的时钟

	
	gpio_initstruct.GPIO_Mode = GPIO_Mode_Out_PP;				//设置为输出
	gpio_initstruct.GPIO_Pin = GPIO_Pin_13;						//将初始化的Pin脚
	gpio_initstruct.GPIO_Speed = GPIO_Speed_50MHz;				//可承载的最大频率
	
	GPIO_Init(GPIOC, &gpio_initstruct);							//初始化GPIO
	
	Led_Set(LED_OFF);											//初始化完成后,关闭LED

}


void Led_Set(_Bool status)
{
	
	GPIO_WriteBit(GPIOC, GPIO_Pin_13, status == LED_ON ? Bit_RESET : Bit_SET);		//���status����Led_ON���򷵻�Bit_SET�����򷵻�Bit_RESET
	
	led_info.Led_Status = status;

}
