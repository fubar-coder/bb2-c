# ciatrklib (56): the CIA Protracker library

CIA Protracker Library V1.02b (Crossbones/Binary Assault 1995, from Neil
O'Rourke's tracker library 1.3). It plays Protracker modules (M.K., 31
samples, 4 voices) with the Protracker 2.3A CIA play routine (Peter
"Crayon" Hanning, Mushroom Studios, released as "Public Domain",
`archive/README.md`). The library's changes to the routine: the
busy-wait fix of Delirium Softdesign (`mt_PlaySamples`: waits on
DMACONR/INTREQR instead of DBF loops), a master volume
(`TrackerVolume`), a channel mask (`ChangeTrackerMask`: a masked voice
writes its "Paula registers" into a 60-byte `fake_channel`) and effect
8xx as the "tracker event" (`GetTrackerEvent`, `CheckTrackerEvent`,
`WaitTrackerEvent`).

Modules live in a table of 10 slots {address, length, flag} (flag 0:
given by `DecodeModule`, 1: loaded, 2: copied to chip memory; the error
checks allow slots 0–8). init installs the CIA interrupt (`SetCIAInt`:
`mt_music` at 50 Hz, Fxx ≥ 32 reprograms the timer), finit stops and
removes it; `StartTracker` removes and installs it again before
`mt_init`. After `ChangeTrackerVBlank` the program calls `PlayTracker`
(one tick) itself.

* **Source:** `rwelibsrc/ptlib/CIATrackerLib1.bb` (as TED shows it:
  `build/amiga/rwelibsrc/ptlib/CIATrackerLib1.bb`). `#InHouse=0`: the
  three in-house commands (`TrackerLine`, `TrackerPatternToMem`,
  `TrackerSongPos`) are not part of the library and not in the C file.
* **Reference:** the reference build of that source (`refbuild`,
  `build/amiga/libs/userlibs/rwe/ref/ciatrklib.obj`); the CD binaries are
  1.02a (OL) and 1.03 (BUM9B) (`docs/rwelibs/README.md` §3.2).
  Declarations equal (ctest `rwe_declarations_ciatrklib`).
* **C file:** `libs/userlibs/rwe/ciatrklib.c`; test
  `libs/tests/rwe/ciatrklib.c` (snitch case "rwe ciatrklib").

## C and assembler

Everything is C behind `BLITZ_THUNK`s (they keep d0–d1/a0–a1 but the
result; the original keeps fewer). The CIA interrupt's `is_Code` is a
thunk to the same C function as `PlayTracker`. Paula, the CIAs and
VHPOSR are volatile accesses of the original's width (the long write of
`MOVE.L n_replen(A6),4(A5)` in `mt_DoRetrig` stays a long: AUDxLEN and
AUDxPER). DIVU/DIVS/MULU are `bl_divu_w`/`bl_divs_w`/`bl_mulu_w`.

The data are three structs with the original's layout: the library's
variables (`pause_status` … `modtable`, 110 bytes), the CIA part
(`RealTempo` … `musintname`, 110 bytes, with the `struct Interrupt`) and
the play routine's block (`mt_FunkTable` … `nor_LstEvt`, 1584 bytes).
The layout is needed: the routine reads behind `mt_PeriodTable` into
`mt_chan1temp` and behind `mt_SampleStarts` (below), and the test finds
the blocks in both builds. `master_mask` is a word whose first byte the
original addresses with BSET/BTST/BCLR; the C struct has the two bytes.

The pt23f changelog (8bitbubsy, only the list of fixes was used as a
checklist) names the known bugs of the 2.3 routine: `mt_LowMask` not
initialised, the tremolo, the arpeggio table, `mt_init` writing out of
bounds. They are kept here (below).

## BUG(orig), kept

* `SetCIAInt` loads the ICR bit for `AddICRVector` from the addresses 1
  (ciaa timer A) and 2 (ciab timer B): `MOVE.l %00000001,D0` without
  `#`, a long read from an odd address (an address error on a 68000). In
  Amiberry (A1200, ExecBase in Zorro III memory) the long at 1 is $40
  and at 2 $4000: `AddICRVector` accepts ciaa with "bit" $40, timer A of
  ciaa is programmed and started, but no interrupt ever calls
  `mt_music` – in CIA mode the library is silent there (orig and C
  alike). `ResetCIAInt` removes the vector with bit 0 (its `MOVEQ #1,D0`
  is overwritten) – in Amiberry the system's timer A vector of ciaa (a
  first version of the test that called the real resources made the
  later tests in the same session hang) – and toggles
  the filter (`BCHG #1,$bfe001`).
* `LoadTrackerModule` has no `!libs`: the dos calls go through whatever
  a6 holds. Length = file size + 3 as a **byte** add (a size with the low
  byte ≥ $FD loses the carry: $10FE gives $1001, and only that much is
read); the chip memory is not
  checked; after a Read error it unlocks the lock that `OpenFromLock`
  took over. The FileInfoBlock is freed only on success (the runtime
  errors leave it).
* `DecodeModule` of a module in chip memory (address < $200000, signed)
  keeps the address (length 1, flag 0); from other memory it copies to
  chip memory with the size + 3 as a **word** add; without memory the
  slot keeps the old address with flag 2 and the new length (a later
  `FreeTrackerModule` frees memory that was never allocated).
  `FreeTrackerModule` does not stop a module that plays from the slot.
* `StartTracker` of an empty slot returns the slot number (the MOVEM
  restores d0 over the 0). `ModuleToChip` is an RTS: it returns its
  argument. `ModulePatterns` returns the song length (byte 950).
  `CheckTrackerID` returns the long at **address 0** (`MOVE.l 0,d0`) for
  "no M.K./M!K!" and an empty slot. `GetModuleName$` of an empty slot
  returns slot × 10 as the length and writes nothing (not even the 0).
* `WaitTrackerEvent` calls WaitTOF (−270) through whatever a6 holds (no
  `!libs`); `GetTrackerEvent` changes only d0's low byte,
  `CheckTrackerEvent` its low byte (no event) or word (−1).
* `ModulePositionJump n` jumps to position n + 1 (the next tick's
  `mt_NextPosition` increments it).
* `TrackerVolume 0` and `64` do not change the volume of the notes that
  play (only 0 < volume < 64 calls `Change_Volume_Levels`).
* `mt_init` compares the module **length** with the sample addresses
  (`CMP.l a2,a4`): it clears the first long of a sample only when the
  length is greater than its address (for a loaded module practically
  never). Its scan of the song positions (also `_modtochip`'s) reads
  positions 0–126 (SUBQ for a new maximum, DBRA otherwise; a new maximum
  at 126 reads ~64 KB more); pattern numbers ≥ $80 never count (BGT).
* Play routine (Protracker 2.3A): `mt_LowMask` starts as 0 – the first
  1xx/2xx before any E1x/E2x does not slide; 9xx with a note applies the
  offset twice to `n_start`/`n_length` (`mt_SetRegs` calls
  `mt_CheckMoreEfx`, then `mt_SetPeriod` runs it again; the hardware got
  the first one); the tremolo's
  ramp-down tests the **vibrato** position; `mt_SetPeriod` without a
  matching period takes entry 37, the arpeggio reads up to 15 words
  behind the finetune's table (for finetune 15 into `mt_chan1temp`); a
  sample number above 31 reads behind `mt_SampleStarts`; E8x
  (Karplus-Strong) with a loop length of 0 rewrites 64 KB; the repeat
  offset is repeat × 2 as a word; F00 stops the module (`mt_end`);
  effect 8 is seen only on ticks other than 0 (never at speed 1).

## DIFF and not reproducible

No DIFF in the results. A finetune byte above 15 in a sample header
(`n_finetune` × 72) reads behind the play routine's data – the
original's own code there, other memory in the C build. The C build
keeps a3 in `FreeTrackerModule` (the original's `LEA modtable,a3`
changes the string pointer) and generally more registers.

## Test

Paula cannot be read, so the test compares the library's state: the
three blocks (pointers into the modules relative to each set's module),
`fake_channel`, DMACONR's audio bits, the filter bit and the module bytes
after every call. The modules are built in memory (fixed random seed: 6
patterns, 9 positions, 31 samples with loops, finetunes 0–14, a volume
above 64, a sample of length 0; every effect but F00, E5F and sample
numbers above 31).

* Phase A (CIA, each set alone): `AddICRVector`/`RemICRVector` of both
  CIA resources are replaced (SetFunction) by stand-ins that record the
  calls – the real ones with the original's bits break the system's
  ciaa timer A (above); ciaa's stand-in fails, so the library takes the
  ciab path (timer B, free). Compared: the calls (resource, bit = the
  long at 1 / 2, the interrupt), init's state (CIAAbase, TimerFlag,
  CIAAaddr, TimerValue, the timer's start bit), the interrupt server
  called by hand (`is_Code` with `is_Data`: row 0 with F40 sets
  `RealTempo` 64 and the timer, the event of tick 7), `StartTracker`
  (removes and adds again), `WaitTrackerEvent` with a WaitTOF that runs
  the server, `ChangeTrackerVBlank` (CIAAbase 0, the timer stopped, the
  filter toggled, `RemICRVector` with bit 0).
* Phase B (both sets call by call): `DecodeModule` (chip; fast memory:
  the copy), the information functions also on an empty slot, the error
  checks without an error, `mt_init`'s clearing with a slot length of
  $7FFFFFFF, ~540 `PlayTracker` ticks with the masks $F, 5 and 0,
  `TrackerVolume` (0, 1, 32, 63, 64, 65, garbage in the upper word),
  `ModulePositionJump`, `GetModulePosition`, `PauseModule`,
  Get/`CheckTrackerEvent`, `WaitTrackerEvent` (its WaitTOF plays a
  tick), `StopTracker`, `LoadTrackerModule` (a6 = DOSBase; sizes 8768
  and $10FE), `FreeTrackerModule`, `ModuleToChip`, finit; memlib's memory
  all back: the balance of what the test task allocated and freed from
  the start to the end (`rwe_mem_start`/`rwe_mem_check` of `rwemem.h`),
  not `AvailMem()`, which also counts what other tasks allocate
  meanwhile.

Not covered: the runtime errors (missing file, failing error checks), a
Read error, failing allocations, sample numbers above 31, finetune 15
and above (they read behind the tables into pointers or behind the
data), E8x with loop length 0.
