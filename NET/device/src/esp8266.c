#include "stm32f10x.h"

//�����豸����
#include "esp8266.h"

//Ӳ������
#include "delay.h"
#include "usart.h"

//C��
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
//	�������ƣ�	ESP8266_Clear
//
//	�������ܣ�	��ջ���
//
//	��ڲ�����	��
//
//	���ز�����	��
//
//	˵����		
//==========================================================
void ESP8266_Clear(void)
{

	memset(esp8266_buf, 0, sizeof(esp8266_buf));
	esp8266_cnt = 0;

}

//==========================================================
//	�������ƣ�	ESP8266_WaitRecive
//
//	�������ܣ�	�ȴ��������
//
//	��ڲ�����	��
//
//	���ز�����	REV_OK-�������		REV_WAIT-���ճ�ʱδ���
//
//	˵����		ѭ�����ü���Ƿ�������
//==========================================================
_Bool ESP8266_WaitRecive(void)
{

	if(esp8266_cnt == 0) 							//������ռ���Ϊ0 ��˵��û�д��ڽ��������У�����ֱ����������������
		return REV_WAIT;
		
	if(esp8266_cnt == esp8266_cntPre)				//�����һ�ε�ֵ�������ͬ����˵���������
	{
		esp8266_cnt = 0;							//��0���ռ���
			
		return REV_OK;								//���ؽ�����ɱ�־
	}
		
	esp8266_cntPre = esp8266_cnt;					//��Ϊ��ͬ
	
	return REV_WAIT;								//���ؽ���δ��ɱ�־

}

//==========================================================
//	�������ƣ�	ESP8266_SendCmd
//
//	�������ܣ�	��������
//
//	��ڲ�����	cmd������
//				res����Ҫ���ķ���ָ��
//
//	���ز�����	0-�ɹ�	1-ʧ��
//
//	˵����		
//==========================================================
_Bool ESP8266_SendCmd(char *cmd, char *res)
{
	
	unsigned short timeOut = ESP8266_SENDCMD_TIMEOUT_STEPS;

	Usart_SendString(USART2, (unsigned char *)cmd, strlen((const char *)cmd));
	
	while(timeOut--)
	{
		if(ESP8266_WaitRecive() == REV_OK)							//����յ�����
		{
			if(strstr((const char *)esp8266_buf, res) != NULL)		//����������ؼ���
			{
				esp8266_sendcmd_ok++;
				ESP8266_Clear();									//��ջ���
				
				return 0;
			}
		}
		
		DelayXms(ESP8266_POLL_STEP_MS);
	}

	esp8266_sendcmd_fail++;
	
	return 1;

}

//==========================================================
//	�������ƣ�	ESP8266_SendData
//
//	�������ܣ�	��������
//
//	��ڲ�����	data������
//				len������
//
//	���ز�����	��
//
//	˵����		
//==========================================================
void ESP8266_SendData(unsigned char *data, unsigned short len)
{

	char cmdBuf[32];
	
	ESP8266_Clear();								//��ս��ջ���
	sprintf(cmdBuf, "AT+CIPSEND=%d\r\n", len);		//��������
	if(!ESP8266_SendCmd(cmdBuf, ">"))				//�յ���>��ʱ���Է�������
	{
		Usart_SendString(USART2, data, len);		//�����豸������������
		esp8266_tx_bytes += len;
	}

}

//==========================================================
//	�������ƣ�	ESP8266_GetIPD
//
//	�������ܣ�	��ȡƽ̨���ص�����
//
//	��ڲ�����	�ȴ���ʱ��(����10ms)
//
//	���ز�����	ƽ̨���ص�ԭʼ����
//
//	˵����		��ͬ�����豸���صĸ�ʽ��ͬ����Ҫȥ����
//				��ESP8266�ķ��ظ�ʽΪ	"+IPD,x:yyy"	x�������ݳ��ȣ�yyy����������
//==========================================================
unsigned char *ESP8266_GetIPD(unsigned short timeOut)
{

	char *ptrIPD = NULL;
	
	do
	{
		if(ESP8266_WaitRecive() == REV_OK)								//����������
		{
			ptrIPD = strstr((char *)esp8266_buf, "IPD,");				//������IPD��ͷ
			if(ptrIPD == NULL)											//���û�ҵ���������IPDͷ���ӳ٣�������Ҫ�ȴ�һ�ᣬ�����ᳬ���趨��ʱ��
			{
				//UsartPrintf(USART_DEBUG, "\"IPD\" not found\r\n");
			}
			else
			{
				ptrIPD = strchr(ptrIPD, ':');							//�ҵ�':'
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
		
		DelayXms(ESP8266_POLL_STEP_MS);									//��ʱ�ȴ�
	} while(timeOut--);
	
	return NULL;														//��ʱ��δ�ҵ������ؿ�ָ��

}

//==========================================================
//	�������ƣ�	ESP8266_Init
//
//	�������ܣ�	��ʼ��ESP8266
//
//	��ڲ�����	��
//
//	���ز�����	��
//
//	˵����		
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
//	�������ƣ�	USART2_IRQHandler
//
//	�������ܣ�	����2�շ��ж�
//
//	��ڲ�����	��
//
//	���ز�����	��
//
//	˵����		
//==========================================================
void USART2_IRQHandler(void)
{

	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET) //�����ж�
	{
		if(esp8266_cnt >= sizeof(esp8266_buf))	esp8266_cnt = 0; //��ֹ���ڱ�ˢ��
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
