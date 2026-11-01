#ifndef ETHERNET_OTA_TCP_SERVER_H_
#define ETHERNET_OTA_TCP_SERVER_H_

int OTA_Client_Init(void);
void OTA_Client_Task(void);
void ota_start (void);
void ota_stop(void);

#endif /* ETHERNET_TCP_CLIENT_H_ */
