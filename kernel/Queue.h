// 
// Copyright 1998-2012 Jeff Bush
// 
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// 

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
