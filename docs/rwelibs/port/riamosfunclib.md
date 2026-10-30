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
  number). A number below `maxbanks` is a bank, anything else an address
  (FillMem, BLoad, BSave).
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
unlocks ChangeDir's lock, frees every bank and the tables and closes
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
is the original's: the source, or 0 for a negative count). The
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
the original starts with an RTS (BUG(orig), #485); the Lock test behind it and
its runtime error "Unable to load file!" are never reached and not
ported.

## BUG(orig)

(All still in the AB3 binary; in the C file's head too.)

* No range checks: a negative bank number counts as a bank and indexes
  before the tables (Start, Length, Erase, Reserve, BLoad, FillMem,
  SwapBank, NextBank); `Reserve(bank,…)` with bank ≥ maxbanks writes
  behind the start table (#484). `Reserve(length)` tests the sign of the
  length, not NextBank's result: with every bank in use it reserves
  "bank -1" (the entry before the start table is the last length, the
  one before the length table memlib's block header) (#498).
* `Reserve(length)` returns 0 for a negative length – also a bank
  number (#500) – and -1 if AllocMem fails. A bank in use is erased
  before the new allocation, also when that fails (#499).
* `NextBank` without an argument starts at whatever d0 holds (both forms
  share the routine) (#497).
* `EraseAll` counts with DBRA on the low word while the bank number is
  the whole long: with more than 65536 banks it frees only those from
  maxbanks − 1 down to the next multiple of 65536, the banks below stay
  allocated; `BankLimit 0` runs through 65536 negative banks (#495).
* `CopyByte`/`CopyWord`/`CopyLong` and `FillMem` with a count of 0 copy
  or fill 2^32 units (#492).
* `BLoad(name$)` returns the bank also when reading fails (the bank stays
  reserved), -1 for an empty or missing file and 0 when no bank is free
  (#486); the other forms leave a bank they reserved for a file that
  cannot be opened (#487), and treat Read's -1 as success (#488). A bank
  given to BLoad is erased and reserved anew with the file's length (or
  the given one) (#499). The error check never checks (RTS first)
  (#485).
* `BSave(name$,bank,length)` writes `length` bytes from the bank's start,
  whatever the bank's size (#489).
* `MakeDir` unlocks d1 after CreateDir, not the returned lock. The
  dos.library of OS 3.2 (the emulator's) has the lock in d1 too: the test
  sees the directory unlocked; other versions may leak it or unlock
  garbage (#496).
* `ChangeDir` keeps no copy of the program's own current directory:
  finit and BankLimit unlock the last ChangeDir lock while it is still the
  current directory, and do not clear it (a second finit or BankLimit, or
  the next ChangeDir, unlocks it again) (#491). If CurrentDir returns 0,
  the new lock is current but not remembered and the result is 0 (#490).
* `BankLimit n` returns the address of the new start table (init's d0)
  (#483).
* `DeviceName$` takes the name from the string space (a3), not from d0;
  a ":" in front of an empty argument (the byte before it) is cleared
  (#493). The task is read through `dol_Task` without a check (a device
  without a handler task reads near address 0) (#494).
* `SaveExec`: a file it cannot create returns the file name's address; an
  empty list (first pointer 0) leaves the file open with only the header
  (#501); the header gets one size slot (first = last = 0), there is one
  HUNK_END behind all hunks, each RELOC32 block names its own hunk's
  index (#503); symbol and other hunks are skipped but counted (#502).

Original behaviour that is not a defect:

* `SaveExec` stops at the node whose next is 0. The list is Exec-style:
  that node is the list header's tail (`lh_Tail`), not an item, so every
  item is written.
* init and BankLimit do not check globalloc's result: memlib raises
  "Out of Memory" itself.

## Test

Both builds side by side (sysload), each with its own memlib, banks and
files (`RAM:rwamos0`/`1`, names of the same length); compared: d0, a3 and
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

Not covered: Reboot; counts of 0 and bank numbers outside the tables
(they write outside the test's memory); BankLimit 0 or above 65536; a
ChangeDir lock that is current at finit/BankLimit (the test restores the
directory first); CurrentDir returning 0.

**Loader pitfall:** sysload resolves an ALibJsr placeholder to the table
address with bit 0 (Amiga mode only) set; the compiler instead makes a
stub (`tokecode`/`cutejsr`). The original's ALibJsr calls of FileSize and
BLoad then jump to an odd address. `libs/tests/rwe/amosfunc.h`
(`amosfunc_fixjsrs`) clears that bit in every resolved `JSR abs.l` after
`load_sets` – needed by every test that loads this library (also the
rigfxlib and ripacklib tests: their BSave/BLoad calls).

**Failing allocations:** a Reserve of a huge length (AllocMem fails for
real) runs exec's low-memory handlers; their expunges crashed the
emulated system a little later (a black screen, depending on timing).
The test lets AllocMem fail with a stub instead (SetFunction under
Forbid, as in the other RWE tests).
