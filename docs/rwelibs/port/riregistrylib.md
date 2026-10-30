# riregistrylib (3): the interface to registry.library

Red When Excited, 1997. 35 commands, each one call of registry.library
(REDRegistry). That library was only given out as a pre-release and
exists nowhere; neither do its includes, so the LVOs, registers and the
structure below are what the source itself defines.

init opens registry.library (any version). Without it IsRegistryOpen
is 0 and every command returns 0. finit unlocks every structure still
locked and closes the library.

* **Source:** `rwelibsrc/Registry/Registry3.bb` (file date 1997-07-10),
  a BB22 source: converted by `tools/bb22` in the build (readable:
  `build/amiga/rwelibsrc/Registry/Registry3.bb`). Registry2 has 27 of
  the commands, Registry 13.
* **Reference:** the reference build of the source (`REFERENCE
  refbuild`, stage D: no binary exists). The declarations equal it
  (ctest `rwe_declarations_riregistrylib`).
* **C file:** `libs/userlibs/rwe/riregistrylib.c`; test
  `libs/tests/rwe/riregistrylib.c` (snitch case "rwe riregistrylib").

## Data

* **The structure stack:** `structs`, 16 longs; `curstruct`, a word
  byte offset (the first structure at 4); `regstruct`, the current
  structure; then `regbase`. LockRegistry pushes what the library
  returns (also 0), UnlockRegistry pops.
* **CallLibrary:** a0 = regstruct, a6 = regbase, `JMP 0(a6,d7.w)`; no
  call and d0 = 0 without a structure. Every command but
  IsRegistryOpen and LockRegistry goes through it.
* **The registry structure:**
  * +14 the name (ParseItemName$);
  * +18.w the type: 1 string, 2 data, 3 boolean, 4 integer;
  * +24 the value: a string for type 1, the number for types 3/4.

The registers of the calls (as the source loads them; a0 = the
structure):

| LVO | Call | Registers |
|---|---|---|
| −30 | LockRegistry | d0 = access mode |
| −36 | UnlockRegistry | – |
| −42 | SetRegistryBaseLibrary | a1 |
| −48 | ExistsLibrary | a1, d0 = 0 |
| −54 | GetLibraryValue | a1, d0 = 0 |
| −60 | GetRegistryVariable | a1, d0 = 0 (also the Get…Variable functions) |
| −66 | SetRegistryVariable | a1 name, a2 value, d0 type, d1 size |
| −72 | ChangeRegistryVariable | a1, a2, d0 type, d1 size |
| −78 | NewRegistryLibrary | a1, a2, a3 = file name (form 1: 0) |
| −84 | ParseRegistryLibrary | a1 |
| −96 | NextParseItem | – |
| −102 | ChangeRegistryAccess | – |
| −108/−114/−120/−138 | Delete…Library/Variable, NumRegistryLibraryVariables, DeleteRegistryLibraryVariables | a1 |
| −126/−132/−144/−156/−162/−174/−186/−192/−198 | Save/LoadRegistryLibrary, SetLibraryValue, RenameRegistryVariable, SaveLibraryVariables, RenameRegistryLibrary, Get/FreeStructureVariables, LoadLibraryVariables | a1, a2 |
| −150 | SetRegistryVariableIndex | a1, a2, d0 type, d1 size, d2 index |
| −180 | InsertNewRegistryLibrary | a1, a2, a3 = file name (form 1: 0), a4 = insert before |

The string functions (GetLibraryValue$, GetRegistryStringVariable,
ParseItemName$, ParseItemValue$) copy the structure's name or string
value to a3; the first two write over their argument (`CORRECTa3`).
GetRegistryInt/BoolVariable return the value if the call returns the
type 3 or 4.

## C and assembler

All C behind thunks that keep d1/a0/a1 (and d0 in the statements). The
library calls go through `regjsr()`: inline assembler, because a3/a4
are the fixed registers of the Blitz program and the call's offset is a
register (`JSR 0(a6,d7.w)`). The stack, curstruct, regstruct and
regbase lie one after the other as in the original, with the two longs
the original has in front of the stack (the end of its token table).

The registers the source does not load are not handed through (the
original passes the caller's on): a library that keeps to the interface
above does not see a difference.

## BUG(orig), kept

* **The structure stack has no limits.** LockRegistry beyond 15 levels
  writes over curstruct and regstruct. UnlockRegistry without a lock
  goes below the stack, into the end of the token table: first the
  libfin's `Dc.l 0` (no structure), then `$0000FFFF`, which the next
  UnlockRegistry "unlocks" (a0 = $FFFF) and overwrites with 0 (#781).
* **GetLibraryValue$ and GetRegistryStringVariable without a structure**
  read the type and the value at 18/24 of address 0 (CallLibrary's 0
  does not stop them) (#780).
* **The string results are copied with their 0**, and a3 is left behind
  it (#783).
* **SetRegistryVariable name,value$ and SetRegistryInt/BoolVariable**
  pass the caller's d2 as the size (#782).
* **finit leaves regstruct and the base set:** after a new init, the
  calls use the old structure (#779).

LockRegistry stores its result also when it is 0: the calls do nothing
then, but UnlockRegistry still pops it. That is no defect, it keeps
LockRegistry and UnlockRegistry balanced.

## DIFF

* **NewRegistryLibrary and InsertNewRegistryLibrary restore a3/a4**
  after the call. The original leaves the file name in a3 and the
  "insert before" name in a4: the program's string pointer and locals.
* **Further below the stack** (a third UnlockRegistry without a lock and
  more) the C build reads other memory than the original's token table.
* **a2 stays unchanged** in the string functions (the original takes
  the argument's length from it), as in all C ports.

## Test

registry.library exists nowhere, so the test runs both sets first
without it, then with a mock made by the test: MakeLibrary, in the
system list as "registry.library". Each of its 33 functions logs its
LVO and registers and returns a value the test chooses. LockRegistry
returns one of three registry structures shared by both sets (name,
type, value set by the test). Both sets call the same mock.

Compared:

* d0;
* the log: the LVOs and the registers the source loads for each call
  (pointers into a set's string area relative to it);
* for the string functions a3 and the string area;
* the kept registers; the mock closed again by finit; the memory back.

Covered:

* Without the library: init, IsRegistryOpen, LockRegistry and every
  command (nothing called, 0; the reads at address 0), finit.
* With the mock:
  * init, IsRegistryOpen;
  * every command and form before a lock (nothing called) and with a
    structure;
  * the string functions with a string value, another type, no value,
    no name; GetRegistryInt/BoolVariable with the types 2, 3, 4 (and a
    type with an upper word);
  * nested locks, a failed lock (0 pushed), UnlockRegistry back to the
    structure below;
  * finit with three locks (unlocked from the top);
  * after finit and a new init: the stale structure, UnlockRegistry
    below the stack (the libfin's 0, then `$0000FFFF` unlocked).

Not covered:

* more than 15 locks (curstruct and regstruct get the high word of a
  structure's address);
* a fourth UnlockRegistry below the stack (DIFF);
* a structure while registry.library is missing (the call goes to the
  address of the LVO).
