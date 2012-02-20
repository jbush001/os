
TARGET := $(BUILDHOME)/bin/leaky

include $(BUILDHOME)/make.rules

START_ADDRESS := 0x1000
CFLAGS += -fno-pic -fno-exceptions -fno-rtti -nostdinc
INCLUDES += -I $(BUILDHOME)/include
LIBS := $(BUILDHOME)/bin/libuser.a $(BUILDHOME)/bin/libc.a $(BUILDHOME)/bin/libgcc.a

SRCS := leaky.cpp

OBJS := $(SRCS_LIST_TO_OBJS)

$(TARGET): makefile $(OBJS)
	$(LD)  $(LDFLAGS) -o $(TARGET) $(BUILDHOME)/bin/crt0.o $(OBJS) $(LIBS)
