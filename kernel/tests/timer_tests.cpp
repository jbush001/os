#include "Timer.h"

int test_num = -1;
int interrupts_received = 0;
bigtime_t lastPeriodic;
const bigtime_t periodicInterval = 500000;

InterruptStatus timer_handler(void *param)
{
	kprintf("*\n");
	ASSERT(!INTERRUPTS_ON());
	switch (test_num) {
	case -1:
		panic("spurious timer interrupt\n");
		break;
	
	case 1:
		panic("cancel failed!\n");
		break;
	
	case 2:
		kprintf("Test #2: got interrupt correctly\n");
		break;
		
	case 3:
		interrupts_received++;
		kprintf(".");
		break;
		
	case 4: {
		bigtime_t now = SystemTime();
		ASSERT(now > lastPeriodic);
		
		bigtime_t late = (now - lastPeriodic) - periodicInterval;
		if (late > 10000) {
			kprintf("periodic timer was %L us late\n", late);
			panic("");
		}

		if (late < -1000) {
			kprintf("periodic timer was %L us too early\n", -late);
			panic("");
		}
			
		lastPeriodic = now;
		interrupts_received++;
		break;
	}
		
	default:
		kprintf("bad timer test num %d\n", test_num);
		panic("");
	}

	return HANDLED_INTERRUPT;	
}

void spin(bigtime_t us)
{
	bigtime_t timeup = SystemTime() + us;
	while (SystemTime() < timeup)
		;
}

void test_timers()
{
	kprintf("Running timer tests\n");
	
	// 1. Test cancelling a timer
	test_num = 1;
	Timer timer1;
	timer1.Set(SystemTime() + 100000, ONE_SHOT_TIMER, timer_handler, 0);
	if (timer1.Cancel() != true)
		panic("failed to cancel timer #1\n");
	
	test_num = -1;
	spin(2000000);
	kprintf("TIMER TEST #1 PASSED\n");

	// 2. Test cancelling a timer that has already expired
	test_num = 2;
	Timer timer2;
	timer2.Set(SystemTime() + 100000, ONE_SHOT_TIMER, timer_handler, 0);
	spin(1000000);
	if (timer2.Cancel() != false)
		panic("Cancelled expired timer!\n");

	// Make sure we don't get spurious interrupts
	test_num = -1;
	spin(2000000);
	
	kprintf("TIMER TEST #2 PASSED\n");
	
	// 3. Test 3 code paths for inserting timers.
	test_num = 3;
	interrupts_received = 0;
	Timer timer3;
	Timer timer4;
	Timer timer5;
	bigtime_t now = SystemTime();
	timer3.Set(now + 100000, ONE_SHOT_TIMER, timer_handler, 0);	// Only element
	timer4.Set(now + 50000, ONE_SHOT_TIMER, timer_handler, 0);	// New head of queue
	timer5.Set(now + 75000, ONE_SHOT_TIMER, timer_handler, 0);	// traverse queue
	spin(2000000);
	if (interrupts_received != 3)
		panic("Interrupt insertion failed\n");

	// Make sure we don't get spurious interrupts
	test_num = -1;
	spin(2000000);
			

	// 4. Test periodic timer
	test_num = 4;
	interrupts_received = 0;
	Timer timer6;
	lastPeriodic = SystemTime();
	timer6.Set(periodicInterval, PERIODIC_TIMER, timer_handler, 0);
	spin(periodicInterval * 10 + periodicInterval / 2);
	if (timer6.Cancel() != true)
		panic("failed to cancel periodic timer\n");
		
	test_num = -1;
	if (interrupts_received != 10)
		panic("received wrong number of periodic interrupts\n");

	// Make sure we don't get spurious interrupts
	spin(2000000);

	kprintf("TIMER TESTS PASSED\n");			
}

