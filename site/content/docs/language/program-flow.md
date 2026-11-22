---
title: Program flow
weight: 4
description: If, While, Repeat, For and Select, Goto and Gosub with labels, leaving a loop with Pop, ending a program, interrupts and error handlers.
---

Before this page you should know how expressions give truth values
([Expressions and operators]({{< relref "expressions#truth-values" >}})).

A program runs from its first line to its last, statement after
statement. The statements on this page change that order: they decide,
repeat and jump. Each of them takes a condition or a value in the form of
an expression; a condition is true when its value is not 0.

## If

`If` has two forms. With `Then`, the rest of the line is the part that
runs when the condition is true, all its statements up to the end of the
line or up to an `Else`:

```
If a=5 Then NPrint "five" : b=1           ; both only if a=5
If a=5 Then NPrint "five" Else NPrint "not five"
```

Without `Then`, the part reaches to the next `EndIf` (also written `End
If`), and an `Else` between them starts the part for a false condition:

```
If a=5
  NPrint "five"
Else
  NPrint "not five"
EndIf
```

The block form also works on one line, with colons:
`If a=5 : NPrint "five" : EndIf`. An `If` without its `EndIf` is the
compile error "If Without End If". There is no `ElseIf`: put another `If`
into the `Else` part.

## Loops

**While** repeats its part as long as the condition is true, and tests it
first, so the part may not run at all. The part ends with `Wend`, also
written `End While`:

```
While a<20
  NPrint a
  a+1
Wend
```

**Repeat** runs its part first and tests afterwards: `Until` ends the loop
when its condition is true, `Forever` never ends it. A `Repeat` part
always runs at least once.

```
Repeat
  a+1
Until a>=20
```

**For** counts a variable from a start value to an end value, by 1 or by
the value after `Step`:

```
For i=1 To 10 : NPrint i : Next
For i=10 To 1 Step -1 : NPrint i : Next
For x.q=0 To 1 Step 0.25 : NPrint x : Next
```

The end value and the step are computed once, when the loop starts; a
change of a variable in them does not change the loop. The test comes
before each pass, so `For i=3 To 1` does not run at all. After the loop,
the variable holds the first value that failed the test (11 after `For
i=1 To 10`). The variable can be of any number type, not a string.

`Next` ends the innermost loop; with the name of its variable,
`Next i`, the compiler checks that it is the right one, and `Next j,i`
ends two loops at once. Two nested loops over the same variable are the
compile error "Duplicate For...Next Error".

## Select

`Select` computes a value once, and each `Case` after it compares it with
its own value. The part of the first `Case` that is equal runs, up to the
next `Case`, `Default` or `End Select`, and the program goes on after
`End Select`. The part after `Default` runs when no `Case` was equal:

```
Select key
  Case 1
    NPrint "one"
  Case 2
    NPrint "two"
  Default
    NPrint "something else"
End Select
```

A `Case` takes one value, which may be an expression (`Case n+1`); a list
of values, such as `Case 1,2`, is the compile error "Garbage at End of
Line". The values are compared in the type of the `Select` expression, and
strings can be selected as well (`Select a$` with `Case "yes"`). Only the
first matching `Case` runs, even if a later one has the same value.

## Labels, Goto and Gosub

`Goto label` jumps to a label and goes on from there.
`Gosub label` jumps the same way but remembers where it came from; a
`Return` there goes back to the statement after the `Gosub`. The part
between the label and its `Return` is called a subroutine:

```
Gosub message
Gosub message
End

message:
  NPrint "Hello"
  Return
```

How labels are written is on the page
[The language]({{< relref "/docs/language#a-program-line-by-line" >}}).
A label can be used before it is defined, further down the source.

`On n Goto a,b,c` and `On n Gosub a,b,c` jump to the first label when the
value is 1, to the second when it is 2, and so on. With a value below 1 or
above the number of labels, the program goes on after the `On` statement
without a jump.

## Leaving a loop: Pop

A `Gosub`, a `For` loop and a `Select` keep data on the stack until their
`Return`, `Next` or `End Select`. A program that leaves one of them with
a `Goto` first removes that data with `Pop`:

```
For i=1 To 100
  If a(i)=0 Then Pop For : Goto found
Next
found:
```

`Pop Gosub`, `Pop For` and `Pop Select` are needed in this case. `Pop If`,
`Pop While` and `Pop Repeat` are accepted as well, but do nothing: these
statements keep nothing on the stack.

## Ending a program

`End` ends the program, at its last line or anywhere in it. The program
returns to the editor when it was run from there, and to the Shell or the
Workbench as an executable; the libraries clean up in either case
([Libraries in a compiled program]({{< relref "/docs/libraries/in-a-program" >}})).
The word `End` is also the first half of the endings `End If`,
`End While`, `End Select`, `End Statement`, `End Function`,
`End NEWTYPE`, `End Macro`, `End SetInt` and `End SetErr`.

`Stop` interrupts the program in the debugger, as a breakpoint; it needs
runtime error checking, without it the compiler reports "Debugger must
be enabled for STOP" ([Runtime error checking and the debugger]({{< relref "/docs/how-blitz2-works/editor-and-compiler#runtime-error-checking-and-the-debugger" >}})).

## Interrupts: SetInt

`SetInt n` up to `End SetInt` is code that runs on the Amiga's interrupt
number n while the rest of the program goes on; it is not run where it
stands in the source. The most used one is 5, the vertical blank, which
comes once per frame, 50 times a second on a PAL Amiga:

```
SetInt 5
  ticks+1
End SetInt
```

`ClrInt n` removes the code from interrupt n again. The numbers, as the
Blitz Basic 2 manual lists them:

| n | Interrupt |
|---|---|
| 0 | serial transmit buffer empty |
| 1 | disk block read or written |
| 2 | software interrupt |
| 3 | CIA ports |
| 4 | copper |
| 5 | vertical blank |
| 6 | blitter finished |
| 7 to 10 | audio channel 0 to 3 |
| 11 | serial receive buffer full |
| 12 | disk sync |
| 13 | external |

The code of an interrupt must be short, and must not use strings. In
Amiga mode it must also not use the blitter, Intuition or files.

## Error handlers: SetErr

`SetErr` up to `End SetErr` is code that runs instead of the debugger's
report when a runtime error happens. It needs runtime error checking,
which finds the errors. A handler usually ends the program or jumps to a
safe place; `ErrFail` inside it reports the error as usual after all, and
`ClrErr` removes the handler:

```
SetErr
  NPrint "Something went wrong"
  End
End SetErr
```

Based on the Blitz Basic 2 manual by Acid Software.
