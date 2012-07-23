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

#include <types.h>
#include <syscall.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void test_wait();
int test_vm();
void test_fp();
void test_exec();
void test_kill_thread();
void test_producer_consumer();
int test_loop(void*);
void test_kill_thread();
void test_fault();
void test_objects();
void time_faults();
void time_syscall();
void test_ide();
void test_heap();

int main()
{
	for (;;) {
		printf("Tests:\n");	
		printf("1. Wait for multiple objects\n");
		printf("2. Virtual Memory\n");
		printf("3. Floating point saving\n");
		printf("4. Exec/exit\n");
		printf("5. Crash\n");
		printf("6. Kill thread\n");
		printf("7. Producer/Consumer\n");
		printf("8. Fault\n");
		printf("9. Loop\n");
		printf("a. Many objects\n");
		printf("b. Time faults\n");
		printf("c. Time system call\n");
		printf("d. IDE drive\n");
		printf("e. Heap\n");
		printf("z. Quit\n");
		printf("> ");
		switch (getc()) {
			case '1':
				test_wait();
				break;	
			case '2':
				test_vm();
				break;
			case '3':
				test_fp();
				break;
			case '4':
				test_exec();
				break;
			case '5':
				*((char*) 0) = 1;
				printf("crash failed\n");
				break;
			case '6':
				test_kill_thread();
				break;
			case '7':
				test_producer_consumer();
				break;
			case '8':
				test_fault();
				break;
			case '9':
				spawn_thread(test_loop, "loop", 0, 16);
				break;
			case 'a':
				test_objects();
				break;
			case 'b':
				time_faults();
				break;
			case 'c':
				time_syscall();
				break;
			case 'd':
				test_ide();
				break;
			case 'e':
				test_heap();
				break;
			case 'z':
				return 0;
				
			default:
				printf("invalid test, try again\n");
				continue;
		}
	}
	
	return 0;
}

int test_loop(void*)
{
	for (;;)
		;

	return 0;
}

void test_objects()
{
	const int kNumObjects = 8192;
	int id[kNumObjects];
	for (;;) {
		printf("*");
		for (int i = 0; i < kNumObjects; i++)
			id[i] = create_sem("filler", 0);
	
		for (int i = 0; i < kNumObjects; i++)
			if (close_handle(id[i]) < E_NO_ERROR)
				_serial_print("! close_handle failed\n");
		
		for (int i = 0; i < kNumObjects; i++)
			if (close_handle(id[i]) >= E_NO_ERROR)
				_serial_print("! close_handle succeeded on bad object");

		sleep(500000);
	}
}

void time_syscall()
{
	const int calls = 10000;
	bigtime_t start = system_time();
	for (int i = 0; i < calls; i++)
		think();
	
	bigtime_t time1 = system_time() - start;
	printf("total time %Ld us time per call %d.%d us\n", time1, (int) (time1 / calls),
		(int) time1 % calls);
}

void test_ide()
{
	const int kBlockSize = 512;
	uchar test_buf[kBlockSize];
	int fd = open("/dev/disk/ide0", O_RDWR);
	for (int pattern = 0; pattern < 255; pattern++) {
		memset(test_buf, pattern, kBlockSize);
	
		if (write_pos(fd, 5120, (char*) test_buf, kBlockSize) < 0)
			printf("Write got error\n");
	
		memset(test_buf, 0xff, kBlockSize);
		if (read_pos(fd, 5120, (char*) test_buf, kBlockSize) < 0) {
			printf("Read got error\n");
			break;
		} else {
			for (int i = 0; i < kBlockSize; i++)
				if (test_buf[i] != pattern)
					printf("offset %d is not correct: %d\n", i, test_buf[i]);
		}
	
		printf(".");
	}

	printf("tests passed\n");	
	close_handle(fd);
}

void test_heap()
{
	int kMaxBlocks = 255;
	void *blocks[kMaxBlocks];

	for (int i = 0; i < kMaxBlocks; i++)
		blocks[i] = 0;

	for (int testCount = 0; testCount < 200000; testCount++) {
		int slot = rand() % kMaxBlocks;
		if (blocks[slot] == 0) {
			int blockSize = rand() % 2048;
			blocks[slot] = malloc(blockSize);
			memset(blocks[slot], 29, blockSize);
		} else {
			free(blocks[slot]);
			blocks[slot] = 0;
		}
		
		if ((testCount % 1000) == 0)
			printf(".");
		
	}

	for (int i = 0; i < kMaxBlocks; i++)
		free(blocks[i]);

	printf("looks good to me\n");
}
