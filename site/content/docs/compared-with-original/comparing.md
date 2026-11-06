---
title: Comparing with the original yourself
weight: 5
description: Compile your own program with the original compiler and with the port, compare the results, and write a test for a bug you found.
next: /docs/ab3-mode
---

Before this page you should have built the port and run its tests
(see the chapter [Project structure]({{< relref "/docs/project-structure" >}})).

The claims of this chapter are checked by tests that run the original
Blitz Basic 2 and the port side by side in an emulator. You can use the
same machinery for your own programs: to see whether a program behaves
the same with both, or to show a bug you suspect in the original.

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

  Without them, every test that compares with the original is skipped.

## Your program in the corpus

The *corpus* is a set of small Blitz Basic programs, kept as plain text.
The test runs compile every one of them three times in the emulator and
run the results:

| Run | Compiler | Libraries | Must be |
|---|---|---|---|
| ref | the original blitz2 2.10 | the original libraries | the reference |
| port | the port's blitz2 | the original libraries | the same executable and the same output as ref |
| clib | the port's blitz2 | the port's libraries | the same output as ref |

So the corpus checks the compiler (the executable must be identical) and
the libraries (the program must do the same) separately.

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

### Run the comparison

```
ctest --preset emulator -R '^corpus$'
```

(`corpus_runerr` for the directory with runtime error checking.)

### Read the report

The report is written to `build/amiga/tests/corpus/corpus.txt`, one line
per program. A program that behaves the same with both looks like this:

```
t03_str          port: exe identical, output same; clib: output same
```

{{% /steps %}}

If the port's run differs from the reference, you have found a difference
that is not on the list of fixed bugs: please open an issue with the
program and the report.

## A test for a bug of the original

If the original itself behaves wrongly, the result is the same in all
three runs, and the corpus cannot show it. For that there are the bug
tests ([Bugs the port keeps]({{< relref "/docs/compared-with-original/kept-bugs" >}})):
a program and a description of what it *should* do.

Such a test is two files in a directory under `tests/bug-orig/`, named
after the bug, for example `compiler/compiler-const-no-value.bas` and
`compiler-const-no-value.expect`. The program, with a comment that says what is wrong and what is right:

```
; compiler-const-no-value (#972) - a constant defined without a value
; holds what the memory held. Correct: its value is 0.
; @pass wrap=@INJECT@|fill=a5
#c
#d=5
#d
NPrint #c
CNIF #c
NPrint "cnif true"
CELSE
NPrint "cnif false"
CEND
NPrint #d
End
```

The line `; @pass ...` is an instruction for the test run: here it fills
every block of memory the compiler allocates with a pattern, so that the
random value of the original is never 0 by chance. The expectation, in
sections, says what the compile and the run must give, with the
program's return code at the end:

```
[compile]
ok
[run]
0
cnif false
5
rc=0
```

Run it on its own with

```
ctest --preset bug-orig -R bug_orig_compiler_const_no_value
```

The test compiles and runs the program with the original and with the
port. If the original does not match the expectation, the test shows the
bug: it comes out REPRODUCED while the port keeps the bug, and FIXED once
the port is fixed. If the original already matches, the result is BROKEN:
the program does not show a bug, or the expectation describes the wrong
behaviour. The full description of the format, with the extra lines a
program can use in its header, is in the
[description of the tests](https://github.com/fubar-coder/bb2-c/blob/main/tests/README.md)
in the repository.

## Comparing the libraries

The libraries are compared with the originals in two ways. On the host,
without an emulator, the declarations of every library (its commands,
their arguments and help texts) are compared with the original library
file. In the emulator, a test program loads each library of the port and
the original, calls their routines with the same arguments and compares
results, registers and memory. Both run with the normal test presets.
Where a fixed bug changes a declaration on purpose, the difference is
listed in the repository and the comparison expects it.
