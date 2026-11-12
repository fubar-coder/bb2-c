---
title: Introduction
weight: 1
description: What the BB2 C port is, what you get, what you need, and the licence of the programs you write with it.
next: /docs/introduction/building
---

The BB2 C port is Blitz Basic 2, the BASIC compiler for the Amiga by Acid
Software, translated from its original 68000 assembler sources into C.
Its compiler, its editor and its libraries are built with a modern cross
compiler and run on the Amiga, or in an Amiga emulator, as the originals
did.

It is not a new dialect and not an emulator. It is Blitz Basic 2.10, the
version on the Blitz Basic 2.1 CD: the compiler produces the same
executables as the original compiler, and the libraries do what the
original libraries do. The one thing the port does not take over is the
original's bugs: they are fixed one at a time, and every fix and every
other difference is written down
([Differences from the original]({{< relref "/docs/compared-with-original" >}})).
If you need the original behaviour, bug for bug, use the original
compiler and libraries.

## What you get

| Part | What it is |
|---|---|
| **blitz2** | the compiler and the program you start; it opens the editor, and you compile from there |
| **ted** | the editor, TED 1.1, which blitz2 loads |
| **acidlibs** | the libraries with the commands of Blitz Basic 2 and the runtime code the compiler needs |
| **deflibs** | the declarations of the AmigaOS functions, for calling the system directly |
| **blitz2-ab3** | blitz2 with a second mode that compiles AmiBlitz3 sources from the Shell ([AB3 mode]({{< relref "/docs/ab3-mode" >}})) |
| **ab3cli** | a helper of blitz2-ab3 |

Together these files are the `blitz2` directory that you copy to an
Amiga or an emulator.

The repository also holds everything that checks the port: tools that
check each built file, and tests that run the port in an emulator and
compare what it does with what the original did ([Project structure]({{< relref "/docs/project-structure" >}})).

{{< callout type="info" >}}
There are no ready-made downloads of the port: you build it yourself
from the sources. The next page shows how.
{{< /callout >}}

## What you need

* **A Linux machine** on x86-64 to build on. This is the platform the
  port is built and tested on.
* **The cross compiler** for AmigaOS and a few common build tools
  ([Building the BB2 C port]({{< relref "/docs/introduction/building" >}})).
* **An Amiga emulator, or an Amiga**, with your own Kickstart ROM and
  Workbench, to run what you built. They are not part of the port and
  cannot be shipped with it
  ([Running it in an emulator]({{< relref "/docs/introduction/emulator" >}})).

## Your programs and the licence

The port is free software under the GNU General Public License, version
3 or later, as the original sources were released under the GNU licence.
A program you compile with it contains code that the compiler copies from
the libraries. A runtime library exception allows you to distribute your
compiled programs, including that code, under any licence you choose. The
compiler, the editor and the libraries themselves stay under the GPL. The
[licence section of the README](https://github.com/fubar-coder/bb2-c#license)
gives the details.

{{< cards >}}
  {{< card link="building" title="Building the BB2 C port" subtitle="From a fresh clone to the blitz2 directory." >}}
  {{< card link="emulator" title="Running it in an emulator" subtitle="Amiberry with Kickstart and Workbench from Amiga Forever, and the first start of blitz2." >}}
  {{< card link="first-program" title="Your first program" subtitle="Type, run, save and compile a program, in the editor and from the Shell." >}}
{{< /cards >}}
