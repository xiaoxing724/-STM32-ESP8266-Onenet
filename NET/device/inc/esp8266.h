#ifndef _ESP8266_H_
#define _ESP8266_H_





#define REV_OK		0	//������ɱ�־
#define REV_WAIT	1	//����δ��ɱ�־


void ESP8266_Init(void);

void ESP8266_Clear(void);

_Bool ESP8266_SendCmd(char *cmd, char *res);

void ESP8266_SendData(unsigned char *data, unsigned short len);

unsigned char *ESP8266_GetIPD(unsigned short timeOut);

unsigned long ESP8266_GetTxBytes(void);

unsigned long ESP8266_GetRxBytes(void);

unsigned long ESP8266_GetIpdPackets(void);

unsigned long ESP8266_GetSendCmdOkCount(void);

unsigned long ESP8266_GetSendCmdFailCount(void);

void ESP8266_ResetStats(void);


#endif
