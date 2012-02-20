#include <types.h>
#include <syscall.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int victim1(void*)
{
	acquire_sem(create_sem("test", 0), INFINITE_TIMEOUT);
}

int victim2(void*)
{
	for (;;)
		printf("victim2\n");
}

void test_kill_thread()
{
	int vthr;
	
	printf("spawn victim thread 1\n");
	vthr = spawn_thread(victim1, "victim", 0, 16);
	sleep(250000);
	kill_thread(vthr);
	printf("killed it\n");

	printf("spawn victim thread 2\n");
	vthr = spawn_thread(victim2, "victim", 0, 16);
	sleep(250000);
	kill_thread(vthr);
	printf("killed it\n");
}
