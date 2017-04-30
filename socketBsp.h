#ifndef _SOCKET_BSP_H
#define _SOCKET_BSP_H
#include <arpa/inet.h>




typedef enum{NET_ERR_NONE	= 0,	//no err
	     NET_ERR_SOCKET	=-1,	//bind err
	     NET_ERR_SERVER	=-2,	//connect to server failed
	     NET_RRR_SEND	=-3,	//send data err
	     NET_ERR_UNKOW	=-4	//unkow err
	      
}NET_ERR_CODE;

typedef enum {NET_NONE		=0,	//init state wait for modem init
	      NET_READY		=1,	//4G modem ready ,need init socket and connect to server
	      NET_CONNECTED	=2,	//socket ok,can do anything to server
	      NET_ERR		=3	//net err
	      
}NET_STATE;

typedef struct {
  struct sockaddr_in 	servAddr;
  NET_STATE 		netState;
  NET_ERR_CODE 		lastErr;
  int 			txMsgid;
  int 			rxMsgid;
  int 			err_cnt;
}NET_DEV;

typedef struct {
    char filename[32];
    int size;
    int numPackage;
} FILE_HEADER;

/*
void* NetTxThread(void *argv);
void* NetRxThread(void *argv);
*/
void *NetTx_Thread(void *args);
void *NetRx_Thread(void *args);

#endif

