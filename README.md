# -STM32-ESP8266-Onenet
物联网-STM32+ESP8266+DHT11+OLED+舵机：实现APP--OneNet云端--硬件通信，实时互通温湿度数据，控制LED灯与风扇档位。

硬件接线：
1.ESP8266（WiFi）
ESP8266 TX -> STM32 PA3（USART2_RX）
ESP8266 RX -> STM32 PA2（USART2_TX）
ESP8266 VCC -> 3.3V
ESP8266 GND -> GND

2.OLED（I2C，软件模拟）
OLED SCL -> STM32 PB8
OLED SDA -> STM32 PB9
OLED VCC -> 3.3V（多数模块也支持5V，但建议3.3V）
OLED GND -> GND

3.DHT11
DHT11 DATA -> STM32 PA0
DHT11 VCC -> 3.3V 或 5V（看你模块规格）
DHT11 GND -> GND
如果是裸DHT11（非模块），DATA 需要 4.7k~10k 上拉到VCC

4.按键（2个）
KEY1 -> STM32 PB1（另一端接GND）
KEY2 -> STM32 PB11（另一端接GND）
代码里是上拉输入+下降沿中断，所以按键按下应把引脚拉低

5.LED
使用 STM32 PC13 控制（很多开发板是板载LED，无需外接）
如果外接LED：PC13 -> 电阻 -> LED -> 3.3V（该工程是低电平点亮逻辑）

6.风扇/电机驱动
PWM 输出：STM32 PA1（TIM2_CH2）
方向控制：STM32 PA4、PA5
这三根线接到电机驱动板的 PWM/IN1/IN2（或 EN/INx）输入端
驱动板电机电源按风扇额定供电，驱动板地与STM32地必须共地
不要让STM32引脚直接带电机

APP界面
<img width="693" height="858" alt="image" src="https://github.com/user-attachments/assets/339e5601-f4de-47f0-93af-dacaafb61997" />

目前功能：温湿度检测、控制LED灯、控制风扇档位

