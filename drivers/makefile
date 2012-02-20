TARGET := $(BUILDHOME)/bin/drivers.a

include $(BUILDHOME)/make.rules

CFLAGS += -fno-pic -fno-exceptions -fno-rtti
INCLUDES = -I$(BUILDHOME)/kernel -I$(BUILDHOME)/include -I$(BUILDHOME)/kernel/arch/$(ARCH)

SRCS :=  	Device.cpp \
			NetbootFS.cpp DevFS.cpp RootFS.cpp \
			IDE.cpp Keyboard.cpp VGAGraphics.cpp VGAText.cpp FAT.cpp NE2000.cpp \
			PCI.cpp Matrox.cpp Beep.cpp Floppy.cpp

OBJS := $(SRCS_LIST_TO_OBJS)

$(TARGET): makefile $(SRCS_LIST_TO_DEPS) $(OBJS)
	@echo "Building driver library..."
	@ar -ru $(TARGET) $(OBJS)
