/*
 * osMalloc.h
 *
 *  Created on: Apr 14, 2023
 *      Author: lanzy
 */
#pragma once
#ifndef INC_AUOSMALLOC_H_
#define INC_AUOSMALLOC_H_

#include <auOsConfig.h>
#include <auOsTypes.h>

typedef struct
{
	int Result;					 // 结果 0:未发现问题 -1:头块不应有上一个块 -2:出现问题(长度数据被修改 尾部不对齐) -3:存在连续未分配的块 -4:上下块信息不匹配 -5:与下一块信息不匹配 -6:与上一块信息不匹配
	void *ErrPtr;				 // 错误地址
	osAlignSizeDef UseSize;		 // 使用内存量
	osAlignSizeDef FreeSize;	 // 可申请重量
	osAlignSizeDef OccupySize;	 // 实际占用量
	osAlignSizeDef NoOccupySize; // 空闲量
} auMallocCoreDef;

typedef struct
{
	auRegLen *StartSize_Ptr; // 用于存放第一个内存池首地址
	auRegLen *EntrySize_Ptr; // 下次malloc时开始搜索的地址
	void *EndAddr;			 // 边界地址 为(内存池首指针+内存池大小)
} auOsMallocHeaderTypeDef;

/**
 * @brief 初始化内存池
 * @param MemAddr 作为内存池数组的指针
 * @param MemSize 数组大小(字节)
 * @return 无
 */
void auMallocCoreInit(void *MemAddr, osAlignSizeDef MemSize);

/**
 * @brief 向指定内存池申请内存
 * @param MemAddr 内存池地址
 * @param Size 要申请的内存大小
 * @return 申请到的内存指针 如果为NULL则为失败
 */
void *auMalloc(void *MemAddr, osAlignSizeDef Size);

/**
 * @brief 将内存释放回内存池
 * @param MemAddr 内存池地址
 * @param Ptr 要释放的内存指针
 * @return 无
 */
void auFree(void *MemAddr, void *Ptr);

/**
 * @brief 获取内存池状态
 * @param MemAddr 内存池地址
 * @param info 用于存放信息的指针
 * @return 无
 */
void auGetOsMallocInfo(void *MemAddr, auMallocCoreDef *info);

#endif /* INC_AUOSMALLOC_H_ */
