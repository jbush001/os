

#define SYSCALL(_name_, _number_)							\
						.globl	_name_;						\
			_name_:		movl	$_number_, %eax;			\
						int		$50;						\
						ret

	SYSCALL(sleep, 0)
	SYSCALL(_serial_print, 1)
	SYSCALL(spawn_thread, 2)
	SYSCALL(thread_exit, 3)
	SYSCALL(create_sem, 4)
	SYSCALL(acquire_sem, 5)
	SYSCALL(release_sem, 6)

	SYSCALL(think, 7)
	
	
	SYSCALL(open, 8)
	SYSCALL(close_handle, 9)
	SYSCALL(stat, 10)
	SYSCALL(ioctl, 11)
	SYSCALL(write, 12)
	SYSCALL(read, 13)
	SYSCALL(mkdir, 14)
	SYSCALL(rmdir, 15)
	SYSCALL(rename, 16)
	SYSCALL(readdir, 17)
	SYSCALL(rewinddir, 18)
	SYSCALL(lseek, 19)
	
	SYSCALL(exec, 20)

	SYSCALL(create_area, 21)
	SYSCALL(clone_area, 22)
	SYSCALL(delete_area, 23)
	SYSCALL(resize_area, 24)

	SYSCALL(wait_for_multiple_objects, 25)
	SYSCALL(kill_thread, 27)
	SYSCALL(read_pos, 28)
	SYSCALL(write_pos, 29)
	SYSCALL(chdir, 30)
	SYSCALL(getcwd, 31)
	SYSCALL(mount, 32)
	SYSCALL(map_file, 33)
	
								.globl	atomic_add
			atomic_add:			pushl	%ebx
								movl	8(%esp), %ecx		# pointer to variable
			try_add:			movl	(%ecx), %eax		# get original value
								movl	12(%esp), %ebx		# get amount to add
								addl	%eax, %ebx			# what result should be
								lock
								cmpxchg	%ebx, (%ecx)
								jne		try_add				# failed, try again
								popl	%ebx
								ret
	
								.globl	atomic_and
			atomic_and:			pushl	%ebx
								movl	8(%esp), %ecx		# pointer to variable
			try_and:			movl	(%ecx), %eax		# get original value
								movl	12(%esp), %ebx		# get amount to add
								andl	%eax, %ebx			# what result should be
								lock
								cmpxchg	%ebx, (%ecx)
								jne		try_and				# failed, try again
								popl	%ebx
								ret
	
								.globl	atomic_or
			atomic_or:			pushl	%ebx
								movl	8(%esp), %ecx		# pointer to variable
			try_or:				movl	(%ecx), %eax		# get original value
								movl	12(%esp), %ebx		# get amount to add
								orl		%eax, %ebx			# what result should be
								lock
								cmpxchg	%ebx, (%ecx)
								jne		try_or				# failed, try again
								popl	%ebx
								ret
	

									.end
