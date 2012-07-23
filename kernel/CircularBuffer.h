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

#ifndef _CIRCULAR_BUFFER_H
#define _CIRCULAR_BUFFER_H

#include "KernelDebug.h"

template <class TYPE, int COUNT>
class CircularBuffer {
public:
	inline CircularBuffer();
	inline bool IsEmpty() const;
	inline bool IsFull() const;
	inline int CountQueuedItems() const;
	inline void Insert(const TYPE&);
	inline int Insert(const TYPE*, int count);
	inline const TYPE& Remove();
	inline int Remove(TYPE*, int count);

private:
	int fIn;
	int fOut;
	int fQueuedItems;
	TYPE fBuffer[COUNT];
};

template<class TYPE, int COUNT>
CircularBuffer<TYPE, COUNT>::CircularBuffer()
	:	fIn(0),
		fOut(0),
		fQueuedItems(0)
{
}

template<class TYPE, int COUNT>
bool CircularBuffer<TYPE, COUNT>::IsEmpty() const
{
	return fQueuedItems == 0;
}

template<class TYPE, int COUNT>
bool CircularBuffer<TYPE, COUNT>::IsFull() const
{
	return fQueuedItems == COUNT;
}

template<class TYPE, int COUNT>
int CircularBuffer<TYPE, COUNT>::CountQueuedItems() const
{
	return fQueuedItems;
}

template<class TYPE, int COUNT>
void CircularBuffer<TYPE, COUNT>::Insert(const TYPE &object)
{
	ASSERT(fQueuedItems < COUNT);		
	fBuffer[fIn] = object;
	fIn = (fIn + 1) % COUNT;
	fQueuedItems++;
}

template<class TYPE, int COUNT>
int CircularBuffer<TYPE, COUNT>::Insert(const TYPE *objects, int count)
{
	if (count > COUNT - fQueuedItems)
		count = COUNT - fQueuedItems;
		
	for (int index = 0; index < count; index++) {
		fBuffer[fIn] = objects[index];	
		fIn = (fIn + 1) % COUNT;
	}

	fQueuedItems += count;
	return count;
}

template<class TYPE, int COUNT>
const TYPE& CircularBuffer<TYPE, COUNT>::Remove()
{
	ASSERT(fQueuedItems > 0);		
	TYPE &element = fBuffer[fOut];
	fOut = (fOut + 1) % COUNT;
	fQueuedItems--;
	return element;
}

template<class TYPE, int COUNT>
int CircularBuffer<TYPE, COUNT>::Remove(TYPE *objects, int count)
{
	if (count > fQueuedItems)
		count = fQueuedItems;
		
	for (int index = 0; index < count; index++) {
		objects[index] = fBuffer[fOut];	
		fOut = (fOut + 1) % COUNT;
	}

	fQueuedItems -= count;
	return count;
}

#endif
