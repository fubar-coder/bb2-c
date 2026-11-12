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

The details of the format, as `capture.c` writes them (`capture.h`; the
judge parses the same):

- The header is `# capture NAME 1` (`1`: the version of the format); there
  is no closing line. Lines end with LF.
- A key part is printable where its byte is `$20`-`$7E`; the hex digits of
  `\xHH` are upper case. The occurrence number is counted per escaped
  `ROUTINE|CHECK|VECTOR` and is the part after the last unescaped `|`.
  A program has at most 114 688 distinct key parts (a fault beyond).
- Numbers `$XXXXXXXX`; bytes `bytes N HEX` with N decimal and upper-case
  hex (`bytes 0` for none), longer than 64 bytes `bytes N fnv XXXXXXXX`
  (FNV-1a 32 of the bytes); a record holding pointers (`OBSBYTES` for
  `SAMEPTRBYTES`) first has each long at any offset that points into the
  test program replaced by the bytes `PROG` (`50524F47`), up to 16 KB.
- Text `"..."` with `\"`, `\\` and `\xHH` for bytes outside `$20`-`$7E`
  (TAB as `\x09`); a NULL string is `null`.
- Addresses: 0 is `$00000000`, a value inside the test program's hunks or
  stack `prog`, else `NAME+$OFF` or `NAME-$OFF` relative to the base the
  program names (hex upper case, no leading zeros), without a base the
  plain number.
- Kept registers `kept` or `lost` with the lost ones in the order
  `d0`-`d7`, `a0`-`a6` (`lost d3 a1`).
- `not captured: REASON`, REASON with `\\` and `\xHH`.
- A block case (D6) is `BLOCK:N<TAB>COUNT XXXXXXXX first=KEY`: N counted
  from 1, COUNT decimal (4096 except for the last block before
  `cap_blocks(0)` or the end), XXXXXXXX the FNV-1a 32 of the block's
  observation lines `KEY<TAB>VALUE<LF>` as plain cases would be written,
  except that their occurrence number is the observation's number among
  the program's block observations (from 1), and `first=` the full key
  of the block's first observation. Its key has no `|`. A `not captured`
  case is a plain case in block mode too.
- `LIBS_CAP_DUMP` holds a full key with its occurrence number (no
  qualifier) or `BLOCK:N`; the run writes `NAME.dump` next to `NAME.txt`
  (the output stays comparable): for a bytes case lines
  `KEY<TAB>at $OFFS HEX` (OFFS four hex digits, 32 bytes a line), for a
  block its observation lines, whose hash is the block's.
- A fault of the writer (output not opened or written, a key longer than
  1023 bytes, too many distinct keys) fails the run through
  `bl_test_check` with `capture NAME: WHY`.

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

What the judge (task 2.3, `tools/lib/libscmp/libscmp.h`) adds to the
writer's details above, for the files it reads:

- The header is exactly line 1: NAME of `[A-Za-z0-9_.-]`, at most 63
  characters, the file's name without `.txt`. There are no other comment
  lines and no empty lines; the last line may lack its LF.
- One spelling per key, so that keys compare as text: in a key, `\xHH` is
  only for a byte outside `$20`-`$7E` that is not a TAB (a TAB is `\t`);
  a needless escape (`\x41`), another escape (`\n`), lower-case hex or a
  raw byte outside `$20`-`$7E` is a malformed line. A key has exactly four
  parts (the three escaped ones and the occurrence number, decimal from 1
  without a leading zero) or is `BLOCK:N` (N from 1 without a leading
  zero); the judge names block N as the block observations
  (N-1)*4096+1 to (N-1)*4096+COUNT.
- A qualifier (only in committed files) is one space after the key, then
  `[orig]` or `[fixed #N]` (N decimal from 1), then the TAB; one `[fixed]`
  case per key.
- A value is printable (`$20`-`$7E`), not empty. The judge compares values
  as text and reads only three forms: a value starting `not captured: `
  (two such values pass whatever the reasons; one against another value is
  a difference), a value `lost` or starting `lost ` (refused by
  `libs-accept`), and the COUNT of a block.
- The order of the cases does not count; a key is looked up.

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

Decided with the tool commands (`results libs-accept`, `libs-update`;
`tools/results/README.md`), before the targets of task 2.4:

- Accept writes the run's cases in the run's order. A key that has only
  an `[orig]` case in the file (a case of the original only) stays
  `[orig]`; an `[orig]` case whose key the run no longer has is dropped
  and reported. A `[fixed #N]` case whose key the run lacks is a case of
  the port only: it is kept, after the case it followed in the old file,
  and reported. "Identical" means the same bytes: the same cases in
  another order are not deterministic either.
- Accept and update are all or nothing over the programs of a call: if
  one program is refused, no file is written; an unchanged file is not
  rewritten. Without a filter, expected data of a program the runs do not
  have is refused (not silently kept).
- Update also takes `--only`. A `--fix KEY#N` (KEY as written in the file,
  split at the last `#`) must apply in exactly one program; one whose port
  value equals the expected value, or whose key is qualified already, is
  refused. A fix of a case the port no longer has makes `KEY [orig]`
  alone; a fix of a key only the port's run has adds `KEY [fixed #N]`
  alone after the case the run has before it. A `[fixed #N]` case the
  port's run lacks refuses the update.

Decided with the targets (task 2.4, `libs/tests/expected.cmake`):

- The filter and the fixes are cache variables of the build, as
  `FIX_SCOPE_BASE` of the target `fix-scope`: `LIBS_CAPTURE_FILTER` (a
  snitch filter of `libs_tests`, for both targets; the `-DFILTER` above)
  and `LIBS_UPDATE_FIX` (`KEY#N;...`, the `-DFIX` above), set with
  `cmake -D...` before `cmake --build --target libs-capture`; the
  target's comment names the filter, so a filter left in the cache shows.
  With a filter the tool gets `--only` for each program that wrote cases;
  one that wrote none fails the target.
- The second recording run has `LIBS_PAD=4104` (a page and 8 bytes; run 1
  has none). The runs stay in `build/<preset>/libs/tests/libs-capture/`
  (`run1`, `run2`) and `libs-update/run`.
- During the migration (D11) `libs-capture` runs the unconverted programs
  live (no tape mode: they compare the two sets as before), `libs-update`
  runs them in replay against their tapes (no CD); a failing run writes
  nothing. Group 8 removes the tape mode of `libs-update`.
- A skipped run (77: no Amiberry, no CD, no reference build of the RWE
  sources) fails the target with what it needs.
- `libs_capture_target` (host preset) runs `expected.cmake` with a fake
  emulator script (`libs/tests/selftest/fake-emulator.cmake`): a
  deterministic record, a refused non-deterministic one with the expected
  data unchanged, an update with a fix (and `LIBSET=obj` in the port's
  run), a skipped run, and a filter that leaves another program's file.

### D6. Block hashes for the very large programs and libtest

A program writes a case `BLOCK:N` with `COUNT FNV1A32 first=CHECK|VECTOR`
for each 4096 observations when it calls `cap_blocks(on)`; only
`rwe_ridisasmlib` (3.06 million observations), `systest_med` (3.57
million) and the two `libtest` programs do (user decisions of 2026-11-12).
`libtest` is hashed because of its size, not its count: its 41 459
observations per program are random 32-bit quotients and products, 1.87 MB
each and nearly incompressible, 49 % of the compressed text of all other
programs (task 1.3), and as hex pairs not readable anyway. Every other
program writes plain cases; `systest_mathstr` (56 861 cases) stays text
because its maths and string results are readable. A differing block names the block number and its observation range;
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

### D12. A fix changes only its own cases

A fix that updates expected data can change more than the cases of its
issue: a library routine shared by several commands, a changed register
use, a shifted occurrence number. Such a side effect is a change of
behaviour of its own and needs a review. Before the merge into `main` the
changed cases are therefore checked against the issues the branch names
(`Fixes #N`): only `[fixed #N]` cases of these issues and their paired
`[orig]` cases may change. `tools/results fix-scope` (the judge library of
D3) compares the expected data of `main` with the branch and lists every
other added, changed or removed case by key; the CMake target `fix-scope`
only exports the base files with git and calls it. The agent of a fix
runs it and names the result in its report; the coordinator runs it again
before the fast-forward. A reported case is not an error by itself: it is
reviewed and named in the commit with its reason, or it gets its own issue
and `[fixed #M]` case. The check works on the expected data of the shared
model, so the follow-ups for bug-orig and the corpus get it without a
second tool.

Decided with the check (task 2.7, `tools/results/README.md` "fix-scope"):
the base is the merge base of `BASE` and `HEAD` (exported with
`git archive`, the checkout untouched), the branch side is the working
tree of the checkout, the issues are the lines `Fixes #N` (alone on their
line) of the commits `BASE..HEAD`. The target takes the base from the
cache variable `FIX_SCOPE_BASE` (default `main`), which it passes to
`cmake/fix-scope.cmake` as `-DBASE`. A case is a key with its qualifier,
so a `[fixed #8]` that becomes `[fixed #7]` is a removal and an addition.
Allowed are a `[fixed #N]` case of a named issue, the `[orig]` case of
such a pair, and the unqualified case that became such a pair; a case the
fix removes (an `[orig]` case alone) carries no issue number and is
reported, to be named in the commit.

## The first conversion (group 2)

Facts of tasks 2.2-2.6 (2026-11-12):

- `capture.c` is linked into `libs_tests` with its static table as it is
  (2^17 entries, about 1.5 MB). The tapes that depended on the size of
  the program are fixed at their sites instead: `systest_mathstr`'s `SAME`
  and the table words of `rwe_blitzbomberslibtwo` pass their numbers
  through `tape_val` (no range test), and both tapes were recorded again
  from the originals; with `capture.c` linked every other tape still
  replays.
- `libtest` takes one library file and writes `libtest_NAME.txt`: one plain
  case `|routine count||1`, then block cases of the routines' `d0`
  (`ROUTINE|d0|$D0 $D1|N`), 11 blocks per program (41 458 observations,
  the same 20 729 inputs per routine as before). The seed of its random
  inputs is set at the start of each program; before, `lmullib` continued
  the sequence `ldivlib` left, so its inputs depended on the programs run
  before it (a filter for `lmullib` alone got other inputs).
- `libs_emulator_tape_wrong` corrupts the tape of `systest` (filter
  `[sys]`), `libs_tapes_compare_wrong` now changes `rwe_bb_blitlib.tape`
  and leaves out `rwe_blitzbomberslibtwo.tape` (the first two files in
  order), since the `libtest` tapes are gone.
- `libs_emulator_wrong` (filter `libtest ldivlib`, `CAPTURE_CORRUPT`):
  the run's `|routine count||1` changed to `$00000000` fails the compare
  with `libtest_ldivlib: |routine count||1: expected $00000002, got
  $00000000`.
- `libs-capture` with the filter `[libtest]` wrote both files; a second
  run wrote nothing ("2 unchanged"); `libs-update` with the same filter
  changed nothing.

## The framework macros (group 3)

Facts of tasks 3.1 and 3.2 (2026-11-12):

- A program switches to one set by `#define LIBS_ONE_SET` before it
  includes `sysload.h` (or `rwe/rwecmp.h`); the headers then offer only the
  one-set API (`loaded`, `load_set`, `OBS`..., `KEEPS`) and none of the
  two-set one (`sets`, `load_sets`, `cmp_preserve`, the `tape_*` calls,
  `SAME_FIXED`, `check_fixed`), so a half-converted program does not
  compile. Group 8 removes the switch with the two-set branch.
- `libs_tests.cpp` no longer begins a tape for every systest and rwe
  program: the program's first `load_sets` begins its tape, its first
  `load_set` its cases (`bl_prog_tape`, `bl_prog_cases`), and
  `run_systest` ends what was begun. A converted program needs no change
  in `libs_tests.cpp` or `CMakeLists.txt`.
- `OBS` and its variants and every `kept` case count as one check, so a
  program's summary line stays comparable with task 1.1's numbers. The
  first converted `systest` has 48 checks against 49: its old loop
  compared 23 values, of which the last was never set (0 on both sides).
- `KEEPS(routine, regs)` takes the registers as C text (`d2-d7|a0|a6`,
  `none`) and always adds a4 and a5 (the convention). A routine called
  without a stated set fails once (`KEEPS: no kept set stated for
  "ROUT"`); `kept_summary()` (and `rwe_done`) prints, per routine, the
  calls, the mask of the registers kept in all calls of the run's set and
  the `KEEPS(...)` line to paste, and marks a routine that changed a4 or
  a5 (a result: `DONTCARE`). The summary counts the registers before
  `DONTCARE`, so its line is never stricter than the original allows.
- `libs_emulator_lost`: `libs/tests/lost/datalib.c` is datalib whose
  thunks change d2 (a library file of `build/<preset>/libs/tests/lost/`,
  never in acidlibs; `prev_results` counts it as added). `emulator.cmake`
  takes it in place of `datalib.obj` (`OBJ_VARIANTS`), `systest` writes
  `lost d2` for both datalib calls, and the compare fails with
  `systest: datalib init|kept||1: expected kept, got lost d2`.
- `libs_emulator_tape_wrong` corrupts the tape of `systest_runtime` now
  (`[runtime]`): the tape of `systest` is gone.
- `rwe/rwecmp.h` in one-set mode: `R`, `IN`, `ARGS[6]`, `POP`, `CCR`
  without `[2]`, `ENT`/`ERR` on `loaded`, `DO1`/`DO1S` in place of
  `DO2`/`DO2S` (the call, then `preserve()`: the kept case and
  `OBS("popped", POP)`), `BOTH` as `OBS` of the truth value counted twice
  (its old count), `rwe_done` prints `kept_summary()`. The first converted
  rwe program is `riarrayslib` (task 4.2): 820 checks as in task 1.1, 818
  cases (two `BOTH`).
- `expected.cmake` passed `USERLIBS` to the runs as a list, so the runs got
  only its first entry; fixed (one argument, `;` escaped), and
  `libs_capture_target` checks that every entry arrives.

### Converting a program

The steps for the programs of groups 4-6, one program at a time; a
conversion touches only the program, its expected file and its tape (and
`tasks.md`), so programs can be converted in parallel.

1. Define `LIBS_ONE_SET` on the line before the first include of
   `sysload.h` or `rwecmp.h` (also before a direct include of `sysload.h`
   next to `rwecmp.h`).
2. One set: `load_sets(dir, names)` becomes `load_set(dir, names)`,
   `sets[cur]`, `sets[0]`, `sets[1]` become `loaded`. Remove every `cur`
   loop and every `[2]` array: `R[cur]` is `R`, `ARGS[cur][i]` is
   `ARGS[i]`, `POP[cur]` `POP`, `CCR[cur]` `CCR`, `buf[cur]` `buf`;
   a `for (cur = 0; cur < 2; cur++)` body runs once. `DO2`/`DO2S` become
   `DO1`/`DO1S`. A `rnd()` inside a removed `cur` loop ran twice per case;
   once now gives other inputs from there on (allowed: the expected data
   is recorded anew; the coverage stays). A systest program that runs `run(&sets[s], &res[s])` for
   both sets and compares afterwards writes each value with `OBS` where
   it is computed (`systest.c`).
3. Observations, with the old check text as `what`: `SAME` -> `OBS`,
   `SAMEBYTES(what, x, n)` -> `OBSBYTES(what, x, n)` (x the one buffer),
   `SAMEPTRBYTES` -> `OBSPTRBYTES`, `tape_strsame` and string compares ->
   `OBSSTR`, `check(..., tape_same(...))` -> `OBS`. `BOTH` stays. A check
   that compared the two sets (`check(w, o == c)`, `cmp_preserve`,
   `check_fixed`, `SAME_FIXED`) becomes an `OBS` of the value or the kept
   case; `check()` stays only for a condition that holds in every run of
   either set (the program's own consistency). A part under `tape_pause`
   becomes deterministic or `NOT_CAPTURED(what, "REASON")`; a
   `tape_orig()` condition becomes a condition of the own set.
4. Numbers and pointers: `OBS` writes a number as it is, never
   normalised. A value that is an address is written as an offset (`R.a[0]
   - (ULONG)buf`, as the programs already do) or with `OBSPTR(what, v,
   "NAME", base)`; a pointer into the test program is `OBSPTR(what, v,
   NULL, 0)` (written `prog`), a record holding such pointers
   `OBSPTRBYTES`. Only where the site names a pointer (the list of
   "Measurements (group 1)", task 1.2); a number the tape normalised by
   chance (`tape_val` sites, `blitzbomberslibtwo.c:50`,
   `systest_mathstr.c:272`, `:389`) is a plain `OBS`.
5. Keys: `ROUT`, the check text and `VEC` must not hold a value or an
   address, or the key changes with the result or the layout. Such a part
   moves into the value: `"value %d: orig $%08lX, C $%08lX"` becomes the
   check text `"value N"` (or a name) with the value as the case; an
   address in `VEC` becomes an offset or goes. Known sites:
   `rilesdebuglib.c:150`, `riwbspritelib.c:156`, `systest_runtime.c:351`,
   `systest.c:157` (done), `ricommoditieslib.c:374`, `systest_system.c`
   (about line 470). `VEC` holds the inputs of the case where they are
   data; a check repeated with the same key gets the next occurrence
   number by itself.
6. Kept registers: every `ROUT` that is called needs a `KEEPS(ROUT,
   regs)` statement (after `load_set`); a routine without one fails once
   (`KEEPS: no kept set stated`). Find the sets with a run against the
   original: state `none` first (or nothing), run `libs-capture` with the
   program's filter (below), and take the `KEEPS("...", ...);` lines from
   the summary in `build/amiga/libs/tests/libs-capture/run1/test/
   libs_tests.txt` (the original's intersection over all calls; a4/a5 are
   always added). A routine whose line says `a4/a5 changed` returns a
   result there: `DONTCARE` (bits 12/13) around that call, cleared after
   it. `KEEPS` matches `ROUT` by text: a routine with several `ROUT` names
   (forms) needs a line for each.
7. Block programs only (`systest_med`, `ridisasmlib`, D6): `OBS_BLOCKS(1)`
   around the observations to hash.
8. Record and check: build, then
   ```
   cmake --preset amiga "-DLIBS_CAPTURE_FILTER=rwe NAME"   # a snitch filter: [sys], "rwe NAME"
   cmake --build --preset amiga --target libs-capture       # CD; RWE refbuild: ctest --preset originals -R rwe_refbuild
   ```
   It writes `libs/tests/expected/NAME.txt` only if the runs with two
   memory layouts agree (else "test not deterministic" with the cases),
   and refuses a `lost` case (a stated set too strict). The program's
   summary line (`NAME: N checks`) must give task 1.1's number (`OBS` and
   each kept case count one, `BOTH` two); name a difference and its reason
   in the commit. Then `ctest --preset all -R '^libs_emulator$'` (the C
   build against the expected data, no CD) and `ctest --preset originals
   -R libs_emulator_originals`; reset the filter with
   `-DLIBS_CAPTURE_FILTER=` afterwards.
9. Commit the program, `libs/tests/expected/NAME.txt` and `git rm
   libs/tests/tapes/NAME.tape`; nothing in `libs_tests.cpp` or
   `CMakeLists.txt` (the program's first `load_set` opens its cases).
   Tests that need a particular tape move when it goes:
   `libs_emulator_tape_wrong` corrupts `systest_runtime.tape`,
   `libs_tapes_compare_wrong` the first two tapes in name order
   (`rwe_bb_blitlib`, `rwe_blitzbomberslibtwo`).

## Measurements (group 1)

Facts measured on 2026-11-12 with throwaway patches (not committed): the
original libraries run live in the record mode of the tape, every observation
written as a D4 case; seven runs of all 60 programs (two binaries, heap shift
0, 4104, 12296, 20488, 36872 bytes, one exact repeat). Details: the
coordinator's scratch file `libs-capture-group1.md`.

**Checks (task 1.1), for D11.** The programs' own summary lines, in the order
of `libs_tests.cpp`: `systest` 49, `systest_mathstr` 56864, `systest_runtime`
113, `systest_system` 178, `systest_cmdlibs` 2059, `systest_med` 11884;
`libtest` prints no `checks` line (41458 tests per tape file: 2 routines times
20729); the rwe programs: `bb_blitlib` 8143, `blitzbomberslibtwo` 6080,
`ciatrklib` 4606, `neilsreqtoolslib` 1238, `popupwinlib` 224, `riamosfunclib`
818, `rianimlib` 440, `riapplib` 602, `riarexxlib` 1463, `riarrayslib` 820,
`ribbmaplib` 4314, `richunkylib` 1318, `ricommoditieslib` 399,
`ricompactdisclib` 12193, `ricopperfxlib` 2260, `ridatatypeslib` 683,
`ridebuglib` 216, `ridisasmlib` 3058392, `ridisplaylib` 194, `ridoslib` 664,
`riencryptlib` 1202, `rievallib` 2049, `rifastseriallib` 270, `rifnslib` 7318,
`rifxlib` 348, `rigfxlib` 447, `rigtmenulib` 2154, `riintuilib` 215,
`rilesdebuglib` 8078, `rilinkedlistlib` 967, `rilvcallhooklib` 1217,
`rimapeditorlib` 2967, `rimapshapeslib` 2028, `rimisclib` 3334,
`rinonvolatilelib` 593, `ripacklib` 434, `riregistrylib` 866, `rireqlib` 120,
`riseriallib` 840, `rishapeslib` 574, `rishapezlib` 99, `risnowlib` 5107,
`risortlib` 1251, `risoundlib` 301, `risoundlib2` 3060, `ristcdecrunchlib`
1019, `ristonecrackerlib` 909, `rithreadlib` 196, `ritooltypeslib` 2594,
`ritrackdisklib` 951, `ritrackerlib` 13481, `riwbspritelib` 3720,
`rizonejoylib` 5405. Seven rwe programs and the six systest programs print
the line without a name. The numbers are the same in all seven runs. The
check count differs from the D4 case count (a string observation is one case;
`kept` is one case per call; `systest_med` has 3 567 195 cases for 11884
checks); the D4 case counts are in the scratch file.

**Addresses (task 1.2).** No observed value changes with the heap shift: the
programs already turn heap, library and `AllocAbs` addresses into offsets
before they observe them, so no current value needs `base+$OFF`. Pointers into
the test program are normalised by the range test of `tape_obs`
(`prog`): `neilsreqtoolslib.c:307`, `:573`, `riamosfunclib.c:199`,
`rinonvolatilelib.c:107`, `:109`, `:111`, `riregistrylib.c:146`, `:231`,
`:256`, `riapplib.c:207`, `rievallib.c:99`, `riseriallib.c:464`,
`systest_runtime.c:353`, and inside records (`SAMEPTRBYTES`)
`neilsreqtoolslib.c:297`, `:602`, `:614`, `rimisclib.c:204`, `:261`, `:271`,
`rireqlib.c:110`, `risortlib.c:122`; their raw values change with the binary.
Three programs have an address in the key (check text or vector), which
changes with the heap shift: `rilesdebuglib.c:150` (`D_HexWord` vector),
`riwbspritelib.c:156` (the `MoveWBSprite` vector holds a ViewPort address),
`systest_runtime.c:351` (the stack pointer in the check text). Four programs
put the two values into the check text: `systest.c:157`,
`systest_runtime.c:351`, `ricommoditieslib.c:374`, `systest_system.c` (about
line 470); a key must not hold a value. The range test normalises two kinds
of values wrongly: `blitzbomberslibtwo.c:50` (data words of the table; 7 of
6080 observations fall into the range; the committed tape of this program
differs from a recording with a binary of another size, segment 2, and equals
one with the committed layout) and `systest_mathstr.c:272`, `:389` (numeric
results in the range). A number must not be normalised (D4: `prog` only where
the site names a pointer). `capture.c`'s static buffers move the range too.

**Text size (task 1.3).** Written as D4 cases, 6 909 451 cases, 310.9 MB
plain; `rwe_ridisasmlib` 199.5 MB (gzip -9 9.7 MB) and `systest_med` 97.6 MB
(gzip 11.4 MB) as plain text, as D6 blocks 747 + 871 lines, about 100 KB. The
other 59 files: 283 865 cases, 13.86 MB raw (49 bytes a case; the 40 of the
estimate holds for `libtest` and `mathstr`), gzip -9 per file 1.93 MB, xz -9
1.36 MB; with the blocks about 13.96 MB raw and 2.0 MB compressed. The two
`libtest` files (1.87 MB each, 0.94 MB gzip together) and `systest_mathstr`
(2.3 MB, 0.33 MB gzip) are 43 % of the raw and 66 % of the compressed text;
without `libtest` 10.13 MB raw and 0.99 MB gzip are left; no other file is
above 1.1 MB. Literal CD data: none found. Hex buffers (up to 64 bytes, 22
programs) and strings (5 programs) are records filled in by the routines,
texts computed from the test's inputs, or the test's own names; of 54
original files (the CD's `ACIDLIBS`, the CD userlibs, the reference builds)
only 27 short hex values occur in the expected data, all zero runs or short
headers followed by zeros, and 20 strings that are public routine names.
`ridisasmlib`'s 382 298 text values (disassembly lines) occur nowhere in the
originals.

**Determinism (task 1.4).** In seven runs with six layouts no observation of
any program differs (normalised value, raw value, kept mask), apart from the
layout-dependent `blitzbomberslibtwo.c:50` hash above and the keys of the
three programs above. Not covered by that: `riencryptlib` under `tape_pause`
(16 observations: the kept mask of `Decrypt 5 (random)` alternates `$7EF1` and
`$7EF9` between runs, and the wheels differ in every run in both sets;
vallib's `Rnd` is hardware noise (VHPOSR, JOY0DAT, CIA timer,
`libs/acidlibs/vallib.c:623`) with no seed state; a `sysload_stubs` entry for
its token would be the way to a defined state). `ciatrklib`: in six runs
with printing (and the live check passed in the seven full runs) the
original's DMACONR after all 39 F00-phase calls is identical every time
(`$0 $8 $C $E $F`); a C-only run reads `$F` after 33 of the 39 and `$0` after
6, by construction (hardware against RAM copy), so the two sets cannot agree
there; that DMACONR is a live-only `check()` today, no observation. Not
measured: values that depend on date or timing (every run starts from the
same savestate).

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
  data. Measured: no literal CD data; with `systest_med`, `rwe_ridisasmlib`
  and `libtest` hashed about 10.2 MB raw and 1.0 MB compressed remain (D6).
- [Non-deterministic checks found late] The two-run record refuses them;
  task 1.4 lists today's before the conversion, so the cost is known.
  Timing-dependent ones are not found by the layout shift (D8).
- [Large rewrite of 60 programs by hand] A conversion can change what a
  scenario covers → the number of checks per program is kept and compared
  (D11); one commit per group; the compare must pass before and after.
- [Observations that held addresses] The tape found them by shifting the
  heap; the text format shows them → task 1.2 runs the compare with a
  padded binary and a shifted heap once, as the experiment of task 9.8;
  afterwards every `libs-capture` repeats it (D8). The normalisation has
  the opposite fault as well: a value that is no address but falls inside
  the test program's memory is written as `prog`, and whether it does
  depends on the layout. Linking `capture.c` (1.6 MB of static data) into
  `libs_tests` makes the tape replay of `rwe_blitzbomberslibtwo` fail
  (observations 1024..3071, first `table`; most likely the pseudo-random
  image longs of `sameimg`, compared through `tape_obs`, which normalises
  every value, not yet proven; task 1.2 checks it). So `cap_num` writes numbers as they are and only `cap_ptr` and
  `cap_ptrbytes` normalise; `cap_ptrbytes` is for records that hold
  pointers, never for random data. With the first conversion (task 2.6)
  `capture.c` is linked into `libs_tests`; the two tapes that normalised
  numbers pass them through `tape_val` ("The first conversion (group 2)").
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

- Whether vallib's `Rnd` can be seeded for `riencryptlib` (D8): measured,
  it has no seed state (hardware noise); a stub for its token through
  `sysload_stubs` is the remaining way to a defined state, else `not
  captured: REASON`. Whether the F00 phase of `ciatrklib` can be put into a
  state where original and C build agree: measured, no (hardware against RAM
  copy, by construction); `not captured: REASON` stands for it. See
  "Measurements (group 1)". Decided during implementation.
- Keys that hold values or addresses (four programs hold values in the check
  text, three hold an address in the check text or vector) and values the
  range test of `tape_obs` hits by chance: the conversions of their groups
  change the check texts and use `prog` only where a site names a pointer
  ("Measurements (group 1)").
- Whether `libs-update --fix` should also accept a hand-edit workflow only
  (no flag); the flag is a convenience.
