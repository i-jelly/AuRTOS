/*
 * osPort.c
 *
 *  Created on: Apr 14, 2023
 *      Author: lanzy
 */

#include <auOsThread.h>
#include "auOsDefines.h"
// #include "Peripheral.h"

extern auOsThreadCore *auOsThreadSchedulerCore;

void **auOsThread;
void *PendSV_Call;
extern void *StartFirstThread;

volatile IRQCallbackTypeDef SysTickIRQCallback =
	{.IRQCallback = NULL, .v = NULL};
void SysTickSetIRQCallback(RunPtr IRQCallback, void *v);

// 获取上次调用于本次调用之间的时间间隔,仅在本C中可用
// 不需要此函数也能运行
// 不知道用户会不会瞎搞，因此Systick是不被相信其计数值与精度的，所以此处需要单独使用一个时钟源，这里
static auInt32_t port_GetDt()
{
	extern TIM_HandleTypeDef htim17;
	static volatile auInt32_t lastT = 0; // 此static变量还需考虑
	auInt32_t nowT;
	auInt32_t r;

	nowT = (auInt32_t)htim17.Instance->CNT;

	r = nowT - lastT;
	lastT = nowT;

	return r;
}

// 刷新Thread 更新任务堆栈
void *auOsThreadSPRefresh()
{
	auOsThreadInfoTypeDef *p;
	auInt32_t dt;
	auInt32_t Status;
	auInt32_t EmptyNum;
	void *BlockObject;

	if (SchedulerSuspended == auOsThreadSchedulerCore->SchedulerStatus)
	{
		return (void *)0; // 调度已挂起
	}

	p = auOsThread[1];

	if (p->Stack > auOsThread[0])
	{

		OsErrorHandler(); // 堆栈溢出
	}

	dt = port_GetDt();
	auOsThreadSchedulerCore->runTime += dt;
	if (NULL == p->next)
	{
		// 下一个为NULL 为空闲线程
		auOsThreadSchedulerCore->idleTime += dt;
	}
	if (auOsThreadSchedulerCore->runTime > 400000)
	{
		auOsThreadSchedulerCore->CPU_Utilization = 1000 - (1000 * auOsThreadSchedulerCore->idleTime / auOsThreadSchedulerCore->runTime);
		auOsThreadSchedulerCore->runTime = 0;
		auOsThreadSchedulerCore->idleTime = 0;
	}

	p = ((auOsThreadCore *)p->parent)->root;

	while (NULL != p)
	{
		Status = p->Status;
		if (0 == Status)
		{
			// 就绪态
			if (auOsThread == p->StackPointer)
			{
				return (void *)0;
			}
			else
			{
				auOsThread = p->StackPointer;
				return (void *)0xFFFFFFFF;
			}
			// 是否就绪由消息管理
			BlockObject = p->BlockObject;
		}
		p = p->next;
	}

	OsErrorHandler(); // 由于有空闲任务的存在 不可能存在没有就绪任务的情况
}

void auOsStartFirstThread()
{

	PendSV_Call = &StartFirstThread;
	auOsThread = auOsThreadSchedulerCore->root->StackPointer;
	auOsThreadSchedulerCore->RunThread = &auOsThread;
	//__ISB();//针对Cortex-M7核心需要开启，cache barrier
	//__DSB();

	extern void portSysTick(void *v);
	SysTickSetIRQCallback(portSysTick, NULL);

	TrigScheduler();

	OsErrorHandler(); // 来这肯定又问题
}

void portSysTick(void *v)
{
	auOsThreadInfoTypeDef *p;
	auOsThreadInfoTypeDef *p_last;
	auInt32_t isScheduler;

	if (auOsThreadSchedulerCore->SchedulerStatus)
	{
		// 调度已挂起
		return;
	}

	isScheduler = 0;

	p = auOsThreadSchedulerCore->DelayRoot;
	while (NULL != p)
	{

		if (p->Status > 0)
		{
			p->Status--;
			if (p->Status == 0)
			{
				isScheduler = -1;
			}
		}

		p = p->DelayNext;
	}

	if (isScheduler)
	{

		p = auOsThreadSchedulerCore->DelayRoot;

		while (NULL != p)
		{
			if (p->Status)
			{
				break;
			}
			else
			{
				p = p->DelayNext;
			}
		}
		auOsThreadSchedulerCore->DelayRoot = p;

		if (NULL != auOsThreadSchedulerCore->DelayRoot)
		{
			p_last = auOsThreadSchedulerCore->DelayRoot;
			p = p_last->DelayNext;
			while (NULL != p)
			{
				if (p->Status)
				{
					p_last = p;
					p = p->DelayNext;
				}
				else
				{
					p_last->DelayNext = p->DelayNext;
					p = p->DelayNext;
				}
			}
		}
		TrigScheduler();
	}
}

// 此程序参考Cortex权威指南，
auOsThreadInfoTypeDef *auOsNewThread(void (*Code)(void *), void *v, auInt32_t StackSize, auInt32_t Priority)
{
	auOsThreadInfoTypeDef *t;
	void **r;
	auUint32_t *Stack;
	auInt32_t StackSize_div4;

	StackSize += 3;
	StackSize -= StackSize % 4;

	StackSize_div4 = StackSize / 4;

	r = auOsThreadSchedulerCore->Mem.Malloc(sizeof(void *) * 2);
	t = auOsThreadSchedulerCore->Mem.Malloc(sizeof(auOsThreadInfoTypeDef));
	t->Priority = Priority;
	t->StackSize = StackSize;
	t->Status = 0;
	t->BlockObject = NULL;
	t->Stack = auOsThreadSchedulerCore->Mem.Malloc(StackSize);

	r[1] = t;

	Stack = t->Stack;
	Stack[StackSize_div4 - 1] = 0x01000000;					// xPSR
	Stack[StackSize_div4 - 2] = ((auUint32_t)Code);			//&0xFFFFFFFE;//PC
	Stack[StackSize_div4 - 3] = (auUint32_t)auOsThreadExit; // LR
	// LR寄存器记录当前调用函数结束后自动回跳的地址，也就是return之后会跳到哪里。
	//  Stack[StackSize_div4-4]=0x00000000;//R12
	//  Stack[StackSize_div4-5]=0x00000000;//R3
	//  Stack[StackSize_div4-6]=0x00000000;//R2
	//  Stack[StackSize_div4-7]=0x00000000;//R1
	Stack[StackSize_div4 - 8] = (auUint32_t)v; // R0

	Stack[StackSize_div4 - 9] = 0xFFFFFFFD; // LR 退回到线程模式 使用PSP
	// PSP MSP均为SP，区别在于使用的模式，ARM于特权模式下可使用MSP PSP，但在用户模式下
	// 只能使用PSP
	Stack[StackSize_div4 - 10] = 0; // R11
	Stack[StackSize_div4 - 11] = 0; // R10
	Stack[StackSize_div4 - 12] = 0; // R9
	Stack[StackSize_div4 - 13] = 0; // R8
	Stack[StackSize_div4 - 14] = 0; // R7
	Stack[StackSize_div4 - 15] = 0; // R6
	Stack[StackSize_div4 - 16] = 0; // R5
	Stack[StackSize_div4 - 17] = 0; // R4

	r[0] = &Stack[StackSize_div4 - 17];

	t->StackPointer = r;

	return t;
}

void SysTickSetIRQCallback(void (*IRQCallback)(void *), void *v)
{
	SysTickIRQCallback.v = v;
	SysTickIRQCallback.IRQCallback = IRQCallback;
}
