---
title: Forms the original refused
weight: 5
description: Sources that Blitz Basic 2 refused and the BB2 C port compiles - bit operators with a constant with a fraction, PlayWait without a tape number and InstallFNS with a number of characters.
---

Most sources of this chapter go one way: the original compiled them and
the port does not, or compiles them to something else. The cases on this
page go the other way, as
[`Frac` of a string]({{< relref "/docs/compared-with-original/new-errors/runtime-errors#frac-string" >}})
does: a source that the original refused compiles with the port.

The outputs shown were recorded in an emulator. The original is Blitz
Basic 2 built from its published source, with the libraries of the
Blitz Basic 2 CD. An error is shown with the line number the compiler
reported, counted in the program as it is shown here.

## A bit operator with a constant with a fraction {#bit-operator-fraction}

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
([The type of each operation]({{< relref "/docs/compared-with-original/expression-types#bit-operators" >}})).

**What to change.** Nothing. A source that must also compile with the
original writes the whole number, `vw OR 1`.

Issue: [The type of an operation depends on the order of its operands and its place in a chain (#1216)](https://github.com/fubar-coder/bb2-c/issues/1216).

## `PlayWait` without a tape number {#playwait-without-tape}

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

## `InstallFNS` with a number of characters {#installfns-characters}

**What happened.** `InstallFNS(fontnum,address)` of
[rifnslib]({{< relref "/docs/libraries/userlibs/rifnslib" >}}) puts an
FNS font that a program holds in memory into a font slot. The font
does not say how many characters it has, and the original drew a
character behind its last glyph from the memory behind the font. It
took no third argument: the compiler refused `InstallFNS(1,f,100)`.

**With the port.** A character the font has no glyph for is printed as
a space ([Characters an FNS font has no glyph for]({{< relref "/docs/compared-with-original/libraries/userlibs/rifnslib#fns-missing-glyphs" >}})).
For that the library needs the number of glyphs. The form
`InstallFNS(fontnum,address,characters)` gives it, counted from the
space on; the form without it takes the 91 glyphs, from the space to
`z`, that every known FNS font has.

**What to change.** Nothing for a font that ends at `z`. Give a font
with more characters its number as the third argument; a source that
must also compile with the original loads such a font with `FNSLoad`,
which takes the number from the size of the file.

Issue: [Character codes from 160 on end the text (sign test after CMPI.b #32) (#657)](https://github.com/fubar-coder/bb2-c/issues/657).
