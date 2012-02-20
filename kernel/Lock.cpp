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


