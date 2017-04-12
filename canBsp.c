/*说明：读写can数据，并将CAN数据写到SD卡里
 * 创建文档：20170412 yyj
 */ 
#include <unistd.h>
#include <stdio.h>
#include "canBsp.h"
#include "logBsp.h"


int canInit(void)
{
  return 0;
}

void* CanTxThread(void *argv)
{
  int i=0;
  while(1)
  {
    printf("can tx thread %d\n",i++);
    usleep(500*1000);//线程休眠500ms
  }
}

void* CanRxThread(void *argv)
{
  int i=0;
  while(1)
  {
    printf("can rx thread %d\n",i++);
    usleep(500*1000);//线程休眠500ms
  }
}
