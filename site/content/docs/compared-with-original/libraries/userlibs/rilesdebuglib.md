---
title: rilesdebuglib
weight: 28
description: The bugs of Blitz Basic 2 in the RWE userlib rilesdebuglib that the BB2 C port fixes.
aliases: [/docs/compared-with-original/fixed-bugs/libraries/userlibs/rilesdebuglib/]
---

{{< fixed-bugs-library >}}

## Changes for existing programs

### `D_FindVolume` returned an address instead of -1 {#d-findvolume-result}

`D_FindVolume(name)` of
[rilesdebuglib]({{< relref "/docs/libraries/userlibs/rilesdebuglib" >}})
tells, as `FindVolume` of riamosfunclib does, whether a device, volume or
assign of that name is there; the argument is the address of the name
without the colon. It returns -1 if the name is there and 0 if it is not.

**What happened.** For a name that is there, the original returned the
address of its entry in the system's list of devices, volumes and
assigns instead of -1, after it had released the list again. This
program asks for `RAM`, which is always there, and for a name that is
not:

```
DEFTYPE .l
v$="RAM"
r=D_FindVolume(&v$)
If r=-1 Then NPrint "RAM: -1"
If r=0 Then NPrint "RAM: 0"
If r<>0 AND r<>-1 Then NPrint "RAM: an address"
v$="RWENOSUCHVOLUME"
NPrint "missing: ",D_FindVolume(&v$)
End
```

**With the original**

```
RAM: an address
missing: 0
```

**With the port**, on Kickstart 1.x as on later ones:

```
RAM: -1
missing: 0
```

**What to change.** A program that tests the result as a truth value, as
in `If D_FindVolume(&v$)`, works as it is. One that compared it with -1
now finds the volume. One that used the address of the entry gets -1; the
entry belonged to a list the program no longer held, so it could change
or go away at any time.

Issue: [D_FindVolume returns the DosList entry after the list is unlocked (#1167)](https://github.com/fubar-coder/bb2-c/issues/1167).

## Changes you may notice

### Labels, register names and inverse text of rilesdebuglib {#rilesdebuglib-changes}

[rilesdebuglib]({{< relref "/docs/libraries/userlibs/rilesdebuglib" >}})
is the helper library of the REDDebugger program by Leading Edge
Software. Three of its fixes change a result that a program may have
relied on.

**`ExtractLabel` ends a label at a space.** `ExtractLabel(address)`
returns the name of a label in a line of text: from the tenth character
on, it searches the first `.`, and the name runs from behind it to a
space, a `:` or the end of the line, as the library's source says. In
the original library a fault of the compiler that built it had turned
the test for a space into a second test for the end of the line, so the
name ran on over spaces:

```
DEFTYPE .l
b=AllocMem_(64,$10001)
If b=0 Then NPrint "no memory" : End
Poke$ b,"012345678.a b"
a$=ExtractLabel(b)
FreeMem_ b,64
NPrint "<",a$,">"
End
```

The original printed `<a b>`, the port prints `<a>`.

**`GetCustomRegAdr` finds a register only by its whole name.**
`GetCustomRegAdr(name$)` returns the offset of the custom chip register
of that name, or -1 if there is none. The original took a register
whose name was only the start of the string: `DMACONXYZ` gave the
offset of `DMACON`. And it never found a register whose name has 8
characters, such as `DENISEID` or `SPR7DATB`:

```
DEFTYPE .l
n$="DMACONXYZ" : NPrint GetCustomRegAdr(n$)
n$="DENISEID" : NPrint GetCustomRegAdr(n$)
n$="SPR7DATB" : NPrint GetCustomRegAdr(n$)
n$="DMACON" : NPrint GetCustomRegAdr(n$)
End
```

The original printed `150`, `-1`, `-1` and `150`; the port prints `-1`,
`124`, `382` and `150`.

**`FInverse` inverts `FPrintDetoke` too.** `FInverse` switches inverse
printing of the font printer on with any value but 0. The original's
`FPrintDetoke` looked only at the upper byte of the value, so
`FInverse 1` inverted the text of `FPrint` and `FNPrint`, but not that
of `FPrintDetoke`. And the inverse space that `FPrintDetoke` draws as
its marker cleared that byte, so after a marker `FInverse $100` was off
for everything printed. With the port, every value but 0 inverts the
text of all three commands, and the marker leaves the setting as it
was.

**What to change.** A program that needs the text up to the `:` takes
it from the line itself. Give `GetCustomRegAdr` the register's name
alone. Before an `FPrintDetoke` that should not be inverted, switch
inverse printing off with `FInverse 0`.

Issues:

* [ExtractLabel does not end the label at a space: every binary has CMPI.b #0 for the source's CMPI.b #" " (compiler bug #343) (#704)](https://github.com/fubar-coder/bb2-c/issues/704)
* [GetCustomRegAdr matches a prefix of the name and never finds a register with an 8-character name (#713)](https://github.com/fubar-coder/bb2-c/issues/713)
* [FInverse sets a word that FPrint tests as a word but FPrintDetoke as its high byte; the cursor marker resets it (#706)](https://github.com/fubar-coder/bb2-c/issues/706)

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in rilesdebuglib {#rilesdebuglib-fixes}

These fixes concern
[rilesdebuglib]({{< relref "/docs/libraries/userlibs/rilesdebuglib" >}}),
the helper library of the REDDebugger program by Leading Edge Software.
The fixes of [`D_FindVolume`'s result](#d-findvolume-result) and of
[`ExtractLabel`, `GetCustomRegAdr` and `FInverse`](#rilesdebuglib-changes)
are described above.

Memory and registers as text:

* A count of 0 makes `D_HexWord`, `D_CheckNMove`, `D_CheckNMoveA` and
  `D_CheckNMoveWord` write nothing, and `GetDataItem` reads a long for
  every size but 1 and 2, as its help text says. The original ran the
  copy 65536 times for a count of 0 and wrote over 300 KB, and
  `GetDataItem` with a size of 0 read 64 KB
  ([#701](https://github.com/fubar-coder/bb2-c/issues/701)).
* Addresses from $80000000 on are no longer taken for low memory:
  `D_HexWord`, `D_CheckNMoveA` and `D_CheckNMoveWord` show their bytes
  instead of `*`, and `GetDataItem` reads them instead of returning 0
  ([#717](https://github.com/fubar-coder/bb2-c/issues/717)).
* `GetHexAdr$` reads a word or long byte by byte, so an odd address
  works on a 68000. The original stopped there with an address error
  ([#1168](https://github.com/fubar-coder/bb2-c/issues/1168)).
* `PeekMax$` returns at most the number of characters asked for. The
  original wrote one more and broke the string expression behind it:
  `PeekMax$(&t$,4)+"|"` gave `abcde`
  ([#718](https://github.com/fubar-coder/bb2-c/issues/718)).

The custom chip registers and the copper:

* `GetCustomReg` and `GetCustomRegAdr` know the last register, $1FE
  `NOOP`; the original stopped at $1FC
  ([#702](https://github.com/fubar-coder/bb2-c/issues/702)).
* `GetCustomReg` of a register with a name of 8 characters returns just
  that name. The original added the first character of the next name and
  broke the string expression behind it: `GetCustomReg($5A)+"|"` gave
  `BLTCON0LB` ([#712](https://github.com/fubar-coder/bb2-c/issues/712)).
* `GetCustomReg` of an offset whose low word is above $1FE gives an empty
  name. The original read a name from in front of its table of names
  ([#714](https://github.com/fubar-coder/bb2-c/issues/714)).
* `GetCopIns$` shows the high digit of a horizontal position or mask of
  `WAIT` and `SKIP` whole: `$2C31,$FFFE` shows as
  `WAIT (30,2C) MASK (FE,FF) [BFD]`, where the original showed
  `(20,2C)` and `(EE,FF)`
  ([#711](https://github.com/fubar-coder/bb2-c/issues/711)).

The font printer:

* `FPrint` prints a string of 32769 characters or more up to the right
  edge, as a shorter one; the original printed nothing
  ([#710](https://github.com/fubar-coder/bb2-c/issues/710)).
* The `>` marker of `FPrintDetoke` is never followed by an inverse
  space; in the original it was when the column counter ran round to 0
  ([#708](https://github.com/fubar-coder/bb2-c/issues/708)).
* With inverse printing on and token names drawn in two bitplanes,
  `FPrintDetoke` draws each character into the second bitplane with its
  own shape. The original drew another character there
  ([#709](https://github.com/fubar-coder/bb2-c/issues/709)).

The parser:

* `D_InitParse` takes at most 255 characters of its string. The original
  copied a longer one over the memory behind its buffer
  ([#716](https://github.com/fubar-coder/bb2-c/issues/716)).
* `ExtractLabel` returns an empty string for a line without a `.` behind
  its first nine characters. The original searched on through the memory
  behind the line and took the label from there
  ([#703](https://github.com/fubar-coder/bb2-c/issues/703)).
* `ProcessString` returns an empty string before `D_InitParse` and after
  the last argument of `D_NextArg$`. The original read the text at
  address 0. Its help text no longer names an argument: the command never
  took one ([#719](https://github.com/fubar-coder/bb2-c/issues/719)).
* A `%` at the end of the text of `ProcessString` ends it; the original
  read on behind the text
  ([#720](https://github.com/fubar-coder/bb2-c/issues/720)).

The memory search:

* `SearchMem` with an empty search string finds nothing and returns 0.
  The original compared the bytes behind the string with memory
  ([#721](https://github.com/fubar-coder/bb2-c/issues/721)).
* A word or long search of `SearchMem` finds a value only if it lies
  wholly in front of the end address. The original also found one that
  went on behind it ([#722](https://github.com/fubar-coder/bb2-c/issues/722)).
* The string search of `SearchMem` finds `ab` in `aab`. After a partial
  match, the original went on behind the mismatch and missed a match
  that began inside the partial one
  ([#723](https://github.com/fubar-coder/bb2-c/issues/723)).

Other fixes:

* A second run of a program from the editor starts the library with its
  defaults: the cursor and the inverse printing of the font printer, the
  result of `SearchMemFound`, the token list of `SetupLESDebug` and the
  codes of `AddProcessPtr`. The original kept them from the run before,
  so a second run printed inverse text after a run that ended with
  `FInverse` on, and `FPrintDetoke` before `SetupLESDebug` read the
  token list of the run before
  ([#1169](https://github.com/fubar-coder/bb2-c/issues/1169)).
* `D_FindVolume` works on Kickstart 1.x: there it reads the list of
  devices, volumes and assigns in the way Kickstart 1.x knows. The
  original called functions of Kickstart 2.0, and a program crashed at
  `D_FindVolume` on Kickstart 1.x
  ([#1148](https://github.com/fubar-coder/bb2-c/issues/1148)).

#### The bitmap of the font printer of rilesdebuglib {#font-printer-bitmap}

The font printer of
[rilesdebuglib]({{< relref "/docs/libraries/userlibs/rilesdebuglib" >}}),
`FPrint`, `FNPrint` and `FPrintDetoke`, prints with an 8x8 font into
the first bitplane of the used bitmap. It writes rows of 80 bytes, so
the bitmap must be 640 pixels wide, and the cursor of `FLocate` counts
rows of 8 pixels.

**What happened.** The original never looked at the size of the bitmap.
On a narrower bitmap the characters landed in the wrong rows, and a
cursor row below the bitmap wrote into the memory behind it. The
columns 208 to 255 were printed into the next rows, and `FPrintDetoke`
drew the character behind a marker without testing its column.

**With the port.** The font printer prints nothing from column 80 on.
With runtime error checking, the three commands
report two errors:

* a bitmap narrower than 640 pixels is the error "Font printer needs a
  bitmap of 640 pixels";
* a cursor whose row of 8 pixels, with the lines of `FYOffset` added,
  does not lie inside the bitmap is the error "Font printer row outside
  the bitmap".

This program stops at `FPrint` with the first error:

```
DEFTYPE .l
BitMap 0,320,16,1
NPrint "before"
FLocate 0,0
FPrint "x"
NPrint "printed"
End
```

Without runtime error checking the commands print as with the original.

**What to change.** Print with the font printer only on a bitmap of 640
pixels or wider, and keep the cursor inside it.

Issue: [FPrint/FPrintDetoke: the column check is a byte compare, columns 208-255 print beyond the row; the bitmap size is never checked (#707)](https://github.com/fubar-coder/bb2-c/issues/707).

#### More than 20 codes of `AddProcessPtr` {#addprocessptr-20-codes}

`AddProcessPtr` of
[rilesdebuglib]({{< relref "/docs/libraries/userlibs/rilesdebuglib" >}})
adds a `%` code that `ProcessString` replaces by a text. Its table has
room for 20 codes besides `%p` and `%c`.

**What happened.** The original did not count the codes: the 21st went
over the end of the table and over the code of the library behind it.

**With the port.** The table holds at most 20 codes, and `AddProcessPtr`
ignores further ones. With runtime error checking, a 21st code is the
error "AddProcessPtr table holds at most 20 entries". `ClearProcessPtrs`
empties the table, and every run of a program starts with an empty one.

**What to change.** Keep to 20 codes, or call `ClearProcessPtrs` before
adding new ones.

Issue: [AddProcessPtr has no limit of 20 entries: the 21st overwrites the code behind the table (#700)](https://github.com/fubar-coder/bb2-c/issues/700).

{{% fixed-bugs-library part="elsewhere" %}}
