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

#ifndef _CPU_ASM_H
#define _CPU_ASM_H

#include "types.h"
#include "x86.h"

typedef int cpu_flags;

typedef int (*CallHook)(...);

inline cpu_flags DisableInterrupts()
{
	cpu_flags fl;
	asm volatile("pushfl; popl %0; cli" : "=g" (fl));
	return fl;
}

inline void RestoreInterrupts(const cpu_flags flags)
{
	asm volatile("pushl %0; popfl\n" : : "g" (flags));
}

inline void EnableInterrupts()
{
	asm("sti");
}

/// Invalidate Translation Lookaside Buffer for a specific virtual address
/// This removes any cached page mappings for this address.  It must be called
/// whenever the mapping for a virtual address is changed.
inline void InvalidateTLB(unsigned int va)
{
	asm volatile("invlpg (%0)" : : "r" (va));
}

/// Returns the virtual address that was being accessed when the page fault occured
inline unsigned int GetFaultAddress()
{
	unsigned int retval;
	asm volatile("movl %%cr2, %0" : "=g" (retval));
	return retval;
}

inline void write_io_8(int value, int port)
{
	asm volatile("outb %%al, %%dx" : : "a" (value), "d" (port));
}

inline void write_io_16(int value, int port)
{
	asm volatile("outw %%ax, %%dx" : : "a" (value), "d" (port));
}

inline void write_io_32(int value, int port)
{
	asm volatile("outl %%eax, %%dx" : : "a" (value), "d" (port));
}

inline unsigned int read_io_8(unsigned int port)
{
	unsigned char retval;
	asm volatile("inb %%dx, %%al" : "=a" (retval) : "d" (port));
	return retval;
}

inline unsigned int read_io_16(unsigned int port)
{
	unsigned short retval;
	asm volatile("inw %%dx, %%ax" : "=a" (retval) : "d" (port));
	return retval;
}

inline unsigned int read_io_32(unsigned int port)
{
	unsigned int retval;
	asm volatile("inl %%dx, %%eax" : "=a" (retval) : "d" (port));
	return retval;
}

inline void SaveFp(FpState &state)
{
	asm volatile("fnsave (%0); fwait" : : "r" (&state));
}

inline void RestoreFp(const FpState &state)
{
	asm volatile("frstor (%0)" : : "r" (&state));
}

inline void ClearTrapOnFp()
{
	asm volatile("clts");
}

inline void SetTrapOnFp()
{
	asm volatile("movl %cr0, %eax; orl $8, %eax; movl %eax, %cr0");
}

inline bool _get_interrupt_state()
{
	unsigned int result;
	asm("pushfl; popl %0" : "=m" (result));
	return (result & (1 << 9)) != 0;
}

/// Return the physical address of the current page directory
inline unsigned int GetCurrentPageDir()
{
	unsigned int val;
	asm("movl %%cr3, %0" : "=q" (val));
	return val;
}

/// Set the physical address of the current page directory
inline void SetCurrentPageDir(unsigned int addr)
{
	asm("movl %0, %%cr3" : : "q" (addr));
}

inline bool cmpxchg32(volatile int *var, int oldValue, int newValue)
{
	int success;
	asm volatile("lock; cmpxchg %%ecx, (%%edi); sete %%al; andl $1, %%eax"
		: "=a" (success)
		: "a" (oldValue), "c" (newValue), "D" (var));

	return success;
}

inline int64 rdtsc()
{
	unsigned int high, low;
	asm("rdtsc" : "=a" (low), "=d" (high));
	return (int64) high << 32 | low;
}

inline void LoadIdt(const IdtEntry base[], unsigned int limit)
{
	struct desc {
		unsigned short limit;
		const IdtEntry *base;
	} PACKED;
	desc d;
	d.base = base;
	d.limit = limit;
	asm("lidt (%0)" : : "r" (&d));
}

inline void LoadGdt(const GdtEntry base[], unsigned int limit)
{
	struct desc {
		unsigned short limit;
		const GdtEntry *base;
	} PACKED;
	desc d;
	d.base = base;
	d.limit = limit;
	asm("lgdt (%0);"
		"movw $0x10, %%ax;"
		"movw %%ax, %%ds;"
		"movw %%ax, %%es;"
		"movw %%ax, %%gs;"
		"movw %%ax, %%fs;"
		"movw %%ax, %%ss;"
		"movw $0x28, %%ax;"
		"ltr %%ax;"
		: : "r" (&d) : "eax");
}

inline void Halt()
{
	asm("hlt");
}

inline void ClearPage(void *va)
{
	int dummy0, dummy1;
	asm volatile("rep; stosl"
		: "=&c" (dummy0), "=&D" (dummy1)
		: "a" (0), "1" (va), "0" (PAGE_SIZE / 4));
}

inline void CopyPageInternal(void *dest, const void *src)
{
	int dummy0, dummy1, dummy2;
	asm volatile("rep; movsl"
		: "=&c" (dummy0), "=&D" (dummy1), "=&S" (dummy2)
		: "a" (0), "0" (PAGE_SIZE / 4), "1" (dest), "2" (src));
}

inline int AtomicAdd(volatile int *var, int val)
{
	int oldVal;
	int dummy;
	asm volatile(
		"1:"
		"movl (%%edi), %%eax;"
		"movl %%eax, %%ecx;"
		"addl %%edx, %%ecx;"
		"lock; cmpxchg %%ecx, (%%edi);"
		"jnz 1b;"
		: "=a" (oldVal), "=c" (dummy)
		: "D" (var), "d" (val));

	return oldVal;
}

inline int AtomicAnd(volatile int *var, int val)
{
	int oldVal;
	int dummy;
	asm volatile(
		"1:"
		"movl (%%edi), %%eax;"
		"movl %%eax, %%ecx;"
		"andl %%edx, %%ecx;"
		"lock; cmpxchg %%ecx, (%%edi);"
		"jnz 1b;"
		: "=a" (oldVal), "=c" (dummy)
		: "D" (var), "d" (val));

	return oldVal;
}

inline int AtomicOr(volatile int *var, int val)
{
	int oldVal;
	int dummy;
	asm volatile(
		"1:"
		"movl (%%edi), %%eax;"
		"movl %%eax, %%ecx;"
		"orl %%edx, %%ecx;"
		"lock; cmpxchg %%ecx, (%%edi);"
		"jnz 1b;"
		: "=a" (oldVal), "=c" (dummy)
		: "D" (var), "d" (val));

	return oldVal;
}

extern "C" {
	bigtime_t SystemTime();

	void write_io_str_16(int port, short buf[], int count);
	void read_io_str_16(int port, short buf[], int count);

	int CopyUserInternal(void *dest, const void *src, unsigned int size, unsigned int *handler);

	// Yes, func is a pointer to a pointer to a function.  Sorry.
	int InvokeSystemCall(const CallHook *func, int stackData[], int stackSize);

	void SetWatchpoint(unsigned int va);
	unsigned int GetDR6();
	void ContextSwitch(unsigned int *oldEsp, unsigned int newEsp, unsigned int pdbr);
	void SwitchToUserMode(unsigned int _start, unsigned int user_stack) NORETURN;
};

#endif
