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

/// @file Dispatcher.h

#ifndef _DISPATCHER_H
#define _DISPATCHER_H

#include "Queue.h"
#include "Timer.h"
#include "types.h"

/// Dispatcher is the fundamental synchronization primitive in the system,
/// from which every other synchronization primitives is derived.
/// Any thread that is blocked will be enqueued on one or more Dispatchers
/// Each dispatcher exists in either a signalled or unsignalled state.  If a dispatcher
/// is signalled, it means that threads that execute a wait on this or are already
/// waiting can be made ready.  If it is unsignalled, threads must block.
class Dispatcher {
public:
	Dispatcher();
	virtual ~Dispatcher();

	/// If this dispatcher is unsignalled, block until it either becomes signalled
	/// or the passed timeout elapses.
	/// @param timeout Maximum amount of time to wait, in microseconds.  If INFINITE_TIMEOUT
	///   iis passed, this will block forever.
	/// @returns
	///   -E_NO_ERROR if the dispatcher is signalled
	///   -E_TIMED_OUT if the timeout elapsed first
	status_t Wait(bigtime_t timeout = INFINITE_TIMEOUT);

	/// Wait on multiple dispatchers. The calling thread can wait for either at least one
	/// of the dispatchers to be signalled or for all of the dispatchers to be signalled.
	/// @param dispatcherCount Number of elements in dispatchers array
	/// @param dispatchers array of pointers of dispatcher objects to wait on
	/// @param flags flags describing how to wait, including whether to wait on one or all
	///    of the passed dispatchers to be signalled.
	/// @param timeout Maximum amount of time to wait, in microseconds.
	static status_t WaitForMultipleDispatchers(int dispatcherCount, Dispatcher *dispatchers[],
		WaitFlags flags, bigtime_t timeout = INFINITE_TIMEOUT);

protected:
	/// Called by derived classes to signal that threads can enter.  This function may
	/// make threads runnable as a side effect.
	/// @param reschedule  If this flag is true, the scheduler will be invoked immediately
	///  (allowing the newly woken threads to start running).  This allows lower latency
	///  for threads that need it.  If this flag is passed as false, the reschedule won't occur
	///  until the current thread has used its quantum.
	void Signal(bool reschedule);

	/// Called by derived classes to set the dispatchers state to unsignalled so threads
	/// will block.
	void Unsignal();

	/// This method should be overridden by derived classes.  Any time a thread calls
	/// Wait on a signalled Dispatcher, this method is called.  Derived classes use this function
	/// to seletively Unsignal the Dispatcher depending on their semantics.
	virtual void ThreadWoken();

private:
	static status_t WaitInternal(int dispatcherCount, Dispatcher *dispatchers[],
		WaitFlags flags, bigtime_t timeout, class WaitTag[]);
	bool fSignalled;
	Queue fTags;
};

#endif
