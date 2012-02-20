/// @file Semaphore.h

#ifndef _SEMAPHORE_H
#define _SEMAPHORE_H

#include "Resource.h"
#include "Queue.h"

/// Implementation of a counting semaphore
class Semaphore : public Resource {
public:
	Semaphore(const char *name, int initialCount = 1);
	void Release(int releaseCount = 1, bool reschedule = true);
protected:
	virtual void ThreadWoken();
private:
	volatile int fCount;
};

#endif
