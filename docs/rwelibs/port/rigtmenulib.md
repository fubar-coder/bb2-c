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
number", the existschk walk of menuslib). No assembler. The thunks
follow all check functions: before, the check functions behind the
first thunk landed in the normal code (`docs/libs/deviations.md`,
section 6; the three `blitzlib check` warnings of rigtmenulib).

## Fixed

Intended deviations from the original:

* **GTMenuItem form 4** (with UserData) takes its mutual exclude from
  −12(a2), where the compiler pushes the 7th argument (from the 7th on
  the arguments are pushed, d6 holds the last one: UserData). The
  original jumps past `MOVE.l d6,me`: the item got the mutual exclude of
  the GTMenuItem/GTSubItem before (#692; test
  `rigtmenulib-menuitem-form4-mutex`, `libs_emulator` cases
  `[fixed #692]` of `rwe_rigtmenulib`).
* **GTFreeMenu** tests the item and sub item numbers as words, as they
  are declared. The original tests them as longs (`TST.l`): a negative
  upper word, which the compiled code leaves in d2/d3 as it likes, made
  the number "not given", and GTFreeMenu freed the whole item (or the
  whole title) instead (#691; test `rigtmenulib-freemenu-tst-long`).
* **The title strings are freed** (maxslib FreeSt) with their titles, by
  GTFreeMenu of a title and by the object's free routine. The original
  freed only the 12-byte node: each removed title kept its string until
  the end of the program (#695; test
  `rigtmenulib-title-strings-not-freed`).
* **finit closes gadtools.library** if it is open. The original tests
  the base the wrong way round (`TST.l gtbase:BNE 'ok`) and never closes
  it: every run left gadtools.library open once more (#690; test
  `rigtmenulib-finit-never-closes`, through inject: the opens minus the
  closes). The objects are freed before finit (the compiler's free
  loop of a library comes before its finit, `makefinits`), so the free
  routine's FreeMenus needs no test of the base.
* **CreateMenuStrip checks what it builds from.** On a list without
  titles it frees the old strip, as the original does first, and returns
  0: the object has no strip, and a later GTSetMenu attaches nothing
  (user decision W3 A finding 3). The original reads the "title" at
  address 0, walks what it finds there and makes a strip of it, which
  GTSetMenu attaches (its error check `createchk` reports the empty
  list). When CreateMenusA, GetVisualInfoA
  (the screen cannot be locked) or LayoutMenusA fails, it frees the strip
  and returns 0; the original checks none of them, passes a strip or a
  visual info of 0 on to LayoutMenusA and FreeVisualInfo and returns -1
  (#689; test `rigtmenulib-createmenustrip-unchecked`, `libs_emulator`
  cases `[fixed #689]` of `rwe_rigtmenulib`: gadtools patched). A failed
  allocation of the NewMenu array does not return in the original
  (memlib raises "Out of Memory!", `libs/acidlibs/memlib.c`); if the
  runtime error returns in the port, makearray gives 0 before writing.
* **GTMenuItem and GTSubItem test the whole address of shortcut$.** The
  original tests its low word (`TST.w d5`/`d6`): a string at an address
  with low word 0 counted as no shortcut, the item lost its key (#694;
  `libs_emulator` cases `[fixed #694]` of `rwe_rigtmenulib`: a string at
  such an address).

## No defects

* **GTMenuState and GTSetMenuChecked** change the flags of the strip
  (`BSET`/`BCLR` on MENUENABLED, ITEMENABLED, CHECKED), also while it is
  attached to a window, and not in the lists: a strip made again by
  CreateMenuStrip has the flags of the lists. That is how the library is
  documented (RIGTMenuLib.guide in
  `archive/aminet/ab2.42-distri/Amiblitz2/REDHelp/REDBlitzLibs`:
  GTMenuChecked "you must have called either CreateMenuStrip or
  GTSetMenu before you use this command", GTSetMenuChecked "The same
  rules apply"; GTFreeMenu "This change will not be reflected in the
  actual menulist created by CreateMenuStrip until you recall
  CreateMenuStrip"), and it follows the Acid library
  ("designed so that the only major differences between them are the
  slight command name changes"), whose menuslib changes the flags of its
  strip the same way (`libs/acidlibs/menuslib.c`). Kept as it is (#693;
  user decision W3 Q2).

Like the original, and no defects: GTMenuTitle allocates the title
uncleared (MEMF_PUBLIC; all its fields are written); the count at +8
grows with every entry added and is never decreased by GTFreeMenu (the
NewMenu array of CreateMenuStrip, (count + 1) × 20 bytes, is only
larger than needed); the free routine calls FreeMenus with the gadtools
base unchecked (a strip exists only if gadtools was open); GTMenuChecked
returns −1/0 in the low word (`SNE:EXT.w`), the upper word of d0 (the
list number's) stays (a word result).

## Test

Each run loads one library set (the C build or the original); the
scenario writes its observations as cases, compared with the expected
data `libs/tests/expected/rwe_rigtmenulib.txt` recorded from the
original. The set has its memlib/maxslib, two GTMenuList objects and a
window on the Workbench screen, the "used screen".

Observed:

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
* GTMenuItem forms 0–4 (the original's form 4 keeps the previous mutual
  exclude, #692; the arguments from the 7th on as the compiler passes
  them: pushed, d6 the last one) and
  GTSubItem forms 0–4, with missing titles, items and sub items;
* CreateMenuStrip twice (the old strip freed), and with CreateMenusA,
  GetVisualInfoA or LayoutMenusA failing (gadtools patched for the test
  task);
* GTMenuItem and GTSubItem with a shortcut$ at an address with low
  word 0;
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
* CreateMenuStrip without titles (the original reads address 0; the
  bug-orig test `rigtmenulib-createmenustrip-unchecked`);
* walks past the end of the strip (they write near address 0).
