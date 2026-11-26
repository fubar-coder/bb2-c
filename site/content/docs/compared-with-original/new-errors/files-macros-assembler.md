---
title: Include files, macros and inline assembler
weight: 3
description: Text after a quoted path, paths that are too long, macro lines ending in End, the Macro Buffer option and inline assembler operands that are too long, where the BB2 C port compiles a source differently from Blitz Basic 2.
---

These sources compiled with Blitz Basic 2, mostly to a wrong result or to
a crash of the compiler or the program. The BB2 C port refuses them or
compiles them to what they say. Each program can be copied; a comment
behind a line, or above it, gives what the original did and what the port
does with that line. A program that the
port compiles is run by the project's tests; one that it refuses is shown
as plain text. The original's results were recorded in an emulator, with
Blitz Basic 2 built from its published source and the libraries of the
Blitz Basic 2 CD.

## Include files and paths

### Text after a quoted path {#text-after-path}

The original took the whole rest of the line as the path of `IncDir`, and
as the file name of `Include`, `XInclude` and `IncBin`, closing quote and
all. In the port a path or name in quotes ends at its closing quote.

```blitzbasic
a=1
IncDir "RAM:" : a=2
NPrint a        ; original: 1    port: 2
End
```

In the original `: a=2` became part of the path, and the assignment was
never compiled. In the port a `:` or `Else` after the path of `IncDir` or
`IncBin` ends the statement as usual. After `Include` and `XInclude` only
a comment may follow:

```
Include "inc.bb2" : a=1
NPrint a
End
; original: "Can't open Include"    port: "Syntax Error", both in line 1
```

The original failed even where `inc.bb2` exists, since the rest of the
line was part of the file name. Other text after the closing quote is a
"Syntax Error" too. A path without quotes still takes the rest of the
line.

**What to change:** put a statement that follows `Include` or `XInclude`
on its own line.

Issue: [Text after an IncDir path became part of the path (#17)](https://github.com/fubar-coder/bb2-c/issues/17).

### Paths that are too long {#path-too-long}

The original copied paths into buffers of a fixed size without a limit.
The port stops with "Path too Long" at the line of the path.

```
; a path of 98 characters
; original: compiles, the program crashes    port: "Path too Long"
IncDir "ppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppppp"
x=5
NPrint x
End
```

The original's program crashed with a "Software Failure" instead of
printing 5. Three limits are checked:

* an `IncDir` path of 96 characters or more, which overwrote the
  compiler's data;
* the `IncDir` path and the name of an `Include`, `XInclude` or `IncBin`
  together, which could overwrite the compiler's data from 128
  characters on;
* a `UsePath` path of 128 characters or more, possible through a macro,
  which overwrote the compiler's own code, so that the compiler did not
  finish.

**What to change:** use a shorter path, for example with an assign.

Issues:
[IncDir path of 96 characters or more (#16)](https://github.com/fubar-coder/bb2-c/issues/16),
[include file name of more than 127 characters (#325)](https://github.com/fubar-coder/bb2-c/issues/325),
[UsePath path of 128 characters or more (#31)](https://github.com/fubar-coder/bb2-c/issues/31).

## Macros

### A macro line ending in `End` {#macro-end-line}

In the original a line of a macro that ended in `End` cut off the rest of
the macro and of the line that called it. The port keeps the rest of the
macro, as AmiBlitz3 does.

```blitzbasic
Macro m
  If `1
    End
  End If
  NPrint "after"
End Macro
!m{0}            ; original: "If Without End If"    port: after
NPrint "done"    ; port: done
End
```

The original lost the `End If` and the rest of the macro, so it stopped
at the call of the macro.

**What to change:** nothing, unless a source relied on the cut; then
remove the lines that the original left out.

Issue: [A macro line ending in End cut off the rest of the macro (#968)](https://github.com/fubar-coder/bb2-c/issues/968).

### A macro larger than the Macro Buffer {#macro-buffer}

The compiler options have a **Macro Buffer** size. In the original it had
no effect: the buffer for macro texts always had its default size of
8192 bytes. In the port the option counts, in both directions.

A source with a macro text of 9120 bytes, whose options set the Macro
Buffer to 16384 bytes, was refused by the original with "Macro too Big"
inside the macro, and blitz2 hung afterwards; the port compiles it. A
source with a macro text of 1520 bytes, whose options set the buffer to
1024 bytes, compiled in the original; the port gives "Macro too Big" at
the line where the macro's text passes the size. A macro text of more
than 65535 bytes is always "Macro too Big".

The buffer gets its new size when a source is loaded or the COMPILER
OPTIONS are closed. If there is not enough memory for it, the compiler
reports out of memory and keeps the old buffer with its size; the next
change of the options tries again.

**What to change:** a source saved with the default size is not affected.
If one of yours fails with "Macro too Big", set its Macro Buffer back to
the default or larger in the COMPILER OPTIONS.

Issues:
[The Macro Buffer option had no effect (#336)](https://github.com/fubar-coder/bb2-c/issues/336),
[A macro text of 65536 bytes or more was cut (#964)](https://github.com/fubar-coder/bb2-c/issues/964).

## Inline assembler

### An operand that is too long {#operand-too-long}

The compiler keeps the text of an inline assembler operand in a buffer of
a fixed size when the text comes from a macro or an include, or contains a
local label. The original copied the text without a limit; the port stops
with "Operand too Long".

```
Macro m
move.l #`1+`1,dat
End Macro
Goto skip
.dat
Dc.l 0
.skip
; a source operand of 127 characters
; original: "Syntax Error"    port: "Operand too Long"
!m{1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1+1}
NPrint Peek.l(?dat)
End
```

There is one buffer for the source operand and `Dc`, and one for the
destination operand right behind it. In the original a source operand of
more than 125 characters reached into the destination's buffer, and the
destination overwrote its end: wrong code or "Syntax Error", as here,
where the program should store 64. Longer texts overwrote the compiler's
data behind the buffers: a source operand or `Dc` of more than 261
characters, a destination operand of more than 133. The port reports
each of these at the line of the instruction, here the call of the
macro. Every text that fits compiles as before, also a source operand of
up to 261 characters when the destination is a register.

**What to change:** split the expression, for example into a constant
defined before the instruction, or use shorter macro parameters.

Issue: [copyconst copies an operand text into namebuff without a length check (#997)](https://github.com/fubar-coder/bb2-c/issues/997).
