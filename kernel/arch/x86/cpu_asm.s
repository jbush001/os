# 
# Copyright 1998-2012 Jeff Bush
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# 

#
# Context Switch.
# Note that the current address is implicitly saved/restored
# by the call/return of this function.  Note also that other bits of code depend
# on the call frame layout of this function (when setting up a new thread, it
# starts exection at the 'switch to new stack' portion of this routine)
# This is why this isn't inline.
# Since this is an explicit function call, only non-volatile registers need to be saved
# (although flags also gets saved, to make sure the interrupt state is correct).
#
# 1. Store non-volatile registers on old stack
# 2. Save the stack pointer of the old process
# 3. Change to a new address space if necessary
# 4. Pop registers off new stack
# 5. Return to address on new stack, resuming thread
#



					.globl ContextSwitch
					.align 8
ContextSwitch:		pushf
					pushl %ebp
					pushl %esi
					pushl %edi
					pushl %ebx
					movl 24(%esp), %eax			# Get location to save stack pointer at
					movl %esp, (%eax)			# Save old stack pointer
					movl 32(%esp), %eax			# Get new PDBR
					movl 28(%esp), %ebx			# Get new stack pounter
					cmpl $0xffffffff, %eax		# Need to change addr. space?
					je skip_change_cr3 			# If parameter was -1, no
					movl %eax, %cr3				# Change address space
skip_change_cr3:	movl %ebx, %esp				# Switch to the new stack
					popl %ebx
					popl %edi
					popl %esi
					popl %ebp
					popf
					ret





#
# Jump to a specific system call.  This copies parameters from the user stack
# to the kernel stack before calling the function, then cleans up the stack.
#

					.globl InvokeSystemCall
					.align 8
InvokeSystemCall:	pushl %ebp
					movl %esp, %ebp
					movl 8(%ebp), %edx			# Get call function
					movl 12(%ebp), %esi			# Get stack data
					movl 16(%ebp), %ecx			# Get stack size
					testl %ecx, %ecx			# Is the number of params zero?
					jz no_params				# If so, skip copying the params
					movl %ecx, %eax				# Get number of params
					shl $2, %eax				# Multiply num params * 4 (size of int)
					subl %eax, %esp				# Reserve space on stack for params
					movl %esp, %edi				# Destination is reserved space on kernel stack
					rep
					movsl
no_params:			call *(%edx)				# Invoke kernel function
					movl %ebp, %esp				# Note: stack gets cleaned up here
					popl %ebp
					ret





					.global	write_io_str_16
					.align 8
write_io_str_16:	pushl %edx
					pushl %esi
					movl 12(%esp), %edx
					movl 16(%esp), %esi
					movl 20(%esp), %ecx
					rep
					outsw (%esi), %dx 
					popl %esi
					popl %edx
					ret



					.global	read_io_str_16
					.align 8
read_io_str_16:		pushl %edx
					pushl %edi
					movl 12(%esp), %edx
					movl 16(%esp), %edi
					movl 20(%esp), %ecx
					rep
					insw %dx, (%edi)
					popl %edi
					popl %edx
					ret

			

					.globl CopyUserInternal
					.align 8
CopyUserInternal:	pushl %edi
					pushl %esi
					pushl %ebx
					movl 28(%esp), %ebx
					movl $on_fault, (%ebx)
					xorl %eax, %eax				# Clear success flag
					movl 24(%esp), %ecx
					movl 16(%esp), %edi
					movl 20(%esp), %esi
					rep
					movsb
					movl $1, %eax				# Set success flag
on_fault:			movl $0, (%ebx)				# Clear fault handler
					popl %ebx
					popl %esi
					popl %edi
					ret



					.globl SwitchToUserMode
					.align 8
SwitchToUserMode:	movl 4(%esp), %eax			# start address
					movl 8(%esp), %ebx			# stack
					movw $0x20, %cx				# set up segment registers
					movw %cx, %ds
					movw %cx, %es
					movw %cx, %fs
					movw %cx, %gs

					# Set up a cross-protection level interrupt frame to jump to
					# the new thread.
					pushl $0x23					# User data segment
					pushl %ebx					# User stack
					pushl $(1 << 9) | 2			# User Flags (note that interrupts are on)
					pushl $0x1b					# User CS
					pushl %eax 					# EIP
					iret						# Jump to user space

					.global SetWatchpoint
					.align 8
SetWatchpoint:		movl $0x10001, %eax
					movl %eax, %dr7
					movl 4(%esp), %eax
					movl %eax, %dr0
					ret

					

					.globl GetDR6
					.align 8
GetDR6:				movl %dr6, %eax
					ret

					.end
