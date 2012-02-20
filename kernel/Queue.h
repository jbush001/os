#ifndef _QUEUE_H
#define _QUEUE_H

#include "List.h"

typedef ListNode QueueNode;

class Queue : public List {
public:
	inline QueueNode* Enqueue(QueueNode*);
	inline QueueNode* Dequeue();
};

inline QueueNode* Queue::Enqueue(QueueNode *element)
{
	return AddToTail(element);
}

inline QueueNode* Queue::Dequeue()
{
	QueueNode *node = GetHead();
	if (node == 0)
		return 0;

	node->RemoveFromList();
	return node;
}           

#endif
