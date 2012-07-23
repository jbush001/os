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

#define BBSIZE 3192

struct bbuffer {
	long buffer[BBSIZE];
	object_id fullSlotsSem;
	object_id emptySlotsSem;
};

static int producer(void *_bbuf)
{
	bbuffer *bbuf = (bbuffer*) _bbuf;
	long produceSlot = 0;
	long nextItem = 1;
	while (true) {
		acquire_sem(bbuf->emptySlotsSem, INFINITE_TIMEOUT);
		bbuf->buffer[produceSlot] = nextItem++;
		produceSlot = (produceSlot + 1) % BBSIZE;
		release_sem(bbuf->fullSlotsSem, 1);
		printf("o");
	}
}

static int consumer(void *_bbuf)
{
	bbuffer *bbuf = (bbuffer*) _bbuf;
	long shouldGet = 1;
	long consumeSlot = 0;
	while (true) {
		acquire_sem(bbuf->fullSlotsSem, INFINITE_TIMEOUT);
		if (bbuf->buffer[consumeSlot] != shouldGet) {
			_serial_print("Consumer error:");
			return 0;
		} else
			printf("-");
		
		shouldGet++;
		consumeSlot = (consumeSlot + 1) % BBSIZE;
		release_sem(bbuf->emptySlotsSem, 1);
	}	
}

void test_producer_consumer()
{
	bbuffer b;
	b.fullSlotsSem = create_sem("buffer_full", 0);
	b.emptySlotsSem = create_sem("buffer_empty", BBSIZE);
	spawn_thread(producer, "producer", (void*) &b, 16);
	spawn_thread(consumer, "consumer", (void*) &b, 16);
	sleep(INFINITE_TIMEOUT);
}
