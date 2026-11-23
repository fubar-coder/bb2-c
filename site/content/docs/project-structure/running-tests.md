---
title: Running a single test
weight: 4
description: How to run one test that judges itself, or one case of it - directly with the test runner corpusrun, or through CTest with name filters, labels and presets - what to build first, what the output and the exit status mean, and where the files of a run are kept.
---

This page shows how to run one [test that judges
itself]({{< relref "self-judging-tests" >}}), or a single case of it,
while you write or debug it. There are two ways:

* **The test runner, directly.** One command compiles the case, runs it
  in the emulator and prints the verdict. It reads the source and its
  configuration each time, so a change needs no build step, and a new
  test needs no registration. This is the quickest way while you write a
  test.
* **CTest.** The build registers every case as a CTest test, so the
  normal test runs include it. CTest is the way to run many cases at
  once, or the same set the presets run.

Both give the same report and the same verdict, because CTest calls the
same runner.

## What to build first

A run needs the port's compiler and libraries, the test library, the
runner, a few small Amiga programs that start the compiler and the
program in the emulator, and a file that tells the runner where all of
them are. The normal build makes all of them:

```
cmake --preset amiga
cmake --build --preset amiga
```

If you only want to run tests that judge themselves, these targets are
enough; they make about half of what the whole build makes:

```
cmake --preset amiga
cmake --build --preset amiga --target blitz2_model_tests tedstub reqtrap rcput inject
```

`blitz2_model_tests` registers the tests of `tests/blitz2/` and builds
what that needs: the compiler, both library sets, the test library and
the host tools with the runner. For the tests in `tests/bug-orig/`, build
`bug_orig_model_tests` as well, and `bug_orig_sources` for tests that copy
test data into the session (`@TESTDATA@` in their configuration); without
the test data such a test is reported as skipped.

After the build, these are the files a run uses:

| File | What it is |
|---|---|
| `build/amiga/host/corpus-runner/corpusrun` | the test runner |
| `build/amiga/tests/model-env.toml` | the *environment file*: the paths of the compilers, the libraries, the test library, the helper programs and the work directory, written when CMake configures the build |
| `build/amiga/blitz2/` | the port's compiler and libraries |

The emulator runs need Amiberry in your `PATH` and the Kickstart and
Workbench files in `amiga-emulation/`
([Setting up the emulator tests]({{< relref "tests#setting-up-the-emulator-tests" >}})).
The side `originals` (below) also needs the Blitz Basic 2.1 CD, fetched
with `cmake -P cmake/fetch-originals.cmake`, and, for the cells with
Blitz Basic 2 version 1.95, that compiler, built from the original source
by the target `blitz2_ref`:

```
cmake --build --preset amiga --target blitz2_ref
```

When a file a cell needs is missing, the run does not fail: it is
skipped and names the file.

## Running a single test directly

The runner's command `case` runs one case of a test, in the cells of one
side. From the repository's root:

```
build/amiga/host/corpus-runner/corpusrun case --env build/amiga/tests/model-env.toml \
    --source tests/blitz2/ID.test.bas --case NAME --side port
```

The options:

| Option | Meaning |
|---|---|
| `--env FILE` | the environment file of the build, always `build/amiga/tests/model-env.toml` |
| `--source FILE` | the test source `ID.test.bas`; its configuration files are found from there |
| `--case NAME` | the case to run, the name in `!TestCase{NAME}`; without it every case of the source runs ([Running all cases of a source](#running-all-cases-of-a-source)) |
| `--side port` | the cells without original material: the port's compiler with the port's libraries (the default) |
| `--side originals` | the cells with an original compiler or the CD's libraries |
| `--require-deviation` | for a test in `tests/bug-orig/` (not in `regression/`): without a declared deviation the test is broken, as in its CTest test |

The runner preprocesses the source, writes the program that calls the
case (the *driver*), compiles and runs it in every cell of the side, and
judges the result files. Cells that need the same emulator settings
share one emulator session, unless the setting `isolate` gives them a
session of their own (`isolate = "on"` for every cell, or a list of cell
selectors such as `["libs=cd"]`), for an original that wrecks the memory
the next cell would run in. This is the case `calls` of a test whose
configuration makes `runtime-errors` an axis, so its side `port` has two
cells, `off` and `on`:

```
build/amiga/host/corpus-runner/corpusrun case --env build/amiga/tests/model-env.toml \
    --source tests/blitz2/intrinsic-asc.test.bas --case calls --side port
```

```
corpusrun case: emulator session 0: c0 c1
@stat v=1 test=intrinsic-asc__calls config=s0 machine=a1200 ... verdict=done
== intrinsic-asc.test.bas, case calls, side port: off (c0) on (c1)
intrinsic-asc.test.bas: case calls: PASSED
```

The line that starts with `==` names the cells of the run, each with a
short tag (`c0`, `c1`, ...) that the run's files use. The line that
starts with `@stat` gives timings of the session for the test
statistics; you can ignore it.

When a case does not pass, the report says why, cell by cell: each failed
check with its case, name, line of the source, the expression and the
expected and actual value; each assertion that does not hold, with the
file and line it comes from; a compile error at its line in the source. For a test `probe.test.bas` whose case `limits` has
two failing checks:

```
== probe.test.bas, case limits, side port: default (c0)
case limits, cell default:
  check limits mod (probe.test.bas:56) failed: -7 MOD -3: expected -1 ($FFFFFFFF), actual -4 ($FFFFFFFC)
  check limits word (probe.test.bas:58) failed: s$: expected "Help", actual "Hello"
probe.test.bas: case limits: FAILED
corpusrun case: the files of the run: .../build/amiga/tests/model-runs/probe__limits/run-xZKg6v
```

### The verdict

The line `ID.test.bas: case NAME: VERDICT` near the end of the report
gives the verdict, and the exit status says the same:

| Exit status | Verdict | Meaning |
|---|---|---|
| 0 | passed | every cell shows what it must |
| 1 | reproduced | a test of an open original bug: every cell shows the declared deviation (on the side originals the status is 0, see below) |
| 2 | error | a configuration, source or result that cannot be judged, or a wrong command line |
| 3 | broken | a precondition (`!Require`) failed, or a test of an original bug declares no deviation: the tested behaviour was not reached |
| 4 | failed | a check or an assertion does not hold |
| 77 | skipped | no emulator, or a file of the side is missing |

A case name that the source does not have is not refused before the run:
the compile of the driver fails with `Label not Found` and the verdict is
*failed*. A test without a cell of the side you ask for, for example
`--side originals` for a test that runs only with the port, is an error:
`... has no cell of the side originals`.

### Running all cases of a source

Without `--case` the runner runs every case of the source on the side,
and still judges each case on its own:

```
build/amiga/host/corpus-runner/corpusrun case --env build/amiga/tests/model-env.toml \
    --source tests/blitz2/model-numbering.test.bas --side port
```

To save time the cases share one run of a program that calls all of
them, and each case is judged from its own part of the result file. A
case runs on its own instead:

* when its configuration asserts something of the whole program (the
  exit code, a requester, the printed output, how the session ended, ...)
  or it announces a runtime error with `!ExpectError`: those observations
  belong to the whole run, not to one case;
* when the shared run did not end cleanly (the program ended inside a
  case, crashed or showed a requester): the case that was running then
  runs on its own, the others run again without it;
* when it did not pass in the shared run: its verdict on its own counts,
  as its CTest test would show it. If the two differ, the report says so,
  for example `case victim: PASSED alone (FAILED in the run shared with
  pass1, fails, behind)`: a case that depends on what another case left
  behind.

The report gives each case as for a single case, marked ` (alone)` when
it was judged on its own, and ends with a summary line such as
`model-numbering.test.bas: 2 cases, side port: PASSED`. The exit code is
that of the worst verdict, in the order error, broken, failed, skipped,
reproduced, passed (on the side originals a reproduced verdict gives 0).
The runs live in `build/amiga/tests/model-runs/ID/`
(`ID_originals`), one directory per run: `shared-0`, `alone-CASE`, and
`isolated-CASE` for the cells with a session of their own. CTest still
runs one case per test.

### The files of a run

Every run works in a directory of its own below
`build/amiga/tests/model-runs/`, named after the test and the case:

{{< filetree/container >}}
  {{< filetree/folder name="build/amiga/tests/model-runs" >}}
    {{< filetree/folder name="ID__CASE (ID__CASE_originals for the side originals)" >}}
      {{< filetree/folder name="run-XXXXXX" >}}
        {{< filetree/file name="driver.bas (the program the runner compiles)" >}}
        {{< filetree/file name="test.bb2 (the same, tokenised for the compiler)" >}}
        {{< filetree/folder name="data (the test's own data, prepared once)" state="closed" >}}{{< /filetree/folder >}}
        {{< filetree/folder name="c0 (one directory per cell, by its tag)" >}}
          {{< filetree/folder name="vol/out" >}}
            {{< filetree/file name="blitz2.out (what the compiler wrote)" >}}
            {{< filetree/file name="session.log (the log of the program that drives the compiler)" >}}
            {{< filetree/file name="test (the compiled program)" >}}
            {{< filetree/file name="test.res (the result file of the test library)" >}}
            {{< filetree/file name="run-K.out (what a program started with !RunProgram printed)" >}}
          {{< /filetree/folder >}}
          {{< filetree/folder name="emu" >}}
            {{< filetree/file name="amiberry.log (the emulator's log)" >}}
          {{< /filetree/folder >}}
        {{< /filetree/folder >}}
      {{< /filetree/folder >}}
    {{< /filetree/folder >}}
  {{< /filetree/folder >}}
{{< /filetree/container >}}

The directory `vol` is the volume `CORPUS:` the cell ran in. A run that
passes leaves nothing behind. A run that does not pass keeps its
directory and prints its path as the last line, so you can look at the
compiler's log, the result file or the program itself. It stays until
the next run of the same case starts, which removes it. Two runs at the
same time, for example by you and by a CTest run in the same build
directory, each get their own directory.

The result file has one line per check, with the values in hexadecimal.
For a failed string check and a passed long check it holds lines such as:

```
check	14	ok	$00000002	$00000002
checkstr	16	fail	Help	4	$A8B5F392	Hello	5	$F7D18982	-
```

You rarely need to read it: the report already gives each failed check
with its source line and values.

### Looking without running

Three options print or write something and start no emulator:

* **`--print-config`** prints the effective configuration of the test:
  the files read, every setting with its value and the file and line it
  comes from, the assertions and deviations, the axes and the cells with
  their side
  ([Seeing the effective configuration]({{< relref "self-judging-tests#seeing-the-effective-configuration" >}})).
  It needs no `--case`.
* **`--case NAME --print-driver`** prints the driver of one case: the
  macros of the test library, the source with every check numbered, and
  the calls of the batch init, the case and the batch cleanup at the end.
  This is the program the compiler sees; a compile error in the report
  points into the source, and this is where to look when it is unclear.
  Without `--case` it prints the driver of all cases, the text that
  `--write-driver` writes.
* **`--write-driver FILE`** writes the driver of all cases, tokenised as
  the compiler reads it, to `FILE`, with its text beside it as a `.bas`
  file. The build uses it to compare the compilers of two commits; it
  needs no `--case`.

```
build/amiga/host/corpus-runner/corpusrun case --env build/amiga/tests/model-env.toml \
    --source tests/blitz2/intrinsic-asc.test.bas --case calls --print-driver
```

`corpusrun case --help` lists all options, including the ones of the
comparison with a previous build, which only the build uses.

## Running tests with CTest

The build registers one CTest test per case and side. Every CTest command
here runs from the repository's root.

### When CTest knows a new test

CTest learns the tests from a list the build writes; it does not read the
sources itself. So after you add a test, rename it, add a case or rename
a case, **build before you run CTest**:

```
cmake --build --preset amiga
```

You do not need to run `cmake --preset amiga` again: the build notices a
new or renamed `ID.test.bas` or `ID.test.toml` by itself, configures
again and writes the list anew. Building only the target
`blitz2_model_tests` (or `bug_orig_model_tests`) is enough for that. Until
then, `ctest -N` shows the old list: a new test is missing, a renamed one
still has its old name. If the build refuses the source or its
configuration, for example a case after the batch cleanup, it stops with a
message naming the file and line, and the list stays as it was.

### The names

A CTest name is made from the directory's prefix, the test's name with
`-` written as `_`, two underscores, and the case:

| Name | Registered for |
|---|---|
| `blitz2_ID__CASE` | a test in `tests/blitz2/`, the side `port` |
| `bug_orig_ID__CASE` | a test in `tests/bug-orig/AREA/`, the side `port` |
| `regression_ID__CASE` | a test in `tests/bug-orig/regression/`, the side `port` |
| `..._originals` | the same case, the side `originals`: the cells with an original compiler or the CD's libraries |
| `blitz2_ID_prev` | the whole test: its program compiled by this build's compiler and by the previous commit's, and the two compared; only in a build configured with `-DBLITZ2_PREV=ON` ([the prev build](https://github.com/fubar-coder/bb2-c/blob/main/tests/README.md#the-prev-build)). The tests of the compiler in `tests/bug-orig/` get it too, as `bug_orig_ID_prev` and `regression_ID_prev` |

A case that has no cell of a side gets no test for it. For the test
`tests/bug-orig/libs/shapeslib-loadshape-anim-interleave.test.bas`, with
the case `interleave`, these are
`bug_orig_shapeslib_loadshape_anim_interleave__interleave` and
`bug_orig_shapeslib_loadshape_anim_interleave__interleave_originals`.

### Choosing tests by name

`-R REGEX` runs the tests whose names match the regular expression, `-E
REGEX` leaves out those that match. Since every name of a case ends in
the case's name or in `_originals`, a `$` at the end picks one side:

| To run | Command |
|---|---|
| one case, the side `port` | `ctest --preset all -R 'blitz2_intrinsic_asc__calls$'` |
| one case, the side `originals` | `ctest --preset originals -R 'blitz2_model_numbering__first_originals$'` |
| all cases of a test | `ctest --preset all -R 'blitz2_intrinsic_asc__'` |
| all tests in `tests/blitz2/` | `ctest --preset all -R '^blitz2_'` |
| all tests of an area of `tests/bug-orig/` | the area is not part of the name: use the beginning of the test names, such as `-R '^bug_orig_shapeslib_'` |
| a test without its side `originals` | `-R 'blitz2_model_numbering__' -E '_originals$'` |

`-N` lists the tests a command would run, without running them. Use it
to check a regular expression before a long run:

```
ctest --preset all -N -R 'blitz2_intrinsic_asc__'
```

### Presets and labels

A preset chooses tests by their *labels* and sets useful defaults: as
many tests at once as your machine has processors, the output of a
failed test in the report, and the emulator's logs. `-R` and `-E` narrow
a preset's choice further; they cannot add a test the preset leaves out.
The build gives the tests that judge themselves these labels:

| Label | Given to | Preset that runs it |
|---|---|---|
| `emulator` | the side `port` | `all`, `emulator` |
| `bug-orig` | in addition, the side `port` of a test of an original bug that is not fixed yet | `bug-orig` (`all` leaves it out) |
| `originals` | the side `originals` | `originals` |

So a fixed bug runs in `all`, an open one in `bug-orig`, and the cells
with original material only in `originals`. A test's configuration can
add labels of its own with the key `labels`. If you name a test that the
preset leaves out, `-N` lists nothing: choose the preset of its side.

Without a preset, give the build directory with `--test-dir`. Then no
label is excluded, and `-L REGEX` runs only the tests with a matching
label, `-LE REGEX` leaves them out:

```
ctest --test-dir build/amiga -L emulator -R 'blitz2_intrinsic_'
ctest --test-dir build/amiga -LE originals -R 'blitz2_model_numbering__'
```

### Snapshots: why a later run starts faster

Booting the emulated Amiga takes a few seconds, and a full run starts
hundreds of sessions. So the emulator does not boot for every session. It
boots once, saves the machine's state at the moment it waits for work (a
*snapshot*), and every later session with the same machine settings starts
from that saved state, in about a second. The test's own files and
programs are never part of a snapshot: the session sets them up after the
start, so one test cannot leave anything behind for the next.

What you notice is that the **first session of a configuration is
slower**: it boots, saves the snapshot and then restores it. This happens
once per emulator slot (one of the places that bound how many emulators
run at once) and per configuration, not once per run. Snapshots are
kept between runs and belong to the build directory, so a second run is
the fast one. They are small files (a few hundred kilobytes) that live in
the slot directories under `$XDG_RUNTIME_DIR/amiga-emulation-slots/`
(`/tmp/amiga-emulation-slots-UID/` without that variable).

The emulator decides by itself when to make, keep or drop one:

* **A configuration that only one session uses gets no snapshot.** Most
  configurations are used by a single test. A session saves a snapshot
  only if another session of the same configuration started within the
  last ten minutes; otherwise it boots, runs its test in that machine and
  saves nothing. Running the same test again within ten minutes counts as
  shared.
* **A snapshot is replaced when its inputs change.** A new Kickstart ROM,
  hard disk image, machine setting or emulator makes the old snapshot
  unusable. The next session of that configuration boots, saves a new
  one and removes the old.
* **A snapshot that does not restore is saved again or removed.** If a
  restored machine does not report in within 15 seconds, the session
  boots normally instead. A snapshot saved by that same session is
  removed and saved once more, at most twice. An older one is kept for
  one more try and removed when it fails again, so no slot keeps a
  snapshot that never starts.
* **Each slot keeps its 24 most recently used snapshots.**

If something looks odd, for example a session that keeps failing to
restore, you can clear the snapshots of a build directory while no tests
run. The next sessions then boot and save them again:

```
build/amiga/host/tools/amiga_run/amiga_run --purge-snapshots build/amiga
```

`AMIGA_EMULATION_SNAPSHOT=0` in the environment turns snapshots off, so
every session boots. The test statistics name what each session did
(for example `restored`, `first-save`, `single`, `cold`).

### Other useful options

* **`--output-on-failure`** prints the runner's report of a test that
  does not pass; the presets do it already. **`-V`** prints the output of
  every test and the command it runs, **`-N -V`** only the commands. The
  command line of a test is the runner's command from above, so you can
  copy it to run the case directly:

  ```
  ctest --test-dir build/amiga -N -V -R 'blitz2_intrinsic_asc__calls$'
  ```

* **`-j N`** runs `N` tests at once; the presets choose the number of
  processors. However many tests run, the number of emulators at the
  same time is limited for the whole machine, so a test may wait for a
  free emulator before it starts its session. The time it waits does not
  count against its time limit.
* **`--rerun-failed`** runs again the tests that failed in the last run
  of the same build directory, for example after a fix:
  `ctest --preset all --rerun-failed`.
* **`--timeout SECONDS`** has no effect on these tests: each has its own
  limit, made from the `timeout` of its cells. Change the `timeout` in
  the test's configuration instead.

CTest counts exit status 77 as skipped, and every other status but 0 as
failed. A test of an open original bug that comes out *reproduced* is
therefore a failure in CTest: that is what the preset `bug-orig` expects
of it. This holds for the side port only. On the side originals
(the tests ending in `_originals`) the original showing its own bug is
the expected result: the verdict line says `REPRODUCED (as declared,
side originals)` and the exit status is 0, so the test passes in the
preset `originals`. If the original does not show the declared
deviation, that side fails as well.

A test whose cells use the compiler 1.95 needs that compiler first;
CTest builds it by itself with the test `originals_refbuild`, which it
adds to such a run.

{{< callout type="info" >}}
While you write a test, run its case directly with the runner. Once it
passes, build, and run all its cases with
`ctest --preset all -R 'ID__'` (with `-` of the name written as `_`), and
the side `originals` with `--preset originals`.
{{< /callout >}}
