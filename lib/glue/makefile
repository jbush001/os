
CFLAGS += -fno-pic -fno-exceptions -fno-rtti -nostdinc

all: FORCE
	$(CC) $(CFLAGS) -c crt0.c -o $(BUILDHOME)/bin/crt0.o
	cp libgcc.a $(BUILDHOME)/bin

clean:

FORCE: