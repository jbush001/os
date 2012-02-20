//
//	Interface to 8253 Programmable Interrupt Timer
//

#include "cpu_asm.h"
#include "InterruptHandler.h"
#include "Timer.h"

typedef InterruptStatus (*TimerCallback)();
const bigtime_t kPitClockRate = 1193180;
const bigtime_t kMaxTimerInterval = (bigtime_t) 0xffff * 1000000 / kPitClockRate;

class TimerDispatcher : public InterruptHandler {
public:
	TimerDispatcher(TimerCallback callback);
	InterruptStatus HandleInterrupt();

private:
	TimerCallback fCallback;
};

TimerDispatcher::TimerDispatcher(TimerCallback callback)
	:	fCallback(callback)
{
	ObserveInterrupt(0);
}

InterruptStatus TimerDispatcher::HandleInterrupt()
{
	return fCallback();
}

void HardwareTimerBootstrap(TimerCallback callback)
{
	write_io_8(0x30, 0x43);		
	write_io_8(0, 0x40);
	write_io_8(0, 0x40);
	new TimerDispatcher(callback);
}

void SetHardwareTimer(bigtime_t relativeTimeout)
{
	bigtime_t nextEventClocks;
	if (relativeTimeout <= 0)
		nextEventClocks = 2;			
	else if (relativeTimeout < kMaxTimerInterval)
		nextEventClocks = relativeTimeout * kPitClockRate / 1000000;
	else
		nextEventClocks = 0xffff;

	write_io_8(0x30, 0x43);		
	write_io_8(nextEventClocks & 0xff, 0x40);
	write_io_8((nextEventClocks >> 8) & 0xff, 0x40);
}
