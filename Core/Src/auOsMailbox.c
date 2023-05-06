/*
 * auOsMailbox.c
 *
 *  Created on: 2023年5月6日
 *      Author: lanzy
 */

#include <auOsMalloc.h>
#include <auOsThread.h>
#include "auOsTypes.h"
#include "auOsMailbox.h"

auOsThreadMsgIRQTypeDef *auOsThreadMsgIRQ(auInt_t NumMsg)
{
	auOsThreadMsgIRQTypeDef *r;

	auOsThreadSchedulerCore->SchedulerStatus = SchedulerSuspended; // 挂起任务调度

	r = auOsThreadSchedulerCore->Mem.Malloc(sizeof(auOsThreadMsgIRQTypeDef));
	r->Msgs.MsgArray = auOsThreadSchedulerCore->Mem.Malloc(sizeof(void *) * 2 * NumMsg);
	r->thread = NULL;
	r->Msgs.MsgArrayNum = NumMsg;
	r->Msgs.I_Offset = 0;
	r->Msgs.O_Offset = 0;

	auOsThreadSchedulerCore->SchedulerStatus = SchedulerRun; // 允许任务调度
	if (auOsThreadSchedulerCore->SchedulerForIrq)
	{
		auOsThreadSchedulerCore->SchedulerForIrq = 0;
		TrigScheduler(); // 触发调度
	}

	return r;
}

// 中断向邮箱发送消息 返回 0:成功 其他:失败
auInt_t auOsThreadMsgIRQSend(auOsThreadMsgIRQTypeDef *msg, void *v0, void *v1)
{
	void **v;
	auInt_t EmptyNum;

	// 判断是否有足够空间
	if (msg->Msgs.I_Offset < msg->Msgs.O_Offset)
	{
		EmptyNum = msg->Msgs.O_Offset - msg->Msgs.I_Offset;
	}
	else
	{
		EmptyNum = msg->Msgs.MsgArrayNum - msg->Msgs.I_Offset + msg->Msgs.O_Offset;
	}
	// 至少要空出一个容量
	if (EmptyNum < 2)
	{
		return -1;
	}

	v = &msg->Msgs.MsgArray[2 * msg->Msgs.I_Offset];
	v[0] = v0;
	v[1] = v1;

	if (msg->Msgs.I_Offset == (msg->Msgs.MsgArrayNum - 1))
	{
		msg->Msgs.I_Offset = 0;
	}
	else
	{
		msg->Msgs.I_Offset++;
	}

	if (NULL != msg->thread)
	{
		if (-2 == msg->thread->Status)
		{
			// 邮箱阻塞了线程

			if (SchedulerSuspended == auOsThreadSchedulerCore->SchedulerStatus)
			{
				auOsThreadSchedulerCore->SchedulerForIrq = 1;
			}
			else
			{
				TrigScheduler(); // 触发调度
			}
		}
	}
	return 0;
}

// 尝试读取消息 不阻塞 返回 0:成功 其他:邮箱空
auInt_t auOsThreadMsgIRQTryRead(auOsThreadMsgIRQTypeDef *msg, void **dv)
{
	void **v;

	if (msg->Msgs.I_Offset != msg->Msgs.O_Offset)
	{
		v = &msg->Msgs.MsgArray[2 * msg->Msgs.O_Offset];
		dv[0] = v[0];
		dv[1] = v[1];
		if (msg->Msgs.O_Offset == (msg->Msgs.MsgArrayNum - 1))
		{
			msg->Msgs.O_Offset = 0;
		}
		else
		{
			msg->Msgs.O_Offset++;
		}
		return 0;
	}

	return -1;
}

// 读取消息 如果消息为空则阻塞 直到获取到消息
void auOsThreadMsgIRQRead(auOsThreadMsgIRQTypeDef *msg, void **dv)
{
	auOsThreadInfoTypeDef *_this;

	_this = (*auOsThreadSchedulerCore->RunThread)[1];

	if (0 != auOsThreadMsgIRQTryRead(msg, dv))
	{

		// 阻塞
		_this->BlockObject = msg;
		_this->Status = -2; // 被邮箱阻塞
		msg->thread = _this;
		TrigScheduler(); // 触发调度

		while (0 != auOsThreadMsgIRQTryRead(msg, dv))
		{
			// 等待读取到数据
		}
	}
}

// 删除邮箱
void auOsThreadMsgIRQDel(auOsThreadMsgIRQTypeDef *msg)
{
	auOsThreadSchedulerCore->SchedulerStatus = SchedulerSuspended; // 挂起任务调度

	auOsThreadSchedulerCore->Mem.Free(msg->Msgs.MsgArray);
	auOsThreadSchedulerCore->Mem.Free(msg);

	auOsThreadSchedulerCore->SchedulerStatus = SchedulerRun; // 允许任务调度
	if (auOsThreadSchedulerCore->SchedulerForIrq)
	{
		auOsThreadSchedulerCore->SchedulerForIrq = 0;
		TrigScheduler(); // 触发调度
	}
}

// 新建邮箱
auOsThreadMsgIRQTypeDef *auOsAddNewThreadMsg(auInt_t NumMsg)
{
	return auOsThreadMsgIRQ(NumMsg);
}

// C文件内部函数
static auInt_t _isThreadMsgEmpty(auOsThreadMsgIRQTypeDef *msg)
{
	auInt_t EmptyNum;
	// 判断是否有足够空间
	if (msg->Msgs.I_Offset < msg->Msgs.O_Offset)
	{
		EmptyNum = msg->Msgs.O_Offset - msg->Msgs.I_Offset;
	}
	else
	{
		EmptyNum = msg->Msgs.MsgArrayNum - msg->Msgs.I_Offset + msg->Msgs.O_Offset;
	}
	// 至少要空出一个容量
	if (EmptyNum < 2)
	{
		return -1;
	}

	return 0;
}

// 邮箱发送消息
void auOsSendThreadMsg(auOsThreadMsgIRQTypeDef *msg, void *v0, void *v1)
{
	void **v;
	auInt_t EmptyNum;
	auOsThreadInfoTypeDef *_this;

	_this = (*auOsThreadSchedulerCore->RunThread)[1];

	auOsThreadSchedulerCore->SchedulerStatus = SchedulerSuspended; // 挂起任务调度

	// 判断是否有足够空间
	if (msg->Msgs.I_Offset < msg->Msgs.O_Offset)
	{
		EmptyNum = msg->Msgs.O_Offset - msg->Msgs.I_Offset;
	}
	else
	{
		EmptyNum = msg->Msgs.MsgArrayNum - msg->Msgs.I_Offset + msg->Msgs.O_Offset;
	}
	// 至少要空出一个容量
	if (EmptyNum < 2)
	{

		// 阻塞
		_this->BlockObject = msg;
		_this->Status = -4; // 被邮箱阻塞
		msg->thread = _this;

		auOsThreadSchedulerCore->SchedulerStatus = SchedulerRun; // 允许任务调度
		if (auOsThreadSchedulerCore->SchedulerForIrq)
		{
			auOsThreadSchedulerCore->SchedulerForIrq = 0;
		}
		TrigScheduler(); // 触发调度

		while (msg->thread != _this)
		{
		}

		auOsThreadSchedulerCore->SchedulerStatus = SchedulerSuspended; // 挂起任务调度
	}

	v = &msg->Msgs.MsgArray[2 * msg->Msgs.I_Offset];
	v[0] = v0;
	v[1] = v1;

	if (msg->Msgs.I_Offset == (msg->Msgs.MsgArrayNum - 1))
	{
		msg->Msgs.I_Offset = 0;
	}
	else
	{
		msg->Msgs.I_Offset++;
	}

	if (NULL != msg->thread)
	{

		if (msg->thread->Priority < _this->Priority)
		{

			// 读取线程优先级更高

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

	return;
}

// 邮箱发送消息 返回 0:成功 其他:失败
auInt_t auOsTrySendThreadMsg(auOsThreadMsgIRQTypeDef *msg, void *v0, void *v1)
{
	void **v;
	auInt_t EmptyNum;
	auOsThreadInfoTypeDef *_this;

	_this = (*auOsThreadSchedulerCore->RunThread)[1];

	auOsThreadSchedulerCore->SchedulerStatus = SchedulerSuspended; // 挂起任务调度

	// 判断是否有足够空间
	if (msg->Msgs.I_Offset < msg->Msgs.O_Offset)
	{
		EmptyNum = msg->Msgs.O_Offset - msg->Msgs.I_Offset;
	}
	else
	{
		EmptyNum = msg->Msgs.MsgArrayNum - msg->Msgs.I_Offset + msg->Msgs.O_Offset;
	}
	// 至少要空出一个容量
	if (EmptyNum < 2)
	{

		auOsThreadSchedulerCore->SchedulerStatus = SchedulerRun; // 允许任务调度
		if (auOsThreadSchedulerCore->SchedulerForIrq)
		{
			auOsThreadSchedulerCore->SchedulerForIrq = 0;
			TrigScheduler(); // 触发调度
		}
		return -1;
	}

	v = &msg->Msgs.MsgArray[2 * msg->Msgs.I_Offset];
	v[0] = v0;
	v[1] = v1;

	if (msg->Msgs.I_Offset == (msg->Msgs.MsgArrayNum - 1))
	{
		msg->Msgs.I_Offset = 0;
	}
	else
	{
		msg->Msgs.I_Offset++;
	}

	if (NULL != msg->thread)
	{

		if (msg->thread->Priority < _this->Priority)
		{

			// 读取线程优先级更高

			auOsThreadSchedulerCore->SchedulerStatus = SchedulerRun; // 允许任务调度
			if (auOsThreadSchedulerCore->SchedulerForIrq)
			{
				auOsThreadSchedulerCore->SchedulerForIrq = 0;
			}
			TrigScheduler(); // 触发调度
			return 0;
		}
	}

	auOsThreadSchedulerCore->SchedulerStatus = SchedulerRun; // 允许任务调度
	if (auOsThreadSchedulerCore->SchedulerForIrq)
	{
		auOsThreadSchedulerCore->SchedulerForIrq = 0;
		TrigScheduler(); // 触发调度
	}

	return 0;
}

// 尝试向邮箱发送消息 超时失败 返回 0:成功 其他:失败
auInt_t auOsTrySendThreadMsgWithTimeout(auOsThreadMsgIRQTypeDef *msg, void *v0, void *v1, auInt_t TimeOut)
{

	auInt_t r;

	r = auOsTrySendThreadMsg(msg, v0, v1);
	if (0 == r)
	{
		return 0;
	}
	while (TimeOut)
	{
		TimeOut--;

		auOsThreadSleep(1);

		r = auOsTrySendThreadMsg(msg, v0, v1);
		if (0 == r)
		{
			return 0;
		}
	}

	return 0;
}

// 尝试读取消息 不阻塞 返回 0:成功 其他:邮箱空
auInt_t auOsTryReadThreadMsg(auOsThreadMsgIRQTypeDef *msg, void **dv)
{
	return auOsThreadMsgIRQTryRead(msg, dv);
}

// 读取消息 如果消息为空则阻塞 直到获取到消息
void auOsReadThreadMsg(auOsThreadMsgIRQTypeDef *msg, void **dv)
{
	auOsThreadMsgIRQRead(msg, dv);
}

// 尝试读取消息 阻塞 超时返回失败 返回 0:成功 其他:失败
auInt_t auOsReadThreadMsgWithTimeout(auOsThreadMsgIRQTypeDef *msg, void **dv, auInt_t TimeOut)
{
	auInt_t r;

	r = auOsTryReadThreadMsg(msg, dv);
	if (0 == r)
	{
		return 0;
	}
	while (TimeOut)
	{
		TimeOut--;

		auOsThreadSleep(1);

		r = auOsTryReadThreadMsg(msg, dv);
		if (0 == r)
		{
			return 0;
		}
	}

	return r;
}

// 删除邮箱
void auOsDeleteThreadMsg(auOsThreadMsgIRQTypeDef *msg)
{
	auOsThreadMsgIRQDel(msg);
}
