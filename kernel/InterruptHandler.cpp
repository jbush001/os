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
#include "InterruptHandler.h"
#include "interrupt.h"
#include "KernelDebug.h"

InterruptHandler* InterruptHandler::fHandlers[kMaxInterrupts];

InterruptHandler::InterruptHandler()
	:	fVector(-1),
		fActive(false)
{
}

InterruptHandler::~InterruptHandler()
{
	if (fActive)
		panic("Attempt to delete active interrupt handler!");

	if (fVector != -1)
		panic("Attept to delete registered interrupt handler!\n");
}

InterruptStatus InterruptHandler::HandleInterrupt()
{
	return kUnhandledInterrupt;
}

void InterruptHandler::ObserveInterrupt(int vector)
{
	ASSERT(vector >= 0);
	ASSERT(vector <= kMaxInterrupts);

	cpu_flags st = DisableInterrupts();
	if (fHandlers[vector] == 0)
		EnableIrq(vector);

	fNext = fHandlers[vector];
	fHandlers[vector] = this;
	fVector = vector;
	RestoreInterrupts(st);
}

void InterruptHandler::IgnoreInterrupts()
{
	if (fVector == -1)
		panic("Attempt to unregister handler that isn't registered");

	cpu_flags st = DisableInterrupts();
	if (fHandlers[fVector] == 0)
		panic("Attempt to remove interrupt handler that is not installed");
		
	if (fHandlers[fVector] == this)
		fHandlers[fVector] = fNext;
	else {
		for (InterruptHandler *handler = fHandlers[fVector];;handler = handler->fNext) {
			if (handler->fNext == 0)
				panic("Interrupt handler is registered, but is not in list");
				
			if (handler->fNext == this) {
				handler->fNext = handler->fNext->fNext;
				break;
			}
		}
	}

	if (fHandlers[fVector] == 0)
		DisableIrq(fVector);

	fVector = -1;
	RestoreInterrupts(st);
}

InterruptStatus InterruptHandler::Dispatch(int vector)
{
	InterruptStatus result = kUnhandledInterrupt;
	for (InterruptHandler *handler = fHandlers[vector]; handler;
		handler = handler->fNext) {
		if (handler->fActive)
			panic("Interrupt handler called reentrantly");

		handler->fActive = true;
		result = handler->HandleInterrupt();
		handler->fActive = false;
		if (result != kUnhandledInterrupt)
			break;
	}

	return result;
}
