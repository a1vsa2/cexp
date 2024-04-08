#ifndef __MYCLIENT_H_
#define __MYCLIENT_H_

#include "common.h"

typedef void(*CB_Data_Received)(char*, int);

typedef void(*CB_State_Changed)(int);

typedef struct {
    CB_Data_Received dataReceived;
    CB_State_Changed stateChanged;
} socket_cbs;

void SetCallBacks(socket_cbs* cbs);

int ReadySocket(char* serverIp, int port);

int socketWrite(char* data, int len);

void runReceive();

void mysocket_clean();


#endif // __MYCLIENT_H_