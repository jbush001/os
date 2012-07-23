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

#include "AddressSpace.h"
#include "cpu_asm.h"
#include "KernelDebug.h"
#include "ThreadContext.h"
#include "interrupt.h"
#include "InterruptHandler.h"
#include "memory_layout.h"
#include "Scheduler.h"
#include "stdio.h"
#include "string.h"
#include "SystemCall.h"
#include "Thread.h"
#include "types.h"
#include "x86.h"

#define IDT_ENTRY(func) { (unsigned int) func & 0xffff, 8, 0, 0xe, 3, 1, (unsigned int) func >> 16 }

extern "C" {
	void trap0(); void trap1(); void trap2(); void trap3(); void trap4();
	void trap5(); void trap6(); void trap7(); void trap8(); void trap9();
	void trap10(); void trap11(); void trap12(); void trap13(); void trap14();
	void trap16(); void trap17(); void trap18(); void trap32(); void trap33();
	void trap34(); void trap35(); void trap36(); void trap37(); void trap38();
	void trap39(); void trap40(); void trap41(); void trap42(); void trap43();
	void trap44(); void trap45(); void trap46(); void trap47(); void trap50();
	void bad_trap();
	void HandleTrap(InterruptFrame);
};

const int kMasterIcw1 = 0x20;
const int kMasterIcw2 = 0x21;
const int kSlaveIcw1 = 0xa0;
const int kSlaveIcw2 = 0xa1;
const int kUserCs = 0x1b;

static const IdtEntry idt[kMaxInterrupt] = {
	IDT_ENTRY(trap0), IDT_ENTRY(trap1), IDT_ENTRY(trap2), IDT_ENTRY(trap3),
	IDT_ENTRY(trap4), IDT_ENTRY(trap5), IDT_ENTRY(trap6), IDT_ENTRY(trap7),
	IDT_ENTRY(trap8), IDT_ENTRY(trap9), IDT_ENTRY(trap10), IDT_ENTRY(trap11),
	IDT_ENTRY(trap12), IDT_ENTRY(trap13), IDT_ENTRY(trap14), IDT_ENTRY(bad_trap),
	IDT_ENTRY(trap16), IDT_ENTRY(trap17), IDT_ENTRY(bad_trap),
	IDT_ENTRY(bad_trap), IDT_ENTRY(bad_trap), IDT_ENTRY(bad_trap),
	IDT_ENTRY(bad_trap), IDT_ENTRY(bad_trap), IDT_ENTRY(bad_trap), IDT_ENTRY(bad_trap),
	IDT_ENTRY(bad_trap), IDT_ENTRY(bad_trap), IDT_ENTRY(bad_trap), IDT_ENTRY(bad_trap),
	IDT_ENTRY(bad_trap), IDT_ENTRY(bad_trap), IDT_ENTRY(trap32), IDT_ENTRY(trap33),
	IDT_ENTRY(trap34), IDT_ENTRY(trap35), IDT_ENTRY(trap35), IDT_ENTRY(trap37),
	IDT_ENTRY(trap38), IDT_ENTRY(trap39), IDT_ENTRY(trap40), IDT_ENTRY(trap41),
	IDT_ENTRY(trap42), IDT_ENTRY(trap43), IDT_ENTRY(trap44), IDT_ENTRY(trap45),
	IDT_ENTRY(trap46), IDT_ENTRY(trap47), IDT_ENTRY(bad_trap), IDT_ENTRY(bad_trap),
	IDT_ENTRY(trap50)
};

// Speed of the machine in MHz.  This is currently hard coded for my test machine.
// It needs to be calculated at boot time.
static bigtime_t timeBase = 150;

void InterruptBootstrap()
{
	// Set up interrupt controllers.
	write_io_8(0x11, kMasterIcw1);	// Start initialization sequence for #1.
	write_io_8(0x11, kSlaveIcw1);	// ...and #2.
	write_io_8(0x20, kMasterIcw2);	// Set start of interrupts for #1 (0x20).
	write_io_8(0x28, kSlaveIcw2);	// Set start of interrupts for #2 (0x28).
	write_io_8(0x04, kMasterIcw2);	// Set #1 to be the master.
	write_io_8(0x02, kSlaveIcw2);	// Set #2 to be the slave.
	write_io_8(0x01, kMasterIcw2);	// Set both to operate in 8086 mode.
	write_io_8(0x01, kSlaveIcw2);
	write_io_8(0xfb, kMasterIcw2);	// Mask off all interrupts (except slave pic line).
	write_io_8(0xff, kSlaveIcw2); 	// Mask off interrupts on the slave.

	LoadIdt(idt, sizeof(idt));
	EnableInterrupts();
}

void EnableIrq(int irq)
{
	if (irq < 8)
		write_io_8(read_io_8(kMasterIcw2) & static_cast<unsigned char>(~(1 << irq)), kMasterIcw2);
	else
		write_io_8(read_io_8(kSlaveIcw2) & static_cast<unsigned char>(~(1 << (irq - 8))), kSlaveIcw2);
}

void DisableIrq(int irq)
{
	if (irq < 8)
		write_io_8(read_io_8(kMasterIcw2) | (1 << irq), kMasterIcw2);
	else
		write_io_8(read_io_8(kSlaveIcw2) | (1 << (irq - 8)), kSlaveIcw2);
}

void HandleTrap(InterruptFrame iframe)
{
	switch (iframe.vector) {
		case kDebugTrap: {
			unsigned int status = GetDR6();
			if (status & 7) {
				printf("Breakpoint\n");
				iframe.Print();
			} else if ((status & (1 << 14)) == 0) {
				printf("Unknown debug Trap\n");
				iframe.Print();
			}
			
			Debugger();
			break;
		}

		case kDeviceNotAvailable:
			ThreadContext::SwitchFp();
			break;

		case kPageFault: {
			// It is important to get the fault address (from cr2) before re-enabling
			// interrupts because it is not saved across task switches.
			unsigned int va = GetFaultAddress();
			EnableInterrupts();

#if DEBUG_SHIT
			if (iframe.errorCode & kPageFaultReserved)
				panic("Bad PTE entry (kernel fucked up)");
#endif
		
			AddressSpace *space = va >= kKernelBase
				? AddressSpace::GetKernelAddressSpace()
				: AddressSpace::GetCurrentAddressSpace();
			if (space->HandleFault(va, iframe.errorCode & kPageFaultWrite,
				iframe.errorCode & kPageFaultUser) < 0) {
				// Invalid page fault.  If there is a fault handler (used by CopyUser
				// functions), jump to it.
				if (Thread::GetRunningThread()->GetFaultHandler()) {
					printf("invoking fault handler\n");
					iframe.eip = Thread::GetRunningThread()->GetFaultHandler();
				} else {
					printf("unhandled page fault.\n");
					printf("Thread %s in %s mode attempted to %s %s address %08x\n",
						Thread::GetRunningThread()->GetName(), 
						(iframe.errorCode & kPageFaultUser) ? "user" : "supervisor",
						(iframe.errorCode & kPageFaultWrite) ? "write" : "read",
						(iframe.errorCode & kPageFaultProtection) ? "protected" : "unmapped", va);
					iframe.Print();
					Debugger();
				}
			}
			
			break;
		}

		case kSystemCall: {
			EnableInterrupts();
			const SystemCallInfo &info = systemCallTable[iframe.eax & 0xff];
			iframe.eax = InvokeSystemCall(&info.hook, reinterpret_cast<int*>(iframe.user_esp) + 1,
				info.parameterSize);

			break;
		}
		
		case 32: case 33: case 34: case 35: case 36: case 37: case 38: case 39:
		case 40: case 41: case 42: case 43: case 44: case 45: case 46: case 47: {
			InterruptStatus result = InterruptHandler::Dispatch(iframe.vector - 32);

			// Note: this assumes level triggered interrupts.  It works
			// with edge triggered ones, but may lose interrupts if the
			// ISR is slow.
			if (iframe.vector - 32 > 7)
				write_io_8(0x20, 0xa0);	// EOI to pic 2
	
			write_io_8(0x20, 0x20);	// EOI to pic 1
			
			if (result == kReschedule)
				gScheduler.Reschedule();
			else if (result == kUnhandledInterrupt) {
				iframe.Print();
				panic("Unhandled Interrupt");
			}

			break;
		}
		
		default:
			printf("Unknown trap %d occured.\n", iframe.vector);
			iframe.Print();
			Debugger();
	}	

	// Dispatch APC if one is pending.  This is only done before switching
	// back to user mode.	
	if (iframe.cs == kUserCs) {
		APC *apc = Thread::GetRunningThread()->DequeueAPC();
		if (apc) {
			APC temp = *apc;
			delete apc;
			if (temp.fIsKernel)
				(*temp.fCallback)(temp.fData);
			else
				panic("User APCs not implemented\n");
		}
	}
}

void InterruptFrame::Print() const
{
	const char kFlagLetters[] = "cxpxaxzstipollnxrv";
	printf("eax %08x       edi %08x\n", eax, edi);
	printf("ebx %08x       esi %08x\n", ebx, esi);
	printf("ecx %08x       ebp %08x\n", ecx, ebp);
	printf("edx %08x       ", edx);
	if ((cs & 3) == 0)
		printf("esp %08x\n", esp);
	else
		printf("esp %04x:%08x\n", user_ss, user_esp);
	
	printf("eip %04x:%08x  ", cs, eip);
	printf("eflags ");
	for (int i = 17; i >= 0; i--)
		printf("%c", (flags & (1 << i)) ? kFlagLetters[i] : kFlagLetters[i] +
			('A' - 'a'));
	printf("\ntrap %08x      error code %08x\n", vector, errorCode);
}

bigtime_t SystemTime()
{
	return rdtsc() / timeBase;
}

