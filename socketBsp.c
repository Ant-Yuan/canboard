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
#include "UTIL_APA.h"



#define PATH_BUFFER_LENGTH 128
#define MSGSZ PATH_BUFFER_LENGTH
#define SOCK_BUF_SIZE 1000 
#define LOCAL_BUF_SIZE 512 // the maximum can data size to transfer to server 
#define REQUEST_DATA_SIZE 269 // 2 + 255 + 8 +4 
#define DATA_SIZE 538 // 2 + 16 + 4 + 4 + 512, the decorated data's size
#define FRAME_SIZE 1008
#define TONGXUNID_LENGTH_BYTES 16
#define REQUEST_FRAME_SIZE 277
#define REQUEST_ACK_SIZE  273
#define FIN_SIZE  REQUEST_ACK_SIZE
#define DIR_SIZE 255
const char* confFilePath = "../conf.ini";

struct msgbuf {
    long    mtype;
    char    mtext[MSGSZ];
};

int readCFG(const char *path, char ip[], int *port);
int initSocket(const char *ip, int port);

void fillin_request(char *buf, int bufsize,const char *path, int path_length, uint64_t filesz, uint32_t packagenum)
{
	char *cp = buf;
	int i =0;
	for(i = 0; i != bufsize; i++)
		buf[i] = 0;
	*cp++ = 0x00;
    *cp++ = 0x01;
	strncpy(cp, path, path_length);
	cp += path_length;
	//int i =0;
	for (i = 0; i != 8; i++)
	{
		*cp++ = (uint8_t)filesz;
		filesz >>= 8;
	}
	for (i = 0; i != 4; i++)
	{
		*cp++ = (uint8_t)packagenum;
		packagenum >>= 8;
	}
}

// correct 
void fillin_request_test()
{
	int bufsize = 269;
    char buf[bufsize];
    char *path = "/20170427/109481948.Bin";
    int path_length = strlen(path);
    uint64_t filesz = 0x0123456789ABCDEF;
    uint32_t packagenum = 0x01234567;
    fillin_request(buf, bufsize, path, path_length, filesz, packagenum);
    int i = 0;
    /*
    for (i = 0; i < bufsize; i++)
    {
        printf("%02X\n", buf[i]);
    }
    */
}

void decorate_data(uint8_t *tongxunID, uint8_t  tongxunIDSize, uint32_t frameIdx, uint32_t datasize, uint8_t *data, uint8_t *newdata)
{
    int i = 0;
    uint8_t *cp = newdata;
    for (; i != DATA_SIZE; i++)
    {
        newdata[i] = 0;
    }
    *cp++ = 0x00;
    *cp++ = 0x02;
    for(i=0;i != tongxunIDSize; ++i)
    {
        *cp++ = tongxunID[i];
    }
    for (i = 0; i != 4; i++)
	{
		*cp++ = (uint8_t)frameIdx;
		frameIdx >>= 8;
	}
	for (i = 0; i != 4; i++)
	{
		*cp++ = (uint8_t)datasize;
		datasize >>= 8;
	}
	for (i = 0; i != datasize; i++)
	{
		*cp++ = data[i];
	}
}

void send_finish_frame(uint8_t *path, uint8_t *tongxunID, uint16_t tongxunIDSize, int sockfd)
{
    uint8_t fin_buf[FIN_SIZE];
    fin_buf[0] = 0x00;
    fin_buf[1] = 0x03;
    strncpy((char*)fin_buf+2, path, DIR_SIZE);
    strncpy((char*)fin_buf+2+DIR_SIZE, tongxunID, tongxunIDSize);
    // encode
    uint8_t buf[FRAME_SIZE];
    uint16_t len;
    uint8_t res= AP_A_Encoder(fin_buf, FIN_SIZE, buf, &len);
    printf("res of encoding in send_finish_frame from socketBsp.c:%d\n", res);
    if(res == 0)
    {
        // encoding succeed.
        send(sockfd, buf , len, 0);
    }
}

// This assumes buffer is at least x bytes long,
// and that the socket is blocking.
uint8_t readXBytes(int socket, uint16_t x, uint8_t* buffer)
{
    uint16_t bytesRead = 0;
    int result;
    while (bytesRead < x)
    {
        result = read(socket, buffer + bytesRead, x - bytesRead);
        if (result < 1)
        {
            printf("read from socket error in readXBytes\n");
            return 1;
        }
        bytesRead += result;
    }
    return 0;
}

// assuming buf is of length FRAME_SIZE
uint8_t read1Frame(uint8_t *buff , int sockfd, uint16_t *frame_len)
{
    // read data from canboard
    if (readXBytes(sockfd, 3, buff))
    {
        return 1;
    }
    // check whether receiving 0x7E 0x04 0x01
    if (buff[0] != 0x7E || buff[1] != 0x04 || buff[2] != 0x01)
    {
        printf("frame format error when receiving data frame in server\n");
        return 1;
    }
    // read data length string
    if (readXBytes(sockfd, 2, buff+3))
    {
        return 1;
    }
    uint16_t framesz;
    framesz *= 256;
    framesz += buff[4];
    framesz *= 256;
    framesz += buff[3];

    printf("frame data size: %d\n", framesz);

    if (readXBytes(sockfd, framesz, buff + 5))
    {
        return 1;
    }
    // read CRC and 0x7E
    if (readXBytes(sockfd, 3, buff+5+framesz))
    {
        return 1;
    }

    *frame_len = 8 + framesz;

    return 0;
}

void send2Server(uint8_t *path, const char *ip, int port, int sockfd)
{
    char readFromLocal_buf[LOCAL_BUF_SIZE];
    int numPackages;
    
    /*
    * open the file to send to server
    */
    FILE *fp = fopen((char*)path, "rb");
    if (fp == NULL)
    {
        printf("FILE open error in send2Server from socketBsp.c\n");
        exit(1);
    }
    // Get the size of file.
    fseek(fp, 0L, SEEK_END);
    int sz = ftell(fp); // size in bytes
    rewind(fp);
    /*
     * calculate number of packages that needs to be sent
     */
    numPackages = sz / LOCAL_BUF_SIZE + 1;
    // send sending_file_request package to server
    char request_buf[FRAME_SIZE];
    fillin_request(request_buf, FRAME_SIZE, path, 255, sz, numPackages);
    //printf("filling in request data\n");
    //printf("path: %s\n", _path);
    //printf("file size: %d\n", sz);
    //printf("package number: %d\n", numPackages);
    /*
    for (int i = 0; i < REQUEST_DATA_SIZE; i++)
    {
        printf("%02X\n", request_buf[i]);
    }
    */
    
    uint8_t tgt[FRAME_SIZE];
    uint16_t p_len;
    AP_A_Encoder((uint8_t*)request_buf, REQUEST_DATA_SIZE, tgt, &p_len);
    /*
    printf("Encoded request data: %d\n", p_len);
    for (int i = 0; i < p_len; i++)
    {
        printf("%02X\n", tgt[i]);
    }
    */
    send(sockfd, tgt, p_len, 0);
    printf("Request frame sent: %s.\n", path);
    
    // Receive request ack from server here.
    uint8_t recvbuf[FRAME_SIZE];
    uint16_t len;
    if (read1Frame(recvbuf, sockfd, &len)!=0)
    {
        printf("Receive request ack failed.");
        return;
    }
    
    uint8_t request_ack[REQUEST_ACK_SIZE];
    uint16_t req_len;
    uint8_t res = AP_A_Decoder(recvbuf, len, request_ack, &req_len);
    uint8_t tongxunID[TONGXUNID_LENGTH_BYTES];
    if (res == 0){
        printf("Receive ack of %s\n", path);
        // decode succeed
        // get tongxunID
        strncpy(tongxunID, request_ack+257, TONGXUNID_LENGTH_BYTES);
        /*
        printf("tongxunID--\n");
        for(int i = 0; i != TONGXUNID_LENGTH_BYTES; i++)
        {
            printf("%02X\n", tongxunID[i]);
        }
        */
    }
    else{
        printf("decode request ack frame failed:%d\n", res);
        printf("Received request ack size: %d\n", len);
        return;
    }
    
    uint32_t nread;
    uint32_t frameIdx = 1;
    uint8_t filledin_data[DATA_SIZE];
    
    while ((nread = fread(readFromLocal_buf, 1, LOCAL_BUF_SIZE, fp)) == LOCAL_BUF_SIZE)
    {
        decorate_data(tongxunID, (uint8_t)TONGXUNID_LENGTH_BYTES,frameIdx++, nread, (uint8_t*)readFromLocal_buf, filledin_data);
        // Encode the data buffer according to AP_A_Encoder
        uint8_t res =  AP_A_Encoder((uint8_t*)filledin_data,DATA_SIZE,(uint8_t*)tgt,&p_len);
        if (res == 1){
            // Check for error here. 
             
        }
        if (send(sockfd, tgt, p_len, 0) < 0)
        {
            printf("send msg error: %s(errno: %d) in socketBsp.c\n", strerror(errno), errno);
            exit(1);
        }
    }
    
    if (nread < LOCAL_BUF_SIZE && feof(fp))
    {
        printf("End of file\n");
        decorate_data(tongxunID, (uint8_t)TONGXUNID_LENGTH_BYTES,frameIdx++, nread, (uint8_t*)readFromLocal_buf, filledin_data);
        // Encode the data buffer according to AP_A_Encoder
        uint8_t res =  AP_A_Encoder((uint8_t*)filledin_data,DATA_SIZE,(uint8_t*)tgt,&p_len);
        if (res == 1){ 
            // Check for error here.
        }
        if (send(sockfd, tgt, p_len, 0) < 0)
        {
            printf("send msg error: %s(errno: %d) in socketBsp.c\n", strerror(errno), errno);
            exit(1);
        }
    }
    else 
    {
        // error reading handle 
    }
    
    // tell server file transfer finished
    send_finish_frame(path, tongxunID, TONGXUNID_LENGTH_BYTES, sockfd);
    
    // Receive finish ack from server
    uint8_t fin_ack_buf[FRAME_SIZE];
    uint16_t fin_ack_buf_len;
    uint8_t result = read1Frame(fin_ack_buf , sockfd, &fin_ack_buf_len);
    if(result)
    {
        perror("Error when receiving finish ack frame\n");
    }
    uint8_t decoded[FRAME_SIZE];
    uint16_t decoded_len;
    uint16_t decode_result = AP_A_Decoder(fin_ack_buf, fin_ack_buf_len, decoded, &decoded_len);
    if (decode_result == 0 ) 
    {
        if(decoded[18] == 0x01)
        {
            printf("finish ack received:yes.\n");
        }
        else if (decoded[18] == 0xFE)
        {
            printf("finish ack received:no.\n");
        }
        else
        {
            printf("finish ack frame format error..\n");
        }
    }
    
    // 
    /*
    * cleaning up
    */
    fclose(fp);
}



//yyj add tx demo
void *NetTx_Thread(void *args)
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
  struct msgbuf rbuf;
  key = 1234;
  while(1)
  {
    switch(CanboardDev.netDev.netState)
    {
      case NET_NONE://do nothing,wait for modem init		
	break;
      case NET_READY:
	//need init socket and connect to server
        readCFG(confFilePath, ip, &port);
        sockfd = initSocket(ip, port);
        if(sockfd != -1)
        {
            CanboardDev.netDev.netState=NET_CONNECTED;
        }
        else
        {
            CanboardDev.netDev.netState=NET_ERR;
        }
        break;
    case NET_CONNECTED:
	//wait file ok msg
    /*
     * get message queue instance
     */
    if ((msgid = msgget(key, 0666)) < 0) {
        perror("msgget");
        exit(1);
    }
    /*
     * Receive an answer of message type 1.
     */
    if (msgrcv(msgid, &rbuf, MSGSZ, 1, 0) < 0) {
        perror("msgrcv");
        exit(1);
    }
    else
    {
        //send package to server
        // printf("received from message queue: %s\n", rbuf.mtext);
        send2Server(rbuf.mtext, ip, port, sockfd);
    }
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
            // printf("ip hit\n");
            strcpy(ip, value);
        }
        else if(strcmp(key, "port") == 0)
        {
            // printf("port hit\n");
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
  struct msgbuf rbuf;
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
         * Read configuration file 
         * get server IP and port number
         */
        readCFG(confFilePath, ip, &port);
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
