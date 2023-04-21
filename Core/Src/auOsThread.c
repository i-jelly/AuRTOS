/*
 * osThread.c
 *
 *  Created on: Apr 14, 2023
 *      Author: lanzy
 */

#include <auOsThread.h>
#include "auOsDefines.h"

auOsThreadCore *auOsThreadSchedulerCore AU_RAM_DATA_AFTER = {0}; // 放在内存中的数组需要手动初始化，是否需要？

static auByte_t Mem[16 * 1024] AU_ALIGNED_AFTER AU_RAM_DATA_AFTER = {0}; // 内存池地址

extern auOsThreadInfoTypeDef *auOsNewThread(RunPtr Code, void *v, auInt_t StackSize, auInt_t Priority);
extern void auOsStartFirstThread(void);

static void *Malloc(auSize_t Size)
{
	void *r;
	r = auMalloc(Mem, Size);
	if (r == NULL)
	{
		OsErrorHandler(); // 按malloc规则，分配未成功需返回NULL
	}
	return r;
}
static void Free(void *p)
{
	auFree(Mem, p);
}

auInt_t auOsGetMemBfb()
{
	auMallocCoreDef info;

	auOsThreadSchedulerCore->SchedulerStatus = SchedulerSuspended; // 挂起任务调度

	auGetOsMallocInfo(Mem, &info);

	auOsThreadSchedulerCore->SchedulerStatus = SchedulerRun; // 允许任务调度
	if (auOsThreadSchedulerCore->SchedulerForIrq)
	{
		auOsThreadSchedulerCore->SchedulerForIrq = 0;
		TrigScheduler(); // 触发调度
	}

	return 100 * info.UseSize / (info.UseSize + info.FreeSize);
}

static void _tryDeleteThread()
{
	auOsThreadInfoTypeDef *t;
	auOsThreadInfoTypeDef *p;
	auOsThreadInfoTypeDef *plast;

	p = auOsThreadSchedulerCore->root;

	auOsThreadSchedulerCore->SchedulerStatus = SchedulerSuspended; // 挂起任务调度

	while (NULL != p)
	{
		if (-1 != p->Status)
		{
			break;
		}
		else
		{
			t = p;
			p = p->next;
			auOsThreadSchedulerCore->Mem.Free(t->StackPointer);
			auOsThreadSchedulerCore->Mem.Free(t->Stack);
			auOsThreadSchedulerCore->Mem.Free(t);
		}
	}
	auOsThreadSchedulerCore->root = p;

	if (NULL != auOsThreadSchedulerCore->root)
	{
		plast = auOsThreadSchedulerCore->root;
		p = plast->next;
		while (NULL != p)
		{
			if (-1 != p->Status)
			{
				plast = p;
				p = p->next;
			}
			else
			{
				plast->next = p->next;
				t = p;
				p = p->next;
				auOsThreadSchedulerCore->Mem.Free(t->StackPointer);
				auOsThreadSchedulerCore->Mem.Free(t->Stack);
				auOsThreadSchedulerCore->Mem.Free(t);
			}
		}
	}
	else
	{
		OsErrorHandler(); // 根节点不可能为空
	}

	auOsThreadSchedulerCore->SchedulerStatus = SchedulerRun; // 允许任务调度
	if (auOsThreadSchedulerCore->SchedulerForIrq)
	{
		auOsThreadSchedulerCore->SchedulerForIrq = 0;
		TrigScheduler(); // 触发调度
	}
}

void auOsIdleThread(void *v)
{
	for (;;)
	{
		// 空闲任务删除待删除
		if (auOsThreadSchedulerCore->isDelete)
		{
			auOsThreadSchedulerCore->isDelete = 0;
			_tryDeleteThread();
		}
	}
}

// 初始化 传入空闲任务参数和空闲任务堆栈大小
void auOsThreadInit(void *v, auInt_t StackSize)
{
	auOsThreadInfoTypeDef *t;

	auMallocCoreInit(Mem, 16 * 1024);

	auOsThreadSchedulerCore = Malloc(sizeof(auOsThreadSchedulerCore));
	auOsThreadSchedulerCore->Mem.Malloc = Malloc;
	auOsThreadSchedulerCore->Mem.Free = Free;
	auOsThreadSchedulerCore->RunThread = NULL;
	auOsThreadSchedulerCore->DelayRoot = NULL;
	auOsThreadSchedulerCore->isDelete = 0;
	auOsThreadSchedulerCore->SchedulerForIrq = 0;
	auOsThreadSchedulerCore->CPU_Utilization = -1;
	auOsThreadSchedulerCore->idleTime = 0;
	auOsThreadSchedulerCore->runTime = 0;

	t = auOsNewThread(auOsIdleThread, v, StackSize, 0x7FFFFFFF);
	t->next = NULL;
	t->parent = auOsThreadSchedulerCore;

	auOsThreadSchedulerCore->root = t;
}

// 创建任务
void auOsCreateThread(auOsThreadInfoTypeDef **taskPtr, RunPtr Code, void *v, auInt_t StackSize, auInt_t Priority)
{
	auOsThreadInfoTypeDef *t;
	auOsThreadInfoTypeDef *p;
	auOsThreadInfoTypeDef *plast;
	auOsThreadInfoTypeDef *_this;

	if (Priority == 0x7FFFFFFF)
	{
		// 不能使用这个优先级
		OsErrorHandler();
	}

	auOsThreadSchedulerCore->SchedulerStatus = SchedulerSuspended; // 挂起任务调度

	t = auOsNewThread(Code, v, StackSize, Priority);
	t->next = NULL;
	t->parent = auOsThreadSchedulerCore;

	if (NULL != taskPtr)
	{
		*taskPtr = t;
	}

	if (auOsThreadSchedulerCore->root->Priority > Priority)
	{
		t->next = auOsThreadSchedulerCore->root;
		auOsThreadSchedulerCore->root = t;
	}
	else
	{
		plast = auOsThreadSchedulerCore->root;
		p = plast->next;

		while (NULL != p)
		{
			if (p->Priority > Priority)
			{
				t->next = p;
				plast->next = t;

				auOsThreadSchedulerCore->SchedulerStatus = SchedulerRun; // 允许任务调度
				if (auOsThreadSchedulerCore->SchedulerForIrq)
				{
					auOsThreadSchedulerCore->SchedulerForIrq = 0;
					TrigScheduler(); // 触发调度
				}
				else
				{
					if (NULL != auOsThreadSchedulerCore->RunThread)
					{
						_this = (*auOsThreadSchedulerCore->RunThread)[1];
						if (_this->Priority > Priority)
						{
							// 新建的任务优先级比当前任务优先级更高 触发调度
							TrigScheduler();
						}
					}
				}
				return;
			}
			plast = p;
			p = p->next;
		}
		t->next = NULL;
		plast->next = t;
	}

	auOsThreadSchedulerCore->SchedulerStatus = SchedulerRun; // 允许任务调度
	if (auOsThreadSchedulerCore->SchedulerForIrq)
	{
		auOsThreadSchedulerCore->SchedulerForIrq = 0;
		TrigScheduler();
	}
	else
	{
		if (NULL != auOsThreadSchedulerCore->RunThread)
		{
			_this = (*auOsThreadSchedulerCore->RunThread)[1];
			if (_this->Priority > Priority)
			{
				// 新建的任务优先级比当前任务优先级更高 触发调度
				TrigScheduler();
			}
		}
	}
}

// 线程休眠一段时间
void auOsThreadSleep(auInt_t NumOfTick)
{
	auOsThreadInfoTypeDef *_this;
	auOsThreadInfoTypeDef *p;

	if (NumOfTick <= 0)
	{
		// 进来个不大于0的家伙 寄了
		return;
	}

	_this = (*auOsThreadSchedulerCore->RunThread)[1];

	auOsThreadSchedulerCore->SchedulerStatus = SchedulerSuspended; // 挂起任务调度

	_this->Status = NumOfTick;
	if (NULL == auOsThreadSchedulerCore->DelayRoot)
	{
		_this->DelayNext = NULL;
		auOsThreadSchedulerCore->DelayRoot = _this;
	}
	else
	{
		p = auOsThreadSchedulerCore->DelayRoot;
		while (NULL != p->DelayNext)
		{
			p = p->DelayNext;
		}
		_this->DelayNext = NULL;
		p->DelayNext = _this;
	}
	auOsThreadSchedulerCore->SchedulerStatus = SchedulerRun; // 允许任务调度
	if (auOsThreadSchedulerCore->SchedulerForIrq)
	{
		auOsThreadSchedulerCore->SchedulerForIrq = 0;
	}
	TrigScheduler();
	// while (_this->Status)
	; // 等待阻塞结束
}

// 直接终止一个线程 并释放线程占用的资源 如果传入的为NULL 则为终止调用它的线程
void auOsThreadAbort(auOsThreadInfoTypeDef *Thread)
{
	auOsThreadInfoTypeDef *_this;

	_this = (*auOsThreadSchedulerCore->RunThread)[1];
	if (NULL != Thread)
	{
		if (_this != Thread)
		{
			// 终止的不是本线程

			auOsThreadSchedulerCore->SchedulerStatus = SchedulerSuspended; // 挂起任务调度

			Thread->Status = -1; // 线程结束标记
			auOsThreadSchedulerCore->isDelete = -1;

			auOsThreadSchedulerCore->SchedulerStatus = SchedulerRun; // 允许任务调度
			if (auOsThreadSchedulerCore->SchedulerForIrq)
			{
				auOsThreadSchedulerCore->SchedulerForIrq = 0;
				TrigScheduler(); // 触发调度
			}

			return;
		}
	}

	// 终止的是本线程

	auOsThreadSchedulerCore->SchedulerStatus = SchedulerSuspended; // 挂起任务调度

	_this->Status = -1; // 线程结束标记
	auOsThreadSchedulerCore->isDelete = -1;

	auOsThreadSchedulerCore->SchedulerStatus = SchedulerRun; // 允许任务调度
	if (auOsThreadSchedulerCore->SchedulerForIrq)
	{
		auOsThreadSchedulerCore->SchedulerForIrq = 0;
	}
	TrigScheduler(); // 触发调度
	for (;;)
	{
	}
}

// 退出当前线程 并释放线程占用的资源
void auOsThreadExit()
{
	auOsThreadInfoTypeDef *_this;

	_this = (*auOsThreadSchedulerCore->RunThread)[1];
	auOsThreadSchedulerCore->SchedulerStatus = SchedulerSuspended; // 挂起任务调度

	_this->Status = -1; // 线程结束标记
	auOsThreadSchedulerCore->isDelete = -1;

	auOsThreadSchedulerCore->SchedulerStatus = SchedulerRun; // 允许任务调度
	if (auOsThreadSchedulerCore->SchedulerForIrq)
	{
		auOsThreadSchedulerCore->SchedulerForIrq = 0;
	}
	TrigScheduler();  // 触发调度
	OsErrorHandler(); // 要是能来这肯定是出错了
}

// 开始调度
void auOsStartScheduler()
{
	FPU->FPCCR |= FPU_FPCCR_ASPEN_Msk | FPU_FPCCR_LSPEN_Msk;
	HAL_NVIC_SetPriority(PendSV_IRQn, 15U, 0U);
	auOsStartFirstThread();
}

// 获取cpu占用率
auInt_t auOsGetCPUPrecent()
{
	return auOsThreadSchedulerCore->CPU_Utilization;
}
