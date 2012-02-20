#ifndef _INTERRUPT_H
#define _INTERRUPT_H

#include "types.h"

void InterruptBootstrap();
void EnableIrq(int);
void DisableIrq(int);

struct InterruptFrame {
	int edi;
	int esi;
	int ebp;
	int esp;
	int ebx;
	int edx;
	int ecx;
	int eax;
	int vector;
	int errorCode;
	int eip;
	int cs;
	int flags;
	int user_esp;
	int user_ss;
	
	void Print() const;
};

#endif
