
TARGET := $(BUILDHOME)/bin/libc.a

include $(BUILDHOME)/make.rules

CFLAGS += -fno-pic -fno-exceptions -fno-rtti -nostdinc
INCLUDES += -I$(BUILDHOME)/include -I$(BUILDHOME)/kernel/arch/$(ARCH)

SRCS := string.c vsnprintf.c strerror.c snprintf.c ctype.c atoi.c rand.c malloc.c

OBJS := $(SRCS_LIST_TO_OBJS)

$(TARGET): makefile $(SRCS_LIST_TO_DEPS) $(OBJS)
	@echo "Building $(TARGET)"
	@$(AR) -ru $(TARGET) $(OBJS)

