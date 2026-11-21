---
title: Running blitz2
weight: 2
description: Fixed bugs of Blitz Basic 2 in blitz2 as a program - loading the libraries, starting and ending, out of memory during a compile, and the editor's compiler menu.
---

These bugs concern blitz2 as a program rather than the code it compiles:
how it loads its libraries, how it starts and ends, what it does when
memory runs out during a compile, and what the compiler menu of the
editor does. Each is a short point here, and its
issue has the details. Each point says first what the original did,
then, after the semicolon, what the port does.
The table on [Important Fixes & Breaking Changes]({{< relref "/docs/compared-with-original/important-fixes" >}})
lists all fixed bugs by what they can change in a program.

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
* A library file whose relocation data holds no entries, which only a
  hand-made file has, counted as out of memory: blitz2 stopped at its
  start, or crashed under the editor; it is loaded as a library without
  relocations ([#1175](https://github.com/fubar-coder/bb2-c/issues/1175)).
* When blitz2 could not examine `blitz2:deflibs` or `blitz2:acidlibs`, it
  loaded the file with the length of the file examined before; the file
  counts as missing ([#27](https://github.com/fubar-coder/bb2-c/issues/27)).
* A directory in `blitz2:userlibs` with a name of 32 characters or more,
  as on file systems with long names or host directories in an emulator,
  overwrote memory; such a directory is skipped
  ([#28](https://github.com/fubar-coder/bb2-c/issues/28)).

## Starting and ending

* blitz2 always returned 0 to the shell, also when it could not start; it
  returns -1 when it cannot start and 0 after a normal session. In AB3
  mode, a compile from the command line keeps its own return codes
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
  example under a memory checker such as MungWall; blitz2 has ended
  before the Workbench unloads it
  ([#337](https://github.com/fubar-coder/bb2-c/issues/337)).

## Out of memory during a compile

When memory runs out during a compile from the editor, the compiler of
the port stops as it does at a compile error: it closes the include
files, frees the memory of the compile, and returns to the editor, which
then shows its alert "Not enough memory for operation!". The next compile
starts afresh. When the port's compiler runs under the editor of Blitz Basic 2,
or from the Shell, it reports the error "Out of Memory" instead.

* The original compiler got its memory from the editor, which on failure
  showed its alert and jumped straight back into its own main loop: the
  compile was abandoned in the middle, its include files stayed open, its
  memory was lost, and the next compile did not come back. A failed
  change of the buffer sizes in COMPILER OPTIONS left the compiler with
  buffers that had been freed; the compile ends through the compiler's
  own cleanup, as described above
  ([#369](https://github.com/fubar-coder/bb2-c/issues/369)).
* When the object buffer runs full, the requester "Not Enough Object
  Memory Available!" offers INCREASE 150% - RECOMPILE. If the larger
  buffers could not be allocated, the original's code went back to the
  old sizes and compiled again with them, so the same requester came
  back until CANCEL COMPILE, with no word about the memory (the original
  itself never got that far, because the editor abandoned the compile
  first); the compile ends with the alert or error described above
  ([#32](https://github.com/fubar-coder/bb2-c/issues/32)).

## The editor's compiler menu

* LOAD DEBUG MODULE left the busy pointer on when no debugger module had
  been loaded at the start; the pointer returns to normal
  ([#23](https://github.com/fubar-coder/bb2-c/issues/23)).
* When RUN or COMPILE AND RUN could not start the program for lack of
  memory, the `EXECUTING` requester stayed open; it is closed, with no
  error message, as in the original
  ([#25](https://github.com/fubar-coder/bb2-c/issues/25)).
