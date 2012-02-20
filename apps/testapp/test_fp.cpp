#include <syscall.h>
#include <stdlib.h>
#include <stdio.h>

static int fp_worker(void*)
{
	float a = 0.0;
	float b = 1.0;
	int update = 0;
	while (true) {
		b += 0.01;
		a = 2 * b;
		if (a != 2 * b)
			_serial_print("FP error\n");
			
		if (update++ > 10000) {
			printf(".");
			update = 0;
		}
	}
}

void test_fp()
{
	spawn_thread(fp_worker, "fp_worker", (void*) "-", 16);	
	spawn_thread(fp_worker, "fp_worker", (void*) "+", 16);
}

