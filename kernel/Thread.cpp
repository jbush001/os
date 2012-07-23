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

#include "AddressSpace.h"
#include "Area.h"
#include "cpu_asm.h"
#include "KernelDebug.h"
#include "PageCache.h"
#include "Scheduler.h"
#include "stdio.h"
#include "string.h"
#include "Team.h"
#include "Thread.h"
#include "VNode.h"

const unsigned int kKernelStackSize = 0x3000;
const unsigned int kUserStackSize = 0x20000;

Thread* Thread::fRunningThread = 0;
Queue Thread::fReapQueue;
Semaphore Thread::fThreadsToReap("threads to reap", 0);

Thread::Thread(const char name[], Team *team, thread_start_t startAddress, void *param,
	int priority)
	:	Resource(OBJ_THREAD, name),
		fThreadContext(team->GetAddressSpace()->GetPhysicalMap()),
		fBasePriority(priority),
		fCurrentPriority(priority),
		fFaultHandler(0),
		fLastEvent(SystemTime()),
		fState(kThreadCreated),
		fTeam(team),
		fKernelStack(0),
		fUserStack(0)
{
	char stackName[OS_NAME_LENGTH];
	snprintf(stackName, OS_NAME_LENGTH, "%.12s stack", name);

	fKernelStack = AddressSpace::GetKernelAddressSpace()->CreateArea(stackName,
		kKernelStackSize, AREA_WIRED, SYSTEM_READ | SYSTEM_WRITE, new PageCache, 0,
		INVALID_PAGE, SEARCH_FROM_TOP);
	if (fKernelStack == 0) {
		printf("team = %p\n", fTeam);
		panic("Can't create kernel stack for thread: out of virtual space\n");
	}

	unsigned int kernelStack = fKernelStack->GetBaseAddress() + kKernelStackSize - 4;
	unsigned int userStack = 0;
	if (team->GetAddressSpace() != AddressSpace::GetKernelAddressSpace()) {
		// Create the user stack
		fUserStack = fTeam->GetAddressSpace()->CreateArea(stackName, kUserStackSize,
			AREA_NOT_WIRED, USER_READ | USER_WRITE | SYSTEM_READ | SYSTEM_WRITE,
			new PageCache, 0, INVALID_PAGE, SEARCH_FROM_TOP);
		if (fUserStack == 0) {
			printf("team = %p\n", fTeam);
			panic("Can't create user stack for thread: out of virtual space\n");
		}

		userStack = fUserStack->GetBaseAddress() + kUserStackSize - 4;
	}

	fThreadContext.Setup(startAddress, param, userStack, kernelStack);

	// Inherit the current directory from the thread that created this.
	fCurrentDir = GetRunningThread()->fCurrentDir;
	if (fCurrentDir)
		fCurrentDir->AcquireRef();

	AcquireRef();	// This reference is effectively owned by the actual thread
					// of execution.  It will be released by the Grim Reaper
	team->ThreadCreated(this);
	gScheduler.EnqueueReadyThread(this);
	gScheduler.Reschedule();
}

void Thread::Exit()
{
	ASSERT(GetRunningThread() == this);

	cpu_flags fl = DisableInterrupts();
	SetState(kThreadDead);
	fReapQueue.Enqueue(this);
	fThreadsToReap.Release(1, false);
	RestoreInterrupts(fl);

	gScheduler.Reschedule();
	panic("terminated thread got scheduled");
}

void Thread::SwitchTo()
{
	cpu_flags cs = DisableInterrupts();
	fState = kThreadRunning;
	if (fRunningThread != this) {
		bigtime_t now = SystemTime();
		fRunningThread->fLastEvent = now;
		fLastEvent = now;
		fRunningThread = this;
		fThreadContext.SwitchTo();
	}

	RestoreInterrupts(cs);
}

bool Thread::CopyUser(void *dest, const void *src, int size)
{
	ASSERT(GetRunningThread() == this);

	return CopyUserInternal(dest, src, size, &fFaultHandler);
}

bigtime_t Thread::GetQuantumUsed() const
{
	if (fState == kThreadRunning)
		return SystemTime() - fLastEvent;

	return 0;
}

bigtime_t Thread::GetSleepTime() const
{
	if (fState == kThreadWaiting)
		return SystemTime() - fLastEvent;

	return 0;
}

APC* Thread::DequeueAPC()
{
	cpu_flags fl = DisableInterrupts();
	APC *apc = static_cast<APC*>(fApcQueue.Dequeue());
	RestoreInterrupts(fl);

	return apc;
}

void Thread::EnqueueAPC(APC *apc)
{
	cpu_flags fl = DisableInterrupts();
	fApcQueue.Enqueue(apc);
#if 0
	if (GetState() == kThreadWaiting)
		Wake(E_INTERRUPTED);
#endif

	RestoreInterrupts(fl);
}

void Thread::Bootstrap()
{
	fRunningThread = new Thread("init thread");
	AddDebugCommand("st", "Stack trace of current thread", StackTrace);
}

void Thread::SetKernelStack(Area *area)
{
	fKernelStack = area;
}

void Thread::SetTeam(Team *team)
{
	fTeam = team;
	team->ThreadCreated(this);

	// Threading is ready to go... start the Grim Reaper.
	// This is kind of a weird side effect, the function should be more explicit.
	new Thread("Grim Reaper", team, GrimReaper, this, 30);
}

// This is the constructor for bootstrap thread.  There is less state to
// setup since it is already running.
Thread::Thread(const char name[])
	:	Resource(OBJ_THREAD, name),
		fBasePriority(16),
		fCurrentPriority(16),
		fFaultHandler(0),
		fLastEvent(SystemTime()),
		fCurrentDir(0),
		fState(kThreadRunning),
		fKernelStack(0),
		fUserStack(0)
{
}

Thread::~Thread()
{
	AddressSpace::GetKernelAddressSpace()->DeleteArea(fKernelStack);
	if (fUserStack)
		fTeam->GetAddressSpace()->DeleteArea(fUserStack);

	fTeam->ThreadTerminated(this);
}

void Thread::StackTrace(int, const char**)
{
	GetRunningThread()->fThreadContext.PrintStackTrace();
}

// The Grim Reaper thread reclaims resources for threads and teams that
// have exited.
int Thread::GrimReaper(void*)
{
	for (;;) {
		fThreadsToReap.Wait();
		cpu_flags fl = DisableInterrupts();
		Thread *victim = static_cast<Thread*>(fReapQueue.Dequeue());
		RestoreInterrupts(fl);

		// The thread may not actually get deleted here if someone else has
		// a handle to it.
		victim->ReleaseRef();
	}
}
