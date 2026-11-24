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
multi-load and closes the library. Without IsReqToolsActive every
command returns what it returns on cancel and calls nothing (#463).

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
(rtEZRequestA: a3 = rtReqInfo, a4 = the argument array; rtFileRequestA,
rtFontRequestA and rtScreenModeRequestA: a3 = the title; rtGetStringA, rtGetLongA,
rtPaletteRequestA: a3 = 0) through the macro `RTJSR`: a3 and a4 are the
fixed registers of the Blitz program (`-ffixed-a3/a4`), so a few lines of
inline assembler save them, load them for the call and restore them,
like the original's `MOVEM a3-a6`. maxslib (`allocst`/`freest`, the 108
byte copy of FileName$) and memlib (`globalloc`/`globfree`, the buffer of
RTEZGetString) through `BLITZ_ALIBJSR`. The tag lists are static arrays
with the original's contents and are changed in place like the original's
(`freqFlags+4` …); init stores a requester only when it was allocated,
as the original, and on a failure frees the ones it has (#460). String
results are built from the title argument's address, like the original
(`MOVE.l d0,a3`); a3 ends on the result's 0 (fixed, #461 #465).
The thunks keep d1/a0/a1 (finit also d0), the C code d2-d7/a2-a6; the
original changes d0-d2, a0-a2 and what reqtools changes.

**DIFF:** a2 stays unchanged (the original loads reqtools arguments
there; `docs/libs/cmdlibs-port.md` "General").

## Fixed

Intended deviations from the original (`docs/libs/deviations.md`,
kind F):

* RTEZLoadFile and RTFileRequest leave a3 on the 0 of their result (on
  cancel at the title, as before); the original wrote the 0 with
  `MOVE.b #0,(a3)+` and left a3 behind it, so in a string expression
  the next part was written one byte too far (#461).
* RTEZSaveFile leaves a3 on the 0 of its result, as RTEZLoadFile does;
  the original built the result at the title but restored a3 (`MOVEM
  a3-a6`), so the next part of a string expression was written at the
  old a3, behind the two arguments (over the end of a longer result, or
  behind the bytes of a shorter one) (#465).
* RTEZMultiLoadFile tests the whole file list address: -1 when files
  were chosen, 0 on cancel; the original tested it with `CMP.b #0,d0`
  and returned a list at an address ending in `$00` as its address
  instead of -1 (#462).
* RTEZPaletteRequest and RTASyncPaletteRequest store rtPaletteRequestA's
  result for RTReturnValue (RTASyncPaletteRequest unconverted, as
  RTASyncRequest); the original did not, so RTReturnValue still gave
  the previous call's result (#464).
* RTEZFlagsRequest puts its title in its own tag list, an empty title as
  0 (reqtools' default title, as RTEZRequest without a title); the
  original stored it in RTEZRequest's list (where the title tag is off
  and the next RTEZRequest resets it), so its requester always had the
  default title (#455).
* RTEZFreePattern 3 frees the multi-load requester's pattern, as
  RTEZSetPattern 3 set it; 2 does nothing (the path requester has no
  pattern). The original reset the multi-load requester for 2 and did
  nothing for 3 (#456).
* RTEZGetString frees its buffer also on cancel; the original freed it
  only after a choice, so every cancel kept MaxChars bytes until the end
  of the program (#458).
* init: when a requester cannot be allocated, init frees the requesters
  allocated so far and closes the library (IsReqToolsActive 0, d0 0);
  finit clears what it freed. The original kept the library open and the
  requesters allocated (finit did nothing then, as IsReqToolsActive was
  0), and finit cleared nothing (#460).
* Without IsReqToolsActive (no reqtools.library V38, or a failed init)
  every command calls nothing and returns what it returns on cancel:
  RTEZRequest, RTRequest, RTEZFlagsRequest, the font and screen mode
  requesters, RTEZMultiLoadFile 0; the file and path requesters and
  RTEZGetString an empty string (a3 as on cancel); RTEZPaletteRequest
  -1; RTEZGetLong what it gives on cancel, RTEZGetLongRange 0; RTASyncRequest and RTASyncPaletteRequest 0 (not
  opened), their check and end 0; RTLockWindow 0; RTVersion and
  RTRevision 0 (d0's high word as before); RTEZSetDefaultDirectory and
  RTEZSetPattern change no requester. The original called through a6 = 0
  (a crash; RTVersion/RTRevision read the addresses `$14`/`$16`) (#463).
* RTEZScreenModeRequest calls `rtScreenModeRequestA` (-144); the original
  called `rtFontRequestA` (-96) with the screen mode requester. The
  screen mode requester appeared all the same: in reqtools.library 38
  the vectors -54, -96 and -144 lead to one routine, which takes the
  kind of requester from the long in front of the requester
  (`RT_SCREENMODEREQ`, set by rtAllocRequestA in init); the documented
  call does not depend on that (#466).
* RTEZGetLong sets its value to 0 after each call: without a default the
  requester starts at 0 and cancel returns 0, with a default cancel
  returns the default (as before; RTReturnValue tells cancel from OK).
  The original kept the value (`glValue`) for the next call, so a call
  without a default showed the previous call's number and returned it on
  cancel (#457).
* RTEZGetLongRange sets its value to 0 after each call in the same way:
  without a default the requester starts at 0, with a default at the
  default; cancel returns 0 (as before). reqtools shows the start value
  as it is, also when 0 is outside Min..Max, and checks the range only
  on OK (a value outside flashes a message in the title, the requester
  stays open).
  The original kept the value (`glrValue`), so a call without a default
  showed the previous call's number (#1078).
* RTASyncRequest/RTASyncPaletteRequest no longer read the window object;
  RTCheckASync… and RTEndASync… pass the handler's WaitMask (the long at
  +4 of reqtools' handler info; 0 before any asynchronous requester) as
  the signals, those of the requester's window, which a program that
  polls has to assume arrived. The original read a "user port" at offset
  86 of the window object (not of the Window it points to), kept its
  signal bit shifted by one (`LSL.b #1`) in a byte nobody read, and
  called the handler with no signals. reqtools' EZ and palette handlers
  ignore the signals, so with reqtools 38 nothing visible changes (#454).

## Kept on purpose (not fixed, decision)

* BUG(orig) (#459): RTEZGetString copies the default string into the buffer of MaxChars
  bytes without a limit.

Original behaviour that is not a defect: RTEZGetString's result and
RTEZPathRequest's path have no 0 behind them (the usual convention of a
string result, which has its length); FileName$ is copied with a length
of 108 whatever its own length is (reqtools' file name buffer size).

## Test

Each run loads one library set (the C build or the original); the
scenario writes its observations as cases, compared with the expected
data `libs/tests/expected/rwe_neilsreqtoolslib.txt` recorded from the
original.

The requesters need a user, and the emulator's system has no
reqtools.library (none of the three hardfiles has it in `Libs`). So the
test puts a fake reqtools.library in the library list (priority 10:
before a real one, if there is one). Each fake function logs its registers, the
tag list (pairs up to TAG_DONE) and the strings and buffers it gets –
pointers into the library, into the test's requesters and into the
string area made independent of the layout – and answers with a
chosen result: the file requester writes the chosen directory into the
requester and the chosen name into the buffer, rtGetStringA/rtGetLongA
the chosen value (only on OK, as reqtools), rtFontRequestA/rtScreenModeRequestA the fields of the
font and screen mode requester, the multi-load a file list, an RT_ReqHandler tag gets a
handler info (with a WaitMask). Observed after every call: d0, a3, the string area, the kept
registers, the log of reqtools calls and RTReturnValue.

Cases: the routines that need no library before init; init when
OpenLibrary fails and when each of the seven rtAllocRequestA fails (the
fixed init frees the requesters it has and closes the library, the
original keeps them; finit does nothing), then ok; RTVersion/RTRevision
(d0's high word); RTEZRequest all forms with and without title (the
defaults restored); RTEZFlagsRequest both forms with and without gadgets
and with an empty title after them;
RTRequest; RTEZLoadFile/RTEZSaveFile/RTFileRequest/RTEZPathRequest with
four directories (empty, `RAM:`, `RAM:t`, `Work:a/b`) and cancel;
RTEZSetDefaultDirectory and RTEZSetPattern 0-4, RTEZFreePattern 0-3 and
the flags the next requesters get; RTEZMultiLoadFile (a list of three, a
list at an address ending in `$00`, cancel) with RTNextPathEntry to the
end, then after RTEZFreePattern 3 and, with a new pattern, 2; RTEZFontRequest, RTEZScreenModeRequest both forms, ok and cancel
(the TextAttr / screen mode block); RTEZPaletteRequest; RTEZGetLong,
RTEZGetLongRange, RTEZGetString in all forms, ok and cancel (and the
memory each RTEZGetString keeps);
RTLockWindow/RTUnlockWindow; both asynchronous requesters up and not up
with their check and end, the palette one also after RTEZRequest (for
RTReturnValue); finit (seven rtFreeRequest, rtFreeFileList,
CloseLibrary) and the fake's open count left by the failed inits. Then, only on a system with a real reqtools.library V38
(not the emulator's: there the run prints `no reqtools.library V38 - the
real one not tested`): init, IsReqToolsActive, RTVersion/RTRevision,
RTEZSetDefaultDirectory/RTEZSetPattern 0-4, finit.
Besides these: the result string of one file requester, the reqtools
function RTEZScreenModeRequest calls, the missing title of
RTEZFlagsRequest, RTEZGetLong's value on cancel, the multi-load's
address result.

After finit, in the C build only, every command with the library
closed: its cancel value and no reqtools call (#463; cases `[fixed #463]`
alone). The bug-orig test `neilsreqtoolslib-no-library-check` runs a
program with the open of reqtools.library failing (inject).

Not covered: the real requesters (they need a user); the routines
without reqtools.library in the original (they call through a6 = 0); a
second finit.
