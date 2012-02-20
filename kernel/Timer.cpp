#include "cpu_asm.h"
#include "interrupt.h"
#include "KernelDebug.h"
#include "stdio.h"
#include "Timer.h"

extern void HardwareTimerBootstrap(InterruptStatus (*callback)());
extern void SetHardwareTimer(bigtime_t relativeTimeout);

Queue Timer::fTimerQueue;

Timer::Timer()
	:	fPending(false)
{
}

Timer::~Timer()
{
	if (fPending)
		panic("Attempt to delete a pending timer\n");
}

void Timer::SetTimeout(bigtime_t time, TimerMode mode)
{
	ASSERT(mode == kPeriodicTimer || mode == kOneShotTimer);
	if (fPending)
		panic("Attempt to Set() a pending timer\n");
	
	fMode = mode;
	fPending = true;
	if (fMode == kPeriodicTimer) {
		fInterval = time;
		if (fInterval <= 0)
			panic("Attempt to set periodic timer for zero or negative interval");
	
		fWhen = SystemTime() + fInterval;
	} else
		fWhen = time;

	cpu_flags st = DisableInterrupts();
	const Timer *oldHead = static_cast<const Timer*>(fTimerQueue.GetHead());
	Enqueue(this);

	// If a different timer is now the head of the timer queue,
	// reset the hardware timer.
	if (static_cast<Timer*>(fTimerQueue.GetHead()) != oldHead)
		ReprogramHardwareTimer();
		
	RestoreInterrupts(st);
}

bool Timer::CancelTimeout()
{
	cpu_flags st = DisableInterrupts();
	bool wasHead = (fTimerQueue.GetHead() == this);
	if (fPending)
		fTimerQueue.Remove(this);
	
	bool wasPending = fPending;
	fPending = false;
	if (wasHead)
		ReprogramHardwareTimer();
		
	RestoreInterrupts(st);
	return wasPending;
}

InterruptStatus Timer::HandleTimeout()
{
	return kUnhandledInterrupt;
}

void Timer::Bootstrap()
{
	HardwareTimerBootstrap(HardwareTimerInterrupt);
	AddDebugCommand("timers", "list pending timers", PrintTimerQueue);
}

InterruptStatus Timer::HardwareTimerInterrupt()
{
	bigtime_t now = SystemTime();		
	bool reschedule = false;
	while (fTimerQueue.GetHead() &&
		now >= static_cast<Timer*>(fTimerQueue.GetHead())->fWhen) {
		Timer *expiredTimer = static_cast<Timer*>(fTimerQueue.Dequeue());
		if (expiredTimer->fMode == kPeriodicTimer) {
			expiredTimer->fWhen += expiredTimer->fInterval;
			Enqueue(expiredTimer);
		} else
			expiredTimer->fPending = false;

		InterruptStatus ret = expiredTimer->HandleTimeout();
		if (ret == kReschedule)
			reschedule = true;
	}

	ReprogramHardwareTimer();
	return reschedule ? kReschedule : kHandledInterrupt;
}

void Timer::Enqueue(Timer *newTimer)
{
	if (fTimerQueue.GetTail() == 0)
		fTimerQueue.Enqueue(newTimer); // Empty list.  Add this as the only element.
	else if (static_cast<Timer*>(fTimerQueue.GetHead())->fWhen >= newTimer->fWhen)
		fTimerQueue.AddBefore(fTimerQueue.GetHead(), newTimer); // First Element.
	else {
		// Search newTimer list from end to beginning and insert this in order.
		for (Timer *timer = static_cast<Timer*>(fTimerQueue.GetTail()); timer;
			timer = static_cast<Timer*>(fTimerQueue.GetPrevious(timer))) {
			if (timer->fWhen < newTimer->fWhen) {
				fTimerQueue.AddAfter(timer, newTimer);
				break;
			}
		}
	}
}

void Timer::ReprogramHardwareTimer()
{
	Timer *head = static_cast<Timer*>(fTimerQueue.GetHead());
	if (head)
		SetHardwareTimer(head->fWhen - SystemTime());
}

void Timer::PrintTimerQueue(int, const char**)
{
	cpu_flags st = DisableInterrupts();
	bigtime_t now = SystemTime();
	printf("\nCurrent time %Ld\n", now);
	printf("  %8s %16s %16s\n", "Timer", "Wake time", "relative time");
	for (const Timer *timer = static_cast<const Timer*>(fTimerQueue.GetHead()); timer;
		timer = static_cast<const Timer*>(fTimerQueue.GetNext(timer))) {
		printf("  %p %16Li %16Li\n", timer, timer->fWhen, timer->fWhen - now);
	}

	RestoreInterrupts(st);
}
