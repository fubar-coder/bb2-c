---
title: Blitz objects
description: Screens, windows, bitmaps, shapes and the other numbered objects
  the libraries manage - the currently used object, Use, Free, Used, Addr and
  Maximum, and the object maximums.
sidebar:
  order: 8
next:
  link: /docs/libraries/
  label: Libraries
---

Before this page you should know how a program uses libraries
([Libraries](/docs/how-blitz2-works/libraries/)).

Many things a program works with, such as screens, windows, bitmaps,
shapes, sounds and files, are *Blitz objects*. A library manages each
kind of object for the program: it creates them, keeps their memory and
frees them again. The program names an object by its kind and a number,
`Window 0` or `Shape 12`, and never handles the memory itself.

## Creating and freeing objects

Every kind has commands of its library that create an object with a
number: `BitMap 0,320,200,2` makes bitmap 0, `Window 1,...` opens window
1, `LoadShape 5,"ball.iff"` loads shape 5. Creating a bitmap or a window
under a number that is already in use frees the old one first.

`Free kind number` frees an object and its memory; for a window or a
screen it also closes it on the display. Freeing an object that does not
exist does nothing. An object the program has not freed is freed when the
program ends, at `End` or at its last line
([When start and end routines run](/docs/libraries/in-a-program/#when-start-and-end-routines-run)).

```
BitMap 0,320,200,2
Free BitMap 0
```

## The currently used object

Of each kind, one object is the *currently used* one, and many commands
work on it without being told its number: drawing commands draw into the
used bitmap or window, `Blit` puts a shape onto the used bitmap, a new
window opens on the used screen. An object becomes the used one when it
is created, and `Use kind number` makes another one the used one:

```
BitMap 0,320,200,2
BitMap 1,320,200,2      ; bitmap 1 is the used bitmap now
Use BitMap 0
Blit 3,10,10            ; puts shape 3 onto bitmap 0
```

The function `Used kind` returns the number of the used object, as a
Word, or -1 while no object of the kind has been used yet. A procedure
or an interrupt that changes the used object can remember it with `Used`
and give it back with `Use` before it ends.

## The maximum of a kind

Each kind has a *maximum*: the objects are numbered from 0 to the maximum
less 1. The maximum is set per kind in the **Object Maximums** of the
compiler options, and the function `Maximum kind` returns it:

```
FindScreen 0
Window 0,20,20,240,80,$100f,"Window 0",1,2
Window 1,60,60,240,80,$100f,"Window 1",1,2
a=Used Window           ; 1
Use Window 0
b=Used Window           ; 0
Free Window 1
DefaultOutput
NPrint a," ",b," ",Maximum Window   ; 1 0 5
```

With runtime error checking, a number outside 0 to the maximum less 1
stops the program with "Value out of Maximum Range". Without it the
number is not checked, and the program uses memory that belongs to
something else.

## The address of an object

`Addr kind(number)` returns the address of the object's data in memory,
as a Long. The libraries keep there what they need for the object, and
for objects of the system the first Long is usually the address of the
AmigaOS structure: for a window the Intuition window, for a screen the
Intuition screen. A program can pass it to AmigaOS functions:

```
FindScreen 0
Window 0,20,20,240,80,$100f,"Moving",1,2
win.l=Peek.l(Addr Window(0))
MoveWindow_ win,40,0    ; intuition.library moves the window
```

What lies behind the address differs from library to library; the
sources of the libraries describe it.

## The kinds of objects

`Use`, `Free`, `Used`, `Addr` and `Maximum` take the name of a kind.
A word that names no kind of object, such as `Use Print 0`, is the
compile error "Illegal Token". The kinds of the libraries that come with
the BB2 C port:

| Kind | Library | Default maximum |
|---|---|---|
| `Anim` | [animlib](/docs/libraries/commands/animlib/) | 10 |
| `Bank` | [banklib](/docs/libraries/commands/banklib/) | 5 |
| `BitMap` | [bitmaplib](/docs/libraries/commands/bitmaplib/) | 10 |
| `BlitzFont` | [fontlib](/docs/libraries/commands/fontlib/) | 4 |
| `Buffer` | [bblitlib](/docs/libraries/commands/bblitlib/) | 10 |
| `CopList` | [displaylib](/docs/libraries/commands/displaylib/) | 10 |
| `File` | [fileiolib](/docs/libraries/commands/fileiolib/) | 5 |
| `GadgetList` | [gadgetslib](/docs/libraries/commands/gadgetslib/) | 5 |
| `GTList` | [gtlib](/docs/libraries/commands/gtlib/) | 5 |
| `IntuiFont` | [intuifontlib](/docs/libraries/commands/intuifontlib/) | 5 |
| `MedModule` | [medlib](/docs/libraries/commands/medlib/) | 8 |
| `MenuList` | [menuslib](/docs/libraries/commands/menuslib/) | 5 |
| `Module` | [trackerlib](/docs/libraries/commands/trackerlib/) | 5 |
| `Page` | [blitzoslib](/docs/libraries/commands/blitzoslib/) | 4 |
| `Palette` | [palettelib](/docs/libraries/commands/palettelib/) | 4 |
| `Queue` | [qblitlib](/docs/libraries/commands/qblitlib/) | 10 |
| `Screen` | [screenslib](/docs/libraries/commands/screenslib/) | 5 |
| `Shape` | [shapeslib](/docs/libraries/commands/shapeslib/) | 100 |
| `Slice` | [blitzcoplib](/docs/libraries/commands/blitzcoplib/) | 10 |
| `Sound` | [audiolib](/docs/libraries/commands/audiolib/) | 10 |
| `Sprite` | [spriteslib](/docs/libraries/commands/spriteslib/) | 20 |
| `Stencil` | [stencillib](/docs/libraries/commands/stencillib/) | 5 |
| `Tape` | [brexxlib](/docs/libraries/commands/brexxlib/) | 5 |
| `Window` | [windowslib](/docs/libraries/commands/windowslib/) | 5 |

Some of the userlibs add kinds of their own:
`ChunkyBitmap` ([richunkylib](/docs/libraries/userlibs/richunkylib/)),
`GTMenuList` ([rigtmenulib](/docs/libraries/userlibs/rigtmenulib/)),
`MapObject` ([rimapeditorlib](/docs/libraries/userlibs/rimapeditorlib/))
and `Rast` ([riintuilib](/docs/libraries/userlibs/riintuilib/)).
The page of each library lists the commands that create and use its
objects.

A few kinds differ from the rest. Files are opened with the functions
`OpenFile`, `ReadFile` and `WriteFile`, which return whether it worked,
and closed with `CloseFile`; `Free File` closes one as well. Slices
cannot be freed one by one: `FreeSlices` frees them all. A palette is
created by loading it or with `PalRGB`, and `Use Palette` sets the
colours of the used screen or slice to it, unless `NewPaletteMode On` is
set.

Based on the Blitz Basic 2 manual by Acid Software.
