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
#include<sys/socket.h>
#include<netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

#define PATH_BUFFER_LENGTH 128
#define MSGSZ PATH_BUFFER_LENGTH
#define SOCK_BUF_SIZE 1025
const char* confFilePath = "./conf.ini";

typedef struct msgbuf {
    long    mtype;
    char    mtext[MSGSZ];
} message_buf;

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


void trim(const char* src, char* buff, const unsigned int sizeBuff)
{
    if(sizeBuff < 1)
    return;

    const char* current = src;
    unsigned int i = 0;
    while(current != '\0' && i < sizeBuff-1)
    {
        if(*current != ' ' && *current != '\t' && *current != '\n')
            buff[i++] = *current;
        ++current;
    }
    buff[i] = '\0';
}

void 
parseLine(char* line, char *key, char *value)
{
    char *cp = 0; // Point to '='
    cp = strchr(line, '=');
    if (cp)
    {
        char keytemp[32] = {0};
        char valuetemp[32] = {0};
        strncpy(keytemp, line, cp - line);
        strcpy(valuetemp , cp+1);
        trim(keytemp, key, 32);
        trim(valuetemp , value , 32);        
    }
    else
    {
        perror("Format error in parseLine");
        return;
    }
}

/*
 * 0: success
 * 1: fail
 */
int readCFG(const char *path, char ip[], int *port)
{
    FILE *fp = fopen(path, "r");
    if (fp == NULL)
    {
        perror("Error in readCFG");
        return 1;
    }
    
    char line[128];
    while(fgets(line, sizeof(line), fp))
    {
        char key[32] = {0};
        char value[32] = {0};
        parseLine(line, key, value);
        // printf("Parsed line %s-%s\n", key, value);
        if (strcmp(key, "ip") == 0)
        {
            printf("ip hit\n");
            strcpy(ip, value);
        }
        else if(strcmp(key, "port") == 0)
        {
            printf("port hit\n");
            *port = atoi(value);
        }
        else
        {
            printf("%s-%s\n", key, value);
        }
    }
    /*
     * Check for feof
     */
    if (!feof(fp)) 
    {
        perror("Read error in readCFG");
        return 1;
    }

    fclose(fp);
    
}



void* NetTxThread(void *argv)
{
  int i=0;
  int sockfd;
  char ip[16];
  int port;
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
        /*
         * read configuration file 
         * get server IP and port number
         */
        readCFG(confFilePath, ip, &port);
        sockfd = initSocket(ip, port);
        /*========
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
