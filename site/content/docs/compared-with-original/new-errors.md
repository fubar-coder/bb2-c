---
title: Sources that compile differently
weight: 2
description: Sources that Blitz Basic 2 accepted and the BB2 C port refuses or compiles to something else, and how to change them.
---

Before this page you should know the fixed bugs of the compiler
([Fixed original bugs]({{< relref "/docs/compared-with-original/fixed-bugs" >}})).

Almost every source compiles with the port exactly as with the original.
The exceptions on this page all come from fixed bugs: the original
accepted these sources, but compiled them to a random or wrong result.
If one of your sources is among them, the change below makes it compile
the same way with both compilers.

## A NewType that contains itself

A NewType cannot contain a field of its own type: it would have to be
larger than itself. The original accepted it anyway and gave the field a
random size, so the following fields and the size of the whole type were
random too. The port reports "Illegal Type" at that field:

```
NEWTYPE .node
  value.l
  child.node        ; Illegal Type
End NEWTYPE
```

What such a source usually means is a pointer to another item of the same
type, and a pointer field is allowed:

```
NEWTYPE .node
  value.l
  *child.node       ; a pointer: allowed
End NEWTYPE
```

Issue: [A NewType field of its own type took a random size (#973)](https://github.com/fubar-coder/bb2-c/issues/973).

## A macro larger than the Macro Buffer

The compiler options have a **Macro Buffer** size. In the original it had
no effect: the buffer for macro texts always had its default size,
whatever the option said. In the port the option counts, in both
directions:

* With a **larger** Macro Buffer, a source can define macros longer than
  the default allows. The original refused them with "Macro too Big".
* With a **smaller** Macro Buffer than the default, a macro longer than the
  set size gives "Macro too Big", although the original accepted it.

A source saved with the default size is not affected. If one of yours
fails with "Macro too Big", set its Macro Buffer back to the default or
larger in the COMPILER OPTIONS.
Issue: [The Macro Buffer option had no effect (#336)](https://github.com/fubar-coder/bb2-c/issues/336).

## A constant without a value

A line with only a constant name, without `=` and a value, is accepted by
both compilers. The original never gave such a constant a value: it held
whatever was in memory, so `CNIF #c` could be true or false from one
compile to the next. In the port its value is 0:

```
#c
CNIF #c
  NPrint "never compiled with the port"
CEND
```

If a source relies on such a line, give the constant the value it needs,
`#c=1` for example.
Issue: [A constant defined without a value held a random value (#972)](https://github.com/fubar-coder/bb2-c/issues/972).

## A ~ inside a string

Outside strings, `~expr~` puts the value of a constant expression into the
source: `NPrint ~1+2~` prints `3`. The original also did this inside
string literals, so `"a~1+2~b"` became `a3b`, and a single `~` in a string,
in `Print`, `Dc.b` or `Data$`, gave "Illegal Constant Expression". In the
port a `~` inside quotes is an ordinary character:

```
NPrint "a~1+2~b"     ; prints a~1+2~b
NPrint ~1+2~         ; prints 3, as before
```

A source that used `~...~` inside a string to build a text from constants
has to build that text outside the quotes instead.
Issue: [A ~ inside a string literal started a constant expression (#966)](https://github.com/fubar-coder/bb2-c/issues/966).
