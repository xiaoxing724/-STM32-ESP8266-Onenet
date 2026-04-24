#include "stm32f10x.h"
#include "OLED.h"
#include "OLED_Font.h"
#include <string.h>

#ifndef OLED_I2C_ADDRESS
#define OLED_I2C_ADDRESS	0x78
#endif

#ifndef OLED_I2C_ADDRESS_ALT
#define OLED_I2C_ADDRESS_ALT	0x7A
#endif

/*引脚配置*/
#define OLED_W_SCL(x)		GPIO_WriteBit(GPIOB, GPIO_Pin_8, (BitAction)(x))
#define OLED_W_SDA(x)		GPIO_WriteBit(GPIOB, GPIO_Pin_9, (BitAction)(x))

#define OLED_PAGE_COUNT	8
#define OLED_COL_COUNT	128

static uint8_t oled_gram[OLED_PAGE_COUNT][OLED_COL_COUNT];
static uint8_t oled_dirty_min[OLED_PAGE_COUNT];
static uint8_t oled_dirty_max[OLED_PAGE_COUNT];
static uint8_t oled_dirty = 0;

/*软I2C延时，避免时钟翻转过快导致OLED无响应*/
static void OLED_I2C_Delay(void)
{
	volatile uint16_t i;
	for (i = 0; i < 80; i++)
	{
		__NOP();
	}
}

/* 先声明底层函数，避免C89下隐式声明导致类型冲突 */
void OLED_I2C_Start(void);
void OLED_I2C_SendByte(uint8_t Byte);
void OLED_I2C_Stop(void);

static void OLED_WriteDataBurst(uint8_t Address, const uint8_t *Data, uint8_t Len)
{
	uint8_t i;

	OLED_I2C_Start();
	OLED_I2C_SendByte(Address);
	OLED_I2C_SendByte(0x40);
	for(i = 0; i < Len; i++)
	{
		OLED_I2C_SendByte(Data[i]);
	}
	OLED_I2C_Stop();
}

static void OLED_MarkDirty(uint8_t page, uint8_t col)
{
	if(page >= OLED_PAGE_COUNT || col >= OLED_COL_COUNT)
		return;

	if(oled_dirty_min[page] == 0xFF)
	{
		oled_dirty_min[page] = col;
		oled_dirty_max[page] = col;
	}
	else
	{
		if(col < oled_dirty_min[page]) oled_dirty_min[page] = col;
		if(col > oled_dirty_max[page]) oled_dirty_max[page] = col;
	}

	oled_dirty = 1;
}

static void OLED_ResetDirty(void)
{
	uint8_t i;
	for(i = 0; i < OLED_PAGE_COUNT; i++)
	{
		oled_dirty_min[i] = 0xFF;
		oled_dirty_max[i] = 0;
	}
	oled_dirty = 0;
}

static void OLED_WriteByte(uint8_t Address, uint8_t Control, uint8_t Data)
{
	OLED_I2C_Start();
	OLED_I2C_SendByte(Address);
	OLED_I2C_SendByte(Control);
	OLED_I2C_SendByte(Data);
	OLED_I2C_Stop();
}

/*引脚初始化*/
void OLED_I2C_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
 	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
 	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	OLED_W_SCL(1);
	OLED_W_SDA(1);
}

/**
  * @brief  I2C开始
  * @param  无
  * @retval 无
  */
void OLED_I2C_Start(void)
{
	OLED_W_SDA(1);
	OLED_W_SCL(1);
	OLED_I2C_Delay();
	OLED_W_SDA(0);
	OLED_I2C_Delay();
	OLED_W_SCL(0);
	OLED_I2C_Delay();
}

/**
  * @brief  I2C停止
  * @param  无
  * @retval 无
  */
void OLED_I2C_Stop(void)
{
	OLED_W_SDA(0);
	OLED_I2C_Delay();
	OLED_W_SCL(1);
	OLED_I2C_Delay();
	OLED_W_SDA(1);
	OLED_I2C_Delay();
}

/**
  * @brief  I2C发送一个字节
  * @param  Byte 要发送的一个字节
  * @retval 无
  */
void OLED_I2C_SendByte(uint8_t Byte)
{
	uint8_t i;
	for (i = 0; i < 8; i++)
	{
		OLED_W_SDA(!!(Byte & (0x80 >> i)));
		OLED_I2C_Delay();
		OLED_W_SCL(1);
		OLED_I2C_Delay();
		OLED_W_SCL(0);
		OLED_I2C_Delay();
	}
	OLED_W_SCL(1);	//额外的一个时钟，不处理应答信号
	OLED_I2C_Delay();
	OLED_W_SCL(0);
	OLED_I2C_Delay();
}

/**
  * @brief  OLED写命令
  * @param  Command 要写入的命令
  * @retval 无
  */
void OLED_WriteCommand(uint8_t Command)
{
	OLED_WriteByte(OLED_I2C_ADDRESS, 0x00, Command);
	if (OLED_I2C_ADDRESS_ALT != OLED_I2C_ADDRESS)
	{
		OLED_WriteByte(OLED_I2C_ADDRESS_ALT, 0x00, Command);
	}
}

/**
  * @brief  OLED写数据
  * @param  Data 要写入的数据
  * @retval 无
  */
void OLED_WriteData(uint8_t Data)
{
	OLED_WriteByte(OLED_I2C_ADDRESS, 0x40, Data);
	if (OLED_I2C_ADDRESS_ALT != OLED_I2C_ADDRESS)
	{
		OLED_WriteByte(OLED_I2C_ADDRESS_ALT, 0x40, Data);
	}
}

/**
  * @brief  OLED设置光标位置
  * @param  Y 以左上角为原点，向下方向的坐标，范围：0~7
  * @param  X 以左上角为原点，向右方向的坐标，范围：0~127
  * @retval 无
  */
void OLED_SetCursor(uint8_t Y, uint8_t X)
{
	OLED_WriteCommand(0xB0 | Y);					//设置Y位置
	OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4));	//设置X位置高4位
	OLED_WriteCommand(0x00 | (X & 0x0F));			//设置X位置低4位
}

/**
  * @brief  OLED清屏
  * @param  无
  * @retval 无
  */
void OLED_Clear(void)
{  
	uint8_t i, j;
	for (j = 0; j < OLED_PAGE_COUNT; j++)
	{
		for(i = 0; i < OLED_COL_COUNT; i++)
		{
			oled_gram[j][i] = 0x00;
		}
		oled_dirty_min[j] = 0;
		oled_dirty_max[j] = OLED_COL_COUNT - 1;
	}
	oled_dirty = 1;
	OLED_Flush();
}

/**
  * @brief  OLED显示一个字符
  * @param  Line 行位置，范围：1~4
  * @param  Column 列位置，范围：1~16
  * @param  Char 要显示的一个字符，范围：ASCII可见字符
  * @retval 无
  */
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char)
{      	
	uint8_t i;
	uint8_t page = (Line - 1) * 2;
	uint8_t col = (Column - 1) * 8;

	if(Line < 1 || Line > 4 || Column < 1 || Column > 16)
		return;

	if(Char < ' ' || Char > '~')
		Char = ' ';

	for (i = 0; i < 8; i++)
	{
		oled_gram[page][col + i] = OLED_F8x16[Char - ' '][i];
		OLED_MarkDirty(page, col + i);
	}

	page++;
	for (i = 0; i < 8; i++)
	{
		oled_gram[page][col + i] = OLED_F8x16[Char - ' '][i + 8];
		OLED_MarkDirty(page, col + i);
	}
}

/**
  * @brief  OLED显示字符串
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  String 要显示的字符串，范围：ASCII可见字符
  * @retval 无
  */
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String)
{
	uint8_t i;
	if(String == NULL)
		return;

	for (i = 0; String[i] != '\0'; i++)
	{
		if(Column + i > 16)
			break;
		OLED_ShowChar(Line, Column + i, String[i]);
	}
}

void OLED_ShowCustom8x16(uint8_t Line, uint8_t Column, const uint8_t *Glyph)
{
	uint8_t i;
	uint8_t page = (Line - 1) * 2;
	uint8_t col = (Column - 1) * 8;

	if(Line < 1 || Line > 4 || Column < 1 || Column > 16 || Glyph == NULL)
		return;

	for(i = 0; i < 8; i++)
	{
		oled_gram[page][col + i] = Glyph[i];
		OLED_MarkDirty(page, col + i);
	}

	page++;
	for(i = 0; i < 8; i++)
	{
		oled_gram[page][col + i] = Glyph[i + 8];
		OLED_MarkDirty(page, col + i);
	}
}

void OLED_Flush(void)
{
	uint8_t page;
	uint8_t len;

	if(oled_dirty == 0)
		return;

	for(page = 0; page < OLED_PAGE_COUNT; page++)
	{
		if(oled_dirty_min[page] != 0xFF)
		{
			len = oled_dirty_max[page] - oled_dirty_min[page] + 1;
			OLED_SetCursor(page, oled_dirty_min[page]);
			OLED_WriteDataBurst(OLED_I2C_ADDRESS, &oled_gram[page][oled_dirty_min[page]], len);
			if (OLED_I2C_ADDRESS_ALT != OLED_I2C_ADDRESS)
			{
				OLED_SetCursor(page, oled_dirty_min[page]);
				OLED_WriteDataBurst(OLED_I2C_ADDRESS_ALT, &oled_gram[page][oled_dirty_min[page]], len);
			}
		}
	}

	OLED_ResetDirty();
}

/**
  * @brief  OLED次方函数
  * @retval 返回值等于X的Y次方
  */
uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
	uint32_t Result = 1;
	while (Y--)
	{
		Result *= X;
	}
	return Result;
}

/**
  * @brief  OLED显示数字（十进制，正数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：0~4294967295
  * @param  Length 要显示数字的长度，范围：1~10
  * @retval 无
  */
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i++)							
	{
		OLED_ShowChar(Line, Column + i, Number / OLED_Pow(10, Length - i - 1) % 10 + '0');
	}
}

/**
  * @brief  OLED显示数字（十进制，带符号数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：-2147483648~2147483647
  * @param  Length 要显示数字的长度，范围：1~10
  * @retval 无
  */
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length)
{
	uint8_t i;
	uint32_t Number1;
	if (Number >= 0)
	{
		OLED_ShowChar(Line, Column, '+');
		Number1 = Number;
	}
	else
	{
		OLED_ShowChar(Line, Column, '-');
		Number1 = -Number;
	}
	for (i = 0; i < Length; i++)							
	{
		OLED_ShowChar(Line, Column + i + 1, Number1 / OLED_Pow(10, Length - i - 1) % 10 + '0');
	}
}

/**
  * @brief  OLED显示数字（十六进制，正数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：0~0xFFFFFFFF
  * @param  Length 要显示数字的长度，范围：1~8
  * @retval 无
  */
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i, SingleNumber;
	for (i = 0; i < Length; i++)							
	{
		SingleNumber = Number / OLED_Pow(16, Length - i - 1) % 16;
		if (SingleNumber < 10)
		{
			OLED_ShowChar(Line, Column + i, SingleNumber + '0');
		}
		else
		{
			OLED_ShowChar(Line, Column + i, SingleNumber - 10 + 'A');
		}
	}
}

/**
  * @brief  OLED显示数字（二进制，正数）
  * @param  Line 起始行位置，范围：1~4
  * @param  Column 起始列位置，范围：1~16
  * @param  Number 要显示的数字，范围：0~1111 1111 1111 1111
  * @param  Length 要显示数字的长度，范围：1~16
  * @retval 无
  */
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i++)							
	{
		OLED_ShowChar(Line, Column + i, Number / OLED_Pow(2, Length - i - 1) % 2 + '0');
	}
}

/**
  * @brief  OLED初始化
  * @param  无
  * @retval 无
  */
void OLED_Init(void)
{
	uint32_t i, j;
	OLED_ResetDirty();
	
	for (i = 0; i < 1000; i++)			//上电延时
	{
		for (j = 0; j < 1000; j++);
	}
	
	OLED_I2C_Init();			//端口初始化
	
	OLED_WriteCommand(0xAE);	//关闭显示
	
	OLED_WriteCommand(0xD5);	//设置显示时钟分频比/振荡器频率
	OLED_WriteCommand(0x80);
	
	OLED_WriteCommand(0xA8);	//设置多路复用率
	OLED_WriteCommand(0x3F);
	
	OLED_WriteCommand(0xD3);	//设置显示偏移
	OLED_WriteCommand(0x00);
	
	OLED_WriteCommand(0x40);	//设置显示开始行
	
	OLED_WriteCommand(0xA1);	//设置左右方向，0xA1正常 0xA0左右反置
	
	OLED_WriteCommand(0xC8);	//设置上下方向，0xC8正常 0xC0上下反置

	OLED_WriteCommand(0xDA);	//设置COM引脚硬件配置
	OLED_WriteCommand(0x12);
	
	OLED_WriteCommand(0x81);	//设置对比度控制
	OLED_WriteCommand(0xCF);

	OLED_WriteCommand(0xD9);	//设置预充电周期
	OLED_WriteCommand(0xF1);

	OLED_WriteCommand(0xDB);	//设置VCOMH取消选择级别
	OLED_WriteCommand(0x30);

	OLED_WriteCommand(0xA4);	//设置整个显示打开/关闭

	OLED_WriteCommand(0xA6);	//设置正常/倒转显示

	OLED_WriteCommand(0x8D);	//设置充电泵
	OLED_WriteCommand(0x14);

	OLED_WriteCommand(0xAF);	//开启显示
		
	OLED_Clear();				//OLED清屏
}
