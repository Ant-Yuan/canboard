#ifndef _SYS_MANAGER_H
#define _SYS_MANAGER_H

#include "socketBsp.h"


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



typedef struct {
  NET_DEV netDev;
}CANBOARD_DEV; //device state struct

extern CANBOARD_DEV CanboardDev;// global param need capital the first character

void* SysManagerThread(void *argv);




#endif

