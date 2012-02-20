OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)

ENTRY(_start)
SECTIONS
{
	. = 0xc0000000 + SIZEOF_HEADERS;

	/* text/read-only data */
	.text :	{ *(.text .gnu.linkonce.t.*) } = 0xcccc
	.rodata : { *(.rodata) } = 0xcccc

	/* writable data  */
	. = ALIGN(0x1000);
	__data_start = .;
	.data : { *(.data .gnu.linkonce.d.*) }

	__CTOR_LIST__ = .;
	.ctors : { *(.ctors) LONG(0) }

	/* unintialized data (in same segment as writable data) */
	.bss : { *(.bss) }
	_end = . ;

	/* Strip unnecessary stuff */
	/DISCARD/ : { *(.comment .note .eh_frame .dtors) }
}
