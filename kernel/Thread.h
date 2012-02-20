/// @file Thread.h
#ifndef _THREAD_H
#define _THREAD_H

#include "APC.h"
#include "Resource.h"
#include "Queue.h"
#include "Semaphore.h"
#include "ThreadContext.h"
#include "Timer.h"
#include "types.h"

class Team;
class Area;
class VNode;

enum ThreadState {
	kThreadCreated,
	kThreadWaiting,
	kThreadReady,
	kThreadRunning,
	kThreadDead
};

/// A thread is a logical execution context.  This class contains all of the architecture
/// independent state for a thread.
class Thread : public Resource, public QueueNode {
public:
	Thread(const char name[], Team*, thread_start_t, void *param, int priority = 16);

	/// This function must be called from within the context of this thread.  This function
	/// will not return.  When this is called, the current thread will stop executing and
	/// its resources will be reclaimed by the operating system
	void Exit() NORETURN;

	/// Context switch from whichever thread is currently running to this thread.
	/// All state for the current running thread will be saved.  When the currently running
	/// thread is resumed, it will act as if it just returned from SwitchTo.
	void SwitchTo();

	/// Get a pointer to the thread that is currently running.
	static inline Thread *GetRunningThread();

	/// Get the state of this thread structure
	inline ThreadState GetState() const;

	/// Set the state of this thread structure
	inline void SetState(ThreadState);

	/// Return the team that this thread is associated with
	inline Team* GetTeam() const;

	/// Get the current priority of this thread.  The current priority starts out
	/// being the same os the base priority, but will change depending on how
	/// much IO this thread is performing (the range and meaning of the priorities
	/// are defined by the scheduler class)
	inline int GetCurrentPriority() const;

	/// Set the current priority of this thread
	inline void SetCurrentPriority(int);

	/// Get the base priority of this thread.  The base priority is generally
	/// defined by the application depending on the latency requirements of
	/// the functions it is performing.
	inline int GetBasePriority() const;

	/// Get the current file directory that filesystem operations in this thread
	/// are accessing, in the form of a VNode.
	inline VNode* GetCurrentDir() const;

	/// Set the current file directory that filesystem operations in this thread
	/// are accessing
	inline void SetCurrentDir(VNode*);

	/// Copy some data to or from user space from the address space that
	/// this thread is located in.  This function guarantees that an invalid
	/// page fault will not occur on behalf of kernel code (even though
	/// this may have process page faults that can be satisfied and thus
	/// may block for an indeterminate amount of time).
	/// @note This must be called in the context of the current running thread.
	/// @param dest Virtual address to copy data to
	/// @param src Virtual address to copy data from
	/// @returns
	///   - true if the data could successfully be copied
	///   - false if an unrecoverable page fault occured while copying data
	///      (note that a portion of the data may have been copied even if
	///      this function has failed)
	bool CopyUser(void *dest, const void *src, int size);

	/// Return the PC of the instruction right after CopyUser copies.
	/// If an invalid page fault occurs, execution will jump to this point and
	/// return an error.
	inline unsigned int GetFaultHandler() const;

	/// Return number of microseconds that this thread has been running
	bigtime_t GetQuantumUsed() const;

	/// Return number of microseconds that this thread has slept prior to
	/// waking up
	bigtime_t GetSleepTime() const;

	/// Enqueue an asynchronous procedure call.  This will be called in the
	/// context if this thread right before the thread returns to user space.
	/// An APC is akin to a UNIX signal, but is more general because any function
	/// can be called.
	void EnqueueAPC(APC*);

	/// Get the next asynchronous procedure call to be executed.
	APC* DequeueAPC();

	/// This is called once, from initialization code to initialize thread
	/// structures.
	static void Bootstrap();

	/// Used at boot time to initialize the first thread.  Set the area object
	/// associated with the kernel stack
	void SetKernelStack(Area*);

	/// Used at boot time to initialize the first thread.  Set the Team object
	/// associated with the kernel stack.
	void SetTeam(Team*);

private:
	/// This is used to bootstream the first thread.
	Thread(const char name[]);
	virtual ~Thread();

	/// Print a trace of the kernel functions called by this thread.
	static void StackTrace(int, const char**);

	/// The GrimReaper thread reclaims resources owned by threads
	/// Since this operation requires being in a thread, a thread can't really
	/// self destruct.
	static int GrimReaper(void*) NORETURN;

	ThreadContext fThreadContext;
	int fBasePriority;
	int fCurrentPriority;
	unsigned int fFaultHandler;
	bigtime_t fLastEvent;
	VNode *fCurrentDir;
	ThreadState fState;
	Team *fTeam;
	Area *fKernelStack;
	Area *fUserStack;
	Thread *fTeamListNext;
	Thread **fTeamListPrev;
	Queue fApcQueue;
	static Thread *fRunningThread;
	static Queue fReapQueue;
	static Semaphore fThreadsToReap;

	friend class Team;
};

inline Thread* Thread::GetRunningThread()
{
	return fRunningThread;
}

inline ThreadState Thread::GetState() const
{
	return fState;
}

inline void Thread::SetState(ThreadState state)
{
	fState = state;
}

inline Team* Thread::GetTeam() const
{
	return fTeam;
}

inline VNode* Thread::GetCurrentDir() const
{
	return fCurrentDir;
}

inline void Thread::SetCurrentDir(VNode *dir)
{
	fCurrentDir = dir;
}

inline unsigned int Thread::GetFaultHandler() const
{
	return fFaultHandler;
}

inline int Thread::GetCurrentPriority() const
{
	return fCurrentPriority;
}

inline int Thread::GetBasePriority() const
{
	return fBasePriority;
}

inline void Thread::SetCurrentPriority(int priority)
{
	fCurrentPriority = priority & 31;
}

#endif
