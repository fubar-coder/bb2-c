---
title: Names, procedures and NewTypes
weight: 2
description: Accented letters, constants and labels without a name or a value, long names, the end of a procedure and NewTypes that Blitz Basic 2 compiled and the BB2 C port refuses or compiles differently.
---

These sources compiled with Blitz Basic 2, mostly to a wrong or random
result. The BB2 C port refuses them or compiles them to what they say.
Each program can be copied; a comment
behind a line, or above it, gives what the original did and what the port
does with that line. A program that the port compiles is run by the
project's tests; one that it refuses is shown as plain text. The
original's results were recorded in an emulator, with Blitz Basic 2 built
from its published source and the libraries of the Blitz Basic 2 CD.

## Characters

### An accented letter outside a string {#latin1-outside-string}

The original editor lost a Latin-1 letter other than the umlauts ä ö ü
Ä Ö Ü, such as ß or é, as you typed it, so a name with it became another
name without a message. The port's editor keeps the letter, and the
compiler accepts it only in strings and comments.

```
; original: the editor makes the line caf=1 : NPrint caf
; port:     "Syntax Error"
café=1 : NPrint café
```

In the port such a letter in a name or as a statement stops the compiler
with "Syntax Error" at its line; in AB3 mode the message is "Token Not
Found". In a text file loaded into the original, the letter was taken for
a keyword. The editor's side of the fix is on
[ß, é and other accented letters were lost]({{< relref "/docs/compared-with-original/fixed-bugs/editor#latin1-in-strings" >}}).

**What to change:** write names with plain letters, and accented letters
only in strings and comments.

Issue: [Latin-1 characters in strings and comments (#1018)](https://github.com/fubar-coder/bb2-c/issues/1018).

## Constants, labels and names

### A constant definition without a name {#constant-without-name}

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

### A `#`, `Goto` or `Gosub` without a name {#missing-name}

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

### A constant without a value {#constant-without-value}

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

### A name of more than 127 characters {#long-name}

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

## Procedures

### The end of a procedure of the other kind {#end-of-procedure}

The original accepted `End Statement` and `Statement Return` inside a
Function, and `End Function` and `Function Return` inside a Statement.
The port checks that the end fits the kind of procedure.

```
Function$ f{}
  NPrint "in f"
; original: compiles; the program prints in f and crashes
; port:     "Illegal End Procedure"
End Statement
NPrint f{}
End
```

In the original the program crashed when `f` returned. In the port the
`End` of the other kind gives "Illegal End Procedure", the `Return` of the
other kind "Illegal Procedure return", at its line. Two examples on the
Blitz Basic 2 CD, `DARTS_POLY.BB` and `DARTS_WIRE.BB`, end the Function
`gendart` with `End Statement` and are refused for this reason. The bug
is shown in full on
[`End Statement` could close a Function]({{< relref "/docs/compared-with-original/fixed-bugs/compiler#end-of-procedure" >}}).

**What to change:** close a Function with `End Function` and leave it with
`Function Return`, a Statement with `End Statement` and `Statement Return`.

Issue: [End Statement could close a Function, and the other way round (#940)](https://github.com/fubar-coder/bb2-c/issues/940).

## NewTypes

### A NewType that contains itself {#newtype-contains-itself}

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

### A field of a NewType defined later {#newtype-defined-later}

A NewType may have a field of a type that is defined further down, as long
as a pointer field has named that type before. The original gave such a
field a wrong size, and it stayed wrong after the type was defined; the
port lays the NewType out again when the type is defined.

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
NPrint "y at ",o            ; original: y at 10    port: y at 8
NPrint "size ",SizeOf .dd   ; original: size 14    port: size 12
NPrint v\x\b," ",v\y        ; original: 7 9        port: 7 9
End
```

`.fwd` has 8 bytes, so `y` belongs at offset 8. Using the size of such a
NewType, or a field behind the early field, before the type is defined
is the error "Type not yet defined":

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

**What to change:** a program that worked with the wrong layout, for
example one that reads a file written with it, has to account for the new
offsets. Move a use of the NewType's size behind the definition of the
type, and break a cycle of two types with a pointer field.

Issue: [A NewType field of a type defined later got a wrong size (#974)](https://github.com/fubar-coder/bb2-c/issues/974).

### An array field of 65536 elements or more {#newtype-field-count}

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
