#include "stm32f10x.h"                  // Device header
#include "PWM.h"
#include "Motor.h"

const uint8_t FAN_SPEED_PWM_TABLE[FAN_SPEED_LEVELS] = {0, 20, 40, 60, 80, 100};

uint8_t fan_speed = 0;

static uint8_t Motor_NormalizeGear(uint8_t Gear)
{
	if(Gear <= FAN_GEAR_MIN)
	{
		return FAN_GEAR_MIN;
	}

	if(Gear >= FAN_GEAR_MAX)
	{
		return FAN_GEAR_MAX;
	}

	return Gear;
}

void Motor_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	PWM_Init();
}

void Motor_SetSpeed(uint8_t Speed)
{
	uint8_t gear = Motor_NormalizeGear(Speed);

	GPIO_SetBits(GPIOA, GPIO_Pin_5);
	GPIO_ResetBits(GPIOA, GPIO_Pin_4);
	fan_speed = gear;
	PWM_Setcompare2(FAN_SPEED_PWM_TABLE[gear]);
}
