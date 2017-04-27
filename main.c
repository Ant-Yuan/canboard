/* 说明：main函数，初始化参数及创建线程任务
 * 创建文档：20170412 yyj
 * */
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "canBsp.h"
#include "socketBsp.h"
#include "logBsp.h"
#include "sysManager.h"
#include <sys/msg.h>

// test whether sd card is full

char file2send_filepath[4096]; // 4096 is the max length of path in ubuntu

#define version 20170427

int sysInit(void);
int msgQueueInit(void);

pthread_t SysManagerThreadId;
pthread_t NetTxThreadId;
pthread_t NetRxThreadId;
pthread_t CanTxThreadId;
pthread_t CanRxThreadId;
pthread_t LogThreadId;

int main(int argc, char **argv) {
  int err;
  printf("version :%d\n",version);
  sysInit();
  
  /*
  err = pthread_create(&SysManagerThreadId, NULL, &SysManagerThread, NULL);
  if(err!=0)
  {
    printf("Create SysManager Thread error!\n");  
    return -1;
  }*/
 
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
    printf("12");
    printf("Create can rx thread error!\n");  
    return -1;
  }
  /*
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
  }*/
  
  //pthread_join(SysManagerThreadId,NULL);
  //pthread_join(NetTxThreadId,NULL);
  //pthread_join(NetRxThreadId,NULL);
  pthread_join(CanTxThreadId,NULL);
  pthread_join(CanRxThreadId,NULL);
  //pthread_join(LogThreadId,NULL);
  while(1)
  {
  }
  return 0;
}

int sysInit(void)
{
  SystemState=SYS_NOT_INIT;
  canInit();
  msgQueueInit();
  
  SystemState=SYS_READY;
  
  return 0;
}

int msgQueueInit(void)
{
  //remove msg queue
  CanMsgId=-1;
  SdMsgId=-1;
  
  //0660表示用户和同组用户有读写权限，其他用户没有任何访问权限。 
  CanMsgId = msgget((key_t)MSGQUEUE_CAN, (IPC_CREAT|0666));
  SdMsgId = msgget((key_t)MSGQUEUE_SD, (IPC_CREAT|0666));
  
  if(CanMsgId<0||SdMsgId<0)
  {
    printf("create msg queue failed!\n");
    return -1;
  }
  else
    return 0;
}








