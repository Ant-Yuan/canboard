#ifndef _UTIL_APA_H
#define _UTIL_APA_H
#include "stdint.h"
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
uint8_t AP_A_Decoder(uint8_t* src,uint16_t src_len,uint8_t* tgt,uint16_t* p_len);


/*
AP_A封装器
src 待处理的NP包
src_len NP包长度
tgt 封装后的包数据
p_len 封装后的包长度
返回0 正常
返回1 长度错误
*/
uint8_t AP_A_Encoder(uint8_t* src,uint16_t src_len,uint8_t* tgt,uint16_t* p_len);

#endif
