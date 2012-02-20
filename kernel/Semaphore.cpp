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
