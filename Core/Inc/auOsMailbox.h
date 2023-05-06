/*
 * auOsMailbox.h
 *
 *  Created on: 2023年4月19日
 *      Author: lanzy
 */
#pragma once
#ifndef INC_AUOSMAILBOX_H_
#define INC_AUOSMAILBOX_H_

#include <auOsThread.h>

extern auOsThreadCore *auOsThreadSchedulerCore;

// 用于中断向线程发送消息的消息句柄
typedef struct
{
	auOsThreadInfoTypeDef *thread; // 正在尝试读取邮箱的线程

	struct
	{
		void **MsgArray;
		auInt_t I_Offset;
		auInt_t O_Offset;
		auInt_t MsgArrayNum;
	} Msgs;
} auOsThreadMsgIRQTypeDef;


// 新建邮箱
auOsThreadMsgIRQTypeDef *auOsThreadMsgIRQ(auInt_t NumMsg);

// 中断向邮箱发送消息 返回 0:成功 其他:失败
auInt_t auOsThreadMsgIRQSend(auOsThreadMsgIRQTypeDef *msg, void *v0, void *v1);

// 尝试读取消息 不阻塞 返回 0:成功 其他:邮箱空
auInt_t auOsThreadMsgIRQTryRead(auOsThreadMsgIRQTypeDef *msg, void **dv);

// 读取消息 如果消息为空则阻塞 直到获取到消息
void auOsThreadMsgIRQRead(auOsThreadMsgIRQTypeDef *msg, void **dv);

// 删除邮箱
void auOsThreadMsgIRQDel(auOsThreadMsgIRQTypeDef *msg);

// 新建邮箱
auOsThreadMsgIRQTypeDef *auOsAddNewThreadMsg(auInt_t NumMsg);

// 邮箱发送消息
void auOsSendThreadMsg(auOsThreadMsgIRQTypeDef *msg, void *v0, void *v1);

// 邮箱发送消息 返回 0:成功 其他:失败
auInt_t auOsTrySendThreadMsg(auOsThreadMsgIRQTypeDef *msg, void *v0, void *v1);

// 尝试向邮箱发送消息 超时失败 返回 0:成功 其他:失败
auInt_t auOsTrySendThreadMsgWithTimeout(auOsThreadMsgIRQTypeDef *msg, void *v0, void *v1, auInt_t TimeOut);

// 尝试读取消息 不阻塞 返回 0:成功 其他:邮箱空
auInt_t auOsTryReadThreadMsg(auOsThreadMsgIRQTypeDef *msg, void **dv);

// 读取消息 如果消息为空则阻塞 直到获取到消息
void auOsReadThreadMsg(auOsThreadMsgIRQTypeDef *msg, void **dv);

// 尝试读取消息 阻塞 超时返回失败 返回 0:成功 其他:失败
auInt_t auOsReadThreadMsgWithTimeout(auOsThreadMsgIRQTypeDef *msg, void **dv, auInt_t TimeOut);

// 删除邮箱
void auOsDeleteThreadMsg(auOsThreadMsgIRQTypeDef *msg);

#endif /* INC_AUOSMAILBOX_H_ */
