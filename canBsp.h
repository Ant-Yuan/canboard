#ifndef _CAN_BSP_H
#define _CAN_BSP_H



#define MSGQUEUE_CAN		0x0101//can tx rx thread msg
#define MSGQUEUE_SD		0x0102//write file finish and send path to net tx thread




#define ROOT_CAN_STD_ID		0x01


#define CAN_NAME_CAN0 "can0"
#define CAN_NAME_CAN1 "can1"
#define CHILD_DEV_START 		0x00 	
#define CHILD_DEV_BROADCAST 		0xFF	
#define MAX_CHILD_DEV_NUM 		32	


typedef enum {DEV_INIT=0,DEV_NORMAL,DEV_STOP,DEV_ERR}DEV_STATUS;
typedef enum {CHILD_OFFLINE=0,CHILD_OK}CHILD_STATUS;
typedef enum {FREQ_256HZ=4,
	      FREQ_200HZ=5,
	      FREQ_128HZ=8,
	      FREQ_100HZ=10
  
}AD_FREQ; // 1000/AD_FREQ=HZ

typedef struct ChildNode_STRUCT
{
  CHILD_STATUS childStatus[MAX_CHILD_DEV_NUM];
  AD_FREQ devFreq;
}ChildNodeInfo;

typedef enum {MSG_NONE=0,				
	      MSG_CHECK=0x10,		//检查是否在线,下发配置,有返回
	      MSG_SYNC=0x11,		//开始同步采集数据
	      MSG_STOP=0x12,		//停止发送数据
	      MSG_UPDATE,		//子节点升级	
	      MSG_REBOOT,		//子节点重启
	      MSG_ERR,			//子节点报错
	      MSG_DATA1=0x21,		//子节点发送X和Y轴数据
	      MSG_DATA2=0x22,		//子节点发送z和YBP数据
	      MSG_DATA_RX_CPLT=0x30,	//子节点发送数据完成
	      MSG_SD_WT_CPLT=0x31,	//数据写入SD卡完成
	      MSG_SD_WT_START=0x32	//数据开始写入SD卡
}ROOT_MSG;

int canInit(void);
void* CanTxThread(void *argv);
void* CanRxThread(void *argv);


#endif
