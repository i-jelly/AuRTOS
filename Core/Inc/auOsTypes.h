/*
 * osTypes.h
 *
 *  Created on: Apr 14, 2023
 *      Author: lanzy
 */
#pragma once
#ifndef INC_AUOSTYPES_H_
#define INC_AUOSTYPES_H_


typedef unsigned char        auByte_t;

typedef int                  auInt_t;
typedef unsigned int         auUint_t;

typedef short                auInt16_t;
typedef unsigned short       auUint16_t;

typedef int                  auInt32_t;
typedef unsigned int         auUint32_t;

typedef long long            auInt64_t;
typedef unsigned long long   auUint64_t;


typedef unsigned char*       auByte_pt;

typedef int*                 auInt_pt;
typedef unsigned int*        auUint_pt;

typedef short*               auInt16_pt;
typedef unsigned short*      auUint16_pt;

typedef int*                 auInt32_pt;
typedef unsigned int*        auUint32_pt;

typedef long long*           auInt64_pt;
typedef unsigned long long*  auUint64_pt;

//与(void*) 占用空间相同的整型
typedef int auSize_t;
//单独定义一个与CPU长度有关的类型，这个类型需要与CPU的寄存器位数一致，大部分情况下保持不变
typedef auUint32_t auRegLen;

//与(void*) 占用空间相同的无符号整型
typedef unsigned int auUsize_t;


#ifndef NULL
#define NULL ((void*)0)
#endif // !NULL


#endif /* INC_AUOSTYPES_H_ */
