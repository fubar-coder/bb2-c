# riamosfunclib (userlib 75): AMOS functions

`libs/userlibs/rwe/riamosfunclib.c`, the port of
`rwelibsrc/AMOSFunclib/func-library7.bb` (1996-09-18; as TED shows it:
`build/amiga/rwelibsrc/AMOSFunclib/func-library7.bb`) to C, in the state
of the reference chosen in [README](../README.md) 3.2: AmiBlitz3's SVN
binary `archive/amiblitz3/svn-r279/BlitzLibs/UserLibs/userlibs/RIAmosFuncLib.obj`.
The declarations equal the reference (ctest `rwe_declarations_riamosfunclib`);
the code is C, not byte-identical. Test: `libs/tests/rwe/riamosfunclib.c`
(snitch case "rwe riamosfunclib").

## What it does

AMOS-like helpers by RI for Ott of The Farm:

* **Banks:** two tables of `maxbanks` longs (default 50) in one memlib
  `globalloc` block (`MEMF_CLEAR`): the length and the address (exec
  `AllocMem`) of each bank. `Reserve(length)` (the first free bank,
  `MEMF_PUBLIC`; its number or -1), `Reserve(bank,length[,requirements])`
  (-1 / 0; a bank in use is erased first), `Start(bank)`, `Length(bank)`,
  `Erase bank`, `EraseAll`, `NextBank [startbank]` (-1 if none),
  `SwapBank(a,b)`, `BankLimit [numbanks]` (finit + init with the new
  number). A number from 0 to `maxbanks` − 1 is a bank, anything else
  an address (FillMem, BLoad, BSave).
* **Files:** `FileSize(name$)` (Examine with an AllocMem'd
  FileInfoBlock, for OS 1.3), `BLoad(name$[,address/bank[,length[,offset[,memtype]]]])`
  (form 0: into a new bank, its number), `BSave(name$,address/bank[,length])`,
  `MakeDir(dir$)`, `Rename(old$,new$)`, `ChangeDir dir$`,
  `SaveExec name$,list()` (an executable from a Blitz list of NEWTYPE
  `.hunk`: code hunks with RELOC32), `FindVolume(name)` (the DosList
  entry), `DeviceName$(volumename$)` (the handler task's name with ":").
* **Memory:** `CopyByte`/`CopyWord`/`CopyLong src,dest,n`,
  `FillMem address,length[,value]`, `MemFree` (AvailMem MEMF_PUBLIC).
* **System / hardware:** `CachesOn`/`CachesOff bitmask` (CacheControl,
  the old bits), `Reboot` (ColdReboot), `Lisa` (DENISEID into the low word
  of d0), `Timer` (CIA-A TOD), `KeyCode` (CIA-A SDR), `BlitterDone`,
  `WaitBlitter`, `Min`/`Max(a,b)` (signed), `GTTagValue([tag])`
  (`$80080000` + tag, inline).

init opens dos.library (version 0) and allocates the tables; finit
makes the directory the first ChangeDir replaced current again, unlocks
ChangeDir's lock, frees every bank and the tables and closes
dos.library.

## From RWE's source to the reference

The analysis (`.cache/rwelibs-analysis/versions/AMOSFunc.md`) was checked
against the disassembly of both binaries (`tail -c +33 FILE`,
`m68k-amigaos-objdump -D -b binary -m m68k:68000`):

* **BSS2 = func-library7 + the UnLock in MakeDir**
  (`BEQ oops:JSR _UnLock(a6):MOVEQ #-1,d0` after CreateDir), which
  func-library4–6 and the older OL/BUM binary have and func-library7 lost.
  Without it the source returns the directory's lock and never frees it;
  with it MakeDir returns -1 / 0 (as the BSS2 guide says). Taken over.
* **AB3 = BSS2 + three changes** of the AmiBlitz3 team (the code compared
  instruction by instruction, only these differ):
  * `ChangeDir`: `MOVE oldlock,d1` (a word: the high word of the old lock
    tested and passed to UnLock – func-library4–7 and BSS2) became
    `MOVE.L oldlock,d1`. Taken over: the previous ChangeDir lock is
    unlocked.
  * `DeviceName$` rewritten: the registers d1–d3/d7/a0/a4/a6 kept
    (MOVEM), dos base PC-relative, the task through `dol_Task` →
    `mp_SigTask` (the source: `dol_Task - 92`, assuming the port is a
    process's `pr_MsgPort`), `UnLockDosList` with the flags of
    `LockDosList` (13; the source unlocked with 12), FindDosEntry on the
    string at a3 (the source: d0, the same string), and a terminating 0
    behind the ":" (the source wrote none). Taken over.
  * the help text `(volumename$)` (source and BSS2: `volumename$`).

## C and assembler

All routines are C behind `BLITZ_THUNK`s that keep d1/a0/a1 – the
original changes most registers (d0–d7, a0–a2, a6 in places), the C build
keeps d1–d7/a0–a6 (statements d0 too, except the copy functions, whose d0
is the original's: the source, or 0 for a count of 0 or below). The
original's ALibJsr calls into its own routines (BLoad form 0: FileSize,
Reserve form 0, Start, BLoad form 1; Reserve form 0: NextBank, Reserve
form 1) are plain C calls. The string length of DeviceName$ is read below
a2 without changing it (DIFF as in all C ports,
`docs/libs/cmdlibs-port.md` "General"). The unused `_xorlong` of the
source (no table entry) is not ported.

Assembler remains in:

* the two inline forms of `GTTagValue` (`BLITZ_INLINE`, copied by the
  compiler: `MOVE.l #$80080000,d0` / `ADDI.l #$80080000,d0`);
* the CreateDir call of `MakeDir`: the original unlocks whatever
  dos.library leaves in **d1**, not the lock in d0 (BUG(orig) below,
  #496); the
  NDK inline gives no access to d1 after the call.

The error check of BLoad (`bloadchk`) is a C function that does nothing:
the original starts with an RTS; the Lock test behind it and its runtime
error "Unable to load file!" are never reached and not ported. Not a
defect (#485): the check is switched off on purpose, BLoad reports a
missing file with its result (the manual: "-1 for success, or 0 for
failure (not enough RAM, file does not exist)"); with the check a program
that tests the result would stop under the debugger.

## Fixed

Intended deviations from the original (`docs/libs/deviations.md`,
kind F):

* `EraseAll` (also finit and `BankLimit`) counts the whole long from
  maxbanks − 1 down to 0; the original counted with DBRA on the low word
  while the bank number is the whole long: with more than 65536 banks it
  freed only those down to the next multiple of 65536, and `BankLimit 0`
  ran through 65536 negative banks (#495).
* `CopyByte`/`CopyWord`/`CopyLong` and `FillMem` copy or fill nothing
  for a count of 0 (`FillMem` also for a negative length, as the copies
  do for a negative count); the original tested the count only after
  the first unit and copied or filled 2^32 units (#492).
* `Reserve(length)` returns -1 when every bank is in use; the original
  tested the sign of the length, not NextBank's result, and reserved
  "bank -1" (the entry before the start table is the last length, the
  one before the length table memlib's block header) (#498).
* `Reserve(length)` returns -1 for a negative length, as for its other
  failures (the manual: "the number of the bank ... or -1 for
  failure"); the original returned 0, the number of the first bank
  (#500).
* `NextBank` without an argument searches from bank 0 (the manual:
  "the first available bank"); the original used the routine of the form
  with an argument for both forms and started at whatever d0 held
  (#497). ripacklib's `LoadIFF`, which relied on that (form 0 with
  d0 = 51), calls the form with an argument.
* A bank number is 0 to maxbanks − 1, compared unsigned: `Start`,
  `Length`, `Erase` and `SwapBank` treat a negative number as a free
  bank, `Reserve(bank,…)` returns 0 for a bank outside (one compare
  before AllocMem), `NextBank` returns -1 for a start outside, and
  `FillMem`, `BLoad` and `BSave` take a negative number as an address
  (the manual's "bank/address"). The original compared signed, so a
  negative number indexed before the tables (`Start(-1)` gave the last
  bank's length, `Erase -1` freed it with memlib's header long as the
  size), and `Reserve(bank,…)` did not compare at all: a bank ≥ maxbanks
  wrote behind the start table and over another bank's start (#484).
* `ChangeDir` does not test CurrentDir's result, the previous current
  directory, which may be 0 (the boot volume's root): the new lock is
  remembered and the result is -1. The original returned 0 when the
  previous directory was 0, with the new lock already current but not
  remembered (never unlocked) (#490).
* `ChangeDir` keeps the current directory its first call replaces (the
  program's own, also 0); finit makes it current again, unless the
  program changed the directory itself since, then unlocks the last
  ChangeDir lock and clears it; `BankLimit` does not touch the
  directory (as #157 of freqlib: the start directory is restored at the
  end). The original kept no copy: finit and BankLimit unlocked the last
  ChangeDir lock while it was still the current directory, without
  clearing it (a second finit or BankLimit, or the next ChangeDir,
  unlocked it again); the Shell the program was started from was left
  with a freed lock (#491).
* `BLoad` opens the file before it touches a bank: a file that cannot be
  opened leaves the bank as it was. The original erased the bank and
  reserved it anew first, and left it so (a bank in use lost its
  contents and got the new length, a free bank stayed reserved) (#487).
* `BLoad` reports a failed read (Read's -1) with 0; the original tested
  Read's result for 0 only and returned -1 (success). A short read stays
  a success: a length beyond the file's end is a valid call (#488).
* `BLoad(name$)` returns -1 for every failure, as the manual says ("the
  number of the bank ... or -1 for failure"): an empty or missing file,
  no bank (Reserve fails: no memory or no free bank), and a failed load
  (the bank is erased again). The original returned 0, the number of the
  first bank, when Reserve failed, and the bank, left reserved with
  undefined contents, when the load failed (#486).
* `BSave(name$,bank,length)` writes at most the bank's length, as the
  manual says ("If length > bank length, then the length of the bank is
  saved instead"); the original wrote `length` bytes from the bank's
  start, whatever the bank's size (#489).
* `BankLimit n` returns the new number of banks, as `BankLimit` without
  an argument does (the manual's syntax:
  `[numbanks=]BankLimit[(maxbanks)]`); the original returned the address
  of the new start table, init's d0 (#483).

## BUG(orig), kept

(All in the AB3 binary; in the C file's head too.)

* `MakeDir` unlocks d1 after CreateDir, not the returned lock. The
  dos.library of OS 3.2 (the emulator's) has the lock in d1 too: the test
  sees the directory unlocked; other versions may leak it or unlock
  garbage (#496).
* `DeviceName$` takes the name from the string space (a3), not from d0;
  a ":" in front of an empty argument (the byte before it) is cleared
  (#493). The task is read through `dol_Task` without a check (a device
  without a handler task reads near address 0) (#494).
* `SaveExec`: a file it cannot create returns the file name's address; an
  empty list (first pointer 0) leaves the file open with only the header
  (#501); the header gets one size slot (first = last = 0), there is one
  HUNK_END behind all hunks, each RELOC32 block names its own hunk's
  index (#503); symbol and other hunks are skipped but counted (#502).
* `FindVolume` returns the DosList entry that FindDosEntry found, taken
  after UnLockDosList (an entry of a list the program no longer holds);
  the manual documents -1 for a volume that is present and 0 otherwise.
  As a truth value it works; a comparison with -1 never finds the
  volume (test `riamosfunclib-findvolume-result`, #1082).

Original behaviour that is not a defect:

* `SaveExec` stops at the node whose next is 0. The list is Exec-style:
  that node is the list header's tail (`lh_Tail`), not an item, so every
  item is written.
* init and BankLimit do not check globalloc's result: memlib raises
  "Out of Memory" itself.
* `Reserve(bank,…)` and `BLoad` into a bank erase a bank in use before
  the new allocation, also when that fails: documented ("If the bank
  does exist, Blitz will erase the bank from memory, and allocate a new
  one", node BLoad of the manual); the other order would need the memory
  of both banks at once, and a re-reservation that works now could fail
  when memory is short (#499). BLoad opens the file before it touches the
  bank (#487, fixed).
* BLoad's error check is switched off (#485, above).

## Test

One set per run (sysload), with its own memlib, banks and files
(`RAM:rwamos0`); observed as cases and compared with the expected data
`libs/tests/expected/rwe_riamosfunclib.txt` (recorded from the
original): d0, a3 and
the string area of DeviceName$, the banks' lengths and contents (where
the memory is defined), the memory of copies, fills and loads, the files
of BSave and SaveExec, the current directory after ChangeDir, and every
register the original keeps.

Covered: Min/Max (signed edges), both GTTagValue inlines (copied with an
RTS), the copy functions (counts 1..100, negative), KeyCode, Timer
(range, runs), Lisa (AGA: $F8, high word kept), BlitterDone, WaitBlitter,
MemFree, CachesOff/CachesOn (masks 0, EnableI, ClearI; the state restored
before every call); the banks in all forms (free, used, above the limit,
an erased bank in use, chip memory with MEMF_CLEAR, a failing
allocation – AllocMem replaced by a failing stub, also for a bank in use –,
a negative length, NextBank from used/free/limit/-1, FillMem
on banks, addresses and a free bank, SwapBank used/free, Erase used/free,
BankLimit get/60/50); MakeDir (new, existing, missing parent, the lock
given back), FileSize (file, directory, missing), BSave (bank, address
with/without length, free bank, bad path), BLoad in all five forms (new
bank, address, bank, bank in use, length, offset, chip memory; empty and
missing files, the bank left by a failed open), its error check, Rename,
ChangeDir (absolute, relative, missing), FindVolume, DeviceName$ (with and
without ":", volumes, devices, unknown, empty, d0 elsewhere), SaveExec (two
code hunks – relocations, memory flags, a length not a multiple of 4 –
and a symbol hunk; no list; no file; an empty list), EraseAll, finit.

Not covered: Reboot; counts of 0 (the original writes over all memory;
the bug-orig test `riamosfunclib-count-zero`) and bank numbers outside
the tables (the original writes outside the test's memory; the bug-orig
test `riamosfunclib-bank-range`); BankLimit 0 or above 65536 (the
bug-orig test `riamosfunclib-eraseall-dbra`); a
BankLimit after a ChangeDir; CurrentDir returning 0 (the bug-orig test
`riamosfunclib-changedir-currentdir-0`).

**Loader pitfall:** sysload resolves an ALibJsr placeholder to the table
address with bit 0 (Amiga mode only) set; the compiler instead makes a
stub (`tokecode`/`cutejsr`). The original's ALibJsr calls of FileSize and
BLoad then jump to an odd address. `libs/tests/rwe/amosfunc.h`
(`amosfunc_fixjsrs`) clears that bit in every resolved `JSR abs.l` after
`load_set` – needed by every test that loads this library (also the
rigfxlib and ripacklib tests: their BSave/BLoad calls).

**Failing allocations:** a Reserve of a huge length (AllocMem fails for
real) runs exec's low-memory handlers; their expunges crashed the
emulated system a little later (a black screen, depending on timing).
The test lets AllocMem fail with a stub instead (SetFunction under
Forbid, as in the other RWE tests).
