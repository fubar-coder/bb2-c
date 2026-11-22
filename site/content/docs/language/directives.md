---
title: Directives and macros
weight: 6
description: Include files and binary data, conditional compiling with CNIF and CSIF, macros with parameters, constant text with ~, and the statements that set how a program is compiled and started.
---

Before this page you should know constants
([Types, variables and constants]({{< relref "types#constants" >}})).

A directive is a statement for the compiler, not for the program: it acts
while the source is compiled, from top to bottom, and the program never
sees it. This page describes the directives that bring in other files,
compile parts of a source only on a condition, and expand macros, and the
few statements with which a source sets how it is compiled and started.

## Include files

`INCLUDE` compiles another source file at this place, as if its lines
stood here. The file must be a tokenised source, as the editor saves it.
`XINCLUDE` does the same, but only once: a later `XINCLUDE` of the same
file is ignored, which lets several parts of a program each name the
files they need. `IncBin` puts the bytes of any file into the program at
this place, for data the program reads through a label:

```
INCDIR "work:mygame/"
XINCLUDE "maths.bb2"
INCLUDE "levels.bb2"
...
End
pic: IncBin "title.raw"     ; ?pic is the address of the bytes
```

`INCDIR` sets a path that is put in front of the file names of
`INCLUDE`, `XINCLUDE` and `IncBin`; a name with a device of its own, such
as `"dh0:inc/a.bb2"`, is used as it is. The names can be written with or
without quotes; with quotes they are safe from the editor's
tokenising, and only a comment may follow them
([Text after a quoted path]({{< relref "/docs/compared-with-original/new-errors#text-after-path" >}})).
A path that is too long is the compile error "Path too Long"
([Paths that are too long]({{< relref "/docs/compared-with-original/new-errors#path-too-long" >}})).

Precompiled NewTypes, macros and constants come from residents, which the
compiler options load before the source
([Residents and the .xtra file]({{< relref "/docs/how-blitz2-works/residents" >}})).

## Conditional compiling

`CNIF` compares two constant values. When the comparison is true, the
lines after it are compiled; when it is false, they are left out of the
program, up to the matching `CEND`. `CELSE` between them switches to the
other part:

```
#demo=1
CNIF #demo=1
  NPrint "Demonstration version"
CELSE
  NPrint "Full version"
CEND
```

The comparison takes `=`, `<>`, `<`, `>`, `<=` and `>=`, and both sides
must be constants or numbers: a variable is the compile error "Illegal
Constant Expression". `CSIF` compares two strings in quotes the same way,
which is useful for the parameters of macros (below). `CNIF` and `CSIF`
blocks can be nested.

Unlike an `If` in the program, the part that is left out does not exist
in the executable at all. A typical use is a debugging part that is
compiled only with runtime error checking, with the constant `#__debug`
([#__debug]({{< relref "/docs/compared-with-original/extensions#__debug-is-runtime-error-checking-on" >}})):

```
CNIF #__debug
  NPrint "x=",x
CEND
```

`CERR "text"` stops the compile with the text as error message. It is
meant for conditional parts and macros that find something wrong.

## Macros

A macro is a piece of source text with a name. It is defined between
`Macro name` and `End Macro`, and an exclamation mark in front of the
name inserts the text at that place, before the line is compiled:

```
Macro countup
  a=a+1
  NPrint "a is ",a
End Macro

!countup
!countup          ; a is 2
```

A short macro can stand on one line: `Macro one : NPrint "one" : End
Macro`.

**Parameters.** A macro is called with parameters in curly brackets, and
in its text a backquote with a digit stands for them: `` `1 `` for the
first, `` `2 `` for the second, up to `` `9 ``; the letters `` `a ``,
`` `b `` and so on stand for the tenth, the eleventh and the following
ones. `` `0 `` is the number of parameters that were given:

```
Macro distance
  Sqr(`1*`1+`2*`2)
End Macro

NPrint !distance{3,4}     ; Sqr(3*3+4*4): 5
```

A parameter is inserted as text, not as a value. `!distance{x+1,y}`
becomes `Sqr(x+1*x+1+y*y)`, so put brackets around a parameter that is
an expression: `!distance{(x+1),y}`.

**Checks and recursion.** With `CNIF` and `CSIF` a macro can look at its
parameters, and with `CERR` it can refuse them. A macro can also call
itself; here it prints each of its parameters, until the next one is
empty:

```
Macro needtwo
  CNIF `0<>2
    CERR "Needs 2 parameters"
  CEND
  `1=`1+`2
End Macro

Macro printall
  NPrint `1
  CSIF "`2">""
    !printall{`2,`3,`4}
  CEND
End Macro
!printall{18,19,20}
```

**Constant text with `~`.** Between two tilde characters, the compiler
computes a constant expression and puts the number into the line as
text. In a macro this builds names from parameters:

```
var3=30
Macro showvar
  NPrint var~`1~
End Macro
!showvar{2+1}              ; NPrint var3: 30
```

Without the tildes the line would read `NPrint var2+1`. Inside a string,
`~` is an ordinary character.

A macro text has at most the size of the **Macro Buffer** of the
compiler options
([A macro larger than the Macro Buffer]({{< relref "/docs/compared-with-original/new-errors#macro-buffer" >}})).

## Settings in the source

A few statements make a setting for the whole program, or for a part of
it, instead of doing something when they run:

* **`RunErrsOff` and `RunErrsOn`** switch runtime error checking off and
  on again for the lines that follow, within a program compiled with
  runtime error checking. A part that is known to be correct can run at
  full speed this way.
* **`WBStartup`** lets the program be started from the Workbench. It
  does not matter where it stands
  ([What pulls in wbstartuplib]({{< relref "/docs/libraries/in-a-program#what-pulls-in-wbstartuplib" >}})).
* **`CloseEd`** closes the editor's screen while the program runs from
  the editor, which leaves more chip memory to the program. **`NoCli`**
  keeps the window for the program's text output from opening when it
  runs from the editor. Both have no effect on an executable.
* **`BLITZ`, `AMIGA` and `QAMIGA`** switch between the modes of the
  display commands: in Blitz mode the program has the machine to itself,
  without the operating system, in Amiga mode it shares it as every
  program does, and Quick Amiga mode lies in between.
* **`USEPATH`** sets the path for the names that start with a backslash
  ([USEPATH]({{< relref "newtypes-arrays-lists#usepath" >}})).

Based on the Blitz Basic 2 manual by Acid Software.
