#include <string.h>
#include "cpu_asm.h"
#include "PhysicalMap.h"
#include "stdio.h"
#include "syscall.h"
#include "ThreadContext.h"

#define PUSH(stack, value) 						\
	stack = (unsigned int)(stack) - 4; 				\
	*(unsigned int*)(stack) = (unsigned int)(value);

// One TSS is shared by all tasks.  It is only referenced by the processor
// during a switch from user to supervisor mode, in which case it is consulted
// to find the kernel stack pointer and selector.  It is not used for task switching,
// as a software based mechanism is used.
static Tss tss = {
	0, 0, 0x10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static GdtEntry gdt[] = {
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},					// Null segment	0x0
	{0xffff, 0, 0, 0xa, 1, 0, 1, 0xf, 0, 0, 1, 1, 0},			// OS Code		0x8
	{0xffff, 0, 0, 0x2, 1, 0, 1, 0xf, 0, 0, 1, 1, 0},			// OS Data		0x10
	{0xffff, 0, 0, 0xa, 1, 3, 1, 0xf, 0, 0, 1, 1, 0},			// User Code	0x1b
	{0xffff, 0, 0, 0x2, 1, 3, 1, 0xf, 0, 0, 1, 1, 0},			// User Data	0x23
	{sizeof(tss), reinterpret_cast<unsigned int>(&tss) & 0xffff,	// main TSS		0x28
		(reinterpret_cast<unsigned int>(&tss) >> 16) & 0xff, 9, 0, 0, 1, 0, 0, 0, 1, 1,
		(reinterpret_cast<unsigned int>(&tss) >> 24) & 0xff}
};

ThreadContext *ThreadContext::fCurrentTask = 0;
ThreadContext *ThreadContext::fFpuOwner = 0;
FpState ThreadContext::fDefaultFpState;

// This is used to set up the first task.
ThreadContext::ThreadContext()
	:	fStackPointer(0),
		fPageDirectory(GetCurrentPageDir()),
		fKernelStackBottom(0),
		fKernelThread(true)
{
	SaveFp(fDefaultFpState);
	LoadGdt(gdt, sizeof(gdt));
	fCurrentTask = this;
}

ThreadContext::ThreadContext(const PhysicalMap *physicalMap)
	:	fStackPointer(0),
		fPageDirectory(physicalMap->GetPageDir()),
		fKernelStackBottom(0),
		fKernelThread(physicalMap == PhysicalMap::GetKernelPhysicalMap())
{
}

ThreadContext::~ThreadContext()
{
	cpu_flags fl = DisableInterrupts();
	if (fFpuOwner == this)
		fFpuOwner = 0;

	RestoreInterrupts(fl);
}

void ThreadContext::Setup(thread_start_t startAddress, void *param,
	unsigned int userStack, unsigned int kernelStack)
{
	fKernelStackBottom = kernelStack;
	fStackPointer = kernelStack;
	memcpy(&fFpState, &fDefaultFpState, sizeof(FpState));

	if (fKernelThread) {
		// Set up call to kernel entry point
		PUSH(fStackPointer, thread_exit); // return address
		PUSH(fStackPointer, startAddress); // thread start address
	} else {
		// Set up call to UserThreadStart, passing user stack, entry point, and
		// desired parameters
		PUSH(fStackPointer, param);
		PUSH(fStackPointer, userStack)
		PUSH(fStackPointer, startAddress);
		PUSH(fStackPointer, 0);
		PUSH(fStackPointer, UserThreadStart);
	}

	// State saved in SwitchTo.  Note that interrupts start off for user threads
	// (as they call into the kernel function UserThreadStart and do some more
	// setup before jumping to user mode).
	PUSH(fStackPointer, fKernelThread ? (1 << 9) : 0);	// eflags
	PUSH(fStackPointer, 0);	// ebp
	PUSH(fStackPointer, 0);	// esi
	PUSH(fStackPointer, 0);	// edi
	PUSH(fStackPointer, 0);	// ebx
}

void ThreadContext::SwitchTo()
{
	ThreadContext *previousTask = fCurrentTask;
	fCurrentTask = this;
	tss.esp0 = fKernelStackBottom; // kernel stack for user threads
	if (fFpuOwner == this)
		ClearTrapOnFp(); // My data is still in the fpu
	else
		SetTrapOnFp(); 	// Another thread's data is in the fpu, trap if I need
						// to use it.

	// Note: the INVALID_PAGE will tell the context switching
	// code to *not* switch address spaces.
	ContextSwitch(&previousTask->fStackPointer, fCurrentTask->fStackPointer,
		fPageDirectory != previousTask->fPageDirectory && !fKernelThread ? fPageDirectory
		: INVALID_PAGE);
}

void ThreadContext::PrintStackTrace() const
{
	unsigned int *bottom = reinterpret_cast<unsigned int*>(__builtin_frame_address(1));
	printf("frame     return addr\n");
	for (const unsigned int *stackPtr = bottom; stackPtr < reinterpret_cast<unsigned int*>(fKernelStackBottom)
		&& stackPtr >= bottom;	stackPtr = reinterpret_cast<const unsigned int*>(*stackPtr))
		printf("%08x  %08x\n", *stackPtr, *(stackPtr + 1));
}

void ThreadContext::SwitchFp()
{
	ClearTrapOnFp();
	if (fFpuOwner)
		SaveFp(fFpuOwner->fFpState);

	RestoreFp(fCurrentTask->fFpState);
	fFpuOwner = fCurrentTask;
}

void ThreadContext::UserThreadStart(unsigned int startAddress, unsigned int userStack,
	unsigned int param)
{
	// First, put a small amount of code on the stack that
	// invokes the system call thread_exit().  The return address
	// for the first function call points to this code.  This allows
	// a user thread to simply return the entry function and self terminate
	// cleanly.
	// movl $3, %eax; int $50
	const unsigned char kExitStub[] = { 0xb8, 3, 0, 0, 0, 0xcd, 0x32 };
	userStack = (userStack - sizeof(kExitStub)) & ~3;
	memcpy(reinterpret_cast<void*>(userStack), kExitStub, sizeof(kExitStub));

	PUSH(userStack, param);				// Param for user start func
	PUSH(userStack, userStack + 8);		// Return address (the exit stub)

	SwitchToUserMode(startAddress, userStack);
}

