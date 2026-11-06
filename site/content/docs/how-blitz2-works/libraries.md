---
title: "Libraries: acidlibs, deflibs, userlibs"
weight: 3
description: The kinds of Blitz libraries, where they come from, and what ends up in your program when you use a command.
---

Before this page you should know what a token is ([Tokenised
sources]({{< relref "/docs/how-blitz2-works/tokens" >}})).

Almost every command of Blitz Basic 2 comes from a library. A Blitz
library is not an AmigaOS library: it is a piece of code with a table of
commands that the compiler copies into your program. This page explains
the kinds of libraries, where blitz2 finds them, and how they end up in
an executable.

## Where blitz2 finds its libraries

At start, blitz2 loads its libraries in this order:

1. `blitz2:acidlibs`, a file with the libraries of Blitz Basic 2 itself:
   the command libraries and the runtime libraries the compiler needs.
2. `blitz2:deflibs`, a file with the declarations of the AmigaOS calls.
3. Every library file in the directories under `blitz2:userlibs/`, for
   example `blitz2:userlibs/mylibs/mylib.obj`. These are libraries you
   add yourself.

`acidlibs` and `deflibs` are *BigLib* files: many library files joined
into one, each with its length in front. A userlib is a single library
file. Inside, both look the same: an AmigaOS executable with one code
hunk, whose first part is the table of the commands (the chapter
[blitz2 internals]({{< relref "/docs/blitz2-internals" >}}) describes
it).

Every library has a number, and no two loaded libraries may have the same
one: the number is part of every token of the library's commands
([Tokenised sources]({{< relref "/docs/how-blitz2-works/tokens" >}})). The BB2 C port checks this when it builds
the library files.

## Four kinds of library

| Kind | Where | Has command names | What goes into your program |
|---|---|---|---|
| Command library | acidlibs, userlibs | yes | the whole library, when you use one of its commands |
| Runtime library | acidlibs | no | the whole library, when the compiler needs it |
| AmigaOS call declarations | deflibs | yes, like `Open_` | only the code that opens and closes the AmigaOS library |
| Built-in words | in blitz2 itself | yes | only the code the compiler generates |

**Command libraries** provide the commands you write: `Print` and
`NPrint` come from one, `Left$` and `Mid$` from another, screens, shapes,
sound and files from others. Some of them manage *objects*, such as
screens, windows or shapes, numbered in your program; for those the
compiler options have **Object Maximums**, the highest number your program
may use.

**Runtime libraries** have no commands. They hold code the compiler calls
by itself: setting up the variables, managing memory, copying and
assigning strings, multiplication and division the 68000 cannot do in
one instruction, floating point, `Read` and `Data`, the end of the
program, and the runtime error checks. You never name them; they are in
your program because the compiler's code uses them.

**AmigaOS call declarations** let a program call the operating system
directly. A command like `Open_` is declared with its AmigaOS library, its
offset and the registers of its arguments; the compiler loads the
arguments into the registers and calls the AmigaOS function itself. Such
a declaration library has almost no code: only the part that opens the
AmigaOS library at the program's start and closes it at the end goes into
the program.

## The whole library, not just the command

The compiler has no linker that could pick single routines. When your
program uses a command of a library for the first time, the compiler
copies the code of the **whole library** into the program. It also copies
every library that library needs. `Print`, for example, needs the
library for console input and output, which needs the AmigaOS DOS library,
and the floating point library for printing numbers. That is why even a
program that never calls the operating system directly contains the code
that opens `dos.library`.

Every library can have a start routine and an end routine. The compiler
collects them: the start routines run before the first line of your
program, the end routines after the last one, also when the program
stops with `End` in the middle. How that looks in the executable is on
the next page, [From source to executable]({{< relref "/docs/how-blitz2-works/compiling" >}}).

## Where the libraries come from

The BB2 C port builds every library of Blitz Basic 2.10 from a C source,
one file per library, and joins them into `acidlibs` and `deflibs` in the
original's order. Each library has the same number, the same commands and
the same declarations as the original, apart from a few deliberate fixes
([Differences from the original]({{< relref "/docs/compared-with-original" >}})),
and the tests compare their behaviour with the original's. How to write such a library yourself is in
[blitz2 internals]({{< relref "/docs/blitz2-internals" >}}).
