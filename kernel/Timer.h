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

/// @file Timer.h
#ifndef _TIMER_H
#define _TIMER_H

#include "InterruptHandler.h"
#include "Queue.h"
#include "types.h"

enum TimerMode {
	kPeriodicTimer,
	kOneShotTimer
};

/// Device indepent timer.  This simulates an arbritrary number of virtual hardware timers,
/// which are multiplexed onto a single hardware timer.  The HandleTimeout function will
/// be called when the timeout expires.  This class must be derived to use its functionality.
///@todo: Shouldn't Timer be derived from InterruptHandler?  It has a lot of similar semantics
class Timer : private QueueNode {
public:
	Timer();
	virtual ~Timer();

	/// Start the timer running with a specific timeout
	/// @param timeout
	///    - If this is a periodic timer this represents the relative time in microseconds between
	///       each timer event.
	///    - If this is a one-shot timer, this represents the absolute system time at which
	///       this timer should expire (in microseconds)
	/// @param mode Tells whether this timer is periodic or one shot.  If it is periodic,
	///    it will continue to expire repeatedly until CancelTimeout is called.
	void SetTimeout(bigtime_t timeout, TimerMode mode);

	/// Cancel an existing timeout.
	/// @returns
	///   - true if the timeout was still pending when this was cancelled.
	///   - If the timeout was not pending (the timer had already expired)
	/// @note it is safe to call this function from both interrupt and thread level, and
	///   it is safe to call it from the timer callback itself.
	bool CancelTimeout();

	/// Derived classes can override this function, which will be called when the timeout
	/// expires.
	/// @returns InterruptStatus
	///   - kReschedule if the scheduler should be invoked immediately
	///   - kHandledInterrupt if the scheduler does not need to be invoked immediately
	///   (kUnhandledInterrupt is ignored if returned)
	virtual InterruptStatus HandleTimeout();

	/// Called at boot time to initialize global timers
	static void Bootstrap();

private:
	/// This is called when the hardware timer expires
	static InterruptStatus HardwareTimerInterrupt();

	/// Stick the given timer into the timer queue, sorted by expiration
	static void Enqueue(Timer*);

	/// Set up the hardware timer for the expiration of the first (soonest) item in the timer queue
	static void ReprogramHardwareTimer();

	/// Print all data in timer queue (parameters are ignored)
	static void PrintTimerQueue(int, const char**);

	bigtime_t fWhen;
	TimerMode fMode;
	bool fPending;
	bigtime_t fInterval;
	static Queue fTimerQueue;
};

#endif
