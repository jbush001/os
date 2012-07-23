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
#include "Scheduler.h"
#include "Semaphore.h"
#include "string.h"
#include "Thread.h"

const int kQuantum = 8000;

Scheduler gScheduler;

Scheduler::Scheduler()
	:	fReadyThreadCount(0),
		fHighestReadyThread(0)
{
}

void Scheduler::Reschedule() 
{
	cpu_flags st = DisableInterrupts();
	Thread *thread = Thread::GetRunningThread();
	if (thread->GetState() == kThreadRunning) {
		if (fHighestReadyThread <= thread->GetCurrentPriority()
			&& thread->GetQuantumUsed() < kQuantum) {
			// This thread hasn't used its entire timeslice, and there
			// isn't a higher priority thread ready to run.  Don't reschedule.
			// Instead, continue running this thread.  Try to let the thread
			// use its entire timeslice whenever possible for better performance.
			RestoreInterrupts(st);
			return;
		}

		EnqueueReadyThread(thread);
	}

	CancelTimeout();	
	if (fReadyThreadCount > 1)
		SetTimeout(SystemTime() + kQuantum, kOneShotTimer);

	PickNextThread()->SwitchTo();
	RestoreInterrupts(st);
}

void Scheduler::EnqueueReadyThread(Thread *thread)
{
	cpu_flags st = DisableInterrupts();
	if (thread->GetSleepTime() > kQuantum * 4) {
		// Boost this thread's priority if it has been blocked for a while
		if (thread->GetCurrentPriority() < MIN(kMaxPriority, thread->GetBasePriority() + 3))
			thread->SetCurrentPriority(thread->GetCurrentPriority() + 1);
	} else {
		// This thread has run for a while.  If it's priority was boosted,
		// lower it back down.
		if (thread->GetCurrentPriority() > thread->GetBasePriority())
			thread->SetCurrentPriority(thread->GetCurrentPriority() - 1);
	}
	
	if (thread->GetCurrentPriority() > fHighestReadyThread)
		fHighestReadyThread = thread->GetCurrentPriority();
		
	fReadyThreadCount++;
	thread->SetState(kThreadReady);
	fReadyQueue[thread->GetCurrentPriority()].Enqueue(thread);
	RestoreInterrupts(st);
}

Thread *Scheduler::PickNextThread() 
{
	fReadyThreadCount--;
	Thread *nextThread = static_cast<Thread*>(fReadyQueue[fHighestReadyThread].Dequeue());
	while (fReadyQueue[fHighestReadyThread].GetHead() == 0)
		fHighestReadyThread--;

	ASSERT(nextThread);			
	return nextThread;
}

InterruptStatus Scheduler::HandleTimeout()
{
	return kReschedule;
}
