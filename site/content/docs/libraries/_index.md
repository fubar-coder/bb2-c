---
title: Libraries
weight: 3
description: What the libraries of Blitz Basic 2 provide, how to look up a command, and how to call AmigaOS directly.
prev: /docs/how-blitz2-works/residents
next: /docs/compared-with-original
---

Almost every command of Blitz Basic 2 comes from a library
([Libraries: acidlibs, deflibs, userlibs]({{< relref "/docs/how-blitz2-works/libraries" >}})
explains how). This chapter gives an overview of what the libraries
provide and shows how to find out what a command takes.

## Looking up a command

**In the editor**, put the cursor on a command and press the HELP key.
The editor shows the command's name and its help text in the screen's
title bar: the arguments it takes, as the library declares them. For
example, the help text of `Left$` is `(String$,Length)`, that of `Mid$`
`(String$,Startchar[,Length])`; square brackets mark optional
arguments.

**In the source**, every library of the port is a C file whose
declaration part lists each command with its kind, its argument forms
and the same help text, and whose first comment summarises what the
library does:
[the libraries of Blitz Basic 2](https://github.com/fubar-coder/bb2-c/tree/main/libs/acidlibs)
on GitHub.

## What the libraries provide

**Text and numbers.** Printing (`Print`, `NPrint`, `Format`), the string
functions (`Left$`, `Mid$`, `Instr`, `Replace$`, `UCase$` and many
more), converting and computing with numbers (`Val`, `Int`, `Abs`, `Rnd`
and the transcendental functions), dates, sorting, the commands for
lists created with `Dim List`, and direct memory access with `Peek`,
`Poke` and memory banks.

**Input, output and files.** Reading and writing files, input from the
console with editing, the arguments a program was started with, facts
about the machine it runs on, the serial port, ARexx, and the file
requester and the ASL requesters of AmigaOS.

**The Intuition user interface.** Screens, windows and their events,
menus, gadgets of Intuition and of GadTools, and fonts: everything a
program needs to run politely beside other programs on the Workbench.

**Graphics and games.** Bitmaps, shapes and sprites, several kinds of
blitting (direct, queued to be cleared again, and buffered with the
background saved), stencils, palettes with fading and colour cycling,
the displays and copper lists of Blitz mode, scrolling, collision
detection, drawing, loading and saving IFF pictures and animations, and
the joysticks, game pads, mouse and keyboard for games.

**Sound.** Sound objects played through the audio hardware, speech, and
players for tracker and MED music modules.

**System.** Low-level access to Exec, such as message ports, for
programs that need more than the commands above.

## Calling AmigaOS directly

The second library file, `deflibs`, declares the functions of the
AmigaOS libraries, so a program can call them without further
declarations. Their names are the names of the AmigaOS functions with an
underscore added, such as `Delay_`, `Open_` or `DisplayBeep_`:

```
Delay_ 50                     ; wait one second (50 ticks)
```

Each declaration knows the AmigaOS library, the function's offset and
the registers of its arguments; the compiler loads the arguments into
those registers and calls the function. The help text gives the argument
names, as in `Open_` with `(name,accessMode)`. What each function does
is described by the documentation of AmigaOS itself. The AmigaOS
library is opened when the program starts and closed when it ends.

The libraries a program can use beyond these, such as the RWE userlibs,
are described in
[The RWE userlibs]({{< relref "/docs/blitz2-internals/rwe-userlibs" >}}).
