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

#include "cpu_asm.h"
#include "Lock.h"
#include "Thread.h"


Mutex::Mutex(int)
{
	Signal(false);
}

status_t Mutex::Lock()
{
	return Wait();
}

void Mutex::Unlock()
{
	Signal(false);
}

void Mutex::ThreadWoken()
{
	Unsignal();
}

const int kMaxReaders = 0x50000000;

RWLock::RWLock()
	:	fCount(0),
		fWriteSem("rwlock:writer_wait", 0),
		fReadSem("rwlock:reader_wait", 0)
{
}

status_t RWLock::LockRead()
{
	status_t error = E_NO_ERROR;
	if (AtomicAdd(&fCount, 1) < 0)
		error = fReadSem.Wait();

	return error;
}

void RWLock::UnlockRead()
{
	if (AtomicAdd(&fCount, -1) < 0)
		fWriteSem.Release();
}

status_t RWLock::LockWrite()
{
	status_t error = fWriteLock.Lock();
	if (error != E_NO_ERROR)
		return error;
		
	int readerCount = AtomicAdd(&fCount, -kMaxReaders);
	while (readerCount-- > 0) {
		error = fWriteSem.Wait();
		if (error != E_NO_ERROR)
			break;
	}

	return error;
}

void RWLock::UnlockWrite()
{
	int waitingReaders = AtomicAdd(&fCount, kMaxReaders) + kMaxReaders;
	if (waitingReaders > 0)
		fReadSem.Release(waitingReaders);

	fWriteLock.Unlock();
}


RecursiveLock::RecursiveLock(const char[])
	:	fHolder(0),
		fRecursion(0)
{
}

status_t RecursiveLock::Lock()
{
	if (fHolder != Thread::GetRunningThread()) {
		while (fMutex.Lock() == E_INTERRUPTED)
			;
			
		fHolder = Thread::GetRunningThread();
	}

	fRecursion++;
	return E_NO_ERROR;	
}

void RecursiveLock::Unlock()
{
	ASSERT(fHolder == Thread::GetRunningThread());
	if (--fRecursion == 0) {
		fHolder = 0;
		fMutex.Unlock();
	}
}

bool RecursiveLock::IsLocked() const
{
	return fHolder == Thread::GetRunningThread();
}


