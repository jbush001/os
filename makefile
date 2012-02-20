
SUBDIRS := drivers lib kernel boot apps

include $(BUILDHOME)/make.rules

run:
	@boot/bootmaker boot/netboot.cfg bin/boot_image.bin
	@boot/netboot bin/boot_image.bin $(DEBUG_MACHINE_IP)

floppy:
	@boot/bootmaker boot/netboot.cfg bin/floppy_image.bin -floppy 
	@dd if=bin/floppy_image.bin of=/dev/disk/floppy/raw bs=512
	@echo "Floppy written"

floppy_image:
	boot/bootmaker boot/netboot.cfg bin/floppy_image.bin -floppy 
