#include "UTIL_APA.h"
#include "UTIL_CRC.h"
#include <string.h>
#include <stdio.h>

/*
AP_A解析器
src 待解析的包
src_len 待解析包长度
tgt 解析后数据
p_len PDU长度
返回0 正常
返回1 包基本结构错误
返回2 CRC错误
返回3 包内容错误
返回4 协议类型标志错
返回5 承载协议类型错
返回6 长度错误
*/
uint8_t AP_A_Decoder(uint8_t* src,uint16_t src_len,uint8_t* tgt,uint16_t* p_len)
{
	uint16_t i=0;
	uint16_t rlen=0;//目标序号
	uint16_t crc_res=0;
	
	//判断包基本结构
	if(src_len<4||src_len>1008||src[0]!=0x7E||src[src_len-1]!=0x7E)
	{
		printf("Decode error in AP_A_Decoder: src_len: %d; src[0]=%d; src[src_len-1]=%d\n", src_len, src[0], src[src_len-1]);
		return 1;
	}

	//进行转义
	for(i=1;i<src_len-1;i++)
	{
		if(src[i]!=0x5E)
		{
			tgt[rlen++]=src[i];
		}
		else
		{
			i++;//src序号递增
			if(src[i]==0x5D)
			{
				tgt[rlen++]=0x5E;
			}else if(src[i]==0x7D)
			{
				tgt[rlen++]=0x7E;
			}else
			{
				return 3;//包内容错误
			}
		}
		if(rlen>1006)
		{
			printf("Decode error in AP_A_Encoder: rlen(%d) too long.\n", rlen);
			return 1;//包长度检查
		}
		  
		
	}
	if(tgt[1]!=0x01)
	  return 5;//承载协议类型错
	if(rlen>=4)
	{
		crc_res=CRC_CCITT_16(0,tgt,rlen-2);
	}else
	{
		return 6;
	}
	
	//if((crc_res>>8)==tgt[rlen-1]&&((uint8_t)crc_res)==tgt[rlen-2])//CRC匹配
	//crc_res++;
	if(1)
	{
		for(i=0;i<rlen-4;i++)//COPY PDU 包头和CRC共4字节抛弃
		{
			tgt[i]=tgt[i+4];
		}
		*p_len=rlen-6;
	}	
	else
	{
		return 2;//CRC错误
	}
	return 0;
}
/*
AP_A封装器
src 待处理的NP包
src_len NP包长度
tgt 封装后的包数据
p_len 封装后的包长度
返回0 正常
返回1 长度错误
*/
uint8_t AP_A_Encoder(uint8_t* src,uint16_t src_len,uint8_t* tgt,uint16_t* p_len)
{
	uint16_t i=0,rlen=0;
	uint16_t crc_res=0;
	
	if(src_len>1000)return 1;//包基本结构错误，太大，无法打包
	
	for(i=0;i<src_len;i++)
	{
		src[src_len-1-i+4]=src[src_len-1-i];
	}
	
	src[0]=0x04;//协议类型标识
	src[1]=0x01;//承载协议类型
	src[2]=src_len&0xFF;
	src[3]=(src_len>>8)&0xFF;

	crc_res=CRC_CCITT_16(0,src,src_len+4);//计算CRC
	src[src_len+4]=((uint8_t)crc_res);
	src[src_len+5]=(crc_res>>8);

	
	//封装到tgt
	tgt[rlen++]=0x7E;//头标志
	
	for(i=0;i<src_len+6;i++)
	{
		if(src[i]!=0x5E&&src[i]!=0x7E)
		{
			tgt[rlen++]=src[i];
		}else
		{
			tgt[rlen++]=0x5E;
			if(src[i]==0x5E)
			{
				tgt[rlen++]=0x5D;
			}else
			{
				tgt[rlen++]=0x7D;
			}
		}
	}
	tgt[rlen++]=0x7E;//包尾标志
	*p_len=rlen;
	return 0;
}
