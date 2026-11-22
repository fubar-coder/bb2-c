---
title: Inline assembler
weight: 7
description: 68000 instructions in a Blitz Basic 2 program - GetReg and PutReg, the instructions and addressing modes the compiler assembles, labels and constants in operands, Dc, Ds and Dcb, and calls of library routines.
---

Before this page you should know labels and constants
([The language]({{< relref "/docs/language#a-program-line-by-line" >}}),
[Types, variables and constants]({{< relref "types#constants" >}})).

The compiler has a 68000 assembler built in. A line of a program can hold
68000 instructions instead of Basic statements, or both, separated by
colons; the compiler assembles each instruction at the place where it
stands, between the code of the statements around it. There are three
ways to use it:

* **Inline**, in the middle of the program, with `GetReg` and `PutReg` to
  move values between Basic expressions and the registers. This page
  describes it.
* **In a procedure** made entirely of assembler, which gets its
  parameters in registers
  ([Procedures in assembler]({{< relref "procedures#procedures-in-assembler" >}})).
* **In a library** of your own, whose commands can be used like any
  other
  ([Writing your own library]({{< relref "/docs/blitz2-internals/own-userlib" >}})).

## Registers the program needs

Three address registers belong to the program, and assembler code must
give them back with their old values: a5 points to the global variables,
a4 to the local variables of the current procedure, and a6 holds a
library base the compiler may still use. All other registers are free,
but they keep nothing for you between two Basic statements: any
statement may change them. Values go in and out with `GetReg` and
`PutReg`.

## GetReg and PutReg

`GetReg register,expression` computes the expression as a Long and puts
the result into the register, any of d0 to d7 or a0 to a7. All other
registers keep their values. `PutReg register,variable` stores the
register in the variable and changes no register:

```
a.w=5
b.w=10
GetReg d0,a
GetReg d1,b
MULU d0,d1
PutReg d1,c.w
NPrint c                 ; 50
```

The variable of `PutReg` can be a plain variable, an array element or a
field of a NewType, such as `PutReg d0,list(3)` or `PutReg d0,pos\x`.
The store follows its type: a Byte gets the low byte of the register, a
Word the low word, every other type all 32 bits. A Quick or Float
variable gets those bits as they are, without conversion: a 1 in the
register is not the Quick 1, which is `$10000`. Do not use `PutReg` with
a string variable: the compiler accepts it, but the register overwrites
the variable's reference to its text.

## Instructions

The assembler knows the instructions of the 68000 and no others:

{{< details title="The instructions" closed="true" >}}
`ABCD` `ADD` `ADDA` `ADDI` `ADDQ` `ADDX` `AND` `ANDI` `ASL` `ASR`
`BCC` `BCS` `BEQ` `BGE` `BGT` `BHI` `BLE` `BLS` `BLT` `BMI` `BNE` `BPL`
`BVC` `BVS` `BCHG` `BCLR` `BSET` `BTST` `BRA` `BSR` `CHK` `CLR` `CMP`
`CMPA` `CMPI` `CMPM` `DBCC` `DBCS` `DBEQ` `DBGE` `DBGT` `DBHI` `DBLE`
`DBLS` `DBLT` `DBMI` `DBNE` `DBPL` `DBVC` `DBVS` `DBF` `DBRA` `DBT`
`DIVS` `DIVU` `EOR` `EORI` `EXG` `EXT` `ILLEGAL` `JMP` `JSR` `LEA`
`LINK` `LSL` `LSR` `MOVE` `MOVEA` `MOVEM` `MOVEP` `MOVEQ` `MULS` `MULU`
`NBCD` `NEG` `NEGX` `NOP` `NOT` `OR` `ORI` `PEA` `RESET` `ROL` `ROR`
`ROXL` `ROXR` `RTE` `RTR` `RTS` `SBCD` `SCC` `SCS` `SEQ` `SGE` `SGT`
`SHI` `SLE` `SLS` `SLT` `SMI` `SNE` `SPL` `SVC` `SVS` `ST` `SF` `STOP`
`SUB` `SUBA` `SUBI` `SUBQ` `SUBX` `SWAP` `TAS` `TRAP` `TRAPV` `TST`
`UNLK`
{{< /details >}}

The other names some assemblers have for the same conditions, such as
`BHS` and `BLO`, are not among them: write `BCC` and `BCS`. Like every
word the compiler knows, an instruction can be typed in any case; the
editor shows it in capitals.

The size follows the instruction as `.b`, `.w` or `.l`; without one, an
instruction works on a word. `.s` is the same as `.b`. A size the
instruction does not have, such as `MULU.l`, is the compile error
"Illegal Assembler Instruction Size".

`ADD`, `SUB`, `CMP`, `AND`, `OR`, `EOR` and `MOVE` choose their special
forms themselves: with an immediate source `ADD` becomes `ADDI`, with an
address register as destination `ADDA`, and so on. `MOVEQ`, `ADDQ` and
`SUBQ` must be written as such.

## Operands

An operand is written in one of the 68000's addressing modes:

| Form | Example | Meaning |
|---|---|---|
| `Dn` | `d0` | data register d0 to d7 |
| `An` | `a0`, `sp` | address register a0 to a7; `sp` is a7 |
| `(An)` | `(a0)` | the memory a0 points to |
| `(An)+` | `(a0)+` | the same, then a0 is moved on |
| `-(An)` | `-(a0)` | a0 is moved back, then the memory it points to |
| `d(An)` | `8(a0)` | a0 plus a displacement of -32768 to 32767 |
| `d(An,Xn)` | `4(a0,d1.l)` | a0 plus a displacement of -128 to 127 plus an index register, as `.w` (the default) or `.l` |
| `d(PC)` | `table(pc)` | a label, reached relative to the instruction |
| `d(PC,Xn)` | `table(pc,d0.w)` | the same with an index register |
| address | `table`, `4` | an absolute address, a label or a number, as a long |
| `#value` | `#10` | an immediate value |
| register list | `d0-d3/a0` | the registers of `MOVEM` |
| `sr`, `ccr`, `usp` | `MOVE d0,ccr` | the status register, its low byte, the user stack pointer |

`sp` is accepted only as a register on its own; inside brackets write
`a7`, as in `-(a7)`. An absolute address is always a long: the short
form with `.w` or `.l` after the address is a "Syntax Error".

A value in an operand is a constant expression: numbers, constants and
labels with the operators of Basic, and character constants of up to four
characters in quotes, such as `#"ABCD"`. A name stands for a label, and
its value is the label's address. Because constants and labels share their
names, a constant can be written with or without its `#`: after
`#width=320`, both `MOVE #width,d0` and `MOVE ##width,d0` load 320. A
name that is neither, such as the name of a variable, is the compile error
"Constant Not Found"; the `?` in front of a label is for Basic
expressions only and is a "Syntax Error" in an operand.

The compiler works the values out at the end of the compile, so a label
may stand further down the source. Values that do not fit are compile
errors: a displacement out of its range is "Illegal Displacement", and
so is a `MOVEQ` value outside -128 to 127; an `ADDQ`, `SUBQ` or shift
count outside 1 to 8 is "Illegal Immediate Value", a `TRAP` number above
15 "Illegal Trap Vector". An immediate value of a byte or word
instruction is not checked: its low byte or word is used.

## Labels and branches

A label in assembler is an ordinary label of the program. `BRA`, `BSR`,
the `Bcc` and the `DBcc` instructions branch to a label, and `JMP`,
`JSR`, `LEA` and `PEA` take one as an address:

```
MOVEQ #5,d0
BSR addone
PutReg d0,v.l
NPrint v                 ; 6
End

addone:
ADDQ #1,d0
RTS
```

A label must not be a word the editor knows: a label `sub` becomes the
instruction `SUB`, and the line gives an error.

A branch is a word branch of 4 bytes. With the size `.s` or `.b` it is a
short branch of 2 bytes, which reaches 128 bytes back and 126 bytes
ahead. The short branch is an addition of the port; the
[Extensions]({{< relref "/docs/compared-with-original/extensions#short-branches-in-inline-assembler" >}})
page has the details.

**Local labels** keep the names of small loops apart. A label that
begins with an apostrophe belongs to the last ordinary label before it,
and an operand that begins with an apostrophe means that local label:

```
Dim buf.w(9)
.fill
GetReg a0,&buf(0)
MOVEQ #9,d1
'loop
MOVE.w d1,(a0)+
DBRA d1,'loop
NPrint buf(0)," ",buf(9)   ; 9 0
```

`'loop` here is `fill'loop`, so another part of the program can have a
`'loop` of its own under another label. A local label with no ordinary
label before it is the compile error "Illegal Local Name".

## Data: Dc, Ds and Dcb

These statements put data into the program at the place where they
stand. The size is `.b`, `.w` or `.l`; without one it is `.w`.

* `Dc` writes values: `Dc.w 10,20,30`. A value is a constant expression
  or a label, whose address `Dc.l` writes. `Dc.b` also takes text in
  quotes, as the bytes of its characters: `Dc.b "Name",0`. The
  backslash is an ordinary character there.
* `Ds` reserves room: `Ds.l 4` is 16 bytes, all 0.
* `Dcb` writes one value several times: `Dcb.w 8,-1` is eight words of
  -1. A count of 0 writes nothing; a negative count is the compile error
  "Illegal Constant".

`Dc.w`, `Dc.l`, `Dcb.w` and `Dcb.l` first move to an even address, as a
word or long of the 68000 needs. After byte data, `Even` moves to the
next even address, `Even4` to the next multiple of 4 and `Even8` to the
next multiple of 8, with bytes of 0 in between. A label for the aligned
data goes after the `Even`, not before it.

The program runs through these lines as through any other: data in the
middle of the program would be executed as instructions. Put the data
after `End`, or jump over it, and reach it with its label:

```
Goto start
.table
Dc.w 10,20,30
.start
NPrint Peek.w(?table+2)  ; 20
```

`IncBin` puts the bytes of a whole file into the program in the same
way ([Include files]({{< relref "directives#include-files" >}})).

## Calling library routines

`TokeJsr command[,form]` calls the routine of a library command directly.
The command is given by its name or by its token number, which is the
library number times 128 plus the command's place in the library. A
command with several forms has one routine per form; *form* chooses one,
counted from 0. The routine takes its arguments in d0, d1 and so on, as
the compiler would put them there, and returns its result in d0
([How arguments arrive]({{< relref "/docs/blitz2-internals/library-anatomy#how-arguments-arrive" >}})):

```
MOVEQ #0,d0              ; port 0
TokeJsr Joyb
PutReg d0,b.l            ; the buttons pressed on port 0
NPrint b
```

The library of the command goes into the program as if the command were
used. After `BLITZ` the call goes to the routine's Blitz mode version, if
the command has one. A number that is no command is the compile error
"Token Not Found", with the number; a built-in word of the compiler, or a
value that is not a whole number from 0 to 65535, is "Illegal TokeJsr
token number".

`SysJsr number` calls a routine of the runtime libraries, the libraries
without commands that the compiler calls by itself, by its internal
number. These numbers are not documented for programs; `SysJsr` is meant
for writing libraries.

`ALibJsr command[,form]` and `BLibJsr command[,form]` are for the source
of a Blitz library only. They leave a call to the routine of another
library's command that the compiler completes when it copies both
libraries into a program: `ALibJsr` to the Amiga mode version of the
routine, `BLibJsr` to the Blitz mode version. In a program, nothing
completes the call, and it leads nowhere. The libraries of the port are
written in C and make these calls with a macro
([Calling other libraries]({{< relref "/docs/blitz2-internals/c-libraries#calling-other-libraries" >}})).

## In AB3 mode

In [AB3 mode]({{< relref "/docs/ab3-mode" >}}) the inline assembler
works as described here, with two differences. A branch with `.s` or
`.b` stays a word branch, as in AmiBlitz3
([Other additions]({{< relref "/docs/ab3-compared/language#other-additions" >}})).
Text in `Dc.b` has the date and version escapes of AmiBlitz3
([Escapes in strings]({{< relref "/docs/ab3-compared/language#escapes-in-strings" >}})).
As in BB2 mode, only the 68000's instructions are assembled, not
those of the 68020 that AmiBlitz3 knows
([68020 code]({{< relref "/docs/ab3-compared/not-supported#68020-code" >}})).

Based on the Blitz Basic 2 manual by Acid Software. The manual says that
`GetReg` should only be used with data registers; the compiler loads
address registers as well.
