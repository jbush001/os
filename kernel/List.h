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

#ifndef _LIST_H
#define _LIST_H

#include "KernelDebug.h"
#include "types.h"

struct ListNode {
public:
	inline ListNode();
	virtual ~ListNode() {}
	inline void RemoveFromList();

private:
	ListNode *fNext;
	ListNode *fPrev;
	friend class List;
	friend class Queue;
};

class List {
public:
	inline List();
	inline ListNode* AddToTail(ListNode*);
	inline ListNode* AddToHead(ListNode*);
	inline ListNode* AddBefore(ListNode *next, ListNode *newEntry);
	inline ListNode* AddAfter(ListNode *prev, ListNode *newEntry);
	inline ListNode* Remove(ListNode*);
	inline ListNode* GetHead() const;
	inline ListNode* GetTail() const;
	inline ListNode* GetNext(const ListNode*) const;
	inline ListNode* GetPrevious(const ListNode*) const;
	inline bool IsEmpty() const;
	inline int CountItems() const;

protected:

	// The nodes are stored in a doubly-linked circular list
	// of entries, with this dummy node as the head.
	ListNode fDummyHead;

};

inline ListNode::ListNode()
	:	fNext(0),
		fPrev(0)
{
}

inline void ListNode::RemoveFromList()
{
	fPrev->fNext = fNext;
	fNext->fPrev = fPrev;
	fNext = 0;
	fPrev = 0;
}

inline List::List()
{
	fDummyHead.fNext = &fDummyHead;
	fDummyHead.fPrev = &fDummyHead;
}

inline ListNode* List::AddToTail(ListNode *node)
{
	ASSERT(!node->fPrev);
	node->fNext = &fDummyHead;
	node->fPrev = fDummyHead.fPrev;
	node->fNext->fPrev = node;
	node->fPrev->fNext = node;
	return node;
}

inline ListNode* List::AddToHead(ListNode *node)
{
	ASSERT(!node->fPrev);
	node->fPrev = &fDummyHead;
	node->fNext = fDummyHead.fNext;
	node->fNext->fPrev = node;
	node->fPrev->fNext = node;
	return node;
}

inline ListNode* List::Remove(ListNode *node)
{
	node->RemoveFromList();
	return node;
}

inline ListNode* List::GetHead() const
{
	if (fDummyHead.fNext == &fDummyHead)
		return 0;

	return fDummyHead.fNext;
}

inline ListNode* List::GetTail() const
{
	if (fDummyHead.fPrev == &fDummyHead)
		return 0;

	return fDummyHead.fPrev;
}

inline ListNode* List::GetNext(const ListNode *node) const
{
	if (node->fNext == &fDummyHead)
		return 0;

	return node->fNext;
}

inline ListNode* List::GetPrevious(const ListNode *node) const
{
	if (node->fPrev == &fDummyHead)
		return 0;

	return node->fPrev;
}

inline ListNode* List::AddBefore(ListNode *nextEntry, ListNode *newEntry)
{
	ASSERT(!newEntry->fPrev);
	newEntry->fNext = nextEntry;
	newEntry->fPrev = nextEntry->fPrev;
	newEntry->fPrev->fNext = newEntry;
	newEntry->fNext->fPrev = newEntry;

	return newEntry;
}

inline ListNode* List::AddAfter(ListNode *previousEntry, ListNode *newEntry)
{
	ASSERT(!newEntry->fPrev);
	newEntry->fNext = previousEntry->fNext;
	newEntry->fPrev = previousEntry;
	newEntry->fPrev->fNext = newEntry;
	newEntry->fNext->fPrev = newEntry;

	return newEntry;
}

inline bool List::IsEmpty() const
{
	return fDummyHead.fNext == &fDummyHead;
}

inline int List::CountItems() const
{
	int count = 0;
	for (ListNode *node = fDummyHead.fNext; node != &fDummyHead; node = node->fNext)
		count++;

	return count;
}

#endif
