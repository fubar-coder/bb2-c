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

Capture and test are separate operations over this one switch:

| | `LIBSET=orig` | `LIBSET=obj` |
|---|---|---|
| capture (`libs_capture`) | the committed reference; two runs must agree; CD | debugging output in the build directory, diff against the reference; never committed |
| test | `libs_test_originals` (CD, label `originals`) | `libs_test` (no CD, label `emulator`, preset `all`) |

Both tests call the same compare code (D3). The old mix, in which the
originals test re-captured and compared with the committed file, is gone.
Names: `libs_emulator` becomes `libs_test`, `libs_emulator_originals`
becomes `libs_test_originals`; the capture is not a ctest test.

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
observation: `$%08lX` for numbers; bytes as `bytes N HEX` up to 64 bytes, longer
buffers as `bytes N fnv FNV1A32` (a differing long buffer is then named
by check and vector; the byte is found with `LIBS_CAP_DUMP=CHECK`, which
makes the run write the bytes of the named check as hex lines next to
its output). Text: the string itself in quotes with `\xHH`
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
`cmake -DRUN=DIR -DCOMMITTED=DIR [-DEXCEPTIONS=FILE] -P capture-compare.cmake`.
It reports per program the first differing line (number, check, vector,
captured value, run's value) and up to ten more, files missing on either
side, and the line count if one is a prefix of the other. `libs_test`
passes the exceptions table; `libs_test_originals` does not, so the
original's value is expected everywhere (no mode switch: the same code,
one argument fewer). Block lines (D5) are compared the same way (the line
differs, and it names the block). Alternative: compare in the m68k program
against a copy of the capture. Rejected: the compare logic is easier to
test on the host with fake files (no emulator for the checks' tests), and
the emulator run stays the same for capture and compare.

A second script, `capture-accept.cmake`, is the accept step of
`libs_capture` (D7): it takes the two run directories, compares them
byte for byte and, if equal, copies the files to `libs/tests/captures/`;
else it prints "test not deterministic: NAME" with the differing lines
(the same line report as the compare) and copies nothing. It also refuses
files with a `lost` line (D6).

### D4. Exceptions: `libs/tests/exceptions.tsv`

Like `tests/corpus/notes.tsv` (header comment, tab-separated rows):

```
PROGRAM<TAB>CHECK<TAB>#NNN<TAB>ORIGINAL<TAB>PORT<TAB>REASON
```

`CHECK` is `CHECK` and `VECTOR` of the line joined by a tab-free separator
`|`; it must match exactly one line of the capture. The compare of
`libs_test` replaces the capture's value by `PORT` for that line before
comparing with the C build; `libs_test_originals` ignores the table (the
original gives the original's value). A row is stale and fails when: the capture's value at that line
is not `ORIGINAL` (the row does not describe the committed original), the
C build's value is `ORIGINAL` (the fix is undone, or no fix is needed), or
the key matches no line or more than one. The table starts empty (no
`expect_fixed` is in use today: the tapes have no `fixed` line).
`selftest_fixed.c` and `libs_expect_fixed*` are replaced by CMake tests on
fake captures (D8, `libs_compare_row_*`). The rule "a fix does not change the scenario to
avoid a difference" is kept in `tests/README.md`.

### D5. Block hashes for the two very large programs

A program writes a block line `block N COUNT FNV1A32 first=CHECK|VECTOR`
for each 4096 observations when it calls `cap_blocks(on)`; only
`rwe_ridisasmlib` (3.06 million observations) and `systest_med` (3.57
million; user decision of 2026-11-12) do. Every other program writes
lines, `libtest` included (two programs of 41 459 observations each,
about 1.5 MB of text each at 40 bytes a line; `systest_mathstr` has 56
861). A differing block line gives the block number and its observation
range; the run with `LIBS_CAP_DUMP=BLOCK:N` writes the observations of
that block as lines, to find the differing one.

### D6. Kept registers: stated by the test, written as kept/lost

`DO2` still loads marker values (`markregs`) and compares the registers
after the call with the inputs. The test program states per routine the
registers that must be kept: per routine the set the original keeps in
all calls of the scenario (the intersection, never less than the
convention's a4, a5 and a7 through the stack), written in the test
program as `KEEPS(routine, d2|a2)`. After a call the program writes one
line `kept` or `lost d3 a1` for that set, the output being the same for
original and C. A capture run with the original refuses (fails) if any
stated register shows `lost`, so a set that is too strict cannot be
committed. The listing `kept registers ... all calls` of `rwe_done` (the
per-routine intersection) is the aid to state the sets; for routines whose
kept registers depend on the data (`DisAssemble$` keeps d7 or not by the
instruction) the set is the registers kept in all calls. `DONTCARE` stays
as the way to exempt a result register. This replaces `tape_kept`, the
prediction and the `k` lines (30000 entries). The cost, accepted (user
decision of 2026-11-12): a register the original keeps only in some
calls of a routine is not in the set and is not compared, so a C build
that loses it is not caught (today the C build must keep every register
the original keeps in that call). Alternative: write the full kept mask per call (15 bits)
and compare equally. Rejected: the C build keeps more than the original
in many calls (about 30000 of the entries), so output would differ
between the sets; the user decision is no subset rule.

### D7. Determinism

`libs_capture` with the original proves it: each program runs twice, the
second time with a different memory layout (`LIBS_PAD=N`: the program
allocates N bytes of padding before it loads the sets, with N chosen so
that heap and slot addresses shift; run 1 uses 0, run 2 a value that is
not a multiple of the page or slot size). `capture-accept.cmake` (D3)
compares the two outputs and refuses with "test not deterministic" and
the differing lines. Because the normalisation of addresses (D2) is part
of the output, a shifted layout also finds an observation that still
holds an address. The runs of task 1.4 show today's non-deterministic
checks before the conversion.

- `riencryptlib`: Encrypt without wheels takes them from vallib's `Rnd`
  (hardware timing, comment at the head of `rwe/riencryptlib.c`). The
  program sets the wheels by seeding or by calling `GetWheel`'s setter
  first; where vallib's `Rnd` state can be seeded (the program writes the
  seed word of vallib's data, to be checked in `libs/acidlibs/vallib.c`
  and the original's layout), the run is deterministic. Else the part is
  written as `not captured: Rnd of vallib takes the wheels from hardware
  timing`, and the round trip (Decrypt restores the data) stays checked
  inside the run as a check that holds for the one set (`ASSERT`).
  Decided during implementation.
- `ciatrklib`: in the F00 phase the original writes DMACON through
  `$DFF096` into the hardware, the C build through its patched pointer into
  a RAM copy (`rwe/ciatrklib.c`, the comment above `dmaconr`). The
  program puts DMACON into a defined state before the phase (all audio DMA
  off) and writes the register after the phase; the original's value
  and the C build's value are different by construction (hardware against
  RAM copy), so that observation is `not captured: F00 phase: the original
  writes DMACON into the hardware, the C build into its RAM copy` unless
  the task finds a state in which both give the same (decided during
  implementation). The checks that `mt_end`'s `$DFF000` was found
  (`tape_orig()` today) become checks of the one run's own set.

### D8. Tests of the checks (broken input)

The logic tests run on the host or as CMake scripts on fake files, no
emulator, no CD:
`libs_compare_same` (equal files pass), `_value` (one changed value fails
naming the line), `_missing` and `_extra` (a line less and more),
`_block` (a changed block hash names the block), `_longbuf` (a changed
buffer hash names check and vector), `_row_stale_original` (row's
original value not the capture's), `_row_stale_port` (C build gives the
original's value), `_row_missing_line` and `_row_ambiguous`, `_row_pass`
(a correct row passes), `_row_originals` (the same row is not applied
without the table: the original's value is expected), `_not_captured` (a
`not captured` line is skipped on both sides and must be on both),
`libs_accept_nondeterministic` (two outputs differing in one line: the
accept step fails with "test not deterministic", names the line and the
committed directory stays unchanged), `libs_accept_same` (equal outputs
are written), `libs_accept_lost` (a `lost` line is refused). The emulator
side: `libs_test_wrong` runs one small program (libtest ldivlib) with a
build whose output has a changed line and must fail with that line's
message (replaces `libs_emulator_tape_wrong`), `libs_test_originals_wrong`
(the same for the originals test, needs the CD), and `libs_test_lost` (a
routine that loses a stated register, from a variant file with a broken
thunk, gives `lost` and fails).

### D9. Placement

`libs_test` (label `emulator`; `all`): `LIBSET=obj`, then
`capture-compare.cmake` against `libs/tests/captures` with the exceptions
table. `libs_test_originals` (label `originals`, fixture `rwe_refbuild`;
needs the CD): `LIBSET=orig`, the same compare without the table.
`libs_capture` (a custom target, not in `all`, needs the CD): two runs
with `LIBSET=orig` and different `LIBS_PAD`, then `capture-accept.cmake`;
with `LIBSET=obj` it writes one run to the build directory
(`libs-capture-obj/`), prints the compare report against the committed
files, and copies nothing. Renewing a capture is `cmake --build --preset
amiga --target libs_capture` followed by a review of the git diff
(documented in `tests/README.md`). The emulator steps are one
`emulator.cmake` for all three.

### D10. Migration order keeps `all` green

Infrastructure first, beside the tape: `LIBSET` unset means the old
behaviour, so unconverted programs keep running with the tape. A converted
program reads `LIBSET`; during the migration `libs_emulator` (the old
name, until the rename in group 9) runs the program with
`LIBSTAPE=replay LIBSET=obj` and compares the programs that wrote a
capture; the originals run uses `LIBSTAPE=record LIBSET=orig`.
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
  of text in git → measure in task 1.3 before converting; only the two
  named programs use blocks; if the measured total is unacceptable, the
  user decides again (the rule "plain lines for everything else" is a
  decision, not a budget).
- [Non-deterministic checks found late] The two-run capture refuses them;
  task 1.4 lists today's before the conversion, so the cost is known.
- [Large rewrite of 62 programs by hand] A conversion can change what a
  scenario covers → the number of checks per program is kept and
  compared (D10); one commit per group; the compare must pass before
  and after.
- [Observations that held addresses] The tape found them by shifting the
  heap; the text format shows them → task 1.2 runs the compare with a
  padded binary and a shifted heap once, as the experiment of task 9.8;
  afterwards every `libs_capture` repeats it (D7).
- [A stated kept set too weak] See D6; the set is the intersection over
  all calls, so a register the original keeps in some calls only is not
  covered. Accepted.
- [Capture of the original changes with a toolchain or CD update] The CD
  is fixed; a capture only changes with a scenario →
  `libs_test_originals` fails and names the line.

## Migration Plan

1. Infrastructure (`capture.[ch]`, `LIBSET`, compare and accept scripts,
   exceptions table, the `libs_capture` target, their tests) beside the
   tape.
2. Convert program groups, delete each tape in the commit that converts it.
3. Remove the tape code, `LIBSTAPE`, the replay copy, the old tests and
   rewrite the documents.
Rollback: a conversion commit is reverted alone (its tape comes back with
it).

## Open Questions

- Whether vallib's `Rnd` can be seeded for `riencryptlib` (D7), and
  whether the F00 phase of `ciatrklib` can be put into a state where
  original and C build agree; else `not captured: REASON` lines stand.
  Decided during implementation.
