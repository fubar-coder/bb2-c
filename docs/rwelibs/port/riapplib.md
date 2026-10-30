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
set it to 0 for the tag list. Assembler remains for:

* `AppEventIDCMP`: an inline routine (`BLITZ_INLINE`), by nature a few
  instructions copied by the compiler.
* the `AddAppIconA` call: its DiskObject and tag list go in a3 and a4,
  which are fixed registers of the Blitz program (`-ffixed-a3/a4`), so
  GCC cannot load them for a call; a small `__asm` sets them – and leaves
  them set, as the original does (BUG(orig) below, #516).

Not ported: `replymsg` and `waitappevent` (dead code without a token in
the original).

## BUG(orig), kept

* `AddAppIcon` loads the DiskObject into a3 and the tag list (0) into a4
  for `AddAppIconA` and restores neither: after it the string pointer is
  the DiskObject and the locals base of the Blitz program is 0 (#516).
* finit (`delapp`) clears the slot at byte offset `index*4/4` instead of
  `index*4` (`ASR.l #2,d7` before the store) – a long store at an odd
  address for odd indexes (an address error on a 68000), harmless at
  program end on a 68020 (#524).
* `AddAppMenu` returns the AppMenuItem address instead of -1 (the
  `MOVEQ #-1,d0` behind the `RTS` is never reached) (#518).
* `DelAppWindow` calls `RemoveAppWindow` for an empty slot too (NULL);
  `DelAppIcon` calls `FreeDiskObject` for an empty DiskObject slot
  (#523).
* `AddAppIcon` on a used id removes the old AppIcon and frees its
  DiskObject first; if `AddAppIconA` then fails, the removed AppIcon stays
  in the slot (a later `DelAppIcon` removes it again) and the new
  DiskObject stays in its slot (freed only by finit) (#517).
* `NextAppFile`/`AppFile` unlock the lock of the WBArg – Workbench's own
  lock (`ChangeMode` + `UnLock`, "Let us Unlock() it!") – and clear it in
  the message, unless the path ends with `:`. A second `AppFile` of the
  same argument then has lock 0. The `:` test reads the byte before the
  string if the path is empty (#521). a3 ends **behind** the 0 (d0 = the
  length is right) (#519). `AppFile(n)` with n ≤ 0 reads the WBArg
  before the list (#520).
* `AppLock` reads the first WBArg without looking at `am_NumArgs` (#522).
* finit replies to the messages still queued at the port only if a
  message is current (the last `AppEvent`); the flag `msgportflag` for the
  other case is never set. Otherwise they stay unreplied when the port is
  deleted (#525).
* init: if a library does not open, the port is deleted and the memory
  freed, but the libraries already opened stay open; `AppMsgPort` still
  returns the deleted port (#526).

## Test

`libs/tests/rwe/riapplib.c` (snitch "rwe riapplib"): the C build and the
CD original side by side, every call compared (d0, strings and a3, a4
after AddAppIcon, the registers the original keeps). workbench.library's
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
current message (with menus, windows and icons left).

Not covered: the failure paths of init (no memory, a missing library), a
real Workbench, `AppFile(n)` with n ≤ 0 and `AppLock` without arguments
(they read foreign memory), the odd-address store on a 68000.
