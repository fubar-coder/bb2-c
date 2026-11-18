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
object in a0 (`!libs {#windowslib,#ia0|#pd0}` = `BL_IA0|BL_PD0`).
The structures `FrStruct` (req.library's 356 bytes, `frq_SIZEOF`) and
the colour requester's are packed C structs with the original's initial
values. The
title length is read from -4(a2) without changing a2 (DIFF as in all C
ports, `docs/libs/cmdlibs-port.md` "General"). init returns d0 unchanged
when the library is already open, like the original. The thunks keep
d1/a0/a1 where the original keeps them, ReqFileLoc returns the buffer in
a0 too.

## Fixed

Intended deviations from the original (`docs/libs/deviations.md`,
kind F):

* ReqFileRequest leaves a3 on the terminating 0 of its result; the
  original left it behind the 0, so in a string expression the next part
  was written one byte too far (see `docs/libs/cmdlibs-port.md`, "String
  results"). On cancel d0 = 0 and a3 stays (with a title: at its start),
  as before (#784).
* ReqColour passes its argument to the colour requester as DefColour;
  the original ignored it, so the requester always started on colour 0
  (#786).
* The default flags are caching, info gadget and cache purge ($94, the
  masks RIReqLib.guide lists). The original's
  `#frq_Flags = #FRQCACHINGB|#FRQINFOGADGETB|#FRQCACHEPURGEB` ORed bit
  numbers (2|4|7 = 7): bits 0-2, extended select and caching (#787).
* Without req.library (not installed, or init's AllocMem failed, which
  closes it again) ReqFileRequest returns "" as on cancel (with a title:
  a3 back at its start) and ReqColour the colour it is given; the
  original checked no base and called through a6 = 0, a crash (#788).
  ReqFileLoc returns 0 then, as before.
* init puts the four buffers into `FrStruct`'s title, directory, file and
  path fields, and finit clears them, as the first version `ReqLib.bb`
  had them (its fields pointed to its static buffers; AmiBlitz3's
  `riReqLib.ab3` is that version). The original stored the addresses
  only in its own longs `frq_Title`…`frq_PathName` behind the structure:
  the requester got 0 for them, and ReqFileRequest returned the path
  buffer that only the program (through ReqFileLoc) wrote, never the
  user's choice (#785). Before the first call with a title, the title
  field points to the library's title buffer, which is empty.
* `FrStruct` is req.library's whole file requester structure, 356 bytes
  (`frq_SIZEOF`: the field list of RIReqLib.guide, "STRUCTURE
  AFileRequester", with `#DSIZE=130`, `#FCHARS=30`, `#WILDLENGTH=30` of
  the CD's `REQBASE.BB2`). The original's ended at 162 (34 bytes, then
  32 longs), so the fields the requester keeps there from call to call
  (`frq_Show` from 152, the cursor positions at 184, `frq_Memory`,
  `frq_Memory2`, `frq_Lock` at 200-208, its directory buffer at 212)
  were written over the library's data behind it: the four buffer
  pointers, the colour requester structure and, at 208, the library base
  (#1088). Version 1 had its 256-byte title buffer there.

## BUG(orig)

* ReqFileRequest(title$,flags) changes the flags for good (like
  ReqFlags), and a title stays for later calls without one (#789).

## Test

Each run loads one library set (the C build or the original); the
scenario writes its observations as cases, compared with the expected
data `libs/tests/expected/rwe_rireqlib.txt` recorded from the original.

The requesters need a user, so the test adds a fake `req.library` to the
library list; its FileRequester and ColorRequester record the structure
they are given (162 / 30 bytes) and return a chosen value; a confirmed
FileRequester writes `FR:` and the path into the structure's path buffer
when it has one, as req.library does. Observed:
init when OpenLibrary fails, when AllocMem fails (the
library is closed again), when it works and a second time (nothing
happens, d0 unchanged); ReqFileLoc (= init's buffer, cleared, a0);
ReqFlags; ReqOutput; all three forms of ReqFileRequest with ok and cancel
(d0, a3, the result – the test writes another path through ReqFileLoc –
and the fields of the FrStruct the requester sees: version, the four
buffers relative to init's path buffer, window, flags, the rest, and the
title text); ReqColour (d0, the structure); the requester's own fields
behind offset 162: the fake writes offsets 152-207 once (frq_Show, the
cursor positions, frq_Memory, frq_Memory2; not frq_Lock at 208, where
both builds of the 162-byte structure have the library base), then
ReqFileLoc and the colour structure show whether the library's data is
intact, and the test writes the old bytes back; finit (library closed,
memory back). Not covered: the real req.library (not installed in the
emulator, and its requesters need a user); ReqFileRequest/ReqColour
without the library (the original calls through a6 = 0: the bug-orig
test `rireqlib-no-base-check`).
