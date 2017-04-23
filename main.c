/* 说明：main函数，初始化参数及创建线程任务
 * 创建文档：20170412 yyj
 * */
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "canBsp.h"
#include "socketBsp.h"
#include "logBsp.h"


// test whether sd card is full

char file2send_filepath[4096]; // 4096 is the max length of path in ubuntu


#define version 20170412

int sysInit(void);

pthread_t NetTxThreadId;
pthread_t NetRxThreadId;
pthread_t CanTxThreadId;
pthread_t CanRxThreadId;
pthread_t LogThreadId;

int main(int argc, char **argv) {
  int err;
  printf("version :%d\n",version);
  sysInit();
  
  //创建CAN发送线程
  err = pthread_create(&CanTxThreadId, NULL, &CanTxThread, NULL);
  if(err!=0)
  {
    printf("Create can tx thread error!\n");  
    return -1;
  }
  
  //创建CAN接收线程
  err = pthread_create(&CanRxThreadId, NULL, &CanRxThread, NULL);
  if(err!=0)
  {
    printf("Create can rx thread error!\n");  
    return -1;
  }
  
  //创建网络接收线程
  err = pthread_create(&NetTxThreadId, NULL, &NetTxThread, NULL);
  if(err!=0){
    printf("Create net tx Thread error!\n");  
    return -1;
  }
  
  //创建网络发送线程
  err = pthread_create(&NetRxThreadId, NULL, &NetRxThread, NULL);
  if(err!=0){
    printf("Create net rx Thread error!\n");  
    return -1;
  }
  
  //创建Log线程
  err = pthread_create(&LogThreadId, NULL, &LogThread, NULL);
  if(err!=0)
  {
    printf("Create log thread error!\n");  
    return -1;
  }
  
  pthread_join(NetTxThreadId,NULL);
  pthread_join(NetRxThreadId,NULL);
  pthread_join(CanTxThreadId,NULL);
  pthread_join(CanRxThreadId,NULL);
  pthread_join(LogThreadId,NULL);
  
  return 0;
}

int sysInit(void)
{
  return 0;
}
