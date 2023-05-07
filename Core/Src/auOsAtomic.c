/*
 * auOsAtomic.c
 *
 *  Created on: 2023年5月7日
 *      Author: lanzy
 */

#include "auOsThread.h"
#include "auOsAtomic.h"

auOsLockTypeDef *auOsNewThreadLock()
{
	auOsLockTypeDef *r;

	auOsThreadSchedulerCore->SchedulerStatus = SchedulerSuspended; // 挂起任务调度

	r = auOsThreadSchedulerCore->Mem.Malloc(sizeof(auOsLockTypeDef));
	r->isHighPriorityLockReq = 0;
	r->thread = NULL;

	auOsThreadSchedulerCore->SchedulerStatus = SchedulerRun; // 允许任务调度
	if (auOsThreadSchedulerCore->SchedulerForIrq)
	{
		auOsThreadSchedulerCore->SchedulerForIrq = 0;
		TrigScheduler(); // 触发调度
	}

	return r;
}

// 锁定Lock 如果Lock已被其他线程占用 则阻塞当前线程并启动任务调度
void auOsThreadLock(auOsLockTypeDef *lock)
{
	auOsThreadInfoTypeDef *_this;

	_this = (*auOsThreadSchedulerCore->RunThread)[1];

	auOsThreadSchedulerCore->SchedulerStatus = SchedulerSuspended; // 挂起任务调度

	if ((NULL != lock->thread) && (lock->thread != _this))
	{
		// 已有其他线程使用这个锁

		if (lock->thread->Priority > _this->Priority)
		{
			// 占用锁的优先级低
			lock->isHighPriorityLockReq = -1; // 标记请求 已便于低优先级的线程释放锁时能触发任务调度
		}

		_this->BlockObject = lock;
		_this->Status = -3; // 被锁阻塞

		auOsThreadSchedulerCore->SchedulerStatus = SchedulerRun; // 允许任务调度
		if (auOsThreadSchedulerCore->SchedulerForIrq)
		{
			auOsThreadSchedulerCore->SchedulerForIrq = 0;
		}
		TrigScheduler(); // 触发调度
		while (lock->thread != _this)
		{
		}

		return;
	}
	else
	{
		lock->thread = _this; // 锁定
	}

	auOsThreadSchedulerCore->SchedulerStatus = SchedulerRun; // 允许任务调度
	if (auOsThreadSchedulerCore->SchedulerForIrq)
	{
		auOsThreadSchedulerCore->SchedulerForIrq = 0;
		TrigScheduler(); // 触发调度
	}
}

// 尝试锁定一个锁 返回0:成功
auInt_t auOsTryThreadLock(auOsLockTypeDef *lock)
{
	auOsThreadInfoTypeDef *_this;

	_this = (*auOsThreadSchedulerCore->RunThread)[1];

	auOsThreadSchedulerCore->SchedulerStatus = SchedulerSuspended; // 挂起任务调度

	if ((NULL != lock->thread) && (lock->thread != _this))
	{
		// 已有其他线程使用这个锁

		auOsThreadSchedulerCore->SchedulerStatus = SchedulerRun; // 允许任务调度
		if (auOsThreadSchedulerCore->SchedulerForIrq)
		{
			auOsThreadSchedulerCore->SchedulerForIrq = 0;
			TrigScheduler(); // 触发调度
		}

		return -1;
	}
	else
	{
		lock->thread = _this; // 锁定
	}

	auOsThreadSchedulerCore->SchedulerStatus = SchedulerRun; // 允许任务调度
	if (auOsThreadSchedulerCore->SchedulerForIrq)
	{
		auOsThreadSchedulerCore->SchedulerForIrq = 0;
		TrigScheduler(); // 触发调度
	}

	return 0;
}

// 尝试锁定一个锁 超时检测
auInt_t auOsTryThreadLockTimeout(auOsLockTypeDef *lock, auInt_t TimeOut)
{
	auInt_t r;

	r = auOsTryThreadLock(lock);
	if (0 == r)
	{
		return 0;
	}
	while (TimeOut)
	{
		TimeOut--;

		auOsThreadSleep(1);

		r = auOsTryThreadLock(lock);
		if (r == 0)
		{
			return 0;
		}
	}

	return r;
}

// 释放锁
void auOsUnlockThread(auOsLockTypeDef *lock)
{
	auOsThreadInfoTypeDef *_this;

	_this = (*auOsThreadSchedulerCore->RunThread)[1];

	auOsThreadSchedulerCore->SchedulerStatus = SchedulerSuspended; // 挂起任务调度

	if (lock->thread == _this)
	{
		lock->thread = NULL;
		if (lock->isHighPriorityLockReq)
		{
			// 存在更高优先级的线程尝试占用这个锁
			lock->isHighPriorityLockReq = 0;

			auOsThreadSchedulerCore->SchedulerStatus = SchedulerRun; // 允许任务调度
			if (auOsThreadSchedulerCore->SchedulerForIrq)
			{
				auOsThreadSchedulerCore->SchedulerForIrq = 0;
			}
			TrigScheduler(); // 触发调度
			return;
		}
	}

	auOsThreadSchedulerCore->SchedulerStatus = SchedulerRun; // 允许任务调度
	if (auOsThreadSchedulerCore->SchedulerForIrq)
	{
		auOsThreadSchedulerCore->SchedulerForIrq = 0;
		TrigScheduler(); // 触发调度
	}
}

// 删除锁
void auOsDeleteThreadLock(auOsLockTypeDef *lock)
{
	auOsThreadSchedulerCore->SchedulerStatus = SchedulerSuspended; // 挂起任务调度

	auOsThreadSchedulerCore->Mem.Free(lock);

	auOsThreadSchedulerCore->SchedulerStatus = SchedulerRun; // 允许任务调度
	if (auOsThreadSchedulerCore->SchedulerForIrq)
	{
		auOsThreadSchedulerCore->SchedulerForIrq = 0;
		TrigScheduler(); // 触发调度
	}
}
