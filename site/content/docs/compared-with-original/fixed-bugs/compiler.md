---
title: Compiling
weight: 1
description: Fixed bugs of Blitz Basic 2 in what the compiler makes of a source, with examples of the ones ordinary programs meet.
---

These bugs of the original compiler changed what a source compiled to: a
wrong result, a program that crashed, or an error message where none
belonged. The first sections show the ones an ordinary program can run
into, each with a short program. The other fixes follow as a list, one
sentence each with a link to its issue.

## Operators

### `=<` meant "not equal" {#less-or-equal}

Blitz Basic 2 has two spellings for each comparison that includes
equality: `<=` and `=<` for less-or-equal, `>=` and `=>` for
greater-or-equal. Every one of them worked except `=<`, which the
original compiled as "not equal", without any message.

**What happened**

```
a=1:b=2:NPrint a=<b
a=2:b=2:NPrint a=<b
a=3:b=2:NPrint a=<b
x.f=1.5:y.f=1.5:NPrint x=<y
NPrint 3=<2
For i=1 To 3
  If i=<2 Then NPrint "le ",i Else NPrint "gt ",i
Next
End
```

The original prints the results of `<>`: true (-1) where the two values
differ, false (0) where they are equal.

```
-1
0
-1
0
-1
le 1
gt 2
le 3
```

**With the port**

`=<` is less-or-equal, as `<=`:

```
-1
-1
0
-1
0
le 1
le 2
gt 3
```

**What to change**

Nothing, if `=<` was meant as less-or-equal: the program now does what
its source says. A program that only worked because of the bug, with
`=<` where it needed "not equal", has to write `<>` there
([Sources that compile differently]({{< relref "/docs/compared-with-original/new-errors#less-or-equal" >}})).

Issue: [The operator =< meant "not equal" (#9)](https://github.com/fubar-coder/bb2-c/issues/9).

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

A source that the original compiled and the port refuses for this reason
is described with the others in
[Sources that compile differently]({{< relref "/docs/compared-with-original/new-errors#end-of-procedure" >}}).

Issue: [End Statement could close a Function, and the other way round (#940)](https://github.com/fubar-coder/bb2-c/issues/940).

## Ending a program

### A program with the Blitz OS commands hung at its end {#os-commands-end}

The Blitz OS commands, `InitPage` and the others, draw with the shapes
library. A program that uses them, but no shapes command of its own, did
not get the memory for its shapes: at `End` the program then freed 100
shapes from address 0, which corrupts the system's memory. The program
did not even have to run one of these commands; using one anywhere in the
source was enough.

**What happened**

```
NPrint "x"
End
InitPage 0,0,2
```

The original prints

```
x
```

and then the system hangs, although `InitPage` behind `End` is never
reached. A shapes command somewhere in the program, for example
`LoadShape 0,"x"` behind `End`, avoided the hang.

**With the port**

The memory for the shapes is reserved. The program prints `x` and ends
normally, with return code 0.

**What to change**

Nothing. A shapes command that a program contains only to avoid the hang
is no longer needed and can be removed.

Issue: [A program could hang the system at its end when it used a library that needs objects only through another library (#335)](https://github.com/fubar-coder/bb2-c/issues/335).

## Other fixes

### Expressions and statements

* A number ending in a decimal point, as in `a.f=1.`, was a syntax error;
  `1.` is the float 1 ([#10](https://github.com/fubar-coder/bb2-c/issues/10)).
* An exponent after a fraction of 0 was ignored, so `1.0e5` gave "Garbage
  at End of Line"; it is 100000
  ([#11](https://github.com/fubar-coder/bb2-c/issues/11)).
* A string addition from the seventh argument of a library command on,
  such as the title of a `Window`, got a wrong length; the length is right
  ([#971](https://github.com/fubar-coder/bb2-c/issues/971)).
* A `~` inside a string literal started a constant expression, so
  `"a~1+2~b"` became `a3b`; inside strings `~` is an ordinary character
  ([#966](https://github.com/fubar-coder/bb2-c/issues/966)).
* `For a$=… To …` with a global string variable compiled and treated the
  string as a number; it gives "Bad Type for For...Next", as with a local
  string variable ([#39](https://github.com/fubar-coder/bb2-c/issues/39)).
* `Dcb` with a count of 0 made 2^32 copies and ran for hours; a count of 0
  emits nothing, a negative count gives "Illegal Constant"
  ([#37](https://github.com/fubar-coder/bb2-c/issues/37)).

### Names, labels and constants

* A constant defined without a value, a line `#c` alone, held a random
  value; its value is 0
  ([#972](https://github.com/fubar-coder/bb2-c/issues/972)).
* A constant definition without a name, `#=5`, was accepted and could make
  the compiler search 65536 bytes of memory; it gives "Syntax Error"
  ([#7](https://github.com/fubar-coder/bb2-c/issues/7)).
* A `#`, `Goto`, `Gosub`, `Restore`, `?` or `.` without a name made the
  compiler compare 65536 bytes of memory for each search; it gives "Syntax
  Error" ([#30](https://github.com/fubar-coder/bb2-c/issues/30)).
* A name of more than 127 characters, possible through a macro, overwrote
  the compiler's data; it gives "Syntax Error"
  ([#29](https://github.com/fubar-coder/bb2-c/issues/29)).
* A jump to a label with the name of a constant of value 0 gave a nonsense
  error at a line that does not exist, or read and wrote random memory; it
  gives "Label has been used as a Constant", as for any other value
  ([#977](https://github.com/fubar-coder/bb2-c/issues/977)).

### Arrays and NewTypes

* Arrays with 256-byte elements, also List arrays of a 248-byte NewType
  and field arrays `x.t[n]`, were indexed with the wrong element size, so
  element 1 of an array lay 64 KB behind element 0, outside the array,
  and a field array always gave its first element; the elements are 256
  bytes apart
  ([#15](https://github.com/fubar-coder/bb2-c/issues/15)).
* A List array indexed by an element of a normal array, as in `a(b(1))`,
  or the other way round, used the other array's element size and reached
  the wrong element; each array uses its own size
  ([#937](https://github.com/fubar-coder/bb2-c/issues/937)).
* A NewType that contained a field of its own type got a random size; the
  compiler reports "Illegal Type", and a pointer to the own type stays
  allowed ([#973](https://github.com/fubar-coder/bb2-c/issues/973)).
* A NewType field of a type defined later got a wrong size, so the fields
  behind it and the size of the NewType were wrong; the NewType is laid out
  again when the type is defined, and using it too early gives "Type not
  yet defined" ([#974](https://github.com/fubar-coder/bb2-c/issues/974)).

### Include files and paths

* The path of `IncDir` stayed for the following compiles of the session,
  so another source failed with "Can't open Include"; every compile
  starts without an `IncDir` path
  ([#342](https://github.com/fubar-coder/bb2-c/issues/342)).
* After an `IncDir`, a file name with a device or assign, such as
  `Include "blitzlibs:libnums.bb"`, got the `IncDir` path in front of it and
  could not be opened; such a name is used as written, except in AB3 mode
  ([#341](https://github.com/fubar-coder/bb2-c/issues/341)).
* Text after a quoted path, as in `IncDir "dh0:inc/" : a=1`, became part
  of the path and was never compiled; a quoted path or name ends at its
  closing quote ([#17](https://github.com/fubar-coder/bb2-c/issues/17)).
* An `IncDir` path of 96 characters or more overwrote the compiler's data,
  and the program crashed when it ran; it gives "Path too Long"
  ([#16](https://github.com/fubar-coder/bb2-c/issues/16)).
* An include file name of more than 127 characters together with the
  `IncDir` path overwrote the compiler's data; it gives "Path too Long"
  ([#325](https://github.com/fubar-coder/bb2-c/issues/325)).
* A `UsePath` path of 128 characters or more overwrote the compiler's own
  code; it gives "Path too Long"
  ([#31](https://github.com/fubar-coder/bb2-c/issues/31)).
* `IncBin` kept a file or directory in use after an error; it is closed
  ([#38](https://github.com/fubar-coder/bb2-c/issues/38)).

### Macros

* The Macro Buffer option had no effect, and blitz2 could crash when it
  ended if a source set another size; the option sets the largest macro
  text ([#336](https://github.com/fubar-coder/bb2-c/issues/336)).
* A macro line ending in `End` cut off the rest of the macro and of the
  line that called it; the rest is kept, as in AmiBlitz3
  ([#968](https://github.com/fubar-coder/bb2-c/issues/968)).
* A one-line macro with a colon right after its name, `Macro a:1:End
  Macro`, expanded to a text starting with a colon; `!a` expands to `1`
  ([#942](https://github.com/fubar-coder/bb2-c/issues/942)).
* An error inside a macro definition could crash the compiler; the error
  is shown ([#965](https://github.com/fubar-coder/bb2-c/issues/965)).
* A macro text of 65536 bytes or more was cut and part of its memory lost
  until a reboot; it gives "Macro too Big"
  ([#964](https://github.com/fubar-coder/bb2-c/issues/964)).

### Residents

* The last constant of a resident pointed behind the resident, so every
  compile freed memory that did not belong to it; the list ends where it
  should ([#34](https://github.com/fubar-coder/bb2-c/issues/34)).
* Resident files contained bytes nobody had set, so CREATE RESIDENT gave a
  different file each time; those bytes are zero
  ([#970](https://github.com/fubar-coder/bb2-c/issues/970)).
* A resident with a `NewType` without fields lost the types, macros and
  constants behind it when it was loaded; everything is loaded as defined
  ([#35](https://github.com/fubar-coder/bb2-c/issues/35)).
* A resident with a macro text of 32767 bytes or more loaded the
  definitions behind it from the wrong place; it is written and loaded
  correctly ([#995](https://github.com/fubar-coder/bb2-c/issues/995)).

### The executable

* A program ending with an `IncBin` or `Dc.b` of odd length crashed at its
  `End` with an address error; the end code starts at an even address
  ([#979](https://github.com/fubar-coder/bb2-c/issues/979)).
* Calls of AmigaOS functions with an argument in register a4 or a5 crashed
  or destroyed the program's variables; the compiler saves both registers
  around such a call ([#958](https://github.com/fubar-coder/bb2-c/issues/958)).
* Very large executables, with very many relocations, did not load on
  AmigaOS 2.0 and later; their relocation table is split into groups
  AmigaOS accepts ([#943](https://github.com/fubar-coder/bb2-c/issues/943)).
* Pad bytes, from `Even`, from the alignment of data and code and from
  `Ds`, held random values; they are zero
  ([#969](https://github.com/fubar-coder/bb2-c/issues/969)).
* On a 68010 processor, runtime errors and the debugger took the trap
  vectors from the wrong table when the system had moved it; the moved
  table is used ([#42](https://github.com/fubar-coder/bb2-c/issues/42)).

### Inline assembler

* After a line with a calculation on a character constant, such as
  `MOVE.w #">"*8,d0`, a `#" "` on the lines before it could become 0;
  character constants are read correctly on every line
  ([#343](https://github.com/fubar-coder/bb2-c/issues/343)).
* An inline assembler expression of 240 characters or more in a macro or
  include was freed with a wrong size; it is freed correctly
  ([#1](https://github.com/fubar-coder/bb2-c/issues/1)).

### The compiler's memory

* After the object buffer ran full during an assignment or a `Select`,
  the compiler lost a little memory; the buffer is freed in every case
  ([#14](https://github.com/fubar-coder/bb2-c/issues/14)).
* A compile error in some assignments or in `Select` lost a little
  memory; the buffer is freed at the end of every pass
  ([#994](https://github.com/fubar-coder/bb2-c/issues/994)).
