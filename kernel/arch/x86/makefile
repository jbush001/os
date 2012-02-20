#
#	x86 hardware specific library
#

TARGET := $(BUILDHOME)/bin/arch-x86.a

include $(BUILDHOME)/make.rules

CFLAGS += -fno-pic -fno-exceptions -fno-rtti -DDEBUG=1
INCLUDES = -I$(BUILDHOME)/kernel -I$(BUILDHOME)/kernel/arch/$(ARCH) -I$(BUILDHOME)/include 

SRCS := ThreadContext.cpp PhysicalMap.cpp cpu_asm.s interrupt.cpp traps.S \
	SerialDebug.cpp Processor.cpp HardwareTimer.cpp

OBJS := $(SRCS_LIST_TO_OBJS)

$(TARGET): makefile $(SRCS_TO_DEPS) $(OBJS)
	@echo "Building x86 kernel library"
	@ar -ru $(TARGET) $(OBJS)
