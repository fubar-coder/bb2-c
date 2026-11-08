---
title: Running blitz2
weight: 2
description: Fixed bugs of Blitz Basic 2 in blitz2 as a program - loading the libraries, starting and ending, and the editor's compiler menu.
---

These bugs concern blitz2 as a program rather than the code it compiles:
how it loads its libraries, how it starts and ends, and what the
compiler menu of the editor does. Each is one sentence here; its issue
has the details.

## Loading the libraries

At its start, and with RELOAD ALL LIBS, blitz2 loads the command libraries
from `blitz2:deflibs`, `blitz2:acidlibs` and the directories in
`blitz2:userlibs`.

* A read error while loading a library file went unnoticed, and blitz2
  went on with whatever was left in its buffer; a library that cannot be
  read completely is left out
  ([#3](https://github.com/fubar-coder/bb2-c/issues/3)).
* A library file whose relocation data could not be read was used anyway
  and patched at random places; it is left out
  ([#993](https://github.com/fubar-coder/bb2-c/issues/993)).
* When blitz2 could not examine `blitz2:deflibs` or `blitz2:acidlibs`, it
  loaded the file with the length of the file examined before; the file
  counts as missing ([#27](https://github.com/fubar-coder/bb2-c/issues/27)).
* A directory in `blitz2:userlibs` with a name of 32 characters or more,
  as on file systems with long names or host directories in an emulator,
  overwrote memory; such a directory is skipped
  ([#28](https://github.com/fubar-coder/bb2-c/issues/28)).

## Starting and ending

* blitz2 always returned 0 to the shell, also when it could not start; it
  returns -1 when it cannot start and 0 after a normal session, and in AB3
  mode a compile from the command line keeps its own return codes
  ([Compiling from the command line]({{< relref "/docs/ab3-mode/command-line" >}}),
  [#22](https://github.com/fubar-coder/bb2-c/issues/22)).
* Without chip memory for the arrows of the options requester, blitz2
  wrote to and freed address 0; it gives up at the start with return code
  -1 ([#8](https://github.com/fubar-coder/bb2-c/issues/8)).
* With 256 MB or more of free memory, blitz2 lost 256 MB when it ended;
  the memory is freed again at once
  ([#26](https://github.com/fubar-coder/bb2-c/issues/26)).
* Started from the Workbench, blitz2 left a short or damaged icon file in
  use until the reboot; the file is closed after any read
  ([#19](https://github.com/fubar-coder/bb2-c/issues/19)).
* Started from the Workbench without a project icon, blitz2 could ask the
  editor to load its own icon as a program; the editor starts with no
  program ([#20](https://github.com/fubar-coder/bb2-c/issues/20)).
* Started from the Workbench, blitz2 did not go back to its start
  directory when it ended; the start directory is restored
  ([#21](https://github.com/fubar-coder/bb2-c/issues/21)).
* Started from the Workbench, blitz2 could crash at its very end, for
  example under a memory checker such as MungWall; it has ended before the
  Workbench unloads it
  ([#337](https://github.com/fubar-coder/bb2-c/issues/337)).

## The editor's compiler menu

* LOAD DEBUG MODULE left the busy pointer on when no debugger module had
  been loaded at the start; the pointer returns to normal
  ([#23](https://github.com/fubar-coder/bb2-c/issues/23)).
* When RUN or COMPILE AND RUN could not start the program for lack of
  memory, the `EXECUTING` requester stayed open; it is closed, with no
  error message, as in the original
  ([#25](https://github.com/fubar-coder/bb2-c/issues/25)).
