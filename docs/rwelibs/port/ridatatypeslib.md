# ridatatypeslib (DataTypes, library 25)

**What it does:** one file at a time through datatypes.library.
`DataTypeOpen file$` locks the file and obtains its DataType
(`ObtainDataTypeA(DTST_FILE)`); `DataTypeName`, `DataTypeBaseName`,
`DataTypeID`, `DataTypeGroup` return the strings of its
DataTypeHeader. `DataTypeCreate [&taglist]` makes the object
(`NewDTObjectA` of the file's name), `DataTypeSuper` returns its class
ID, `DataTypeBitmap` its BitMapHeader (then `DataTypeWidth`, `Height`,
`Depth`, `DataTypeModeID [taglist]` = BestModeID for it),
`DataTypesPalette palette#` copies its colours into a palette object,
`SetDataTypeAttr`/`GetDataTypeAttr` set and get attributes.
`AttachDataType window,id[,x,y,w,h] | window,id,&taglist` adds the
object to a window (`AddDTObject`), `DataTypeReady` tells whether its
layout is done, `DetachDataType` removes it, `DataTypeClose` frees
everything. init opens datatypes, dos, intuition and graphics (V37) and
allocates a 1024 byte buffer (memlib, MEMF_CLEAR) for the tag lists
and GetAttr values; finit detaches, closes and frees. All commands are
Amiga mode only; entry 15 is an empty nullsub (the commented-out
`RedrawDataType`).

**Source and reference:** `rwelibsrc/datatypeslib6.bb` (Leading Edge
Software, S. Matty, 1996-07-29), with `#beta=1` (`BitmapDataType` left
out). The reference is the reference build of this source (REFERENCE
`refbuild`, `ref/ridatatypeslib.obj`, ctest `rwe_refbuild`); the BSS2
`RIDATATYPESLIB.OBJ` (= AmiBlitz3) has the same declarations but is an
earlier build without the 13 `TST.l database` checks (`.cache`
analysis), so the source is newer. The C build has the declarations of
the reference build (`rwe_declarations_ridatatypeslib`); the code is
not byte-identical. Library number `LIB_ridatatypeslib` (25) in the
rwe-1999 libnums.

**C:** everything (`libs/userlibs/rwe/ridatatypeslib.c`): the
libraries through the NDK inlines on the original's base table
(`libbases`, in the order of the original, so finit's loop stops at the
same entry), memlib globalloc/globfree through `BLITZ_ALIBJSR`. The
string functions write at a3 (`blitz_string`) like the original:
DataTypeName/BaseName/Super copy the 0 too (a3 behind it, d0 the
length), ID/Group up to four characters without a 0. Word results are
returned as UWORD (the upper word of d0 is not part of a word result).
SetDataTypeAttr with pairs reads them below a2 without changing it.
Every entry is a thunk that keeps those of d0/d1/a0/a1 that are not the
result; the original changes a6 everywhere, a0/a1 nearly everywhere,
d1 in the string functions, d2 (DataTypeOpen), d3 (DataTypesPalette),
a2 (init, finit, AttachDataType) and d7/a2 (SetDataTypeAttr pairs).

**BUG(orig)** (kept):

* DataTypeOpen copies the name into its 384 byte buffer without a limit
  (#604).
* A second DataTypeCreate loses the object before it (#600).
* AttachDataType window,id,x,y,w,h puts its word arguments into the tag list as longs, with whatever
  upper words the registers have; window,id,&taglist passes the id as
  the tag list of its second SetAttrsA and ignores the tag list (#597,
  #596).
* DataTypesPalette copies as many colours as the palette object has
  (reads behind the picture's colours; 0 colours are 65536); without
  colours it returns GetAttr's result, otherwise the buffer's address
  (#606, #607).
* SetDataTypeAttr with pairs: 0 pairs are 65536 (DBRA), more than 127
  overflow the buffer (#608).
* DataTypeModeID taglist writes its own tags into the caller's list
  (#603).
* finit closes the libraries only up to the first one that did not
  open (#602).

**Fixed** (intended deviations, `docs/libs/deviations.md` 3a, kind F):

* DataTypeBitmap clears the buffer before GetAttr: an object that has no
  BitMapHeader (not a picture) gives 0. The original took the first long
  the command before had left in the buffer (a tag of SetDataTypeAttr,
  a value of GetDataTypeAttr) as the BitMapHeader, and DataTypeWidth,
  Height, Depth, ModeID and AttachDataType read through it (#598).
* AttachDataType window,id needs an object, and without DataTypeBitmap
  before it gets the BitMapHeader itself, as DataTypeBitmap does (it is
  then the library's for DataTypeWidth etc. too); an object without one
  (not a picture) is not attached, the result is 0. The original read
  the size through the unchecked pointer: without DataTypeBitmap the two
  words at address 0, an object of 0 x 0 (#595).
* DataTypeClose detaches the object from its window first, as finit
  does. The original disposed of an attached object: the window kept the
  freed gadget (its next refresh, a click or closing it used freed
  memory), and the library still held the window, so the next
  AttachDataType returned 0 and DetachDataType removed the object 0
  (#599).
* DataTypeOpen unlocks the file again and returns 0 when ObtainDataType
  fails (on Kickstart 3.1 for an empty file), and
  DataTypeClose unlocks the file whenever it is locked. The original
  read the DataTypeHeader through address 28 (DataTypeName and
  DataTypeGroup then copied from there) and kept the lock, which
  DataTypeClose released only with a DataType: the file stayed locked
  and DataTypeOpen refused every further file of the run (#605).
* DataTypeCreate without a tag list needs an open file, as the form with
  one; without, the result is 0. The original made an object of the name
  the last DataTypeOpen had copied: at first "" (NewDTObject fails for
  it on Kickstart 3.1), later the name of a file closed since, an object
  of a file the library does not count as open (#601).

Not a defect (kept): DataTypeHeight, Depth, ModeID, DataTypesPalette,
Set/GetDataTypeAttr, DetachDataType and AttachDataType without x,y,w,h
do not check datatypes.library (without it there is no file or object).

**DIFF:** SetDataTypeAttr with pairs leaves a2 unchanged (the original
leaves it below the pairs), as in all C ports
(`docs/libs/cmdlibs-port.md`).

**Test** (`libs/tests/rwe/ridatatypeslib.c`, snitch case "rwe
ridatatypeslib"): each run loads one library set (the C build or the
original); the scenario writes its observations as cases, compared with
the expected data `libs/tests/expected/rwe_ridatatypeslib.txt` recorded
from the original. The test writes a 16×8 ILBM with 2 planes and 4
colours and a text file to RAM: and opens a window on the Workbench
screen; the library uses the system's datatypes.library (ilbm.datatype,
ascii.datatype). memlib is replaced by stubs (`sysload_stubs`,
AllocMem/FreeMem, sizes and flags logged and observed). Observed after
every call: d0 (the low word of word results), a3 and the string
area, the BitMapHeader, the library's buffer after every command that
fills it (tags of DataTypeModeID, SetDataTypeAttr, all three
AttachDataType forms), the values of GetDataTypeAttr, the palette and
tag list written by the commands, the kept registers; the picture's
strings ("ILBM", "ilbm", "ilbm", "pict", "ilbm.datatype"), size, depth
and colour 1 are also observed as the truth of a comparison with their
known values. DataTypeReady is polled until the layout is done. Cases: every command
and form with nothing open; DataTypeCreate without a file; a missing
file; the text file; a second DataTypeOpen; the picture with every
command and form (unknown attributes, 1 and 2 pairs with a marker in
d7's upper word, 4 and 2 colours, x and y with upper words 1 and 2, id
0 as tag list); a second AttachDataType; DetachDataType twice;
DataTypeClose; DataTypeCreate with a tag list; finit with the object
attached (detached, closed, the file unlocked: an exclusive Lock
succeeds); init and finit again. Not covered: a library missing at
init, ObtainDataType failing, AttachDataType window,id without
DataTypeBitmap, DataTypeClose of an attached object, DataTypesPalette
with more colours than the picture or 0, SetDataTypeAttr with 0 or over
127 pairs, names over 383 characters.
