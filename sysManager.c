/**
 * manage sdcard state,can state and 4g state
 */

#include "sysManager.h"


CANBOARD_DEV CanboardDev;

void* SysManagerThread(void *argv)
{
  while(1)
  {
    switch(CanboardDev.netDev.netState)
    {	
      case NET_NONE://do nothing,wait for modem init
	//modem_init();
	CanboardDev.netDev.netState=NET_READY;	
	break;
      default:
	break;
    }
    usleep(200000);
  }
}




