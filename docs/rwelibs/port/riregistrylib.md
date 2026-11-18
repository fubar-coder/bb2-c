# riregistrylib (3): the interface to registry.library

Red When Excited, 1997. 35 commands, each one call of registry.library
(REDRegistry). That library was only given out as a pre-release and
exists nowhere; neither do its includes, so the LVOs, registers and the
structure below are what the source itself defines.

init opens registry.library (any version). Without it IsRegistryOpen
is 0 and every command returns 0. finit unlocks every structure still
locked and closes the library.

* **Source:** `rwelibsrc/Registry/Registry3.bb` (file date 1997-07-10),
  a BB22 source: converted by `bbsrc bb22` in the build (readable:
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

## Fixed

Intended deviations from the original (`docs/libs/deviations.md`,
kind F):

* The string results (GetLibraryValue$, GetRegistryStringVariable,
  ParseItemName$, ParseItemValue$) leave a3 on their 0; the original
  copied them with their 0 and left a3 behind it, so in a string
  expression the next part was written one byte too far (#783).
* finit clears regstruct (also without the library) and the base. The
  original left both set: after a new init (the next run of the
  program while the library stays loaded) every call before the first
  LockRegistry went with the structure finit had unlocked, and
  UnlockRegistry unlocked it a second time (#779; `libs_emulator` cases
  `[fixed #779]` of `rwe_riregistrylib`).
* GetLibraryValue$ and GetRegistryStringVariable return "" without a
  structure, as ParseItemValue$ does. The original read the type and the
  value at 18/24 of address 0 (CallLibrary's 0 did not stop them) - and
  without registry.library every call takes that path: when the word at
  18 was 1, the string at the address in the long at 24 was copied
  (#780; test `riregistrylib-getstring-address0`).
* UnlockRegistry does nothing without a pushed lock. The original went
  below the stack, into the end of its token table: first the libfin's
  `Dc.l 0` (no structure), then `$0000FFFF`, which the next
  UnlockRegistry "unlocked" (a0 = $FFFF) and overwrote with 0. Without
  registry.library, which exists nowhere, LockRegistry pushes nothing,
  so every LockRegistry/UnlockRegistry pair went one entry lower, and
  the call went through a base of 0: a crash after a few pairs (#781,
  the lower limit; test `riregistrylib-unlock-below-stack`,
  `libs_emulator` cases `[fixed #781]` of `rwe_riregistrylib`).

## BUG(orig), kept

* **The structure stack has no upper limit.** LockRegistry beyond 15
  levels writes over curstruct and regstruct (#781).
* **SetRegistryVariable name,value$ and SetRegistryInt/BoolVariable**
  pass the caller's d2 as the size (#782).

LockRegistry stores its result also when it is 0: the calls do nothing
then, but UnlockRegistry still pops it. That is no defect, it keeps
LockRegistry and UnlockRegistry balanced.

## DIFF

* **NewRegistryLibrary and InsertNewRegistryLibrary restore a3/a4**
  after the call. The original leaves the file name in a3 and the
  "insert before" name in a4: the program's string pointer and locals.
* **a2 stays unchanged** in the string functions (the original takes
  the argument's length from it), as in all C ports.

## Test

Each run loads one library set (the C build or the original); the
scenario writes its observations as cases, compared with the expected
data `libs/tests/expected/rwe_riregistrylib.txt` recorded from the
original.

registry.library exists nowhere, so the test runs first without it, then
with a mock made by the test: MakeLibrary, in the system list as
"registry.library". Each of its 33 functions logs its LVO and registers
and returns a value the test chooses. LockRegistry returns one of three
registry structures (name, type, value set by the test).

Observed:

* d0;
* the log: the LVOs and the registers the source loads for each call
  (pointers into the string area relative to it);
* for the string functions a3 and the string area;
* the kept registers; the mock closed again by finit; the memory back.

Covered:

* Without the library: init, IsRegistryOpen, LockRegistry and every
  command (nothing called, 0; the string functions ""), finit.
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
  * after finit and a new init: no stale structure (the original's,
    fixed #779), three UnlockRegistry without a lock (nothing; the
    original goes below the stack to the libfin's 0, then unlocks
    `$0000FFFF`, fixed #781).

Not covered:

* more than 15 locks (curstruct and regstruct get the high word of a
  structure's address);
* a structure while registry.library is missing (the call goes to the
  address of the LVO).
