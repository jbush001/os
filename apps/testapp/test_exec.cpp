#include <types.h>
#include <syscall.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int exec_loop(void *name)
{
	for (;;) {
		exec((char*) name);
		sleep(200000 + rand() % 50000);
	}
}

void test_exec()
{
	spawn_thread(exec_loop, "exec0", (void*) "/boot/leaky", 5);	
	sleep(3700);
	spawn_thread(exec_loop, "exec1", (void*) "/boot/leaky", 5);
}


