#include "stm32f10x.h"

//硬件驱动
#include "led.h"


LED_INFO led_info = {0};


/*
************************************************************
* Function: Led_Init
* Purpose : Initialize LED GPIO.
* Params  : None.
* Return  : None.
************************************************************
*/
void Led_Init(void)
{

	GPIO_InitTypeDef gpio_initstruct;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);		// Enable GPIOC clock.

	
	gpio_initstruct.GPIO_Mode = GPIO_Mode_Out_PP;				// Push-pull output.
	gpio_initstruct.GPIO_Pin = GPIO_Pin_13;						// Use PC13.
	gpio_initstruct.GPIO_Speed = GPIO_Speed_50MHz;				// GPIO speed.
	
	GPIO_Init(GPIOC, &gpio_initstruct);							//初始化GPIO
	
	Led_Set(LED_OFF);											//初始化完成后,关闭LED

}


void Led_Set(_Bool status)
{
	
	GPIO_WriteBit(GPIOC, GPIO_Pin_13, status == LED_ON ? Bit_SET : Bit_RESET);		// Active-high LED on PC13.
	
	led_info.Led_Status = status;

}
