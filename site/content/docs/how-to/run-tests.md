---
title: Run a single test or one preset
weight: 6
description: Run the tests of the BB2 C port with CTest - one preset, or a single test by its name - what each preset needs, how to find a test's name, and where the results and logs go.
next: /docs/blitz2-internals
---

The BB2 C port has a few thousand tests. They run with CTest, from the
repository's root, and are grouped into *presets*: a preset chooses a
set of tests and the settings to run them with. This page shows how to
run one preset, how to run a single test, and where to find what a run
left behind. What the tests compare, and how to set up the emulator for
them, is on [Tests]({{< relref "/docs/project-structure/tests" >}}).

## Before you start

Build the port first: the tests use what the build made, and the build
registers them with CTest.

```
cmake --preset amiga && cmake --build --preset amiga
```

Most tests run the Amiga programs in Amiberry and need the emulator set
up ([Setting up the emulator tests]({{< relref "/docs/project-structure/tests#setting-up-the-emulator-tests" >}})).
Some compare with the original Blitz Basic 2 and need the original
material, which one command downloads into `archive/`:

```
cmake -P cmake/fetch-originals.cmake
```

A test whose emulator or material is missing is reported as *skipped*,
not as failed.

## Run one preset

| Preset | Runs | Needs |
|---|---|---|
| `host` | the tests on your machine: the tools with deliberately broken inputs, the editor's core, the declarations of every library against the original's, the token lists; a few seconds | the build |
| `emulator` | the tests in the emulator: the libraries, the compiled programs, the editor, AB3 mode, the extensions and the fixed bugs of the original | Amiberry and the emulator setup |
| `all` | `host` and `emulator`: the run for every change | as `emulator`; without the CD it skips only the compilation of the CD's own sources |
| `bug-orig` | the bugs of the original that are not fixed yet, and known defects of the port; these tests fail on purpose | as `emulator`; the CD for the TED tests, for the configuration `port` and for the tests that mount `BB2:` |
| `originals` | the same scenarios with the original compiler, editor and libraries, and the checks of the recorded data against them | Amiberry and the CD |
| `oracle` | AmiBlitz3's own compiler on the test programs of AB3 mode, to check their expected results | Amiberry and the AmiBlitz3 files of the original material |
| `perf` | the benchmark of the libraries: the cycles each command needs, against a budget | Amiberry |

Give the preset's name to CTest:

```
ctest --preset host
ctest --preset all
```

A preset runs as many tests at once as your machine has processors and
prints the output of every test that does not pass. A full run of `all`
starts hundreds of emulator sessions and takes a while; for a quick check
after the first build, run `host`.

## Find a test's name

`-N` lists the tests a command would run, without running them, and
`-R` chooses the tests whose names match a regular expression:

```
ctest --preset all -N -R 'blitz2_intrinsic_asc__'
```

Without `-R`, `-N` lists the whole preset, and the last line counts its
tests. Many names start with the group of tests they belong to, for
example `blitz2_` for tests of the compiler,
`bug_orig_` for the bugs of the original, `ab3_cli_` for AB3 mode and
`perf_` for the benchmark.

## Run a single test

Give the preset and the test's name, with `$` at the end so that no
longer name matches as well:

```
$ ctest --preset all -R 'blitz2_intrinsic_asc__calls$'
    Start 3182: blitz2_intrinsic_asc__calls
1/1 Test #3182: blitz2_intrinsic_asc__calls ......   Passed    1.63 sec

100% tests passed out of 1
```

A preset leaves out the tests of other presets: `-R` narrows its choice
but cannot add to it. If `-N` lists nothing for a name you know, the
test belongs to another preset. The tests of the original's open bugs,
for example, run only in `bug-orig`, where a test that shows the bug
fails on purpose and says `REPRODUCED`:

```
$ ctest --preset bug-orig -R 'bug_orig_compiler_quick_negative_const__negative$'
...
compiler-quick-negative-const.test.bas: case negative: REPRODUCED
corpusrun case: the files of the run: .../build/amiga/tests/model-runs/compiler-quick-negative-const__negative/run-kFUUIc
...
The following tests FAILED:
	2592 - bug_orig_compiler_quick_negative_const__negative (Failed) bug-orig emulator
```

Without a preset, name the build directory with `--test-dir`. Then no
test is left out, and `--output-on-failure` prints the output of a test
that does not pass, as the presets do:

```
ctest --test-dir build/amiga -R 'blitz2_intrinsic_asc__calls$' --output-on-failure
```

The same works for one benchmark program, for example
`ctest --preset perf -R '^perf_bcmp$'`.

### Repeat a test that fails now and then

`ctest --repeat until-fail:N -R NAME` repeats a test up to N times. It also
repeats the setup test a fixture needs, such as `rwe_refbuild`, the
reference build of the RWE sources, with every run. After one normal run,
add `-FA '.*'` (fixtures are not added automatically) so that only the named
test is repeated; the fixture must be in place from that first run.

### The tests of the host tools

The tests of the tools that run on your machine, such as bbsrc, live in a
build directory of their own, `build/amiga/host`; in `build/amiga` they
are a single test named `host`. To run one of them, name that directory:

```
$ ctest --test-dir build/amiga/host -R '^bb22' --output-on-failure
...
2/2 Test #63: bb22 .............................   Passed    0.00 sec

100% tests passed out of 2
...
The following tests did not run:
	 29 - bb22_rwelibsrc (Skipped)
```

Here `bb22_rwelibsrc` is skipped because it reads the Blitz Basic 2 CD,
which was not downloaded.

## Where the results go

Each preset writes a JUnit file, the format that most continuous
integration systems read, into the build directory:
`build/amiga/junit-host.xml` for `host`, `build/amiga/junit-all.xml` for
`all`, and so on. `--output-junit FILE` writes it to another name,
relative to the build directory, also without a preset:

```
ctest --preset all -R 'blitz2_intrinsic_asc__calls$' --output-junit one.xml
```

writes `build/amiga/one.xml`.

CTest keeps the full output of the last run in
`build/amiga/Testing/Temporary/LastTest.log`
(`build/amiga/host/Testing/Temporary/` for the host tools). A test that
runs in the emulator and does not pass keeps its files and prints where
they are, as the line `the files of the run:` above shows: the program,
the compiler's output and the emulator's log. A test that passes leaves
nothing behind.

After a fix, run only the tests that failed in the last run of the same
build directory:

```
ctest --preset all --rerun-failed
```

[Running a single test]({{< relref "/docs/project-structure/running-tests" >}})
explains the names, the labels and the files of the tests that judge
themselves in detail, and how to run one of them without CTest.
