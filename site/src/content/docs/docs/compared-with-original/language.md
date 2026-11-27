---
title: Language
description: Fixed bugs of Blitz Basic 2 in how the compiler reads a source and
  what it accepts - arrays and NewTypes, procedures, names and constants,
  include files and paths, macros, residents and inline assembler.
sidebar:
  order: 2
next:
  link: /docs/compared-with-original/expressions/
  label: Expressions
aliases:
  - /docs/compared-with-original/compiler/
  - /docs/compared-with-original/fixed-bugs/compiler/
  - /docs/compared-with-original/new-errors/names-and-types/
  - /docs/compared-with-original/new-errors/files-macros-assembler/
anchorMoves:
  latin1-outside-string: editor
  operators-and-numbers: expressions
  less-or-equal: expressions
  float-literals: expressions
  byte-compare: expressions
  quick-compare: expressions
  quick-long-arithmetic: expressions
  operand-order: expressions
  shift-count-with-point: expressions
  quick-bit-operators: expressions
  float-condition: expressions
  expressions-and-statements: expressions
  division-by-zero: expressions
  mod-fraction-divisor: expressions
  expression-too-complex: expressions
  float-bit-operator-comparison: expressions
  amigaos-functions-and-objects: compiled-programs
  amigaos-call-in-expression: compiled-programs
  object-numbers-above-32k: compiled-programs
  object-function-in-string: compiled-programs
  ending-a-program: compiled-programs
  os-commands-end: compiled-programs
  odd-end: compiled-programs
  the-executable: compiled-programs
  the-progress-bar-while-compiling: running-blitz2
  the-compilers-memory: running-blitz2
---

These bugs of the original compiler concern how it read a source and what
it accepted: the arrays and NewTypes a source declares, procedures, names
and constants, include files and paths, macros, residents and inline
assembler. Some gave a correct source a wrong result or refused it; others
let through a source that should have been refused, with a random result.
The first sections show the ones an ordinary program can run into, each
with a short program. The other fixes follow in tables by topic.

What the compiler makes of an expression is on
[Expressions](/docs/compared-with-original/expressions/),
and the fixes in the program it generates are on
[Compiled programs](/docs/compared-with-original/compiled-programs/).
The table on [Important Fixes & Breaking Changes](/docs/compared-with-original/important-fixes/)
lists all fixed bugs by what they can change in a program.

The outputs shown were recorded in an emulator. The original is Blitz
Basic 2 built from its published source, with the libraries of the Blitz
Basic 2 CD.

## Arrays and NewTypes

### Arrays with 256-byte elements {#arrays-256}

An array whose elements are exactly 256 bytes long, such as an array of a
NewType of 256 bytes, was indexed with the wrong element size: the
original multiplied the index by 65536 instead of 256. The same happened
with a List array of a 248-byte NewType, since a list item adds 8 bytes,
and with a field array `x.t[n]` of 256-byte elements.

**What happened**

```
NEWTYPE .rec
  v.l
  p.b[252]
End NEWTYPE
Dim a.rec(2)
NPrint SizeOf .rec
NPrint &a(1)-&a(0)
NPrint &a(2)-&a(0)
End
```

The program prints the size of an element and the distance of elements 1
and 2 from element 0. The original prints

```
256
65536
131072
```

Element 1 lies 64 KB behind element 0, far outside the array, so reading
or writing it uses memory that belongs to something else.

**With the port**

```
256
256
512
```

Issue: [Arrays with 256-byte elements had their elements 65536 bytes apart (#15)](https://github.com/fubar-coder/bb2-c/issues/15).

### A List array indexed by another array {#list-array-index}

A List array has 8 more bytes per element than a normal array of the same
type, for the list's links. When the index of one kind of array was an
element of the other kind, as in `a(b(1))`, the original used the element
size of the wrong array.

**What happened**

```
Dim List a.l(10)
Dim b.l(5)
b(1)=3
For i.l=0 To 10
  a(i)=i*100
Next
NPrint a(3)
NPrint a(b(1))
End
```

`b(1)` is 3, so both lines should print element 3 of `a`. The original
prints

```
300
100
```

**With the port**

Each array uses its own element size:

```
300
300
```

Issue: [A List array indexed by an element of another array used a wrong element size (#937)](https://github.com/fubar-coder/bb2-c/issues/937).

### A field of a NewType defined later {#newtype-defined-later}

A NewType may have a field of a type that is defined further down in the
source, once a pointer field has named that type. The original gave such
a field a wrong size, so the fields behind it and the size of the whole
NewType were wrong, also after the type was defined.

**What happened**

```blitzbasic
NEWTYPE .pp
  *p.fwd
End NEWTYPE
NEWTYPE .dd
  x.fwd
  y.l
End NEWTYPE
NEWTYPE .fwd
  a.l
  b.l
End NEWTYPE
DEFTYPE .dd v
v\x\b=7
v\y=9
o.l=SizeOf .dd\y
NPrint "y at ",o
NPrint "size ",SizeOf .dd
NPrint v\x\b," ",v\y
End
```

`.fwd` has 8 bytes, so `y` belongs at offset 8 and `.dd` has 12 bytes. The
original prints

```
y at 10
size 14
7 9
```

**With the port**

The NewType is laid out again when the type is defined:

```
y at 8
size 12
7 9
```

**What to change**

A program that worked with the wrong layout, for example one that reads
files written with it, has to account for the new offsets.

Two kinds of source that the original compiled are refused. Using the
size of such a NewType, or a field behind the early field, before the
type is defined is the error "Type not yet defined":

```
NEWTYPE .pp
  *p.fwd
End NEWTYPE
NEWTYPE .dd
  x.fwd
End NEWTYPE
; original: compiles, prints 4    port: "Type not yet defined"
o.l=SizeOf .dd
NEWTYPE .fwd
  a.l
End NEWTYPE
NPrint o
End
```

Two types that would contain each other are "Illegal Type", and a field
of a type that is never defined is "Type Not Found":

```
NEWTYPE .pp
  *p.tb
End NEWTYPE
NEWTYPE .ta
  x.tb
End NEWTYPE
NEWTYPE .tb
  ; original: compiles, prints compiled    port: "Illegal Type"
  y.ta
End NEWTYPE
NPrint "compiled"
End
```

Move a use of the NewType's size behind the definition of the type, and
break a cycle of two types with a pointer field.

Issue: [A NewType field of a type defined later got a wrong size (#974)](https://github.com/fubar-coder/bb2-c/issues/974).

## Procedures

### `End Statement` could close a Function {#end-of-procedure}

A Statement ends with `End Statement` and is left early with
`Statement Return`; a Function ends with `End Function` and is left with
`Function Return`. The original did not check that the keyword fits the
procedure it is in, so a Function closed with `End Statement`, an easy
slip when a Statement is turned into a Function, compiled without an
error.

**What happened**

```
Function$ f{}
  NPrint "in f"
End Statement
NPrint f{}
End
```

The original compiles this program. When it runs, it prints

```
in f
```

and then crashes as `f` returns, instead of printing its result: a string
Function needs the end that `End Function` gives it. Leaving a string
Function with `Statement Return` crashes the same way.

**With the port**

The compiler refuses the source with the error "Illegal End Procedure" at
the line `End Statement`. Each of the other mix-ups is an error as well:
`End Function` inside a Statement is "Illegal End Procedure", and
`Statement Return` inside a Function or `Function Return` inside a
Statement is "Illegal Procedure return".

**What to change**

Close and leave each procedure with the keywords of its own kind:

```
Function$ f{}
  NPrint "in f"
  Function Return "x"
End Function
```

A Statement closes with `End Statement` and is left with
`Statement Return`. Two examples on the Blitz Basic 2 CD,
`DARTS_POLY.BB` and `DARTS_WIRE.BB`, end the Function `gendart` with
`End Statement` and are refused for this reason.

Issue: [End Statement could close a Function, and the other way round (#940)](https://github.com/fubar-coder/bb2-c/issues/940).
## Include files and paths

### Text after `IncDir "…"` {#text-after-path}

The original took the whole rest of a line as the path of `IncDir`, and as
the file name of `Include`, `XInclude` and `IncBin`, including the closing
quote and anything behind it. A statement after a `:` on the same line was
never compiled.

**What happened**

```blitzbasic
a=1
IncDir "RAM:" : a=2
NPrint a
End
```

The original prints `1`: the `: a=2` became part of the path, and the
assignment was never compiled.

**With the port**

A path or name in quotes ends at its closing quote, and a `:` after
`IncDir` and `IncBin` starts the next statement as usual. The program
prints

```
2
```

**What to change**

After `Include` and `XInclude`, only a comment may follow the closing
quote; a statement behind a `:` there is a "Syntax Error":

```
Include "inc.bb2" : a=1
NPrint a
End
; original: "Can't open Include"    port: "Syntax Error", both in line 1
```

The original failed even where `inc.bb2` exists, since the rest of the
line was part of the file name. Other text after the closing quote is a
"Syntax Error" too. A path without quotes still takes the rest of the
line. Put a statement that follows `Include` or `XInclude` on its own
line.

Issue: [Text after an IncDir path became part of the path (#17)](https://github.com/fubar-coder/bb2-c/issues/17).

### `IncDir` stayed for the next compile {#incdir-next-compile}

The path set by `IncDir` was never cleared. In one session of the editor,
every source compiled after a source with `IncDir`, also after a resident
made from one, looked for its include files in that directory.

**What happened**

First a resident is made from a source with the line

```
IncDir "CORPUS:nowhere/"
```

Then this program, which has an `inc.bb2` next to it, is compiled in the
same session:

```
Include "inc.bb2"
NPrint #k
End
```

The original looks for `CORPUS:nowhere/inc.bb2` and stops with "Can't open
Include" at the first line.

**With the port**

Every compile starts without an `IncDir` path. The program finds its
include file and prints the constant `#k` defined there:

```
7
```

Issue: [The path of IncDir stayed for the following compiles (#342)](https://github.com/fubar-coder/bb2-c/issues/342).

### An include name with a device after `IncDir` {#include-with-device}

After an `IncDir`, the original put its path in front of every file name,
also of a name that already starts with a device or an assign. So after
`IncDir "BlitzLibs:"` the line `Include "blitzlibs:libnums.bb"` opened
`BlitzLibs:blitzlibs:libnums.bb`.

**What happened**

```
IncDir "CORPUS:sub/"
Include "CORPUS:src/inc.bb2"
NPrint #k
End
```

The original opens `CORPUS:sub/CORPUS:src/inc.bb2` and stops with "Can't
open Include" at the second line.

**With the port**

A name with a device or assign is used as written, for `Include`,
`XInclude` and `IncBin`. The program prints

```
7
```

AB3 mode still puts the `IncDir` path in front of every name.

Issue: [After an IncDir, a file name with a device or assign could not be included (#341)](https://github.com/fubar-coder/bb2-c/issues/341).

## Other fixes

The fixes below need rarer conditions. Each row of a table says what the
original did and what the port does, and links the issue with the
details. A fix that makes the port refuse a source the original accepted,
or that needs more room, follows the table of its group as an entry of
its own, with a program whose comments give what the original did and
what the port does, and what to change in a source. A program that the
port compiles is run by the project's tests; one that it refuses is shown
as plain text.

### Names, labels and constants

| What was wrong | Now | Issue |
|---|---|---|
| A jump to a label with the name of a constant of value 0 gave a nonsense error at a line that does not exist, or read and wrote random memory. | It gives "Label has been used as a Constant", as for any other value. | [#977](https://github.com/fubar-coder/bb2-c/issues/977) |
| `Supervisor_`, the first AmigaOS function of exec, gave "Syntax Error": the compiler looked it up in the dos library, whose token numbers ran into exec's. | It compiles. | [#960](https://github.com/fubar-coder/bb2-c/issues/960) |

#### A constant definition without a name {#constant-without-name}

The original accepted `#=5`, a constant with an empty name. The port
refuses it.

```
; original: compiles, prints ok    port: "Syntax Error"
#=5
NPrint "ok"
End
```

A second such line made the original's compiler search 65536 bytes of
memory for the name.

**What to change:** give the constant a name, or remove the line.

Issue: [A constant definition without a name was accepted (#7)](https://github.com/fubar-coder/bb2-c/issues/7).

#### A `#`, `Goto` or `Gosub` without a name {#missing-name}

A `#`, `Goto`, `Gosub`, `Restore`, `?` or `.` without a name made the
original search its lists for a name of length 0 and report an error
that did not fit. The port reports the missing name.

```
a=#+1           ; original: "Constant Not Found"    port: "Syntax Error"
NPrint a
End
```

```
Goto            ; port: "Syntax Error"
Goto
NPrint "ok"
End
; original: "Label not Found"
```

The original compared up to 65536 bytes of memory with the empty name
before it gave up.

**What to change:** write the missing name.

Issue: [A #, Goto, Gosub, Restore, ? or . without a name made the compiler read far past the name (#30)](https://github.com/fubar-coder/bb2-c/issues/30).

#### A constant without a value {#constant-without-value}

A line with only a constant name, without `=` and a value, was accepted,
but the original never gave the constant a value. In the port such a
constant is 0.

```blitzbasic
#c
#d=5
#d
NPrint #c               ; original: a random value    port: 0
CNIF #c
NPrint "cnif true"      ; original: this line, in the recording
CELSE
NPrint "cnif false"     ; port: this line
CEND
NPrint #d               ; original: 5    port: 5
End
```

The original's constant held whatever was in the compiler's memory, so
`CNIF #c` could be true or false from one compile to the next. In the
recording, with the compiler's memory filled with the byte `$A5`, it
printed -1515870811 and `cnif true`. A constant that already had a value,
as `#d`, keeps it in both.

**What to change:** give the constant the value it needs, `#c=1` for
example.

Issue: [A constant defined without a value held a random value (#972)](https://github.com/fubar-coder/bb2-c/issues/972).

#### A name of more than 127 characters {#long-name}

The original copied a name into a buffer of 128 bytes without a limit,
and a longer name overwrote the compiler's data. The port accepts names
of up to 127 characters.

```
Macro s
`1`1`2=1
NPrint `1`1`2
End Macro
; a name of 127 characters    original: prints 1    port: prints 1
!s{aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,aaaaaaa}
; a name of 128 characters    original: prints 1    port: "Syntax Error"
!s{bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb,bbbbbbbb}
End
```

A line of the editor has at most 128 characters, so such a name can only
come from a macro that joins its parameters, as `s` does here.

**What to change:** use names of at most 127 characters.

Issue: [A name of more than 127 characters overwrote the compiler's data (#29)](https://github.com/fubar-coder/bb2-c/issues/29).

### Strings in the source {#strings-in-the-source}

| What was wrong | Now | Issue |
|---|---|---|
| A Latin-1 character other than the umlauts, such as ß or é, could not reach a string, since the editor lost it. | The port's editor keeps it ([ß, é and other accented letters were lost](/docs/compared-with-original/editor/#latin1-in-strings)). It gives its byte in a string, a `Data$` or `Dc.b` string and a quoted constant; outside a string it is a "Syntax Error" ("Token Not Found" in AB3 mode). In AB3 mode, the Latin-1 characters in the strings and comments of an AmiBlitz3 text source give the bytes AmiBlitz3 gives them. | [#1018](https://github.com/fubar-coder/bb2-c/issues/1018) |

#### A `~` inside a string {#tilde-in-string}

Outside strings, `~expr~` puts the value of a constant expression into the
source. The original did this inside strings too; in the port a `~`
inside quotes is an ordinary character.

```blitzbasic
NPrint "a~1+2~b"    ; original: a3b    port: a~1+2~b
NPrint ~1+2~        ; original: 3      port: 3
NPrint "x~y"        ; original: "Illegal Constant Expression"    port: x~y
End
```

A single `~` in a string, as in the third line, was the compile error
"Illegal Constant Expression" in the original, so the original compiles
the program only without that line.

**What to change:** build a text from constants outside the quotes.

Issue: [A ~ inside a string literal started a constant expression (#966)](https://github.com/fubar-coder/bb2-c/issues/966).

### Statements {#statements}

#### `For` with a string variable {#for-string}

The original refused a local string variable as the variable of a `For`
loop, but accepted a global one and treated the string as a number. The
port refuses both.

```
; original: compiles, prints done    port: "Bad Type for For...Next"
For a$="a" To 10
Next
NPrint "done"
End
```

**What to change:** use a numeric variable for the loop.

Issue: [For a$=… To … with a global string variable compiled (#39)](https://github.com/fubar-coder/bb2-c/issues/39).

#### `Dcb` with a count of 0 or less {#dcb-count}

The original took the count of `Dcb` as an unsigned number, so a count of
0 or -1 made about four thousand million copies. In the port a count of 0
emits nothing and a negative count is an error.

```blitzbasic
Goto skip
; original: the compiler does not finish    port: no data
Dcb.w 0,0
.skip
NPrint "ok"
End
```

```
Goto skip
; original: the compiler does not finish    port: "Illegal Constant"
Dcb.w -1,0
.skip
NPrint "ok"
End
```

The original's compiler ran for hours, or until it had overwritten its
own memory.

**What to change:** leave out a `Dcb` with a negative count.

Issue: [Dcb with a count of 0 made 2^32 copies (#37)](https://github.com/fubar-coder/bb2-c/issues/37).

### NewTypes

#### A NewType that contains itself {#newtype-contains-itself}

A NewType cannot contain a field of its own type: it would have to be
larger than itself. The original gave such a field a random size; the
port refuses it.

```
NEWTYPE .selft
  x.selft                       ; port: "Illegal Type"
  y.l
End NEWTYPE
DEFTYPE .selft v
v\y=5
o.l=SizeOf .selft\y
NPrint "size ",SizeOf .selft    ; original: size 262
NPrint "y at ",o                ; original: y at 258
NPrint "y ",v\y                 ; original: y 5
End
```

In the original the fields behind such a field and the size of the whole
type were random too; the values above were recorded with the compiler's
memory filled with the byte `$01`. What such a source usually means is a
pointer to another item of the same type, and a pointer field is
allowed:

```blitzbasic
NEWTYPE .node
  value.l
  *child.node
End NEWTYPE
```

Issue: [A NewType field of its own type took a random size (#973)](https://github.com/fubar-coder/bb2-c/issues/973).

#### An array field of 65536 elements or more {#newtype-field-count}

A field of a NewType can be an array of a fixed number of elements, such
as `a.b[10]`, a field of ten bytes. The original kept the count in 16
bits; the port checks the whole count.

```
NEWTYPE .t
  a.b[65540]        ; original: a field of 4 bytes    port: "Type too Big"
End NEWTYPE
NPrint SizeOf .t    ; original: 4
End
```

A NewType must stay below 32768 bytes, so a larger one is the error
"Type too Big". The original took a count of 65536 or more, and a
negative one such as `a.b[-65532]`, modulo 65536, so the NewType passed
the size check. With runtime error checking an index behind the fourth
element stopped the program; without it, a program that used those
elements wrote over the memory behind the field.

**What to change:** give the array a count that fits the NewType, or keep
the data in an array of its own with `Dim`.

Issue: [An array field's element count of 65536 or more is taken modulo 65536 without an error (#1084)](https://github.com/fubar-coder/bb2-c/issues/1084).

### Paths and include files

| What was wrong | Now | Issue |
|---|---|---|
| `IncBin` kept a file or directory in use after an error. | It is closed. | [#38](https://github.com/fubar-coder/bb2-c/issues/38) |

#### Paths that are too long {#path-too-long}

The original copied paths into buffers of a fixed size without a limit.
The port stops with "Path too Long" at the line of the path.

```
; a path of 98 characters
; original: compiles, the program crashes    port: "Path too Long"
IncDir "ppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppp"
x=5
NPrint x
End
```

The original's program crashed with a "Software Failure" instead of
printing 5. Three limits are checked:

* an `IncDir` path of 96 characters or more, which overwrote the
  compiler's data;
* the `IncDir` path and the name of an `Include`, `XInclude` or `IncBin`
  together, which could overwrite the compiler's data from 128
  characters on;
* a `UsePath` path of 128 characters or more, possible through a macro,
  which overwrote the compiler's own code, so that the compiler did not
  finish.

**What to change:** use a shorter path, for example with an assign.

Issues:
[IncDir path of 96 characters or more (#16)](https://github.com/fubar-coder/bb2-c/issues/16),
[include file name of more than 127 characters (#325)](https://github.com/fubar-coder/bb2-c/issues/325),
[UsePath path of 128 characters or more (#31)](https://github.com/fubar-coder/bb2-c/issues/31).

### Macros

| What was wrong | Now | Issue |
|---|---|---|
| A one-line macro with a colon right after its name, `Macro a:1:End Macro`, expanded to a text starting with a colon. | `!a` expands to `1`. | [#942](https://github.com/fubar-coder/bb2-c/issues/942) |
| An error inside a macro definition could crash the compiler. | The error is shown. | [#965](https://github.com/fubar-coder/bb2-c/issues/965) |
| A compile error on a line that came from a macro kept the memory of the expanded line, in a nested macro also that of the outer one, until the next compile or for good. | The memory is freed with the error. | [#1203](https://github.com/fubar-coder/bb2-c/issues/1203) |
| A line typed in the debugger's direct mode that used a macro kept the memory of the expanded line until the next macro or compile, and for good if blitz2 ended first. | The memory is freed once the line is compiled. | [#1204](https://github.com/fubar-coder/bb2-c/issues/1204) |
| A macro whose last line ended in some commands or operators (`NOT`, `OR`, `MOD`, `Print`, `Len` and others) kept a colon at its end, so a calling line that went on after the macro, as in `c = !m b` with the text `NOT`, gave "Syntax Error". | The macro text ends without the colon. | [#1206](https://github.com/fubar-coder/bb2-c/issues/1206) |
| The blank between a macro call and the rest of its line was dropped, so a macro text ending in a letter or digit was glued to the next name or number: `!mw b` with the text `!mq` became `!mqb` ("Macro not Found" or another macro). | The blank is kept. | [#1207](https://github.com/fubar-coder/bb2-c/issues/1207) |

#### A macro line ending in `End` {#macro-end-line}

In the original a line of a macro that ended in `End` cut off the rest of
the macro and of the line that called it. The port keeps the rest of the
macro, as AmiBlitz3 does.

```blitzbasic
Macro m
  If `1
    End
  End If
  NPrint "after"
End Macro
!m{0}            ; original: "If Without End If"    port: after
NPrint "done"    ; port: done
End
```

The original lost the `End If` and the rest of the macro, so it stopped
at the call of the macro.

**What to change:** nothing, unless a source relied on the cut; then
remove the lines that the original left out.

Issue: [A macro line ending in End cut off the rest of the macro (#968)](https://github.com/fubar-coder/bb2-c/issues/968).

#### A macro larger than the Macro Buffer {#macro-buffer}

The compiler options have a **Macro Buffer** size. In the original it had
no effect: the buffer for macro texts always had its default size of
8192 bytes. In the port the option counts, in both directions.

A source with a macro text of 9120 bytes, whose options set the Macro
Buffer to 16384 bytes, was refused by the original with "Macro too Big"
inside the macro, and blitz2 hung afterwards; the port compiles it. A
source with a macro text of 1520 bytes, whose options set the buffer to
1024 bytes, compiled in the original; the port gives "Macro too Big" at
the line where the macro's text passes the size. A source that set
another size could also make blitz2 crash when it ended. A macro text of
more than 65535 bytes is always "Macro too Big"; the original cut such a
text, and part of its memory was lost until a reboot.

The buffer gets its new size when a source is loaded or the COMPILER
OPTIONS are closed. If there is not enough memory for it, the compiler
reports out of memory and keeps the old buffer with its size; the next
change of the options tries again.

**What to change:** a source saved with the default size is not affected.
If one of yours fails with "Macro too Big", set its Macro Buffer back to
the default or larger in the COMPILER OPTIONS.

Issues:
[The Macro Buffer option had no effect (#336)](https://github.com/fubar-coder/bb2-c/issues/336),
[A macro text of 65536 bytes or more was cut (#964)](https://github.com/fubar-coder/bb2-c/issues/964).

### Residents

| What was wrong | Now | Issue |
|---|---|---|
| The last constant of a resident pointed behind the resident, so every compile freed memory that did not belong to it. | The list ends where it should. | [#34](https://github.com/fubar-coder/bb2-c/issues/34) |
| Resident files contained bytes nobody had set, so CREATE RESIDENT gave a different file each time. | Those bytes are zero. | [#970](https://github.com/fubar-coder/bb2-c/issues/970) |
| A resident with a `NewType` without fields lost the types, macros and constants behind it when it was loaded. | Everything is loaded as defined. | [#35](https://github.com/fubar-coder/bb2-c/issues/35) |
| A resident with a macro text of 32767 bytes or more loaded the definitions behind it from the wrong place. | It is written and loaded correctly. | [#995](https://github.com/fubar-coder/bb2-c/issues/995) |
| Two residents that defined the same constant with different values were never noticed, and the first resident's value was used silently. | The compiler gives the warning "Clash in Residents" with the name of the constant ([Warnings](/docs/how-blitz2-works/editor-and-compiler/#warnings)) and still uses the first value. | [#36](https://github.com/fubar-coder/bb2-c/issues/36) |

### Inline assembler

| What was wrong | Now | Issue |
|---|---|---|
| After a line with a calculation on a character constant, such as `MOVE.w #">"*8,d0`, a `#" "` on the lines before it could become 0. | Character constants are read correctly on every line. | [#343](https://github.com/fubar-coder/bb2-c/issues/343) |
| An inline assembler expression of 240 characters or more in a macro or include was freed with a wrong size. | It is freed correctly. | [#1](https://github.com/fubar-coder/bb2-c/issues/1) |
| `PutReg` into a local variable of a `Statement` or `Function` stored the register next to the global variables, overwriting 1 to 4 bytes there, and the local kept its old value. | The register is stored into the local, and a program that reads the local afterwards sees its value: GMODPLAY's `GMInitMusic` on the CD reports a failed init of a module, where with the original it always reported success. | [#1202](https://github.com/fubar-coder/bb2-c/issues/1202) |

#### An operand that is too long {#operand-too-long}

The compiler keeps the text of an inline assembler operand in a buffer of
a fixed size when the text comes from a macro or an include, or contains a
local label. The original copied the text without a limit; the port stops
with "Operand too Long".

```
Macro m
move.l #`1+`1,dat
End Macro
Goto skip
.dat
Dc.l 0
.skip
; a source operand of 127 characters
; original: "Syntax Error"    port: "Operand too Long"
!m{1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1}
NPrint Peek.l(?dat)
End
```

There is one buffer for the source operand and `Dc`, and one for the
destination operand right behind it. In the original a source operand of
more than 125 characters reached into the destination's buffer, and the
destination overwrote its end: wrong code or "Syntax Error", as here,
where the program should store 64. Longer texts overwrote the compiler's
data behind the buffers: a source operand or `Dc` of more than 261
characters, a destination operand of more than 133. The port reports
each of these at the line of the instruction, here the call of the
macro. Every text that fits compiles as before, also a source operand of
up to 261 characters when the destination is a register.

**What to change:** split the expression, for example into a constant
defined before the instruction, or use shorter macro parameters.

Issue: [copyconst copies an operand text into namebuff without a length check (#997)](https://github.com/fubar-coder/bb2-c/issues/997).
