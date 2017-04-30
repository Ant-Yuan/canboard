#ifndef _SYS_MANAGER_H
#define _SYS_MANAGER_H

#include "socketBsp.h"




typedef struct {
  NET_DEV netDev;
}CANBOARD_DEV; //device state struct

extern CANBOARD_DEV CanboardDev;// global param need capital the first character

void* SysManagerThread(void *argv);




#endif

