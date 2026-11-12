# Design

## Context

The current design is task 9.8 of the archived change
`2026-11-12-expected-results-testing` (design.md, "Task 9.8"). Observed
state:

- 62 test programs: `libtest.c`, six `systest*.c`, 57 files in `rwe/`
  (`libs_tests.cpp` runs each as a snitch test case). Each loads both sets
  (`load_sets` in `sysload.h`: `sets[0]` = `NAME.orig`, `sets[1]` =
  `NAME.obj`), runs the scenario on both (`DO2`, the `cur` loop, `R[2]`,
  `buf[2]`) and compares (`SAME`, `SAMEBYTES`, `SAMEPTRBYTES`, `BOTH`,
  `cmp_preserve`, `check_fixed`, `SAME_FIXED`).
- Every comparison goes through `tape_*` (`libs/tests/tape.h`). In
  `libs_emulator` the C build is also loaded as `NAME.orig`
  (`emulator.cmake`, `replay`); a comparison that bypasses `tape_*`
  compares C with C. The tapes hold segment hashes of 1024 observations,
  the original's kept-register masks where a prediction misses them
  (about 30000 entries) and `fixed` lines; `tape_pause` and `tape_orig`
  exist for parts that cannot be recorded.
- Observation counts (the `end` lines of `libs/tests/tapes/*.tape`):
  `systest_med` 3 567 195, `rwe_ridisasmlib` 3 058 391, `systest_mathstr`
  56 861, `libtest_ldivlib` and `libtest_lmullib` 41 459 each, the rest
  below 21 000; 6.9 million in all.
- The convention: library routines may change every register except
  a4/a5/a7 (`docs/libs/deviations.md`, "Register preservation"); the
  port's agreement is that thunks preserve at least what the original
  preserves. Today `cmp_preserve` (`sysload.h`) asks for that subset, and
  the tape therefore needs the original's masks.

## Goals / Non-Goals

**Goals:**
- A comparison cannot silently compare the C build with itself: a program
  sees one library set, the reference is a file.
- The reference is readable: a failure shows the line and both values.
- Same coverage as today (the scenarios stay; no scenario is dropped).

**Non-Goals:**
- Explicit setup/act/verify asserts for pure compute libraries: a later
  change (the capture stays the oracle until then).
- Changing library code, scenarios' inputs, or the bug-orig library tests
  (`bugorig.c`: they run the C build only and judge expected behaviour).
- A bug-preserving mode or any use of the capture as a spec of correct
  behaviour: the capture is the original's behaviour; deviations are rows.

## Decisions

### D1. Run mode: one set, chosen by the run

`libs_tests` reads the shell variable `LIBSET` (`obj` or `orig`), like
`LIBSTAPE` today, and `load_set(dir, names)` loads `NAME.<LIBSET>` only
(`sets[]` becomes one `struct set`). `emulator.cmake` copies the files the
run needs: `NAME.obj` for `obj`, the extracted originals for `orig`; the
replay copy of `NAME.obj` as `NAME.orig` disappears. Alternative: two
program binaries. Rejected: a variable keeps one binary and one set of
snitch cases, and the run script stays one line.

### D2. Output: `libs/tests/capture.[ch]`

A program opens its output with `cap_begin(dir, name)` and ends with
`cap_end()`; lines are written to `PROGDIR:captures/NAME.txt` through a
static buffer flushed with `Write` (static, so the tests' `AvailMem`
balance checks stay as they are, as with the tape buffer today). Lines:

```
# capture NAME 1
CHECK<TAB>VECTOR<TAB>VALUE
```

`CHECK` is the check text (`d0`, `popped`, a routine's name), `VECTOR` the
input of the case (`VEC`, the register inputs, the argument), `VALUE` the
observation: `$%08lX` for numbers; bytes as `bytes N HEX` up to 32 bytes, longer
buffers as `bytes N fnv FNV1A32` (a differing long buffer is then named
by check and vector; the byte is found with the dump on demand, see Open
Questions). Text: the string itself in quotes with `\xHH`
escapes. Address-valued observations use the normalisation of `tape.c`
(`PROG`: a value inside the test program's hunks or stack; relative to
the base for library bases, the registry mock, `AllocAbs` buffers, free
memory): the value is written as `prog` or `base+$OFF`. A line `not
captured: REASON` stands for a part that cannot be reproduced.

`OBS(what, expr)` replaces `SAME`, `OBSBYTES` replaces `SAMEBYTES` and
`SAMEPTRBYTES`, `OBSSTR` replaces `tape_strsame`; `BOTH` becomes a plain
`OBS` of each value (the check "holds for both sets" was a comparison of
the sets' own results, now each run writes its own). A program that must
reach a result by an own loop writes it with the same calls, so there is
no comparison to bypass: the only way to observe is to write a line.

### D3. Compare: `capture-compare.cmake`

A CMake script (no tool needed: line comparison) compares the run's
`captures/NAME.txt` with `libs/tests/captures/NAME.txt`:
`cmake -DRUN=DIR -DCOMMITTED=DIR -DEXCEPTIONS=FILE -P capture-compare.cmake`.
It reports per program the first differing line (number, check, vector,
captured value, run's value) and up to ten more, files missing on either
side, and the line count if one is a prefix of the other. The same script
in mode `-DSAME=ON` demands byte-identical files (the check that a fresh
capture of the original equals the committed one, no exceptions applied).
Block lines (D5) are compared the same way (the line differs, and it
names the block). Alternative: compare in the m68k program against a
copy of the capture. Rejected: the compare logic is easier to test on the
host with fake files (no emulator for the checks' tests), and the emulator
run stays the same for capture and compare.

### D4. Exceptions: `libs/tests/exceptions.tsv`

Like `tests/corpus/notes.tsv` (header comment, tab-separated rows):

```
PROGRAM<TAB>CHECK<TAB>#NNN<TAB>ORIGINAL<TAB>PORT<TAB>REASON
```

`CHECK` is `CHECK` and `VECTOR` of the line joined by a tab-free separator
`|`; it must match exactly one line of the capture. The compare replaces
the capture's value by `PORT` for that line before comparing with the
C build. A row is stale and fails when: the capture's value at that line
is not `ORIGINAL` (the row does not describe the committed original), the
C build's value is `ORIGINAL` (the fix is undone, or no fix is needed), or
the key matches no line or more than one. The table starts empty (no
`expect_fixed` is in use today: the tapes have no `fixed` line).
`selftest_fixed.c` and `libs_expect_fixed*` are replaced by CMake tests on
fake captures (D8). The rule "a fix does not change the scenario to
avoid a difference" is kept in `tests/README.md`.

### D5. Block hashes for the two very large programs

A program writes a block line `block N COUNT FNV1A32 first=CHECK|VECTOR`
for each 4096 observations when it calls `cap_blocks(on)`; `ridisasmlib`
(3.06 million) and `systest_med` (3.57 million) do, nothing else (the
next, `systest_mathstr`, has 56 861 observations, about 2 MB as text at
the average line of the other programs, and writes lines). A differing
block line gives the block number and its observation range; the run with
`LIBS_CAP_DUMP=1` writes the observations of one block (`BLOCK=N`) as
lines, to find the differing one. The user decision names `ridisasmlib`
only; `systest_med` has the same volume (the tape's `end` line of
`systest_med.tape` is 3 567 195), so the same rule applies (Open
Questions).

### D6. Kept registers: stated by the test, written as kept/lost

`DO2` still loads marker values (`markregs`) and compares the registers
after the call with the inputs. The test program states per routine the
registers that must be kept: by default the convention's (a4, a5, and a7
through the stack), and a routine whose original keeps more (the port's
agreement) lists them (`KEEPS(routine, d2|a2)`). After a call the
program writes one line `kept` or `lost d3 a1` for the required set, the
output being the same for original and C. The required set per routine
comes from the original: a capture run with the original refuses (fails)
if any required register shows `lost`, so a mask that is too strict
cannot be committed. The listing `kept registers ... all calls` of
`rwe_done` (the per-routine intersection) is the aid to state the masks;
for routines whose kept registers depend on the data (`DisAssemble$`
keeps d7 or not by the instruction) the mask is the registers kept in
all calls. `DONTCARE` stays as the way to exempt a result register. This
replaces `tape_kept`, the prediction and the `k` lines (30000 entries).
The cost: kept registers beyond the stated mask are no longer compared
(today: the C build must keep every register the original keeps in that
call); the stated masks are the intersection over all calls of a routine,
so a C build that loses a register the original keeps in some calls only
is not caught. Alternative: write the full kept mask per call (15 bits)
and compare equally. Rejected: the C build keeps more than the original
in many calls (about 30000 of the entries), so output would differ
between the sets; the user decision is no subset rule.

### D7. Determinism

- `riencryptlib`: Encrypt without wheels takes them from vallib's `Rnd`
  (hardware timing, comment at the head of `rwe/riencryptlib.c`). The
  program sets the wheels by seeding or by calling `GetWheel`'s setter
  first; where vallib's `Rnd` state can be seeded (the program writes the
  seed word of vallib's data, to be checked in `libs/acidlibs/vallib.c`
  and the original's layout), the run is deterministic. Else the part is
  written as `not captured: Rnd of vallib takes the wheels from hardware
  timing`, and the round trip (Decrypt restores the data) stays checked
  inside the run as a check that holds for the one set (`ASSERT`).
- `ciatrklib`: in the F00 phase the original writes DMACON through
  `$DFF096` into the hardware, the C build through its patched pointer into
  a RAM copy (`rwe/ciatrklib.c`, the comment above `dmaconr`). The
  program puts DMACON into a defined state before the phase (all audio DMA
  off) and writes the register after the phase; the original's value
  and the C build's value are different by construction (hardware against
  RAM copy), so that observation is `not captured: F00 phase: the original
  writes DMACON into the hardware, the C build into its RAM copy` unless
  the task finds a state in which both give the same. The checks that
  `mt_end`'s `$DFF000` was found (`tape_orig()` today) become checks of
  the one run's own set.
- Every program is run twice in a capture run for the check
  `libs_capture_repeat` (same bytes), so a new non-determinism is found.

### D8. Tests of the checks (broken input)

All logic tests run on the host or as CMake scripts on fake files, no
emulator, no CD:
`libs_capture_compare_same` (equal files pass), `_value` (one changed
value fails naming the line), `_missing` and `_extra` (a line less and
more), `_block` (a changed block hash names the block), `_lost`
(a `lost` line in a capture is refused), `_row_stale_original` (row's
original value not the capture's), `_row_stale_port` (C build gives the
original's value), `_row_missing_line` and `_row_ambiguous`, `_row_pass`
(a correct row passes), `libs_capture_not_captured` (a `not captured`
line is skipped on both sides and must be on both). The emulator side:
`libs_emulator_wrong` runs one small program (libtest ldivlib) with a
build whose output has a changed line and must fail with that line's
message (replaces `libs_emulator_tape_wrong`).

### D9. Placement

`libs_emulator` (labels `emulator`; `all`): `LIBSET=obj`, then
`capture-compare.cmake` against `libs/tests/captures`. `libs_emulator_originals`
(labels `emulator originals`, fixture `rwe_refbuild`): `LIBSET=orig`, then
`capture-compare.cmake -DSAME=ON`. A capture is renewed by copying the
files of the originals run's `test/captures/` into `libs/tests/captures/`
(documented in `tests/README.md`).

### D10. Migration order keeps `all` green

Infrastructure first, beside the tape: `LIBSET` unset means the old
behaviour, so unconverted programs keep running with the tape. A converted
program reads `LIBSET`; `libs_emulator` runs the program with
`LIBSTAPE=replay LIBSET=obj` and compares the programs that wrote a
capture; `libs_emulator_originals` with `LIBSTAPE=record LIBSET=orig`.
A converted program's tape file is deleted in the commit that converts
it. The last group removes the tape code, the modes and the old tests.
The check that a converted program's capture equals the old tape's
verdict: before the commit that deletes a tape, the capture of the
original is made, then the C build is compared with it (this is the
acceptance of each conversion), and the numbers of the program's checks
stay the same (the number printed by the program, `NAME: N checks, M
failures`, before and after; a change is named in the commit).

## Risks / Trade-offs

- [Size of the text captures] 6.9 million observations in all, about 0.3
  million outside the two block programs; at about 40 bytes a line 12 MB
  of text in git → measure in task 1.3 before converting; programs above
  the budget (one million observations; the two named) use blocks; if the
  total exceeds a stated size, `libtest` (82 918 observations) also writes
  a line per routine and round instead of per observation (Open
  Questions).
- [Large rewrite of 62 programs by hand] A conversion can change what a
  scenario covers → the number of checks per program is kept and
  compared (D10); one commit per group; the compare must pass before
  and after.
- [Observations that held addresses] The tape found them by shifting the
  heap; the text format shows them → task 1.2 runs the compare with a
  padded binary and a shifted heap once, as the experiment of task 9.8.
- [A required-kept mask too weak] See D6; the mask is the intersection
  over all calls, listed in the capture run's summary.
- [Capture of the original changes with a toolchain or CD update] The CD
  is fixed; a capture only changes with a scenario → `originals` fails
  and names the line.

## Migration Plan

1. Infrastructure (`capture.[ch]`, `LIBSET`, compare script, exceptions
   table, their tests) beside the tape.
2. Convert program groups, delete each tape in the commit that converts it.
3. Remove the tape code, `LIBSTAPE`, the replay copy, the old tests and
   rewrite the documents.
Rollback: a conversion commit is reverted alone (its tape comes back with
it).

## Open Questions

- Whether `systest_med` writes block hashes like `ridisasmlib` (D5: the
  design says yes, the user named `ridisasmlib` only), and whether
  `libtest` (two programs of 41 459 observations) writes lines or blocks
  (measure, task 1.3).
- The required-kept sets (D6): the convention alone (a4/a5/a7), or the
  stricter set where the original keeps more (the design assumes the
  stricter set per routine, as the port's agreement says). Either keeps the
  output identical.
- Long byte buffers (D2): a hash line names check and vector, not the
  byte; whether the second level is a dump on demand (design) or a hex
  line for buffers up to a larger size.
- Whether vallib's `Rnd` can be seeded for `riencryptlib` (D7), and
  whether the F00 phase can be put into a state where original and C
  build agree; else the `not captured` lines stand.
