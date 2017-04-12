/*说明：输出系统日志文件到SD卡里
 * 
 * 创建文件:20170412 yyj
 */
#include	<unistd.h>
#include	<pthread.h>
#include <stdio.h>
#include "logBsp.h"

int logCreate(char * filename);


int logCreate(char * filename)
{
  
  return 0;
}

void logPrint(char *str)
{
  
}

void* LogThread(void *argv)
{
  int i=0;
  while(1)
  {
    printf("log thread %d\n",i++);
    sleep(1);//线程休眠1s
  }
}
