# rinonvolatilelib (35): nonvolatile.library

Reflective Images, 1994. An interface to the `nonvolatile.library` of
OS 3.1: `IsNonvolatile` (-1 if the library could be opened),
`StoreNVData AppName,ItemName,data,byte_length` (Amiga mode only; deletes
the item, then `StoreNV` with the length in tens of bytes, rounded up),
`GetCopyNV(AppName,ItemName)` (Amiga mode only), `FreeNVData APTR`,
`GetNVList(appname)`. init opens the library (any version), finit closes
it; every routine returns 0 when it is not open. killRequesters is -1
(StoreNV, GetCopyNV, GetNVList; 0 for the DeleteNV).

* **Source:** `rwelibsrc/nonvolatile.bb` (as TED shows it:
  `build/amiga/rwelibsrc/nonvolatile.bb`).
* **Reference:** BSS2 `RINONVOLATILELIB.OBJ`; declarations equal (ctest
  `rwe_declarations_rinonvolatilelib`).
* **C file:** `libs/userlibs/rwe/rinonvolatilelib.c`; test
  `libs/tests/rwe/rinonvolatilelib.c` (snitch case "rwe rinonvolatilelib").

## C and assembler

All routines are C behind `BLITZ_THUNK`, the system calls through the
NDK inlines (`proto/nonvolatile.h` with the base `libbase`); no other
assembler. `DIVU.w #10` is `bl_divu_w` (keeps the overflow behaviour).
The thunks keep d1/a0/a1, the C code a2/a6, which the original changes
through the system calls (StoreNVData also a2 = data). `IsNonvolatile`
keeps the upper word of d0 (`EXT.w`) like the original. StoreNVData
returns StoreNV's `UWORD` result zero-extended; the original passes on
the whole d0 of the library – only the low word is the command's result.

## BUG(orig)

* `StoreNVData`: `DIVU.w #10` overflows for 655360 bytes and more (the
  quotient does not fit into a word, d3 stays unchanged): StoreNV gets
  the low word of the length (+1 if the high word is not 0) as the number
  of tens – 655365 bytes become 60 (#764).
* `StoreNVData` deletes the item unconditionally before storing it (an
  item that cannot be stored again is lost); DeleteNV's result is ignored
  (#763).
* `StoreNVData` returns StoreNV's result (0 = stored, otherwise an
  `NVERR_…` code) - and 0 as well if the library is not open, i.e.
  "stored" (#765).

finit closes the library but leaves `libbase` set; that is no defect (it
is the last call).

## Test

Three rounds with init/finit: without the library (OpenLibrary patched to
fail for the two inits: every routine returns 0, no system call); with
the system's nonvolatile.library (in Amiberry it has no storage – it
looks for `SYS:prefs/env-archive/sys/nv_location`, the system disk is
read-only – so StoreNV fails with NVERR_FAIL and nothing can be read back;
only the answers are compared); with a fake `nonvolatile.library` placed
before the real one, which logs every call (LVO, a0/a1/a2/d0/d1) and
returns chosen values (0, 3, $12340001): the call sequence of every
routine, the lengths in tens for 0, 1, 10, 25, 65535, 655359 and 655365,
killRequesters, the results. Kept registers on every call. Not covered:
real storage (needs NVRAM or a writable `nv_location`).
