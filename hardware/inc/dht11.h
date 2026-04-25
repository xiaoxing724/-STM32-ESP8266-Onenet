#ifndef DHT11_H
#define DHT11_H

#include "stm32f10x.h"
// PA0
#define DHT11_IO_IN()  {GPIOA->CRL&=0XFFFFFFF0;GPIOA->CRL|=8;}
#define DHT11_IO_OUT() {GPIOA->CRL&=0XFFFFFFF0;GPIOA->CRL|=3;} 
// IO helper macros
#define	DHT11_DQ_OUT(X)  GPIO_WriteBit(GPIOA, GPIO_Pin_0, (BitAction)X)
#define	DHT11_DQ_IN  GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0)

u8 DHT11_Init(void); // Initialize DHT11
u8 DHT11_Read_Data(u8 *temp,u8 *humi); // Read humidity and temperature
u8 DHT11_Read_Byte(void); // 读取 1 字节
u8 DHT11_Read_Bit(void); // 读取 1 bit
u8 DHT11_Check(void); // Check DHT11 presence
void DHT11_Rst(void); // 复位 DHT11

#endif
