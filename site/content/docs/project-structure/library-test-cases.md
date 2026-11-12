---
title: Reading library test results
weight: 3
description: How to read a line of the library tests' expected data or a failure message - the four parts of a case key, what the input part says in the test programs, and the forms a value takes.
---

Before this page you should know how the library tests work
([Tests]({{< relref "/docs/project-structure/tests" >}})).

The library tests write everything they observe as *cases*, one line
each, and compare them with the expected data in the repository. This
page explains how to read such a line, in an expected file or in the
message of a failing test, so that you can tell which call, which input
and which result it is about.

## A line, taken apart

Here are four lines of the expected data of the test of the RWE library
riarrayslib, which has the one command `FillArray`:

```
FillArray|inline length|form 0 inline|1	$0000002E
FillArray|kept|form 0 type $0001 size 1 off 5 n 1 value $DE551B45|1	kept
FillArray|array|form 0 type $0001 size 1 off 5 n 1 value $DE551B45|1	bytes 200 fnv 7FA26772
FillArray zero|popped|form 1 type $0001 size 1 off 6 n 1 value $95AEDA77|1	$00000000
```

Each line is a *key* and a *value*, separated by a tab. The key names
the case and is unique in the file; the value is what the library did.
The key has four parts, separated by `|`:

| Part | In the first line | What it is |
|---|---|---|
| routine | `FillArray` | the library routine the case is about |
| check | `inline length` | what was looked at |
| input | `form 0 inline` | the input of the case, as the test program describes it |
| counter | `1` | 1, or a higher number when the same check with the same input happens again |

So the first line reads: *of the first form of `FillArray`, the length
of its inline code is $2E bytes*. The second: *after `FillArray` was
called on a byte array of one element at offset 5 with the value
$DE551B45, the registers the routine must keep were kept*. The third:
*after that call, the 200 bytes of the test buffer have this hash*. The
fourth: *the second form, `FillArray a()`, removed nothing from the
stack itself*.

A key never holds a result or a memory address, only the input. That is
what makes the same key mean the same case in every run: the original
library and the port's library give the same keys, and only the values
may differ.

## The routine

The routine part is the name the test program gives the routine it
calls. It is usually the command's name as Blitz Basic knows it
(`FillArray`, `DisAssemble$`, `Peek`), and for the routines a library
runs when a program starts and ends, `init` and `finit`. A program may
add a word to tell variants apart: riarrayslib names the routine of the
second form of `FillArray` `FillArray zero`, and other programs write `init (fails)` or
`InstrSize (before DisAssemble$)`.

The test of the libraries the compiler calls by itself (ldivlib and
lmullib, which divide and multiply) names a routine by its *token*, the
number the compiler uses for it: `$CB00` is the first routine of ldivlib.

The part is empty when the case is not about one call, for example the
number of routines the program found (`|routine count||1`), or a value a
program writes after the call under its own name
(`|arrmult d0|$4 $9|1`).

## The check

The check part says what was observed. Three checks are written for
nearly every call:

* **`kept`**: whether the routine left the registers unchanged that it
  must keep. Every routine has a stated set of such registers, taken from
  what the original keeps.
* **`popped`**: how many bytes the routine removed from the stack itself.
* a register after the call, such as **`d0`** for a function's result.

The others name what the program looked at: `array` or `string` for a
buffer, `a3 moved` for how far a pointer advanced, `inline` and
`inline length` for the code of a command that the compiler copies into
the program, or a description such as `graphics.library open count`.

## The input

The input part describes the input of the case. Its wording is chosen
by each test program, so **the test program's source is the authority**
on what it means: the place where the program writes it, and the comment
above it, say what each word stands for. Most programs describe the
arguments they pass. Some words recur.

### form N: which form of a command

A Blitz Basic command can accept several argument lists, called its
*forms*. The compiler picks the form by the number of arguments in the
call, and each form is a routine of its own in the library. `FillArray`
has two:

```blitzbasic
Dim a.w(99)
FillArray a(),-1    ; form 0: set every element to -1
FillArray a()       ; form 1: set every element to 0
```

`form 0` in a key is the first form the library declares, `form 1` the
second, and so on, in the order the library's page in the command
reference lists them (for `FillArray`:
[riarrayslib]({{< relref "/docs/libraries/userlibs/riarrayslib" >}})).
A test calls each form directly, as the compiler's code would.

### inline: the routine's code itself

Some commands are not called but copied into the program: their routine
starts with the word `$A000` and the length of the code that follows.
`form 0 inline` is the case of the first form's code, before any call.
The check `inline` is 1 if the routine is such inline code, and
`inline length` is its length in bytes.

### Arguments and registers

Most input parts list the arguments of the call. Five examples from
different programs:

| Program | Input part | Meaning |
|---|---|---|
| [riarrayslib](https://github.com/fubar-coder/bb2-c/blob/main/libs/tests/rwe/riarrayslib.c) | `form 0 type $0001 size 1 off 5 n 1 value $DE551B45` | form 0, an array of type 1 (byte; 2 is word, 3 long) with elements of 1 byte, starting at offset 5 of the test buffer, 1 element, filled with the value $DE551B45 |
| [ridisasmlib](https://github.com/fubar-coder/bb2-c/blob/main/libs/tests/rwe/ridisasmlib.c) | `0000 8A26 7FF3 9ABC ` | the 16-bit words of the code that `DisAssemble$` disassembles, in hex (with a space after each): the instruction word and the extension words behind it |
| [systest_med](https://github.com/fubar-coder/bb2-c/blob/main/libs/tests/systest_med.c) | `MMD0 tick 9` | the step of the scenario (here: playing a module of the format MMD0; other steps are `init`, `JumpMed`, `SetMedVolume`, `StopMed` ...) and the number of player ticks since the song started |
| [systest_cmdlibs](https://github.com/fubar-coder/bb2-c/blob/main/libs/tests/systest_cmdlibs.c) | `type $1 at 100` | `Peek` with the type code 1 at offset 100 of the test memory |
| [rigtmenulib](https://github.com/fubar-coder/bb2-c/blob/main/libs/tests/rwe/rigtmenulib.c) | `0 form 0 1/1/-1 "" "" mx 0 ud 0` | menu list 0, form 0 of `GTMenuItem`, menu 1, item 1, no sub item (-1), an empty text and shortcut, mutual exclude 0, user data 0 |

Other programs register inputs directly (`d0 $5A000001 a4 $11111111`),
name a situation (`before init`, `no port`, `odd address`) or leave the
part empty when the routine has no input, as `init` and `finit` usually
do.

## The counter

The last part is 1 the first time a routine, check and input occur
together, 2 the second time, and so on. It makes a check that a program
repeats with the same input unique, for example the `kept` case of a
second `init`:

```
init|kept||1	kept
init|kept||2	kept
```

## The value

The value is the result, written in one of these forms:

| Value | Means |
|---|---|
| `$0000002E` | a number, always eight hex digits |
| `bytes 32 9999...99` | a buffer of up to 64 bytes: its length and its bytes in hex |
| `bytes 200 fnv 7FA26772` | a longer buffer: its length and a hash of its bytes |
| `"level1"` | a text; bytes that are not printable are written `\xHH` |
| `null` | a text pointer that is 0 |
| `prog` | an address inside the test program itself |
| `NAME+$OFF` | an address relative to a base the program names (the format has it; no test program uses it) |
| `kept` | the routine kept every register it must keep |
| `lost d3 a1` | it changed these registers, which it must keep: a failure |
| `not captured: REASON` | the value cannot be reproduced; the reason is text for the reader |

An address is never written as a plain number, because it changes with
the memory layout from one run to the next: it is `prog`, an offset from
a buffer, or relative to a base. `lost` never appears in expected data,
because a recording refuses it; you meet it in a failing run.

A case `not captured` is skipped on both sides. The test of ciatrklib
has such cases, for example

```
PlayTracker|DMACONR|F00 on voice 1, tick 0|1	not captured: F00 phase: the original writes DMACON into the hardware, the C build into its RAM copy
```

because there the original writes into the sound hardware and the
port's library into a copy in memory, so their values cannot agree.

## Fixed bugs: two cases for one key

Where a fixed bug of the original changes a result, the key appears
twice, with a qualifier after the counter:

```
FillArray|inline length|form 0 inline|1 [orig]	$0000002E
FillArray|inline length|form 0 inline|1 [fixed #123]	$00000030
```

`[orig]` is what the original does, and only the original's libraries
are checked against it. `[fixed #123]` is what the port does after the
fix of issue 123, and only the port's libraries are checked against it.
(These two lines are made up; the expected files hold the real pairs.) A
case without a qualifier holds for both.

## Blocks

Three programs make far too many observations to write them one by one:
the tests of the MED player, of the disassembler and of the division and
multiplication routines make tens of thousands to millions each. They write every 4096
observations as one *block* case:

```
BLOCK:2	4096 BBDD94DD first=$CB00|d0|$FFFFF8C3 $D89B8344|4097
```

The key is the block's number. The value is the number of observations
in the block, a hash of them, and the key of the first one. Inside a
block the counter is the number of the observation in the program, so
the first observation of block 2 has the counter 4097. The key here says:
the routine `$CB00` (ldivlib's first) was called with d0 $FFFFF8C3 and
d1 $D89B8344, and its `d0` was observed.

A block cannot be read, only compared. When one differs, the test names
its number and the range of observations it holds, and a debugging run
can write the block's observations out (the repository's
[description of the tests](https://github.com/fubar-coder/bb2-c/blob/main/tests/README.md),
section "Libraries").

## A failure message

A failing test names the program, the key and both values:

```
rwe_riarrayslib: FillArray|inline length|form 0 inline|1: expected $0000002E, got $0000002F
```

That reads: in the test of riarrayslib, the first form of `FillArray`
now has inline code of $2F bytes instead of $2E. A case the run did not
write is `missing (expected VALUE)`, a case the expected data does not
have is `added (got VALUE)`, and a block names its observations:

```
rwe_ridisasmlib: BLOCK:294 (observations 1200129..1204224): expected ..., got ...
```

The test shows the first differences, up to eleven, and then a count of
the rest. At the end of each program it prints a summary line with the
number of cases, equal, differing, missing and added ones. The whole
output of the run stays in the build directory, one file per program,
in the same format as the expected file, so a `diff` of the two shows
every difference.

## The sources

The test programs are in
[libs/tests](https://github.com/fubar-coder/bb2-c/tree/main/libs/tests)
on GitHub, the expected data in
[libs/tests/expected](https://github.com/fubar-coder/bb2-c/tree/main/libs/tests/expected).
The format is defined by the part of the tests that writes the cases,
[capture.h](https://github.com/fubar-coder/bb2-c/blob/main/libs/tests/capture.h),
and the tool that compares them,
[libscmp.h](https://github.com/fubar-coder/bb2-c/blob/main/tools/lib/libscmp/libscmp.h).
