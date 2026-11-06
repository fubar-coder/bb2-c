---
title: Extensions
weight: 3
description: What the compiler of the BB2 C port accepts beyond Blitz Basic 2, and the rule that keeps old sources unchanged.
---

Before this page you should know how the compiler options work
([The editor and the compiler]({{< relref "/docs/how-blitz2-works/editor-and-compiler" >}})).

The compiler of the BB2 C port follows one rule for everything it adds:
an extension may only give a meaning to a source that the original
refuses. A source that compiles with Blitz Basic 2 compiles to the same
program with the port, extensions or not.

## #__debug: is runtime error checking on?

`#__debug` is a constant that the compiler defines itself. Its value is
−1 when the program is compiled with runtime error checking (the option
**Runtime Error Debugger**) and 0 without it. The original compiler does
not know it and reports "Constant Not Found".

Its use is code that should exist only in a program compiled for
debugging. Put it between `CNIF` and `CEND`:

```
CNIF #__debug
  NPrint "only with runtime error checking"
CEND
```

Compiled with runtime error checking, the program prints the line;
compiled without it, the line is not in the program at all.

Some details:

* `#__debug` can be used wherever a constant can: in expressions, in
  `CNIF`, in the definition of other constants and as an operand of the
  inline assembler.
* Its value comes from the option at the start of the compile. A
  `RunErrsOff` in the source does not change it.
* A source that defines `#__debug` itself, with `#__debug=...`, uses its
  own value instead.
* `Stop` keeps its behaviour: without runtime error checking the compiler
  reports "Debugger must be enabled for STOP". A program that is compiled
  both ways can put its `Stop` between `CNIF #__debug` and `CEND`.

The name and the values are those of AmiBlitz3, so a source that uses
`#__debug` means the same with both. blitz2-ab3 knows it in both of its
modes ([AB3 mode]({{< relref "/docs/ab3-mode" >}})).
