#include <types.h>
#include <syscall.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int wait_invoker(void *_semID)
{
	int semID = (int) _semID;
	printf("wait_invoker started\n");
	sleep(1000000);
	printf("releasing sem\n");
	release_sem(semID, 1);
	return 0;
}

void test_wait()
{
	printf("Testing wait_for_multiple_objects()\n");
	int sem1 = create_sem("sem1", 0);
	int sem2 = create_sem("sem2", 0);
	int status;

	// time out waiting 
	object_id wait_vec[2];
	wait_vec[0] = sem1;
	wait_vec[1] = sem2;	

	status = wait_for_multiple_objects(2, wait_vec, 5000000, WAIT_FOR_ONE);
	if (status == E_TIMED_OUT)
		printf("TEST1: timed out OK\n");
	else
		printf("TEST1 FAILED: returned error %s\n", strerror(status));
		
	// an event occurs, wake
	spawn_thread(wait_invoker, "invoker", (void*) sem1, 16);
	status = wait_for_multiple_objects(2, wait_vec, 2000000, WAIT_FOR_ONE);
	if (status != E_NO_ERROR)
		printf("TEST2 FAILED: returned error\n");
	else
		printf("TEST 2 passed\n");

	// 
	status = release_sem(sem2, 1);
	printf("release sem: %s\n", strerror(status));
	
	// An object is already ready, don't block
	bigtime_t start_time = system_time();
	status = wait_for_multiple_objects(2, wait_vec, 1000000, WAIT_FOR_ONE);
	bigtime_t elapsed = system_time() - start_time;
	if (elapsed > 20000)
		printf("block on ready object FAIL %Ld\n", elapsed);
	
	if (status != E_NO_ERROR)
		printf("TEST4 FAILED: %s\n", strerror(status));
	else
		printf("TEST4 PASSED\n");

	printf("tests finished\n");
}
