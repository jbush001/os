/// @file InterruptHandler.h
#ifndef _INTERRUPT_HANDLER_H
#define _INTERRUPT_HANDLER_H

enum InterruptStatus {
	kUnhandledInterrupt,
	kHandledInterrupt,
	kReschedule
};

const int kMaxInterrupts = 32;

/// Base class for any class that handles hardware interrupts
class InterruptHandler {
public:
	InterruptHandler();
	virtual ~InterruptHandler();

	/// Derived classes override this function, which will be called when the interrupt occurs
	/// @returns InterruptStatus, used for chaining interrupts
	///   - kUnhandledInterrupt This device did not generate the interrupt, execute the next interrupt
	///      handler that is tied to this interrupt.
	///   - kHandledInerrupt This driver has handled this interrupt.  No need to call any other
	///      interrupt handlers.
	///   - kReschedule This driver has handled this interrupt and has also make one or more threads
	///      runnable.  Reschedule after returning from interrupt handler because the threads have
	///      tight latency requirements.
	virtual InterruptStatus HandleInterrupt();

	/// Begin watching an interrupt with the given interrupt vector.  If other InterruptHandlers
	/// are already watching this interrupt, this handler will be chained with them.
	/// @bug The behavior is undefined if you try to watch the same interrupt twice.  Should assert.
	void ObserveInterrupt(int vector);

	/// Don't watch any interrupts.
	void IgnoreInterrupts();

	/// Called by device dependent interrupt handler when an interrupt occurs.
	static InterruptStatus Dispatch(int vector);

private:
	InterruptHandler *fNext;
	int fVector;
	bool fActive;
	static InterruptHandler *fHandlers[kMaxInterrupts];
};

#endif
