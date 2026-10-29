# rireqlib (79): req.library requesters

Reflective Images. The file and colour requesters of `req.library`:
`ReqFileRequest[([title$][,flags])]` (Amiga mode only; the path chosen,
empty on cancel), `ReqFileLoc` (address of the path buffer),
`ReqOutput window#` (window of both requesters), `ReqFlags flags`,
`ReqColour(colour)`. init opens req.library (any version) and allocates
one block of 940 bytes for the requester's buffers; finit closes the
library and frees the block.

* **Source:** `rwelibsrc/reqlib/ReqLib2.bb` (as TED shows it:
  `build/amiga/rwelibsrc/reqlib/ReqLib2.bb`). ReqLib, ReqLib2 and ReqLib3
  have the same declarations; the reference build of ReqLib2 is the one
  that equals the binary (`docs/rwelibs/README.md` 3.1).
* **Reference:** OL `RIREQLIB.OBJ`; declarations equal (ctest
  `rwe_declarations_rireqlib`).
* **C file:** `libs/userlibs/rwe/rireqlib.c`; test
  `libs/tests/rwe/rireqlib.c` (snitch case "rwe rireqlib").

## C and assembler

All routines are C behind `BLITZ_THUNK`; the requesters are called with
`BLITZ_BASEJSR` (a0 = the structure, a6 = the library base), the result
string is written through `blitz_string` (a3). `ReqOutput` gets the window
object in a0 (`!libs {#windowslib,#ia0|#pd0}` = `BL_LA(0) | BL_ITEM |
BL_PD(0)`). The structures `FrStruct` (34 bytes + 32 longs) and the colour
requester's are packed C structs with the original's initial values. The
title length is read from -4(a2) without changing a2 (DIFF as in all C
ports, `docs/libs/cmdlibs-port.md` "General"). init returns d0 unchanged
when the library is already open, like the original. The thunks keep
d1/a0/a1 where the original keeps them, ReqFileLoc returns the buffer in
a0 too.

## BUG(orig)

* `#frq_Flags = #FRQCACHINGB|#FRQINFOGADGETB|#FRQCACHEPURGEB` ORs bit
  numbers (2|4|7 = 7): the default flags are bits 0-2 (extended select,
  caching) instead of caching, info gadget and cache purge.
* init stores the four buffer addresses in `frq_Title`…`frq_PathName`
  behind the structure, never in `FrStruct`'s Directory/File/PathName
  fields: the requester gets 0 for them, and ReqFileRequest copies the
  cleared path buffer of init, which only the program (through
  ReqFileLoc) writes. Unless req.library copes with the 0 pointers itself,
  the result is always empty.
* No routine checks the library base: without req.library (or after
  init's AllocMem failed, which closes it again) ReqFileRequest and
  ReqColour call through a6 = 0.
* ReqFileRequest leaves a3 behind the terminating 0 of the result (d0 =
  length is right); see `docs/libs/cmdlibs-port.md`, "String results".
  On cancel d0 = 0 and a3 stays (with a title: at its start).
* ReqColour ignores its argument: DefColour stays 0.
* ReqFileRequest(title$,flags) changes the flags for good (like
  ReqFlags), and a title stays for later calls without one.

## Test

The requesters need a user, so the test adds a fake `req.library` to the
library list; its FileRequester and ColorRequester record the structure
they are given (162 / 30 bytes) and return a chosen value. Compared with
the original: init when OpenLibrary fails, when AllocMem fails (the
library is closed again), when it works and a second time (nothing
happens, d0 unchanged); ReqFileLoc (= init's buffer, cleared, a0);
ReqFlags; ReqOutput; all three forms of ReqFileRequest with ok and cancel
(d0, a3, the copied path – written by the test through ReqFileLoc – and
the FrStruct the requester sees: version, title, window, flags, the 0
pointers); ReqColour (d0, the structure); finit (library closed, memory
back). Not covered: the real req.library (not installed in the emulator,
and its requesters need a user); ReqFileRequest/ReqColour without the
library (they call through a6 = 0).
