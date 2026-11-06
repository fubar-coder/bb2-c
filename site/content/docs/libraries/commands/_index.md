---
title: Command libraries
weight: 1
description: Every command of the libraries in acidlibs, one page per library, with its forms and argument types.
---

The commands of Blitz Basic 2 come from the command libraries that the
compiler loads from `acidlibs` when it starts. Each page below belongs to
one of these libraries and lists its commands in alphabetical order, as
the library itself declares them. The pages are made from the libraries
the build produces, so they always match what the compiler sees.

## Reading an entry

Each command shows what kind of command it is, its help text and the
argument forms it accepts.

**The kind.** A *statement* stands on its own (`Window 0,...`), a
*function* returns a value and is used in an expression (`a=Event`),
and a *statement or function* can be used both ways. A function names
the type of its result. Some commands take a type suffix, such as
`Peek.w`, and then work on that type. An *object name*, such as
`Shape`, is not a command: it names the kind of object the library
manages, for commands like `Use` and `Free` (`Use Shape 1`) and in the
compiler options, which set how many such objects a program can have.
The page of such a library gives the default of that maximum.

**The help text** is the text the editor shows when you press HELP on
the command, exactly as the library has it. It names the arguments, and
square brackets mark optional ones. It comes from the original library:
some commands have none, and some texts describe only one of several
forms.

**The forms.** A command can accept several argument lists; the
compiler chooses the form by the number of arguments. Each form lists
the types the arguments are converted to:

| Notation | Argument |
|---|---|
| `.b`, `.w`, `.l` | byte, word or long integer |
| `.q`, `.f` | quick (fixed point) or float |
| `$` | string |
| `any` | any type; the library is told which one it got |
| `.Type` | the type of the command's suffix |
| `&` in front, as `&.l` | the address of a variable of that type, written `&a.l` |
| `array()` | a whole array, written `a()` |
| `{ ... }...` | a group of arguments that can be repeated, as in `Print a,b,c` |

For example, `Window` lists three forms; the shortest is
`.w, .w, .w, .w, .w, .l, $, .w, .w`: the window number, its position and
size, the flags, the title and two colours.

## The libraries

{{< library-list what="commands" >}}
