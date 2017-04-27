/*说明：读写can数据，并将CAN数据写到SD卡里
 * 创建文档：20170412 yyj
 * Added by yanjia: one sub-directory for one single day.
 * and the newly created file is named like f1002.bin
 * To test this file, you should run /home/tony-yan/Demos/CAN_TEST/can_send
 * and debug on this project. This file will create a sub-directory within the current working directory within which create new file to write. 
 */
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <net/if.h>
#include <time.h>
#include <sys/stat.h>
#include "canBsp.h"
#include "logBsp.h"
#include <sys/msg.h>
#include <errno.h>
#include <stdlib.h>
#include <bits/signum.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <sys/socket.h>
#include <signal.h>
#include "sysManager.h"

#define INTERFACE "vcan0"
#define BUFFSIZE 4096 // buffer size for writing to sd
#define FRAME_PER_FILE 20
#define DIR_NAME_FMT "%d-%d-%d" // day-mon-year
#define PATH_BUFFER_LENGTH 128
#define DIR_BUFFER_LENGTH 10
#define FILENAME_BUFFER_LENGTH 20
#define MSGSZ PATH_BUFFER_LENGTH
#define CAN_DUMP_PATH_PREFIX "./" // Change to /media/udisk/ when deployed


long fcnt = 1; // file created so far count


int fdCAN0;
int fdCAN1;
ChildNodeInfo CnodeInfo;

void checkChildOnline(void);
void broadcastCollection(void);
int openCanSocket(const char *canInterfaceName);
int canCheckDataHandle(MsgBuf* msg,struct can_frame *pFrame);

int canInit(void)
{
  //make sence in real board
  system("ip link set can0 down");
  system("ip link set can1 down");
  system("ip link set can0 up type can bitrate 800000");
  system("ip link set can1 up type can bitrate 800000");
  
  printf("init can bps 800k\n");
  
  fdCAN0=openCanSocket(CAN_NAME_CAN0);
  fdCAN1=openCanSocket(CAN_NAME_CAN1);
  
  return 0;
}

int openCanSocket(const char *canInterfaceName)
{
  // CAN connection variables
  struct sockaddr_can addr;
  struct ifreq ifr;
  int sockfd=-1;
  // Open the CAN network interface
  sockfd = socket(PF_CAN, SOCK_RAW|SOCK_NONBLOCK, CAN_RAW);
  if (-1 == sockfd)
  {
    printf("create socket error!\n");
    return -1;
  }
  // Get the index of the network interface
  strcpy(ifr.ifr_name, canInterfaceName);
  if (ioctl(sockfd, SIOCGIFINDEX, &ifr) == -1)
  {
    printf("error get the index of interface!\n");
    return -2;
  }
  // Bind the socket to the network interface
  addr.can_family = AF_CAN;
  addr.can_ifindex = ifr.ifr_ifindex;
  if(bind(sockfd,(struct sockaddr*)(&addr),sizeof(addr))<0)
  {
    printf("error bind the socket interface!\n");
    return -3;
  }
  return sockfd;
}

void* CanTxThread(void *argv)
{
  int i=0;
  
  sleep(10);
  
  broadcastCollection();
  SystemState=SYS_COLLECTION;
  
  while(1)
  {
    // printf("can tx thread %d\n",i++);
    usleep(500*1000);//线程休眠500ms
  }
}

// write the frame to the file
void processFrame(const struct can_frame *frame, FILE *out)
{
    
    if (out)
    {
        /* get file written to pointed by out */
        char path[PATH_BUFFER_LENGTH];
        char result[1024];
        int fd = fileno(out);
        sprintf(path, "/proc/self/fd/%d", fd);
        memset(result, 0, sizeof(result));
        readlink(path, result, sizeof(result)-1);

        /* Print the result. */
        printf("writing to %s\n", result);
        fwrite(frame, sizeof(frame), 1, out);
    }
    else
    {
        printf("NULL pointer passed in processFrame\n");
    }
}

void setDirName(struct tm *date, char *buffer)
{
    memset(buffer, 0, DIR_BUFFER_LENGTH);
    time_t seconds = time(NULL);
    struct tm curDate = *localtime(&seconds);
    sprintf(buffer, DIR_NAME_FMT, curDate.tm_mday, curDate.tm_mon+1, curDate.tm_year+1900);
}

void setFileName(char *buffer)
{
    memset(buffer, 0, FILENAME_BUFFER_LENGTH);
    struct tm *date;
    time_t seconds = time(NULL);
    date = localtime(&seconds);
    //sprintf(buffer, DIR_NAME_FMT, date->tm_mday, date->tm_mon, date->tm_year);
    sprintf(buffer, "f%ld.bin", fcnt++);
    printf("set filename %s\n", buffer);
}

FILE* newFileForWrite(FILE *out, char *filenamebuf, char *dirbuf, char *pathbuf)
{
    if (out){
        fclose(out);
    }
    setFileName(filenamebuf);
    memset(pathbuf, 0, PATH_BUFFER_LENGTH);
    // Add the can dump path prefix
    strcat(pathbuf, CAN_DUMP_PATH_PREFIX);
    strcat(pathbuf, dirbuf);
    strcat(pathbuf, "/");
    strcat(pathbuf, filenamebuf);
    out = fopen(pathbuf, "wb");
    if(out == NULL) { /* error handle */}
    
    // write date and TEMPERATURE to file
    time_t seconds = time(NULL);
    fwrite(&seconds, sizeof(time_t), 1, out);
    // WRITE TEMPERATURE TO FILE HERE
    
    return out;
}

void updateDir(struct tm *date, char *dirBuffer)
{
    setDirName(date, dirBuffer);
    struct stat sb;
    if (stat(dirBuffer, &sb) == 0 && S_ISDIR(sb.st_mode))
    {
        // foler exists
    }
    else
    {
        int status = mkdir(dirBuffer, S_IRWXU | S_IRWXG | S_IRWXO);
        if (status != 0) { /* error handle */}
    }
}

// send socketBsp of the path
// for it to send the file to server
/*
void notifySocket2Send(char *filepath, int msgid)
{
    if (msgid == -1)
    {
        fprintf(stderr, "msgget failed with error: %d\n", errno);  
        exit(EXIT_FAILURE);
    }
    struct msgbuf data;
    data.mtype = 1;
    strcpy(data.mtext, filepath);
    if(msgsnd(msgid, (void*)&data, MSGSZ, 0) == -1)
    {
        fprintf(stderr, "msgsnd failed\n");  
        exit(EXIT_FAILURE);  
    }
}*/

void* CanRxThread(void *argv)
{
  MsgBuf msg;
  int childId=0;
  struct can_frame frame;
  ssize_t nbr=CAN_MTU;
  int i=0;
  
  while(1)
  {
    switch(SystemState)
    {
      case SYS_CHECK_CHILD:
	//nbr=read(fdCAN0, &frame, CAN_MTU);
	//if(nbr==CAN_MTU)
	//{
	//  if(msgrcv(CanMsgId,(void*)&msg,1,CAN_MSG_CHECK_START,IPC_NOWAIT))
	//  {
	//    canCheckDataHandle(&msg,&frame);
	//  }
	//}
	break;
      case SYS_COLLECTION:
	nbr=read(fdCAN0, &frame, CAN_MTU);
	if(nbr==CAN_MTU)
	{
	  for(i=0;i<frame.can_dlc;i++)
	    printf("%d",frame.data[i]);
	  printf("\n");
	}
	nbr=read(fdCAN1, &frame, CAN_MTU);
	if(nbr==CAN_MTU)
	{
	  for(i=0;i<frame.can_dlc;i++)
	    printf("%d",frame.data[i]);
	  printf("\n");
	}
	break;
    }
    usleep(50*1000);
  }
}

int canCheckDataHandle(MsgBuf* msg,struct can_frame *pFrame)
{
  int childId=0;
  if(msg->type==CAN_MSG_CHECK_START)
  {
    childId=msg->msg[0];
    if(pFrame->data[0]==MSG_CHECK&&pFrame->data[1]==childId)
    {
      msg->type=CAN_MSG_CHECK_OK;
      msgsnd(CanMsgId,(void*)msg,1,0);
      return 0;
    }
  }
  return -1;
}


void broadcastCollection(void)
{
  struct can_frame frame;
  ssize_t numBytes=0;
  
  frame.can_id=ROOT_CAN_STD_ID;
  frame.can_dlc=2;
  frame.data[0]=MSG_SYNC;
  frame.data[1]=CHILD_DEV_BROADCAST;//child dev number
  
  numBytes = write(fdCAN0, &frame, CAN_MTU);
  if(numBytes!=CAN_MTU)
  {
    printf("write broadcastCollection can0 failed!\n");
  }
  
  numBytes = write(fdCAN1, &frame, CAN_MTU);
  if(numBytes!=CAN_MTU)
  {
    printf("write broadcastCollection can1 failed!\n");
  }
}

void checkChildOnline(void)
{
  uint16_t currChildId;
  struct can_frame frame;
  int i=0;
  ssize_t numBytes=0;
  MsgBuf msg;
  
  CnodeInfo.devFreq=FREQ_200HZ;
  
  SystemState=SYS_CHECK_CHILD;
  
  //need wait for child node ready
  sleep(1);
  
  //0x0-0x0F 使用CAN1 child_node=H0 0x10-0x1F使用CAN2 child_node=h1
  for(i=0;i<MAX_CHILD_DEV_NUM;i++)
  {
    currChildId=i;
    frame.can_id=ROOT_CAN_STD_ID;
    frame.can_dlc=3;
    frame.data[0]=MSG_CHECK;
    frame.data[1]=i+CHILD_DEV_START;//child dev number
    frame.data[2]=CnodeInfo.devFreq;
    
    if(i<0x10)
    {
      numBytes = write(fdCAN0, &frame, CAN_MTU);
      if(numBytes!=CAN_MTU)
	printf("check can0 child node failed!\n");
    }
    else
    {
      numBytes = write(fdCAN1, &frame, CAN_MTU);
      if(numBytes!=CAN_MTU)
	printf("check can1 child node failed!\n");
    }
    //tell the rx thread to recive 
    msg.type=CAN_MSG_CHECK_START;
    msg.msg[0]=i+CHILD_DEV_START;
    
    msgsnd(CanMsgId, (void*)&msg, 1, IPC_NOWAIT);
    sleep(1);

    /*
    if(msgrcv(CanMsgId,(void*)&msg,sizeof(MsgBuf),CAN_MSG_CHECK_OK,0))
    {
      printf("msg ?\n");
      if(msg.type==CAN_MSG_CHECK_OK)
	printf("msg ok\n");
    }
    else
    {
      printf("msg timeout\n");
    }
    */
  }
    
    /*
    if(evt.status==osEventMessage&&evt.value.v==MSG_CHECK)
    {
	    cNodeInfo.childStatus[i]=CHILD_OK;
    }
    else//osEventTimeout
    {
	    cNodeInfo.childStatus[i]=CHILD_OFFLINE;
    }
  }
      
      
      for(int i=0;i<MAX_CHILD_DEV_NUM;i++)
      {
	      if(cNodeInfo.childStatus[i]!=CHILD_OK)
	      {
		      SystemState=SYS_CAN_ERR;
	      }
      }
      if(SystemState!=SYS_CAN_ERR)
      {
	      SystemState=SYS_OK;
      }
	
	__HAL_CAN_DISABLE_IT(&hcan1, CAN_IT_FMP0);
	__HAL_CAN_DISABLE_IT(&hcan2, CAN_IT_FMP0);
	
	hcan1.pTxMsg->Data[0]=MSG_SYNC;						//¿ªÊ¼²ÉÑù
	hcan1.pTxMsg->Data[1]=CHILD_DEV_BROADCAST;//ËùÓÐÉè±¸
	
	hcan2.pTxMsg->Data[0]=MSG_SYNC;						//¿ªÊ¼²ÉÑù
	hcan2.pTxMsg->Data[1]=CHILD_DEV_BROADCAST;//ËùÓÐÉè±¸
	
	HAL_CAN_Transmit(&hcan1,MAX_CAN_TX_TIMEOUT);
	HAL_CAN_Transmit(&hcan1,MAX_CAN_TX_TIMEOUT);
	HAL_CAN_Transmit(&hcan2,MAX_CAN_TX_TIMEOUT);
	HAL_CAN_Transmit(&hcan2,MAX_CAN_TX_TIMEOUT);
	
	__HAL_CAN_ENABLE_IT(&hcan1, CAN_IT_FMP0);
	__HAL_CAN_ENABLE_IT(&hcan2, CAN_IT_FMP0);
	*/
}


void* CanRxThread1(void *argv)
{
    
    // int i=0;
    int rc;
    
    key_t key;
    key = 1234;
    int msgid = -1;
    int msgflg = IPC_CREAT | 0666;
    if ((msgid = msgget(key, msgflg )) < 0) {
        printf("msgget failed from canBsp.c\n");
        perror("msgget");
        exit(1);
    }
  
    // CAN connection variables
    struct sockaddr_can addr;
    struct ifreq ifr;
    int sockfd;
    
    // Open the CAN network interface
    sockfd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (-1 == sockfd) {
        printf("create socket error in CanRxThread!\n");
        _exit(1);
    }
        
    // Get the index of the network interface
    strncpy(ifr.ifr_name, INTERFACE, IFNAMSIZ);
    if (ioctl(sockfd, SIOCGIFINDEX, &ifr) == -1) {
        printf("error when get the index of interface!\n");
        _exit(1);
    }
    
    // Bind the socket to the network interface
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    rc = bind(
        sockfd,
        (struct sockaddr*)(&addr),
        sizeof(addr)
    );
    if (-1 == rc) {
        printf("error when bind the socket to dirthe network interface!\n");
        _exit(1);
    }
    FILE *out = NULL;
    struct tm *today;
    char dir[DIR_BUFFER_LENGTH];
    char path[PATH_BUFFER_LENGTH];
    char filename[FILENAME_BUFFER_LENGTH];
    struct stat sb;
    int frameCount = 0;
    char sameDay = 0;
    time_t seconds = time(NULL);
    today = localtime(&seconds);
    updateDir(today, dir);
    out = newFileForWrite(out, filename, dir, path);
    while(1)
    {
        // printf("can rx thread %d\n",i++);
        struct can_frame frame;
        // read in a can frame
        ssize_t numBytes = read(sockfd, &frame, CAN_MTU);
        switch (numBytes) {
            case CAN_MTU:
                // check if it's the same day
                seconds = time(NULL);
                struct tm *newDay = localtime(&seconds);
                newDay = localtime(&seconds);
                sameDay = newDay->tm_mday == today->tm_mday;
                if (!sameDay) {
                    // update the dir 
                    today = newDay;
                    updateDir(today, dir);
                    
                    // close current file and open a new file to write
                    char pathTemp[PATH_BUFFER_LENGTH] = {0};
                    strcpy(pathTemp, path);
                    out = newFileForWrite(out, filename, dir, path);
                    
                    //notifySocket2Send(pathTemp, msgid);
                }
                
                // check whether current file has been full
                if (frameCount > FRAME_PER_FILE) {
                    frameCount = 0;
                    fclose(out);
                    out = NULL;
                
                    // close the file and open a new file to write
                    char pathTemp[PATH_BUFFER_LENGTH] = {0};
                    strcpy(pathTemp, path);
                    out = newFileForWrite(out, filename, dir, path);
                    //notifySocket2Send(pathTemp, msgid);
                }
                
                processFrame(&frame, out);
                frameCount++;
                printf("frameCount:%d\n", frameCount);
                break;
            case -1:
                //error handle: signal and delay before continue
            default:
                continue;
                
        }
        usleep(500*1000);//线程休眠500ms
    }
    
    // cleanup
    if (close(sockfd) == -1) {
        printf("error when closing the socket!\n");
        _exit(1);
    }
}


