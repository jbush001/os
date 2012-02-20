#
#	Kernel Makefile
#

TARGET := $(BUILDHOME)/bin/kernel-$(ARCH)
SUBDIRS := arch/$(ARCH)

include $(BUILDHOME)/make.rules

ifeq ($(KMALLOC_DEBUG), 1)
	CFLAGS += -DMALLOC_DEBUG=1
endif

CFLAGS += -fno-pic -fno-rtti -fno-exceptions -DDEBUG=1
INCLUDES += -I$(BUILDHOME)/include -I$(BUILDHOME)/kernel/arch/$(ARCH) -I$(BUILDHOME)/kernel
LDFLAGS := --script kernel.ld --trace -v
LIBS := $(BUILDHOME)/bin/arch-$(ARCH).a $(BUILDHOME)/bin/drivers.a $(BUILDHOME)/bin/libc.a $(BUILDHOME)/bin/libgcc.a

SRCS :=	Runtime-GCC.cpp \
		main.cpp \
		AVLTree.cpp \
		KernelDebug.cpp \
		Alloc.cpp \
		Timer.cpp \
		Thread.cpp \
		Scheduler.cpp \
		Semaphore.cpp \
		Lock.cpp \
		AddressSpace.cpp \
		PageCache.cpp \
		Team.cpp \
		SystemCall.cpp \
		HandleTable.cpp \
		Image.cpp \
		Page.cpp \
		Resource.cpp \
		VNode.cpp \
		FileDescriptor.cpp \
		FileSystem.cpp \
		SwapSpace.cpp \
		InterruptHandler.cpp \
		Dispatcher.cpp

OBJS := $(SRCS_LIST_TO_OBJS)

$(TARGET): makefile $(SRCS_LIST_TO_DEPS) $(OBJS)
	@echo "Linking $@..."
	@$(LD) $(LDFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

FORCE:
