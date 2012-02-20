#ifndef _PROCESSOR_H
#define _PROCESSOR_H

class Processor {
public:
	static void Bootstrap();	
	static Processor* GetCurrentProcessor();

private:
	Processor();
	static int ApicID();
	static int IdleLoop(void*) NORETURN;

	static int *fLocalApicRegisters;
	static Processor *fProcessors;
};

#endif
