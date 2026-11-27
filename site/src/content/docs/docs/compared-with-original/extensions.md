---
title: Extensions
description: What the compiler of the BB2 C port accepts beyond Blitz Basic 2,
  the rule that keeps old sources unchanged, the four functions it compiles
  faster, the animations LoadShape loads, and the forms the original refused
  that fixes of bugs compile.
sidebar:
  order: 8
prev:
  link: /docs/compared-with-original/libraries/
  label: Libraries
aliases:
  - /docs/compared-with-original/new-errors/refused-forms/
---

Before this page you should know how the compiler options work
([The editor and the compiler](/docs/how-blitz2-works/editor-and-compiler/)).

The compiler of the BB2 C port follows one rule for everything it adds:
an extension may only give a meaning to a source that the original
refuses. A source that compiles with Blitz Basic 2 compiles to a program
that does the same with the port, extensions or not. The port's
executable differs on purpose in two places, both for speed: a few
functions are compiled as inline code
([Faster Int, Frac, Len and Asc](#faster-int-frac-len-and-asc)),
and a branch with the size `.b` in inline assembler is a short branch
([Short branches in inline assembler](#short-branches-in-inline-assembler)).

The libraries add to Blitz Basic 2 in the same way: `LoadShape` loads
animations that the original stopped with a runtime error
([Animations of every interleave in LoadShape](#animations-of-every-interleave-in-loadshape)).
A few fixes of bugs, finally, compile sources that the original refused
([Forms the original refused](#forms-the-original-refused)).

## #__debug: is runtime error checking on?

`#__debug` is a constant that the compiler defines itself. Its value is
−1 when the program is compiled with runtime error checking (the option
**Runtime Error Debugger**) and 0 without it. The original compiler does
not know it and reports "Constant Not Found".

Its use is code that should exist only in a program compiled for
debugging. Put it between `CNIF` and `CEND`:

```
CNIF #__debug
  NPrint "only with runtime error checking"
CEND
```

Compiled with runtime error checking, the program prints the line;
compiled without it, the line is not in the program at all.

Some details:

* `#__debug` can be used wherever a constant can: in expressions, in
  `CNIF`, in the definition of other constants and as an operand of the
  inline assembler.
* Its value comes from the option at the start of the compile. A
  `RunErrsOff` in the source does not change it.
* A source that defines `#__debug` itself, with `#__debug=...`, uses its
  own value instead.
* `Stop` keeps its behaviour: without runtime error checking the compiler
  reports "Debugger must be enabled for STOP". A program that is compiled
  both ways can put its `Stop` between `CNIF #__debug` and `CEND`.

The name and the values are those of AmiBlitz3, so a source that uses
`#__debug` means the same with both. blitz2-ab3 knows it in both of its
modes ([AB3 mode](/docs/ab3-mode/)).

## Faster Int, Frac, Len and Asc

Four functions of the libraries are compiled as a few instructions in
the program, instead of a call of the library, when their argument
allows it. The result is the library's, to the last bit, with and
without runtime error checking; only the executable is different, and
the program is faster:

| Function | Compiled inline when | Cycles per call on an A500, call → inline |
|---|---|---|
| `Int` | the argument is a Quick | 189 → 63 |
| `Frac` | the argument is a Quick and the call is all of the right side of an assignment to a Quick, as in `q2=Frac(q1)` | 1316 → 62 |
| `Len` | the argument is a string variable, as in `Len(a$)` | 679 → 63 |
| `Asc` | the argument is a string variable, as in `Asc(a$)` | 745 → 99 |

`Len` and `Asc` no longer copy the string first, so their time does not
depend on its length any more: `Len` of a string of 1000 characters took
8261 cycles and now takes 63.

Everything else is compiled with the call, as before:

* `Frac` inside a larger expression, a condition or `Print`, or with a
  Float or Long target;
* `Len` and `Asc` of an expression (`Len(a$+b$)`), of a function
  (`Len(Left$(a$,2))`), of a literal, of an array element or of a field
  of a NewType.

In AB3 mode `Frac`, `Len` and `Asc` stay library calls too,
because AmiBlitz3's libraries are different; `Int` is inline there as
well.

## Short branches in inline assembler

The inline assembler of Blitz Basic 2 makes every branch (`BRA`, `BSR`,
`BEQ`, `BNE` and the other `Bcc`) a word branch of 4 bytes. It does not
know the size `.s`, which most other assemblers use for a short branch:
`BEQ.s` is a "Syntax Error". The port's compiler takes the sizes on a
branch as other assemblers do:

| Size | `BRA`, `BSR`, `Bcc` | Other instructions |
|---|---|---|
| none | word branch, as before | as before |
| `.s` or `.b` | short branch: 2 bytes | `.s` is the same as `.b` |
| `.w` | word branch | as before |
| `.l` | `Bcc`: word branch, as before; `BRA`, `BSR`: "Illegal Assembler Instruction Size" | as before |

A short branch reaches from 128 bytes back to 126 bytes ahead of the
instruction after it, and its target may come before or after it. A
target further away is the error "Illegal Displacement" at the line of
the branch, and so is the next instruction as target: the 68000 cannot
encode a short branch there.

```
MOVEQ #1,d0
l1:
BNE.s l2
MOVEQ #0,d0
l2:
PutReg d0,a.l
NPrint a," ",?l2-?l1
End
```

prints `1 4`: the branch is taken, and the branch and the `MOVEQ` take 4
bytes, where the word branch took 6.

The original accepted `.b` on a `Bcc` and made a word branch of it. With
the port, such a source compiles to a short branch, which does the same
and is 2 bytes shorter; if its target is too far away for a short branch,
the port reports "Illegal Displacement". Use `.w`, or no size, for a
word branch. Of the sources on the Blitz Basic 2 CD, one library source
uses `Bcc.b`; all its branches are in range.

In [AB3 mode](/docs/ab3-mode/) the branches stay word
branches whatever their size, as in AmiBlitz3.

## Animations of every interleave in LoadShape

An IFF ANIM file stores each frame as the differences from an earlier
frame, and the header of each frame says which one: its *interleave*. An
interleave of 1 means the frame before it, a number n means n frames
back, and 0, the default of the ANIM specification, means two frames
back. Most ANIM files use 0, because players show them with two
alternating screens (double buffering).

The original `LoadShape` takes only the interleave 1. Any other value
stops the loading after the first frame with the runtime error "Illegal
ANIM interleave value". The port's `LoadShape` takes every value as the
specification defines it, frame by frame, so such an animation loads
completely, each frame into its own shape. A frame with fewer frames
before it than its interleave asks for, such as the second frame of an
animation with the interleave 0, is made from the first frame, as a
player starts both screens with it. An animation with the interleave 1
loads as before.

The command reference describes `LoadShape` with its limits
([shapeslib](/docs/libraries/commands/shapeslib/#loadshape)).
In [AB3 mode](/docs/ab3-mode/) a program uses
AmiBlitz3's libraries, and with them AmiBlitz3's `LoadShape`.

## Forms the original refused {#forms-the-original-refused}

A few fixes of bugs give a meaning to a source that the original
refused, as an extension does, and follow the same rule: a source that
the original compiled keeps its meaning. `Frac` of a string is one of
them
([`Frac` of a string](/docs/compared-with-original/libraries/commands/vallib/#frac-string));
the others are below.

The outputs shown were recorded in an emulator. The original is Blitz
Basic 2 built from its published source, with the libraries of the
Blitz Basic 2 CD. An error is shown with the line number the compiler
reported, counted in the program as it is shown here.
### A bit operator with a constant with a fraction {#bit-operator-fraction}

**What happened.** Under a Word, Long or Float type so far the original
made a constant with a fraction a Float, and a Float has no `AND`, `OR`,
`EOR` or `NOT`:

```
vw.w=1000 : rw.w=0
rw=vw OR 1.5 : NPrint rw
End
```

stopped with "Illegal Operator for Type" in line 2.

**With the port.** `AND`, `OR`, `EOR` and `NOT` take the whole part of a
constant with a fraction, rounded down, in every type: the program
prints `1001` (1000 OR 1). A Float variable is still refused
([Expression type system](/docs/compared-with-original/expressions/type-system/#bit-operators)).

**What to change.** Nothing. A source that must also compile with the
original writes the whole number, `vw OR 1`.

Issue: [The type of an operation depends on the order of its operands and its place in a chain (#1216)](https://github.com/fubar-coder/bb2-c/issues/1216).

### `PlayWait` without a tape number {#playwait-without-tape}

**What happened.** `PlayWait` is meant to wait until the playback of a
tape has finished, and its help shows it without an argument. The
original refused that form:

```
LoadTape 0,"tape"
PlayBack 0
PlayWait
NPrint "status ",XStatus
End
```

The original stopped with "Syntax Error" in line 3. It took only
`PlayWait Tape#`, which plays `Tape#` in place of a playback that is
running and waits for its end, and returns at once when no playback
runs.

**With the port.** `PlayWait` without an argument waits until the running
playback has finished, and returns at once when none runs; the program
above prints `status 0` when the tape is over. `PlayWait Tape#` works as
in the original, except that it returns at once for an empty tape, where
the original waited for ever.

**What to change.** Nothing. A source that must also compile with the
original waits with `Repeat : VWait : Until XStatus=0`.

Issue: [PlayWait only restarts a playback that is already running (#99)](https://github.com/fubar-coder/bb2-c/issues/99).

### `InstallFNS` with a number of characters {#installfns-characters}

**What happened.** `InstallFNS(fontnum,address)` of
[rifnslib](/docs/libraries/userlibs/rifnslib/) puts an
FNS font that a program holds in memory into a font slot. The font
does not say how many characters it has, and the original drew a
character behind its last glyph from the memory behind the font. It
took no third argument: the compiler refused `InstallFNS(1,f,100)`.

**With the port.** A character the font has no glyph for is printed as
a space ([Characters an FNS font has no glyph for](/docs/compared-with-original/libraries/userlibs/rifnslib/#fns-missing-glyphs)).
For that the library needs the number of glyphs. The form
`InstallFNS(fontnum,address,characters)` gives it, counted from the
space on; the form without it takes the 91 glyphs, from the space to
`z`, that every known FNS font has.

**What to change.** Nothing for a font that ends at `z`. Give a font
with more characters its number as the third argument; a source that
must also compile with the original loads such a font with `FNSLoad`,
which takes the number from the size of the file.

Issue: [Character codes from 160 on end the text (sign test after CMPI.b #32) (#657)](https://github.com/fubar-coder/bb2-c/issues/657).

## Warnings

The compiler of the port can warn about a source without refusing it. A
warning never changes the program that is compiled. There is one so far:
two residents that define the same constant with different values give
the warning "Clash in Residents", where the original said nothing. The
value of the first resident is used, by the original and by the port.

The setting **WARNINGS** in the compiler options turns warnings off,
shows them (the default), or makes every warning an error. It is
described with the other options on the page
[The editor and the compiler](/docs/how-blitz2-works/editor-and-compiler/#warnings);
from the Shell, blitz2-ab3 has the options `-w` and `-werr`
([Compiling from the command line](/docs/ab3-mode/command-line/#warnings)).
