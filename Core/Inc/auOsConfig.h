/*
 * osConfig.h
 *
 *  Created on: Apr 4, 2023
 *      Author: lanzy
 */
#pragma once
#ifndef OSCONFIG_H_
#define OSCONFIG_H_

#ifndef __GCC__
// #error "auOsConfig.h requires GCC to complie"
#endif

// 若是CPU存在Cache，这行指示Cache的宽度(bytes)
#define osCacheLineAlign 8
// 对齐字节数 必须为2^n 并且不小于osAlignSizeDef类型的储存字节数的两倍
#define osAlignSizeDef auUint32_t
// 规定系统申请内存所使用的的内存地址
// 针对有不同内存分区的单片机
#define osMemLocateAddrBegin 0x20000000
#define osMemLocateAddrEnd 0x2001FFFF

#define osUseFullAsert 0 // 使用assert?

#ifdef osUseFullAsert
#define assert(expr) ((expr) ? (void)0 : failed())
void failed();
#else
#define assert(expr) ((void)0)
#endif

#endif /* OSCONFIG_H_ */
