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

/// @file Lock.h
///	Various flavors of kernel locks

#ifndef _LOCK_H
#define _LOCK_H

#include "Dispatcher.h"
#include "Semaphore.h"

class Mutex : public Dispatcher {
public:
	Mutex(int = 0);
	status_t Lock();
	void Unlock();
private:
	virtual void ThreadWoken();
};

/// Reader/Writer lock
class RWLock {
public:
	RWLock();
	status_t LockRead();
	void UnlockRead();
	status_t LockWrite();
	void UnlockWrite();

private:
	volatile int fCount;
	Semaphore fWriteSem;
	Semaphore fReadSem;
	Mutex fWriteLock;
};


class RecursiveLock {
public:
	RecursiveLock(const char name[]);
	status_t Lock();
	void Unlock();
	bool IsLocked() const;
private:
	Mutex fMutex;
	class Thread *fHolder;
	int fRecursion;
};


#endif
