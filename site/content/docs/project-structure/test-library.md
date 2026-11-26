---
title: The test library
weight: 5
description: Every macro a test that judges itself can use - the blocks, the checks of long, quick, float and string values, preconditions, expected runtime errors and started programs - with what each records, how cases, procedures, the batch init and the batch cleanup are written, the placeholders, and the known limits.
---

A [test that judges itself]({{< relref "self-judging-tests" >}}) checks
its results with the *test library*: a small Blitz Basic user library and
a set of macros for it. This page describes every macro a test source can
use, what it records, and the rules for writing the source around it.

The source does not load the library or include the macros. The test
runner puts the macros in front of the source and adds a short main
program, the *driver*, that calls the blocks of the source. That is why
a source consists only of blocks and declarations.

## The blocks

A source has three kinds of blocks, in this order:

| Block | Macros | How many |
|---|---|---|
| batch init | `!TestBatchInit` ... `!EndTestBatchInit` | at most one, before the first case |
| case | `!TestCase{NAME}` ... `!EndTestCase` | one or more |
| batch cleanup | `!TestBatchCleanup` ... `!EndTestBatchCleanup` | at most one, after the last case |

Each macro stands on a line of its own; only a comment may follow it.
Each CTest test runs **one case**: the driver calls the batch init (if
the source has one), then the case the test is for, then the batch
cleanup (if the source has one). When you run all cases of a source at
once with the test runner, it calls several cases in one program to save
time, but still judges each case from its own part of the result file
([Running all cases of a source]({{< relref "running-tests#running-all-cases-of-a-source" >}})).

```basic
!TestBatchInit
  Dim buf.b(15)
  !Require{buf(0)=0,"buffer"}
!EndTestBatchInit

!TestCase{longs}
  a.l=7
  !Check{a>5,"greater"}
  !CheckEq{a*3,21,"product"}
!EndTestCase

!TestBatchCleanup
  !Check{1,"done"}
!EndTestBatchCleanup
```

A block is a subroutine of one program, called with `Gosub`. So the
cases share the program's variables, and since a case must not depend on
what another case left behind, each case sets up what it uses itself. The
batch init only prepares data that the cases read. Inside a block,
`SetErr`, `SetInt`, labels and a local `Dim List` work as in any
subroutine. A loop, a `Select`, a block `If` or a `CNIF` ... `CEND` must
not reach over the start or the end of a block; a whole block may stand
inside a `CNIF` ... `CEND`.

**A case name** is written in lower case: it starts with a letter and
holds letters, digits and `_`, but no `__`, and it does not end in
`_originals`. It must not be the name of a command or function of any
library set, whatever its case: `on`, `add`, `free` or `count` are
refused. Each name appears once in a source.

The same holds for the names of procedures and variables: a name that
is also a command or function of a library, such as `colour`, `bset`,
`eof`, `volume` or `instr`, does not work as a variable or procedure
name. The build refuses such a case name at once, but a procedure or
variable with such a name only fails when the driver is compiled, with a
bare `Syntax Error` at its line.

### Outside the blocks

Outside the blocks only declarations are allowed:

* constants (`#WIDTH=16`), `NEWTYPE` and `DEFTYPE`;
* the definitions of `Macro`, `Statement` and `Function`;
* the compiler directives `CNIF`, `CSIF`, `CELSE`, `CEND`, `CERR`,
  `OPTIMIZE`, `Syntax`, `WBStartup`, `NoCli` and `CloseEd`;
* labels and data: `Data`, `Dc`, `Dcb`, `Even`, `IncBin`.

Any other statement there, also `INCLUDE`, `XINCLUDE` or a call of one of
the source's own macros, is refused by the build with the file and line.
A helper is a procedure (a `Statement` or a `Function`) or a subroutine
inside a case, never a `Gosub` target outside the blocks. A macro
definition of the source must not contain a check, but a check may be an
argument of a macro call inside a block.

A label outside the blocks is written as a Blitz Basic label, `.name`;
the form `name:` counts as code and is refused. `Data` belongs outside
the blocks, behind such a label that the case reads with `Restore`:
inside a case it would run as code. Assembler instructions are code as
well, so an assembler routine stands inside the case, behind a `Goto`
that jumps over it, and its labels are `.name` labels too. A label
behind data of odd length (`IncBin`, `Dc.b`) needs an `Even` before it
when a `Goto` jumps to it.

```basic
.table
Data.l 3,5,7

!TestCase{sum}
  Restore table
  s.l=0
  For i.l=1 To 3
    Read v.l
    s=s+v
  Next
  !CheckEq{s,15,"sum"}
!EndTestCase
```

### Types, line length and procedures

A variable without a type in a case is a quick value (`.q`), not a long
one. A test that keeps pointers or addresses in variables puts
`DEFTYPE .l` outside the blocks; an address in a quick variable leads to
errors such as `Unable to Free Memory`.

A line may be at most 128 characters long after the macros are
expanded, the limit of the editor's format; split a long line that
builds a string.

A `Statement` takes at most 6 parameters; the 7th gives `Too many
parameters`. An array that a `Function` uses cannot be declared in a
case, because its `Dim` must come before the function: use a memory
block from `AllocMem_` or an array declared outside the blocks. The
original compilers mishandle `PutReg` into a local variable of a
`Function` (it leaves the local 0 and overwrites other memory; the port
has this bug fixed). A case that also runs with an original compiler
uses `PutReg` only into `SHARED` or global variables, in a `Statement`
or directly in the case.

A trap or error handler that lets the case continue must restore the
stack: the trap leaves the library's frames on the stack, and the
block's final `Return` would return into the program behind the faulty
call, so the case would run again. Save the stack pointer before the
statement that traps (`LEA sv(pc),a0 : MOVE.l a7,(a0)`) and restore it
in the handler (`MOVE.l (a0),a7`) before the checks. A handler that only
ends the program needs neither. A check cannot follow a `SetErr` jump to
the end of the case, so such a handler holds the checks and the `End`
itself.

## The checks

Each check compares a value the program computes with the value written
in the source. The last argument, a string in quotes, is the check's
*name*: it may be left out, but a check that the configuration names in
an assertion must have one, and a name appears once in its case or
procedure.

| Macro | Passes when |
|---|---|
| `!Check{COND,"name"}` | the condition, a long value, is not 0 |
| `!CheckEq{ACTUAL,EXPECTED,"name"}` | the two long values are equal |
| `!CheckQEq{ACTUAL,EXPECTED,"name"}` | the two quick values are exactly equal |
| `!CheckFltNear{ACTUAL,EXPECTED,DELTA,"name"}` | the two float values differ by at most `DELTA` |
| `!CheckStrEq{ACTUAL$,EXPECTED$,"name"}` | the two strings are equal |
| `!Require{COND,"name"}` | a precondition: the condition is not 0 |
| `!ExpectError{"MESSAGE"}` | announces the runtime error the next statement raises |

A failed check does not end the case: the checks after it run as well,
and the report lists all that failed. For each check that runs, the
library writes a line to the result file with the verdict, the expected
and the actual value, so the report can show what the program really
computed:

```
check limits word (probe.test.bas:58) failed: s$: expected "Help", actual "Hello"
```

The report names the case, the check's name, the line in the source, the
expression and the two values.

### Long values: `!Check` and `!CheckEq`

`!CheckEq` compares two long values. `!Check` takes a condition; it
records -1 as the expected and the condition's truth (-1 or 0) as the
actual value, so the report of a failed one reads `a>5 is 0`.

```basic
!TestCase{sizes}
  w.l=16
  !CheckEq{w*2,32,"double"}
  !Check{w>0 AND w<=320,"in_range"}
!EndTestCase
```

A failed `!CheckEq` shows both values in decimal and in hexadecimal:
`expected -1 ($FFFFFFFF), actual -4 ($FFFFFFFC)`.

### Quick and float values: `!CheckQEq` and `!CheckFltNear`

`!CheckQEq` compares two quick values (the fixed-point type `.q`)
exactly, `!CheckFltNear` two float values (`.f`) within a tolerance:

```basic
!TestCase{fractions}
  q.q=1.25
  !CheckQEq{q*2,2.5,"double"}
  f.f=2/3
  !CheckFltNear{f,0.6667,0.001,"third"}
!EndTestCase
```

Their arguments are evaluated as quick or float values, so nothing is cut
to a whole number on the way. The report shows the values as numbers and
as their 32 bits, for example `expected 1.5 ($00018000), actual 1.25
($00014000)`. The condition of `!Check` and `!Require`, on the other hand,
is evaluated as a long value. Check a quick or float result with these
two macros, not with a comparison inside `!Check`: the original compilers
cut a quick operand of such a comparison to a whole number, so a test
that runs with them would check the wrong value (and see
[the known limits](#known-limits) for the port).

### Strings: `!CheckStrEq`

`!CheckStrEq` compares two strings, byte for byte:

```basic
!TestCase{strings}
  s$=Left$("Hello",4)
  !CheckStrEq{s$,"Hell","left"}
  a$="a"+Chr$(9)+"b"
  !CheckStrEq{a$,"a"+Chr$(9)+"b","tab"}
!EndTestCase
```

The library records each of the two strings with its length and a
checksum (a CRC-32) of the whole string. Bytes that are not printable are
written as `\xHH`, so the tab above reads `a\x09b`. A line of the result
file holds at most 512 bytes: when the two strings do not fit, the
library shortens the recorded text, but the verdict, the lengths and the
checksums still belong to the whole strings. An assertion that compares
such a value with `==` or `!=` is therefore exact for long strings too;
`contains` and `matches` need the whole text and cannot judge a shortened
one. The report shows a shortened value as its beginning, followed by
`(cut, length N)`.

### Preconditions: `!Require`

`!Require` checks what the test needs before it can test anything, such
as a successful allocation. When it fails, the rest of the block does not
run, and the case is *broken*, not failed: the tested behaviour was never
reached. A failed precondition in the batch init breaks every case.

```basic
!TestCase{setup}
  m.l=AllocMem_(1024,1)
  !Require{m<>0,"memory"}
  Poke.l m,42
  !CheckEq{Peek.l(m),42,"stored"}
  FreeMem_ m,1024
!EndTestCase
```

The report names the precondition: `precondition setup memory
(probe.test.bas:76) failed: m<>0`, and the verdict is `BROKEN`.

### Expected runtime errors: `!ExpectError`

When the correct behaviour is a runtime error, the case announces it
right before the statement that raises it. The configuration must switch
on the runtime error checking, `runtime-errors = "on"`:

```basic
!TestCase{out_of_range}
  Dim a(5)
  !ExpectError{"Array Subscript out of Range"}
  a(9)=1
!EndTestCase
```

The runner looks for a runtime error requester after the announcement
whose text contains the message. If it comes, the case passes and ends
there: the program's exit code is not checked, and that requester does
not count as an unexpected one. If no such requester comes, the case
fails with `the announced runtime error "..." did not come`.

Like a check, the announcement may have a name as its last argument,
`!ExpectError{"MESSAGE","NAME"}`. The configuration can then name it as
the check `check.CASE.NAME`: `ok` if its requester came, `fail` if not.
A test of an original bug uses this to declare that the original shows
another error in its place, while the announcement stays for the port.
Since the announcement is judged by its requester, it cannot stand in a
test with a cell that runs the program from the editor (the key
`editor-run`), where requesters are not captured.

### Checks in loops and procedures

A check in a loop runs once per pass and writes a line each time. It
passes if every pass passed; the report and the assertions take the
values of its first failed pass.

```basic
!TestCase{squares}
  For i.l=1 To 3
    !CheckEq{i*i,i+i*(i-1),"square"}
  Next
!EndTestCase
```

A check in a procedure belongs to the procedure: in an assertion it is
named `check.PROCEDURE.NAME`, and its line in the result file belongs to
whichever case called the procedure. Such a procedure must not have the
name of a case, nor `init` or `cleanup`.

```basic
Statement check_width{w.l}
  !CheckEq{w,16,"width"}
End Statement

!TestCase{longs}
  check_width{16}
!EndTestCase
```

## Starting a program: `!RunProgram`

A case can start another program and check what it did.
`!RunProgram{COMMAND$,"name"}` runs a command line as the shell takes
it, with its arguments and redirections, and waits for its end. The
input of the program is `NIL:` unless the line redirects it; what it
prints is kept in a file of the run. The macro stands in an expression
and gives the address of the result, a `TLRun` structure that the case
reads through a pointer:

```basic
!TestCase{child}
  *r.TLRun=!RunProgram{"@TESTDIR@/child","child"}
  !CheckEq{*r\rc,20,"rc"}
  !CheckStrEq{*r\printed,"done"+Chr$(10),"printed"}
!EndTestCase
```

The fields of `TLRun`:

| Field | What it holds |
|---|---|
| `rc.l` | the return code, 0 if the system gave none |
| `rcknown.w` | -1 if the system gave the return code; 0 on Kickstart 1.x, which gives none |
| `cut.w` | -1 if the output was longer than 1024 bytes and `printed` holds only its start |
| `size.l` | the number of bytes the program printed |
| `printed.s` | the first 1024 bytes of what it printed, as they are |

Read the fields only through the pointer (`*r\rc`; `r\rc` without the
`*` is another variable and does not compile) and never assign to one:
the structure belongs to the library. There is only one: a later
`!RunProgram` overwrites it, so a case that needs the results of two
programs copies what it needs of the first (`a$=*r\printed`,
`c.l=*r\rc`) before it starts the second. When the return code is at
or above the shell's fail limit, the shell's own line such as `child
failed returncode 20` is part of the printed output.

There is no time limit for the program: a program that hangs is ended by
the hang check of the session, like the test itself. `!RunProgram`
cannot start a program outside a block or in Blitz mode (also not while
multitasking or interrupts are off); it then records the reason, and the
case is broken. A case that starts a program runs on its own when the
runner runs all cases of a source together, so the requesters, a crash
or a hang of the program count for that case alone.

What the case cannot check itself, whether the program returned at all
and everything it printed beyond the first 1024 bytes, the configuration
checks with the queries `run.CASE.NAME`
([Assertions]({{< relref "self-judging-tests#assertions-what-else-a-run-must-show" >}})).
That is why a started program that an assertion names needs a name.

The program is usually one the test brings along in its own data
directory: a Blitz Basic source there is compiled before the test runs
and is found under `@TESTDIR@` (see
[Data of a test]({{< relref "self-judging-tests#data-of-a-test" >}})).

## Placeholders in the source

The runner replaces these words in the code and in strings, not in
comments:

| Placeholder | Becomes |
|---|---|
| `@LINE@` | the number of the line it stands in |
| `@CASE@` | the name of the case it stands in |
| `@TEST@` | the test's name, `ID` |
| `@TESTDATA@` | `CORPUS:data`, where `files = ["data/NAME\|@TESTDATA@/NAME"]` puts the test data |
| `@TESTDIR@` | `CORPUS:ID.test`, where the test's own data arrive ([Data of a test]({{< relref "self-judging-tests#data-of-a-test" >}})) |

Any other word of the form `@NAME@` in the source is refused. A Blitz
Basic source among the test's own data gets the same placeholders.

```basic
!TestCase{places}
  !CheckStrEq{"@CASE@ @TEST@","places probe","names"}
!EndTestCase
```

## What the library leaves alone

The library is written so that it does not disturb what it tests:

* It writes only to its result file, never to the program's window or
  standard output, so a test can also check what the program prints.
* Between the start of the program and its end it allocates and frees no
  memory, so a case can compare the free memory before and after a call.
  The one exception is `!RunProgram`: starting a program takes memory of
  the system and of the started program.
* Its routines keep the registers `d2` to `d7` and `a2` to `a6`. They do
  not keep what the compiler leaves in other registers or in its string
  buffer: if the tested statement depends on such a value, set it in the
  statement right before, with no check between the two.
* It takes the name of its result file from the driver, not from the
  command line, so `NumPars` and `Par$` give the program's own arguments.
* In Amiga mode every line is written at once, so the lines before a
  crash are kept. In Blitz mode, or with multitasking or interrupts off,
  up to 1024 bytes of lines are kept and written at the next check in
  Amiga mode or at the end of the program; what does not fit is lost,
  and the result file says so.

## Known limits

An open bug of the port's libraries changes what a check sees. Write a
test so that it does not depend on it: **`MOD` with a negative dividend
and a negative divisor** gives a remainder outside the divisor's range:
`-7 MOD -3` is -4 instead of -1. With a positive divisor the remainder is
between 0 and the divisor minus 1: `-7 MOD 3` is 2
([open issue](https://github.com/fubar-coder/bb2-c/issues/1197)).

The repository's
[description of the test sources](https://github.com/fubar-coder/bb2-c/blob/main/tests/README.md#blitz-test-sources-and-the-result-file)
has the exact form of the result file and of the driver.
