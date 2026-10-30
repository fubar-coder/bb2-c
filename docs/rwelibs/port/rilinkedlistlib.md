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
kept in statics; `_size` matters: AddListItem uses the size the last
command left. Every entry is a thunk that keeps d1/a0/a1 (the original
changes d1, d2, a0, a1, InitList also a2).

**BUG(orig)** (kept):

* No command checks list# or (except DelListItem) whether the list has a
  current node: on an empty list AddListItem, SetListItem, GetListItem,
  NextListItem, PrevListItem and AddFirstListItem read and write from
  address 0 on (#732, #733).
* InitList does not free a list that is already there (#731).
* A size word below 8 in front of the variable gives a size of almost
  4 GB (the allocation fails); exactly 8 gives 0, which CopyByte takes
  as 2^32 bytes (#736).
* AddListItem does not set the prev of the node behind the new one (it
  still points to the current one) (#727); the new node's size field is
  the size of the last variable a command looked at (or of the last node
  DelListItem read) (#735).
* SetListItem does not free the old copy and does not set the size
  field (#734, #735); GetListItem copies as many bytes as the variable
  has, not as the copy has (#730).
* AddFirstListItem writes the new node into the old first node's *next*
  (not prev: the list becomes a loop) (#724), leaves prev, pad, data and
  size of the new node as the allocation left them (not cleared) (#726),
  does not touch the last node and returns list# * 4 (#725).
* DelListItem still reads the v0.9 node (without pad): its data is the
  long at offset 8 (the pad word and the upper word of the data
  pointer), its size the long at 14. So it frees a wrong address (with
  memlib: the runtime error "Unable to Free Memory") and the copy is
  lost (#729). Deleting the first node clears the *next* (not prev) of
  the node behind it (the list ends there) (#728).

DelListItem on the only node frees nothing and returns 0; that is not a
defect (the command refuses to delete the last node and reports it).

**DIFF:** none in the tested range. list# outside 0..4 indexes past the
tables: into the code behind them in the original, into other data in
the C build (not reproducible, not tested).

**Test** (`libs/tests/rwe/rilinkedlistlib.c`, snitch case "rwe
rilinkedlistlib"): memlib is replaced by stubs (`sysload_stubs`) that
allocate from a 64 KB aligned region per set (blocks filled with a
pattern like uncleared memory, freed ones overwritten), log every call
and can make the n-th allocation of a call fail; the wrong free of
DelListItem is only logged (the real memlib would stop the program).
CopyByte is the real one of each set's RIAmosFuncLib. The variables lie
in the region. Compared after every call: d0 (region addresses
relative), the memlib log, the used region word by word, the variables,
and the kept registers. Cases: init/finit; SizeOfListItem (also size
words 3 and 7); every command that is safe on empty lists 0–4;
InitList with each allocation failing and with too large sizes, again
on a list in use; AddListItem at the end and in the middle;
SetListItem (twice, failing, too large); GetListItem (no data, a larger
variable); Next/Prev over the list and past its ends; AddLastListItem
(also failing); AddFirstListItem (failing, twice, the loop);
DelListItem in the middle, at the end, at the start, the only node,
with and without data. Not covered: the commands that read and write at
address 0 on an empty list, list# outside 0..4, a size of 0.
