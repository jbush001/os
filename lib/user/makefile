
TARGET := $(BUILDHOME)/bin/libuser.a

include $(BUILDHOME)/make.rules

CFLAGS += -fno-pic -fno-exceptions -fno-rtti -nostdinc
INCLUDES += -I$(BUILDHOME)/include -I$(BUILDHOME)/kernel/arch/$(ARCH)

SRCS := misc.c malloc.c sbrk.c stdio.c syscalls.S 

OBJS := $(SRCS_LIST_TO_OBJS)

$(TARGET): makefile $(SRCS_LIST_TO_DEPS) $(OBJS)
	$(AR) -ru $(TARGET) $(OBJS)

