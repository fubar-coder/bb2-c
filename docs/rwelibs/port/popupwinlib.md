# popupwinlib (16): popup menus in borderless windows

Reflective Images, 1994. Two commands:

* `PopUpInit`: the message port for the windows, if there is none (init
  makes it).
* `PopUpWindow screen#,*items`: the menu `items` at 0,0 of the screen of
  screenslib's object; returns when a mouse button is pressed.

The number: none is known; ours is 16 (RWE's `RESERVEDLIB_10`;
AmiBlitz3's port took 24, RWE's `#richunkylib`).

* **Source:** `rwelibsrc/popuplib/popupwinlib.bb` (readable:
  `build/amiga/rwelibsrc/popuplib/popupwinlib.bb`), with `#GADTOOLS = 0`
  as the source builds. Its `.bak` is the same with a debug `ILLEGAL`.
* **Reference:** the reference build of the source (`REFERENCE
  refbuild`, stage D: no binary exists). The declarations equal it
  (ctest `rwe_declarations_popupwinlib`).
* **C file:** `libs/userlibs/rwe/popupwinlib.c`; test
  `libs/tests/rwe/popupwinlib.c` (snitch case "rwe popupwinlib").

## What it does

* **An item:** +0 next, +4 the text (0: a separator line), +12 the
  first item of its sub menu (0: none). +8, +10, +16 and +20 are named in
  the source but not used.
* **A menu level:** a 64 byte structure (AllocVec, cleared) and a window:
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

init opens intuition.library and graphics.library (any version, not
checked) and creates the port; finit replies what is still in it,
deletes it and closes the libraries.

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

## BUG(orig), kept

* **PopUpWindow always returns 0.** The menu levels return the chosen
  item, but the result is `selecteditem`, which is only cleared. (It is
  a statement anyway.) (#471)

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
* the number of waits and of menu levels opened, d0, the kept
  registers;
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

A failed AllocVec: the bug-orig test `popupwinlib-allocvec-unchecked`
(through inject). Not covered: a failed OpenWindow; real mouse input.
