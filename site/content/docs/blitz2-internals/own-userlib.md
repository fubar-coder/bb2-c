---
title: Writing your own userlib
weight: 3
description: A tutorial - write a Blitz library in C, build it with the BB2 C port, install it as a userlib and call it from a Blitz Basic program.
---

Before this page you should know how libraries are written in C
([Libraries in C]({{< relref "/docs/blitz2-internals/c-libraries" >}}))
and have compiled a program with the port.

This page takes you from an empty file to a Blitz Basic program that
calls a command of your own. The library has one function, `Twice`, which
returns its argument times two. Everything on this page was carried out
with the port exactly as shown.

A userlib is built inside a clone of the BB2 C port's repository, with
its build: the CMake function that compiles and checks a library is part
of that build, and there is no separate kit for libraries.

{{% steps %}}

### Choose a library number

Every library needs a number of its own; its commands' tokens are made
from it ([Tokenised sources]({{< relref "/docs/how-blitz2-works/tokens" >}})).
The numbers in use are listed in the file
[libnums.h of the default variant](https://github.com/fubar-coder/bb2-c/blob/main/libs/variants/bb2-2.10/libnums.h).
Command libraries have numbers from 1 to 255. Choose one that is not in
that list and that no other userlib you use has. This example uses 9.

### Write the library

Create a directory for your libraries, `libs/userlibs/mylibs/`, and in it
the file `twicelib.c`:

```c
/*
 * twicelib - a userlib with one function: Twice(Value) returns Value*2.
 */
#include "blitzlib.h"

/* the routine, defined below */
LONG BLITZ_ROUTINE twice(LONG value BL_REG(d0));

/* library number 9, no start or end routine, no error check area */
BLITZ_LIBHEADER(9, 0, BL_NORET, 0, 0);

/* Twice: a function returning a long, one long argument */
BLITZ_AFUNCTION(BL_LONG);
BLITZ_ARGS(BL_LONG);
BLITZ_LIBS0();
BLITZ_SUBS(twice, 0, 0);
BLITZ_NAME("Twice", "(Value)  Value times two");

BLITZ_LIBFIN();

LONG BLITZ_ROUTINE twice(LONG value BL_REG(d0))
{
    return value * 2;
}
```

The header comes first, then one entry per command, then the end of the
table, then the code. The entry says: a function returning a long, with
one long argument, needing nothing from other libraries, implemented by
`twice`, named `Twice`.

### Add it to the build

Next to it, create `libs/userlibs/mylibs/CMakeLists.txt`:

```cmake
blitz_library(twicelib
	OBJ ${BLITZ_VARIANTS_DIR}/${BLITZ_DEFAULT_VARIANT}/userlibs/mylibs/twicelib.obj
	twicelib.c
	CHECK ${BLITZ2_DIR}/acidlibs ${BLITZ2_DIR}/deflibs)
```

`OBJ` says where the library file goes: into the userlibs directory of
the default library variant. `CHECK` names the library files it will be
loaded with, so the build can refuse a number that is already taken.

Then add one line to `libs/userlibs/CMakeLists.txt`:

```cmake
add_subdirectory(mylibs)
```

### Build it

Configure again, since there is a new directory, and build:

```
cmake --preset amiga
cmake --build --preset amiga
```

The library is compiled, linked into one code hunk and checked in the
same step. The file is
`build/amiga/variants/bb2-2.10/userlibs/mylibs/twicelib.obj`. If the
number is already taken, the build stops and names both libraries, for
example with number 187:

```
blitzlib numbers: library number 187 twice: .../userlibs/mylibs/twicelib.obj (library 1) and .../blitz2/acidlibs (library 42)
```

### Install it

blitz2 loads every library file in the directories under
`blitz2:userlibs/`. Copy the file into a directory of its own there:

```
MakeDir blitz2:userlibs
MakeDir blitz2:userlibs/mylibs
Copy twicelib.obj blitz2:userlibs/mylibs/
```

If blitz2 is already running, choose RELOAD ALL LIBS in the COMPILER
menu: it loads the userlibs again and adds their commands to the names
the editor and the compiler know. Otherwise start blitz2 again. A line you
typed before the reload still holds the new command's name as plain text
and gives a compile error: type it again, and the editor turns the name
into the new token.

### Use it

Write a program that calls the new function:

```
NPrint Twice(21)
NPrint Twice(-4)
End
```

Compiled and run, it prints

```
42
-8
```

{{% /steps %}}

{{< callout type="info" >}}
Try it: change the help text in `BLITZ_NAME`, build again, copy the file
again, choose RELOAD ALL LIBS and press the HELP key with the cursor on
`Twice` in the editor.
{{< /callout >}}

## Documenting the library

A library has two kinds of documentation, for two kinds of readers.

**The help text in the table** is for the Blitz programmer. It is the
second argument of `BLITZ_NAME`, here `"(Value)  Value times two"`: the
command's arguments in Blitz notation and a few words on what it does.
It is part of the library file, so the editor's HELP key shows it in the
title bar, and it is what the
[command reference]({{< relref "/docs/libraries" >}}) of this site lists
for every command, generated from the built library files. Write the
Blitz syntax there, and nowhere else.

**The comment of the routine** is for the C developer who reads or changes
the library. The port writes it as a Doxygen comment, from which the
[API reference](https://fubar-coder.github.io/bb2-c/api/) is made and
which editors with clangd show on hover. For `Twice`:

```c
/**
 * @file
 * twicelib - a userlib with one function: Twice(Value) returns Value*2.
 */
```

at the top of the file, and before the routine:

```c
/**
 * @brief Twice(Value): the value times two.
 *
 * The product is not checked: a value beyond the range of a long gives
 * the low 32 bits of the product.
 *
 * @param value d0: the value
 * @return d0: value * 2
 */
LONG BLITZ_ROUTINE twice(LONG value BL_REG(d0))
```

The conventions of the port's libraries:

* The `@brief` of a routine that implements a command starts with the
  command's name, so that the routine is found by the name a Blitz
  programmer knows.
* `@param` and `@return` name the register first (`d0: ...`), since that
  is how the routine is called. A routine that returns nothing has no
  `@return`.
* A routine in assembler, written with `BLITZ_ASM_ROUTINE` or
  `BLITZ_INLINE`, has no C parameters: its register arguments are `@arg`
  items and its result is a paragraph `@par Result`.

The full rules are in the repository's
[development notes](https://github.com/fubar-coder/bb2-c/blob/main/docs/development.md#function-comments-and-the-references).

### The API reference of your own library

The API reference of the site covers the libraries of Blitz Basic 2, the
RWE userlibs and the shared code, not other directories under
`libs/userlibs/`. For your own library, run Doxygen locally with the
repository's configuration and your directory as its input. From the
repository's root, with Doxygen in its official container:

```
mkdir -p build/api-mylibs
{ cat Doxyfile
  echo 'INPUT = libs/include libs/userlibs/mylibs'
  echo 'OUTPUT_DIRECTORY = build/api-mylibs'
  echo 'USE_MDFILE_AS_MAINPAGE ='
} | podman run --rm -i -v "$PWD":/src -w /src ghcr.io/doxygen/doxygen:Release_1_16_1 -
```

The configuration treats comment errors as errors, so a `@param` for a
parameter that does not exist stops the run. The result is HTML in
`build/api-mylibs/`; open `build/api-mylibs/index.html` in a browser.

### The command reference

The command reference lists the libraries whose command tables are part
of the repository: the build of such a library, with the option
`CMDREF userlibs/<name>` of `blitz_library()`, checks that the table it
generates from the built file equals the committed file in
`site/data/libraries/`, and the target `cmdref_update` copies the
generated files there. That is for libraries that become part of the
port. A library of your own does not need it: its help texts are in the
library file, and the editor's HELP key shows them.

## Going further

* **More commands**: add further entries before `BLITZ_LIBFIN()`. The
  order of the entries is the order of the tokens, so add new commands at
  the end once a library is in use, or the sources written for it will
  mean other commands.
* **Strings, statements, several forms**: the libraries of Blitz Basic 2
  show every kind of entry. A short one with strings is the library of the
  command line arguments
  ([cliargslib](https://github.com/fubar-coder/bb2-c/blob/main/libs/acidlibs/cliargslib.c)).
* **Argument checks**: give `BLITZ_SUBS` a check routine and the header
  `BLITZ_ERROR_AREA`, so the check is only in programs compiled with
  runtime error checking.
* **Another set of numbers**: if your library must have a number another
  library has, it belongs to a variant of its own
  ([Library variants]({{< relref "/docs/blitz2-internals/variants" >}})).
