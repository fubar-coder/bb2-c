---
title: Sources that compile differently
weight: 2
description: Sources that Blitz Basic 2 accepted and the BB2 C port refuses or compiles to something else, and the new runtime errors of the libraries, with what the original did and how to change them.
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

## Runtime errors of the libraries {#runtime-errors}

The cases above are found by the compiler. The ones in this section
compile as before but stop when the program runs: where the original
went on with a wrong argument and wrote over memory or hung, the port's
library reports a runtime error. Most of them are error checks, so they
come only in a program compiled with runtime error checking, like the
other checks of Blitz Basic 2; without it, the program runs as with the
original.

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

Issues: [Bitmaps of depth 0 made the plane loops run 65536 times (#393)](https://github.com/fubar-coder/bb2-c/issues/393),
[BBlit with depth 0 (#74)](https://github.com/fubar-coder/bb2-c/issues/74),
[QBlit with a shape of depth 0 (#243)](https://github.com/fubar-coder/bb2-c/issues/243),
[A shape gadget of depth 0 (#167)](https://github.com/fubar-coder/bb2-c/issues/167).

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
