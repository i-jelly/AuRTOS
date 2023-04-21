/*
 * osThread.h
 *
 *  Created on: Apr 14, 2023
 *      Author: lanzy
 */
#pragma once
#ifndef INC_AUOSTHREAD_H_
#define INC_AUOSTHREAD_H_

#include <auOsMalloc.h>
#include "auOsTypes.h"
#include "auOsDefines.h"

typedef enum
{
	SchedulerRun,
	SchedulerSuspended
} auOsSchedulerStatus;

typedef struct _auOsThreadInfoTypeDef
{
	void *parent;
	void *StackPointer;
	void *Stack;
	auSize_t StackSize;
	struct _auOsThreadInfoTypeDef *next;
	struct _auOsThreadInfoTypeDef *DelayNext; // 用于延时列表的指针

	auInt_t Priority;		 // 优先级 仅影响线程在列表中的排列 值越小 在列表的位置就靠前 优先级高 若有两个线程优先级相同 实际优先级也是一高一低 取决于创建线程时线程加入列表的方式
	volatile auInt_t Status; // 线程状态 0:就绪 -1:线程结束标记 -2:被邮箱阻塞 -3:被锁阻塞 -4:发送时被邮箱阻塞 大于0:因延时导致的阻塞 此段还表示为延时的tick数  其他:阻塞态
	void *BlockObject;		 // 阻塞对象 当Status为被中断邮箱阻塞时 此段为中断邮箱指针 当被锁阻塞时 此段为锁的指针

} auOsThreadInfoTypeDef;

typedef struct
{
	auOsThreadInfoTypeDef *root;
	auOsThreadInfoTypeDef *DelayRoot;
	void ***RunThread;
	volatile auOsSchedulerStatus SchedulerStatus; // 线程调度状态
	volatile auInt_t SchedulerForIrq;			  // 挂起线程调度时 中断产生了调度请求后 此段被置为非0

	auInt_t isDelete; // 是否有待删除的任务 此段由删除任务函数设置 空闲任务清除

	auInt_t CPU_Utilization; // 1000为百分百占用 负数表示cpu占有率获取未实现或者还未得到第一个占有率
	auUint32_t idleTime;	 // 线程空闲时间
	auUint32_t runTime;		 // 线程运行时间

	struct
	{
		void *(*Malloc)(auSize_t);
		void (*Free)(void *);
	} Mem;
} auOsThreadCore;

// 初始化 传入空闲任务参数和空闲任务堆栈大小
void auOsThreadInit(void *v, auInt_t StackSize);

// 创建任务
void auOsCreateThread(auOsThreadInfoTypeDef **taskPtr, RunPtr Code, void *v, auInt_t StackSize, auInt_t Priority);

// 线程休眠一段时间
void auOsThreadSleep(auInt_t NumOfTick);

// 退出当前线程 并释放线程占用的资源
void auOsThreadExit(void);

// 开始调度
void auOsStartScheduler(void);

// 获取cpu占用率
auInt_t auOsGetCPUPrecent(void);

#endif /* INC_AUOSTHREAD_H_ */
