# How the tests work

This page explains the tests of this repository for people. It says why
they are built the way they are, shows the common model with small
examples, and walks through the jobs you will actually do. The complete
reference (every target, variable and file format) is
[../tests/README.md](../tests/README.md); the rules for changes are in
[../CLAUDE.md](../CLAUDE.md).

Contents: [Why](#why) · [The model](#the-model-in-plain-words) ·
[The suites](#the-suites) · [Presets and the CD](#presets-and-the-cd) ·
[Walkthroughs](#walkthroughs) · [Glossary](#glossary)

## Why

This project is a C translation of Blitz Basic 2. The original programs
(compiler, editor, libraries) are the definition of "correct". So the
tests ask one question again and again: does the port do what the
original does?

Two things follow.

* **The original is the source of the expected data.** We run the
  original once, write down what it did, and commit that. Later runs of
  the port are compared with the committed text. Everyone can run those
  tests without the original CD; only the people who record new expected
  data need it.
* **A fixed original bug is a deliberate difference.** The goal is
  compatibility with the bugs fixed. When the port gives another result
  than the original on purpose, the test says so: it keeps the original's
  behaviour and the fixed behaviour side by side, with the issue number.
  Nobody has to wonder whether a difference is a regression.

## The model in plain words

A test is a list of **cases**. A case has a **key** (its name, unique in
the test), an input, and an **expected output**. Running the test gives
an actual output for every case. The verdict is the difference between
the two, and a failure is reported by key, with both values.

The expected output is committed data. A test only compares; it never
writes it. Writing expected data is a separate command you run on
purpose and review with `git diff`.

### Example: a case

The library tests write one line per case, `KEY<TAB>VALUE`. From
`libs/tests/expected/rwe_riarrayslib.txt`:

```
FillArray|inline length|form 0 inline|1	$0000002E
FillArray|kept|form 0 type $0001 size 1 off 5 n 1 value $DE551B45|1	kept
```

The key has four parts: the routine (`FillArray`), what is checked
(`inline length`), the input (`form 0 inline`) and a counter that is 1
unless the same check with the same input happens again. The value is
`$0000002E`: the length word of the routine's inline code (the
compiler copies that many bytes of the command into the program). The second line says
that the routine kept the registers it must keep (`kept`; a failure reads `lost
d3 a1`).

If the C library gave `$0000002F`, the test would say:

```
rwe_riarrayslib: FillArray|inline length|form 0 inline|1: expected $0000002E, got $0000002F
```

### Example: both agree, or a fix

Where original and port agree, there is one case and both are judged
against it. When a fix changes a result, there are two, with the issue
number (the numbers here are made up):

```
FillArray|inline length|form 0 inline|1 [orig]	$0000002E
FillArray|inline length|form 0 inline|1 [fixed #123]	$00000030
```

`[orig]` is what the original does. It runs only against the original
(test `libs_emulator_originals`, needs the CD). `[fixed #123]` is what the
port does after the fix. It runs only against the port (`libs_emulator`,
no CD). If the port slips back to the original's value, the second case
fails. If the original's files change, the first case fails.

### Example: a block

Two library test programs make millions of observations. Written out they
would be hundreds of megabytes, so every 4096 observations are one line
with a hash:

```
BLOCK:1	4096 AEEEBB1A first=$CB00|d0|$00000000 $00000000|1
```

A differing block is reported by number and range of observations. A
block cannot be read by a person; the run can write its lines out
(see below).

### Words

To **record** is to write expected data from the original. To **update** is
to write it from the port (for `[fixed #N]` cases). A recording is only
accepted if two runs in a different memory layout give the same result;
otherwise the command says `test not deterministic` and writes nothing.

## The suites

All suites compare the port with an expectation. The library tests follow
the model above completely. The others keep their own form today.

| Suite | What it tests | Expected data | A fix is |
|---|---|---|---|
| **libs** (`libs_emulator`, `libs_emulator_originals`) | the C libraries, call by call, in the emulator | `libs/tests/expected/NAME.txt`, cases as above | the pair `[orig]` / `[fixed #N]` |
| **corpus** (`corpus`, `corpus_runerr`, ...) | Blitz programs compiled and run: output and executable | `tests/corpus/expect/`; the original compiler's results in `tests/data/` | a row in `tests/corpus/notes.tsv` naming the issue |
| **bug-orig** (`bug_orig_ID`) | one test per original bug | `ID.expect` (the correct behaviour) and `ID.orig` (what the original does) | `; @fixed #N` in the test's header: the test must then pass |
| **TED oracle** | TED's detokeniser, tokeniser and load/save against the original TED | `tests/data/ted-oracle.sha256` and others, generated by the original TED | an override row with the issue |
| **ted_ab** | TED-C against the original TED, scripted | expectations in the scripts, runs compared pairwise | `; @fixed #NNN` in the script |
| **prev** (`prev_results`) | a rework changes no result: the build is compared with the build of the previous commit | the previous commit itself | named in the commit (`-DPREV_INTENDED`) |

How they differ from the model:

* The corpus keeps the port's result in the committed file and the
  original's result in a note; the model keeps both as cases.
* bug-orig keeps the original's observation in a file recorded by a
  separate run; the model's recording also requires two agreeing runs.
* TED A/B and the oracle use their own forms.

Planned follow-ups (not done): bug-orig (the recording test becomes a
compare, a recording target accepts only agreeing runs, files that
legitimately vary get a marked exception) and the corpus (the notes rows
become `[orig]` / `[fixed #N]` cases). They will also get the fix-scope
check described below. `TODO.md` lists them.

## Presets and the CD

`ctest --preset NAME` selects tests; a preset says where they run.

| Preset | In short | Needs |
|---|---|---|
| `host` | tools and host tests; seconds | a compiler |
| `emulator` | tests in the Amiga emulator | Amiberry |
| `all` | `host` and `emulator`: the acceptance run. It starts no session of the original | Amiberry; the CD only for a few tests that read its sources (skipped without it) |
| `originals` | everything that compares with the original, and recordings | Amiberry and the CD (some also the reference build of the RWE sources) |
| `bug-orig` | open original bugs; they fail on purpose | Amiberry |
| `oracle` | the compiler checks of the AmiBlitz3 tests | Amiberry and the fetched AmiBlitz3 files |

So: **`all` is what you run for every change and needs no CD.** The CD
(`cmake -P cmake/fetch-originals.cmake`) is needed to check the expected
data against the originals and to record. Details and the full list:
`tests/README.md`, "Original material and presets".

## Walkthroughs

### I fixed an original bug. What do I change and run?

1. One issue for the bug. Put `Fixes #N` on its own line in the commit
   message.
2. Fix the code. Change the comment `BUG(orig)` into `Fixed (#N)`.
3. Change the test so that it expects the correct behaviour:
   * a library test: write the pair of cases. Say which case changes and
     let the update write it:
     ```
     cmake --preset amiga "-DLIBS_UPDATE_FIX=KEY#N"
     cmake --build --preset amiga --target libs-update
     ```
     `KEY` is the key as written in the expected file. Then `git diff
     libs/tests/expected`: only the pair should appear.
   * a bug-orig test: add `; @fixed #N` to its header.
   * corpus or TED: the row or override with the issue number
     (`tests/README.md`).
4. Add the entry to `docs/*/deviations.md` as an intended deviation (kind
   F).
5. Run `ctest --preset all`. The fixed test must pass.
6. **Run the fix-scope check:**
   ```
   cmake --build --preset amiga --target fix-scope
   ```
   It compares the expected data of your branch with `main` and lists every
   case that changed and is not a `[fixed #N]` case of an issue in your
   `Fixes #N` lines (or its `[orig]` partner). Empty is good. A listed
   case means your fix also changes something else: name it in the commit
   message with the reason, or give it its own issue. Say the result in
   your report. The coordinator runs it again before the merge.

The site page and issue labels are the coordinator's job (`CLAUDE.md`).
Do not change a test's inputs so that it no longer meets the bug.

### A test fails. How do I read it and find the cause?

Start with the key in the message:

```
libtest_ldivlib: |routine count||1: expected $00000002, got $00000000
```

It names the program (`libtest_ldivlib`), the check, and both values. The
rest of the run is in the build directory:

* the port's whole output: `build/amiga/libs/tests/libs_emulator/test/captures/NAME.txt`.
  Compare with `diff -u libs/tests/expected/NAME.txt` against it. For the
  original's run: `libs_emulator_originals/` instead.
* the program's log: `test/libs_tests.txt` next to `captures/`. It also
  lists the kept registers per routine.
* a long buffer shows only a hash (`bytes 200 fnv 7FA26772`). To see its
  bytes, run with the variable `LIBS_CAP_DUMP` set to the key (with the
  counter) and read `captures/NAME.dump`. For a block, set it to
  `BLOCK:N`, and you get the 4096 observations of that block. (The
  emulator script has no option for it yet: add a `Set LIBS_CAP_DUMP KEY`
  line to `vars_cmd` in `libs/tests/emulator.cmake` for a debugging run.)
* to run one program only, use a snitch filter, for example
  `cmake --preset amiga "-DLIBS_CAPTURE_FILTER=rwe riarrayslib"` and the
  target `libs-update` (it leaves other programs' files alone; set the
  variable empty afterwards).

If the original's test (`libs_emulator_originals`) fails but the port's
passes, the original changed or the scenario changed without a new
recording.

### I add or change a test program. How do I record?

A library test program lives in `libs/tests/` (`rwe/NAME.c` for an RWE
library). Write an observation with `OBS(what, value)`; use `OBSBYTES`
for a buffer and `OBSPTR` for an address (never write an address as a
plain number: it changes with the memory layout). Keep the input in the
key and the result in the value. Use `KEEPS(routine, regs)` to state which
registers the routine keeps.

Then record from the original (needs the CD and the RWE reference
build, `ctest --preset originals -R rwe_refbuild`):

```
cmake --preset amiga "-DLIBS_CAPTURE_FILTER=rwe NAME"
cmake --build --preset amiga --target libs-capture
cmake --preset amiga -DLIBS_CAPTURE_FILTER=
git diff libs/tests/expected
```

Run `ctest --preset all -R '^libs_emulator$'`: the C build must give the
same cases. Commit the program and its expected file together. If the
number of checks printed in the summary line changed, name the reason in
the commit message.

### The record says "test not deterministic"

The record runs the program twice with a different memory layout. If a
case differs, you get:

```
test not deterministic: NAME
NAME: KEY: run 1 VALUE, run 2 VALUE
```

and nothing is written. The cause is a value that depends on where
things are in memory, or on chance. Typical fixes:

* an address in the value: write it with `OBSPTR`, or as an offset from a
  buffer;
* an address or a result in the key: move it into the value;
* random numbers: set a fixed seed, or stub the source;
* a part that cannot be made the same (the original writes to hardware,
  the C build to a copy): mark it `NOT_CAPTURED(what, "reason")`. The
  judge skips such a case on both sides, so use it only when nothing
  else works.

The two runs only vary the memory layout. A value that depends on the
date or on emulator timing can agree twice by chance; if
`libs_emulator_originals` fails on another day, that is the sign.

### The record says "lost"

A stated kept register (`KEEPS`) shows `lost d3` in the original's run:
the set you stated is too strict. The summary at the end of
`test/libs_tests.txt` lists, per routine, the registers kept in all calls
and the `KEEPS(...)` line to paste.

## Glossary

* **Case**: one checked thing: a key and an expected value.
* **Key**: the name of a case, `ROUTINE|CHECK|INPUT|COUNTER`. Unique in a
  test; a failure is reported by it.
* **Expected data**: the committed values the tests compare with
  (`libs/tests/expected/`). Never called "the reference".
* **Recorded original**: expected data that was written from the original.
* **Record**: write expected data from the original (`libs-capture`);
  accepted only if two runs agree.
* **Update**: write expected data from the port (`libs-update`), for
  `[fixed #N]` cases.
* **`[orig]`**: a case that only the original must pass (label
  `originals`).
* **`[fixed #N]`**: a case that only the port must pass; issue `#N` is
  the fix.
* **Block**: one case that stands for 4096 observations of a very large
  program; its value is a hash (`BLOCK:N`).
* **Not captured**: a case that says why a value cannot be reproduced;
  it is skipped on both sides.
* **Kept registers**: the registers a library routine must leave
  unchanged; the test states them per routine.
* **`ref`**: the original program (corpus configuration `ref`,
  `ted_ref`, `blitz2_ref`), as opposed to the expectation.
* **prev**: the build of the previous commit; a rework must not change any
  result compared with it.
* **Fix-scope**: the check that a fix changes only its own cases.
