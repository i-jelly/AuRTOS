/*
 * auOsAtomic.h
 *
 *  Created on: 2023年5月7日
 *      Author: lanzy
 */
#pragma once
#ifndef INC_AUOSATOMIC_H_
#define INC_AUOSATOMIC_H_

#include "auOsTypes.h"
#include "auOsThread.h"

extern auOsThreadCore *auOsThreadSchedulerCore;
// 锁
typedef struct
{
	volatile auOsThreadInfoTypeDef *thread;
	volatile auInt_t isHighPriorityLockReq; // 是否有更高优先级的锁请求
} auOsLockTypeDef;

// 新建一个锁
auOsLockTypeDef *auOsNewThreadLock(void);

// 锁定Lock 如果Lock已被其他线程占用 则阻塞当前线程并启动任务调度
void auOsThreadLock(auOsLockTypeDef *lock);

// 尝试锁定一个锁 返回0:成功
auInt_t auOsTryThreadLock(auOsLockTypeDef *lock);

// 尝试锁定一个锁 超时检测
auInt_t auOsTryThreadLockTimeout(auOsLockTypeDef *lock, auInt_t TimeOut);

// 释放锁
void auOsUnlockThread(auOsLockTypeDef *lock);

// 删除锁
void auOsDeleteThreadLock(auOsLockTypeDef *lock);

#endif /* INC_AUOSATOMIC_H_ */
