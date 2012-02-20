/// @file APC.h
#ifndef _APC_H
#define _APC_H

#include "Queue.h"

/// Asynchrnous Procedure Call
/// A function will be called in kernel mode on behalf of a thread that is just about to exit the kernel.
struct APC : public QueueNode {
	/// Function to invoke when this APC expires
	/// This function may be in user or kernel space
	void (*fCallback)(void *data);

	/// Data will be passed to callback function
	void *fData;

	/// True if this is a kernel mode APC, false if it is a user mode
	bool fIsKernel;
};

#endif
