#include "stm32f10x.h"
#include "misc.h"

//网络协议层
#include "onenet.h"

//网络设备
#include "esp8266.h"

//硬件驱动
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "key.h"
#include "dht11.h"
#include "OLED.h"
#include "Motor.h"

//C库
#include <string.h>


#define ESP8266_ONENET_INFO		"AT+CIPSTART=\"TCP\",\"183.230.40.96\",1883\r\n"

/*
 * 主循环采用10ms节拍 + 多速率任务调度：
 * 1) OLED按固定周期刷新，且仅在数据变化时更新，减少闪烁与I2C占用。
 * 2) 传感器采样与数据上报解耦，避免显示和网络互相阻塞。
 * 3) 网络收包在每个节拍都尝试处理，提升下行响应速度。
 */
#define MAIN_LOOP_PERIOD_MS		10
#define NET_RETRY_INTERVAL_MS		500
#define SENSOR_SAMPLE_INTERVAL_MS	500
#define OLED_REFRESH_INTERVAL_MS	200
#define CLOUD_UPLOAD_INTERVAL_MS	800
#define NET_STATS_INTERVAL_MS		5000
#define CLOUD_UPLOAD_MIN_MS		400
#define CLOUD_UPLOAD_MAX_MS		1200
#define CLOUD_UPLOAD_STEP_MS		100
#define CLOUD_UPLOAD_FAST_MS		400
#define CLOUD_UPLOAD_NORMAL_MS		800
#define CLOUD_UPLOAD_ECO_MS		1200
#define NET_IDLE_TO_ECO_WINDOWS	3

#define UPLOAD_MODE_LOW_LATENCY	0
#define UPLOAD_MODE_BALANCED		1
#define UPLOAD_MODE_ECO		2

/* 自定义8x16字模：摄氏度符号（℃） */
#define OLED_CELSIUS_STYLE_THIN		0
#define OLED_CELSIUS_STYLE_MEDIUM	1
#define OLED_CELSIUS_STYLE_BOLD		2

/* 默认使用中等粗细，便于远距离观察 */
#define OLED_CELSIUS_STYLE		OLED_CELSIUS_STYLE_THIN

static const uint8_t OLED_GLYPH_CELSIUS_THIN[16] =
{
	0x60,0x90,0x90,0x60,0x00,0x00,0x00,0x00,
	0x00,0x0F,0x10,0x10,0x10,0x08,0x00,0x00
};

static const uint8_t OLED_GLYPH_CELSIUS_MEDIUM[16] =
{
	0x70,0xD8,0xD8,0x70,0x00,0x00,0x00,0x00,
	0x00,0x0F,0x18,0x18,0x18,0x0C,0x00,0x00
};

static const uint8_t OLED_GLYPH_CELSIUS_BOLD[16] =
{
	0x70,0xF8,0xF8,0x70,0x00,0x00,0x00,0x00,
	0x00,0x1F,0x30,0x30,0x30,0x18,0x00,0x00
};

static const uint8_t* OLED_GetCelsiusGlyph(void)
{
#if (OLED_CELSIUS_STYLE == OLED_CELSIUS_STYLE_THIN)
	return OLED_GLYPH_CELSIUS_THIN;
#elif (OLED_CELSIUS_STYLE == OLED_CELSIUS_STYLE_BOLD)
	return OLED_GLYPH_CELSIUS_BOLD;
#else
	return OLED_GLYPH_CELSIUS_MEDIUM;
#endif
}


/*
************************************************************
*	函数名称：	Hardware_Init
*
*	函数功能：	硬件初始化
*
*	入口参数：	无
*
*	返回参数：	无
*
*	说明：		初始化单片机功能以及外接设备
************************************************************
*/
void Hardware_Init(void)
{
	uint8_t dhtRetry = 0;
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	//中断控制器分组设置

	Delay_Init();									//systick初始化
	
	Usart1_Init(115200);							//串口1，打印信息用
	
	Usart2_Init(115200);							//串口2，驱动ESP8266用
	
    Key_Init();										//Key初始化
	
	Led_Init();									//LED初始化
	
	OLED_Init();									//OLED初始化
	
	Motor_Init();
	
	OLED_ShowString(1, 1, "Monitor");
	
	while(DHT11_Init())
	{
		dhtRetry++;
		UsartPrintf(USART_DEBUG, "DHT11 Error \r\n");
		OLED_ShowString(2, 1, "DHT11 Error");
		if(dhtRetry >= 5)
		{
			UsartPrintf(USART_DEBUG, "DHT11 Skip Init\r\n");
			OLED_ShowString(2, 1, "DHT11 Skip ");
			break;
		}
		DelayMs(1000);
	}
	
	UsartPrintf(USART_DEBUG, " Hardware init OK\r\n");
	
}

/*
************************************************************
*	函数名称：	main
*
*	函数功能：	
*
*	入口参数：	无
*
*	返回参数：	0
*
*	说明：		
************************************************************
*/

u8 temp,humi;

static void OLED_DrawStaticUi(void)
{
	OLED_ShowString(2, 1, "Temp:      ");
	OLED_ShowString(3, 1, "Humi:  %");
	OLED_ShowString(4, 1, "LED:    Fan:");
	OLED_Flush();
}

static void OLED_UpdateNetModeUi(unsigned char mqttReady, unsigned char uploadMode)
{
	static unsigned char lastReady = 0xFF;
	static unsigned char lastMode = 0xFF;
	char *modeText;

	if(lastReady == mqttReady && lastMode == uploadMode)
		return;

	if(mqttReady == 0)
	{
		OLED_ShowString(1, 1, "NET:OFF UP:--- ");
	}
	else
	{
		if(uploadMode == UPLOAD_MODE_LOW_LATENCY)
			modeText = "LOW";
		else if(uploadMode == UPLOAD_MODE_ECO)
			modeText = "ECO";
		else
			modeText = "BAL";

		OLED_ShowString(1, 1, "NET:ON  UP:    ");
		OLED_ShowString(1, 12, modeText);
	}

	OLED_Flush();
	lastReady = mqttReady;
	lastMode = uploadMode;
}

static void OLED_RefreshDynamicUi(_Bool force)
{
	static u8 lastTemp = 0xFF;
	static u8 lastHumi = 0xFF;
	static u8 lastFan = 0xFF;
	static _Bool lastLed = 2;
	_Bool needFlush = 0;

	if(force || lastTemp != temp)
	{
		OLED_ShowNum(2, 6, temp, 2);
		OLED_ShowCustom8x16(2, 8, OLED_GetCelsiusGlyph());
		lastTemp = temp;
		needFlush = 1;
	}

	if(force || lastHumi != humi)
	{
		OLED_ShowNum(3, 6, humi, 2);
		lastHumi = humi;
		needFlush = 1;
	}

	if(force || lastLed != led_info.Led_Status)
	{
		OLED_ShowString(4, 5, led_info.Led_Status ? "ON " : "OFF");
		lastLed = led_info.Led_Status;
		needFlush = 1;
	}

	if(force || lastFan != fan_speed)
	{
		OLED_ShowNum(4, 13, fan_speed, 3);
		lastFan = fan_speed;
		needFlush = 1;
	}

	if(needFlush)
		OLED_Flush();
}

int main(void)
{
	unsigned short netRetryTick = 0;
	unsigned short sensorTick = 0;
	unsigned short oledTick = 0;
	unsigned short uploadTick = 0;
	unsigned short statsTick = 0;
	unsigned short uploadIntervalMs = CLOUD_UPLOAD_INTERVAL_MS;
	unsigned char uploadMode = UPLOAD_MODE_BALANCED;
	unsigned char idleWindows = 0;
	unsigned char mqttReady = 0;
	unsigned char *dataPtr = NULL;
	unsigned long lastIpdCnt = 0;
	unsigned long lastCmdFailCnt = 0;
	unsigned long curIpdCnt;
	unsigned long curCmdFailCnt;
	unsigned long deltaIpd;
	unsigned long deltaCmdFail;
	const char *modeStr;
	
	Hardware_Init();				//初始化外围硬件
	OLED_DrawStaticUi();
	OLED_UpdateNetModeUi(0, UPLOAD_MODE_BALANCED);
	OLED_RefreshDynamicUi(1);
	UsartPrintf(USART_DEBUG, "Connect MQTTs Server...\r\n");
	
	ESP8266_Init();					//初始化ESP8266
	
	while(1)
	{
		/* 每个节拍先处理一次收包，降低控制指令响应延迟 */
		dataPtr = ESP8266_GetIPD(0);
		if(dataPtr != NULL && mqttReady)
			OneNet_RevPro(dataPtr);

		if(mqttReady == 0)
		{
			netRetryTick += MAIN_LOOP_PERIOD_MS;
			if(netRetryTick >= NET_RETRY_INTERVAL_MS)		//每500ms重试一次联网
			{
				netRetryTick = 0;
				if(ESP8266_SendCmd(ESP8266_ONENET_INFO, "CONNECT") == 0)
				{
					if(OneNet_DevLink() == 0)
					{
						OneNET_Subscribe();
						ESP8266_ResetStats();
						uploadMode = UPLOAD_MODE_BALANCED;
						uploadIntervalMs = CLOUD_UPLOAD_NORMAL_MS;
						idleWindows = 0;
						lastIpdCnt = 0;
						lastCmdFailCnt = 0;
						mqttReady = 1;
						OLED_UpdateNetModeUi(mqttReady, uploadMode);
						OLED_RefreshDynamicUi(1);
						UsartPrintf(USART_DEBUG, "Connect MQTTs Success! \r\n");
					}
				}
			}
		}

		sensorTick += MAIN_LOOP_PERIOD_MS;
		if(sensorTick >= SENSOR_SAMPLE_INTERVAL_MS)
		{
			sensorTick = 0;
			DHT11_Read_Data(&temp,&humi);
			UsartPrintf(USART_DEBUG, "temp %d ,humi %d fan %d\r\n",temp,humi,fan_speed);
		}

		oledTick += MAIN_LOOP_PERIOD_MS;
		if(oledTick >= OLED_REFRESH_INTERVAL_MS)
		{
			oledTick = 0;
			OLED_RefreshDynamicUi(0);
		}

		uploadTick += MAIN_LOOP_PERIOD_MS;
		if(mqttReady && uploadTick >= uploadIntervalMs)
		{
			uploadTick = 0;
			UsartPrintf(USART_DEBUG, "OneNet_SendData\r\n");
			OneNet_SendData();
		}

		statsTick += MAIN_LOOP_PERIOD_MS;
		if(statsTick >= NET_STATS_INTERVAL_MS)
		{
			statsTick = 0;
			curIpdCnt = ESP8266_GetIpdPackets();
			curCmdFailCnt = ESP8266_GetSendCmdFailCount();

			deltaIpd = curIpdCnt - lastIpdCnt;
			deltaCmdFail = curCmdFailCnt - lastCmdFailCnt;

			/*
			 * 双模策略：
			 * 1) 下行活跃时进入低延迟模式，快速上报提升交互实时性。
			 * 2) 长时间无下行时进入省流量模式，减小链路占用。
			 * 3) 出现命令失败时优先放慢上报，保障稳定性。
			 */
			if(deltaCmdFail > 0)
			{
				uploadMode = UPLOAD_MODE_ECO;
				uploadIntervalMs = CLOUD_UPLOAD_ECO_MS;
				idleWindows = 0;
			}
			else if(deltaIpd > 0)
			{
				uploadMode = UPLOAD_MODE_LOW_LATENCY;
				if(uploadIntervalMs > CLOUD_UPLOAD_FAST_MS + CLOUD_UPLOAD_STEP_MS)
					uploadIntervalMs -= CLOUD_UPLOAD_STEP_MS;
				else
					uploadIntervalMs = CLOUD_UPLOAD_FAST_MS;
				idleWindows = 0;
			}
			else
			{
				idleWindows++;
				if(idleWindows >= NET_IDLE_TO_ECO_WINDOWS)
				{
					uploadMode = UPLOAD_MODE_ECO;
					uploadIntervalMs = CLOUD_UPLOAD_ECO_MS;
				}
				else
				{
					uploadMode = UPLOAD_MODE_BALANCED;
					uploadIntervalMs = CLOUD_UPLOAD_NORMAL_MS;
				}
			}

			lastIpdCnt = curIpdCnt;
			lastCmdFailCnt = curCmdFailCnt;

			if(uploadMode == UPLOAD_MODE_LOW_LATENCY)
				modeStr = "LOW_LAT";
			else if(uploadMode == UPLOAD_MODE_ECO)
				modeStr = "ECO";
			else
				modeStr = "BAL";

			OLED_UpdateNetModeUi(mqttReady, uploadMode);

			UsartPrintf(USART_DEBUG,
				"NET stat tx=%luB rx=%luB ipd=%lu cmd_ok=%lu cmd_fail=%lu up=%ums mode=%s\r\n",
				ESP8266_GetTxBytes(), ESP8266_GetRxBytes(), ESP8266_GetIpdPackets(),
				ESP8266_GetSendCmdOkCount(), ESP8266_GetSendCmdFailCount(), uploadIntervalMs, modeStr);
		}

		DelayMs(MAIN_LOOP_PERIOD_MS);
	}

}
