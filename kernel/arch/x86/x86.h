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

//
//	Processor specific flags and structures
//

#ifndef _X86_H
#define _X86_H

#include "types.h"

struct Tss {
	int previousTask;				// Backlink for nested task
	int esp0, ss0;					// PL0 stack
	int esp1, ss1;					// PL1 stack
	int esp2, ss2;					// PL2 stack
	int cr3;						// Page directory base
	int eip;						// Instruction Pointer
	int eflags;					// Flags
	int eax, ecx, edx, ebx;		// General Purpose registers
	int esp, ebp;					// Stack Registers
	int esi, edi;					// Index Registers
	int es, cs, ss, ds, fs, gs;	// Selectors
	int ldt_seg;					// LDTR
	ushort trap_fl; 					// least sig. bit is trap flag	
	ushort iomap;						// IO permission map
};

struct GdtEntry {
	ushort limit0_15;
	ushort base0_15;	
	uchar base16_23;
	uchar type : 4;
	uchar system : 1; 		// 0 = system, 1 = application
	uchar dpl : 2; 			// Descriptor Privilege Level
	uchar present : 1;
	uchar limit16_19 : 4;
	uchar unused : 1;
	uchar zeroBit : 1;
	uchar defOpSize : 1; 	// Default operation size, 1 = 32 bit
	uchar granularity : 1; 	// 0 = 16 bit, 1 = 32 bit
	uchar base24_31;		
};

struct IdtEntry {
	ushort offset0_15;
	ushort selector;
	uchar zeroByte;	
	uchar type : 5;
	uchar dpl : 2;
	uchar present : 1;
	ushort offset16_31;
};

enum PteFlags {
	kPagePresent = 1,
	kPageWritable = 2,
	kPageUser = 4,
	kPageWriteThrough = 8,
	kPageCacheDisable = 16,
	kPageAccessed = 32,
	kPageModified = 64,
	kPageLarge = 128,
	kPageGlobal = 256
};

enum PageFaultFlags {
	kPageFaultProtection = 1,
	kPageFaultWrite = 2,
	kPageFaultUser = 4,
	kPageFaultReserved = 8
};

enum FaultType {
	kDivisionByZero,
	kDebugTrap,
	kNonMaskableInterrupt,
	kBreakPoint,
	kOverflow,
	kBoundsViolation,
	kInvalidOpcode,
	kDeviceNotAvailable,
	kDoubleFault,
	kCoprocessorAbort,
	kInvalidTss,
	kSegmentNotPresent,
	kStackFault,
	kGeneralProtectionFault,
	kPageFault,
	kFloatingPointError = 16,
	kAlignmentCheck,
	kMachineCheck,
	kPICBase = 32,
	kPICTop = 47,
	kSystemCall = 50,
	kMaxInterrupt
};

struct FpState {
	char data[108];
};

#endif
