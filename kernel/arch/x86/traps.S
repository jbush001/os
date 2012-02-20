#
#	Interrupt Handler Stubs.  These save all of the CPU state,
#	set up some stuff and call into HandleTrap().
#


						.text

#define TRAP_ERRC(vector) \
						.globl trap##vector; \
						.align 8; \
trap##vector:			pushl $vector; \
						jmp SetupTrapCommon


#define TRAP(vector) \
						.globl trap##vector; \
						.align 8; \
trap##vector:			pushl $0; \
						pushl $vector; \
						jmp SetupTrapCommon


						# Processor exceptions
						TRAP(0)
						TRAP(1)
						TRAP(2)
						TRAP(3)
						TRAP(4)
						TRAP(5)
						TRAP(6)
						TRAP(7)
						TRAP_ERRC(8)
						TRAP(9)
						TRAP_ERRC(10)
						TRAP_ERRC(11)
						TRAP_ERRC(12)
						TRAP_ERRC(13)
						TRAP_ERRC(14)
						TRAP(16)
						TRAP_ERRC(17)
						TRAP(18)

						# IO Interrupts
						TRAP(32)
						TRAP(33)
						TRAP(34)
						TRAP(35)
						TRAP(36)
						TRAP(37)
						TRAP(38)
						TRAP(39)
						TRAP(40)
						TRAP(41)
						TRAP(42)
						TRAP(43)
						TRAP(44)
						TRAP(45)
						TRAP(46)
						TRAP(47)

						# System call
						TRAP(50)

						# Bad Trap
						.globl 	bad_trap; \
						.align 	8; \
bad_trap:				pushl 	$0xffffffff; \
						pushl 	$0xffffffff; \
						jmp		SetupTrapCommon

						# All interrupt handler stubs jump here, which saves
						# the rest of the machine state and calls into HandleTrap.
						.extern HandleTrap
						.align 8
						.globl SetupTrapCommon
SetupTrapCommon:		pusha
						cld
						call HandleTrap
						popa
						addl $8, %esp
						iret

						.end

