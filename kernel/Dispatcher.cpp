#include "cpu_asm.h"
#include "Dispatcher.h"
#include "Queue.h"
#include "Scheduler.h"
#include "Thread.h"
#include "Timer.h"

class ThreadWaitEvent;

/// A WaitTag associates a dispatcher to a wait event.
/// The wait tag is a node that exists in two linked lists:
/// Each dispatcher has a list of WaitTags that represent
/// threads that are waiting on it, and each thread has a list
/// of WaitTags that represent Dispatchers it is waiting on
class WaitTag : public QueueNode {
public:
	ThreadWaitEvent *fEvent;
	WaitTag *fEventNext;
	Dispatcher *fDispatcher;
};

/// A ThreadWaitEvent is created when a thread has blocks waiting for
/// some state change, and records what the thread is waiting for.
class ThreadWaitEvent : public Timer {
public:
	ThreadWaitEvent(Thread *thread, WaitFlags flags);
	virtual InterruptStatus HandleTimeout();

	Thread *fThread;
	WaitTag *fTags;
	WaitFlags fFlags;
	status_t fWakeError;
};

ThreadWaitEvent::ThreadWaitEvent(Thread *thread, WaitFlags flags)
	:	fThread(thread),
		fTags(0),
		fFlags(flags),
		fWakeError(E_NO_ERROR)
{
}

InterruptStatus ThreadWaitEvent::HandleTimeout()
{
	// Remove this event from the wait queues of all the dispatchers.
	for (WaitTag *threadBlock = fTags; threadBlock;
		threadBlock = threadBlock->fEventNext)
		threadBlock->RemoveFromList();

	fWakeError = E_TIMED_OUT;
	gScheduler.EnqueueReadyThread(fThread);

	return kReschedule;
}

Dispatcher::Dispatcher()
	:	fSignalled(false)
{
}

Dispatcher::~Dispatcher()
{
}

status_t Dispatcher::Wait(bigtime_t timeout)
{
	cpu_flags fl = DisableInterrupts();
	status_t result = E_NO_ERROR;
	if (fSignalled)
		ThreadWoken();
	else {
		WaitTag tag;
		Dispatcher *list = this;
		result = WaitInternal(1, &list, WAIT_FOR_ONE, timeout, &tag);
	}

	RestoreInterrupts(fl);
	return result;
}

status_t Dispatcher::WaitForMultipleDispatchers(int dispatcherCount, Dispatcher *dispatchers[],
	WaitFlags flags, bigtime_t timeout)
{
	status_t result = E_NO_ERROR;

	cpu_flags fl = DisableInterrupts();
	bool satisfied;
	if (flags & WAIT_FOR_ALL) {
		satisfied = true;
		for (int dispatcherIndex = 0; dispatcherIndex < dispatcherCount; dispatcherIndex++)
			if (!dispatchers[dispatcherIndex]->fSignalled) {
				satisfied = false;
				break;
			}
	} else {
		satisfied = false;
		for (int dispatcherIndex = 0; dispatcherIndex < dispatcherCount; dispatcherIndex++) {
			if (dispatchers[dispatcherIndex]->fSignalled) {
				satisfied = true;
				break;
			}
		}
	}

	if (satisfied) {
		for (int dispatcherIndex = 0; dispatcherIndex < dispatcherCount; dispatcherIndex++)
			dispatchers[dispatcherIndex]->ThreadWoken();

		RestoreInterrupts(fl);
		return E_NO_ERROR;
	}

	const int kMaxStackAlloc = 5;
	if (dispatcherCount <= kMaxStackAlloc) {
		WaitTag tags[kMaxStackAlloc];
		result = WaitInternal(dispatcherCount, dispatchers, flags, timeout, tags);
	} else {
		WaitTag *tags = new WaitTag[dispatcherCount];
		result = WaitInternal(dispatcherCount, dispatchers, flags, timeout, tags);
		delete [] tags;
	}

	RestoreInterrupts(fl);
	return result;
}

void Dispatcher::Signal(bool reschedule)
{
	cpu_flags fl = DisableInterrupts();
	ASSERT(!_get_interrupt_state());
	fSignalled = true;
	bool threadsWoken = false;
	for (WaitTag *nextDispatcherBlock = static_cast<WaitTag*>(fTags.GetHead());
		nextDispatcherBlock && fSignalled;) {
		WaitTag *dispatcherBlock = nextDispatcherBlock;
		nextDispatcherBlock = static_cast<WaitTag*>(fTags.GetNext(nextDispatcherBlock));

		// See if this thread's wait condition can be satisfied...
		bool wake = true;
		if (dispatcherBlock->fEvent->fFlags & WAIT_FOR_ALL) {
			for (WaitTag *threadBlock = dispatcherBlock->fEvent->fTags; threadBlock;
				threadBlock = threadBlock->fEventNext) {
				if (!threadBlock->fDispatcher->fSignalled) {
					wake = false;
					break;
				}
			}
		}

		if (wake) {
			for (WaitTag *threadBlock = dispatcherBlock->fEvent->fTags; threadBlock;
				threadBlock = threadBlock->fEventNext) {
				threadBlock->RemoveFromList();
				if (dispatcherBlock->fEvent->fFlags & WAIT_FOR_ALL)
					threadBlock->fDispatcher->ThreadWoken();
			}

			if ((dispatcherBlock->fEvent->fFlags & WAIT_FOR_ALL) == 0)
				dispatcherBlock->fDispatcher->ThreadWoken();

			dispatcherBlock->fEvent->CancelTimeout();
			gScheduler.EnqueueReadyThread(dispatcherBlock->fEvent->fThread);
			threadsWoken = true;
		}
	}

	RestoreInterrupts(fl);
	if (reschedule && threadsWoken)
		gScheduler.Reschedule();
}

void Dispatcher::Unsignal()
{
	fSignalled = false;
}

void Dispatcher::ThreadWoken()
{
}

status_t Dispatcher::WaitInternal(int dispatcherCount, Dispatcher *dispatchers[], WaitFlags flags,
	bigtime_t timeout, WaitTag tags[])
{
	status_t result = E_NO_ERROR;
	ThreadWaitEvent waitEvent(Thread::GetRunningThread(), flags);
	for (int dispatcherIndex = 0; dispatcherIndex < dispatcherCount; dispatcherIndex++) {
		tags[dispatcherIndex].fEvent = &waitEvent;
		dispatchers[dispatcherIndex]->fTags.Enqueue(&tags[dispatcherIndex]);
		tags[dispatcherIndex].fEventNext = waitEvent.fTags;
		tags[dispatcherIndex].fDispatcher = dispatchers[dispatcherIndex];
		waitEvent.fTags = &tags[dispatcherIndex];
	}

	if (timeout != INFINITE_TIMEOUT)
		waitEvent.SetTimeout(SystemTime() + timeout, kOneShotTimer);

	waitEvent.fThread->SetState(kThreadWaiting);
	gScheduler.Reschedule();
	result = waitEvent.fWakeError;
	return result;
}
