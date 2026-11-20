# popupwinlib (16): popup menus in borderless windows

Reflective Images, 1994. Two commands:

* `PopUpInit`: the message port for the windows, if there is none (init
  makes it).
* `PopUpWindow screen#,*items` (a statement and a function): the menu
  `items` at 0,0 of the screen of screenslib's object; returns when a
  mouse button is pressed, with the address of the item chosen (0: none).

The number: none is known; ours is 16 (RWE's `RESERVEDLIB_10`;
AmiBlitz3's port took 24, RWE's `#richunkylib`).

* **Source:** `rwelibsrc/popuplib/popupwinlib.bb` (readable:
  `build/amiga/rwelibsrc/popuplib/popupwinlib.bb`), with `#GADTOOLS = 0`
  as the source builds. Its `.bak` is the same with a debug `ILLEGAL`.
* **Reference:** the reference build of the source (`REFERENCE
  refbuild`, stage D: no binary exists). The declarations equal it but
  for PopUpWindow's kind (#471,
  `popupwinlib.declarations-intended.txt`; ctest
  `rwe_declarations_popupwinlib`).
* **C file:** `libs/userlibs/rwe/popupwinlib.c`; test
  `libs/tests/rwe/popupwinlib.c` (snitch case "rwe popupwinlib").

## What it does

* **An item:** +0 next, +4 the text (0: a separator line), +12 the
  first item of its sub menu (0: none). +8, +10, +16 and +20 are named in
  the source but not used.
* **A menu level:** a 64 byte structure (in AllocVec's layout, cleared)
  and a window:
  `WFLG_BORDERLESS|REPORTMOUSE|ACTIVATE`, IDCMP MOUSEBUTTONS,
  INACTIVEWINDOW, MOUSEMOVE and CLOSEWINDOW; all windows share the
  library's message port.
* **Size:** the widest text + 4 (+ 8 with sub menus); each item is the
  screen font's height + 6 lines high, a separator 4. The window gets
  a bevelled frame, the separators a bevelled line.
* **The mouse:** a move over an item highlights it (pen 3 behind the
  text); an item with a sub menu opens it at once, to the right of the
  item. Moving back to the parent menu closes the sub menu (unless the
  mouse is on the parent's active item); moving to a level further up
  closes all levels in between; moving out of all windows clears the
  highlight. A mouse button closes all levels. The position is taken
  from the windows' MouseX/MouseY, not from the messages.

init opens intuition.library and graphics.library (any version) and
builds the port by hand; finit replies what is still in it, frees it
and closes the libraries. From intuition V36 on the windows open with
OpenWindowTagList and the tag list, below it (Kickstart 1.x) with
OpenWindow and a NewWindow of the same fields.

## C and assembler

All C behind thunks; no assembler. The menu levels are a recursive C
function (DoPop) with the structure instead of a5. The tag list of
OpenWindow is static and filled for every window, as in the original.

Not ported: the routines of the source that nothing calls (WPrint,
WinInnerCls, BitMaptoWin, WSizeLimits, WinGadgetHit) and the GadTools
variant of WaitWinEvent (`#GADTOOLS = 0`).

## Fixed

Intended deviations from the original (`docs/libs/deviations.md`,
kind F):

* Sizing a menu with sub menus leaves bit 1 of CIA-A PRA (the power LED
  / audio filter) as it is; the original toggled it with a
  `BCHG #1,$bfe001` left in (#468).
* A menu level whose structure cannot be allocated (AllocVec) returns 0
  at once, which closes all levels, as a failed OpenWindow does; the
  original went on with the structure at address 0 (#467).
* PopUpInit creates the port only when there is none (init makes it);
  the original created a new one each time without deleting the old
  one, a port and a signal bit per call (#470).
* Closing a menu window removes and replies only the messages of that
  window from the shared port; the original replied every message, also
  those already queued for the other levels: a mouse button pressed
  while a sub menu closed was lost (#469).
* PopUpWindow is a statement and a function: it returns the address of
  the item chosen (the item highlighted when a mouse button was
  pressed), 0 for none. The original was a statement only, and its d0
  was `selecteditem`, which nothing but a clear wrote: always 0 (#471).
* The library runs on Kickstart 1.x. The port is built by hand
  (blitzexec.h's `bl_newport`: AllocMem, AllocSignal, FindTask, an empty
  message list) and the structure of a menu level is allocated in
  AllocVec's layout (`bl_allocvec`: AllocMem of 68 bytes, the size in
  the first long), with exec V33 functions, one path on every
  Kickstart. The window opens with OpenWindowTagList from intuition V36
  on, as before, and below it with OpenWindow and a NewWindow of the
  same fields: no title, no gadgets, no IDCMP (the shared port is set
  afterwards, as on V36), the screen's pens, the size limits;
  WA_NewLookMenus has no field and no effect here (the windows have no
  menu strip). The original called CreateMsgPort and DeleteMsgPort,
  AllocVec and FreeVec (exec V36) and OpenWindowTagList (intuition V36)
  without a version check: every program with the library crashed at
  start-up on Kickstart 1.x (#1140). On Kickstart 1.x a menu level whose window does not fit the screen
  (a sub menu at the right or bottom edge) does not open: OpenWindow
  fails, the level returns 0 and the whole menu closes, as with a failed
  OpenWindow on V36+. The library does not move such a window onto the
  screen (left as it is, user decision).

Not a defect: the item count at +8 of the structure is the number of
items − 1, but nothing uses the field.

## Test

Each run loads one library set (the C build or the original); the
scenario writes its observations as cases, compared with the expected
data `libs/tests/expected/rwe_popupwinlib.txt` recorded from the
original.

PopUpWindow waits for the mouse, so the test drives it. While a popup
runs, exec's WaitPort is patched (SetFunction). When the library waits
on its port, the patch:

* takes a snapshot of the popup windows;
* sets their MouseX/MouseY to the next position of a script;
* puts the script's IntuiMessage (MOUSEMOVE, MOUSEBUTTONS,
  INACTIVEWINDOW) into the port, for the window of the level of its
  position (the newest window for an absolute position), and the
  entries marked to come with it: several messages queued at one wait.

The windows open on the Workbench screen at 0,0 (the screen object is
the Workbench screen); the library closes them all before PopUpWindow
returns.

Observed:

* per snapshot: the popup windows (position, size, IDCMP), their
  contents (copied with ClipBlit, hashed), bit 1 of CIA-A PRA relative
  to the start;
* the number of waits and of menu levels opened, d0 (the number of the
  item chosen; 0 in the original, #471), the kept registers;
* no memory lost after the first popup (a check of the program's own,
  not a case). The first windows leave memory in the system's caches
  (layers' ClipRects …), so the memory is counted from there.

Covered:

* init, finit;
* PopUpWindow with three levels: highlighting items, a separator, the
  mouse outside all windows, an INACTIVEWINDOW message, a sub menu
  opened by moving onto its item, back to the parent (the sub menu
  closes), onto the parent's active item (the sub menu stays), the third
  level left for the first (two levels closed), a mouse button with an
  active item;
* a mouse button for the parent queued behind the move that closes a
  sub menu (the original replies it when it closes the sub menu and
  waits once more, #469);
* a mouse button without an active item;
* a menu without sub menus that starts with a separator;
* PopUpInit (the original makes a new port and leaves the old one, which
  the test deletes; the C build keeps the port of init).

PopUpWindow as a function (driven by a vertical blank interrupt of the
program): the bug-orig test `popupwinlib-result-lost`. A failed
AllocVec: the bug-orig test `popupwinlib-allocvec-unchecked`
(through inject). Kickstart 1.x (the library tests do not run there):
the bug-orig tests `popupwinlib-v36-calls` (start-up, PopUpInit, finit)
and `popupwinlib-v36-menu` (the menu of `popupwinlib-result-lost` as a
statement, with a reply port built by hand: the window opens through
OpenWindow, both messages reach it, the button closes the menu), each on
the a500, a500-204 and a1200. Not covered: a failed OpenWindow, a level
that does not fit the screen on Kickstart 1.x, the result of the
function form on Kickstart 1.x; real mouse input.
