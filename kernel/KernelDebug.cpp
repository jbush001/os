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

#include "cpu_asm.h"
#include "ctype.h"
#include "interrupt.h"
#include "KernelDebug.h"
#include "List.h"
#include "stdio.h"
#include "string.h"

enum CharacterCodes {
	kBackspace = 8,
	kEscape = '\e',
	kNewline = '\r',
	kCarriageReturn = '\n'
};

enum EscapeCodes {
	kUpArrow = 65,
	kLeftArrow = 68,
	kRightArrow = 67,
	kDownArrow = 66,
};

static void ReadLine(char inputBuffer[], int size);
static int ParseArguments(char buf[], const char *argv[], int maxArgs);
static void DoHelp(int, const char**);
static void DoMemoryDump(int, const char**);
static void DoUptime(int, const char**);

// Defined in architecture dependent code
void DebugConsoleBootstrap();
char DebugConsoleRead();
void DebugConsoleWrite(const char);

const int kMaxDebugCommands = 64;
const int kCommandBufferLength = 128;
const int kMaxArgs = 16;
const int kCommandHistorySize = 10;
const int kBufferLength = 256;
static char tempBuffer[kBufferLength];
static int commandSlot;
static char commandHistory[kCommandHistorySize][kCommandBufferLength];
static bigtime_t bootTime;
static struct {
	const char *name;
	const char *description;
	DebugHook hook;
} debugCommands[kMaxDebugCommands];

void KernelDebugBootstrap()
{
	bootTime = SystemTime();
	DebugConsoleBootstrap();
	
	AddDebugCommand("help", "List debugger commands", DoHelp);
	AddDebugCommand("dm", "Show memory", DoMemoryDump);
	AddDebugCommand("uptime", "Time the machine has been running", DoUptime);
}

void Debugger()
{
	cpu_flags fl = DisableInterrupts();
	printf("\nJeffOS Kernel Debugger\n");
	for (;;) {
		printf("\n> ");
		ReadLine(tempBuffer, kBufferLength - 1);
		const char *argv[kMaxArgs];
		int argc = ParseArguments(tempBuffer, argv, kMaxArgs);
		if (argc == 0)
			continue;

		if (strcmp(argv[0], "continue") == 0 || strcmp(argv[0], "c") == 0)
			break;

		DebugHook hook = 0;		
		for (int index = 0; index < kMaxDebugCommands; index++) {
			if (debugCommands[index].hook != 0
				&& strcmp(debugCommands[index].name, argv[0]) == 0) {
				hook = debugCommands[index].hook;
				break;
			}
		}

		printf("\n");
		if (hook)
			hook(argc, argv);
		else
			printf("Huh?\a\n");
	}
	
	RestoreInterrupts(fl);
}

void panic(const char fmt[], ...)
{
	for (;;) {
		printf("\nKERNEL PANIC: ");

		va_list arglist;
		VA_START(arglist, fmt);
		vsnprintf(tempBuffer, kBufferLength, fmt, arglist);
		for (const char *c = tempBuffer; *c; c++)
			DebugConsoleWrite(*c);
	
		Debugger();
	}
}

void printf(const char fmt[], ...)
{
	cpu_flags fl = DisableInterrupts();
	va_list arglist;
	VA_START(arglist, fmt);
	vsnprintf(tempBuffer, kBufferLength, fmt, arglist);
	for (const char *c = tempBuffer; *c; c++)
		DebugConsoleWrite(*c);

	RestoreInterrupts(fl);
}

void AddDebugCommand(const char name[], const char description[], DebugHook hook)
{
	for (int index = 0; index < kMaxDebugCommands; index++) {
		if (debugCommands[index].hook == 0) {
			debugCommands[index].name = name;
			debugCommands[index].description = description;
			debugCommands[index].hook = hook;
			break;
		}
	}
}

void RemoveDebugCommand(DebugHook hook)
{
	for (int index = 0; index < kMaxDebugCommands; index++) {
		if (debugCommands[index].hook == hook) {
			debugCommands[index].hook = 0;
			break;
		}
	}
}

void bindump(const char data[], int size)
{
	const int kColumnCount = 16;
	for (int rowStart = 0; rowStart < size; rowStart += kColumnCount) {
		printf("\n%p  ", data + rowStart);
		for (int column = 0; column < kColumnCount; column++)
			printf("%02x ", data[rowStart + column]);

		printf(" ");
		for (int column = 0; column < kColumnCount; column++) {
			char c = data[rowStart + column];
			if (c > 32)
				printf("%c", c);
			else
				printf(".");
		}
	}
	
	printf("\n");
}

void __assert_failed(const char condition[], const char function[], const char file[], int line)
{
	printf("\nASSERT FAILED: (%s) in %s (%s:%d)\n", condition, function, file, line);
	Debugger();
}

static void ReadLine(char inputBuffer[], int size)
{
	int lastHistorySlot = 0;
	int inputPosition = 0;
	int currentBufferSize = 0;
	for (;;) {
		char c = DebugConsoleRead();
		switch (c) {
			case kBackspace:	{
				if (inputPosition > 0) {
					if (inputPosition < currentBufferSize - 1) {
						/* Backspace in middle of line */
						memcpy(inputBuffer + inputPosition - 1,
							inputBuffer + inputPosition,
							currentBufferSize - inputPosition);
						printf("\x1b[1D");	/* Left 1 */

						inputPosition--;
						currentBufferSize--;

						for (int index = inputPosition; index < currentBufferSize; index++) 
							DebugConsoleWrite(inputBuffer[index]);
						
						DebugConsoleWrite(32);

						/* Move back to input position */
						printf("\x1b[%dD", currentBufferSize - inputPosition + 1);
					} else {
						/* Backspace at end of line */
						printf("\x1b[1D");	/* Left 1 */
						printf("\x1b[K");	/* Erase to end of line */
						inputPosition--;
						currentBufferSize--;
					}					
					
				} else
					printf("\x7");
	
				break;
			}
			
			case kNewline:
			case kCarriageReturn:
				inputBuffer[currentBufferSize] = 0;
				strcpy(commandHistory[commandSlot], inputBuffer);
				commandSlot = (commandSlot + 1) % kCommandHistorySize;
				return;
				
			case kEscape: {
				c = DebugConsoleRead();	/* [ */
				c = DebugConsoleRead();
				switch (c) {
					case kLeftArrow:
						if (inputPosition > 0) {
							printf("\x1b[1D");	/* Left 1 */
							inputPosition--;				
						}
					
						break;			
					
					case kRightArrow:
						if (inputPosition < currentBufferSize) {
							printf("\x1b[1C");	/* Right 1 */	
							inputPosition++;
						}
					
						break;			
	
					case kUpArrow:		
					case kDownArrow:		
						if (c == kUpArrow) {
							if (lastHistorySlot < kCommandHistorySize)
								lastHistorySlot++;
							else {
								printf("\a");
								break;
							}
						} else {
							if (lastHistorySlot > 0)
								lastHistorySlot--;
							else if (currentBufferSize > 0) {
								/*
								 *	there are no more recent history items, just clear the
								 *  line.
								 */
								if (inputPosition > 0)
									printf("\x1b[%dD", inputPosition);	/* Beginning of line */
	
								printf("\x1b[K");	/* Clear to end of line */
								inputPosition = 0;
								currentBufferSize = 0;											
								break;
							} else
								printf("\a");
						}
	
						if (inputPosition > 0)
							printf("\x1b[%dD", inputPosition);	/* Beginning of line */
	
						strcpy(inputBuffer, commandHistory[(commandSlot + kCommandHistorySize -
							lastHistorySlot) % kCommandHistorySize]);
						currentBufferSize = strlen(inputBuffer);
						inputPosition = currentBufferSize;
						
						printf("%s", inputBuffer);
						printf("\x1b[K");	/* Clear to end of line */
						break;
				}
				
				break;
			}

			default: {
				if (currentBufferSize < size - 1) {
					/* Insert a character in the middle of the line */
					if (inputPosition < currentBufferSize) {
						currentBufferSize++;
						for (int index = currentBufferSize - 1; index > inputPosition; index--)
							inputBuffer[index] = inputBuffer[index - 1];					
	
						inputBuffer[inputPosition] = c;

						/* Write the rest of the line */
						for (int index = inputPosition; index < currentBufferSize; index++)
							DebugConsoleWrite(inputBuffer[index]);
						
						inputPosition++;

						/* Move back to input position */
						printf("\x1b[%dD", currentBufferSize - inputPosition);
					} else {
						/* Write a character at the end of the line */
						DebugConsoleWrite(c);
						inputBuffer[inputPosition++] = c;
						currentBufferSize++;
					}
				} else
					printf("\x7");
			}
		}
	}
}

static int ParseArguments(char inBuffer[], const char *arguments[], int kMaxArgs)
{
	int argumentCount = 0;
	enum {
		kScanArg,
		kScanSpace
	} state = kScanSpace;
	
	for (char *in = inBuffer; *in != 0 && argumentCount < kMaxArgs; in++) {
		switch (state) {
			case kScanArg:
				if (isspace(*in)) {
					state = kScanSpace;
					*in = '\0';
				}
					
				break;
				
			case kScanSpace:
				if (!isspace(*in)) {
					state = kScanArg;
					arguments[argumentCount++] = in;
				}
				
				break;
		}
	}

	return argumentCount;
}

void DoHelp(int, const char**)
{
	printf("\nCommands:\n");
	for (int index = 0; index < kMaxDebugCommands; index++)
		if (debugCommands[index].hook)
			printf("%20s %s\n", debugCommands[index].name, debugCommands[index].description);	
}

void DoUptime(int, const char*[])
{
	bigtime_t now = SystemTime() - bootTime;
	bigtime_t milliseconds = now / 1000;
	bigtime_t seconds = milliseconds / 1000;
	bigtime_t minutes = seconds / 60;
	bigtime_t hours = minutes / 60;
	bigtime_t days = hours / 24;
	printf("System has been running for ");
	if (days > 0)
		printf("%Ld days, ", days);

	if (hours > 0 || days > 0)
		printf("%Ld hours, ", hours % 24);

	if (minutes > 0 || hours > 0 || days > 0)
		printf("%Ld minutes, ", minutes % 60);

	printf("%Ld.%Ld seconds\n", seconds % 60, milliseconds % 1000);
}

static void DoMemoryDump(int argc, const char *argv[])
{
	if (argc != 3)
		printf("usage: %s <start> <size>\n", argv[0]);
	else
		bindump(reinterpret_cast<const char*>(atoi(argv[1])), atoi(argv[2]));
}
