#pragma once
#ifndef INC_AUOSQUEUE_H_
#define INC_AUOSQUEUE_H_

#include "auOsThread.h"
#include "auOsTypes.h"

typedef struct
{
	volatile void *NodePrev;
	volatile void *PtrMsg;
	volatile void *NodeNext;
	auUint32_t MsgLength;
	auUint32_t MsgPriority;
} auOsMessageBlock;

typedef struct
{
	volatile auOsThreadInfoTypeDef *Owner;
	volatile auByte_t SendFlag;
	struct _auOsMsgQueueRoot
	{
		volatile auOsMessageBlock *I;
		volatile auOsMessageBlock *O;
		volatile auUint32_t QueueLength;
	} auOsMsgQueueRoot;
} auOsMsgQueue;

auByte_t auOsQueueOffer(auOsMsgQueue *queue, auOsMessageBlock *msg);			  // return 0 if failed, otherwise, success
auByte_t auOsQueueOfferIRQ(auOsMsgQueue *queue, auOsMessageBlock *msg);			  // return 0 if failed, use in IRQ
auInt32_t auOsQueueOfferWithPriority(auOsMsgQueue *queue, auOsMessageBlock *msg); // return the position of added msg, -1 if failed
auOsMessageBlock *auOsQueuePoll(auOsMsgQueue *queue);							  // get and delete the first element, NULL if queue is empty
auOsMessageBlock *auOsQueuePeek(auOsMsgQueue *queue);							  // get but do nto delete the first element, NULL if empty

#endif