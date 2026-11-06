---
title: "Directives: OPTIMIZE, SYNTAX and runtime errors"
weight: 2
description: AmiBlitz3's compiler directives OPTIMIZE, SYNTAX, RunErrsOff and LastRunErrsMode, BSS and Chip, and what AB3 mode does with them.
---

Before this page you should know what new syntax is ([Language
additions]({{< relref "/docs/ab3-compared/language" >}})).

AmiBlitz3 sources control the compiler with a few directives in the
source text. AB3 mode understands them; where a feature behind a
directive is missing, it says so.

## OPTIMIZE

`OPTIMIZE` takes a number whose bits switch features on. The value
replaces the previous one, from that line on; every compile starts with
all bits off.

| Value | In AmiBlitz3 | In AB3 mode |
|---|---|---|
| 1 | code for the 68020 | accepted, but the code stays 68000 code |
| 2 | floating point on the FPU | refused: "Not supported: OPTIMIZE 2 (FPU)" |
| 4 | new syntax | supported |
| 8 | no effect | accepted |
| 16 | shorter branches | accepted; the program behaves the same, its size may differ from AmiBlitz3's |

The values add up: `OPTIMIZE 5` is 1 and 4.

The common values are 4 (new syntax), 5 (new syntax and 68020) and 7
(new syntax, 68020 and FPU). Sources with 4 or 5 compile; a source with 7,
or any other value with the FPU bit, stops at the `OPTIMIZE` line:

```
lang2.ab3:2: Not supported: OPTIMIZE 2 (FPU)
```

A program compiled with the 68020 bit runs on every Amiga, since AB3 mode
generates 68000 code for it.

## SYNTAX

`SYNTAX` followed by a digit selects how strictly AmiBlitz3 checks
declarations and types. AB3 mode accepts the line but does not make these
checks: a correct program compiles to the same code with or without it,
and a program that AmiBlitz3 would refuse for a missing declaration may
compile.

## RunErrsOff and LastRunErrsMode

`RunErrsOff` switches runtime error checking off from that line, as in
Blitz Basic 2. In AB3 mode it also remembers the mode it replaced, and
`LastRunErrsMode` brings that mode back. So a part of a program can be
compiled without checks, and the checks return after it to whatever the
compiler options said:

```
RunErrsOff
NPrint "unchecked"
LastRunErrsMode
```

`RunErrsOn` switches checking on, as in Blitz Basic 2. `LastRunErrsMode`
without a `RunErrsOff` before it is an error.

## BSS and Chip

In an AmiBlitz3 executable, the data after the directive `BSS` goes into a
hunk of memory cleared at start, and the data after `Chip` into a hunk
loaded into chip memory, where the custom chips can reach it. AB3 mode
writes these hunks as AmiBlitz3 does. Each directive may appear only once
in a program. With runtime error checking on, both are ignored, as in
AmiBlitz3.
