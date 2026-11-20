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

## Fixed

Intended deviations from the original (`docs/libs/deviations.md`,
kind F):

* `ResetCIAInt` (`ChangeTrackerVBlank`, `StartTracker` in CIA mode, the
  end of the program) leaves the audio filter / power LED (bit 1 of
  CIA-A PRA) as it is; the original toggled it with a
  `BCHG #1,$bfe001` left in (#449).
* `StartTracker` of an empty slot returns False (0); the original set d0
  to 0 and then restored the registers with `MOVEM`, which put the slot
  number back into d0 ("True" although nothing plays) (#451; library
  test `rwe_ciatrklib`, the case `StartTracker|d0|slot 3`).
* `GetModuleName$` of an empty slot is the empty string; the original
  left with d0 = the slot's table offset (slot × 10) and wrote nothing,
  not even the 0: a string of slot × 10 undefined characters (#437;
  library test `rwe_ciatrklib`, the cases `GetModuleName$|...|slot 3`).
* `CheckTrackerID` of "no M.K./M!K!" and of an empty slot is False (0);
  the original returned the long at **address 0** (`MOVE.l 0,d0`)
  (#430; test `ciatrklib-checkid-address0`).
* `CheckTrackerEvent` without a new event is False (0); the original
  loaded only the low byte of d0 with the last event's value (#431;
  library test `rwe_ciatrklib`, the case `CheckTrackerEvent|d0||2`).
* `LoadTrackerModule` loads the whole file: the length is the file size
  + 3; the original added the 3 as a byte, so a size with the low byte
  $FD-$FF lost the carry ($10FE gave $1001, and only that much was read)
  (#442; library test `rwe_ciatrklib`, the cases
  `LoadTrackerModule|...|slot 5, 4350 bytes`).
* After a Read error `LoadTrackerModule` unlocks no lock a file handle
  owns; the original opened the file with `OpenFromLock` (the handle takes
  the lock over, `Close` frees it) and unlocked that lock a second time
  (#444; test `ciatrklib-load-readerror-unlock`).
* `LoadTrackerModule` opens the file by its name (`Open`, `MODE_OLDFILE`)
  while it holds the lock of `Lock`/`Examine`, and unlocks the lock after
  `Close`: one path on every Kickstart. The original called
  `OpenFromLock`, a dos.library V36 function, on the program's
  dos.library; on Kickstart 1.x the offset lies behind the jump table and
  the program crashed at `LoadTrackerModule` (#1146; test
  `ciatrklib-v36-calls` on the a500, a500-204 and a1200). The open is
  checked: when it fails (a directory, which can be locked and examined
  but not opened) the slot is freed, the lock unlocked and the result is
  `IoErr()`; the original passed the handle to `Read` untested (on the
  a1200 its `OpenFromLock` of a directory in `RAM:` gives the same
  results; regression test `ciatrklib-load-open-fails`, a500 and a1200).
* `LoadTrackerModule` is declared with `!libs {#doslib,#la6}`: the
  compiler loads DOSBase into a6; the original's `!libs` was commented
  out and the dos calls went through whatever a6 held (#443; test
  `ciatrklib-load-no-libs`).
* `WaitTrackerEvent` is declared with `!libs {#graphicslib,#la6}`: the
  compiler loads GfxBase into a6; the original's `!libs` was commented
  out and WaitTOF went through whatever a6 held (#453; test
  `ciatrklib-waitevent-no-libs`).
* `DecodeModule` compares the module's address with $200000 unsigned: a
  module at $80000000 or above is copied to chip memory; the original's
  `BLT` took such an address for chip memory (#432; test
  `ciatrklib-decode-chip-signed`).
* `DecodeModule`'s copy to chip memory has the size + 3; the original
  added the 3 as a word, so a size with a low word of $FFFD-$FFFF was
  copied 64 KB short (#434; test `ciatrklib-decode-size-word-add`).
* `ModulePatterns` returns the number of patterns (the highest pattern
  number of the song positions + 1, from `mt_init`'s scan), as its
  documentation says; the original returned the song length (byte 950)
  (#445; library test `rwe_ciatrklib`, the cases `ModulePatterns|d0|...`).
* `ModulePositionJump n` continues at song position n: it stores n − 1,
  as effect Bxx does; the original stored n, which the next tick's
  `mt_NextPosition` incremented (#447; library test `rwe_ciatrklib`,
  `GetModulePosition|d0||2` and the cases of the ticks after it).
* `TrackerVolume 0` and `64` change the volume of the notes that play,
  as every other volume does; the original called `Change_Volume_Levels`
  only for 0 < volume < 64 (#452; library test `rwe_ciatrklib`, the
  library's variables after `TrackerVolume $0`/`$40`).
* `ResetCIAInt` removes the interrupt with the bit of its timer (1 for
  timer B of ciab, 0 for timer A); the original overwrote the
  `MOVEQ #1,D0` with a `MOVEQ #0,d0` at `RemInt` and always removed bit 0
  (#448; library test `rwe_ciatrklib`, the cases
  `ChangeTrackerVBlank|RemICRVector bit||...`).
* `SetCIAInt` passes the interrupt bits 0 (timer A of ciaa) and 1 (timer
  B of ciab) to `AddICRVector`; the original loaded the longs at the
  **addresses** 1 and 2 (`MOVE.l %00000001,D0` without `#`): an address
  error on a 68000; in Amiberry (A1200, ExecBase in Zorro III memory) the
  long at 1 is $40, `AddICRVector` accepted it, but no interrupt ever
  called `mt_music`, so in CIA mode nothing played (#450; test
  `ciatrklib-setciaint-icr-bit`, library test `rwe_ciatrklib`, the cases
  `init (CIA)|AddICRVector ...`).
* With runtime error checking, `FreeTrackerModule` of the loaded or
  copied module that plays is the runtime error "Tracker: Stop the module
  before freeing it." (the error check `_freecheck`); the original's
  `_slotcheck` let it be freed, and the play routine went on reading the
  freed memory. Without the debugger it is freed as before: freeing the
  module that plays is a programming error (the documentation of
  `LoadTrackerModule`: "The results are unpredictable") (#436; test
  `ciatrklib-free-while-playing`).
* E8x (Karplus-Strong) on a voice without a loop (loop length 0) does
  nothing; the original's `DBRA` counter was $FFFE and it rewrote 64 KB
  from the loop start, from address 0 on a voice that played no sample
  yet (#439; test `ciatrklib-karplus-loop0`).
* The tracker event (8xx) is also seen on the row's own tick
  (`mt_CheckMoreEfx`): at speed 1 it comes at all, at other speeds on
  its row; the original saw it only in `mt_CheckEfx`, on the ticks
  between the rows (#435; library test `rwe_ciatrklib`, the play
  routine's state of the ticks with an event).
* `mt_init` clears the first long of every sample that lies inside the
  module, as Protracker does (a sample without a loop then repeats two
  zero bytes); the end is the slot's address + length, for a module
  `DecodeModule` left in place (length 1) the end of its samples. The
  original compared the module's **length** with the sample addresses
  (`CMP.l a2,a4`) and practically never cleared them: every sample
  without a loop ended in a loop of its first two bytes (#438; test
  `ciatrklib-init-length-vs-address`, library test `rwe_ciatrklib`, the cases `...|module bytes|...` after
  `StartTracker` of slot 0).
* Play routine: `mt_LowMask` starts as `$FF`, so the first 1xx/2xx
  slides by its whole parameter; the original started it as 0, and the
  first 1xx/2xx before any E1x/E2x did not slide (#478; library test
  `rwe_ciatrklib`, the case `mt_LowMask 0` and the ticks of the first
  1xx).
* Play routine: the tremolo's ramp-down waveform (E71) takes its half
  from the sign of the tremolo position; the original tested the
  **vibrato** position (#482; library test `rwe_ciatrklib`, the ticks of
  the random 7xy after an E7x).
* Play routine: `mt_SetPeriod` gives a note period below the table
  (1–112) the last entry of the finetune's row; the original compared 37
  entries and took entry 37, the second of the next row (for finetune −1
  the effect word of voice 1) (#481; bug-orig test
  `pt23a-setperiod-entry37-ciatrklib`).
* Play routine: 9xx with a note applies the sample offset once to
  `n_start`/`n_length`; the original's `mt_SetRegs` called
  `mt_CheckMoreEfx` and then `mt_SetPeriod`, which ran it again (Paula got
  the offset once, the voice twice: a retrig, a note delay or a following
  note without a sample number started at the doubled offset) (#473;
  bug-orig test `pt23a-9xx-twice-ciatrklib`).
* Play routine: `mt_SampleOffset` compares the offset with `n_length`
  unsigned, so 9xx on a sample of $8000 words (64 KB) or more moves the
  start; the original compared signed (`CMP.W`, `BGE`) and set the
  length to 1 (#472; bug-orig test `pt23a-9xx-signed-length-ciatrklib`).
* Play routine: the loop start of a sample is start + repeat × 2 doubled
  as a long; the original doubled the repeat as a word (`ASL.W`), so
  loops from 32768 words (64 KB) on started 64 KB too early (#479;
  bug-orig test `pt23a-repeat-word-ciatrklib`).
* Play routine: the arpeggio compares the 36 entries of the finetune's
  row and plays at most the row's last entry; the original compared 37
  and read up to 15 words behind the row – the next finetune's row (a
  low note), for finetune −1 `mt_chan1temp` (#474; bug-orig test
  `pt23a-arpeggio-overrun-ciatrklib`, library test `rwe_ciatrklib`, the
  random arpeggios on masked voices).
* Play routine: after an F00 on voice 1–3 (it stops the module, as in
  every Protracker) the voices behind it read their notes from the
  pattern. The original's `mt_end` loads a0 with `$DFF000` (for its
  `CLR.W $A8(A0)` …), and in `mt_GetNewNote` a0 is the pattern data: the
  voices behind read their notes with `MOVE.L 0(A0,D1.L),(A6)` from the
  custom chip registers at `$DFF000` + the pattern offset and played
  them. The C build keeps that pointer in `patdata`, which `mt_end` no
  longer changes (#475; library test `rwe_ciatrklib`, the F00 phase:
  the cases `voice behind F00: note from $DFF000 + offset`).
* `mt_init`'s scan of the song positions (also `_modtochip`'s, the size
  of `DecodeModule`'s copy) reads all 128 positions with one DBRA per
  byte and compares unsigned; the original read positions 0–126 (SUBQ
  for a new maximum, DBRA otherwise; a new maximum at 126 read ~64 KB
  more), and pattern numbers ≥ $80 never counted (BGT) (#477; bug-orig
  test `pt23a-init-pattern-scan-ciatrklib`).
* `mt_init` sets the sample number of every note above 31 to 0 in the
  patterns (once, instead of a test per note; the module is changed in
  memory; the notes inside the module when the slot's length is known);
  the original's `mt_PlayVoice` took the start of such a sample from
  behind `mt_SampleStarts` (32: 0, 33: `mt_SongDataPtr`, …) and its
  header from the position table and the patterns (#480; bug-orig test
  `pt23a-samplenum-over-31-ciatrklib`).
* `mt_init` masks the finetune byte of the 31 sample headers to 0–15
  (the module is changed in memory); the original took it unmasked, and
  every period lookup (`n_finetune` × 72) of a byte above 15 read behind
  the play routine's data – its own code (#476; bug-orig test
  `pt23a-finetune-above-15-ciatrklib`).
* `PauseModule` pauses only a module that plays (`mt_Enable` set) and
  continues only the module it paused itself; `StartTracker`,
  `StopTracker`, `ChangeTrackerVBlank` and freeing the paused module's
  slot (`FreeTrackerModule`, also `LoadTrackerModule` or `DecodeModule`
  into that slot) end the pause. The original toggled its own state
  (`pause_status`) whatever happened in between: after a pause, the
  next `PauseModule` did not pause a module started again, and played a
  stopped module or the module of a freed slot (the freed memory)
  again; two calls after a `StopTracker` played the stopped module
  again (#1076; tests `ciatrklib-pause-state-kept`,
  `ciatrklib-pause-after-stop`). Two consequences are accepted:
  `StopTracker` followed by two `PauseModule` no longer continues the
  module, and a module that stopped itself (F00) cannot be continued
  that way. Nothing changes on the play routine's tick.

## BUG(orig), kept

* `LoadTrackerModule` does not check the chip memory (#440; not a bug in practice: a failing globalloc never returns, memlib ends the program with "Out of Memory!").
  The runtime errors after `Lock` leave the lock allocated, and the
  FileInfoBlock is freed only on success (#441).
* `DecodeModule` of a module in chip memory keeps the address (length
  1, flag 0); from other memory it copies to chip memory;
  without memory the slot keeps the old address with flag 2 and the new
  length (a later `FreeTrackerModule` frees memory that was never
  allocated) (#433; not a bug in practice: a failing globalloc never returns, memlib ends the program with "Out of Memory!").
* `GetTrackerEvent` changes only d0's low byte; it is declared `#byte`,
  so that is no defect. By design (#446): `ModuleToChip` is an RTS that
  returns its argument; the library's documentation says "the command
  has no function" (since 1.02a `DecodeModule` copies a module to chip
  memory, and the command only still tokenises for old programs).

## DIFF and not reproducible

No DIFF in the results. The C build
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
  filter toggled in the original and kept in the C build (#449),
  `RemICRVector` with bit 0).
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
