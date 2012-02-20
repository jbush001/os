/// @file ThreadContext.h
#ifndef _THREAD_CONTEXT_H
#define _THREAD_CONTEXT_H

#include "types.h"
#include "x86.h"

class PhysicalMap;

/// ThreadContext contains the architecture dependent state of a thread
class ThreadContext {
public:
	ThreadContext();	// Used for first task.
	ThreadContext(const PhysicalMap*);
	~ThreadContext();
	void Setup(thread_start_t, void *param, unsigned userStack,
		unsigned kernelStack);
	void SwitchTo();
	void PrintStackTrace() const;
	static void SwitchFp();

private:
	static void UserThreadStart(unsigned startAddress, unsigned userStack,
		unsigned param) NORETURN;

	unsigned fStackPointer;
	unsigned fPageDirectory;
	unsigned fKernelStackBottom;
	bool fKernelThread;
	FpState fFpState;
	static ThreadContext *fCurrentTask;

	// Note: this assumes single processor operation.  It should
	// move to Processor.
	static ThreadContext *fFpuOwner;
	static FpState fDefaultFpState;
};

#endif
