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

#ifndef _INTERRUPT_H
#define _INTERRUPT_H

#include "types.h"

void InterruptBootstrap();
void EnableIrq(int);
void DisableIrq(int);

struct InterruptFrame {
	int edi;
	int esi;
	int ebp;
	int esp;
	int ebx;
	int edx;
	int ecx;
	int eax;
	int vector;
	int errorCode;
	int eip;
	int cs;
	int flags;
	int user_esp;
	int user_ss;
	
	void Print() const;
};

#endif
