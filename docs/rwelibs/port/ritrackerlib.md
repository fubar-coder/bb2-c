# ritrackerlib (58): the Protracker player of ptlib

Reflective Images. A Protracker module player: the Protracker 2.3A play
routine of Peter "Crayon" Hanning / Mushroom Studios (released as Public
Domain; the decision to port it: `archive/README.md`) with RI's commands
around it. Not the acidlibs `trackerlib` (96, NoiseTracker, Module
objects): no command names in common (checked by `rwe_numbers`/biglib).

* `TrackInit address` – the module in memory (31 samples, `M.K.`):
  `mt_init` (pattern count, sample starts, speed 6, position 0, filter
  off, `mt_end`) unless `Tracker Off`, then playing enabled.
* `TrackPlay` – one tick of the play routine (`mt_music`). The program
  calls it itself, e.g. from a `SetInt 5` vertical blank handler: the
  library installs no interrupt (below).
* `TrackStop` – `mt_end` (volumes 0, audio DMA of the four voices off);
  the module address goes to `mt_olddata` (read by nothing).
* `TrackDMA dmawait.w` – the `DBRA` count of the waits for Paula's DMA
  (default 4000).
* `Tracker on+off` – 0: `mt_end`, status off; else status on.
* `TrackerChannels mask` – the voices to play (bit 3 voice 1 … bit 0
  voice 4); the registers of a cleared voice are zeroed (pointer,
  length, period, volume; its DMA stays on).
* `TrackerPause On|Off` – nonzero: `mt_end`; 0: enable and play one
  tick.
* init: `playing` = 0; finit: `mt_end` if `TrackInit` was called.

* **Source:** `rwelibsrc/ptlib/ptlib6.bb` (as TED shows it:
  `build/amiga/rwelibsrc/ptlib/ptlib6.bb`).
* **Reference:** the reference build of the source by blitz2 2.10
  (`refbuild`, `build/amiga/libs/userlibs/rwe/ref/ritrackerlib.obj`,
  6108 bytes); no binary of the library exists anywhere. The number 58
  only from RWE's list (`docs/rwelibs/README.md` §3.3). Declarations
  equal (ctest `rwe_declarations_ritrackerlib`).
* **C file:** `libs/userlibs/rwe/ritrackerlib.c`; test
  `libs/tests/rwe/ritrackerlib.c` (snitch case "rwe ritrackerlib").
* The CIA tracker library (`ciatrklib`, `ptlib/CIATrackerLib1.bb`)
  contains the same play routine; the two ports share no code.

## Which version: ptlib6

All eight sources (`ptlib`, `ptlib2` … `ptlib7`, `ptlibmatty`) have
`!libheader {#ritrackerlib,init,0,finit,0}`; the file dates are all
1996-07-26 and say nothing. The `.xtra` files all write `PTLib.obj`.
Lines that differ (detokenised, `diff`): ptlib4–ptlib7 31, ptlib5–ptlib6
62, ptlib–ptlib2 134, ptlib–ptlib4 188, ptlib–ptlib5 343; ptlib3 is
600–900 lines away from all (an early state: `TrackInit` calls
`mt_init` directly, no `TrackerChannels` defaults).

Two lines grow from `ptlib`:

* **ptlib4 → ptlib7, ptlibmatty:** the CIA allocation code is rewritten
  (`SetCIAInt` tries timers A/B of ciaa/ciab.resource in turn) and a
  function `TrackerTimer` returns which timer it got; ptlib7 fixes the
  timer numbering of ptlib4 and starts the timer, ptlibmatty makes
  `TrackInit` a function. These three have `status` = 0 as the default
  (`Tracker` off until `Tracker On`).
* **ptlib5 → ptlib6:** ptlib5 starts with the only dated note of all
  versions, "8/4/95": the CIA allocation routines ("TOTALLY SHIT") are
  replaced by a routine that banged the hardware (its own level 6 vector
  through the VBR, CIA-B timer A), `TrackerPause` is added, and
  `Tracker On|Off` and `TrackerChannels` are fixed ("forgot to set
  default values": `status` −1, `mask` 15, also in `ptlib`).
  `TrackerChannels` zeroes the voices it masks (ptlib4/7: only the
  mask). ptlib6 is ptlib5 plus: the vector banging (and `GetVBR`) is
  replaced by intlib's `SetInt` (`ALibJsr $c100`, level `$8043` = the
  vertical blank) – and then disabled by an `RTS` as the first
  instruction of `_ciaplayerinit` –, the `l6_Int` counter that halved
  the rate is gone, and finit only calls `_ciaplayerfinit` when
  `TrackInit` was called (ptlib5 restored the level 6 vector from `old`
  without that guard: 0 if `TrackInit` was never called).

ptlib6 is the latest: it is a step after ptlib5, whose note dates the
removal of the CIA allocation code the other line still develops; it
carries the fixes of that note, has the most commands except
`TrackerTimer` (which only reports the CIA allocation ptlib5/6 dropped)
and adds a bug fix of its own (the finit guard). It compiles with
blitz2 2.10 (intlib's token `$c100` needs no other library file).
That its interrupt code is disabled is the state of the source; the
library then plays only through `TrackPlay`, as `TrackerPause 0`
(`BSR mt_music`) also assumes.

## State and layout

The original keeps its data in the code: the play routine's block from
`mt_FunkTable` to `mt_olddata` (the funk, vibrato and period tables,
four 44-byte voice blocks `n_note` … `n_reallength`, `mt_SampleStarts`,
`mt_SongDataPtr`, speed, counter, positions, flags, `DMA_Wait`,
`playing`, `status`, `mask`) and the glue variables `ciatime` …
`mt_data`. The C build has both as structs with the same layout
(`struct ptdata`, 1538 bytes, `struct ptglue`, 28 bytes,
`_Static_assert`s): the period lookups read behind a row and, for
finetune −1, behind the table into the first voice (BUG(orig), #481,
#474), which
gives the same values only with the same layout; and the test compares
both blocks with the reference build's.

## C and assembler

All C behind `BLITZ_THUNK`s that keep d0–d1/a0–a1 (no command has a
result; the original keeps everything in `TrackPlay`, `Tracker`,
`TrackerPause`, init/finit, and changes d0–d2/a0–a2 in `TrackInit`, a0
in `TrackStop`/`TrackerChannels`). The routine's register traffic
between its parts – the pattern offset d1 of `mt_GetNewNote`, the
pattern pointer a0 (see the F00 bug) – is passed as parameters or a
static. Byte, word and signed/unsigned operations are those of the
original (`CMP.B`/`BGT` signed, `BCS` unsigned, `ADD.W` without carry
into the upper word, `ASL.W` of the loop offset, `DIVU` through
`bl_divu_w`). Paula's registers and the CIAs are written as volatile
accesses of the original's width; the pattern note is read as one
longword (`MOVE.L`). The `DBRA` delays are loops with an empty `asm
volatile` (DMA_Wait + 1 passes). The dead body of `_ciaplayerinit`
(the SetInt call, CIA-B timer setup) and `l6_Int` are kept as unused C
functions: the `ALibJsr $c100` links intlib into a program as with the
original.

## BUG(orig), kept

RI's glue:

* `_ciaplayerinit` returns at once (an `RTS` before its code): no
  interrupt plays the module; `deftime` stays 0, so a tempo command
  (`Fxx` with xx ≥ $20) writes 0 to CIA-B timer A's latch
  (`$BFD400`/`$BFD500`) – a timer the library never allocated – and to
  `ciatime` (#571).
* `Fxx` compares signed (`CMP.B #32: BGE`): F80–FFF set the speed to
  128–255 instead of the tempo (#569).
* `F00` (`mt_SetSpeed: BEQ mt_end`) on voice 1–3: `mt_end` loads a0 with
  `$DFF000`, so the following voices of the row read their notes from
  the custom chip registers (`$DFF000` + pattern offset) instead of the
  pattern (#475).
* `TrackerChannels`: a masked voice does not advance the pattern
  offset, so the following voices play the notes of the voice before
  (#568); the effects (`mt_CheckEfx`) and the loop registers of
  `mt_SetDMA` still go to all four voices (#567).
* `Tracker Off` clears `mt_Enable` (`mt_end`), `Tracker On` only ANDs
  it: the song stays stopped until `TrackInit` or `TrackerPause 0`
  (#572).
* `TrackInit` while `Tracker Off`: `mt_init` returns at once, but playing
  is enabled; `Tracker On` then plays with the old song pointer and
  sample starts (#573).

Not a defect: effect 8xx (RI's track event, `nor_TrackEvent`) stores its
value in `nor_TrkEvt` on the ticks after the first, which no command
reads – an unused variable.

The play routine (the known bugs of the PT2.3A replayer; 8bitbubsy's
pt23f changelog served as the checklist, no code was taken from it):

* `mt_init`'s pattern count: signed compare (pattern numbers ≥ 128);
  the scan ends only through `DBRA`, so order position 127 is scanned
  only if 126 is a new maximum, and then the scan runs on through the
  pattern data (65536 more bytes) (#477). The `CLR.L` of every sample's
  first longword also clears 4 bytes behind the module for empty
  samples at its end (#570).
* `mt_LowMask` starts at 0: the first 1xx/2xx before any E1x/E2x does
  not slide (#478).
* The period lookups (`mt_SetPeriod`, `mt_Arpeggio`, `mt_SetTonePorta`,
  glissando) go on behind the finetune's row when no entry fits (37
  entries; for finetune −1 into the first voice block) (#481, #474); a
  finetune byte above 15 in a sample header reads far behind the table
  (#476).
* Tremolo's ramp-down waveform tests the vibrato position (#482).
* 9xx: the offset is compared signed with the length (samples of 64 KB
  and more get length 1) (#472); with a note it is applied twice
  (`mt_CheckMoreEfx` before and after `mt_SetPeriod`) (#473). The loop
  start is start + repeat shifted as a word (repeats ≥ 32768 words wrap)
  (#479).
* Sample numbers above 31 read behind `mt_SampleStarts` and the sample
  headers (#480).

## Test

Paula's registers are write-only, so the test redirects them: every
absolute address `$DFF000`–`$DFF1FF` in the loaded code of both
libraries is patched to a RAM copy of the custom registers per set (19
places in the reference build, 16 in the C build), filled with a pattern
before each call; the values written are compared after it (AUDxLC as
module offsets, AUDxLEN/PER/VOL, the last DMACON word). No audio is
played. Compared after every call as well: the data block and the glue
variables (found in both libraries by the tables and by `RealTempo` and
the module address; pointers as module offsets), the module and 16 bytes
behind it, the filter bit of CIA-A PRA, CIA-B timer A's counter (the
test allocates the timer through ciab.resource and keeps it stopped, so
writing the latch's high byte loads the counter), and the registers the
original keeps. The modules are built in memory: one with 31 random
samples (loops, finetunes, a volume above 64, empty samples at the end),
four random patterns with all effects and some rows by hand (the first
1xx, tremolo ramp-down with a negative vibrato position, 9xx with a
note, invert loop, retrig); a small one for F81 (a speed), tempos, E01
and an F00 on voice 1 (the voices behind read the redirected registers).
About 1700 calls: 1300 ticks of playing, `TrackDMA`, `TrackerChannels`
(two masks), `Tracker` off/on, `TrackerPause`, `TrackInit` while
`Tracker Off`, `TrackStop`, init, finit with and without `TrackInit`.
Deliberate changes of the C build (tremolo's ramp-down position, the a0
of F00) are found by the test. Not covered: sample numbers above 31 and
finetune bytes above 15 (they read outside the library's tables, where
the two builds differ), the dead interrupt code.
