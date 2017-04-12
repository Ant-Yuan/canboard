#include <unistd.h>
#include <stdio.h>
#include "socketBsp.h"
#include "logBsp.h"



void* NetTxThread(void *argv)
{
  int i=0;
  while(1)
  {
    printf("net tx thread %d\n",i++);
    usleep(500*1000);//线程休眠500ms
  }
}

void* NetRxThread(void *argv)
{
  int i=0;
  while(1)
  {
    printf("net rx thread %d\n",i++);
    usleep(500*1000);//线程休眠500ms
  }
}