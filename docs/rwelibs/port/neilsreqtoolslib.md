# neilsreqtoolslib (54): the ReqTools interface

Neil O'Rourke ("ReqTools V38 for Blitz Basic II V1.70b (c) Neil O'Rourke
20/07/94"), with the changes of 1996/97 by Steve Matty (RWE): the
autoscroll offset of the screen mode requester, a3 in RTEZGetString,
RTReturnValue, RTEZGetLong's default value, the font name no longer
copied. 30 commands on reqtools.library V38:

* `RTEZRequest(title$,body,gadgets$[,pos[,default,flags]])`,
  `RTEZFlagsRequest(title$,body$,gadgets$,idcmp[,pos])`,
  `RTRequest(body$,gadgets$,taglist)` – rtEZRequestA; the position,
  default response and flags apply to the one call.
* `RTEZLoadFile(title$,file$)`, `RTEZSaveFile`, `RTFileRequest(...,
  taglist)`, `RTEZPathRequest(title$)` – rtFileRequestA on requesters of
  their own; the result is the directory, `/` unless it is empty or ends
  in `:`, and the file. `RTEZSetDefaultDirectory n,dir$` (0 load, 1 save,
  2 path, 3 multi-load), `RTEZSetPattern n,pat$` / `RTEZFreePattern n`
  (with/without the pattern gadget).
* `RTEZMultiLoadFile(title$)` – a multi-select file requester; its file
  list is read with `RTNextPathEntry` (directory + name per call).
* `RTEZFontRequest(title$)` – the address of a TextAttr;
  `RTEZScreenModeRequest(title$[,flags])` – the address of DisplayID.l,
  width.w, height.w, depth.w, overscan.w, autoscroll.l;
  `RTEZPaletteRequest(title$,first)`.
* `RTEZGetLong(title$,body$[,default])`, `RTEZGetLongRange(title$,
  body$,min,max[,default])`, `RTEZGetString(title$,body$,max[,default$])`.
* `RTLockWindow(win#)` / `RTUnlockWindow win#,lock`.
* `RTASyncRequest(title$,body$,gadgets$)`, `RTASyncPaletteRequest
  (title$,first)` – with RT_ReqHandler; `RTCheckASync…` (-1 while it is
  up) and `RTEndASync…`.
* `RTVersion`, `RTRevision`, `IsReqToolsActive`, `RTReturnValue` (the
  result of the last reqtools call that stores it).

init opens reqtools.library V38 and allocates seven requesters (five file
requesters, a font and a screen mode requester); IsReqToolsActive is -1
when that worked. finit frees them and the file list of the last
multi-load and closes the library. No routine checks IsReqToolsActive.

* **Source:** `rwelibsrc/RTLib.bb` (1997-01-30; as TED shows it:
  `build/amiga/rwelibsrc/RTLib.bb`). The CD's `NEILSREQTOOLSLIB.OBJ`
  (BUM9B/BUM10A/EXTRAS, all identical) is V1.70b of 1994 without any of
  Steve Matty's changes and without RTReturnValue; AmiBlitz3's
  `NeilsReqToolsLib.obj` is a separate patch of that binary
  (`docs/rwelibs/README.md` 3.2).
* **Reference:** the reference build of the source by blitz2 2.10
  (`refbuild`, ctest `rwe_refbuild`:
  `build/amiga/libs/userlibs/rwe/ref/neilsreqtoolslib.obj`); declarations
  equal (ctest `rwe_declarations_neilsreqtoolslib`).
* **Library number:** 54 = `#neilsreqtoolslib` of the CD's libnums
  (`libs/variants/bb2-2.10/libnums.h`); no library of the 2.10 build has
  it.
* **C file:** `libs/userlibs/rwe/neilsreqtoolslib.c`; test
  `libs/tests/rwe/neilsreqtoolslib.c` (snitch case "rwe
  neilsreqtoolslib").

## C and assembler

All routines are C behind `BLITZ_THUNK`. reqtools calls with arguments in
d0/a0/a1 only go through `BLITZ_BASEJSR`; the ones that take a2-a4
(rtEZRequestA: a3 = rtReqInfo, a4 = the argument array; rtFileRequestA
and rtFontRequestA: a3 = the title; rtGetStringA, rtGetLongA,
rtPaletteRequestA: a3 = 0) through the macro `RTJSR`: a3 and a4 are the
fixed registers of the Blitz program (`-ffixed-a3/a4`), so a few lines of
inline assembler save them, load them for the call and restore them,
like the original's `MOVEM a3-a6`. maxslib (`allocst`/`freest`, the 108
byte copy of FileName$) and memlib (`globalloc`/`globfree`, the buffer of
RTEZGetString) through `BLITZ_ALIBJSR`. The tag lists are static arrays
with the original's contents and are changed in place like the original's
(`freqFlags+4` …); init stores a requester only when it was allocated
(the variable keeps its old value otherwise), as the original. String
results are built from the title argument's address, like the original
(`MOVE.l d0,a3`); a3 ends where the original leaves it (see BUG(orig), #461 #465).
The thunks keep d1/a0/a1 (finit also d0), the C code d2-d7/a2-a6; the
original changes d0-d2, a0-a2 and what reqtools changes.

**DIFF:** a2 stays unchanged (the original loads reqtools arguments
there; `docs/libs/cmdlibs-port.md` "General").

## BUG(orig)

* RTEZScreenModeRequest calls `rtFontRequestA` (-96) with the screen mode
  requester instead of `rtScreenModeRequestA` (-144) (#466).
* RTEZFlagsRequest stores its title in RTEZRequest's tag list (where the
  title tag is off and the next RTEZRequest resets it): its own list has
  `RTEZ_ReqTitle, 0`, the requester always gets reqtools' default title
  (#455).
* RTEZFreePattern 2 resets the multi-load requester's flags; the
  multi-load is 3 in RTEZSetPattern and RTEZSetDefaultDirectory (2 is the
  path requester there, RTEZSetPattern 2 does nothing) (#456).
* RTEZMultiLoadFile tests the file list with `CMP.b #0,d0`: a list at an
  address ending in `$00` is returned as its address instead of -1
  (#462).
* RTEZGetLong returns the value also on cancel (`TST.l d0` without a
  branch); the value (and so the default) stays for the next call
  (#457).
* RTEZGetString does not free its buffer on cancel (memlib frees it at
  the end of the program) (#458); the default string is copied into the
  buffer of MaxChars bytes without a limit (#459).
* RTEZSaveFile builds its result at the title but restores a3 (`MOVEM
  a3-a6`) (#465), RTEZLoadFile and RTFileRequest leave a3 behind the 0
  (on cancel at the title) (#461).
* RTASyncRequest/RTASyncPaletteRequest read the "user port" at offset 86
  of the window object (not of the Window it points to) and keep its
  signal bit shifted by one (`LSL.b #1`) in a byte nobody reads; the
  handler is always called with no signals (`sigs` is never set) (#454).
  RTEZPaletteRequest and RTASyncPaletteRequest do not set RTReturnValue
  (#464).
* init: when a requester cannot be allocated, the library and the
  requesters allocated so far stay (finit does nothing then). finit
  clears nothing: a second finit frees again (#460). Without
  reqtools.library every requester calls through a6 = 0
  (RTVersion/RTRevision read the addresses `$14`/`$16`) (#463).

Original behaviour that is not a defect: RTEZGetString's result and
RTEZPathRequest's path have no 0 behind them (the usual convention of a
string result, which has its length); FileName$ is copied with a length
of 108 whatever its own length is (reqtools' file name buffer size).

## Test

Each run loads one library set (the C build or the original); the
scenario writes its observations as cases, compared with the expected
data `libs/tests/expected/rwe_neilsreqtoolslib.txt` recorded from the
original.

reqtools.library 39.3 is in the emulator's system, but its requesters
need a user. So the test puts a fake reqtools.library before it in the
library list (priority 10). Each fake function logs its registers, the
tag list (pairs up to TAG_DONE) and the strings and buffers it gets –
pointers into the library, into the test's requesters and into the
string area made independent of the layout – and answers with a
chosen result: the file requester writes the chosen directory into the
requester and the chosen name into the buffer, rtGetStringA/rtGetLongA
the chosen value, rtFontRequestA the fields of the font and screen mode
requester, the multi-load a file list, an RT_ReqHandler tag gets a
handler. Observed after every call: d0, a3, the string area, the kept
registers, the log of reqtools calls and RTReturnValue.

Cases: the routines that need no library before init; init when
OpenLibrary fails and when each of the seven rtAllocRequestA fails (the
library stays open, finit does nothing), then ok; RTVersion/RTRevision
(d0's high word); RTEZRequest all forms with and without title (the
defaults restored); RTEZFlagsRequest both forms with and without gadgets;
RTRequest; RTEZLoadFile/RTEZSaveFile/RTFileRequest/RTEZPathRequest with
four directories (empty, `RAM:`, `RAM:t`, `Work:a/b`) and cancel;
RTEZSetDefaultDirectory and RTEZSetPattern 0-4, RTEZFreePattern 0-3 and
the flags the next requesters get; RTEZMultiLoadFile (a list of three, a
list at an address ending in `$00`, cancel) with RTNextPathEntry to the
end; RTEZFontRequest, RTEZScreenModeRequest both forms, ok and cancel
(the TextAttr / screen mode block); RTEZPaletteRequest; RTEZGetLong,
RTEZGetLongRange, RTEZGetString in all forms, ok and cancel;
RTLockWindow/RTUnlockWindow; both asynchronous requesters up and not up
with their check and end; finit (seven rtFreeRequest, rtFreeFileList,
CloseLibrary). Then with the real reqtools.library: init, IsReqToolsActive,
RTVersion/RTRevision, RTEZSetDefaultDirectory/RTEZSetPattern 0-4, finit.
Besides these: the result string of one file requester, the font
requester call of RTEZScreenModeRequest, the missing title of
RTEZFlagsRequest, RTEZGetLong's value on cancel, the multi-load's
address result.

Not covered: the real requesters (they need a user); the routines
without reqtools.library (they call through a6 = 0); a second finit.
