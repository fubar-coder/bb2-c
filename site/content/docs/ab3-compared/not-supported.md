---
title: What AB3 mode does not support yet
weight: 4
description: The AmiBlitz3 features AB3 mode does not compile yet, and how the compiler tells you that a source uses one.
---

Before this page you should know what AB3 mode does support ([Language
additions]({{< relref "/docs/ab3-compared/language" >}})).

AB3 mode is being built up step by step, and it does not cover all of
AmiBlitz3 yet. Many of AmiBlitz3's own test programs already compile and
give the same results as with AmiBlitz3. A source that uses one of the
features below does not compile yet; the compiler then stops with an
error instead of producing a program that behaves differently.

## Floating point on the FPU

AmiBlitz3 can compile floating point arithmetic for a mathematics
coprocessor: the types `.d` and `.f` on the FPU, the directive `OPTIMIZE`
with the FPU value, and the commands `RegLoad` and `RegSave`. AB3 mode
does not generate FPU code. A source that switches it on stops with

```
Not supported: OPTIMIZE 2 (FPU)
```

at the `OPTIMIZE` line.

## 68020 code

AmiBlitz3 can generate code for the 68020 and accepts the 68020's
instructions and addressing modes in inline assembler. AB3 mode accepts
the 68020 value of `OPTIMIZE` but generates 68000 code for the program,
which runs on every Amiga. Inline assembler with 68020 instructions or
addressing modes does not compile.

## Smaller additions

Several smaller additions of AmiBlitz3 are not there yet: type casts with
`(.type)`, `Null`, `Dim` with `KEEP`, constants in `Data` lines, and the
words `USELASTPATH`, `USEPATH2` and `Object`. A compiler word AB3 mode
knows but does not compile yet stops the compile with "Not supported:"
and its name.

## Checks of SYNTAX

`SYNTAX` is accepted, but its declaration and type checks are not made
([Directives: OPTIMIZE, SYNTAX and runtime
errors]({{< relref "/docs/ab3-compared/directives" >}})). AB3 mode
already applies AmiBlitz3's rule for pointers, which holds without
`SYNTAX`: a pointer variable takes only a pointer, a long or an address,
and any other basic value gives "Can't convert types".

## Executables like AmiBlitz3's

AB3 mode aims at programs that behave like AmiBlitz3's, not at
executables that are identical byte for byte. AmiBlitz3's compiler
optimises some code that AB3 mode compiles as Blitz Basic 2 would, and it
writes symbol names in its own way. A program compiled by AB3 mode can
therefore be of another size than AmiBlitz3's, with the same result.
