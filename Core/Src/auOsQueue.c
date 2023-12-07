#include "auOsQueue.h"

auByte_t auOsQueueOffer(auOsMsgQueue *queue, auOsMessageBlock *msg)
{
	if (queue->auOsMsgQueueRoot.QueueLength > 255) // limit reached
		return 0;
	if (NULL == queue || NULL == msg)
		return 0;
	if (queue->Owner)
		if (queue->auOsMsgQueueRoot.I = NULL) // if queue is empty
		{
			msg->NodeNext = NULL;
			msg->NodePrev = NULL;
			queue->auOsMsgQueueRoot.I = msg;
			queue->auOsMsgQueueRoot.O = msg;
			queue->auOsMsgQueueRoot.QueueLength = 1;
			queue->SendFlag = 1;
		}
		else
		{
			msg->NodeNext = queue->auOsMsgQueueRoot.I;
			msg->NodePrev = NULL;
			queue->auOsMsgQueueRoot.I->NodePrev = msg;
			queue->auOsMsgQueueRoot.I = msg;
			queue->auOsMsgQueueRoot.QueueLength++;
			queue->SendFlag = 1;
		}
	return 1;
}

auInt32_t auOsQueueOfferWithPriority(auOsMsgQueue *queue, auOsMessageBlock *msg)
{
	if (queue->auOsMsgQueueRoot.QueueLength > 255) // limit reached
		return 0;
	if (NULL == queue || NULL == msg)
		return 0;
	if (queue->auOsMsgQueueRoot.I = NULL) // if queue is empty
	{
		msg->NodeNext = NULL;
		msg->NodePrev = NULL;
		queue->auOsMsgQueueRoot.I = msg;
		queue->auOsMsgQueueRoot.O = msg;
		queue->auOsMsgQueueRoot.QueueLength = 1;
	}
	else
	{
		volatile auOsMessageBlock *tmp = queue->auOsMsgQueueRoot.I;
		while (tmp->MsgPriority < msg->MsgPriority) // priority 0 = max
		{
			tmp = (auOsMessageBlock *)tmp->NodeNext;
		}
		msg->NodePrev = tmp->NodePrev;
		msg->NodeNext = tmp;
		if (NULL != tmp->NodePrev)
		{
			((auOsMessageBlock *)tmp->NodePrev)->NodeNext = msg;
		}
		else
		{
			queue->auOsMsgQueueRoot.I = msg;
		}
		tmp->NodePrev = msg;
		queue->auOsMsgQueueRoot.QueueLength++;
	}
}

auOsMessageBlock *auOsQueuePoll(auOsMsgQueue *queue)
{
	if (NULL == queue)
		return NULL;
	volatile auOsMessageBlock *retVal = queue->auOsMsgQueueRoot.O;
	if (queue->auOsMsgQueueRoot.QueueLength > 1)
	{
		((auOsMessageBlock *)queue->auOsMsgQueueRoot.O->NodePrev)->NodeNext = NULL;
	}
	return queue->auOsMsgQueueRoot.O;
}

auOsMessageBlock *auOsQueuePeek(auOsMsgQueue *queue)
{
	return queue->auOsMsgQueueRoot.O;
}

auByte_t auOsQueueOfferIRQ(auOsMsgQueue *queue, auOsMessageBlock *msg)
{
	__disable_irq();
	auOsQueueOffer(queue, msg);
	__enable_irq();
}