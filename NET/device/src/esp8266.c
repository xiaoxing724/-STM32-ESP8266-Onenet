#include "stm32f10x.h"

// 网络设备驱动
#include "esp8266.h"

// 硬件驱动
#include "delay.h"
#include "usart.h"

// C library
#include <string.h>
#include <stdio.h>


#define ESP8266_WIFI_INFO		"AT+CWJAP=\"这里小星哦的Xiaomi 15 Pro\",\"cx070724\"\r\n"

/* 串口收发轮询节拍：减小等待步长可提升消息处理响应 */
#define ESP8266_POLL_STEP_MS		5
#define ESP8266_SENDCMD_TIMEOUT_STEPS	400


unsigned char esp8266_buf[512];
unsigned short esp8266_cnt = 0, esp8266_cntPre = 0;

static volatile unsigned long esp8266_tx_bytes = 0;
static volatile unsigned long esp8266_rx_bytes = 0;
static volatile unsigned long esp8266_ipd_packets = 0;
static volatile unsigned long esp8266_sendcmd_ok = 0;
static volatile unsigned long esp8266_sendcmd_fail = 0;


//==========================================================
// Function: ESP8266_Clear
// Purpose : Clear RX buffer.
// Params  : None.
// Return  : None.
//==========================================================
void ESP8266_Clear(void)
{

	memset(esp8266_buf, 0, sizeof(esp8266_buf));
	esp8266_cnt = 0;

}

//==========================================================
// Function: ESP8266_WaitRecive
// Purpose : Poll until UART RX becomes stable.
// Params  : None.
// Return  : REV_OK when done, REV_WAIT while receiving.
//==========================================================
_Bool ESP8266_WaitRecive(void)
{

	if(esp8266_cnt == 0) 							// 当前无数据，继续等待
		return REV_WAIT;
		
	if(esp8266_cnt == esp8266_cntPre)				// 计数未变化，判定接收结束
	{
		esp8266_cnt = 0;							// Reset counter.
			
		return REV_OK;								// 返回接收完成
	}
		
	esp8266_cntPre = esp8266_cnt;					// 更新基准计数
	
	return REV_WAIT;								// Still receiving.

}

//==========================================================
// Function: ESP8266_SendCmd
// Purpose : Send AT command and wait expected token.
// Params  : cmd command string, res expected token.
// Return  : 0 success, 1 fail.
//==========================================================
_Bool ESP8266_SendCmd(char *cmd, char *res)
{
	
	unsigned short timeOut = ESP8266_SENDCMD_TIMEOUT_STEPS;

	Usart_SendString(USART2, (unsigned char *)cmd, strlen((const char *)cmd));
	
	while(timeOut--)
	{
		if(ESP8266_WaitRecive() == REV_OK)							// 收到完整响应
		{
			if(strstr((const char *)esp8266_buf, res) != NULL)		// Expected token found.
			{
				esp8266_sendcmd_ok++;
				ESP8266_Clear();									// Clear RX buffer.
				
				return 0;
			}
		}
		
		DelayXms(ESP8266_POLL_STEP_MS);
	}

	esp8266_sendcmd_fail++;
	
	return 1;

}

//==========================================================
// Function: ESP8266_SendData
// Purpose : Send payload via transparent mode.
// Params  : data pointer, len bytes.
// Return  : None.
//==========================================================
void ESP8266_SendData(unsigned char *data, unsigned short len)
{

	char cmdBuf[32];
	
	ESP8266_Clear();								// Clear buffer before send.
	sprintf(cmdBuf, "AT+CIPSEND=%d\r\n", len);		// Declare payload length.
	if(!ESP8266_SendCmd(cmdBuf, ">"))				// Send after prompt.
	{
		Usart_SendString(USART2, data, len);		// Write to UART.
		esp8266_tx_bytes += len;
	}

}

//==========================================================
// Function: ESP8266_GetIPD
// Purpose : Locate payload in +IPD response.
// Params  : timeOut poll count.
// Return  : Payload pointer, or NULL on timeout.
//==========================================================
unsigned char *ESP8266_GetIPD(unsigned short timeOut)
{

	char *ptrIPD = NULL;
	
	do
	{
		if(ESP8266_WaitRecive() == REV_OK)								// 接收完成
		{
			ptrIPD = strstr((char *)esp8266_buf, "IPD,");				// Find IPD header.
			if(ptrIPD == NULL)											// 未找到则继续等待
			{
				//UsartPrintf(USART_DEBUG, "\"IPD\" not found\r\n");
			}
			else
			{
				ptrIPD = strchr(ptrIPD, ':');							// 定位 ':'
				if(ptrIPD != NULL)
				{
					ptrIPD++;
					esp8266_ipd_packets++;
					return (unsigned char *)(ptrIPD);
				}
				else
					return NULL;
				
			}
		}
		
		DelayXms(ESP8266_POLL_STEP_MS);									// Short polling delay.
	} while(timeOut--);
	
	return NULL;														// Timeout.

}

//==========================================================
// Function: ESP8266_Init
// Purpose : Initialize ESP8266 and connect WiFi.
// Params  : None.
// Return  : None.
//==========================================================
void ESP8266_Init(void)
{
	unsigned char retry;
	_Bool ok;
	
	ESP8266_Clear();
	
	UsartPrintf(USART_DEBUG, "1. AT\r\n");
	ok = 0;
	for(retry = 0; retry < 10; retry++)
	{
		if(ESP8266_SendCmd("AT\r\n", "OK") == 0)
		{
			ok = 1;
			break;
		}
		DelayXms(300);
	}
	if(ok == 0)
	{
		UsartPrintf(USART_DEBUG, "1. AT timeout\r\n");
		return;
	}
	
	UsartPrintf(USART_DEBUG, "2. CWMODE\r\n");
	ok = 0;
	for(retry = 0; retry < 10; retry++)
	{
		if(ESP8266_SendCmd("AT+CWMODE=1\r\n", "OK") == 0)
		{
			ok = 1;
			break;
		}
		DelayXms(300);
	}
	if(ok == 0)
	{
		UsartPrintf(USART_DEBUG, "2. CWMODE timeout\r\n");
		return;
	}
	
	UsartPrintf(USART_DEBUG, "3. AT+CWDHCP\r\n");
	ok = 0;
	for(retry = 0; retry < 10; retry++)
	{
		if(ESP8266_SendCmd("AT+CWDHCP=1,1\r\n", "OK") == 0)
		{
			ok = 1;
			break;
		}
		DelayXms(300);
	}
	if(ok == 0)
	{
		UsartPrintf(USART_DEBUG, "3. CWDHCP timeout\r\n");
		return;
	}
	
	UsartPrintf(USART_DEBUG, "4. CWJAP\r\n");
	ok = 0;
	for(retry = 0; retry < 20; retry++)
	{
		if(ESP8266_SendCmd(ESP8266_WIFI_INFO, "GOT IP") == 0)
		{
			ok = 1;
			break;
		}
		DelayXms(500);
	}
	if(ok == 0)
	{
		UsartPrintf(USART_DEBUG, "4. CWJAP timeout\r\n");
		return;
	}
	
	UsartPrintf(USART_DEBUG, "5. ESP8266 Init OK\r\n");

}

//==========================================================
// Function: USART2_IRQHandler
// Purpose : USART2 RX interrupt handler.
// Params  : None.
// Return  : None.
//==========================================================
void USART2_IRQHandler(void)
{

	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) // 接收中断
	{
		if(esp8266_cnt >= sizeof(esp8266_buf))	esp8266_cnt = 0; // 防止越界
		esp8266_buf[esp8266_cnt++] = USART2->DR;
		esp8266_rx_bytes++;
		
		USART_ClearFlag(USART2, USART_FLAG_RXNE);
	}

}

unsigned long ESP8266_GetTxBytes(void)
{
	return esp8266_tx_bytes;
}

unsigned long ESP8266_GetRxBytes(void)
{
	return esp8266_rx_bytes;
}

unsigned long ESP8266_GetIpdPackets(void)
{
	return esp8266_ipd_packets;
}

unsigned long ESP8266_GetSendCmdOkCount(void)
{
	return esp8266_sendcmd_ok;
}

unsigned long ESP8266_GetSendCmdFailCount(void)
{
	return esp8266_sendcmd_fail;
}

void ESP8266_ResetStats(void)
{
	esp8266_tx_bytes = 0;
	esp8266_rx_bytes = 0;
	esp8266_ipd_packets = 0;
	esp8266_sendcmd_ok = 0;
	esp8266_sendcmd_fail = 0;
}
