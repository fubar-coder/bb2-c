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
  (#786). With the real req.library this takes effect only together with
  #1091 (the next point).
* The colour requester structure is req.library's
  `ExtendedColorRequester` (the CD's `REQBASE.BB2`): DefColour a long at
  0, the window at 4, 5 longs RFU, 28 bytes. The original's (`ecr`) had
  DefColour as a word at 0 and the window at 2 (6 longs RFU): the
  requester read the colour in the high word, with the window pointer's
  high word below it, as DefColour, and a window pointer made of the
  window's low word and 0. Measured with req.library 2.8: ReqColour(3)
  answered with OK returned 0 (#1091).
* The default flags are caching, info gadget and cache purge ($94, the
  masks RIReqLib.guide lists). The original's
  `#frq_Flags = #FRQCACHINGB|#FRQINFOGADGETB|#FRQCACHEPURGEB` ORed bit
  numbers (2|4|7 = 7): bits 0-2, extended select and caching (#787).
* Without req.library (not installed, or init's AllocMem failed, which
  closes it again) ReqFileRequest returns "" as on cancel (with a title:
  a3 back at its start) and ReqColour 65535 ($FFFF, a word), what
  req.library's ColorRequester returns on cancel (measured with
  req.library 2.8), so a program sees the same as when the user cancels
  (user decision of 2026-11-18, replacing W3 Q5); the original checked
  no base and called through a6 = 0, a crash (#788). ReqFileLoc returns
  0 then, as before.
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
* The title of `ReqFileRequest(title$[,flags])` is for that call only: a
  later call without a title has the library's empty title buffer again
  (user decision W3 Q4). The original kept the pointer to the title
  string, which it gives up to the string work area, so a later call
  without a title showed whatever had been built there since (#789).
* init clears `FrStruct`'s window together with the colour requester's
  (`routput`). The original cleared only `routput`; `FrStruct` is
  library data (`#frq_Window=0` only its initial value), so a second run
  of the program with the library still loaded (from the editor) gave the
  file requester the first run's window, closed at its end, until its
  own ReqOutput, while the colour requester got none (#1090).
* finit calls PurgeFiles (req.library -114) for `FrStruct` before it
  closes the library, only when the library is open. The original only
  closed it, so the memory the requester keeps in the structure between
  calls (the cached directory and its buffers) stayed allocated after
  the program: as much as the last directory shown needs (measured with
  req.library 2.8 and AvailMem: 320 bytes for RAM:, 1560 for SYS:C; the
  requester reuses the memory, so it does not grow with the number of
  calls, and PurgeFiles frees all of it) (#1092). What stays is
  req.library's own: about 40 KB (a 40960-byte chip block among them)
  allocated by its first FileRequester after the library is loaded,
  not freed when it is expunged; not rireqlib's to fix (user decision of
  2026-11-18).

## Kept

* `ReqFileRequest(title$,flags)` sets the flags for later calls too, like
  ReqFlags: RIReqLib.guide documents it ("If this is omitted then the
  last flag setting is used"), not a defect (#789).

## Test

Each run loads one library set (the C build or the original); the
scenario writes its observations as cases, compared with the expected
data `libs/tests/expected/rwe_rireqlib.txt` recorded from the original.

The requesters need a user, so the test adds a fake `req.library` to the
library list; its FileRequester and ColorRequester record the structure
they are given (162 bytes; of the colour requester's the 28 bytes
req.library reads, `ExtendedColorRequester` of the CD's `REQBASE.BB2`:
DefColour a long at 0, the window at 4) and return a chosen value; a confirmed
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
memory back, the PurgeFiles calls with `FrStruct` while the library is
open: the fake's PurgeFiles, -114, counts them); a second run as from
the editor (init again after finit, no ReqOutput: the window the file
requester and the colour requester get, then finit). The Workbench
system of the emulator has no req.library; the real req.library 2.8
(Aminet ReqLib28.lha) is staged by the tests that use it, put into
LIBS: through inject `libs=`, and its requesters are answered through
inject `click=` (the bug-orig tests `rireqlib-title-kept`,
`rireqlib-colour-default-word`, `rireqlib-colour-window` and the
regression tests `rireqlib-real-cancel`, `rireqlib-real-needs-libs`;
skipped when the file is missing). Against the real library are checked:
the title of a second call (#789), the colour passed to the colour
requester and the window it gets (#786, #1091), what a cancelled
requester returns (#788), and the layout of the structures behind
offset 162 and the cached directory's size (measured by a C program,
not kept as a test: #1088, #1092). A confirmed file requester, the
buffers in the structure (#785), the writes behind offset 162 (#1088),
the second run from the editor (#1090) and the PurgeFiles call in finit
(#1092) are only checked with the fake library. Not covered by the
fake library: ReqFileRequest/ReqColour
without the library (the original calls through a6 = 0: the bug-orig
test `rireqlib-no-base-check`).
