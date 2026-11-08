---
title: Sources that compile differently
weight: 2
description: Sources that Blitz Basic 2 accepted and the BB2 C port refuses or compiles to something else, with what the original did and how to change them.
---

Before this page you should know the fixed bugs of the compiler
([Fixed original bugs]({{< relref "/docs/compared-with-original/fixed-bugs" >}})).

Almost every source compiles with the port exactly as with the original.
The exceptions on this page all come from fixed bugs: the original
accepted these sources, but compiled them to a wrong or random result, or
crashed while compiling them. Each case shows a small program, what the
original did with it, what the port does, and what to change in a source
that relies on the old behaviour.

The outputs shown were recorded in an emulator. The original is
Blitz Basic 2 built from its published source, with the libraries of the
Blitz Basic 2 CD. An error is shown with the line number the compiler
reported, counted in the program as it is shown here.

## Operators and expressions

### `=<` as "not equal" {#less-or-equal}

**What happened.** The original compiled the comparison `=<` as "not
equal", without any message:

```
a=1:b=2:NPrint a=<b
a=2:b=2:NPrint a=<b
a=3:b=2:NPrint a=<b
```

printed `-1`, `0` and `-1`: true where the two values differ.

**With the port.** `=<` is less-or-equal, as `<=`. The same lines print
`-1`, `-1` and `0`.

**What to change.** Where `=<` was meant as less-or-equal, nothing: the
program now does what its source says. Where a program relied on `=<`
meaning "not equal", write `<>`. The bug is shown in full on its own page
([`=<` meant "not equal"]({{< relref "/docs/compared-with-original/fixed-bugs/compiler#less-or-equal" >}})).

Issue: [The operator =< meant "not equal" (#9)](https://github.com/fubar-coder/bb2-c/issues/9).

### A `~` inside a string {#tilde-in-string}

Outside strings, `~expr~` puts the value of a constant expression into the
source: `NPrint ~1+2~` prints `3`.

**What happened.** The original did the same inside string literals:

```
NPrint "a~1+2~b"
NPrint ~1+2~
End
```

printed

```
a3b
3
```

and a single `~` in a string, as in `NPrint "x~y"`, was the error
"Illegal Constant Expression".

**With the port.** A `~` inside quotes is an ordinary character. The
program prints

```
a~1+2~b
3
```

**What to change.** A source that used `~...~` inside a string to build a
text from constants has to build that text outside the quotes.

Issue: [A ~ inside a string literal started a constant expression (#966)](https://github.com/fubar-coder/bb2-c/issues/966).

### `For` with a string variable {#for-string}

**What happened.** The original refused a local string variable as the
variable of a `For` loop, but accepted a global one and treated the
string as a number:

```
For a$="a" To 10
Next
NPrint "done"
End
```

compiled and printed `done`.

**With the port.** The `For` line gives "Bad Type for For...Next" (line 1),
as for a local string variable.

**What to change.** Use a numeric variable for the loop.

Issue: [For a$=… To … with a global string variable compiled (#39)](https://github.com/fubar-coder/bb2-c/issues/39).

### `Dcb` with a count of 0 or less {#dcb-count}

**What happened.** The original took the count of `Dcb` as an unsigned
number. A count of 0 or -1 made about four thousand million copies, and
the compiler ran for hours or until it had overwritten its memory:

```
Goto skip
Dcb.w -1,0
.skip
NPrint "ok"
End
```

did not finish compiling.

**With the port.** A count of 0 emits nothing, and a negative count gives
"Illegal Constant" (line 2 here).

**What to change.** Leave out a `Dcb` with a negative count; a count of 0
needs no change.

Issue: [Dcb with a count of 0 made 2^32 copies (#37)](https://github.com/fubar-coder/bb2-c/issues/37).

## Constants, labels and names

### A constant definition without a name {#constant-without-name}

**What happened.** The original accepted `#=5`: a constant with an empty
name.

```
#=5
NPrint "ok"
End
```

compiled and printed `ok`. A second such line made the compiler search
65536 bytes of memory for the name.

**With the port.** `#=5` gives "Syntax Error" (line 1).

**What to change.** Give the constant a name, or remove the line.

Issue: [A constant definition without a name was accepted (#7)](https://github.com/fubar-coder/bb2-c/issues/7).

### A `#`, `Goto` or `Gosub` without a name {#missing-name}

**What happened.** A `#`, `Goto`, `Gosub`, `Restore`, `?` or `.` without a
name made the original search its lists for a name of length 0, comparing
up to 65536 bytes of memory. It then reported an error that did not fit:

```
a=#+1
NPrint a
End
```

gave "Constant Not Found" (line 1), and

```
Goto
Goto
NPrint "ok"
End
```

gave "Label not Found".

**With the port.** Both give "Syntax Error" at the line without the name
(line 1).

**What to change.** Write the missing name.

Issue: [A #, Goto, Gosub, Restore, ? or . without a name made the compiler read far past the name (#30)](https://github.com/fubar-coder/bb2-c/issues/30).

### A constant without a value {#constant-without-value}

**What happened.** A line with only a constant name, without `=` and a
value, was accepted, but the original never gave the constant a value:
it held whatever was in the compiler's memory, so `CNIF #c` could be true
or false from one compile to the next.

```
#c
#d=5
#d
NPrint #c
CNIF #c
NPrint "cnif true"
CELSE
NPrint "cnif false"
CEND
NPrint #d
End
```

In the recording, with the compiler's memory filled with the byte `$A5`,
the original printed

```
-1515870811
cnif true
5
```

A constant that already had a value, as `#d`, kept it.

**With the port.** A constant without a value is 0, and `#d` keeps its 5.
The program prints

```
0
cnif false
5
```

**What to change.** Give the constant the value it needs, `#c=1` for
example.

Issue: [A constant defined without a value held a random value (#972)](https://github.com/fubar-coder/bb2-c/issues/972).

### A name of more than 127 characters {#long-name}

A line of the editor has at most 128 characters, so such a name can only
come from a macro that joins its parameters.

**What happened.** The original copied the name into a buffer of 128
bytes without a limit, and a longer name overwrote the compiler's data:

```
Macro s
`1`1`2=1
NPrint `1`1`2
End Macro
!s{aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa,aaaaaaa}
!s{bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb,bbbbbbbb}
End
```

The first call makes a name of 127 characters, the second one of 128. The
original compiled the program, and it printed `1` twice.

**With the port.** A name of up to 127 characters is accepted, a longer
one is a "Syntax Error": here at the second call, line 6.

**What to change.** Use names of at most 127 characters.

Issue: [A name of more than 127 characters overwrote the compiler's data (#29)](https://github.com/fubar-coder/bb2-c/issues/29).

## Procedures

### The end of a procedure of the other kind {#end-of-procedure}

**What happened.** The original did not check that the end of a procedure
fits its kind: it accepted `End Statement` and `Statement Return` inside a
Function, and `End Function` and `Function Return` inside a Statement.

```
Function$ f{}
  NPrint "in f"
End Statement
NPrint f{}
End
```

compiled. The program printed `in f` and then crashed when `f` returned.

**With the port.** The `End` of the other kind gives "Illegal End
Procedure", the `Return` of the other kind "Illegal Procedure return", at
its line: here "Illegal End Procedure" in line 3. Two examples on the
Blitz Basic 2 CD, `DARTS_POLY.BB` and `DARTS_WIRE.BB`, end a Function with
`End Statement` and are refused for this reason.

**What to change.** Close a Function with `End Function` and leave it
with `Function Return`; close a Statement with `End Statement` and leave
it with `Statement Return`. In the two CD examples, the Function `gendart`
needs `End Function`. The bug is shown in full on its own page
([`End Statement` could close a Function]({{< relref "/docs/compared-with-original/fixed-bugs/compiler#end-of-procedure" >}})).

Issue: [End Statement could close a Function, and the other way round (#940)](https://github.com/fubar-coder/bb2-c/issues/940).

## NewTypes

### A NewType that contains itself {#newtype-contains-itself}

A NewType cannot contain a field of its own type: it would have to be
larger than itself.

**What happened.** The original accepted such a field and gave it a
random size, so the fields behind it and the size of the whole type were
random too:

```
NEWTYPE .selft
  x.selft
  y.l
End NEWTYPE
DEFTYPE .selft v
v\y=5
o.l=SizeOf .selft\y
NPrint "size ",SizeOf .selft
NPrint "y at ",o
NPrint "y ",v\y
End
```

In the recording, with the compiler's memory filled with the byte `$01`,
the original printed

```
size 262
y at 258
y 5
```

**With the port.** The field `x.selft` gives "Illegal Type" (line 2).

**What to change.** What such a source usually means is a pointer to
another item of the same type, and a pointer field is allowed:

```
NEWTYPE .node
  value.l
  *child.node
End NEWTYPE
```

Issue: [A NewType field of its own type took a random size (#973)](https://github.com/fubar-coder/bb2-c/issues/973).

### A field of a NewType defined later {#newtype-defined-later}

A NewType may have a field of a type that is defined further down, as
long as a pointer field has named that type before.

**What happened.** The original gave such a field a wrong size, and the
fields behind it and the size of the NewType stayed wrong after the type
was defined:

```
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

printed

```
y at 10
size 14
7 9
```

although `.fwd` has 8 bytes, so `y` belongs at offset 8.

**With the port.** The NewType is laid out again when the type is
defined. The program prints

```
y at 8
size 12
7 9
```

Using the size of such a NewType, or a field behind the early field,
before the type is defined, is the error "Type not yet defined":

```
NEWTYPE .pp
  *p.fwd
End NEWTYPE
NEWTYPE .dd
  x.fwd
End NEWTYPE
o.l=SizeOf .dd
NEWTYPE .fwd
  a.l
End NEWTYPE
NPrint o
End
```

The original compiled it, and it printed `4`;
the port reports "Type not yet defined" in line 7. Two types that would
contain each other are "Illegal Type":

```
NEWTYPE .pp
  *p.tb
End NEWTYPE
NEWTYPE .ta
  x.tb
End NEWTYPE
NEWTYPE .tb
  y.ta
End NEWTYPE
NPrint "compiled"
End
```

The original compiled it, and it printed `compiled`; the port
reports "Illegal Type" in line 8. A field of a type that is never defined
is "Type Not Found".

**What to change.** A program that worked with the wrong layout, for
example one that reads a file written with it, has to account for the
new offsets. Move a use of the NewType's size behind the definition of
the type, and break a cycle of two types with a pointer field.

Issue: [A NewType field of a type defined later got a wrong size (#974)](https://github.com/fubar-coder/bb2-c/issues/974).

## Include files and paths

### Text after a quoted path {#text-after-path}

**What happened.** The original took the whole rest of the line as the
path of `IncDir`, and as the file name of `Include`, `XInclude` and
`IncBin`, closing quote and all:

```
a=1
IncDir "RAM:" : a=2
NPrint a
End
```

printed `1`: the `: a=2` became part of the path, and the assignment
was never compiled.

**With the port.** A path or name in quotes ends at its closing quote.
After `IncDir` and `IncBin`, a `:` or `Else` ends the statement as usual,
so the program above prints `2`. After `Include` and `XInclude` only a
comment may follow:

```
IncDir "CORPUS:src/"
Include "inc.bb2" : a=1
NPrint a
End
```

The original failed with "Can't open Include" in line 2, since the
rest of the line was part of the file name. The port reports "Syntax
Error" in line 2. Other text after the closing quote is a "Syntax Error"
too. A path without quotes still takes the rest of the line.

**What to change.** Put a statement that follows `Include` or `XInclude`
on its own line.

Issue: [Text after an IncDir path became part of the path (#17)](https://github.com/fubar-coder/bb2-c/issues/17).

### Paths that are too long {#path-too-long}

**What happened.** The original copied paths into buffers of fixed size
without a limit:

* An `IncDir` path of 96 characters or more overwrote the compiler's
  data. The program compiled and crashed when it ran.
* The `IncDir` path and the name of an `Include`, `XInclude` or `IncBin`
  together could overwrite the compiler's data from 128 characters on.
* A `UsePath` path of 128 characters or more, possible through a macro,
  overwrote the compiler's own code, and the next `UsePath` ran the
  damaged code. The compiler did not finish.

For example,

```
IncDir "ppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppp"
x=5
NPrint x
End
```

with a path of 98 characters compiled, and the program crashed with a
"Software Failure" instead of printing `5`.

**With the port.** Each of these is the error "Path too Long" at its
line: here line 1.

**What to change.** Use a shorter path, for example with an assign.

Issues: [IncDir path of 96 characters or more (#16)](https://github.com/fubar-coder/bb2-c/issues/16),
[include file name of more than 127 characters (#325)](https://github.com/fubar-coder/bb2-c/issues/325),
[UsePath path of 128 characters or more (#31)](https://github.com/fubar-coder/bb2-c/issues/31).

## Macros

### A macro line ending in `End` {#macro-end-line}

**What happened.** In a macro, a line ending in `End` cut off the rest of
the macro and of the line that called it:

```
Macro m
  If `1
    End
  End If
  NPrint "after"
End Macro
!m{0}
NPrint "done"
End
```

The original stopped with "If Without End If" at the call of the macro,
line 7: the `End If` and the rest of the macro were lost.

**With the port.** The rest of the macro is kept, as in AmiBlitz3. The
program prints

```
after
done
```

**What to change.** Nothing, unless a source relied on the cut; then
remove the lines that the original left out.

Issue: [A macro line ending in End cut off the rest of the macro (#968)](https://github.com/fubar-coder/bb2-c/issues/968).

### A macro larger than the Macro Buffer {#macro-buffer}

The compiler options have a **Macro Buffer** size.

**What happened.** In the original the option had no effect: the buffer
for macro texts always had its default size of 8192 bytes, whatever the
option said. A macro text of 9120 bytes, in a source whose options set
the Macro Buffer to 16384 bytes, was refused with "Macro too Big" inside
the macro, and blitz2 hung afterwards.
A macro text of 1520 bytes, in a source whose options set it to 1024
bytes, was accepted: the program compiled and printed `hello`.

**With the port.** The option counts, in both directions. With a larger
Macro Buffer, a source can define macros longer than the default allows;
the first source compiles. With a smaller Macro Buffer, a macro longer
than the set size gives "Macro too Big" at the line where its text passes
the size. A macro text of more than 65535 bytes is always "Macro too Big".

**What to change.** A source saved with the default size is not affected.
If one of yours fails with "Macro too Big", set its Macro Buffer back to
the default or larger in the COMPILER OPTIONS.

Issues: [The Macro Buffer option had no effect (#336)](https://github.com/fubar-coder/bb2-c/issues/336),
[A macro text of 65536 bytes or more was cut (#964)](https://github.com/fubar-coder/bb2-c/issues/964).

## Inline assembler

### An operand that is too long {#operand-too-long}

The compiler keeps the text of an inline assembler operand when it comes
from a macro or an include, or when it contains a local label. It keeps
it in a buffer of fixed size, one for the source operand and `Dc`, one for
the destination operand right behind it.

**What happened.** The original copied the text without a limit:

* A source operand of more than 125 characters reached into the buffer of
  the destination operand, and the destination overwrote its end. The
  source expression was cut and ran on into the destination's text: wrong
  code or "Syntax Error".
* Longer texts overwrote the compiler's data behind the buffers: a source
  operand or `Dc` of more than 261 characters, a destination operand of
  more than 133.

For example,

```
Macro m
move.l #`1+`1,dat
End Macro
Goto skip
.dat
Dc.l 0
.skip
!m{1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1}
NPrint Peek.l(?dat)
End
```

has a source operand of 127 characters. The original stopped with
"Syntax Error" in line 8 instead of storing 64.

**With the port.** Each of these is the error "Operand too Long" at the
line of the instruction, here the call of the macro in line 8. Every text
that fits compiles as before, also a source operand of up to 261
characters when the destination is a register.

**What to change.** Split the expression, for example into a constant
defined before the instruction, or use shorter macro parameters.

Issue: [copyconst copies an operand text into namebuff without a length check (#997)](https://github.com/fubar-coder/bb2-c/issues/997).
