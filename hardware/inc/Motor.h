#ifndef __MOTOR_H
#define __MOTOR_H

#define FAN_ON		50

#define FAN_OFF		0

#define FAN_GEAR_MIN		0
#define FAN_GEAR_MAX		5
#define FAN_SPEED_MIN		0
#define FAN_SPEED_MAX		100
#define FAN_SPEED_STEP		20
#define FAN_SPEED_LEVELS	6

extern const uint8_t FAN_SPEED_PWM_TABLE[FAN_SPEED_LEVELS];

extern uint8_t fan_speed;

void Motor_Init(void);

void Motor_SetSpeed(uint8_t Speed);

#endif
