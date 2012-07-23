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
#include "KernelDebug.h"
#include "Semaphore.h"

Semaphore::Semaphore(const char name[], int initialCount)
	:	Resource(OBJ_SEMAPHORE, name),
		fCount(initialCount)
{
	if (initialCount < 0)
		panic("Illegal initial semaphore count");
		
	if (initialCount > 0)
		Signal(false);
}

void Semaphore::Release(int releaseCount, bool reschedule)
{
	cpu_flags cs = DisableInterrupts();
	int oldCount = fCount;
	fCount += releaseCount;
	if (oldCount == 0)
		Signal(reschedule);

	RestoreInterrupts(cs);
}

void Semaphore::ThreadWoken()
{
	if (--fCount == 0)
		Unsignal();
}
