/// @file Scheduler.h
#ifndef _SCHEDULER_H
#define _SCHEDULER_H

#include "Timer.h"
#include "types.h"

const int kMaxPriority = 31;

class Thread;

/// The scheduler chooses which thread should run next
class Scheduler : public Timer {
public:
	Scheduler();

	/// Pick the next thread that should be run and call SwitchTo on it.
	void Reschedule();

	/// Mark a thread ready to run and put it on the ready queue
	void EnqueueReadyThread(Thread*);

private:
	Thread *PickNextThread();
	InterruptStatus HandleTimeout();

	int fReadyThreadCount;
	int fHighestReadyThread;
	Queue fReadyQueue[kMaxPriority + 1];
};

extern Scheduler gScheduler;

#endif
