
TARGET := $(BUILDHOME)/bin/testapp

include $(BUILDHOME)/make.rules

START_ADDRESS := 0x1000
CFLAGS += -fno-pic -fno-exceptions -fno-rtti -nostdinc
INCLUDES += -I $(BUILDHOME)/include
LIBS := $(BUILDHOME)/bin/libuser.a $(BUILDHOME)/bin/libc.a $(BUILDHOME)/bin/libgcc.a

SRCS := testapp.cpp test_fp.cpp test_vm.cpp test_prodcons.cpp test_wait.cpp \
	test_kill.cpp test_exec.cpp


OBJS := $(SRCS_LIST_TO_OBJS)

$(TARGET): makefile $(SRCS_LIST_TO_DEPS) $(OBJS)
	$(LD)  $(LDFLAGS) -o $(TARGET) $(BUILDHOME)/bin/crt0.o $(OBJS) $(LIBS)
