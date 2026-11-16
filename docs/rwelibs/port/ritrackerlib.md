# ritrackerlib (58): the Protracker player of ptlib

Reflective Images. A Protracker module player: the Protracker 2.3A play
routine of Peter "Crayon" Hanning / Mushroom Studios (released as Public
Domain; the decision to port it: `archive/README.md`) with RI's commands
around it. Not the acidlibs `trackerlib` (96, NoiseTracker, Module
objects): no command names in common (checked by `rwe_numbers`/`blitzlib numbers`).

* `TrackInit address` – the module in memory (31 samples, `M.K.`):
  `mt_init` (pattern count, sample starts, speed 6, position 0; filter
  off and `mt_end` unless `Tracker Off`), then playing enabled.
* `TrackPlay` – one tick of the play routine (`mt_music`). The program
  calls it itself, e.g. from a `SetInt 5` vertical blank handler: the
  library installs no interrupt (below).
* `TrackStop` – `mt_end` (volumes 0, audio DMA of the four voices off);
  the module address goes to `mt_olddata` (read by nothing).
* `TrackDMA dmawait.w` – the `DBRA` count of the waits for Paula's DMA
  (default 4000).
* `Tracker on+off` – 0: `mt_end` (`mt_Enable` kept), status off; else
  status on.
* `TrackerChannels mask` – the voices to play (bit 3 voice 1 … bit 0
  voice 4); the registers of a cleared voice are zeroed (pointer,
  length, period, volume) and its DMA is switched off.
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
`_Static_assert`s): the original's period lookups read behind the table
into the voices (fixed, #474, #476, #481), and the test compares both
blocks with the reference build's.

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

* `TrackerChannels`: a masked voice does not advance the pattern
  offset, so the following voices play the notes of the voice before
  (#568).

Not a defect: `_ciaplayerinit` returns at once (an `RTS` before its
code), so no interrupt plays the module; the program plays it with
`TrackPlay`, and a tempo command cannot change the speed. Effect 8xx
(RI's track event, `nor_TrackEvent`) stores its value in `nor_TrkEvt`
on the ticks after the first, which no command reads – an unused
variable.

## Fixed

Intended deviations from the original (`docs/libs/deviations.md`,
kind F), in RI's code:

* `Fxx` compares unsigned, so F80–FFF are tempos as in Protracker 2.3A;
  the original compared signed (`CMP.B #32: BGE`) and set the speed to
  128–255 (#569; library test `rwe_ritrackerlib`, the second module's
  F81).
* `Tracker Off` silences the voices (`mt_end`) but keeps `mt_Enable`, so
  `Tracker On` continues the module; a module stopped or paused before
  `Tracker Off` stays stopped. The original's `mt_end` cleared
  `mt_Enable` and `Tracker On` only ANDed it with its value: the song
  stayed stopped until `TrackInit` or `TrackerPause 0` (#572; library
  test `rwe_ritrackerlib`, the ticks "Tracker On after Off").
* `TrackerChannels`: a masked voice is silent. Its audio DMA is
  switched off with the zeroed registers, and the play routine writes
  its registers into a block nobody reads (a table of the four voices'
  register blocks, read once per voice instead of the address computed
  from the voice number: cheaper). The original left the DMA on and
  still wrote the effects of the ticks between the rows (`mt_CheckEfx`:
  period, volume of a slide or tremolo) and `mt_SetDMA`'s loop pointer
  and length of all four voices to Paula, so a masked voice could
  become audible again with its last sample's loop (#567; library test
  `rwe_ritrackerlib`, the redirected registers with the masks $0B and
  $06).
* `TrackInit` while `Tracker Off` initialises the song (pattern count,
  sample starts, speed, position); only the filter and `mt_end`, which
  write to the hardware, are left out while off. The original's
  `mt_init` returned at once, but playing was enabled: `Tracker On` then
  played the old song pointer and sample starts, or from address 0
  without an earlier `TrackInit` (#573; library test `rwe_ritrackerlib`,
  `TrackInit` of the second module while off and the ticks "Tracker 1:
  old song").
* A tempo command (F20–FFF, `SetTempo`) only stores `RealTempo`; the
  original also wrote `deftime` / tempo to `ciatime` and to the latch of
  CIA-B timer A (`$BFD400`/`$BFD500`), a timer the library never
  allocated. `deftime` is set only by the dead `_ciaplayerinit`, so the
  value was always 0; writing the high byte also loads the counter of a
  stopped timer and, in one-shot mode, starts it (#571; library test
  `rwe_ritrackerlib`, the counter of CIA-B timer A after the second
  module's tempo commands). The tempo still does not change the speed:
  that is the program's `TrackPlay` rate.

In the play routine:

* `mt_LowMask` starts as `$FF`, so the first 1xx/2xx slides by its whole
  parameter; the original started it as 0, and the first 1xx/2xx before
  any E1x/E2x did not slide (#478; library test `rwe_ritrackerlib`, the
  ticks of the row with the first 1xx).
* The tremolo's ramp-down waveform (E71) takes its half from the sign of
  the tremolo position; the original tested the vibrato position (#482;
  library test `rwe_ritrackerlib`, the rows with a negative vibrato
  position and a tremolo with the ramp-down waveform).
* `mt_SetPeriod` gives a note period below the table (1–112) the last
  entry of the finetune's row; the original compared 37 entries and took
  entry 37, the second of the next row (for finetune −1 the effect word
  of voice 1) (#481; bug-orig test `pt23a-setperiod-entry37`).
* 9xx with a note applies the sample offset once to `n_start`/`n_length`;
  the original's `mt_SetRegs` called `mt_CheckMoreEfx` and then
  `mt_SetPeriod`, which ran it again (Paula got the offset once, the
  voice twice: a retrig, a note delay or a following note without a
  sample number started at the doubled offset) (#473; bug-orig test
  `pt23a-9xx-twice`).
* `mt_SampleOffset` compares the offset with `n_length` unsigned, so 9xx
  on a sample of $8000 words (64 KB) or more moves the start; the
  original compared signed (`CMP.W`, `BGE`) and set the length to 1
  (#472; bug-orig test `pt23a-9xx-signed-length`).
* The loop start of a sample is start + repeat × 2 doubled as a long;
  the original doubled the repeat as a word (`ASL.W`), so loops from
  32768 words (64 KB) on started 64 KB too early (#479; bug-orig test
  `pt23a-repeat-word`).
* The arpeggio compares the 36 entries of the finetune's row and plays
  at most the row's last entry; the original compared 37 and read up to
  15 words behind the row – the next finetune's row (a low note), for
  finetune −1 the first voice block (#474; library test
  `rwe_ritrackerlib`, the rows by hand with an arpeggio on index 33 of
  sample 1's finetune and of finetune 15).
* `F00` (`mt_SetSpeed: BEQ mt_end`) on voice 1–3: the following voices
  of the row read their notes from the pattern; the original's `mt_end`
  loads a0, the pattern pointer of `mt_GetNewNote`, with `$DFF000`, so
  they read the custom chip registers (`$DFF000` + pattern offset)
  (#475; library test `rwe_ritrackerlib`, the second module's F00 on
  voice 1).
* `mt_init`'s scan of the song positions reads all 128 positions with
  one DBRA per byte and compares unsigned; the original's scan ended
  only through `DBRA`, so position 127 was read only behind a new
  maximum at 126 – and then the scan ran on through the pattern data
  (65535 more bytes), the sample starts lay far behind the module, and
  `mt_init` cleared their first longs there; pattern numbers ≥ 128 never
  counted (#477; bug-orig test `pt23a-init-pattern-scan`, library test
  `rwe_ritrackerlib`, the second module's pattern at position 127).
* `mt_init` sets the sample number of every note above 31 to 0 in the
  counted patterns (once, instead of a test per note; the module is
  changed in memory); the original's `mt_PlayVoice` took the start of
  such a sample from behind `mt_SampleStarts` (32: 0, 33:
  `mt_SongDataPtr`, …) and its header from the position table and the
  patterns (#480; bug-orig test `pt23a-samplenum-over-31`).
* `mt_init` clears the first longword of a sample only if it lies
  before the end of the samples (the sum of the 31 lengths; ciatrklib's
  rule for a module of unknown length); the original's `CLR.L` of every
  sample also cleared up to 4 bytes behind the module for empty samples
  (or one of one word) at its end (#570; library test
  `rwe_ritrackerlib`, the 16 guard bytes behind the first module).
  `TrackInit` has no module length, so this and the note scan of #480
  stay inside the module as its header describes it (patterns, sample
  lengths): a module shorter than that is defective input, and both can
  then write behind it (the test's second module, whose pattern 1 lies
  behind its 2208 bytes).
* `mt_init` masks the finetune byte of the 31 sample headers to 0–15
  (the module is changed in memory); the original took it unmasked, and
  every period lookup (`n_finetune` × 72) of a byte above 15 read far
  behind the table (#476; bug-orig test `pt23a-finetune-above-15`).

## Test

Each run loads one library set (the C build or the original); the
scenario writes its observations as cases, compared with the expected
data `libs/tests/expected/rwe_ritrackerlib.txt` recorded from the
original.

Paula's registers are write-only, so the test redirects them: every
absolute address `$DFF000`–`$DFF1FF` in the loaded code of the library
is patched to a RAM copy of the custom registers (at least one; the
number differs between the builds), filled with a pattern before each
call; the values written are observed after it (AUDxLC as module
offsets, AUDxLEN/PER/VOL, the last DMACON word). No audio is played.
Observed after every call as well: the data block and the glue
variables (found in the library by the tables and by `RealTempo` and
the module address; pointers as module offsets), the module and 16 bytes
behind it, the filter bit of CIA-A PRA, CIA-B timer A's counter (the
test allocates the timer through ciab.resource and keeps it stopped, so
writing the latch's high byte loads the counter), and the registers the
original keeps. The modules are built in memory: one with 31 random
samples (loops, finetunes, a volume above 64, empty samples at the end),
four random patterns with all effects and some rows by hand (the first
1xx, tremolo ramp-down with a negative vibrato position, 9xx with a
note, invert loop, retrig, arpeggios whose note plus the half-tones lies
behind the row); a small one for F81 (a tempo; a speed in the original, #569), tempos, E01
and an F00 on voice 1 (the voices behind read the redirected registers).
About 1700 calls: 1300 ticks of playing, `TrackDMA`, `TrackerChannels`
(two masks), `Tracker` off/on, `TrackerPause`, `TrackInit` while
`Tracker Off`, `TrackStop`, init, finit with and without `TrackInit`.
Deliberate changes of the C build (tremolo's ramp-down position, the a0
of F00) are found by the test. Not covered: sample numbers above 31 and
finetune bytes above 15 (they read outside the library's tables, where
the two builds differ), the dead interrupt code.
