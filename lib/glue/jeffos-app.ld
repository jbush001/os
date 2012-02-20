OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
OUTPUT_ARCH(i386)
ENTRY(_start)
SECTIONS
{
  . = 0x1000 + SIZEOF_HEADERS;
  .interp     : { *(.interp) 	}
  .hash          : { *(.hash)		}
  .dynsym        : { *(.dynsym)		}
  .dynstr        : { *(.dynstr)		}
  .rel.text      :
    {
      *(.rel.text)
      *(.rel.text.*)
      *(.rel.gnu.linkonce.t*)
    }
  .rela.text     :
    {
      *(.rela.text)
      *(.rela.text.*)
      *(.rela.gnu.linkonce.t*)
    }
  .rel.data      :
    {
      *(.rel.data)
      *(.rel.data.*)
      *(.rel.gnu.linkonce.d*)
    }
  .rela.data     :
    {
      *(.rela.data)
      *(.rela.data.*)
      *(.rela.gnu.linkonce.d*)
    }
  .rel.rodata    :
    {
      *(.rel.rodata)
      *(.rel.rodata.*)
      *(.rel.gnu.linkonce.r*)
    }
  .rela.rodata   :
    {
      *(.rela.rodata)
      *(.rela.rodata.*)
      *(.rela.gnu.linkonce.r*)
    }
  .rel.got       : { *(.rel.got)		}
  .rela.got      : { *(.rela.got)		}
  .rel.ctors     : { *(.rel.ctors)	}
  .rela.ctors    : { *(.rela.ctors)	}
  .rel.dtors     : { *(.rel.dtors)	}
  .rela.dtors    : { *(.rela.dtors)	}
  .rel.init      : { *(.rel.init)	}
  .rela.init     : { *(.rela.init)	}
  .rel.fini      : { *(.rel.fini)	}
  .rela.fini     : { *(.rela.fini)	}
  .rel.bss       : { *(.rel.bss)		}
  .rela.bss      : { *(.rela.bss)		}
  .rel.plt       : { *(.rel.plt)		}
  .rela.plt      : { *(.rela.plt)		}
  .init          : { KEEP (*(.init))	} =0xcccc
  .plt      : { *(.plt)	}
  .text      :
  {
    *(.text)
    *(.text.*)
    *(.stub)
    *(.gnu.warning)
    *(.gnu.linkonce.t*)
  } =0xcccc
  _etext = .;
  PROVIDE (etext = .);
  .fini      : { KEEP (*(.fini))		} = 0xcccc
  .rodata    :
  {
    *(.rodata)
    *(.rodata.*)
    *(.gnu.linkonce.r*)
  }
  .rodata1   : { *(.rodata1) } = 0xcccc

  /* Writable segment */
  . = ALIGN(0x1000);
  .data    :
  {
    *(.data)
    *(.data.*)
    *(.gnu.linkonce.d*)
    SORT(CONSTRUCTORS)
  }
  .data1   : { *(.data1) }

  __CTORS__ = .;
  .ctors   : 
  {
    KEEP (*crtbegin.o(.ctors))
    KEEP (*(SORT(.ctors.*)))
    KEEP (*(.ctors))
    LONG(0)	/* Delimit list */
  }

  __DTORS__ = .;
   .dtors         :
  {
    KEEP (*crtbegin.o(.dtors))
    KEEP (*(SORT(.dtors.*)))
    KEEP (*(.dtors))
    LONG(0)	/* Delimit list */
  }

  .got           : { *(.got.plt) *(.got) }
  .dynamic       : { *(.dynamic) }
  .sdata     : { *(.sdata) *(.sdata.*) }
  _edata  =  .;
  PROVIDE (edata = .);
  . = ALIGN(0x1000);
  __bss_start = .;
  .sbss      : { *(.sbss) *(.scommon) }
  .bss       :
  {
   *(.dynbss)
   *(.bss)
   *(COMMON)
   . = ALIGN(32 / 8);
  }
  . = ALIGN(32 / 8);
  _end = . ;
  PROVIDE (end = .);
}
