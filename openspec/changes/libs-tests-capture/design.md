# Design

## Context

The current design is task 9.8 of the archived change
`2026-11-12-expected-results-testing` (design.md, "Task 9.8"). Observed
state:

- 60 test programs: `libtest.c`, six `systest*.c`, 53 files in `rwe/`
  (`libs_tests.cpp` runs each as a snitch test case); 61 tapes, because
  `libtest` writes `ldivlib` and `lmullib`. Each program loads both sets
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
- The other suites keep expected data in other forms: corpus
  `expect/PASS/NAME.out` plus `notes.tsv` rows, bug-orig `ID.expect` and
  `ID.orig`, TED oracle rows, `; @fixed #N`. TED already splits a fixed
  test into a run with the port in `all` and one with the original in
  `originals` (task 9.9 of the archived change).

## Goals / Non-Goals

**Goals:**
- One test model for all suites (spec `testing/test-model`), implemented
  for the library tests.
- A comparison cannot silently compare the C build with itself: a program
  sees one library set, the expected data is a file.
- The expected data is readable: a failure shows the key and both values.
- Same coverage as today (the scenarios stay; no scenario is dropped).

**Non-Goals:**
- Changing the corpus, bug-orig or TED tests or their specs' behaviour.
  Follow-up changes (not part of this one): bug-orig (the recording test
  becomes a compare, a recording target accepts only agreeing runs, the
  known varying files get a wildcard with a reason) and the corpus
  (`notes.tsv` rows become `[orig]`/`[fixed #N]` cases).
- Explicit setup/act/verify asserts for pure compute libraries: a later
  change.
- Changing library code, scenarios' inputs, or the bug-orig library tests
  (`bugorig.c`: they run the C build only and judge expected behaviour).
- A bug-preserving mode.

## Decisions

### D1. The shared model and its terms

`testing/test-model` states the model: a test is a list of cases (key,
input, expected output; verdict = diff; reported by key); inputs are data
for values and code or a file for sequences; the expected output is in the
test definition; agreement of original and port is one case; a fixed bug
is two cases `KEY [orig]` (original only, label `originals`) and `KEY
[fixed #N]` (port only, `all`); record = from the original, update = from
the port; recording targets write, tests compare; the judge is a C tool.
The review of 2026-11-12 found that the corpus, CD table and oracle keep
the port's value in the committed file and the original's in a note, while
the first form of this change kept the original's value forever with an
exception table that only the port test applied; the pair of cases ends
that inversion. This decision replaces the earlier exceptions table
(`expect_fixed`, `exceptions.tsv`): there is none.

Words: "record" writes from the original (`libs-capture`), "update" writes
from the port (`libs-update`, like `corpus-expect-update`); the
expectation is "expected data" or "recorded original", never "the
reference".

### D2. Run mode: one set, chosen by the run

`libs_tests` reads the shell variable `LIBSET` (`obj` or `orig`), like
`LIBSTAPE` today, and `load_set(dir, names)` loads `NAME.<LIBSET>` only
(`sets[]` becomes one `struct set`). `emulator.cmake` copies the files the
run needs: `NAME.obj` for `obj`, the extracted originals for `orig`; the
replay copy of `NAME.obj` as `NAME.orig` disappears. Alternative: two
program binaries. Rejected: a variable keeps one binary and one set of
snitch cases.

| | `LIBSET=orig` | `LIBSET=obj` |
|---|---|---|
| record (`libs-capture`) | the expected data; two runs must agree; CD | - |
| update (`libs-update`) | - | `[fixed #N]` cases from the port |
| test | `libs_emulator_originals` (CD, label `originals`) | `libs_emulator` (no CD, label `emulator`, preset `all`) |

There is no operation "capture with the C build": `libs_emulator` keeps the
run's output (`captures/NAME.txt` in the test's work directory,
`build/<preset>/libs/tests/`), as the corpus keeps `NAME.run` and bug-orig
`observation`; it is diffed against `libs/tests/expected/NAME.txt` by hand
(`LIBS_CAP_DUMP=KEY` adds the bytes of a long buffer).

### D3. The judge: a C tool

`tools/lib/libscmp` (logic) with `tools/results` subcommands, driven by
`emulator.cmake` and the targets; host tests in `tools/tests/` from memory
(equal files, a changed value, a missing and an added case, the
qualifiers, malformed and ambiguous files, a changed block, a long buffer,
`not captured`, accept of equal and different runs, update, the filter).
Measured in the review (CMake 4.4.5): a `foreach(... IN ZIP_LISTS)` compare
of two 300 000-line files takes 1.4-2.9 s, an indexed `list(GET)` loop did
not finish in 3 minutes, and `file(STRINGS)` reads a line holding an
unbalanced `[` as several list elements (`OBSSTR` output). So CMake only
orchestrates, as with `cdtable`/`oracletable`.

- `results libs-compare RUN_DIR EXPECTED_DIR --set obj|orig [--only NAME...]`:
  per program, the cases of the run against the expected data of the set
  (D4): the first differing key with both values and up to ten more, keys
  missing or added, malformed lines. `--only` restricts to the named
  programs (the filter of `libs_emulator_wrong` and of a filtered record:
  the other files are not reported as missing).
- `results libs-accept RUN1 RUN2 EXPECTED_DIR [--only NAME...]`: compares
  the two recording runs; if identical, rewrites the files (D5); else
  prints "test not deterministic: NAME" with the differing cases and
  writes nothing. It refuses a `lost` case.
- `results libs-update RUN_DIR EXPECTED_DIR [--fix KEY#N...]`: D5.

### D4. Output and expected data: `libs/tests/capture.[ch]`, `expected/`

A program opens its output with `cap_begin(dir, name)` and ends with
`cap_end()`; lines are written to `PROGDIR:captures/NAME.txt` through a
static buffer flushed with `Write` (static, so the tests' `AvailMem`
balance checks stay as they are, as with the tape buffer today). Lines:

```
# capture NAME 1
ROUTINE|CHECK|VECTOR|OCC<TAB>VALUE
```

The **key** is routine, check text, vector (the input of the case: `VEC`,
the register inputs, the argument) and an occurrence number counted per
(routine, check, vector) from 1; the number makes the key unique where
the same check with the same vector recurs (`SAME("finit d0")` at
`rilinkedlistlib.c:254` and `:386`; the log loops of `rwecmp.h:187-188`).
Escaping in every part: TAB `\t`, `|` `\|`, `\` `\\`, non-printable bytes
`\xHH` (`VEC` is free text, `char VEC[100]`). A program without routines
uses an empty routine part.

The **value**: `$%08lX` for numbers; bytes as `bytes N HEX` up to 64
bytes, longer buffers as `bytes N fnv FNV1A32` (the byte is found with
`LIBS_CAP_DUMP=KEY`, which makes the run write the bytes of the named case
as hex lines next to its output); text in quotes with `\xHH` escapes.
Address-valued observations use the normalisation of `tape.c` (`PROG`: a
value inside the test program's hunks or stack; relative to the base for
library bases, the registry mock, `AllocAbs` buffers, free memory),
written as `prog` or `base+$OFF`. A case `not captured: REASON` stands for
a part that cannot be reproduced and is skipped on both sides.

`OBS(what, expr)` replaces `SAME`, `OBSBYTES` replaces `SAMEBYTES` and
`SAMEPTRBYTES`, `OBSSTR` replaces `tape_strsame`; `BOTH` becomes a plain
`OBS` of each value. A program that must reach a result by an own loop
writes it with the same calls, so there is no comparison to bypass: the
only way to observe is to write a case.

The **expected data** `libs/tests/expected/NAME.txt` has the same format;
a key may carry a qualifier after the occurrence number: `KEY [orig]` or
`KEY [fixed #N]`.

```
# capture NAME 1
rilinkedlistlib|finit d0||1<TAB>$00000000
rilinkedlistlib|find d0||2 [orig]<TAB>$00000001
rilinkedlistlib|find d0||2 [fixed #123]<TAB>$00000000
```

The judge's rules for a set: unqualified cases apply to both; `[orig]`
only to `orig`, `[fixed #N]` only to `obj`; a key present both unqualified
and qualified, an unknown qualifier, a duplicate key or a malformed line is
a malformed file (reported with the line; exit status 2). A case only in
the port (a fix that adds a case) is `KEY [fixed #N]` alone, one only in the
original `KEY [orig]` alone. Hand-written `[fixed #N]` values use the same
format. No staleness rule: when the port gives the original's value again
`[fixed #N]` fails, when the original differs `[orig]` fails.
`.gitattributes` gets `/libs/tests/expected/** -text diff` as for the
corpus.

### D5. Record and update targets

`libs-capture` (a custom target, not in `all`, needs the CD): two runs with
`LIBSET=orig` and different `LIBS_PAD` (D7), then `results libs-accept`: if
the outputs are identical the expected files are rewritten with the
original's values as unqualified cases, except for a key that has a
`[fixed #N]` case in the file: its original value is written as `[orig]`
and the `[fixed #N]` case is kept unchanged. A `[fixed #N]` key the run no
longer has is reported (the scenario changed), and the new file is written
complete. A filter (`-DFILTER=NAME`, as `emulator.cmake`
already has) records only the named programs and leaves the other files
untouched.

`libs-update` (a custom target, no CD): one run with `LIBSET=obj`, then
`results libs-update`: the value of every `[fixed #N]` case is rewritten
from the port's output; `-DFIX=KEY#N` additionally turns the unqualified
case `KEY` whose port value differs from the expected one into the pair
`KEY [orig]` (the old value) and `KEY [fixed #N]` (the port's value);
nothing else is touched. Both targets are reviewed by `git diff`
(`tests/README.md`).

### D6. Block hashes for the two very large programs

A program writes a case `BLOCK:N` with `COUNT FNV1A32 first=CHECK|VECTOR`
for each 4096 observations when it calls `cap_blocks(on)`; only
`rwe_ridisasmlib` (3.06 million observations) and `systest_med` (3.57
million; user decision of 2026-11-12) do. Every other program writes
plain cases, `libtest` included (two programs of 41 459 observations each,
about 1.5 MB of text each at 40 bytes a line; `systest_mathstr` has 56
861). A differing block names the block number and its observation range;
the run with `LIBS_CAP_DUMP=BLOCK:N` writes the observations of that block
as lines, to find the differing one.

Limits, stated (not solved): a fix inside a block program changes a block
hash, so its pair `[orig]`/`[fixed #N]` holds two hashes nobody can
review; two fixes in one block share one pair and one issue number; a fix
that changes the number of observations shifts the block numbers after it
(every later block becomes a pair or the file is recorded again). A fix
that changes the number of cases in a program with plain cases is
expressible (cases only in one side), but the occurrence numbers after the
change shift.

### D7. Kept registers: stated by the test, written as kept/lost

`DO2` still loads marker values (`markregs`) and compares the registers
after the call with the inputs. The test program states per routine the
registers that must be kept: per routine the set the original keeps in
all calls of the scenario (the intersection, never less than the
convention's a4, a5 and a7 through the stack), written in the test
program as `KEEPS(routine, d2|a2)`. After a call the program writes one
case with the value `kept` or `lost d3 a1` for that set, the output being
the same for original and C. A recording run with the original refuses
(fails) if any stated register shows `lost`, so a set that is too strict
cannot be committed. The listing `kept registers ... all calls` of
`rwe_done` is the aid to state the sets; for routines whose kept registers
depend on the data (`DisAssemble$` keeps d7 or not by the instruction) the
set is the registers kept in all calls. `DONTCARE` stays as the way to
exempt a result register. This replaces `tape_kept`, the prediction and the
`k` lines (30000 entries). The cost, accepted (user decision of
2026-11-12): a register the original keeps only in some calls of a routine
is not in the set and is not compared, so a C build that loses it is not
caught (today the C build must keep every register the original keeps in
that call). Alternative: write the full kept mask per call (15 bits) and
compare equally. Rejected: the C build keeps more than the original in
many calls (about 30000 of the entries), so output would differ between
the sets; the user decision is no subset rule.

A program's own `check()` (`check("ffplib init: open count +2")`,
`systest_mathstr.c:218`) stays for consistency checks and holds in both
runs. Limit: when a fix changes the behaviour such a check states, the
check fails for one of the two sets and no pair can express it; a `check()`
that a fix touches becomes an `OBS` first.

### D8. Determinism

`libs-capture` proves it for the memory layout: each program runs twice,
the second time with a different layout (`LIBS_PAD=N`: the program
allocates N bytes of padding before it loads the sets, with N chosen so
that heap and slot addresses shift; run 1 uses 0, run 2 a value that is
not a multiple of the page or slot size). `results libs-accept` compares
the two outputs and refuses with "test not deterministic" and the
differing cases. Because the normalisation of addresses (D4) is part of
the output, a shifted layout also finds an observation that still holds an
address.

Limit, stated: the two runs vary the layout only. They do not vary date,
time or emulator timing (sessions start from a savestate), so a
timing-dependent value that agrees twice passes (`riencryptlib`'s wheel is
the known case and varies; others are a guess), and a value stable for the
original but timing-dependent in the C build shows as a `libs_emulator`
flake. The detector for these is `libs_emulator_originals` on another day
and the runs of task 1.4. This is not a proof of determinism.

- `riencryptlib`: Encrypt without wheels takes them from vallib's `Rnd`
  (hardware timing, comment at the head of `rwe/riencryptlib.c`). The
  program sets the wheels by seeding or by calling `GetWheel`'s setter
  first; where vallib's `Rnd` state can be seeded (the program writes the
  seed word of vallib's data, to be checked in `libs/acidlibs/vallib.c`
  and the original's layout), the run is deterministic. Else the part is
  written as `not captured: Rnd of vallib takes the wheels from hardware
  timing`, and the round trip (Decrypt restores the data) stays checked
  inside the run as a check that holds for the one set (`check()`).
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

### D9. Tests of the checks (broken input)

The judge's logic is tested on the host (`tools/tests/test_libscmp.c`, from
memory): equal files pass; one changed value fails naming the key; a case
less and more; a changed block names the block; a changed long-buffer hash
names key and hashes; the qualifier rules (`[orig]` ignored in an `obj`
run and applied in an `orig` run, `[fixed #N]` the other way, a regressed
`[fixed #N]` fails, a changed `[orig]` fails, a case only in one side);
malformed, duplicate and ambiguous files; `not captured` skipped on both
sides; the escaping; accept of equal runs (writes, keeps `[fixed #N]`),
accept of differing runs ("test not deterministic", names the case, writes
nothing), accept of a `lost` case (refused); update with and without
`--fix`; the filter. ctest runs a few of these on fake files through
`tools/results` (`libs_compare_wrong_*`, `libs_accept_nondeterministic`) in
the `*_check_wrong` style. The emulator side: `libs_emulator_wrong` runs
one small program (libtest ldivlib, `--only`) with a build whose output has
a changed case and must fail with that key's message (replaces
`libs_emulator_tape_wrong`), and `libs_emulator_lost` (a routine that loses
a stated register, from a variant file with a broken thunk, gives `lost`
and fails). No `..._originals_wrong` test: it would need the CD for logic
the host test already covers.

### D10. Placement

`libs_emulator` (label `emulator`; `all`): `LIBSET=obj`, then
`results libs-compare --set obj` against `libs/tests/expected`.
`libs_emulator_originals` (label `originals`, fixture `rwe_refbuild`;
needs the CD): `LIBSET=orig`, the same compare with `--set orig`.
`libs-capture` and `libs-update`: D5. One `emulator.cmake` orchestrates
the emulator steps for all four. `tools/testinfo/rules.txt` gets a rule
`libs/tests/expected/**` selecting both tests (the `libs/**` rule selects
`label:emulator`, which does not catch an originals-only test).

### D11. Migration order keeps `all` green

Infrastructure first, beside the tape: `LIBSET` unset means the old
behaviour, so unconverted programs keep running with the tape. A converted
program reads `LIBSET`; during the migration `libs_emulator` runs the
program with `LIBSTAPE=replay LIBSET=obj` and compares the programs that
wrote output; the originals run uses `LIBSTAPE=record LIBSET=orig`.
A converted program's tape file is deleted in the commit that converts
it. The last group removes the tape code, the modes and the old tests.
The check that a converted program's expected data equals the old tape's
verdict: before the commit that deletes a tape, the expected data is
recorded from the original, then the C build is compared with it (this is
the acceptance of each conversion), and the numbers of the program's
checks stay the same (the number printed by the program, `NAME: N checks, M
failures`, before and after; a change is named in the commit).

## Risks / Trade-offs

- [Size of the text, and CD content] 6.9 million observations in all, about
  0.3 million outside the two block programs; at about 40 bytes a line
  12 MB of text in git, into a repository whose pack is 11.68 MiB
  (measured 2026-11-12, `git count-objects -vH`), and each renewed file
  adds its delta. Strings and hex buffers up to 64 bytes can hold literal
  data of CD libraries (tables, texts), and CD material is not checked in
  (`CLAUDE.md`). This reverses the argument of task 9.8 deliberately
  ("13 MB, and a copy of what the original computes"). The corpus `.out`
  files are a precedent for outputs. Task 1.3 measures the committed text
  size, also compressed, and checks whether the values hold literal CD
  data; the decision (keep text, or hash more programs) is taken from that
  and recorded in the design.
- [Non-deterministic checks found late] The two-run record refuses them;
  task 1.4 lists today's before the conversion, so the cost is known.
  Timing-dependent ones are not found by the layout shift (D8).
- [Large rewrite of 60 programs by hand] A conversion can change what a
  scenario covers → the number of checks per program is kept and compared
  (D11); one commit per group; the compare must pass before and after.
- [Observations that held addresses] The tape found them by shifting the
  heap; the text format shows them → task 1.2 runs the compare with a
  padded binary and a shifted heap once, as the experiment of task 9.8;
  afterwards every `libs-capture` repeats it (D8).
- [A stated kept set too weak] See D7; the set is the intersection over
  all calls, so a register the original keeps in some calls only is not
  covered. Accepted.
- [Expected data changes with a CD update] The CD is fixed; the data only
  changes with a scenario → `libs_emulator_originals` fails and names the
  key.
- [Limits of pairs] Block programs, fixes that shift occurrence numbers
  and `check()` touched by a fix (D6, D7).

## Migration Plan

1. Infrastructure (`capture.[ch]`, `LIBSET`, the judge in `tools/lib` with
   its host tests, `libs-capture`/`libs-update`, the rule and
   `.gitattributes`) beside the tape.
2. Convert program groups, delete each tape in the commit that converts it.
3. Remove the tape code, `LIBSTAPE`, the replay copy, the old tests and
   rewrite the documents.
Rollback: a conversion commit is reverted alone (its tape comes back with
it).

## Open Questions

- Whether vallib's `Rnd` can be seeded for `riencryptlib` (D8), and
  whether the F00 phase of `ciatrklib` can be put into a state where
  original and C build agree; else `not captured: REASON` cases stand.
  Decided during implementation.
- The result of task 1.3 (size and literal CD data) may change D6's list
  of hashed programs.
- Whether `libs-update --fix` should also accept a hand-edit workflow only
  (no flag); the flag is a convenience.
