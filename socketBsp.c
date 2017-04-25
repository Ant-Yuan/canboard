#include <unistd.h>
#include "socketBsp.h"
#include "logBsp.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include "canBsp.h"
#include <sys/msg.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include "sysManager.h"


#define PATH_BUFFER_LENGTH 128
#define MSGSZ PATH_BUFFER_LENGTH
#define SOCK_BUF_SIZE 1025

typedef struct msgbuf {
    long    mtype;
    char    mtext[MSGSZ];
} message_buf;



//yyj add tx demo
void *NetTx_Thread(void *args)
{
  while(1)
  {
    switch(CanboardDev.netDev.netState)
    {
      case NET_NONE://do nothing,wait for modem init		
	break;
      case NET_READY:
	//need init socket and connect to server
	//if(success)
	// CanboardDev.netDev.netState=NET_CONNECTED;
	//else
	// CanboardDev.netDev.netState=NET_ERR;
	break;
      case NET_CONNECTED:
	//wait file ok msg
	//send package to server
	//recive server ack msg
	//send remain package
	//send EOF file package
	//if send failed in 10 timers goto net err
	break;
      case NET_ERR:
	//wite err log
	//if (connect to server err 5 timers )
	//   reset modem CanboardDev.netDev.netState=NONE;
	//else 
	//    reconnect to server 
	break;
      default:
	break;
    }
    usleep(200000);//200ms
  }
 
}

//yyj add rx demo
void *NetRx_Thread(void *args)
{
  while(1)
  {
    switch(CanboardDev.netDev.netState)
    {
      case NET_CONNECTED:
	//recive server msg
	//if need put msg to other Thread
	break;
      default:
	break;
    }
    usleep(200000);//200ms
  }
  
}


// return socket file descriptor
int initSocket(const char *ip, int port)
{
    int sockfd;
    struct sockaddr_in servaddr;
    if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        printf("create socket error: %s(errno: %d)\n", strerror(errno),errno);
        exit(0);
    }
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    if( inet_pton(AF_INET, ip, &servaddr.sin_addr) <= 0){
        printf("inet_pton error for %s\n",ip);
        exit(1);
    }

    if( connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
        printf("connect error: %s(errno: %d)\n",strerror(errno),errno);
        exit(0);
    }
    return sockfd;
}

void* NetTxThread(void *argv)
{
  int i=0;
  int sockfd;
  const char* ip = "127.0.0.1";
  int port = 6666;
  char buf[SOCK_BUF_SIZE];
    
  /*
   * massage queue variables
   */
  int msgid = -1;
  key_t key;
  message_buf rbuf;
  key = 1234;
  /*
   * get massage queue instance
   */
  if ((msgid = msgget(key, 0666)) < 0) {
        perror("msgget");
        exit(1);
  }

  while(1)
  {
    /*
     * Receive an answer of message type 1.
     */
    if (msgrcv(msgid, &rbuf, MSGSZ, 1, 0) < 0) {
        perror("msgrcv");
        exit(1);
    }
    else
    {
        sockfd = initSocket(ip, port);
        /*
        * Send the file to server. 
        */
        const char *file2send_path = rbuf.mtext;
        /*
        * open the file to send to server
        */
        FILE *fp = fopen(file2send_path, "rb");
        if (fp == NULL)
        {
            printf("FILE open error in socketBsp.c");
            exit(1);
        }
        int nread;
        while ((nread = fread(buf, 1, SOCK_BUF_SIZE, fp)) == SOCK_BUF_SIZE)
        {
            if (send(sockfd, buf, SOCK_BUF_SIZE, 0) < 0)
            {
                printf("send msg error: %s(errno: %d) in socketBsp.c\n", strerror(errno), errno);
                exit(1);
            }
        }
        
        if (nread < SOCK_BUF_SIZE && feof(fp))
        {
            printf("End of file\n");
            if (send(sockfd, buf, nread, 0) < 0)
            {
                printf("send msg error: %s(errno: %d) in socketBsp.c\n", strerror(errno), errno);
                exit(1);
            }
        }
        else 
        {
            /* error reading handle */
        }
        
        /*
        * cleaning up
        */
        fclose(fp);
        close(sockfd);
    }
    
    usleep(500*1000);//线程休眠500ms
  }
  
  
}

void* NetRxThread(void *argv)
{
  int i=0;
  
  while(1)
  {
    // printf("net rx thread %d\n",i++);
    usleep(500*1000);//线程休眠500ms
  }
  
  
}
