#ifndef __OLED_H
#define __OLED_H

/* SSD1306常见地址：0x78（0x3C<<1）或0x7A（0x3D<<1） */
#ifndef OLED_I2C_ADDRESS
#define OLED_I2C_ADDRESS	0x78
#endif

#ifndef OLED_I2C_ADDRESS_ALT
#define OLED_I2C_ADDRESS_ALT	0x7A
#endif

void OLED_Init(void);
void OLED_Clear(void);
void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char);
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String);
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length);
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowCustom8x16(uint8_t Line, uint8_t Column, const uint8_t *Glyph);
void OLED_Flush(void);

#endif
