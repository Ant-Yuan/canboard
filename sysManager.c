/**
 * manage sdcard state,can state and 4g state
 */
#include "sysManager.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>

CANBOARD_DEV CanboardDev;
int SD_handler(void);
void test_plug(void);

void* SysManagerThread(void *argv)
{
  while(1)
  {
    //SD_handler();
    switch(CanboardDev.netDev.netState)
    {	
      case NET_NONE://do nothing,wait for modem init
	//modem_init();
	CanboardDev.netDev.netState=NET_READY;	
	break;
      default:
	break;
    }
    //usleep(200000);
    sleep(10);
  }
}

//TODO manage it by inturrpt
int SDCard_Manager(void)
{
  char cmd_mount[]="mount -t vfat /dev/mmcblk0p1 ~/udisk";
  
  FILE *fstream=NULL;
  char buff[100]={0};
  
  fstream=popen(cmd_mount,"r");
  
   while (NULL!=fgets(buff, sizeof(buff), fstream))
   {
     printf("%s",buff);
  }
  pclose(fstream);
  return 0; 
} 








