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
