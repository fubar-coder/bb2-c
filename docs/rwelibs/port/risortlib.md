# risortlib (13): sorting lists by a string

Reflective Images. Sorts Blitz lists (`Dim List`) by a string field of
the items, optionally by a long priority field first.

A list (the array argument, d0): `lIsT` at -4, the first item at -28,
the current item at -32; an item: next (0), prev (4), the data of
`sizeof.type` bytes (8); the item whose next is 0 ends the list.

* `StringSort list,size[,offset]` – sorts from the first item;
  `StringSortItem list,size[,offset]` from the current one. `offset`
  (0 without it) is the offset of the string pointer in the data. A
  selection sort: each item is compared with every following one
  (utility.library `Stricmp`, case-insensitive) and the data (`size`
  bytes) are swapped where the order is wrong; the links stay. Returns
  -1 when done, 0 without utility.library, for no list (`lIsT`), a start
  item 0, or a string pointer 0 (stopped where it was, partly sorted).
* `ListBase(list)` – the list's address (d0 unchanged).
* `StringSortDir dir` – 0 ascending, anything else descending.
* `SortPriorityOffset offset` – the offset of a long in the data that is
  compared first (signed: the larger priority comes first, whatever the
  direction; equal priorities by the string); -1 (the default, any
  negative value) disables it. Documented nowhere (the 30/8/96 change of
  the source).

init opens utility.library (any version), finit closes it if it was
opened.

* **Source:** `rwelibsrc/sortlib/RISortlib2.bb` (as TED shows it:
  `build/amiga/rwelibsrc/sortlib/RISortlib2.bb`).
* **Reference:** the reference build of the source by blitz2 2.10
  (`refbuild`, `build/amiga/libs/userlibs/rwe/ref/risortlib.obj`): the
  source is newer than the binary of BSS2/BUM/OL/AB3 `RISORTLIB.OBJ`,
  which is the state before 30/8/96 without `SortPriorityOffset`
  (`docs/rwelibs/README.md` §3.2). Declarations equal (ctest
  `rwe_declarations_risortlib`).
* **C file:** `libs/userlibs/rwe/risortlib.c`; test
  `libs/tests/rwe/risortlib.c` (snitch case "rwe risortlib").

## C and assembler

All C behind `BLITZ_THUNK`s: d0–d1/a0–a1 kept for the statements, init
and finit, d1/a0–a1 for the rest (the original changes d0–d2/d5–d7/
a0–a1/a3/a6 in the sorts, d0–d1/a0–a1/a6 in init/finit, nothing in the
statements and `ListBase`).

## Fixed

Original bugs fixed in the port (`docs/libs/deviations.md`, 3a,
kind F):

* finit closed utility.library without checking it was opened: where
  it does not open (Kickstart 1.x has none), `CloseLibrary(0)` crashed
  at the end of every program with the library; finit tests the base
  (#824).
* The priority of the item the others are compared with was read before
  the test whether the priority is enabled: with the default -1 a long at
  data − 1, an odd address – an address error on a 68000 in every sort
  of two or more items without `SortPriorityOffset` (68020 and up read
  it, the result was not used). It is read only for an offset ≥ 0
  (#825).
* After a swap by priority the item's string was not loaded: the compare
  string became the one of the last string comparison (a1, possibly of
  an earlier item, or the caller's a1 if there was none yet), and the
  priority to compare with stayed the old one (the swapped-in item's was
  not taken over): with `SortPriorityOffset` the list was not sorted.
  Both are taken over now; a string pointer 0 in the swapped-in item
  stops the sort with 0, as in a string comparison (#826).
* A `size` of 0 swapped 65536 bytes per swap (`DBRA` with size − 1 =
  `$FFFF`), over the items behind the two swapped ones and the memory
  behind the list. Such a sort returns 0 before anything is swapped
  (one test per call) (#827).

## Test

Each run loads one library set (the C build or the original); the
scenario writes its observations as cases, compared with the expected
data `libs/tests/expected/rwe_risortlib.txt` recorded from the original.
Lists are built in memory; after a sort the data of the items in list
order are observed byte for byte (the string pointers as the strings'
first bytes). Covered: a sort before init
(0); `ListBase`; no list, no current item, an empty list, one item; 60
random lists (2..40 items of 8..17 bytes, strings that differ in case
only, are equal, empty, prefixes; both directions, both start items, both
forms, the string at several offsets, garbage in the upper words of the
word arguments); lists of lower case strings checked to be sorted; a
string pointer 0 at several places (0); 46 priority sorts (the long at 0
or 8, priorities from small ranges with negative ones, priorities
disabled by other negative offsets) including the stale compare string
(the caller's a1 set to a string); the priority sorts are checked to
leave the priorities in order (the original's do not in 29 of 43: the
`[orig]`/`[fixed #826]` pairs). Not covered here: size 0 (fixed #827,
the bug-orig test `risortlib-size0-swaps-65536`); the 68000 address
error (#825) is the bug-orig test `risortlib-priority-odd-read` on the
A600.
