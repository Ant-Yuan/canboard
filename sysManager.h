#ifndef _SYS_MANAGER_H
#define _SYS_MANAGER_H

#include "socketBsp.h"


#define CAN_MSG_CHECK_START	0u	
#define CAN_MSG_CHECK_OK	1u	
#define CAN_MSG_CHECK_FAILED	2u
#define SD_MSG_FILE_PATH	3u


typedef	struct 
{
  long type;
  char msg[255];
}MsgBuf;

typedef enum {
  SD_ERR_NONE		= 0,	//no err
  SD_ERR_WRITE		=-1,	//write file err
  SD_ERR_READ		=-2,	//read file err
  SD_RRR_FORMAT		=-3,	//format err
  SD_ERR_UNKOW		=-4	//unkow err	      
}SD_ERR_CODE;

typedef enum {
  SD_NONE			=0,	//need mount sd card
  SD_READY			=1,	//mount success
  SD_FORMAT			=2,	//format sd card
  SD_ERR				=3	//no sd card or other error
}SD_STATE;


typedef enum {
  SYS_COLLECTION	= 3,//in collection
  SYS_CHECK_CHILD	= 2,
  SYS_READY		= 1,//init success
  SYS_NOT_INIT		= 0,
  
  SYS_SD_ERR	=-1,
  SYS_TEMP_ERR	=-2,
  SYS_CAN_ERR	=-3,
  SYS_NET_ERR	=-4,
	      
  
}SysStatus;

typedef struct {
  NET_DEV netDev;
}CANBOARD_DEV; //device state struct

extern SysStatus SystemState;
extern int CanMsgId;
extern int SdMsgId;
extern char SD_Path[255];//SD logical drive path




extern CANBOARD_DEV CanboardDev;// global param need capital the first character

void* SysManagerThread(void *argv);




#endif

