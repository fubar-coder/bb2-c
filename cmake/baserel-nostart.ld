/* Linker script for the -fbaserel programs without C startup code (ted,
   ab3cli; -nostartfiles): .bss is stored as zeros in the data hunk.

   Why (#963): with -fbaserel, data and bss are one hunk (a4 addresses
   both, ___a4_init = 0x7ffe into it), so the toolchain's default script
   appends .bss to the data hunk as its unstored tail: the hunk header
   gives the full size, the HUNK_DATA block stores only .data.  Kickstart
   1.x's LoadSeg clears only a HUNK_BSS and leaves such a tail as the
   memory was (2.0+ clears it).  libnix's startup (nbcrt0) clears the
   bss at run time, but these programs have none: their uninitialised
   globals would start with garbage.  Here .bss/COMMON go into .data, so
   the HUNK_DATA block covers the whole hunk (costs the bss size in the
   file) and the hunk stays ONE data hunk for a4-relative addressing.
   `hunkcheck --stored` checks the result after every link.

   Derived from the default -fbaserel script of the toolchain's GNU ld
   (m68k-amigaos-ld -fbaserel --verbose, binutils 2.46.1, AmigaDev
   16.2-rc21), which carries no copyright or license notice of its own;
   GNU binutils is GPL-3.0-or-later, as this repository.  Changes: .bss,
   .bss.* and COMMON moved into .data (__bss_start/__end mark them there,
   __edata after them), ___data_size without SIZEOF(.bss),
   ___bss_init_size = 0 (nothing to clear at run time), no ___a4_init2
   and no SEARCH_DIR (the compiler driver gives the library paths). */
OUTPUT_FORMAT("amiga")
OUTPUT_ARCH(m68k)
SECTIONS
{
  PROVIDE(___machtype = 0x0);
  . = 0x0;
  .text :
  {
    __stext = .;
    *(.text)
    *(.text.main)
    *(.text*)
    *(_*)
    *(.rodata*)
    *(.data.rel.ro*)
    *(.gnu.linkonce.t.*)
    *(.gnu.linkonce.r.*)
    *(.gcc_except_table*)
    *(SORT_BY_NAME(.list___EH_FRAME*))
    *(SORT_BY_NAME(.list_*))
    *(.end_of_lists)
    ___datadata_relocs = .;
    __etext = .;
  }
  ___text_size = SIZEOF(.text);
  . = ALIGN(0x0);
  .data :
  {
    __sdata = .;
    *(.data)
    *(SORT_BY_NAME(.data.*))
    CONSTRUCTORS
    *(SORT_BY_NAME(.dlist___EH_FRAME_OBJECT*))
    *(SORT_BY_NAME(.dlist_*))
    *(.end_of_dlists)
    *(.data.__EH_FRAME_OBJECT__*)
    *(.gnu.linkonce.d.*)
    ___a4_init = 0x7ffe;
    __bss_start = .;
    *(.bss)
    *(.bss.*)
    *(COMMON)
    __end = .;
    __edata = .;
  }
  ___data_size = SIZEOF(.data);
  ___bss_size = 0x0;
  ___bss_init_size = 0;
  .datachip :
  {
    *(.datachip)
  }
  .bsschip :
  {
    *(.bsschip)
  }
  /*
   * Individual DWARF sections are collected here.
   * The BFD backend (amigaos.c) will merge them into a single .dwarf2
   * section with length prefixes for the AmigaOS debug format.
   * The order here must match the order expected by amiga_pack_dwarf_sections()
   * in amigaos.c
   */
  .debug_frame :
  {
    __debug_frame_start = .;
    *(.debug_frame)
    __debug_frame_end = .;
  }
  .debug_info :
  {
    __debug_info_start = .;
    *(.debug_info)
    *(.gnu.linkonce.wi.*)
    __debug_info_end = .;
  }
  .debug_abbrev :
  {
    __debug_abbrev_start = .;
    *(.debug_abbrev)
    __debug_abbrev_end = .;
  }
  .debug_loclists :
  {
    __debug_loclists_start = .;
    *(.debug_loclists)
    __debug_loclists_end = .;
  }
  .debug_aranges :
  {
    __debug_aranges_start = .;
    *(.debug_aranges)
    __debug_aranges_end = .;
  }
  .debug_rnglists :
  {
    __debug_rnglists_start = .;
    *(.debug_rnglists)
    __debug_rnglists_end = .;
  }
  .debug_line :
  {
    __debug_line_start = .;
    *(.debug_line)
    *(.debug_line.*)
    __debug_line_end = .;
  }
  .debug_str :
  {
    __debug_str_start = .;
    *(.debug_str)
    __debug_str_end = .;
  }
  .debug_line_str :
  {
    __debug_line_str_start = .;
    *(.debug_line_str)
    __debug_line_str_end = .;
  }
  /* Additional DWARF sections for completeness */
  .debug_types :
  {
    __debug_types_start = .;
    *(.debug_types)
    __debug_types_end = .;
  }
  .debug_macro :
  {
    __debug_macro_start = .;
    *(.debug_macro)
    __debug_macro_end = .;
  }
  .debug_ranges :
  {
    __debug_ranges_start = .;
    *(.debug_ranges)
    __debug_ranges_end = .;
  }
  .debug_addr :
  {
    __debug_addr_start = .;
    *(.debug_addr)
    __debug_addr_end = .;
  }
  .debug_str_offsets :
  {
    __debug_str_offsets_start = .;
    *(.debug_str_offsets)
    __debug_str_offsets_end = .;
  }
}
