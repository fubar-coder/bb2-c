---
title: Sources that compile differently
weight: 3
description: Sources that Blitz Basic 2 accepted and the BB2 C port refuses or compiles to something else, the new runtime errors of the libraries and the forms only the port accepts, with what the original did and how to change them.
---

Before this page you should know the fixed bugs of the compiler
([Fixed original bugs]({{< relref "/docs/compared-with-original/fixed-bugs" >}})).

Almost every source compiles with the port exactly as with the original.
The exceptions on this page all come from fixed bugs: the original
accepted these sources, but compiled them to a wrong or random result, or
crashed while compiling them. Each case shows a small program, what the
original did with it, what the port does, and what to change in a source
that relies on the old behaviour. A few fixes of the libraries work the
same way when the program runs: a command that the original carried out
with a wrong argument stops with a runtime error
([Runtime errors of the libraries](#runtime-errors)).

Three cases go the other way
([Forms the original refused](#forms-the-original-refused)):

* `Frac` of a string, which the original refused when compiling,
  compiles and is a runtime error.
* `PlayWait` without a tape number compiles and waits for the playback.
* `InstallFNS` takes the number of characters of a font.

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

### A division by a constant 0 {#division-by-zero}

**What happened.** When both operands of `/` or `Mod` are constants, the
compiler computes the result while it compiles. The original did not look
at the divisor first. A Byte, Word or Float division by 0, and a Word
`Mod` by 0, crashed the compiler with "Software Failure". A Long or Quick
division by 0, also in a constant definition such as `#c=5/0`, compiled to
the largest Long number, and a Long `Mod` by 0 gave the dividend:

```
NPrint "before"
d.l=7/0
NPrint d
End
```

compiled and printed `before` and `2147483647`.

**With the port.** Each of these cases is the compile error "Division by
Zero" at its line (line 2 here), for every type. The compiler finds only
a divisor that is a constant; a division by a variable that holds 0 is
not found while compiling.

**What to change.** Remove the division, or write the value you meant
instead of the division by 0.

Issue: [A division or Mod by a constant 0 crashed the compiler (#1044)](https://github.com/fubar-coder/bb2-c/issues/1044).

### An operator too deep in an expression {#expression-too-complex}

**What happened.** The compiler works out an expression in the
processor's eight data registers, one register for each level: when a
tighter operator such as `*`, or a part in brackets, has to be computed
first, the value on its left keeps its register and the rest goes into
the next one. The arguments of a statement or function from the 7th on
already start in the 7th register. An operator there whose right operand
binds tighter, such as `a+b*c`, needs a ninth register for `b*c`, and so
does such an operator inside six levels of brackets. The original did
not check this. It computed `b*c` over `a`, and the addition took its
second value from an address register:

```
a.l=1 : b.l=5 : c.l=6
NPrint RectsHit(20,0,10,10,0,0,b*c+a,10)
NPrint RectsHit(20,0,10,10,0,0,a+b*c,10)
End
```

printed `-1` and `0`: the two rectangles overlap, but the second call got
a wrong width. With Word variables, such as `aw+bw*cw`, the compiled
program crashed with "Software Failure", and with constants, such as
`1+5*6`, the compiler itself crashed while compiling. Statements with
many arguments, such as `InitCopList 0,44,256,$13,8,32,a+b*c`, meet it
most easily. None of the programs on the Blitz Basic 2 CD contains such
an expression.

**With the port.** Such an operator is the compile error "Expression too
Complex" at its line (line 3 here). An expression that stays within the
eight registers compiles as before: a chain of operators of one
precedence (`a+b+c`), the tighter operator first (`b*c+a`), or brackets
around the right part (`a+(b*c)`).

**What to change.** Write the tighter operation first, `b*c+a`, or put
it in brackets, `a+(b*c)`. Inside many levels of brackets, compute a
part into a variable first.

Issue: [An operator whose right operand needs a ninth register took a wrong value (#1185)](https://github.com/fubar-coder/bb2-c/issues/1185).

## Characters

### An accented letter outside a string {#latin1-outside-string}

**What happened.** The original editor could not keep a Latin-1
character other than the umlauts ä ö ü Ä Ö Ü, such as ß or é. Typed, it
was lost, so the line

```
café=1:NPrint café
```

became `caf=1:NPrint caf` as you typed it, a program with another
variable name, without a message. In a text file, such a character was
taken for a keyword.

**With the port.** The editor keeps the character as a literal character
([ß, é and other accented letters were lost]({{< relref "/docs/compared-with-original/fixed-bugs/editor#latin1-in-strings" >}})).
In a string or a comment it is compiled as the character; anywhere
else, in a name or as a statement, the compiler stops with "Syntax Error" at its
line. In AB3 mode the message is "Token Not Found".

**What to change.** Write names with plain letters, and accented letters
only in strings and comments.

Issue: [Latin-1 characters in strings and comments (#1018)](https://github.com/fubar-coder/bb2-c/issues/1018).

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

### An array field of 65536 elements or more {#newtype-field-count}

A field of a NewType can be an array of a fixed number of elements,
written with the count in square brackets: `a.b[10]` is a field of ten
bytes. A NewType must stay below 32768 bytes, so a larger one is the
error "Type too Big".

**What happened.** The original kept the count in 16 bits. A count of
65536 or more, and a negative one, was taken modulo 65536: `a.b[65540]`
became a field of 4 bytes, and the NewType passed the size check. With
runtime error checking, an index behind the fourth element stopped the
program; without it, a program that used those elements wrote over the
memory behind the field:

```
NEWTYPE .t
  a.b[65540]
End NEWTYPE
NPrint SizeOf .t
End
```

The original compiled it, and it printed `4`. `a.b[-65532]` gave the
same 4 bytes.

**With the port.** Both fields are "Type too Big" (line 2).

**What to change.** Such a field never had the size it asks for. Give
the array a count that fits the NewType, or keep the data in an array
of its own with `Dim`.

Issue: [An array field's element count of 65536 or more is taken modulo 65536 without an error (#1084)](https://github.com/fubar-coder/bb2-c/issues/1084).

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

Issues:

* [IncDir path of 96 characters or more (#16)](https://github.com/fubar-coder/bb2-c/issues/16)
* [include file name of more than 127 characters (#325)](https://github.com/fubar-coder/bb2-c/issues/325)
* [UsePath path of 128 characters or more (#31)](https://github.com/fubar-coder/bb2-c/issues/31)

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
The buffer gets its new size when a source is loaded or the COMPILER
OPTIONS are closed. If there is not enough memory for it, the compiler
reports out of memory and keeps the old buffer with its size; the next
change of the options tries again.

**What to change.** A source saved with the default size is not affected.
If one of yours fails with "Macro too Big", set its Macro Buffer back to
the default or larger in the COMPILER OPTIONS.

Issues:

* [The Macro Buffer option had no effect (#336)](https://github.com/fubar-coder/bb2-c/issues/336)
* [A macro text of 65536 bytes or more was cut (#964)](https://github.com/fubar-coder/bb2-c/issues/964)

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

## Runtime errors of the libraries {#runtime-errors}

The cases above are found by the compiler. The ones in this section
compile as before (except `Frac` of a string) but stop when the program
runs. Where the original went on with a wrong argument and wrote over
memory or hung, the port's library reports a runtime error. Most of them
are error checks, so they come only in a program compiled with runtime
error checking, like the other checks of Blitz Basic 2; without it, the
program runs as with the original.

### `Frac` of a string {#frac-string}

**What happened.** `Frac` took only a float, so the original refused a
string argument when compiling:

```
f.f=1.25 : NPrint "float ",Frac(f)
a$="1.5"
NPrint "string ",Frac(a$)
End
```

The original stopped with "Can't Convert Types" in line 3.

**With the port.** `Frac` takes a value of any numeric type, as `Int`
does, and the program compiles. With runtime error checking, a string is
the error "Illegal Type", as it is for `Int` in the original and the
port: the program above prints `float .25` and stops at line 3. Without
runtime error checking, `Frac` of a string gives 0.

**What to change.** Turn the string into a number first, for example
`Frac(Val(a$))`.

Issue: [Frac of a Quick loses low fraction bits for integer parts of 256 and more (#1064)](https://github.com/fubar-coder/bb2-c/issues/1064).

### A Long or Quick division by 0 {#long-division-by-zero}

**What happened.** A Word division by 0 stopped a program compiled with
runtime error checking with "GURU! - Division by Zero". A Long or Quick
division by a variable that holds 0 went on without a message and gave
the largest number:

```
a.l=7 : b.l=0
NPrint "before"
c.l=a/b
NPrint "after ",c
End
```

Compiled with runtime error checking, the original printed `before` and
`after 2147483647`.

**With the port.** With runtime error checking, a Long or Quick division
by 0 stops with "GURU! - Division by Zero", as a Word division does; the
program above stops at line 3. Without runtime error checking, the result
is the largest number with the sign of the dividend, here 2147483647, and
the program goes on.

**What to change.** Test the divisor before the division where it can
be 0.

Issue: [Division by 0 returned $7FFFFFFF regardless of the sign (#200)](https://github.com/fubar-coder/bb2-c/issues/200).

### `WindowTags` with too many tag pairs {#windowtags-too-many-tags}

**What happened.** `WindowTags` copies the tag and data pairs given as
arguments into a buffer of the library without a limit. With 16 pairs or
more it wrote behind the buffer, over data and code of the library:

```
DEFTYPE .l
WbToScreen 0
t=$80000064
WindowTags 0,$000E,"Tags",t,10,t,10,t,10,t,10,t,10,t,10,t,10,t,10,t,10,t,10,t,10,t,10,t,10,t,10,t,10,t,10
DefaultOutput
NPrint "opened"
CloseWindow 0
End
```

The 16 pairs all set the left edge of the window. Compiled with runtime
error checking, the original printed `opened`.

**With the port.** With runtime error checking, more than 15 pairs are
the error "Too Many Tags"; the program above stops at `WindowTags`.

**What to change.** Give a long list of tags as the address of a tag
list, `WindowTags Window#,Flags,Title$,&TagList`: that form takes any
number of tags.

Issue: [WindowTags with tag pairs overflowed its tag buffer (#961)](https://github.com/fubar-coder/bb2-c/issues/961).

### `ScreenTags` with too many tag pairs {#screentags-too-many-tags}

**What happened.** `ScreenTags Screen#,Title$,Tag,Data...` copies its tag
and data pairs into a buffer of the library in the same way as
`WindowTags`, without a limit. With 16 pairs or more it wrote behind the
buffer, over code of the libraries.

**With the port.** With runtime error checking, more than 15 pairs are
the error "Too Many Tags".

**What to change.** Give a long list of tags as the address of a tag
list, `ScreenTags Screen#,Title$,&TagList`: that form takes any number of
tags.

Issue: [ScreenTags with tag pairs had no limit for the user tags (#1039)](https://github.com/fubar-coder/bb2-c/issues/1039).

### `StringGadget` with a `Maxlen` of 0 {#stringgadget-maxlen}

**What happened.** `StringGadget` with a `Maxlen` of 0 cleared 65536
bytes of memory behind the gadget's empty text buffer.

**With the port.** With runtime error checking, this is the error
"Maxlen MUST be greater than zero".

**What to change.** Give a `Maxlen` of 1 or more.

Issue: [StringGadget with Maxlen 0 cleared 64 KB (#165)](https://github.com/fubar-coder/bb2-c/issues/165).

### `Inkey$` with a negative count {#inkey-negative}

**What happened.** The original took the count of `Inkey$` as an
unsigned number: `Inkey$(-1)` tried about 4 billion times to read a
character, added every character it got behind the end of the string
buffer, and did not come back.

**With the port.** With runtime error checking, a negative count is the
error "Can't Inkey$ a negative number of Characters!". `Inkey$(0)` is
the empty string, as before.

**What to change.** Use a count of 0 or more.

Issue: [Inkey$ with a negative count made about 4 billion attempts (#1031)](https://github.com/fubar-coder/bb2-c/issues/1031).

### `Peeks$` with a negative length {#peeks-negative}

**What happened.** The original took the length of `Peeks$` as an
unsigned number: `Peeks$(Address,-1)` copied about 4 GB into the string
buffer, over the memory behind it, and the program crashed.

**With the port.** A negative length gives the empty string, as a
length of 0 does. With runtime error checking, it is the error "Can't
Peeks$ a negative number of Characters!".

**What to change.** Use a length of 0 or more.

Issue: [Peeks$ with a negative length copied up to 4 GB (#214)](https://github.com/fubar-coder/bb2-c/issues/214).

### `Cvi`, `Cvl` and `Cvq` of a short string {#cvi-short-string}

**What happened.** `Cvi` always read 2 bytes of its string, `Cvl` and
`Cvq` 4, whatever the length of the string: a shorter string gave a
number made partly of the bytes behind it.

**With the port.** The missing bytes count as 0. With runtime error
checking, a string that is too short is the error "Cvi needs a String of
at least 2 Characters" ("Cvl needs ...", "Cvq needs a String of at least
4 Characters").

**What to change.** Give `Cvi` a string of 2 characters, `Cvl` and `Cvq`
one of 4, as `Mki$`, `Mkl$` and `Mkq$` make them.

Issue: [Cvi, Cvl and Cvq read past the end of a short string (#288)](https://github.com/fubar-coder/bb2-c/issues/288).

### `Instr` with a start position below 1 {#instr-startpos}

**What happened.** `Instr(String$,Findstring$,Startpos)` did not check
the start position: with 0 or less it compared the bytes before the
string, and `Instr(a$,"",0)` gave 0, `Instr(a$,"",-5)` -5.

**With the port.** A start position below 1 searches from the first
character. With runtime error checking, it is the error "Instr Startpos
MUST be greater than zero".

**What to change.** Start at position 1 or later; `Instr(a$,f$)`
searches from the first character.

Issue: [Instr compared the bytes before the string for a start position of 0 or less (#287)](https://github.com/fubar-coder/bb2-c/issues/287).

### A `Dim` of 4 GB or more {#dim-too-large}

**What happened.** Runtime error checking checks each dimension of a
`Dim`, but not the size of the whole array. An array of 4 GB or more,
such as `Dim a.l(32767,32767)`, got a wrong, tiny size, and the `Dim`
succeeded.

**With the port.** Such a `Dim` fails with "Out of Memory!". With
runtime error checking, it is the error "Array Size out of Range", at
the dimension that makes the array too large.

**What to change.** Make the array smaller; no Amiga has the memory for
it.

Issue: [A Dim of 4 GB or more got an array of a few bytes (#58)](https://github.com/fubar-coder/bb2-c/issues/58).

### A `Dim` with the highest index 65535 {#dim-65535}

**What happened.** A dimension of an array or list has at most 65535
elements, the indexes 0 to 65534. `Dim a(65535)` and `Dim List a(65535)`
gave an array or list of no elements, so every access to it lay outside
its memory. Runtime error checking refused only an index of 65536 and
more.

**With the port.** With runtime error checking, the highest index 65535
is the error "Array Dimension out of Range" too.

**What to change.** Make the highest index 65534 or less, or spread the
elements over two dimensions.

The manual states no limit, but the original has one, and the port keeps
it: `Dim` counts a dimension's elements in a word. Without runtime error
checking a larger `Dim` is not refused but silently too small
(`Dim a.l(70000)` gets 17860 bytes, 4465 elements, in the original and in
the port), and an index of 65536 or more reaches outside the array or
wraps into it. Such a size or index is a programming error; turn runtime
error checking on to find it ("Array Dimension out of Range", "Array
Subscript out of Range").

Issue: [Dim a(65535) gave an array of no elements (#1037)](https://github.com/fubar-coder/bb2-c/issues/1037).

### `WSize` beyond the size limits of a window {#wsize-limits}

**What happened.** `WSize` waits until the window has the new size.
Intuition keeps a window with a sizing gadget inside its size limits
(`SizeLimits`), so a `WSize` beyond them never got that size, and the
original waited forever. Runtime error checking tested only the edge of
the screen.

**With the port.** `WSize` and `WMove` stop waiting once Intuition has
changed the window. With runtime error checking, a size outside the
limits of a window with a sizing gadget is the error "Can't Size
Window".

**What to change.** Size the window within the limits given with
`SizeLimits` before it was opened.

Issue: [WMove and WSize waited forever when Intuition limited the values (#320)](https://github.com/fubar-coder/bb2-c/issues/320).

### A bitmap or shape of depth 0 {#depth-zero}

**What happened.** `BitMap` refuses a depth of 0 with runtime error
checking, but `CludgeBitMap` and `InitShape` had no check. A depth of 0
made a bitmap or shape that the drawing and blitting commands treated as
one of 65536 planes, so they ran over the memory behind it; `CludgeBitMap`
itself already wrote 65536 plane pointers behind the bitmap object.

```
DEFTYPE .l
NPrint "before"
InitShape 0,16,8,0
NPrint "after, depth ",Peek.w(Addr Shape(0)+4)
End
```

Compiled with runtime error checking, the original printed

```
before
after, depth 0
```

**With the port.** With runtime error checking, `CludgeBitMap` with a
depth of 0 or more than 8 is the error "Illegal BitMap Depth", and
`InitShape` with a depth of 0 the error "Illegal Shape Depth". The
program above stops after `before`. The check of `InitShape` also keeps
such a shape out of `ShapeGadget`.

**What to change.** Give a depth from 1 to 8.

Issues:

* [Bitmaps of depth 0 made the plane loops run 65536 times (#393)](https://github.com/fubar-coder/bb2-c/issues/393)
* [BBlit with depth 0 (#74)](https://github.com/fubar-coder/bb2-c/issues/74)
* [QBlit with a shape of depth 0 (#243)](https://github.com/fubar-coder/bb2-c/issues/243)
* [A shape gadget of depth 0 (#167)](https://github.com/fubar-coder/bb2-c/issues/167)

### `BitMap` with a wrong depth {#bitmap-depth-text}

**What happened.** With runtime error checking, `BitMap` with a depth of
0 or more than 8 stopped with the error text "No currently used object!",
which has nothing to do with the depth; in an executable the text was
empty.

**With the port.** The error text is "Illegal BitMap Depth", as for
`CludgeBitMap`.

**What to change.** Give a depth from 1 to 8.

Issue: [BitMap with a wrong depth reported "No currently used object!" (#1040)](https://github.com/fubar-coder/bb2-c/issues/1040).

### A bitmap of width or height 0 {#bitmap-size}

**What happened.** `BitMap` and `CludgeBitMap` accepted a width or
height of 0, also with runtime error checking. `CopyBitMap` of such a
bitmap copied about four thousand million words through the memory.

```
BitMap 0,16,10,1
NPrint "16 x 10"
BitMap 1,0,10,1
NPrint "0 x 10"
End
```

Compiled with runtime error checking, the original printed both lines.

**With the port.** With runtime error checking, a width or height below 1
is the error "Illegal BitMap Size"; the program above stops after
`16 x 10`.

**What to change.** Give a width and a height of 1 or more.

Issue: [CopyBitMap of a bitmap of size 0 copied 4 billion words (#78)](https://github.com/fubar-coder/bb2-c/issues/78).

### `Poly` with fewer than 2 points {#poly-points}

**What happened.** `Poly` draws a line from each point to the next. With
1 point or none it counted down past 0 and drew 65535 or 65536 lines from the
words in memory behind the coordinates.

**With the port.** With runtime error checking, this is the error "Poly
needs at least 2 Points".

**What to change.** Give `Poly` at least 2 points; draw a single point
with `Plot`.

Issue: [Poly with 1 point drew 65535 lines (#396)](https://github.com/fubar-coder/bb2-c/issues/396).

### A polygon of more than 16 points {#too-many-points}

**What happened.** `Polyf`, `BitMapPoly`, `ChunkyPoly` and `RenderPoly`
take at most 16 points. More points overwrote the code of the library,
and the program crashed, also with runtime error checking.

**With the port.** With runtime error checking, more than 16 points are
the error "Too Many Points"; for the polygon commands that take a
negative count for the points in the other order, so is a count below
-16.

**What to change.** Split the polygon into polygons of at most 16 points.

Issue: [Polyf, BitMapPoly and ChunkyPoly overflowed with more than 16 points (#395)](https://github.com/fubar-coder/bb2-c/issues/395).

### `ChunkyVlin` with the ends the wrong way round {#chunkyvlin-ends}

**What happened.** `ChunkyVlin x,y1,y2,colour` without `CLIP` draws from
`y1` down to `y2`. With `y1` greater than `y2` it wrote about 65536 words,
one in each chunky line, far beyond the chunky area.

**With the port.** With runtime error checking, this is the error
"ChunkyVlin y1 MUST NOT be greater than y2". The form with `CLIP` takes
the ends in either order.

**What to change.** Give the upper end first, or use the form with
`CLIP`.

Issue: [ChunkyVlin with y2 below y1 wrote about 65536 words (#417)](https://github.com/fubar-coder/bb2-c/issues/417).

### `RenderPoly` with a colour above 14 on a chunky area {#renderpoly-colour}

**What happened.** After `RenderToChunky`, the colour of `RenderPoly` is
a number from 0 to 14 for a fixed set of colours. A colour above 14 was
taken from the memory behind that set.

**With the port.** With runtime error checking, this is the error
"Illegal Colour".

**What to change.** Give a colour from 0 to 14.

Issue: [RenderPoly after RenderToChunky read colours above 14 behind its table (#422)](https://github.com/fubar-coder/bb2-c/issues/422).

### `LoadShape` of a picture with very large planes {#loadshape-large-plane}

**What happened.** A shape keeps two sizes in 16 bits each, in memory
and in the files of `SaveShapes`: the size of one plane, and the size of
one plane with an extra word per row. `BBlit` saves that second amount
of the background. `LoadShape` of an IFF picture with a plane of 64 KB or more,
such as 1024 by 520 pixels, took only the lower 16 bits of that size,
made the shape far too small and wrote the whole picture into it, over
the memory behind it. A picture whose plane is just under 64 KB, such as
992 by 528 pixels, loaded, but `BBlit` then saved and restored the wrong
amount of the background.

**With the port.** Such a picture is the runtime error "Can't Read/Write
Shapes File", also without runtime error checking, as for a file that
cannot be read. The shape stays empty. The limit is that of `InitShape`
and `GetaShape` ([Shapes too large for their size fields](#shape-too-big)):
the size of one plane with its extra word per row,
(⌈width / 16⌉ × 2 + 2) × height bytes, must stay below 65536 (64 KB).
A picture of 992 by 520 pixels loads.

**What to change.** Load the picture as a bitmap, or cut it into shapes
within that limit.

Issues:

* [Large shapes had their line and plane steps and sizes only as a word (#263)](https://github.com/fubar-coder/bb2-c/issues/263)
* [LoadShape and the shape creation checks use different 64 KB plane limits (#1055)](https://github.com/fubar-coder/bb2-c/issues/1055)

### `LoadShape` of a picture too large to blit {#loadshape-blit-size}

**What happened.** A shape keeps the size the blitter draws it with in
one word: 10 bits for the height and 6 for the width in 16 pixel words.
`LoadShape` of a picture taller than 1023 rows or wider than 992 pixels
loaded it all the same, and the size wrapped: a picture 16 by 1100
pixels was blitted 76 rows high.

**With the port.** Such a picture is the runtime error "Can't
Read/Write Shapes File", also without runtime error checking, and the
shape stays empty. This change can stop a program that worked with the
original, such as one that loaded a large picture as a shape only to
read `ShapeWidth` and `ShapeHeight` and never blitted it.

**What to change.** Load the picture as a bitmap, or cut it into shapes
of at most 1023 rows and 992 pixels.

Issue: [The blit size word of a shape taller than 1023 rows overflows (#1046)](https://github.com/fubar-coder/bb2-c/issues/1046).

### `LoadShape` of a picture of 0 bitplanes {#loadshape-no-planes}

**What happened.** `LoadShape` took the number of bitplanes from the
header of the picture without a check. A header that gives 0 bitplanes,
in a damaged or unusual file, made a shape without memory, and the
picture data was written behind it until the file ended.

**With the port.** Such a picture is the runtime error "Illegal Shape
Depth", also without runtime error checking, and the shape stays empty.

**What to change.** Nothing for a correct picture; a picture of 0
bitplanes holds no image.

Issue: [LoadShape of an ILBM with 0 planes writes the file behind an empty block (#1052)](https://github.com/fubar-coder/bb2-c/issues/1052).

### `LoadShape` of a picture of width or height 0 {#loadshape-size-zero}

**What happened.** `LoadShape` took the width and the height from the
header of the picture without a check. A header that gives a width or a
height of 0 made a shape without memory, and the loops over the bytes or
the rows of the picture ran 65536 times, writing the picture data behind
the shape until the file ended.

**With the port.** Such a picture is the runtime error "Illegal Shape
Size", also without runtime error checking, and the shape stays empty.

**What to change.** Nothing for a correct picture; a picture of width or
height 0 holds no image.

Issue: [LoadShape of an ILBM with width 0 or height 0 runs its loops 65536 times (#1054)](https://github.com/fubar-coder/bb2-c/issues/1054).

### Shapes too large for their size fields {#shape-too-big}

**What happened.** A shape keeps the size of a plane, with an extra word
per row, in 16 bits, and the size the blitter draws it with in one word.
`InitShape`, `GetaShape`, `Rotate` and `Scale` made shapes without
checking either: a shape with a plane of 64 KB or more got wrong sizes,
and its memory was overwritten; a shape taller than 1023 rows or wider
than 992 pixels was blitted wrongly. `InitShape` also made a shape of
width or height 0, whose blits ran their loops 65536 times.

**With the port.** With runtime error checking, `InitShape`,
`GetaShape`, `Rotate` and `Scale` stop with "Shape too Big" when the new
shape breaks one of these limits:

* the size of one plane with its extra word per row,
  (⌈width / 16⌉ × 2 + 2) × height bytes, must stay below 65536 (64 KB);
* the height must be at most 1023 rows;
* the width must be at most 992 pixels.

`InitShape` with a width or height of
0 is the error "Illegal Shape Size". Without runtime error checking, the
commands are as before.

**What to change.** Keep shapes within these sizes; draw a larger image
on a bitmap.

Issues:

* [InitShape, GetaShape, Rotate and Scale do not check for planes of 64 KB or more (#1047)](https://github.com/fubar-coder/bb2-c/issues/1047)
* [The blit size word of a shape taller than 1023 rows overflows (#1046)](https://github.com/fubar-coder/bb2-c/issues/1046)

### `CreateShape` with a count of 0 or past the last shape {#createshape-count}

`CreateShape start_shape,count,chipmemblock,like_shape` of
[bb_blitlib]({{< relref "/docs/libraries/userlibs/bb_blitlib" >}}) makes
`count` shapes from `start_shape` on.

**What happened.** The original did not check `count`, also not with
runtime error checking. A count of 0 made 65536 shapes and wrote 2 MB
over the memory behind the shape table, which could crash the machine. A
count that ran past the last shape the Object Maximums of the compiler
options allow wrote the shapes beyond it over the memory behind the
table: with the maximum of 100 shapes, `CreateShape 99,2,m,0` wrote its
second shape there.

**With the port.** With runtime error checking, a count of 0 is the
error "CreateShape count must be at least 1!", and shapes past the last
one the error "CreateShape start+count above Maximum Shapes!". Nothing is
written then. Without runtime error checking, the command is as before.

**What to change.** Give a count of at least 1, and raise the maximum of
shapes in the compiler options if the shapes do not fit.

Issues:

* [CreateShape with count 0 creates 65536 shape objects (#428)](https://github.com/fubar-coder/bb2-c/issues/428)
* [CreateShape writes past the shape table when start_shape + count exceeds Maximum Shapes (#1077)](https://github.com/fubar-coder/bb2-c/issues/1077)

### A shape that was never defined for the Blitz Bombers blits {#bb-blitlib-shape-undefined}

**What happened.** The commands of
[bb_blitlib]({{< relref "/docs/libraries/userlibs/bb_blitlib" >}}) that
blit a shape, `SpecialBlock`, `SpecialBlockBitmap`, `TBlock`,
`PredatorBlit`, `BPredatorBlit`, `BColourBlit` and `SpecialBlockToShape`,
did not check the shape, also not with runtime error checking. A shape
that was never defined has the depth 0, which these commands took as
65536 planes: they blitted over memory all over the system, through
whatever they took for further plane pointers behind the bitmap.

**With the port.** With runtime error checking, such a shape is the
error "Uninitialized Shape", and nothing is blitted. Without runtime
error checking, the commands are as before.

**What to change.** Define the shape before you blit it, for example
with `GetaShape` or `LoadShape`.

Issue: [A shape or bitmap of depth 0 makes the plane loops run 65536 times (#429)](https://github.com/fubar-coder/bb2-c/issues/429).

### `BlockToShape` with a shape too large for one blit {#blocktoshape-too-large}

**What happened.** `BlockToShape` of
[bb_blitlib]({{< relref "/docs/libraries/userlibs/bb_blitlib" >}}) copies
a shape with one blit of as many lines as the shape's height times its
depth. The blitter takes at most 1024 lines and 1024 pixels in one blit.
With a larger shape the size wrapped, and the copy was incomplete without
a message: of a shape 16 by 256 pixels in five planes, 1280 lines, only
256 lines were copied.

**With the port.** With runtime error checking, a shape whose height
times its depth is more than 1024, or one wider than 1024 pixels, is the
error "Shape too large for BlockToShape!". Without runtime error
checking, the copy of such a shape is incomplete, as before.

**What to change.** Keep the shapes you copy with `BlockToShape` within
these limits.

Issue: [BlockToShape copies one word per line: shapes wider than 16 pixels are copied only in part (#425)](https://github.com/fubar-coder/bb2-c/issues/425).

### A palette of 0 colours for `SaveBitmap` and `SaveShape` {#save-palette-no-colours}

**What happened.** `InitPalette p,0` makes a palette without colours.
`SaveBitmap` and `SaveShape` with such a palette wrote 65536 colours,
192 KB, into a buffer of 4 KB and over the memory behind it, also with
runtime error checking.

**With the port.** With runtime error checking, a palette without
colours is the error "Uninitialized Palette", as a palette that was never
made. Without runtime error checking, the commands are as before.

**What to change.** Give the palette at least one colour, or save the
picture without a palette.

Issue: [A palette with 0 colours wrote 65536 colours (#182)](https://github.com/fubar-coder/bb2-c/issues/182).

### `SaveShape` of a shape of depth 0 or more than 14 {#saveshape-depth}

**What happened.** `SaveShape` has room for the addresses of 14 planes.
A shape of more planes, or of depth 0, made it write plane addresses over
the memory of the library, and with depth 0 over 256 KB of it, so the
program crashed or saved a broken file. `LoadShape` takes the depth of a
picture as it is, so a shape loaded from a 24-bit picture has 24 planes.

**With the port.** `SaveShape` of a shape of depth 0 or of more than 14
planes is the runtime error "Illegal Shape Depth", also without runtime
error checking. Shapes of 1 to 14 planes are saved as before.

**What to change.** Save such a picture another way, or reduce it to at
most 14 planes before it is loaded as a shape.

Issue: [SaveShape with depth 0 or 15 and more overwrote its buffer (#326)](https://github.com/fubar-coder/bb2-c/issues/326).

### `LoadBitMap` and `LoadScreen` of a missing file {#loadbitmap-missing-file}

**What happened.** `LoadBitMap` and `LoadScreen` without a palette did
not notice that the file could not be opened, or that it had no picture
header (BMHD). They looked for the picture data in what an earlier IFF
load had left in the read buffer, and showed the old picture again,
garbage, or stopped with "Can't Find IFF 'BODY'".

**With the port.** A file that cannot be opened is the runtime error
"Can't Open File", a file without a picture header the error "Can't Find
IFF 'BMHD'", also without runtime error checking. With a palette, the
colours are read first, and a file that cannot be opened is the error
"Can't Open IFF", as before.

**What to change.** Where the file may be missing, test with `Exists`
that it is there before it is loaded.

Issue: [LoadBitMap ignored a file it could not open (#187)](https://github.com/fubar-coder/bb2-c/issues/187).

### `GetaSprite` with a shape too wide for the sprite channels {#getasprite-width}

**What happened.** With runtime error checking, `GetaSprite` refuses a
shape too wide for the 8 sprite channels with "Sprite too Wide". The
limits of the original were the wrong way round: it allowed 4 sprite
widths for a 4 colour shape and 8 for a 16 colour one, which takes two
channels per sprite. So in `SpriteMode 0` a 4 colour shape of 65 to 128
pixels was refused, although it fits, and a 16 colour one of that width
was accepted and failed at every `ShowSprite` with "Sprite Channel
Unavailable".

**With the port.** A 4 colour shape may be 8 sprite widths wide, a 16
colour shape 4: in `SpriteMode 0`, 128 and 64 pixels, in `SpriteMode 1`
256 and 128, in `SpriteMode 2` 512 and 256. A 16 colour shape of 65 to
128 pixels in `SpriteMode 0` is the error "Sprite too Wide"; a 4
colour one of that width is accepted.

**What to change.** Cut a wider 16 colour shape into several sprites, or
use a wider `SpriteMode`.

Issue: ["Sprite too Wide" had the limits of 4 and 16 colour shapes the wrong way round (#988)](https://github.com/fubar-coder/bb2-c/issues/988).

### `SpriteMode` above 2 {#spritemode-range}

**What happened.** `SpriteMode` takes 0, 1 or 2 for sprites of 16, 32 or
64 pixels. A higher mode took the width from the memory behind the
library's table of widths, and the sprites made after it had a wrong
size.

**With the port.** With runtime error checking, a mode above 2 is the
error "Illegal SpriteMode".

**What to change.** Give a mode from 0 to 2.

Issue: [SpriteMode above 2 read past the width table (#281)](https://github.com/fubar-coder/bb2-c/issues/281).

### `SColl` with two channels of the same pair {#scoll-same-pair}

**What happened.** The sprite channels are used in pairs, 0 and 1, 2 and
3, 4 and 5, 6 and 7, and the hardware reports collisions only between
sprites of different pairs. `SColl(0,1)` tested the collision of other
channels and returned its result.

**With the port.** With runtime error checking, two channels of the same
pair are the error "Sprite channels must be of different pairs". Two
equal channels are the error "Sprite channels must be different", as in
the original.

**What to change.** Test the collision of two sprites on channels of
different pairs.

Issue: [SColl with channel 0/1 as the second argument or channels of one pair tested the wrong bit (#110)](https://github.com/fubar-coder/bb2-c/issues/110).

### `DisplaySprite` across the two sprite banks {#displaysprite-two-banks}

A copper list of the display library has its own sprite channels, 8 for
a list made with 8 sprites. `CustomSprites` adds a second bank of
channels, 8 to 15, for the lines from its position down. A sprite wider
than one channel takes several channels after the one given to
`DisplaySprite`.

**What happened.** A sprite that starts on the last channels of the
first bank and needs channels of the second wrote its other channels
over the instructions behind the list's sprites, which set the scroll
position, the priority of sprites and playfields and the colour banks:

```
InitCopList 0,44,256,$10004,8,32,80
CustomSprites 0,0,14,8
DisplaySprite 0,0,50,20,7
```

Sprite 0 is two channels wide, such as a sprite made with `GetaSprite`
from a 4 colour shape of 32 pixels in `SpriteMode 0`. The original put its second channel
over these instructions, with no error.

**With the port.** The channels of a sprite must all lie in the bank of
its first channel, since the two banks show their sprites in different
parts of the display. Otherwise it is the runtime error "Not Enough
Sprite Channels", also without runtime error checking, as for a sprite
that needs more channels than a list without `CustomSprites` has.

**What to change.** Show the sprite on channels that lie in one bank:
here on channel 6 or lower, or from channel 8 on, below the position of
`CustomSprites`.

Issue: [DisplaySprite of a sprite across the two sprite banks overwrote the copper list (#414)](https://github.com/fubar-coder/bb2-c/issues/414).

### The checks of `FadeIn` and `FadeOut` {#fade-checks}

**What happened.** With runtime error checking, `FadeIn` and `FadeOut`
check the used slice, the palette and the range of colours. The original
took a palette whose colours lie in the lowest 64 KB of memory as one
that was never made, and stopped the program with an error. Two errors
had the wrong message: a palette that was never made gave "Not enough
colours in Slice", and a colour of the range beyond the colours of the
slice gave "Uninitialized Slice".

**With the port.** A palette is accepted wherever its colours lie. A
palette that was never made is the error "Uninitialized Palette", and a
colour beyond the colours of the slice the error "Not enough colours in
Slice". A slice that is not set up is still "Uninitialized Slice".

**What to change.** Nothing; a program that stopped with one of these
errors now stops with the message that names its cause.

Issues:

* [The checks of the fades tested only the upper word of the palette (#135)](https://github.com/fubar-coder/bb2-c/issues/135)
* [An uninitialised palette reported "Not enough colours in Slice" (#330)](https://github.com/fubar-coder/bb2-c/issues/330)
* [A too large colour range reported "Uninitialized Slice" (#331)](https://github.com/fubar-coder/bb2-c/issues/331)

### The check of `SetPeriod` {#setperiod-check}

**What happened.** With runtime error checking, `SetPeriod Sound#,Period`
tested the period as if it were a mask of audio channels, which runs
from 0 to 15. Every period above 15, and so every one a sound is played
with, stopped the program with "Illegal Chanel Mask":

```
InitSound 0,32
NPrint "before"
SetPeriod 0,428
NPrint "after"
End
```

The original printed `before` and stopped with the error.

**With the port.** The check tests only that the sound is set up, and a
sound that is not is still "Uninitialized Sound". The program above
prints `before` and `after`. Without runtime error checking nothing
changes.

**What to change.** Nothing.

Issue: [SetPeriod: error check tests the period as a channel mask (#66)](https://github.com/fubar-coder/bb2-c/issues/66).

### `JumpMed` without a playing module {#jumpmed-no-module}

**What happened.** `JumpMed` makes the MED module that plays go on at
another position of its song. Without a playing module, before the first
`StartMedModule`, after `StopMed` or after the module stopped itself,
the original wrote the position into low memory, into an exception
vector of the processor:

```
NPrint "before"
JumpMed 3
NPrint "after"
End
```

The original printed `before` and `after`, and the exception vector
held the position from then on.

**With the port.** With runtime error checking, the program prints
`before` and stops with the error "No Module Playing". Without runtime
error checking nothing changes.

**What to change.** Call `JumpMed` only while a module plays, after
`StartMedModule`.

Issue: [JumpMed without a playing module writes to address 44-47 (#210)](https://github.com/fubar-coder/bb2-c/issues/210).

### `FreeTrackerModule` of the module that plays {#freetrackermodule-playing}

**What happened.** `FreeTrackerModule` freed a module that the CIA
Protracker library was still playing. The module did not stop: the
library went on reading its notes and samples from the freed memory,
which other parts of the program or the system could already be using,
also with runtime error checking.

```
DEFTYPE .l
r=LoadTrackerModule(0,"mod.tune")
If r<>-1 Then End
r=StartTracker(0)
FreeTrackerModule 0
NPrint "freed"
End
```

The original printed `freed`, and the music went on from the freed
memory.

**With the port.** With runtime error checking, `FreeTrackerModule` of
the module that plays is the runtime error "Tracker: Stop the module
before freeing it.", before anything is freed. A module stopped with
`StopTracker` can be freed. Without runtime error checking, the module
is freed as before.

**What to change.** Call `StopTracker` before `FreeTrackerModule`.

Issue: [FreeTrackerModule frees a module that is still playing (#436)](https://github.com/fubar-coder/bb2-c/issues/436).

### `PopInput` and `PopOutput` with nothing to pop {#pop-without-push}

**What happened.** With runtime error checking, the first `PopInput` or
`PopOutput` with no switch of the channel left to undo, such as one at
the start of the program, passed the check and left the program without
an input or output channel. The error "No Input channel to pop!" or "No
Output channel to pop!" came only at the next pop.

**With the port.** The error comes at the first such pop.

**What to change.** Pop only to undo a switch such as `FileInput` or
`WindowOutput`.

Issue: [The check of PopInput and PopOutput came one pop late (#1032)](https://github.com/fubar-coder/bb2-c/issues/1032).

### `CreateMsgPort` without a free signal {#createmsgport-signal}

**What happened.** A task has a limited number of signals. When none was
free, the original's `CreateMsgPort` did not notice and made a port whose
messages never wake the program.

**With the port.** This is the runtime error "Failed to allocate signal
for Port", also without runtime error checking, like the error "Failed to
allocate memory for Port" of the same command.

**What to change.** Free the signals or ports the program no longer
needs before it makes a new port.

Issue: [CreateMsgPort did not notice a failed signal allocation (#298)](https://github.com/fubar-coder/bb2-c/issues/298).

### `DeleteMsgPort` of a port `CreateMsgPort` did not make {#deletemsgport-foreign}

**What happened.** `DeleteMsgPort` took any address as a port. A port
that `CreateMsgPort` had not made, such as one the program had set up
itself, was taken apart and freed as if it had been, which corrupts
memory. With runtime error checking, only a memory error came, after the
damage.

**With the port.** With runtime error checking, such a port is the error
"Port Not Created By CreateMsgPort", before anything is changed.

**What to change.** Give `DeleteMsgPort` only ports that `CreateMsgPort`
returned, and remove other ports the way they were made.

Issue: [DeleteMsgPort took any address as a port (#1041)](https://github.com/fubar-coder/bb2-c/issues/1041).

### `GetRexxResult` and `GetRexxCommand` with a number out of range {#rexx-numbers}

`GetRexxResult(msg,ResultNum)` reads result 1 or 2 of an ARexx message,
and `GetRexxCommand(msg,ARGNum)` the text of one of its 16 argument
slots, counted from 1.

**What happened.** The original did not check the number and read
whatever lay at that place in the message: `GetRexxResult(msg,0)` gave
the action of the message, `GetRexxResult(msg,3)` the first argument
slot, and `GetRexxCommand(msg,0)` took the second result as the address
of a text.

**With the port.** With runtime error checking, `GetRexxResult` with a
number other than 1 or 2 is the error "Illegal Result Number", and
`GetRexxCommand` with a number outside 1 to 16 the error "Illegal
Argument Number". Without runtime error checking, the program runs as
with the original.

**What to change.** Read the argument slots with `GetRexxCommand`, and
other fields of the message with `Peek`.

Issue: [GetRexxResult and GetRexxCommand did not check their number (#51)](https://github.com/fubar-coder/bb2-c/issues/51).

### `SpotBox` on a page that is not set up {#spotbox-page}

**What happened.** The other Blitz OS commands stop with "UnInitialised
Page" on a page that was not made with `InitPage`, was freed or has no
boxes. `SpotBox` did not check its page: on such a page it searched
65536 boxes in the memory behind the page and could return one it found
there.

**With the port.** With runtime error checking, this is the error
"UnInitialised Page".

**What to change.** Make the page with `InitPage`, with at least one box,
before `SpotBox`.

Issue: [SpotBox did not check its page (#83)](https://github.com/fubar-coder/bb2-c/issues/83).

### `SelectBox` with a box the page does not have {#selectbox-range}

**What happened.** The other Blitz OS commands stop with "Illegal Box#
For Page" on a box number beyond the page's number of boxes.
`SelectBox Box#` did not check the number: it selected and drew
whatever lay behind the page's boxes.

**With the port.** With runtime error checking, this is the error
"Illegal Box# For Page". A negative `Box#` still selects no box.

**What to change.** Select a box from 0 to one less than the number of
boxes given to `InitPage`.

Issue: [SelectBox did not check the box number (#82)](https://github.com/fubar-coder/bb2-c/issues/82).

### `InitPage` with 32768 boxes or more {#initpage-numboxes}

**What happened.** `InitPage Page#,BitMap#,NumBoxes` took a `NumBoxes`
of 32768 or more as a negative number, so the size of the box table came
out wrong and far too large, and the program stopped with "Out of
Memory!", a message that does not name the cause.

**With the port.** With runtime error checking, this is the error
"Illegal Number of Boxes". Without runtime error checking, the command
is as before.

**What to change.** Give a page fewer than 32768 boxes.

Issue: [InitPage with NumBoxes of 32768 or more gives a negative table size (#1043)](https://github.com/fubar-coder/bb2-c/issues/1043).

### `EZRequest` and `EZRequestArgs` with too many arguments {#ezrequest-too-many-arguments}

`EZRequest` of
[rimisclib]({{< relref "/docs/libraries/userlibs/rimisclib" >}}) shows a
system requester whose texts can hold placeholders, filled in by the
arguments after `gadget$`; `EZRequestArgs` shows the values of its
arguments. Both keep these arguments in a table of 16 longs: a string, a
long, a quick or a float takes one long, a word or a byte half of one.

**What happened.** The original did not limit the number of arguments,
also not with runtime error checking. More than fit into the table were
written over the code of the library behind it, the code of
`WorkbenchHere`. With the original, this call with 18 long arguments
showed its requester, and a `WorkbenchHere` afterwards ran into the
overwritten code and could crash the program:

```
EZRequest "BO","x","OK",q,q,q,q,q,q,q,q,q,q,q,q,q,q,q,q,q,q
```

**With the port.** With runtime error checking, arguments that do not
fit into the 16 longs are the error "Too Many EZRequest Arguments", and
no requester appears. Without runtime error checking, the commands are
as before.

**What to change.** Give `EZRequest` and `EZRequestArgs` at most 16
longs of arguments; show more values with a second requester.

Issue: [EZRequest/EZRequestArgs do not limit the number of arguments (16 longs) (#748)](https://github.com/fubar-coder/bb2-c/issues/748).

### A notification slot outside 0 to 63 {#notify-slot-range}

**What happened.** `AddNotify` and `RemNotify` of
[ridoslib]({{< relref "/docs/libraries/userlibs/ridoslib" >}}) keep each
notification in a slot of a table of 64 entries, numbered 0 to 63. The
original did not check the slot number, also not with runtime error
checking: with a number outside, `AddNotify` wrote its request over the
library's data around the table, and `RemNotify` took that data for a
request, ended it and freed it.

**With the port.** With runtime error checking, a slot outside 0 to 63
is the error "Notify number must be 0 to 63", for example in
`AddNotify(-1,"Work:file")`. Without runtime error checking, the
commands are as before.

**What to change.** Use the slots 1 to 63. Slot 0 is allowed, but
`CheckNotify` also returns 0 when no message is waiting.

Issue: [AddNotify/RemNotify do not check the notification id (64 table entries) (#626)](https://github.com/fubar-coder/bb2-c/issues/626).

### A list number outside 0 to 4 {#linked-list-number-range}

**What happened.** The commands of
[rilinkedlistlib]({{< relref "/docs/libraries/userlibs/rilinkedlistlib" >}})
keep five linked lists, numbered 0 to 4. The library's guide warns
against higher numbers, but the original did not check the list
number, also not with runtime error checking: a number outside 0 to 4
wrote into the other lists and the memory behind them. With the
original, this program printed `-1` and `list 0 changed`, because
`InitList 5` took over the last item of list 0:

```
NEWTYPE .t
  a.l
End NEWTYPE
DEFTYPE .t v
v\a=1
r.l=InitList(0,v)
a0.l=AddrListItem(0)
r=InitList(5,v)
NPrint r
r=LastListItem(0)
If AddrListItem(0)=a0 Then NPrint "list 0 kept" Else NPrint "list 0 changed"
End
```

**With the port.** With runtime error checking, a list number outside 0
to 4 is the error "List number must be 0 to 4", in every command with a
list number; in the program above at `InitList(5,v)`. Without runtime
error checking, the commands are as before.

**What to change.** Use only the lists 0 to 4.

Issue: [The list number is not checked: list# outside 0–4 indexes past the list tables (#732)](https://github.com/fubar-coder/bb2-c/issues/732).

### An animation position outside the bitmap {#anim-position-outside}

`RIAnimInit` of
[rianimlib]({{< relref "/docs/libraries/userlibs/rianimlib" >}}) unpacks
the first frame of an animation into a bitmap and takes the position of
the animation in the bitmap either as a pixel position `x`,`y`, with `x`
rounded down to a multiple of 8, or as a byte offset into the
bitplanes. `RINextAnimFrame` then draws each frame at that position.

**What happened.** The original turned `x`,`y` into a byte offset
without a look at the bitmap, also not with runtime error checking. A
position outside the bitmap put the frames into the memory behind it,
and a negative `y` far away from it. With an animation loaded at address
`a`, this program printed `before` and `after -1`, and the first frame
went into the row below the one the position named:

```
BitMap 0,32,16,1
NPrint "before"
r=RIAnimInit(a,0,0,32,0)
NPrint "after ",r
```

**With the port.** With runtime error checking, an `x` outside 0 to
the bitmap's width minus 1 or a `y` outside 0 to its height minus 1 is
the error "Animation position outside the bitmap"; the program above
prints `before` and stops at `RIAnimInit`. The byte offset form is not
checked. Without runtime error checking, the command is as before.

**What to change.** Give a position inside the bitmap, and make the
bitmap large enough for the whole animation at that position: that part
is not checked.

Issue: [RIAnimInit x,y: the row offset is MULU.w of the low words – a negative y gives a huge offset (#514)](https://github.com/fubar-coder/bb2-c/issues/514).

### An ANIM7 animation at an odd byte {#anim7-odd-byte}

`RIAnimInit` places an animation at a byte of the bitmap
([An animation position outside the bitmap](#anim-position-outside)).

**What happened.** The decoder of the ANIM7 format writes whole words
and longs into the bitplanes, which a 68000 can only do at an even
address. The original did not check the position, also not with runtime
error checking: at an odd byte, an `x` of 8 to 15, 24 to 31 and so on or
an odd byte offset, the first ANIM7 frame crashed a 68000 with a Software
Failure. With an ANIM7 animation loaded at address `m`, this crashed at
`RINextAnimFrame`:

```
BitMap 0,64,16,1
If RIAnimInit(m,0,0,8,0)
  r=RINextAnimFrame(0)
EndIf
```

**With the port.** With runtime error checking, `RINextAnimFrame` stops
before an ANIM7 frame at an odd byte with the error "Animation position
on an odd byte for ANIM7", on every processor. ANIM5 frames are written
byte by byte and play at any position. Without runtime error checking,
the commands are as before.

**What to change.** Place ANIM7 animations at an even byte: an `x` of
0 to 7, 16 to 23 and so on, or an even byte offset.

Issue: [RIAnimInit with an odd byte offset (`x>>3` or `xy_offset`) for an ANIM7 animation: address error on a 68000 at RINextAnimFrame, no error check (#1122)](https://github.com/fubar-coder/bb2-c/issues/1122).

### `BBShadowValue` with a shadow set the scenario does not have {#bbshadowvalue-set}

`BBShadowValue` of
[ribbmaplib]({{< relref "/docs/libraries/userlibs/ribbmaplib" >}})
returns one long number, a shadow set, from the `SHDW` chunk of the
scenario chosen with `SetBBScenarioLoc`; the sets are counted from 0.

**What happened.** The original did not compare the set with the length
of the chunk, also not with runtime error checking: a set the scenario
does not have returned the bytes behind the chunk, the next chunk or
memory behind the file. With a scenario at address `m` whose `SHDW`
chunk holds two sets, the values `$11111111` and `$22222222`, followed by
a chunk `NEXT`, this program printed `-1`, `22222222` and `4E455854`,
the name of the next chunk:

```
NPrint SetBBScenarioLoc(m)
NPrint Hex$(BBShadowValue(1))
NPrint Hex$(BBShadowValue(2))
```

**With the port.** With runtime error checking, a negative set or one
the chunk does not hold is the error "Shadow set must lie in the SHDW
chunk"; the program above prints `-1` and `22222222` and stops at
`BBShadowValue(2)`. A scenario without a `SHDW` chunk is not checked,
and `BBShadowValue` returns 0 for it. Without runtime error checking,
the command is as before.

**What to change.** Ask only for the sets the scenario holds, from 0 to
one less than their number.

Issue: [BBShadowValue reads outside the SHDW chunk for a set it does not hold, with no error check (#1114)](https://github.com/fubar-coder/bb2-c/issues/1114).

### `FNSClip` below 0 and `FNSSetTab` below 1 {#fns-clip-tab-checks}

`FNSClip xmin,ymin,xmax,ymax` of
[rifnslib]({{< relref "/docs/libraries/userlibs/rifnslib" >}}) sets the
clip zone, the part of the bitmap that the FNS prints may draw in, and
`FNSSetTab` the distance between the tab positions of a text.

**What happened.** The original checked neither, also not with runtime
error checking. With a clip zone that starts left of or above the
bitmap, a text at a negative position was written about 64 KB away from
the bitmap. After `FNSSetTab 0`, a tab character in a text divided by
zero.

**With the port.** With runtime error checking, an `xmin` or `ymin`
below 0 is the error "FNSClip zone must not start below 0", and a tab
size below 1 the error "FNSSetTab size must be at least 1". Without
runtime error checking, the commands are as before.

**What to change.** Start the clip zone at 0 or further right and down,
for example with `FNSClipOutput`, and give `FNSSetTab` a size of 1 or
more.

Issues:

* [A clip zone with negative xmin/ymin lets text at negative x/y be written about 64K bytes/rows away (#664)](https://github.com/fubar-coder/bb2-c/issues/664)
* [FNSSetTab 0: a tab character divides by zero (#666)](https://github.com/fubar-coder/bb2-c/issues/666)

### The checks of the palette commands of rigfxlib {#rigfxlib-palette-checks}

The functions of
[rigfxlib]({{< relref "/docs/libraries/userlibs/rigfxlib" >}}) that read
a colour, `PalRed`, `AGAPalRed` and the others, and `PalAdjust` work on
the palette chosen with `PaletteInfo`. `PalAdjust` writes the adjusted
colours into a target palette, all of them or those from `start_col`
to `end_col`.

**What happened.** The original checked none of the arguments, also not
with runtime error checking. Before the first `PaletteInfo` the
commands read the colours from address 0. A colour number beyond the
palette read, or for `PalAdjust` wrote, the memory behind its colours,
and one from 2731 on the memory before them. `PalAdjust` with an end
colour one below the start colour wrote 256 KB of colours, and without
a range it wrote as many colours as the palette of `PaletteInfo` has,
also into a smaller target palette.

**With the port.** With runtime error checking, these are runtime
errors:

* a colour function or `PalAdjust` before `PaletteInfo`: "No palette
  selected with PaletteInfo";
* a colour number the palette of `PaletteInfo` does not have, also as
  `start_col` or `end_col` of `PalAdjust`: "Colour number outside the
  PaletteInfo palette";
* an `end_col` of `PalAdjust` below its `start_col`: "PalAdjust end
  colour before the start colour";
* a target palette of `PalAdjust` with fewer colours than it writes:
  "PalAdjust colours outside the target palette".

Without runtime error checking, the commands are as before, so they
stay as fast as in the original, for example when `PalAdjust` fades a
palette in every frame.

**What to change.** Choose the palette with `PaletteInfo` first, ask
only for the colours it has, and give `PalAdjust` a target palette at
least as large as the colours it writes, as the library's guide asks.

Issues:

* [PalRed/…/AGAPalBlue and PalAdjust read from address 0 before PaletteInfo (#686)](https://github.com/fubar-coder/bb2-c/issues/686)
* [Colour numbers from 2731 on index before the palette (12 × colour used as a signed word) (#677)](https://github.com/fubar-coder/bb2-c/issues/677)
* [PalAdjust with end < start runs up to 65536 × 3 channels (#684)](https://github.com/fubar-coder/bb2-c/issues/684)
* [PalAdjust without start/end adjusts as many colours as PaletteInfo's palette has, not the target (#685)](https://github.com/fubar-coder/bb2-c/issues/685)

### Bitmap depths for the effects of rifxlib and `CPUCls` {#rwe-effects-depth}

The effects of
[rifxlib]({{< relref "/docs/libraries/userlibs/rifxlib" >}}), such as
`ClearBitmap`, `FadeInBitmap`, the zooms and `Derez`, and `CPUCls` of
[rigfxlib]({{< relref "/docs/libraries/userlibs/rigfxlib" >}}) work
through the bitplanes of a bitmap object one by one. The guide of
rifxlib allows at most 8 bitplanes.

**What happened.** The original checked the depth nowhere, also not
with runtime error checking. A bitmap of depth 0 ran their loops 65536
or 256 times, over the plane pointers and the memory behind the bitmap
object and over the library's own data. With more bitplanes than its
tables hold, rifxlib wrote its plane pointers over its own variables
and, from 11 or 21 planes on, over its code; `CPUCls` cleared memory through the
pointers behind the bitmap's eight.

**With the port.** With runtime error checking, a bitmap of depth 0 or
more than 8 is the error "Illegal BitMap Depth". For `ZoomXY` the check is on the bitmaps chosen
with `InitZoomXY`. Without runtime error checking, the commands are as
before.

**What to change.** Use bitmaps of 1 to 8 bitplanes.

Issues:

* [Derez and ZoomXY with a depth-0 source bitmap count 65536 planes (#947)](https://github.com/fubar-coder/bb2-c/issues/947)
* [The 16-entry plane pointer arrays are not checked against the depth: FadeInBitmap overflows with more than 8 planes (#674)](https://github.com/fubar-coder/bb2-c/issues/674)
* [CPUCls: depth 0 clears 65536 planes (#679)](https://github.com/fubar-coder/bb2-c/issues/679)

### A delay of 0 for `ClearBitmap` and `FadeInBitmap` {#rifxlib-delay-zero}

`ClearBitmap` and `FadeInBitmap` of
[rifxlib]({{< relref "/docs/libraries/userlibs/rifxlib" >}}) wait after
every line for a number of turns of an empty loop, the delay, so that
the effect can be seen.

**What happened.** The loop counts down before it tests, so a delay of
0 meant 4294967296 turns per line, and the program seemed to hang. The
library's guide warns against 0, but the original did not check it,
also not with runtime error checking.

**With the port.** With runtime error checking, a delay of 0 is the
error "Delay must not be 0". Without runtime error checking, the
commands are as before.

**What to change.** Give a delay of 1 or more.

Issue: [ClearBitmap/FadeInBitmap: delay 0 means 2^32 turns of the busy loop per line (#669)](https://github.com/fubar-coder/bb2-c/issues/669).

### `ZoomyZoom` on a bitmap smaller than 640 x 32 {#zoomyzoom-size}

`ZoomyZoom` of
[rifxlib]({{< relref "/docs/libraries/userlibs/rifxlib" >}}) doubles
the left half of the first 32 lines of a bitmap 640 pixels wide in
place.

**What happened.** The original did not look at the size of the bitmap,
also not with runtime error checking: it always worked on 32 lines of
80 bytes. On a narrower bitmap these were not its lines, and on a
smaller one it wrote behind the bitplanes.

**With the port.** With runtime error checking, a bitmap less than 640
pixels wide or less than 32 lines high is the error "ZoomyZoom needs a
bitmap of at least 640 x 32". Without runtime error checking, the
command is as before.

**What to change.** Use `ZoomyZoom` only on bitmaps of at least 640 x
32 pixels.

Issue: [ZoomyZoom ignores the bitmap's size: always 32 lines of 80 bytes (#676)](https://github.com/fubar-coder/bb2-c/issues/676).

### `ZoomXY` without `InitZoomXY` {#zoomxy-without-initzoomxy}

`ZoomXY` of
[rifxlib]({{< relref "/docs/libraries/userlibs/rifxlib" >}}) takes no
bitmap of its own: it zooms from the source into the destination that
`InitZoomXY` chose, so `InitZoomXY` must come first.

**What happened.** The original did not look whether `InitZoomXY` had
chosen any bitmaps, also not with runtime error checking. Before it,
`ZoomXY` read a bitmap at address 0, took the processor's exception
vectors there for its bitplane pointers and wrote through them, and the
machine crashed.

**With the port.** With runtime error checking, `ZoomXY` before any
`InitZoomXY` of the run is the error "ZoomXY needs InitZoomXY". Without
runtime error checking, the command is as before.

**What to change.** Call `InitZoomXY` before the first `ZoomXY`.

Issue: [ZoomXY before InitZoomXY reads a bitmap at address 0 and writes through the exception vectors (#1131)](https://github.com/fubar-coder/bb2-c/issues/1131).

### A colour split of fewer than one line {#colsplit-numlines}

`DoColSplit` of
[ricopperfxlib]({{< relref "/docs/libraries/userlibs/ricopperfxlib" >}})
writes a colour gradient over `numlines` lines into the custom space of
a coplist, and `RedoColSplit` writes new colours into such a split.

**What happened.** The original did not check `numlines`, also not with
runtime error checking. A count of 0 divided by zero and stopped the
program with a processor exception; a negative count wrote up to 65536
lines of copper instructions over the memory behind the custom space.

**With the port.** With runtime error checking, a `numlines` below 1 is
the error "Colour split needs at least one line". Without runtime error
checking, the commands are as before.

**What to change.** Give every colour split at least one line.

Issue: [DoColSplit/RedoColSplit do not check numlines: 0 divides by zero, a negative value writes up to 65536 lines (#589)](https://github.com/fubar-coder/bb2-c/issues/589).

### `ChunkyToPlanar` with sizes that do not fit {#chunkytoplanar-sizes}

`ChunkyToPlanar` of
[richunkylib]({{< relref "/docs/libraries/userlibs/richunkylib" >}})
converts a chunky bitmap, one byte per pixel, into the eight bitplanes
of a bitmap. It converts the chunky bitmap's width and height into the
top left of the bitmap, row by row.

**What happened.** The original always converted 320 x 256 pixels into
bitplanes 10240 bytes apart, whatever the sizes of the chunky bitmap
and of the bitmap, also with runtime error checking. It read behind a
smaller chunky bitmap, wrote behind a smaller bitmap and mixed up the
bitplanes of an interleaved one.

**With the port.** A chunky bitmap whose width is not a multiple of 16,
one that is wider or higher than the bitmap, and a bitmap whose
bitplanes are not equally far apart are not converted: nothing is
written. With runtime error checking these are the errors "Chunky
bitmap width not a multiple of 16", "Chunky bitmap larger than the
bitmap" and "Bitmap planes not equally spaced". A bitmap of fewer than
8 bitplanes and a chunky bitmap without data do nothing, as in the
original, also with runtime error checking.

**What to change.** Give the chunky bitmap a width that is a multiple
of 16, and convert it into a bitmap of 8 bitplanes that is at least as
wide and as high.

Issue: [ChunkyToPlanar ignores the sizes of the chunky bitmap and of the bitmap (always 320×256×8) (#884)](https://github.com/fubar-coder/bb2-c/issues/884).

### `RaycastSky` outside the chunky bitmap {#raycastsky-outside}

`RaycastSky` of
[richunkylib]({{< relref "/docs/libraries/userlibs/richunkylib" >}})
copies `height` bytes of a column of a texture into a column of a
chunky bitmap, both with rows of 320 bytes. `x` and `column` are
offsets from the start of the chunky bitmap and of the texture.

**What happened.** The original checked neither offset, also not with
runtime error checking. A negative `x` wrote in front of the chunky
bitmap, an `x` whose column runs past the last row wrote behind it, and
a negative `column` read in front of the texture.

**With the port.** With runtime error checking, an `x` below 0, or one
whose column would run past the end of the chunky bitmap, is the error
"RaycastSky outside the chunky bitmap", and a `column` below 0 is the
error "RaycastSky column below 0". A height below 1 is not checked:
`RaycastSky` copies nothing for it. Without runtime error checking, the
offsets are not checked.

**What to change.** Keep `x` and `column` at 0 or above, and the column
of `height` rows inside the chunky bitmap.

Issue: [RaycastSky with height 0 copies 65536 rows; x and column are not checked (#887)](https://github.com/fubar-coder/bb2-c/issues/887).

### The maps of rimapeditorlib: stencil, data and positions {#rimapeditorlib-map-checks}

`MapCanDraw(shape#,mapx,mapy)` of
[rimapeditorlib]({{< relref "/docs/libraries/userlibs/rimapeditorlib" >}})
tells whether a shape may be put into a cell of the current map. It
looks up the value of the cell in the stencil, a byte array set with
`SetStencil`. `MapDataAdr(x,y)` returns the address of a cell, and
`MapDataGrab` fills all cells of the current map from memory.

**What happened.** The original checked none of this, also not with
runtime error checking. Before any `SetStencil`, `MapCanDraw` read the
stencil at address 0. A cell value of 32768 or more counted as negative
and read up to 32 KB in front of the stencil. On a map without data,
such as one whose `CreateMap` could not get memory, `MapCanDraw` read
at address 0. `MapDataAdr(x,y)` returned an address for any position,
also behind the map.

**With the port.** With runtime error checking, these are runtime
errors:

* `MapCanDraw` before any `SetStencil`: "MapCanDraw needs SetStencil";
* a cell value of 32768 or more, where `MapCanDraw` looks it up in the
  stencil: "Map value above 32767 for the stencil";
* `MapCanDraw` or `MapDataGrab` on a map without data, after
  `Free MapObject` or a `CreateMap` that got no memory: "MapObject has
  no map data";
* `MapDataAdr(x,y)` with a position outside the map: "Map position
  outside the map".

Without runtime error checking, the commands are as before, except
that `MapDataGrab` fills nothing into a map without data.

**What to change.** Call `SetStencil` before `MapCanDraw`, keep the
cell values below 32768, and ask `MapDataAdr` only for positions
inside the map.

Issues:

* [MapCanDraw's stencil lookup is unchecked: without SetStencil it reads low memory; map words ≥ $8000 index before the stencil (#893)](https://github.com/fubar-coder/bb2-c/issues/893)
* [MapCanDraw compares x/y signed: negative coordinates pass and index far behind the map data (#892)](https://github.com/fubar-coder/bb2-c/issues/892)

### `FillBitplane` and `ShapeTo1616` with sizes that do not fit {#rimapeditorlib-size-checks}

`FillBitplane bitmap#,width,height` of
[rimapeditorlib]({{< relref "/docs/libraries/userlibs/rimapeditorlib" >}})
fills the top left of the first bitplane of a bitmap with a mesh,
`width` rounded down to a multiple of 16 pixels.
`ShapeTo1616 source,dest` copies the top left 16 by 16 pixels of every
plane of a shape into a shape of 16 by 16 pixels.

**What happened.** The original checked neither, also not with
runtime error checking. `FillBitplane` with a width below 16 wrote
65536 words into each row, and with a height of 0 it filled 65536
rows. `ShapeTo1616` from a shape of height or depth 0 copied 65536
rows or planes. It wrote 16 rows for every plane of the source into
the destination whatever its size, so a destination with fewer planes
or rows was overrun.

**With the port.** With runtime error checking, these are runtime
errors:

* a `FillBitplane` width below 16: "FillBitplane width must be at
  least 16";
* a `FillBitplane` height of 0: "FillBitplane height must be at least
  1";
* a `ShapeTo1616` source shape of height or depth 0: "ShapeTo1616
  source of height or depth 0";
* a `ShapeTo1616` destination that is not 16 by 16 pixels with at least
  the depth of the source: "ShapeTo1616 needs a 16 x 16 destination as
  deep as the source".

Without runtime error checking, the commands are as before.

**What to change.** Give `FillBitplane` a width of 16 or more and a
height of 1 or more, and give `ShapeTo1616` a destination shape of 16
by 16 pixels with as many planes as the source.

Issues:

* [MapDataGrab, ShapeTo1616 and FillBitplane loop 65536 times for a size of 0 (DBRA) (#895)](https://github.com/fubar-coder/bb2-c/issues/895)
* [ShapeTo1616 writes 16 words per source plane into the destination shape regardless of its size (#900)](https://github.com/fubar-coder/bb2-c/issues/900)

### `CreateMap` wider than 32767 cells {#createmap-width}

`CreateMap` of
[rimapeditorlib]({{< relref "/docs/libraries/userlibs/rimapeditorlib" >}})
makes a map of `width` by `height` cells of one word each.

**What happened.** The original computed the size of a row in 16 bits,
so a map wider than 32767 cells got far too little memory: a width of
40000 got 14464 bytes per row instead of 80000. `MapDataGrab` then
wrote behind the map's data, and `MapCanDraw` read there.

**With the port.** With runtime error checking, a width above 32767 is
the error "Map width above 32767". Without runtime error checking,
`CreateMap` is as before.

**What to change.** Keep maps at 32767 cells wide or less.

Issue: [CreateMap with a width above 32767 gets too little data: MapDataGrab writes behind the map (#1152)](https://github.com/fubar-coder/bb2-c/issues/1152).

### Snow field numbers of risnowlib {#snow-list-checks}

The snow commands of
[risnowlib]({{< relref "/docs/libraries/userlibs/risnowlib" >}}) work on
one of 4 snow fields, numbered 0 to 3, which `InitSnowDrop` sets up on
a bitmap. `KillPoint listnum,x,y` removes a pixel of the field's
bitmap, which the library takes as 320 pixels wide.

**What happened.** The original checked neither the number nor the
pixel, also not with runtime error checking. A number from 4 on, or a
negative one, read and wrote the memory behind or in front of the
library's table of fields; `InitSnowDrop` wrote four pointers there.
`LDoSnow`, `LDoSnowActivate` and `KillPoint` on a field without
`InitSnowDrop` read its lists and its bitmap at address 0, and
`KillPoint` outside the bitmap changed the memory around it.

**With the port.** With runtime error checking, these are runtime
errors:

* a field number outside 0 to 3, for every snow command: "Snow list
  number must be 0 to 3";
* `LDoSnow`, `LDoSnowActivate` and `KillPoint` on a field without
  `InitSnowDrop`: "Snow list needs InitSnowDrop";
* `KillPoint` with an `x` outside 0 to 319 or a `y` outside the rows of
  the bitmap: "KillPoint outside the bitmap".

Without runtime error checking, the commands are as before.

**What to change.** Use the fields 0 to 3, set each up with
`InitSnowDrop` before the other commands use it, and keep `KillPoint`
inside the bitmap.

Issue: [The list number is not checked: listnum outside 0–3 indexes past the list table (#913)](https://github.com/fubar-coder/bb2-c/issues/913).

### `ShapeToMapShape` of a shape of depth 0 or more than 8 {#shapetomapshape-depth}

`ShapeToMapShape` of
[rimapshapeslib]({{< relref "/docs/libraries/userlibs/rimapshapeslib" >}})
makes a map shape from a Blitz shape. A map shape has room for the
pointers to 8 planes.

**What happened.** The original checked the depth of the shape
nowhere, also not with runtime error checking. A shape of depth 0
wrote 65536 plane pointers over memory, and one of 14 planes or more
wrote into the next map shape.

**With the port.** With runtime error checking, a shape of depth 0 or
more than 8 is the error "Illegal Shape Depth", as with the shape
commands of Blitz Basic 2. Without runtime error checking, the command
is as before.

**What to change.** Make map shapes from shapes of 1 to 8 planes.

Issue: [ShapeToMapShape of a shape with depth 0 writes 65536 plane pointers; depth 14 and up runs into the next map shape (#903)](https://github.com/fubar-coder/bb2-c/issues/903).

### A group of more than 63 members {#group-63-members}

`AddGroupMember` of
[rimapshapeslib]({{< relref "/docs/libraries/userlibs/rimapshapeslib" >}})
adds a map shape to the group that `BlitGroup` draws in one go. The
group has room for 63 members.

**What happened.** The original did not count the members: from the
64th on, it wrote them behind the memory of the group.

**With the port.** A group holds at most 63 members, and
`AddGroupMember` ignores further ones. With runtime error checking, a
64th member is the error "Group holds at most 63 members". A program
that added 64 members drew them all with the original, if nothing else
used the memory behind the group; with the port, `BlitGroup` draws
the first 63.

**What to change.** Keep a group at 63 members or fewer; draw more map
shapes with a second group or with `BlitMapShape`.

Issue: [AddGroupMember does not count members: from the 64th on it writes behind the 512-byte group block (#904)](https://github.com/fubar-coder/bb2-c/issues/904).

### Map shape numbers beyond `SetNumMapShapes` {#mapshape-number-checks}

The map shapes of
[rimapshapeslib]({{< relref "/docs/libraries/userlibs/rimapshapeslib" >}})
are numbered from 0; `SetNumMapShapes num` makes room for `num` of
them. `BlitMapShapeSize` blits the top left part of a map shape.

**What happened.** The original checked the numbers nowhere, also not
with runtime error checking: a number beyond `SetNumMapShapes` read and
wrote the memory behind the map shapes, and before any
`SetNumMapShapes` the map shapes lay at address 0. `BlitMapShapeSize`
with a width larger than the map shape's read each row on into the
next and the last one behind the shape.

**With the port.** Without map shapes, before `SetNumMapShapes` or
after one that got no memory or asked for 0, the map shape commands
do nothing and `MapShapeAddr` returns 0. With runtime error checking,
the port reports two errors:

* For `ShapeToMapShape`, `BlitMapShape`, `BlitMapShapeSize`,
  `MapShapeAddr`, `FreeMapShapes` and `AddGroupMember`, a map shape
  number not below the number given to `SetNumMapShapes` is the error
  "Map shape not allocated by SetNumMapShapes".
* A `BlitMapShapeSize` wider than the map shape is the error
  "BlitMapShapeSize wider than the map shape".

**What to change.** Call `SetNumMapShapes` first, use only the numbers
it makes room for, and blit at most the width of the map shape.

Issue: [Map shape numbers are not checked; before SetNumMapShapes the map shapes lie at address 0 (#906)](https://github.com/fubar-coder/bb2-c/issues/906).

### Sprite channels and shapes of riwbspritelib {#wbsprite-checks}

The commands of
[riwbspritelib]({{< relref "/docs/libraries/userlibs/riwbspritelib" >}})
name a Workbench sprite by its sprite channel, 0 to 7; `GetWBSprite`
also takes -1 for any free channel. `GetWBSprite` and `ChangeWBSprite`
copy a shape into the sprite.

**What happened.** The original had no error checks at all. A channel
outside 0 to 7 read and wrote the library's own data beside its table
of sprites, and `FreeWBSprite` freed that data as a sprite.
`GetWBSprite` with channel 8 even returned 8, as if it had got a sprite.
A shape of height 0 or depth 0 would have been copied as 65536 rows or
planes. `WBSpriteX` and `WBSpriteY` of a channel without a sprite
returned words from low memory.

**With the port.** With runtime error checking:

* a channel outside 0 to 7 is the error "Sprite channel must be 0 to
  7", and for `GetWBSprite` one outside -1 to 7 the error "Sprite
  channel must be -1 to 7";
* in `GetWBSprite` and `ChangeWBSprite`, a shape of height 0 is the
  error "Illegal Shape Height", and one of depth 0 or more than 8 the
  error "Illegal Shape Depth", as with the shape commands of Blitz
  Basic 2;
* `WBSpriteX` or `WBSpriteY` of a channel without a sprite of the
  library is the error "No sprite on this channel".

Without runtime error checking, the commands are as before. The shape
commands of Blitz Basic 2 make no shape of height or depth 0, so only a
shape changed in memory meets that check.

**What to change.** Use the channels 0 to 7, or -1 with `GetWBSprite`,
and ask for the position only of a sprite the program has got.

Issues:

* [The sprite channel is not checked: channels outside 0–7 index the tag lists, the library name and its base (#916)](https://github.com/fubar-coder/bb2-c/issues/916)
* [A shape with height or depth 0 is copied as 65536 rows/planes (DBRA) (#917)](https://github.com/fubar-coder/bb2-c/issues/917)
* [WBSpriteX/WBSpriteY on a channel without a sprite read the words at addresses 6 and 8 (#922)](https://github.com/fubar-coder/bb2-c/issues/922)

### Devices, tracks and offsets of ricompactdisclib {#compactdisc-checks}

[ricompactdisclib]({{< relref "/docs/libraries/userlibs/ricompactdisclib" >}})
works with one CD device at a time, which `OpenCD` opens and `CloseCD`
closes. `CDTrackLength track#` reads the length of a track from the
table of contents of `CDReadTOC`, and `CDTrackMins` and `CDTrackSecs`
return the position of the last `CDUpdateInfo`, from the start of the
track with an offset of 0 or from the start of the CD with 1.

**What happened.** The original had no error checks. A second `OpenCD`
without `CloseCD` opened the new device and lost the old one, which
stayed open until the next reboot. `CDTrackLength` of a track the CD
does not hold, such as track 0 or one behind the last, read memory
behind the table of contents, and `CDTrackMins` or `CDTrackSecs` with
another offset read other memory of the library. The results meant
nothing.

**With the port.** With runtime error checking:

* `OpenCD` while a device is open is the error "CD device already open:
  CloseCD first";
* `CDTrackLength` of a track outside the first to the last track of the
  table of contents is the error "Track number out of range" (without
  an open device or a table of contents it returns 0 and is no error);
* `CDTrackMins` or `CDTrackSecs` with an offset other than 0 or 1 is
  the error "Offset must be 0 or 1".

Without runtime error checking there are no such errors, and these
calls go wrong as with the original.

**What to change.** Call `CloseCD` before opening another device, as the
library's guide asks. Check a track number against `CDFirstTrack` and
`CDLastTrack` before `CDTrackLength`, and give `CDTrackMins` and
`CDTrackSecs` an offset of 0 or 1 only.

Issues:

* [OpenCD while a device is open loses the open request (#578)](https://github.com/fubar-coder/bb2-c/issues/578)
* [CDTrackLength reads unwritten or foreign memory for the last track, track 0 and tracks behind the last (#583)](https://github.com/fubar-coder/bb2-c/issues/583)
* [CDTrackMins/CDTrackSecs offset: only the low byte is shifted, the index has no limit, the high byte stays in the result (#586)](https://github.com/fubar-coder/bb2-c/issues/586)

### The bitmap of the font printer of rilesdebuglib {#font-printer-bitmap}

The font printer of
[rilesdebuglib]({{< relref "/docs/libraries/userlibs/rilesdebuglib" >}}),
`FPrint`, `FNPrint` and `FPrintDetoke`, prints with an 8x8 font into
the first bitplane of the used bitmap. It writes rows of 80 bytes, so
the bitmap must be 640 pixels wide, and the cursor of `FLocate` counts
rows of 8 pixels.

**What happened.** The original never looked at the size of the bitmap.
On a narrower bitmap the characters landed in the wrong rows, and a
cursor row below the bitmap wrote into the memory behind it.

**With the port.** With runtime error checking, the three commands
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

### More than 20 codes of `AddProcessPtr` {#addprocessptr-20-codes}

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

## Forms the original refused {#forms-the-original-refused}

Most cases of this page go one way: the original compiled them and the
port does not, or compiles them to something else. The cases in this
section go the other way, as `Frac` of a string does: a source that the
original refused compiles with the port.

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
[rifnslib]({{< relref "/docs/libraries/userlibs/rifnslib" >}}) puts an
FNS font that a program holds in memory into a font slot. The font
does not say how many characters it has, and the original drew a
character behind its last glyph from the memory behind the font. It
took no third argument: the compiler refused `InstallFNS(1,f,100)`.

**With the port.** A character the font has no glyph for is printed as
a space ([Characters an FNS font has no glyph for]({{< relref "/docs/compared-with-original/fixed-bugs/libraries#fns-missing-glyphs" >}})).
For that the library needs the number of glyphs. The form
`InstallFNS(fontnum,address,characters)` gives it, counted from the
space on; the form without it takes the 91 glyphs, from the space to
`z`, that every known FNS font has.

**What to change.** Nothing for a font that ends at `z`. Give a font
with more characters its number as the third argument; a source that
must also compile with the original loads such a font with `FNSLoad`,
which takes the number from the size of the file.

Issue: [Character codes from 160 on end the text (sign test after CMPI.b #32) (#657)](https://github.com/fubar-coder/bb2-c/issues/657).
