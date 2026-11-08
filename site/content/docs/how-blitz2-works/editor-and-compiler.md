---
title: The editor and the compiler
weight: 1
description: How blitz2 and its editor TED share one program, and what each item of the COMPILER menu does.
prev: /docs/how-blitz2-works
---

Before this page you should have started blitz2 once and seen its editor.

blitz2 and its editor TED are two files, but at run time they are one
program. This page explains how they work together and what the items of
the editor's COMPILER menu do.

## One program, two parts

What you start is blitz2, from the Shell or from the Workbench. blitz2
does not draw a single window itself: it loads the editor from the file
`blitz2:ted` and calls it. The editor opens its screen, and from then on
it is the user interface. The compiler runs only when the editor calls
it, for an item of the COMPILER menu, and when the editor loads or saves
a source. Both parts run in the same process; the editor is code that
blitz2 has loaded, not a program of its own.

When blitz2 starts, these things happen in order:

1. blitz2 loads the editor, and the editor opens its screen.
2. blitz2 loads its libraries: the two library files `blitz2:acidlibs`
   and `blitz2:deflibs`, then every library file in the directories
   under `blitz2:userlibs/`. The names of all commands go into the list
   the editor uses to recognise them.
3. blitz2 shows its version requester and waits until you close it.
4. blitz2 loads the debugger module `blitz2:defaultdbug`, if there is
   one.
5. blitz2 hands control to the editor, with the file name you gave on the
   command line, if any. When you leave the editor, blitz2 cleans up and
   ends.

The editor can also be started alone, as `blitz2:ted`. It is then a plain
text editor without the COMPILER menu.

## The COMPILER menu

The editor adds the menu COMPILER with the items blitz2 gives it. Each
has a keyboard shortcut, used with the right Amiga key:

| Item | Key | What it does |
|---|---|---|
| COMPILE AND RUN | X | Compiles the source and runs it. |
| RUN | M | Runs the program compiled last, without compiling again; nothing happens if that compile failed. |
| CREATE EXECUTABLE | E | Asks for a file name, compiles the source and writes an executable. |
| COMPILER OPTIONS | O | Opens the options requester: runtime error checking, buffer sizes, residents and more. |
| CREATE RESIDENT | ; | Compiles the source and saves its types, macros and constants as a resident file. |
| VIEW NEWTYPES | - | Shows the NewTypes the compiler knows. |
| CLI ARGUMENT | = | Sets the argument line a program run from the editor receives. |
| CALCULATOR | H | Evaluates an expression you type. |
| RELOAD ALL LIBS | \ | Loads the libraries under `blitz2:userlibs/` again. |
| LOAD DEBUG MODULE | . | Loads another debugger module from a file. |

The options of a source are stored with it, in its `.xtra` file
([Residents and the .xtra file]({{< relref "/docs/how-blitz2-works/residents" >}})).

## Compiling

While the compiler works, a small requester shows `COMPILING` with a
STOP button. The compiler checks for the button before every statement,
so a long compile can be stopped at any time.

The compiler reports **one error at a time**. At the first error it stops,
the editor shows the message, and the cursor goes to the line with the
error. After you fix it, the next compile finds the next error, if there
is one.

## Running a program from the editor

COMPILE AND RUN and RUN start the compiled program as a process of its
own, called "Blitz ][ Program Proc". The program runs directly in the
compiler's memory, at the place where it was compiled: no file is
written. Its text output goes to a window the editor opens for it.

While it runs, a requester shows `EXECUTING` with a `!PANIC!` button.
PANIC removes the program at once, without letting it clean up: memory it
allocated and windows it opened stay. It is meant for a program that
hangs, not as the normal way to end one.

## Runtime error checking and the debugger

The option **Runtime Error Debugger** in the compiler options switches
runtime error checking on; it is on by default. With it, the compiler
adds a check to every statement and lets the libraries check their
arguments, so a program stops with a message instead of crashing, for
example on an array index out of range.

How the error is shown depends on how the program runs:

* **Run from the editor**, the program is connected to the debugger
  module that blitz2 loaded at start, `blitz2:defaultdbug`, a program of
  its own that runs beside yours and reports its errors. LOAD DEBUG MODULE
  replaces it with another one. The BB2 C port does not build a debugger
  module.
* **As an executable**, the program gets a small error handler instead:
  it shows the error text in a requester and ends the program. Such an
  executable does not need the debugger module.

Runtime error checking makes programs larger and slower, so a finished
program is usually compiled without it. What exactly it adds is on the
page [From source to executable]({{< relref "/docs/how-blitz2-works/compiling" >}}).

## Warnings

Besides errors, the compiler of the BB2 C port knows warnings: something
in a source that is probably a mistake, but does not stop the compile
and does not change the program. There is one so far, named `resclash`:
two residents define the same constant with different values. The
compiler then shows "Warning: Clash in Residents: #NAME" for each such
constant, at most for eight names followed by one "Clash in Residents:
N more", and uses the value of the first resident
([Residents and the .xtra file]({{< relref "/docs/how-blitz2-works/residents" >}})).

A warning is shown in the same requester as an error, with "Warning:" in
front of the text. After you click OK, the compiler goes on. Since the
residents are loaded with the options, this warning appears when you
load the source or close the options requester, not during the compile.

The setting **WARNINGS** chooses what happens with warnings. It is in the
requester Runtime Error Options, which opens when you switch on
**Runtime Error Debugger...** in the compiler options. A click on its
button steps through three states:

| State | What it does |
|---|---|
| OFF | No warnings are shown. |
| ON | Warnings are shown, and the compile goes on. This is the default. |
| ERRORS | Every warning is an error and is shown as one. A clash of residents then drops the residents, as a clash of NewTypes or macros does, so a source that needs them does not compile. |

Like the other options, the setting is stored in the source's `.xtra`
file, in a word that Blitz Basic 2 writes but never uses. The original
therefore reads such a file as before, an `.xtra` file written by the
original means ON, and the file of a source whose setting you leave
alone stays exactly the same.
From the Shell, the AB3-mode compiler changes the setting with options
of its own ([Compiling from the command line]({{< relref "/docs/ab3-mode/command-line#warnings" >}})).

## Creating an executable

CREATE EXECUTABLE asks for a file name and compiles the source with the
option **Make Smallest Code** switched on, whatever the options say. The
compiler then compiles twice: the first pass finds out how much memory
each part of the program needs, the second compiles into buffers of
exactly that size, so the file contains no unused space. The result is a
normal AmigaOS executable that runs without blitz2. If the option
**Create Icons for Executable Files** is on, an icon is written next to
it.
