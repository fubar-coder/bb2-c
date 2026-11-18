# riapplib (userlib 61): App windows, App icons, App menus

`libs/userlibs/rwe/riapplib.c`, the port of `rwelibsrc/AppLib/applib2.bb`
to C. Reference: `BSS2/REDBLITZLIBS/LIBRARIES/USERLIBS/RIAPPLIB.OBJ`,
which applib2 builds to with blitz2 2.10 ([README](../README.md) 3.1).
The declarations (header, token table, help texts) equal the reference
(ctest `rwe_declarations_riapplib`); the code is C, not byte-identical.

## What it does

It uses workbench.library V37 (and dos, icon) with one message port of
its own, opened by init:

* `AddAppWindow window#` / `DelAppWindow window#` – the window comes from
  windowslib (`!libs #windowslib,#ia0|#pd0`: a0 = the object, its first
  long the Intuition window), the window number from the compiler's
  stack (`#push`, `MOVE.l -(a2),d0`).
* `AddAppIcon id,text$,filename$[,x,y]` / `DelAppIcon id` – the icon from
  `GetDiskObject(filename$)`, without x/y at `NO_ICON_POSITION`.
* `AddAppMenu id,text$` / `DelAppMenu id` – Tools menu items.
* `AppEvent` replies to the previous message and gets the next one
  (`$800000` or 0); `AppEventType` (1 window, 2 icon, 3 menu, else 0),
  `AppEventID`, `AppNumFiles`, `NextAppFile`/`AppFile(n)` (the full path
  of an argument, a string result at a3), `AppLock` (the lock of the
  first argument), `AppMessage`, `AppMsgPort`; `AppEventIDCMP` is an
  inline routine (`Dc $a000`) that returns `$800000`.

init allocates one block of 768 bytes (MEMF_CLEAR): 16 longs each for
menus (+0), windows (+64) and icons (+128), a 512 byte path buffer (+192)
and the DiskObjects (+704). The ids index these tables unchecked. All
entries except `AppEventIDCMP` are Amiga mode only (odd code addresses).

## C and assembler

Everything is C (thunks keep d1/a0/a1 in every routine, init/finit all
registers – the original saves all of them there with MOVEM, elsewhere it
keeps less). a2 stays unchanged (DIFF, the general rule of
[cmdlibs-port.md](../../libs/cmdlibs-port.md)): the original's
AddAppWindow leaves it decremented, and AddAppMenu/AddAppWindow/AddAppIcon
set it to 0 for the tag list. Assembler remains for `AppEventIDCMP`, an
inline routine (`BLITZ_INLINE`), by nature a few instructions copied by
the compiler. `AddAppIconA` takes its DiskObject and tag list in a3 and
a4, fixed registers of the Blitz program: the libraries' safe form of the
NDK macro keeps them (`cmake/ndk-calls.cmake`).

Not ported: `replymsg` and `waitappevent` (dead code without a token in
the original).

## Fixed

Intended deviations from the original (`docs/libs/deviations.md`,
kind F):

* `NextAppFile`/`AppFile` leave a3 on the 0 of their result; the
  original copied the name with its 0 and left a3 behind it, so in a
  string expression the next part was written one byte too far
  (`NextAppFile+".bak"` held the 0 and lost the `k`) (#519).
* `AddAppIcon` keeps a4 and leaves a3 on `text$`, its first string
  argument (#516). The original loaded the DiskObject into a3 and the
  tag list (0) into a4 for `AddAppIconA` and restored neither: the locals
  base of the Blitz program was 0 after it (a local read after it was
  wrong, the `UNLK a4` at the end of the procedure crashed), and as a
  function in a string expression the rest was written into the
  DiskObject. a3 on `text$` gives back the space of the string arguments,
  which the compiler does not take back after a function with a numeric
  result (#846); the call itself uses the libraries' safe form of the NDK
  macro, which keeps a3 and a4 (`cmake/ndk-calls.cmake`).

* `AddAppMenu` returns -1 on success, as `AddAppWindow` and
  `AddAppIcon`; the original returned the AppMenuItem address (the
  `MOVEQ #-1,d0` behind the `RTS` was never reached), so a test with
  `=-1` failed although the item was added (#518).
* `AppLock` returns 0 for a message without arguments (an App menu item
  chosen without icons); the original read the first WBArg without
  looking at `am_NumArgs` and returned whatever `am_ArgList` pointed at
  (#522).
* `AppFile(n)` with n ≤ 0 is out of range and returns an empty string;
  the original checked only the upper bound and read the WBArg before
  the list (its lock passed to `NameFromLock`, perhaps unlocked and
  cleared) (#520).
* `DelAppWindow` returns 0 without a call for a window number that has
  no App window, as `DelAppMenu` and `DelAppIcon`; the original called
  `RemoveAppWindow(NULL)` and returned -1. `DelAppIcon` calls
  `FreeDiskObject` only for a DiskObject (#523).
* `NextAppFile`/`AppFile` leave the lock of the WBArg in the message. The
  original unlocked it – Workbench's own lock (`ChangeMode` + `UnLock`,
  "Let us Unlock() it!") – and cleared it unless the path ended with
  `:`, so a second `AppFile` of the same argument had lock 0 and
  `AppLock` returned 0; its `:` test read the byte before the string for
  an empty path, which now gets no `/` (#521).

## BUG(orig), kept

* finit (`delapp`) clears the slot at byte offset `index*4/4` instead of
  `index*4` (`ASR.l #2,d7` before the store) – a long store at an odd
  address for odd indexes (an address error on a 68000), harmless at
  program end on a 68020 (#524).
* `AddAppIcon` on a used id removes the old AppIcon and frees its
  DiskObject first; if `AddAppIconA` then fails, the removed AppIcon stays
  in the slot (a later `DelAppIcon` removes it again) and the new
  DiskObject stays in its slot (freed only by finit) (#517).
* finit replies to the messages still queued at the port only if a
  message is current (the last `AppEvent`); the flag `msgportflag` for the
  other case is never set. Otherwise they stay unreplied when the port is
  deleted (#525).
* init: if a library does not open, the port is deleted and the memory
  freed, but the libraries already opened stay open; `AppMsgPort` still
  returns the deleted port (#526).

## Test

`libs/tests/rwe/riapplib.c` (snitch "rwe riapplib"): each run loads one
library set (the C build or the CD original); every call is observed as
cases (d0, strings and a3, a4 after AddAppIcon, the replies to the App
messages, the kept registers), compared with the expected data
`libs/tests/expected/rwe_riapplib.txt` recorded from the original. workbench.library's
six App functions are replaced with `SetFunction` by stubs that log each
call (all argument registers, the DiskObject's position) and return a
fake object or 0 – so no Workbench is needed; dos and icon.library are
real (an icon written to `RAM:rweapp`, WBArgs with locks on `RAM:` and a
directory in it). Covered: every routine before init (libflag 0), every
command with success, replacement of a used id, failure of the
workbench call, a missing icon file, both forms of AddAppIcon, App
messages of all four types put to `AppMsgPort` (replies counted),
`NextAppFile`/`AppFile` on a root and a non-root lock (the unlock),
past the end, the inline routine's bytes, finit with and without a
current message (with menus, windows and icons left); a third run in
which `AddAppIconA` fails for a used id and for a free one (icon.library's
`FreeDiskObject` patched to count its calls, then `DelAppIcon` and
finit), and an init in which icon.library does not open (exec's
`OpenLibrary` patched for that call: `AppMsgPort` and the opens of
workbench and dos.library left behind).

Not covered: init without memory, a real Workbench, `AppFile(n)` with n ≤ 0 and `AppLock` without arguments
(they read foreign memory), the odd-address store on a 68000.
