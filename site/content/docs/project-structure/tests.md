---
title: Tests
weight: 2
description: The test runs of the BB2 C port, what they compare with what, how to set up the emulator tests, and the runner for your own scripts in the emulator.
---

Before this page you should have built the port (see the
[Introduction]({{< relref "/docs/introduction" >}})).

The tests check that the port does what the original does. Some run on
your machine in seconds; most run the Amiga programs in an emulator, side
by side with the originals.

## The test runs

All tests run with CTest, from the repository's root, in four presets:

| Preset | Runs | Needs |
|---|---|---|
| `host` | the tools with deliberately broken inputs, the tests of the editor's core, the comparison of every library's declarations with the original, the token lists | your machine; the original material for some |
| `emulator` | the libraries against the originals, the corpus, the editor against the original editor, AB3 mode, the extensions | Amiberry and the emulator setup; the original material for most |
| `all` | both of the above | both |
| `bug-orig` | the tests of the original's bugs and of known defects of the port, which fail on purpose | as `emulator` |

```
ctest --preset host
ctest --preset emulator
```

A test that cannot run, because the emulator or the original material is
missing, is reported as skipped, not as failed. So `ctest --preset host`
right after the first build is a quick check that everything was built
correctly.

## What the emulator tests compare

* **The corpus** is a set of small Blitz Basic programs. Each is compiled
  by the original compiler with the original libraries, by the port's
  compiler with the original libraries, and by the port's compiler with
  the port's libraries; the executables of the first two must be
  identical, and all three programs must give the same output
  ([Comparing with the original yourself]({{< relref "/docs/compared-with-original/comparing" >}})).
* **The CD's sources** are all compiled by the port and compared with a
  table made by the original compiler.
* **The libraries** are loaded one by one, the port's and the original,
  and their routines are called with the same arguments; results,
  registers and memory are compared.
* **The editor** runs scripts of key presses and mouse actions, once in
  the original editor and once in the port's, and their screens,
  requesters and files are compared.
* **AB3 mode** compiles test programs from the command line and compares
  their output with expected files, and with what AmiBlitz3's own compiler
  gives.

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
emulator test by name, for example `ctest --preset emulator -R '^corpus$'`.
{{< /callout >}}

The full description of every suite, with the options of the runs, is
the repository's
[description of the tests](https://github.com/fubar-coder/bb2-c/blob/main/tests/README.md).
