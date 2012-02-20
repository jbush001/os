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
