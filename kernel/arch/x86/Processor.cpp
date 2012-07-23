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
#include "Processor.h"
#include "cpu_asm.h"
#include "PhysicalMap.h"
#include "Team.h"
#include "Thread.h"

const int kApicPhysicalBase = 0xfee00000;
int* Processor::fLocalApicRegisters = 0;
Processor* Processor::fProcessors;

Processor::Processor()
{
	// There is a zero priority idle thread spawned for each processor.  The thread is
	// not specifically bound to that processor or even treated specially by
	// scheduler code.  This guarantees that there will always be a thread that is
	// ready to run, simplifying the scheduler.
	new Thread("Idle Thread", Thread::GetRunningThread()->GetTeam(), IdleLoop, 0, 0);
}

void Processor::Bootstrap()
{
	fLocalApicRegisters = reinterpret_cast<int*>(AddressSpace::GetKernelAddressSpace()
		->MapPhysicalMemory("Local APIC", kApicPhysicalBase, 0x1000, SYSTEM_READ
		| SYSTEM_WRITE | kUncacheablePage)->GetBaseAddress());
	fProcessors = new Processor[1];
}

Processor* Processor::GetCurrentProcessor()
{
	return &fProcessors[0];
}

int Processor::ApicID()
{
	return (fLocalApicRegisters[8] >> 24) & 0xf;
}

int Processor::IdleLoop(void*)
{
	for (;;)
		Halt();
}

