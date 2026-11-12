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
the blocks in either build. `master_mask` is a word whose first byte the
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
  the filter (`BCHG #1,$bfe001`) (#450, #448, #449).
* `LoadTrackerModule` has no `!libs`: the dos calls go through whatever
  a6 holds (#443). Length = file size + 3 as a **byte** add (a size with
  the low byte ≥ $FD loses the carry: $10FE gives $1001, and only that
  much is read) (#442); the chip memory is not checked (#440; not a bug in practice: a failing globalloc never returns, memlib ends the program with "Out of Memory!"); after a
  Read error it unlocks the lock that `OpenFromLock` took over (#444).
  The runtime errors after `Lock` leave the lock allocated, and the
  FileInfoBlock is freed only on success (#441).
* `DecodeModule` of a module in chip memory (address < $200000, signed)
  keeps the address (length 1, flag 0) (#432); from other memory it
  copies to chip memory with the size + 3 as a **word** add (#434);
  without memory the slot keeps the old address with flag 2 and the new
  length (a later `FreeTrackerModule` frees memory that was never
  allocated) (#433; not a bug in practice: a failing globalloc never returns, memlib ends the program with "Out of Memory!"). `FreeTrackerModule` does not stop a module that
  plays from the slot (#436).
* `StartTracker` of an empty slot returns the slot number (the MOVEM
  restores d0 over the 0) (#451). `ModuleToChip` is an RTS: it returns
  its argument (#446). `ModulePatterns` returns the song length (byte
  950) (#445). `CheckTrackerID` returns the long at **address 0**
  (`MOVE.l 0,d0`) for "no M.K./M!K!" and an empty slot (#430).
  `GetModuleName$` of an empty slot returns slot × 10 as the length and
  writes nothing (not even the 0) (#437).
* `WaitTrackerEvent` calls WaitTOF (−270) through whatever a6 holds (no
  `!libs`) (#453); `CheckTrackerEvent` changes d0's low byte (no event)
  or word (−1) (#431). (`GetTrackerEvent` also changes only d0's low
  byte; it is declared `#byte`, so that is no defect.)
* `ModulePositionJump n` jumps to position n + 1 (the next tick's
  `mt_NextPosition` increments it) (#447).
* `TrackerVolume 0` and `64` do not change the volume of the notes that
  play (only 0 < volume < 64 calls `Change_Volume_Levels`) (#452).
* `mt_init` compares the module **length** with the sample addresses
  (`CMP.l a2,a4`): it clears the first long of a sample only when the
  length is greater than its address (for a loaded module practically
  never) (#438). Its scan of the song positions (also `_modtochip`'s)
  reads positions 0–126 (SUBQ for a new maximum, DBRA otherwise; a new
  maximum at 126 reads ~64 KB more); pattern numbers ≥ $80 never count
  (BGT) (#477).
* Play routine (Protracker 2.3A): `mt_LowMask` starts as 0 – the first
  1xx/2xx before any E1x/E2x does not slide (#478); 9xx with a note
  applies the offset twice to `n_start`/`n_length` (`mt_SetRegs` calls
  `mt_CheckMoreEfx`, then `mt_SetPeriod` runs it again; the hardware got
  the first one) (#473); the tremolo's ramp-down tests the **vibrato**
  position (#482); `mt_SetPeriod` without a matching period takes entry
  37 (#481), the arpeggio reads up to 15 words behind the finetune's
  table (for finetune 15 into `mt_chan1temp`) (#474); a sample number
  above 31 reads behind `mt_SampleStarts` (#480); E8x (Karplus-Strong)
  with a loop length of 0 rewrites 64 KB (#439); the repeat offset is
  repeat × 2 as a word (#479); `mt_SampleOffset` compares the offset
  with `n_length` signed (`CMP.W`, `BGE`): 9xx on a sample of $8000 words
  or more sets the length to 1 (#472); effect 8 is seen only on ticks
  other than 0 (never at speed 1) (#435).
* F00: that it stops the module (`mt_SetSpeed`: `BEQ mt_end`) is normal
  Protracker behaviour. The defect is that `mt_end` loads a0 with
  `$DFF000` (for its `CLR.W $A8(A0)` …), and in `mt_GetNewNote` a0 is
  the pattern data: after an F00 on voice 1–3 the voices behind it read
  their notes with `MOVE.L 0(A0,D1.L),(A6)` from the custom chip
  registers at `$DFF000` + the pattern offset (registers that change by
  the moment or are write-only) and play them in the same row. The C
  build keeps that pointer in `patdata`, which `mt_end` sets to `$DFF000`
  (checked against the reference build: `LEA $DFF000,A0` in `mt_end`,
  nothing else changes a0 between the four `mt_PlayVoice` calls) (#475).

## DIFF and not reproducible

No DIFF in the results. A finetune byte above 15 in a sample header
(`n_finetune` × 72) reads behind the play routine's data – the
original's own code there, other memory in the C build (#476). The C build
keeps a3 in `FreeTrackerModule` (the original's `LEA modtable,a3`
changes the string pointer) and generally more registers.

## Test

Each run loads one library set (the C build or the original); the
scenario writes its observations as cases, compared with the expected
data `libs/tests/expected/rwe_ciatrklib.txt` recorded from the original.
Paula cannot be read, so the test observes the library's state: the
three blocks (pointers into the modules relative to the module),
`fake_channel`, DMACONR's audio bits, the filter bit and the module bytes
after every call. The modules are built in memory (fixed random seed: 6
patterns, 9 positions, 31 samples with loops, finetunes 0–14, a volume
above 64, a sample of length 0; every effect but F00, E5F and sample
numbers above 31).

F00 has a module of its own (sample 1; row 0 notes and F01, row 1 F00 on
voice 1, 2 or 3 and nothing behind it), played to the stop for each
voice. For these calls the test patches the `$DFF000` of `mt_end` in the
code of the loaded set (`LEA $DFF000,A0` before `CLR.W $A8(A0)` in the
original, `MOVE.L #$DFF000,patdata` in the C build; exactly one is
required) to a RAM copy of the registers, filled with a note
(period 428, sample 1, C20), and restores it afterwards. Only this one
address is redirected: the routine polls VHPOSR, DMACONR and INTREQR,
which a RAM copy would never change. Observed besides the blocks: the
voices behind the F00 hold the note from the copy. DMACONR in this phase
is `not captured` (the original writes DMACON into the hardware, the C
build into its RAM copy); the audio DMA is switched off before the phase
and after it.

* Phase A (CIA): `AddICRVector`/`RemICRVector` of both
  CIA resources are replaced (SetFunction) by stand-ins that record the
  calls – the real ones with the original's bits break the system's
  ciaa timer A (above); ciaa's stand-in fails, so the library takes the
  ciab path (timer B, free). Observed: the calls (resource, bit = the
  long at 1 / 2, the interrupt), init's state (CIAAbase, TimerFlag,
  CIAAaddr, TimerValue, the timer's start bit), the interrupt server
  called by hand (`is_Code` with `is_Data`: row 0 with F40 sets
  `RealTempo` 64 and the timer, the event of tick 7), `StartTracker`
  (removes and adds again), `WaitTrackerEvent` with a WaitTOF that runs
  the server, `ChangeTrackerVBlank` (CIAAbase 0, the timer stopped, the
  filter toggled, `RemICRVector` with bit 0).
* Phase B (call by call): `DecodeModule` (chip; fast memory:
  the copy), the information functions also on an empty slot, the error
  checks without an error, `mt_init`'s clearing with a slot length of
  $7FFFFFFF, ~540 `PlayTracker` ticks with the masks $F, 5 and 0,
  `TrackerVolume` (0, 1, 32, 63, 64, 65, garbage in the upper word),
  `ModulePositionJump`, `GetModulePosition`, `PauseModule`,
  Get/`CheckTrackerEvent`, `WaitTrackerEvent` (its WaitTOF plays a
  tick), `StopTracker`, F00 on voices 1–3 (above), `LoadTrackerModule` (a6 = DOSBase; sizes 8768
  and $10FE), `FreeTrackerModule`, `ModuleToChip`, finit; memlib's memory
  all back: the balance of what the test task allocated and freed from
  the start to the end (`rwe_mem_start`/`rwe_mem_check` of `rwemem.h`),
  not `AvailMem()`, which also counts what other tasks allocate
  meanwhile.

Not covered: the runtime errors (missing file, failing error checks), a
Read error, failing allocations, sample numbers above 31, finetune 15
and above (they read behind the tables into pointers or behind the
data), E8x with loop length 0.
