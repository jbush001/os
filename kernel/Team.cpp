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
#include "cpu_asm.h"
#include "KernelDebug.h"
#include "stdio.h"
#include "Team.h"
#include "Thread.h"

List Team::fTeamList;

Team::Team(const char name[])
	:	Resource(OBJ_TEAM, name),
		fThreadList(0)
{
	fAddressSpace = new AddressSpace;
	cpu_flags fl = DisableInterrupts();
	fTeamList.AddToTail(this);	
	RestoreInterrupts(fl);
}

Team::~Team()
{
	cpu_flags fl = DisableInterrupts();
	fTeamList.Remove(this);
	RestoreInterrupts(fl);

	delete fAddressSpace;
}

void Team::ThreadCreated(Thread *thread)
{
	AcquireRef();
	cpu_flags fl = DisableInterrupts();
	thread->fTeamListNext = fThreadList;
	fThreadList = thread;
	thread->fTeamListPrev = &fThreadList;
	if (thread->fTeamListNext)
		thread->fTeamListNext->fTeamListPrev = &thread->fTeamListNext;
		
	RestoreInterrupts(fl);
}

void Team::ThreadTerminated(Thread *thread)
{
	cpu_flags fl = DisableInterrupts();
	*thread->fTeamListPrev = thread->fTeamListNext;
	if (thread->fTeamListNext)
		thread->fTeamListNext->fTeamListPrev = thread->fTeamListPrev;

	RestoreInterrupts(fl);
	ReleaseRef();
}

void Team::DoForEach(void (*EachTeamFunc)(void*, Team*), void *cookie)
{
	cpu_flags fl = DisableInterrupts();
	Team *team = static_cast<Team*>(fTeamList.GetHead());
	team->AcquireRef();
	while (team) {
		RestoreInterrupts(fl);
		EachTeamFunc(cookie, team);
		DisableInterrupts();
		Team *next = static_cast<Team*>(fTeamList.GetNext(team));
		if (next)
			next->AcquireRef();

		team->ReleaseRef();
		team = next;
	}
	
	RestoreInterrupts(fl);
}

void Team::Bootstrap()
{
	Team *init = new Team("kernel", AddressSpace::GetKernelAddressSpace());
	Thread::GetRunningThread()->SetTeam(init);
	AddDebugCommand("ps", "list running threads", PrintThreads);
	AddDebugCommand("areas", "list user areas", PrintAreas);
	AddDebugCommand("handles", "list handles", PrintHandles);
}

Team::Team(const char name[], AddressSpace *addressSpace)
	:	Resource(OBJ_TEAM, name),
		fAddressSpace(addressSpace),
		fThreadList(0)
{
	fTeamList.AddToTail(this);
}

void Team::PrintThreads(int, const char*[])
{
	const char *kThreadStateName[] = {"Created", "Wait", "Ready", "Running", "Dead"};
	int threadCount = 0;
	int teamCount = 0;
	for (const ListNode *node = fTeamList.GetHead(); node;
		node = fTeamList.GetNext(node)) {
		const Team *team = static_cast<const Team*>(node);
		teamCount++;
		printf("Team %s\n", team->GetName());
		printf("Name                 State    CPRI BPRI\n"); 
		for (Thread *thread = team->fThreadList; thread; thread = thread->fTeamListNext) {
			threadCount++;
			printf("%20s %8s %4d %4d\n", thread->GetName(),
				kThreadStateName[thread->GetState()],
				thread->GetCurrentPriority(), thread->GetBasePriority());
		}
		
		printf("\n");
	}

	printf("%d Threads  %d Teams\n", threadCount, teamCount);
}

// This is a little misplaced, but it is the easiest way to get to
// the team list.
void Team::PrintAreas(int, const char**)
{
	for (const ListNode *node = fTeamList.GetHead(); node;
		node = fTeamList.GetNext(node)) {
		const Team *team = static_cast<const Team*>(node);
		printf("Team %s\n", team->GetName());
		team->GetAddressSpace()->Print();
	}
}

void Team::PrintHandles(int, const char**)
{
	for (const ListNode *node = fTeamList.GetHead(); node;
		node = fTeamList.GetNext(node)) {
		const Team *team = static_cast<const Team*>(node);
		printf("Team %s\n", team->GetName());
		team->GetHandleTable()->Print();
	}
}
