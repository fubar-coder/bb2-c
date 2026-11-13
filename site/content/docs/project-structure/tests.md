---
title: Tests
weight: 2
description: The test runs of the BB2 C port, what they compare with what, the expected data recorded from the original, how fixed bugs are tested, how to set up the emulator tests, and the runner for your own scripts in the emulator.
---

Before this page you should have built the port (see the
[Introduction]({{< relref "/docs/introduction" >}})).

The tests check that the port does what the original does, and, where a
bug of the original has been fixed, that the port does what is correct.
Some run on your machine in seconds; most run the Amiga programs in an
emulator.

What the original does is written down once and committed with the
tests: the *expected data*. The normal test runs compare the port with
it and never start the original, so you can run them without the
original material. A separate run checks the expected data against the
originals again; only that run and the recording of new expected data
need the Blitz Basic 2.1 CD.

## The test runs

All tests run with CTest, from the repository's root, in these presets:

| Preset | Runs | Needs |
|---|---|---|
| `host` | the tools with deliberately broken inputs, the tests of the editor's core, the comparison of every library's declarations with the committed data of the original, the token lists | your machine |
| `emulator` | the libraries, the corpus, the editor, AB3 mode, the extensions and the fixed tests of the original's bugs, each against its expected data | Amiberry and the emulator setup; the CD only for the compilation of the CD's own sources |
| `all` | both of the above: the run for every change | both |
| `originals` | the same scenarios with the original compiler, editor and libraries, against the same expected data, and the recordings | Amiberry and the CD |
| `bug-orig` | the tests of the original's bugs not fixed yet and of known defects of the port, which fail until the bug is fixed | as `emulator`; the CD for a few |
| `oracle` | AmiBlitz3's own compiler, run on the test programs of AB3 mode, to check that their expected results are right | Amiberry and the AmiBlitz3 files of the original material |

```
ctest --preset host
ctest --preset all
```

A test that cannot run, because the emulator or the original material is
missing, is reported as skipped, not as failed. So `ctest --preset host`
right after the first build is a quick check that everything was built
correctly.

## What the emulator tests compare

* **The corpus** is a set of small Blitz Basic programs. The port's
  compiler compiles each of them with the port's libraries and runs it,
  and its output must be the program's expected output. The `originals`
  run compiles the same programs with the original compiler and the
  original libraries, with the port's compiler and the original
  libraries, and with the original compiler and the port's libraries:
  the executables of the first two must be identical, and every output
  must be the expected one
  ([Comparing with the original yourself]({{< relref "/docs/compared-with-original/comparing" >}})).
* **The CD's sources** are all compiled by the port and compared with a
  table made by the original compiler.
* **The libraries** are tested one library set per run: a test program
  loads either the port's libraries or the original ones, calls their
  routines, and writes every result, register and buffer it observes as
  a *case*. The cases must be the expected data, which was recorded from
  the original libraries (below).
* **The editor** runs scripts of key presses and mouse actions in the
  port's editor; its screens, requesters and files must meet the
  expectations written in the scripts. The `originals` run also plays
  the scripts in the original editor and compares the runs with each
  other.
* **AB3 mode** compiles test programs from the command line and compares
  their output with expected files; the `oracle` run checks those files
  with AmiBlitz3's own compiler.
* **The fixed bugs** nearly all have a test program with the correct
  behaviour as its expectation. The port, with its own libraries, must
  show that behaviour; the original is not needed for these tests.

## The expected data of the library tests

A library test program sees one library set, the port's or the
original's, and writes one line per case: a key that names the call and
its input, and the value it observed.

```
FillArray|inline length|form 0 inline|1	$0000002E
FillArray|kept|form 0 type $0001 size 1 off 5 n 1 value $DE551B45|1	kept
```

The expected data is one such file per test program, in the repository.
The normal run checks the port's libraries against it; the `originals`
run checks the original libraries against the same file. A case on which
both agree is one line and holds for both. How to read a line and what
its parts mean is on a page of its own,
[Reading library test results]({{< relref "library-test-cases" >}}).

Where a fixed bug of the original changes a result, its key has two
lines: one marked `[orig]` with the original's value, and one marked
`[fixed #N]` with the port's, where `N` is the bug's issue. The original
is checked only against the first, the port only against the second, so
the port's case fails if the fix is undone.

The tests only compare; they never write expected data. Two build
targets do that on purpose, and their result is reviewed with
`git diff` before it is committed:

* **Recording** (`libs-capture`) writes the expected data from the
  original libraries. It needs the CD, and for the RWE libraries their
  original build, made from their sources by the original compiler. It
  runs every program twice with a different memory layout and writes
  nothing if the two runs differ, so that no value that depends on where
  things lie in memory gets into the file.
* **Updating** (`libs-update`) writes the `[fixed #N]` cases from the
  port's libraries, after a fix. It needs no CD.

The tests of the original's bugs follow the same idea. Each keeps two
files beside its program: the expectation, which describes the correct
behaviour, and the *recorded original*, what the original compiler and
libraries did with the program. The recording (`bug-orig-record`) runs
the original twice and writes the file only if both runs agree, like
`libs-capture`. When a bug is fixed, the recorded original stays: the
port is then judged against the expectation alone, and the `originals`
run still checks that the original behaves as recorded
([A test for a bug of the original]({{< relref "/docs/compared-with-original/comparing#a-test-for-a-bug-of-the-original" >}})).

So you need the CD only to record new expected data or to check it
against the originals. Everything in `ctest --preset all` runs without
it, apart from the compilation of the CD's own sources.

## Setting up the emulator tests

The emulator tests run Amiberry without a window, several sessions at
once. They need:

* **Amiberry** in your `PATH`.
* **Your own Kickstart ROMs and Workbench hard disk images** in the
  directory `amiga-emulation/` of the repository. They cannot be part of
  the repository. The images must be prepared for the tests: at start
  they run a script from the test's own directory, and they are mounted
  read-only, so that several sessions can share them.

The repository's
[guide to the emulator setup](https://github.com/fubar-coder/bb2-c/blob/main/AMIGA-EMULATION-SETUP.md)
lists the files, the machines the tests use and the conditions the images
must meet. Besides the default machine there are older ones, with a 68000
and Kickstart 1.3, for bugs that only show there.

The number of emulators running at the same time is limited over the
whole machine, for all test runs together. The guide explains how to
change the limit while tests run.

## Running your own script in the emulator

The tests start the emulator through a small runner, which you can use
too. It boots the emulated Amiga without a window, runs an AmigaDOS
script, and prints the files the script wrote. The script's directory is
the volume `TEST:`, and further host directories can be mounted as
volumes of their own. After the build it is the program `amiga_run` in
the host tools of the build directory.

This compiles a Blitz Basic program with the port and runs it, all
without a window:

```
build/amiga/host/tools/amiga_run/amiga_run \
    --mount-ro BB2C=build/amiga/blitz2 --mount-ro SRC=mysources mysources/run.script
```

with the program `hello.bb` and this `run.script` in the directory
`mysources`:

```
Assign blitz2: BB2C:
Cd SRC:
blitz2:blitz2-ab3 >TEST:out.txt -s hello.bb -e RAM:hello
RAM:hello >>TEST:out.txt
```

The runner prints `out.txt` when the session has ended. `--visible`
shows the emulator's window, which helps when something hangs.

{{< callout type="info" >}}
Try it: run `ctest --preset host`; after the emulator setup, run a single
emulator test by name, for example `ctest --preset all -R '^corpus$'`.
{{< /callout >}}

The full description of every suite, with the options of the runs, is
the repository's
[description of the tests](https://github.com/fubar-coder/bb2-c/blob/main/tests/README.md).
