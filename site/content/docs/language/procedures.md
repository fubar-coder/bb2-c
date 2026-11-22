---
title: Procedures
weight: 5
description: Statements and functions of your own - parameters, return values, local and shared variables, recursion, and procedures in assembler.
---

Before this page you should know the types and how variables get them
([Types, variables and constants]({{< relref "types" >}})).

A procedure is a part of a program with a name, which other parts call
with values, its parameters. Blitz Basic 2 has two kinds: a **Statement**
does something and returns nothing, a **Function** returns a value and
is used in expressions. Both have their own variables, so a procedure can
be written and moved to another program without looking at the variable
names around it.

## Statements

A statement is defined between `Statement name{parameters}` and
`End Statement`, and called with its name and the values in curly
brackets:

```
Statement fact{n}
  a=1
  For k=2 To n : a=a*k : Next
  NPrint a
End Statement

For k=1 To 5
  fact{k}
Next
```

The curly brackets are needed also when there are no parameters:
`Statement title{}` is called as `title{}`. `Statement Return` leaves the
statement early.

## Functions

A function is defined between `Function name{parameters}` and
`End Function`, and returns its value with `Function Return`:

```
Function.l fact{n.l}
  If n>1 Then n=n*fact{n-1}
  Function Return n
End Function

NPrint fact{10}          ; 3628800
x=fact{3}+fact{4}
```

The suffix after `Function` gives the type of the result: `Function.l`,
`Function.f`, `Function$` for a string, and so on. Without one, the
result has the default type, normally Quick. The type must be one of the
six types, not a NewType.

`Function Return` in a statement, or `Statement Return` in a function, is
the compile error "Illegal Procedure return", and an `End Statement` that
closes a function, or the other way round, is an error as well
([The end of a procedure of the other kind]({{< relref "/docs/compared-with-original/new-errors#end-of-procedure" >}})).

## Parameters

A procedure has up to six parameters; a seventh is the compile error "Too
many parameters". Each parameter is a variable of the procedure, with
the type of its suffix or the default type. A parameter can be a number,
a string or a pointer, not a NewType: `Statement s{a.Point}` is the
compile error "Illegal Parameter Type". To hand over a NewType value,
pass its address to a pointer parameter:

```
NEWTYPE .Point
  x.w:y
End NEWTYPE
Statement nudge{*p.Point}
  *p\x+1
End Statement
DEFTYPE .Point here
nudge{&here}
NPrint here\x            ; 1
```

Parameters are passed by value: the procedure gets a copy, and a change
of the parameter inside it does not change the caller's variable. The
values are converted to the type of the parameter, as in an assignment.

A procedure must be defined in the source before it is called; a call
before the definition is the compile error "Procedure not found".

## Local and shared variables

Every variable used inside a procedure is a local variable of the
procedure: it is separate from a variable of the same name in the main
program, and it starts at 0 or empty on every call. Each call gets its
own set, so a procedure can call itself, as `fact` above does.

To use a variable of the main program, a global variable, name it in a
`Shared` statement in the procedure:

```
Statement count{}
  Shared total
  total+1
End Statement
```

Arrays are shared the same way, with empty brackets: `Shared arr()`.

The labels inside a procedure belong to it: a `Goto` or `Gosub` from a
procedure to a label outside it, or from outside to a label inside, is
the compile error "Label reference out of context".

## Procedures in assembler

A procedure can consist of
[inline assembler]({{< relref "inline-assembler" >}}). The parameters arrive in the
data registers d0 to d5, in their order; a function returns the value in
d0. `AsmExit` leaves such a procedure, in place of `Statement Return` or
`Function Return`:

```
Function.l plus{a.l,b.l}
  ADD.l d1,d0
  AsmExit
End Function

NPrint plus{3,4}         ; 7
```

The address registers a4 to a6 must have their old values when the
procedure ends: the program keeps its local variables at a4, its global
variables at a5 and a library base in a6.

Based on the Blitz Basic 2 manual by Acid Software.
