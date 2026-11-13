---
title: Comparing with the original yourself
weight: 5
description: Compile your own program with the original compiler and with the port, compare the results, and write a test for a bug you found.
next: /docs/ab3-mode
---

Before this page you should have built the port and run its tests
([Tests]({{< relref "/docs/project-structure/tests" >}})).

The claims of this chapter are checked by tests that compare the port
with what the original Blitz Basic 2 does. You can use the same
machinery for your own programs: to see whether a program behaves the
same with both, or to show a bug you suspect in the original.

## What you need

* A build of the port and its emulator tests set up: Amiberry, your own
  Kickstart ROM and the hardfiles the tests boot. The tests are skipped
  without them.
* The original material, which the repository does not contain: the
  Blitz Basic 2.1 CD image and the Aminet archive of the original sources.
  One command downloads and unpacks them, once:

  ```
  cmake -P cmake/fetch-originals.cmake
  ```

  The normal test runs need no original material: they compare the port
  with expected data that was recorded from the original. But running the
  original on your own program, or recording what it does, needs it.

## Your program in the corpus

The *corpus* is a set of small Blitz Basic programs, kept as plain text,
each with the output it is expected to give. The normal test run compiles
every program with the port and its libraries and compares the output
with the expected one. The comparison with the original is a run of its
own, in the preset `originals`, which compiles every program three
times:

| Run | Compiler | Libraries | Must give |
|---|---|---|---|
| ref | the original blitz2 2.10 | the original libraries | the expected output |
| port | the port's blitz2 | the original libraries | the same executable as ref, and the expected output |
| orig-clib | the original blitz2 2.10 | the port's libraries | the expected output |

So it checks the compiler (the executable must be identical) and the
libraries (the program must do the same) separately.

{{% steps %}}

### Write the program

Write your program as a text file with the extension `.bas`, for example
`t900_mine.bas`. Let it print what it computes with `NPrint`: the output
is what the runs compare. End it with `End`.

### Put it into the corpus

Copy it into `tests/corpus/tests/` for a program compiled without runtime
error checking, or into `tests/corpus/tests-runerr/` for one with it. The
build picks up every `.bas` file there by itself:

```
cmake --build --preset amiga
```

### Write its expected output

The expected output is written from the port's run, by a build target:

```
cmake --build --preset amiga --target corpus-expect-update
```

It needs the CD and takes about a minute and a half. It leaves the files
of the other programs as they are and writes
`tests/corpus/expect/corpus/t900_mine.out` (`corpus_runerr` in place of
`corpus` for the other directory). Read it: it is what the port's
program printed, followed by its return code.

### Run the comparison with the original

```
ctest --preset originals -R '^corpus_originals$'
```

(`corpus_runerr_originals` for the directory with runtime error
checking.)

### Read the report

The report is written to `build/amiga/tests/corpus/corpus_originals.txt`,
one line per program. A program that behaves the same with both looks
like this:

```
t03_str          ref: output as expected; port: exe identical, output as expected; orig-clib: output as expected
```

{{% /steps %}}

If a run differs from the expected output, the report says `OUTPUT
DIFFERS` for it (or `EXE DIFFERS` for the port's executable). Since the
expected output came from the port, a difference of `ref` means that the
original behaves differently. If it is not on the list of fixed bugs,
you have found a difference: please open an issue with the program and
the report.

## A test for a bug of the original

If the original itself behaves wrongly, the port behaves the same way,
and the corpus cannot show it. For that there are the bug tests
([Bugs not fixed yet]({{< relref "/docs/compared-with-original/bugs-not-fixed-yet" >}})):
a program and a description of what it *should* do.

Such a test is a few files in a directory under `tests/bug-orig/`, named
after the bug, for example `libs/stringfunclib-mid-ueberlauf.bas` and
`stringfunclib-mid-ueberlauf.expect`. The program, with a comment that
says what is wrong and what is right:

```
; stringfunclib-mid-ueberlauf (#286) - Mid$(s$,start,len) compares
; start-1+len with the length of the string signed after the addition:
; with len = $7FFFFFFF the sum overflows, counts as "fits", and the result
; gets len bytes - far beyond the string and the string buffer (printed,
; the program writes 2 GB of memory; here only its length is shown).
; Correct: the length is cut to the rest of the string ("ello", 4).
; @configs clib
; @pass timeout=45
a$="Hello"
NPrint Len(Mid$(a$,2,$7FFFFFFF))
End
```

The lines that start with `; @` are instructions for the test run: here,
which build of the port is judged and how long the run may take.

The expectation, in sections, says what the compile and the run must
give, with the program's return code at the end. It describes the
correct behaviour, from the documentation of the command or from the
issue, never what the original happens to do:

```
[compile]
ok
[run]
4
rc=0
```

What the original does is recorded once, into a third file,
`stringfunclib-mid-ueberlauf.orig`, by two runs of the original compiler
and libraries, the second with the memory laid out differently:

```
cmake --preset amiga -DBUG_ORIG_RECORD_FILTER=stringfunclib-mid-ueberlauf
cmake --build --preset amiga --target bug-orig-record
```

It needs the CD. The file is written only if both runs saw the same;
otherwise the recording says "test not deterministic" and names the
lines that differ. Read the file before you commit it: its `[run]`
section shows what the original printed, here `2147483647` in place of
`4`. A line that changes from run to run for a reason that has nothing
to do with the bug, such as an address, is turned into a pattern, with
a comment line `# varies:` that says why.

With the three files, the test itself runs without the original:

```
ctest --preset bug-orig -R bug_orig_stringfunclib_mid_ueberlauf
```

It compiles and runs the program with the port and compares the result
with the expectation and with the recorded original. While the port shows
the bug, the test comes out REPRODUCED and fails; once the bug is fixed
it comes out FIXED. If the recorded original already matches the
expectation, the result is BROKEN: the program does not show a bug, or
the expectation describes the wrong behaviour.

The change that fixes the bug adds one line to the program's header,
with the number of the bug's issue:

```
; @fixed #286
```

From then on the test is a *fixed test*. It runs with the normal
emulator tests and checks only the port, with its own libraries, against
the expectation: FIXED, or REGRESSED if the fix has been undone. The
recorded original stays: it is the original's side of the pair, what the
original does next to what the port now does. The `originals` run
compares the original with it again (the test
`bug_orig_record_stringfunclib_mid_ueberlauf`, which only compares and
writes nothing), so a changed original or a recording that was not
reliable shows up there.

Further instructions starting with `; @pass` exist, for example one that
fills every block of memory the compiler allocates with a pattern, so
that a random value is never 0 by chance. The full description of the
format is in the
[description of the tests](https://github.com/fubar-coder/bb2-c/blob/main/tests/README.md)
in the repository.

## Comparing the libraries

The libraries are compared with the originals in two ways. On the host,
without an emulator, the declarations of every library (its commands,
their arguments and help texts) are compared with data taken from the
original library file. In the emulator, a test program per library loads
either the port's libraries or the original ones, calls their routines
and writes what it observes, results, registers and memory, as cases.
The normal run checks the port's cases against expected data recorded
from the original libraries; the `originals` run checks the original
libraries against the same data. Where a fixed bug changes a result on
purpose, the expected data holds both values, marked as the original's
and the port's
([Reading library test results]({{< relref "/docs/project-structure/library-test-cases" >}})).
