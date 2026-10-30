# rigtmenulib (37): GadTools menus

Reflective Images. This is an object library: "GTMenuList", 16 bytes per
object, default maximum 5. A program builds menus with GTMenuTitle,
GTMenuItem and GTSubItem; CreateMenuStrip turns them into a GadTools
menu strip (CreateMenusA, LayoutMenusA with the visual info of the used
screen). GTSetMenu and GTUnSetMenu attach the strip to the used window
and remove it.

GTMenuState, GTMenuChecked, GTSetMenuChecked and GTGetUserData work on
the strip itself: MENUENABLED, ITEMENABLED and CHECKED of
Intuition's Menu/MenuItem, and GadTools' UserData behind the MenuItem.
GTFreeMenu removes a title, an item or a sub item from the lists. The
object's free routine frees the strip and the lists. init opens
gadtools.library.

* **Source:** `rwelibsrc/GTMenuLib/GTMenuLib3.bb` (10/03/96; readable:
  `build/amiga/rwelibsrc/GTMenuLib/GTMenuLib3.bb`).
* **Reference:** the reference build of the source (`REFERENCE
  refbuild`). BSS2 `RIGTMENULIB.OBJ` (= AB3) differs from it in GTSubItem
  only (`docs/rwelibs/README.md` §3.2):
  * its form 3 has no `#push` on shortcut$;
  * its code takes the shortcut's address from d6, while the source takes
    the length and the address from the stack.

  The declarations equal the reference build (ctest
  `rwe_declarations_rigtmenulib`).
* **C file:** `libs/userlibs/rwe/rigtmenulib.c`; test
  `libs/tests/rwe/rigtmenulib.c` (snitch case "rwe rigtmenulib").

## Data

* **The object:**
  * +0.l the menu strip;
  * +4.l the title list;
  * +8.w the number of entries added.
* **Title** (12 bytes, memlib globalloc): +0 next, +4 the text (maxslib
  AllocSt), +8 the first item.
* **Item or sub item** (30 bytes, cleared):
  * +0 next, +4 the text (−1 = NM_BARLABEL);
  * +12.w flags, +14 the shortcut;
  * +18 the first sub item;
  * +22 mutual exclude, +26 UserData.

CreateMenuStrip builds a NewMenu array of (count + 1) × 20 bytes from the
lists and frees it afterwards.

## C and assembler

All C behind thunks: d0/d1/a0/a1 are kept for the statements, d1/a0/a1
for the functions. The walks are the original's DBF loops on words:

* n + 1 steps, 65536 for $FFFF;
* inserting "after entry n − 1" is `SUBQ.w #1:BMI` (n = 0: at the head);
* the menu strip walks follow NextMenu/FirstItem (+18), NextItem/SubItem
  (+28) and read address 0 when an entry is missing.

The string lengths (and GTSubItem's pushed shortcut$, mutual exclude and
UserData) are read from the compiler's a2 stack in the original's order,
without moving a2 (DIFF, as in all ports: `docs/libs/cmdlibs-port.md`
"General"). Stack offsets, in order of reading:

* GTMenuItem: −4 the text's length, −8 the shortcut's length; form 4
  UserData at −16;
* GTSubItem: −4 the text's length, −8 the shortcut's length, −12 the
  shortcut's address; −16 mutual exclude, −20 UserData.

The error checks are C in the error check area (BLITZ_ERRCHK):
createchk/setchk ("GTMenu has no data to build GTMenuList from!"),
statechk2–4 and chkchk3–5 ("Illegal Menu / Menu Item / Menu Sub Item
number", the existschk walk of menuslib). No assembler.

## BUG(orig), kept

* **finit** closes gadtools.library only if it is NOT open (`TST.l
  gtbase:BNE 'ok`), so it never closes it (#690).
* **GTMenuItem form 4** (with UserData) jumps past `MOVE.l d6,me`: the
  item gets the mutual exclude of the GTMenuItem/GTSubItem before (#692).
* **The shortcut$ is tested by the low word of its address** (`TST.w
  d5`/`d6`): a string at an address with low word 0 counts as no shortcut
  (#694).
* **CreateMenuStrip:** with no title, the lists are read at address 0; a
  failed allocation writes the array to address 0; CreateMenusA and
  GetVisualInfoA are not checked (#689).
* **The title strings are never freed** (the object's free routine,
  GTFreeMenu) (#695).
* **GTFreeMenu** tests the item and sub item numbers as longs (`TST.l`),
  so the upper word of a word argument decides whether they are given
  (#691).
* **GTMenuState and GTSetMenuChecked** change the flags of the strip
  directly, also while it is attached to a window; a strip made again
  loses them (#693).

Like the original, and no defects: GTMenuTitle allocates the title
uncleared (MEMF_PUBLIC; all its fields are written); the count at +8
grows with every entry added and is never decreased by GTFreeMenu (the
NewMenu array of CreateMenuStrip, (count + 1) × 20 bytes, is only
larger than needed); the free routine calls FreeMenus with the gadtools
base unchecked (a strip exists only if gadtools was open); GTMenuChecked
returns −1/0 in the low word (`SNE:EXT.w`), the upper word of d0 (the
list number's) stays (a word result).

## Test

Each set has its own memlib/maxslib, two GTMenuList objects and a window
on the Workbench screen, the "used screen" of both.

Compared:

* the lists: every title, item and sub item (texts, flags, shortcuts,
  mutual exclude, UserData) and the count;
* the menu strips: every Menu/MenuItem field but the pointers, the texts,
  the commands, GadTools' UserData;
* the window's MenuStrip;
* d0 of the functions and the kept registers;
* memlib's memory, back at the end.

Covered:

* GTMenuTitle at the head, between, at the end, and behind a missing
  title;
* GTMenuItem forms 0–4 (form 4 keeps the previous mutual exclude) and
  GTSubItem forms 0–4, with missing titles, items and sub items;
* CreateMenuStrip twice (the old strip freed);
* GTSetMenu with and without a strip, and on a window that has one;
  GTUnSetMenu with and without a strip;
* GTMenuState in all forms;
* GTMenuChecked and GTSetMenuChecked in both forms;
* GTGetUserData in both forms (sub item −1);
* GTFreeMenu in all forms, with missing entries;
* every error check, with existing entries;
* the object's free routine, init and finit.

Not covered:

* the error checks with missing entries (runtime errors, TRAP #0);
* a shortcut$ at an address with low word 0;
* CreateMenuStrip without titles;
* walks past the end of the strip (they write near address 0).
