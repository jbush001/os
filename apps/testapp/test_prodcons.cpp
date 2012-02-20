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
