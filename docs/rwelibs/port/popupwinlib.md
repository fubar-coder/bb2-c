# popupwinlib (16): popup menus in borderless windows

Reflective Images, 1994. Two commands:

* `PopUpInit`: a new message port for the windows.
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

## BUG(orig), kept

* **PopUpWindow always returns 0.** The menu levels return the chosen
  item, but the result is `selecteditem`, which is only cleared. (It is
  a statement anyway.) (#471)
* **Sizing a menu with sub menus toggles bit 1 of CIA-A PRA**
  (`BCHG #1,$bfe001`: the power LED / audio filter), a debug leftover
  (#468).
* **PopUpInit creates a new port** without deleting the old one (#470).
* **A failed AllocVec is not checked:** the menu structure is then at
  address 0. A failed OpenWindow returns 0 and closes nothing else
  (#467).
* **Closing a menu window replies every message in the shared port**,
  also those for the other levels (#469).

Not a defect: the item count at +8 of the structure is the number of
items − 1, but nothing uses the field.

## Test

PopUpWindow waits for the mouse, so the test drives it. While a popup
runs, exec's WaitPort is patched (SetFunction). When the library waits
on its port, the patch:

* takes a snapshot of the popup windows;
* sets their MouseX/MouseY to the next position of a script;
* puts the script's IntuiMessage (MOUSEMOVE, MOUSEBUTTONS,
  INACTIVEWINDOW) into the port.

The windows open on the Workbench screen at 0,0 (the screen object of
both sets is the Workbench screen); the library closes them all before
PopUpWindow returns.

Compared:

* per snapshot: the popup windows (position, size, IDCMP), their
  contents (copied with ClipBlit, hashed), bit 1 of CIA-A PRA relative
  to the start;
* the number of waits and of menu levels opened, d0, the kept
  registers;
* no memory lost after the first popup. The first windows leave memory
  in the system's caches (layers' ClipRects …), differently for each
  set, so the memory is counted from there.

Covered:

* init, finit;
* PopUpWindow with three levels: highlighting items, a separator, the
  mouse outside all windows, an INACTIVEWINDOW message, a sub menu
  opened by moving onto its item, back to the parent (the sub menu
  closes), onto the parent's active item (the sub menu stays), the third
  level left for the first (two levels closed), a mouse button with an
  active item;
* a mouse button without an active item;
* a menu without sub menus that starts with a separator;
* PopUpInit (a new port; the old one is left, the test deletes it).

Not covered: a failed AllocVec or OpenWindow; real mouse input.
