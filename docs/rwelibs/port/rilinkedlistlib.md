# rilinkedlistlib (linked lists, library 12)

**What it does:** up to five doubly linked lists (list# 0–4) of copies
of variables. `InitList list#,var` starts a list with one node holding a
copy of `var`; `AddListItem` inserts an empty node behind the current
one, `AddFirstListItem`/`AddLastListItem` at the start/end;
`SetListItem`/`GetListItem` copy a variable into/out of the current
node; `FirstListItem`, `LastListItem`, `NextListItem`, `PrevListItem`
move the current node; `DelListItem` deletes it; `AddrListItem` returns
it, `SizeOfListItem var` the size of a variable. A node is 18 bytes
(memlib globalloc, flags 0): next.l, prev.l, pad.w, data.l, size.l (the
v0.91 layout). The size of a variable is the word in front of it minus
8; the copies are made by RIAmosFuncLib's `CopyByte`.

**Source and reference:** `rwelibsrc/Listlib/listlib4.bb` (Reflective
Images, 1995, v0.91). The reference is the reference build of this
source (REFERENCE `refbuild`, `ref/rilinkedlistlib.obj`, ctest
`rwe_refbuild`): the OL `RILISTLIB.OBJ` is listlib4 without
`SizeOfListItem` (`.cache` analysis), the source is newer. The C build
has the declarations of the reference build
(`rwe_declarations_rilinkedlistlib`); the code is not byte-identical.
Library number `LIB_rilinkedlistlib` (12) in the rwe-1999 libnums.

**C:** everything (`libs/userlibs/rwe/rilinkedlistlib.c`); memlib
globalloc/globfree and CopyByte (`(75 << 7) + 10`, d0 source, d1
destination, d2 bytes: `BLITZ_ALIBJSR_D4`) through ALibJsr. The four
tables of five longs (first, last, an unused `_sizes`, current) are one
array in the original order, indexed by list# * 4 as in the original.
The state the original keeps in its code (`_size`, `listnum`, ...) is
kept in statics (no node takes over the size the last command left in
`_size`, #735). Every entry is a thunk that keeps d1/a0/a1 (the original
changes d1, d2, a0, a1, InitList also a2).

**Fixed:** intended deviations from the original
(`docs/libs/deviations.md`, kind F):

* AddFirstListItem returns -1, as the manual says. The original ended
  without setting its result: d0 was still the table offset list# * 4,
  so a successful call on list 0 returned 0, the failure result (#725;
  test `rilinkedlistlib-addfirst-result`, `libs_emulator` cases
  `[fixed #725]` of `rwe_rilinkedlistlib`).
* AddFirstListItem clears prev, pad, data and size of the new node. The
  original allocated it with flags 0 (no `MEMF_CLEAR`) and wrote only
  its next: prev, data and size were whatever the memory held, so
  PrevListItem, GetListItem and DelListItem used garbage pointers (#726;
  test `rilinkedlistlib-addfirst-uncleared`, `libs_emulator` cases
  `[fixed #726]` of `rwe_rilinkedlistlib`: the memlib stubs fill each
  block with a pattern).
* AddFirstListItem makes the new node the prev of the old first node.
  The original wrote it into that node's *next* (offset 0, where v0.9
  had prev): the old first node lost its link to the second one, and it
  and the new node pointed to each other, so NextListItem from the first
  node looped between the two (#724; test `rilinkedlistlib-addfirst-next`,
  `libs_emulator` cases `[fixed #724]` of `rwe_rilinkedlistlib`).
* AddListItem in the middle of a list sets the prev of the node behind
  the new one to the new node. The original left it pointing to the old
  current node: going back from there skipped the new node, and
  DelListItem of that node unlinked the new one from the forward chain
  (#727; test `rilinkedlistlib-addlistitem-prev`, `libs_emulator` cases
  `[fixed #727]` of `rwe_rilinkedlistlib`).
* DelListItem of the first node clears the prev of the node behind it,
  the new first node. The original cleared its *next* (offset 0, where
  v0.9 had prev): the list ended at the new first node, whose prev still
  pointed to the freed node (#728; test `rilinkedlistlib-dellistitem-first`,
  `libs_emulator` cases `[fixed #728]` of `rwe_rilinkedlistlib`).
* A node's size field (offset 14) is the size of its data: 0 for the
  new node of AddListItem and AddLastListItem, the size of the new copy
  after SetListItem. The original stored in AddListItem's node the size
  of the last variable any command looked at (InitList, SetListItem,
  GetListItem, SizeOfListItem) or of the node DelListItem last read, and
  SetListItem left the field as it was (#735; test
  `rilinkedlistlib-size-field`, `libs_emulator` cases `[fixed #735]` of
  `rwe_rilinkedlistlib`).
* DelListItem reads the node of v0.91 and frees the node's copy with the
  node. The original still read the v0.9 node (without pad): its data
  was the long at offset 8 (the pad word and the upper word of the data
  pointer), its size the long at 14, so it freed a wrong address (with
  memlib the runtime error "Unable to Free Memory", without runtime
  error checking a crash) and the copy was lost. The only node is
  looked at first and kept with its copy (the original freed the
  "data" before it looked) (#729; test
  `rilinkedlistlib-dellistitem-v09-layout`, `libs_emulator` cases
  `[fixed #729]` of `rwe_rilinkedlistlib`: the memlib stubs log the
  frees).
* SetListItem frees the node's old copy once the new one is allocated
  (without memory the old copy stays). The original stored the new copy
  over the pointer: every SetListItem on a node with data lost a copy
  until the end of the program (#734; test
  `rilinkedlistlib-setlistitem-leak`, `libs_emulator` cases
  `[fixed #734]` of `rwe_rilinkedlistlib`).
* GetListItem copies the smaller of the variable's size and the node's
  size field; the rest of a larger variable stays as it is. The original
  copied as many bytes as the variable has: into a larger variable also
  what lay behind the copy's allocation (#730; test
  `rilinkedlistlib-getlistitem-size`, `libs_emulator` cases
  `[fixed #730]` of `rwe_rilinkedlistlib`).
* A list without nodes: AddListItem, AddLastListItem, SetListItem,
  GetListItem, NextListItem, PrevListItem, FirstListItem, LastListItem
  and AddFirstListItem return 0 and change nothing (nothing is
  allocated), as the manual says ("0 indicates failure (no list ...)");
  AddFirstListItem does not start a list. The original took the current
  (or first) node, 0, without a check (only DelListItem tested it): the
  commands read and wrote from address 0 on (SetListItem wrote its
  copy's address to address 10, PrevListItem made ExecBase at address 4
  the current node), and FirstListItem and LastListItem returned -1.
  init clears the tables: a program run again with the library still
  loaded would otherwise find the freed nodes of the run before (#733;
  test `rilinkedlistlib-no-current-check`, `libs_emulator` cases
  `[fixed #733]` of `rwe_rilinkedlistlib`).
* InitList on a list that is already there frees its nodes and their
  copies first, from the first node on. The original overwrote the
  table entries: the old list was lost until the end of the program
  (memlib frees it there) (#731; test `rilinkedlistlib-initlist-no-free`,
  `libs_emulator` cases `[fixed #731]` of `rwe_rilinkedlistlib`: the
  memlib stubs log the frees).
* With runtime error checking a list# outside 0..4 is the runtime error
  "List number must be 0 to 4" (`listnumchk`, the error check of the
  twelve commands with a list number, in an error check area the
  original has not); the release code is unchanged (an invalid argument;
  the manual: "Do NOT use list numbers of 5 or higher! Your machine will
  crash if you try. Future version will check for this."). The original
  indexes its tables of five longs with list# * 4 without a check: a
  larger list number reaches the other tables and the code behind them
  (#732; test `rilinkedlistlib-listnum-unchecked`;
  `rilinkedlistlib.declarations-intended.txt` names the 14 differences
  of the declarations).

**BUG(orig)** (kept):

* A size word below 8 in front of the variable gives a size of almost
  4 GB (the allocation fails) (#736). No NEWTYPE variable has one: the
  compiler refuses a NEWTYPE of 32768 bytes or more ("Type too Big"),
  so memlib's length (the size + 8) fits in the word and is 8 or more;
  only an address that is not such a variable has one. Exactly 8 (an
  empty NEWTYPE) gives a size of 0, for which RIAmosFuncLib's CopyByte
  copies nothing (since #492; the original's CopyByte copied 2^32
  bytes).

DelListItem on the only node frees nothing and returns 0; that is not a
defect (the command refuses to delete the last node and reports it).

**DIFF:** none in the tested range. Without runtime error checking a
list# outside 0..4 indexes past the tables: into the code behind them
in the original, into other data in the C build (not reproducible, not
tested).

**Test** (`libs/tests/rwe/rilinkedlistlib.c`, snitch case "rwe
rilinkedlistlib"): each run loads one library set (the C build or the
original); the scenario writes its observations as cases, compared with
the expected data `libs/tests/expected/rwe_rilinkedlistlib.txt` recorded
from the original. memlib is replaced by stubs (`sysload_stubs`) that
allocate from a 64 KB aligned region (blocks filled with a pattern like
uncleared memory, freed ones overwritten), log every call and can make
the n-th allocation of a call fail; the wrong free of DelListItem is
only logged (the real memlib would stop the program). CopyByte is the
real one of the set's RIAmosFuncLib. The variables lie in the region.
Observed after every call: d0 (region addresses
relative), the memlib log, the used region word by word, the variables,
and the kept registers. Cases: init/finit; SizeOfListItem (also size
words 3 and 7); every command that is safe on empty lists 0–4 in the
original, and in the C build only the others on empty list 3;
InitList with each allocation failing and with too large sizes, again
on a list in use; AddListItem at the end and in the middle;
SetListItem (twice, failing, too large); GetListItem (no data, a larger
variable); Next/Prev over the list and past its ends; AddLastListItem
(also failing); AddFirstListItem (failing, twice, the loop);
DelListItem in the middle, at the end, at the start, the only node,
with and without data. Not covered: the original's commands that read
and write at address 0 on an empty list, list# outside 0..4, a size of
0.
