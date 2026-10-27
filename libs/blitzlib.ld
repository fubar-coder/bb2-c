/* Linker script for Blitz Basic 2 libraries (see include/blitzlib.h).
   Everything goes into ONE code hunk, like a library built by Blitz2:
     header + token table      (.blitzlib.head, definition order)
     code, constants, data     (.text, .rodata, .data, .bss)
     error check routines      (.blitzlib.errchk; header field `error`)  */
OUTPUT_FORMAT("amiga")
OUTPUT_ARCH(m68k)
SECTIONS
{
  . = 0x0;
  .text :
  {
    _blitz_tok = .;           /* base for ALibJsr placeholders (blitzlib.h) */
    KEEP(*(.blitzlib.head))
    *(.text) *(.text.*)
    *(.rodata) *(.rodata.*)
    *(.data) *(.data.*)
    *(.bss) *(.bss.*) *(COMMON)
    . = ALIGN(2);
    _blitz_errchk_start = .;
    KEEP(*(.blitzlib.errchk))
    . = ALIGN(4);
  }
  /DISCARD/ : { *(.comment) *(.stab) *(.stabstr) *(.debug*) }
}
