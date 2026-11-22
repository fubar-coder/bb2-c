---
title: From source to executable
weight: 4
description: Why a small program becomes an executable of several kilobytes, what Make Smallest Code does, what runtime error checking adds, and in which type an expression is computed.
---

Before this page you should know the kinds of libraries ([Libraries:
acidlibs, deflibs, userlibs]({{< relref "/docs/how-blitz2-works/libraries" >}})).

This page follows a program through the compiler. It starts with what you
can see from the outside, the size of the executable and the effect of
the options, then shows how the compiler chooses the type in which an
expression is computed, and ends with the inner layout, for readers who
write libraries.

## Why a small program is several kilobytes

Take this program:

```
a$="Hello"
Print Left$(a$,2)
End
```

Its own code is a few dozen machine instructions, yet the executable is
several kilobytes long. Almost all of it is library code. `Print` brings
its whole library and the libraries that library needs (console output,
the DOS library, floating point for numbers), `Left$` brings the string
library, and the runtime libraries come in for the variables, the memory,
the string handling and the end of the program. Each library is copied
whole, once, however many of its commands you use
([The whole library, not just the command]({{< relref "/docs/how-blitz2-works/libraries#the-whole-library-not-just-the-command" >}})).

So the size of an executable grows in steps: a little with every line,
and a lot with the first command of a library you have not used before.

## Make Smallest Code

The compiler works on a block of memory whose parts have fixed sizes,
taken from the compiler options (**Object Buffer**, **Libs Buffer** and
the others). With **Make Smallest Code** on, it compiles the program
twice: the first pass shows how much of each part the program really
needs, the second compiles again into parts of exactly that size. Without
it, the unused space of each part would be in the program as well.
CREATE EXECUTABLE always uses it.

If a part is too small for the program, the compiler stops with the
requester "Not Enough Object Memory Available!" and offers three ways
on: make the buffers larger and compile again, use the smallest sizes and
compile again, or cancel.

## What runtime error checking adds

With the option **Runtime Error Debugger** on, the compiler adds three
things to the program:

* **A check point before every statement.** It tells the runtime which
  line and statement is running, so that an error can be reported at the
  right place. It costs a few bytes and an exception on every statement,
  which makes the program noticeably slower.
* **The libraries' check routines.** Each library has an area with checks
  of its arguments (an index out of range, an object that does not exist).
  Without runtime error checking that area is cut off when the library is
  copied; with it, it is copied and called around every command.
* **The runtime error libraries.** Run from the editor, the program gets
  the connection to the debugger; as an executable it gets a small handler
  that shows the error in a requester and ends the program.

The check points contain addresses of the source lines in the editor's
memory. So two executables compiled from the same source with runtime
error checking on are usually not identical byte for byte; without it,
they are.

{{< callout type="info" >}}
Try it: compile the program above with CREATE EXECUTABLE twice, once
with **Runtime Error Debugger** on and once with it off, and compare the
two file sizes in the Shell with `List`.
{{< /callout >}}

## The type of an expression

Blitz Basic 2 has five number types: Byte (`.b`), Word (`.w`), Long
(`.l`), Quick (`.q`) and Float (`.f`). Quick is a fixed-point number with
a fraction and a range of about ±32768, and it is the default type: a
variable without a type suffix is a Quick. The compiler decides for every
expression in which of these types it is computed, and the answer is not
always the one the source suggests.

### Where the computation starts

The compiler reads an expression from left to right and keeps one type,
the type so far. It starts with the type the result is wanted in: the
type of the variable in an assignment, or the type of the parameter the
expression is passed to. `If`, `While` and `Print` want no particular
type, so there the expression starts without one. Each value then raises
the type so far if its own type is higher, in the order Byte, Word, Long,
Quick, Float; a lower value is converted up.

The type so far also becomes Long when a Long operand stands to the left,
and after an addition or multiplication of two Words, whose result is a
Long. A small whole number such as `1` counts as a Byte, and two Bytes
added give a Word.

### A Quick in a Long computation

There is one exception to "the higher type wins": when the type so far is
Long and a Quick value comes in, the Quick does not make the computation a
Quick one. The reason is the default type. A line such as

```
adr.l=base+i*4
```

with a Long `base` and a variable `i` without a suffix would otherwise be
computed in Quick, and an address does not fit into the range of a Quick.
With the exception it is computed in Long, as intended.

In the original compiler the Quick is converted to a Long at once and its
fraction is cut off (towards minus infinity, so -1.5 becomes -2); only
then is the operation done, and a fraction disappears before the
arithmetic happens. The BB2 C port fixes this
([#1178](https://github.com/fubar-coder/bb2-c/issues/1178)): the
operation is computed with the fraction and gives a whole number, rounded
down for `+`, `-` and `*` and towards zero for `/`, as Blitz Basic's
divisions do:

```
q=1.5 : l.l=10 : w.w=3 : v.w=4
r.l=q*2     : NPrint r   ; 3 (the original: 2, q is cut to 1 first)
r.l=l-q     : NPrint r   ; 8, 8.5 rounded down (the original: 9)
r.l=l/q     : NPrint r   ; 6, 6.67 towards zero (the original: 10)
x.w=w+v+q*2 : NPrint x   ; 10 (the original: 9)
x.w=q*2+w+v : NPrint x   ; 10
NPrint 1+1+1+q           ; 4, not 4.5: 1+1+1 gives a Long
NPrint 1+1+q             ; 3.5: 1+1 is still a Word
```

The computation still follows the type so far, so the last two lines
differ: a sum that starts with whole numbers is a Long, and adding the
Quick to it gives a whole number. Shifts and the logic operators (`AND`,
`OR`, `EOR`, `Not`) take the Quick rounded down, as in the original. A
program that relies on the old result writes `Int(q)`: `r.l=Int(q)*2`
gives 2.

A Float is never rounded on the way: in a Long computation it makes the
type Float, and `r.l=2.0*q` gives 3. A Float and a Quick round
differently when they end up in a whole number: a Float is cut towards
zero (-1.5 gives -1), a Quick is rounded down (-1.5 gives -2).

To get the exact result with a fraction, compute in a Quick or Float
variable first (`t=q*2 : r.l=t`), or start the expression with a Float
literal (`r.l=2.0*q`). AmiBlitz3 keeps the original's rule.

### Comparisons

In the original compiler the same cut happens to the operands of a
comparison, and there it gives a wrong answer:

```
q=1.5 : l.l=1
r.l=(q>1)    : NPrint r     ; 0 in the original, but q>1 is true (-1)
If q>l Then NPrint "q>l"    ; printed
If l<q Then NPrint "l<q"    ; not printed in the original: the Long l comes first
```

The BB2 C port fixes this
([#1177](https://github.com/fubar-coder/bb2-c/issues/1177)): the operands
of a comparison are compared with their own values, a Long against a
Quick exactly, and only the result takes the type of the destination.
The example prints -1 and both lines. The same holds for a Quick or Float
destination, where the original compared whole numbers as Quick or Float
values (`rq.q=(l>1)` gives 0 there for `l.l=65537`).

Two cases are not fixed yet, because the compiler emits the code of the
left operand before it sees the comparison: a left operand that is an
expression, such as `r.l=(q*2>2.4)` with `q=1.25`, is computed as a whole
number first (2.5 is rounded down to 2, so the comparison is false;
[#1183](https://github.com/fubar-coder/bb2-c/issues/1183)), and
a large whole-number constant on the left in a Float context
(`rf.f=(100000001>l)`) is still compared as a Float. Put such an
operand on the right side of the comparison, or compute it into a
variable first. The right operand is computed in its own types:
`If base<i*4+base` with a default-typed (Quick) `i` computes `i*4` as a
Quick, which overflows from `i=8192` on; give `i` the type Long there.

### Array indexes

The first index of an array is always computed as a Long, so a Quick in
it is rounded as in a Long computation: `a(q*10)` with `q=1.5` is element
15 in the port (the original cut `q` first: element 10). A
later index is computed differently with and without runtime error
checking, a known original bug
([#1179](https://github.com/fubar-coder/bb2-c/issues/1179)); until it is
fixed, compute such an index into a Word or Long variable first.

## The layout of the executable

An executable of blitz2 has exactly one hunk, a code hunk, with
everything in it: your program, the libraries and the data. It is the
compiler's block of memory written out as it is, part after part, so its
layout follows the buffers of the compiler options:

```mermaid
flowchart TB
  subgraph H["The code hunk"]
    direction TB
    subgraph P["Program: Object Buffer"]
      direction TB
      J["1. call of the start routines"]
      Y["3. your program"]
      E["4. the end routines,<br/>back to AmigaOS"]
      I["2. the start routines"]
      J ~~~ Y ~~~ E ~~~ I
    end
    L["the library code<br/>Libs Buffer"]
    S["the system data<br/>System Buffer"]
    D["the Data values<br/>Data Buffer"]
    P ~~~ L ~~~ S ~~~ D
  end
```

The libraries lie in the order of their first use; the system data
holds the string literals and tables, and the last part the values of
the `Data` statements. The numbers give the order in which the parts of
the program part run. The program starts
at the top of the hunk with a call of the start routines, which come
back to your first line. When your program ends, at its last line or at
an `End` anywhere, it goes on with the end routines, which return to
AmigaOS. If no library in the program has a start routine, the call is
left out and the hunk starts with your first line.

Around the hunk the file has what AmigaOS needs to load it: the header
in front, and behind the code the table of every place that holds an
address, which AmigaOS corrects when it loads the program. With the
option **Create Debug Info for Executable Files** on, the names of the
labels follow as symbols for a debugger. In AB3 mode the directives
`BSS` and `Chip` can add a hunk of their own each
([Directives]({{< relref "/docs/ab3-compared/directives" >}})).

## One pass, one block, one hunk

{{% details title="The inner layout, for library authors" closed="true" %}}

**One pass.** The compiler reads the source once, from top to bottom, and
writes machine code as it goes. A label that is used before it is defined
is remembered and filled in when the definition comes. There is no
optimiser across statements.

**One block of memory.** The compiler divides one block of memory into
four parts, each with its own write position:

| Part | Holds | Size from the option |
|---|---|---|
| program | the code of your statements | Object Buffer |
| libraries | the copied library code | Libs Buffer |
| system data | string literals, tables | System Buffer |
| data | the values of `Data` statements | Data Buffer |

Every address in this code is an absolute address inside the block, and
the compiler keeps a list of all places that hold one. That is why RUN
can start the program right in the block, and why writing an executable
is simple: the compiler turns each listed address into an offset from the
start and writes the list as the relocation table of the file.

**One code hunk.** There is no data or BSS hunk. Variables and the string work area are
allocated when the program starts: the first start routine to run is the
one of the variables library, which allocates the memory for all global
variables and points register a5 into it. Global variables are then
addressed relative to a5, the local variables of a procedure relative to
a4, and a3 points into the string work area while strings are built.

**Start and end.** The start routines run in descending order of library
number, the end routines in ascending order. A library whose start
routine returns a value (an AmigaOS library base, a memory pointer) gets
a global variable for it, which its commands use later. `End` in the
middle of a program jumps to the end routines, so the libraries clean up
in every case.

{{% /details %}}
