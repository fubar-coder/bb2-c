---
title: Extensions
weight: 7
description: What the compiler of the BB2 C port accepts beyond Blitz Basic 2, the rule that keeps old sources unchanged, the four functions it compiles faster, and the animations LoadShape loads.
---

Before this page you should know how the compiler options work
([The editor and the compiler]({{< relref "/docs/how-blitz2-works/editor-and-compiler" >}})).

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
modes ([AB3 mode]({{< relref "/docs/ab3-mode" >}})).

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

In [AB3 mode]({{< relref "/docs/ab3-mode" >}}) the branches stay word
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
([shapeslib]({{< relref "/docs/libraries/commands/shapeslib#loadshape" >}})).
In [AB3 mode]({{< relref "/docs/ab3-mode" >}}) a program uses
AmiBlitz3's libraries, and with them AmiBlitz3's `LoadShape`.

## Warnings

The compiler of the port can warn about a source without refusing it. A
warning never changes the program that is compiled. There is one so far:
two residents that define the same constant with different values give
the warning "Clash in Residents", where the original said nothing. The
value of the first resident is used, by the original and by the port.

The setting **WARNINGS** in the compiler options turns warnings off,
shows them (the default), or makes every warning an error. It is
described with the other options on the page
[The editor and the compiler]({{< relref "/docs/how-blitz2-works/editor-and-compiler#warnings" >}});
from the Shell, blitz2-ab3 has the options `-w` and `-werr`
([Compiling from the command line]({{< relref "/docs/ab3-mode/command-line#warnings" >}})).
