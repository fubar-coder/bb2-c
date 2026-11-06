---
title: Libraries in C
weight: 2
description: How the BB2 C port writes Blitz libraries in C - the compiler options, register parameters, strings, check routines and the build step that checks every library.
---

Before this page you should know what a library file contains
([Anatomy of a Blitz library]({{< relref "/docs/blitz2-internals/library-anatomy" >}})).

Every library of the BB2 C port is one C file, compiled with the same
cross compiler as blitz2 but with different rules. This page explains
those rules, so that you can read the existing libraries and write your
own.

## A different world from a normal program

A library's code runs inside a Blitz program, not inside a C program.
There is no C library, no start-up code and no C stack frame convention
the caller would know about. Three registers belong to the Blitz program
and must never be touched: a5 points to its global variables, a4 to the
local variables of the current procedure, and a3 into the string work
area. The library build therefore compiles every library with:

* **`-ffixed-a3 -ffixed-a4 -ffixed-a5`**, so the C compiler never uses
  these registers for its own purposes. C code reaches them through
  global register variables; `blitz_string`, for example, is a3.
* **`-ffreestanding`**, without the C library, and with loop
  optimisations off that would turn a loop into a `memset` or `memcpy`
  call. A call into a library that is not there fails at link time,
  never at run time.
* **`-fno-toplevel-reorder`**, so the declarations stay in the order they
  are written: the table of commands must be in the order of the tokens.
* **`-Os`**, also in debug builds, because unoptimised code would call
  helper routines of the C compiler that a library cannot have.

A linker script puts the header and the table at the very start of the
single code hunk, followed by the code and data, and the error check
routines last. The result is stripped and checked
([What the build checks]({{< relref "/docs/project-structure/build-checks" >}})).

You do not set these options yourself: the CMake function
`blitz_library()` does it for every library it builds.

## A routine in C

A routine gets its arguments in the registers the table says, and
returns its result in d0. In C, that is a function with register
parameters:

```c
LONG BLITZ_ROUTINE twice(LONG value BL_REG(d0))
{
    return value * 2;
}
```

`BL_REG(d0)` puts the parameter into d0; a value requested with
`BLITZ_LIBS`, for example the base of `dos.library` in a6, is a parameter
`BL_REG(a6)`. The C compiler keeps the registers d2 to d7 and a2 to a6
unchanged, which is at least what the original routines kept, so a plain
C function is a valid library routine.

When a routine must keep more registers than the C compiler does, or
return something in another register or in the condition codes, a short
assembler *thunk* sits between the table and the C function:
`BLITZ_THUNK(entry, cfunc, "d0-d1/a0-a1")` saves the named registers,
calls the C function and restores them. A routine that is assembler by
nature, because it changes the stack pointer or talks to the hardware,
is written with `BLITZ_ASM_ROUTINE` and keeps the original instructions.

## Strings

A function returning a string writes its characters to the string work
area through `blitz_string` (a3), leaves a3 on the end of the string, and
returns the length. The lengths of string arguments are on a stack below
register a2, the first argument's on top; the helper macros `BL_POPL` and
`BL_POPW` read them, through a pointer of the routine's own, so that a2
itself stays unchanged.

## Check routines and runtime errors

A check routine, the second and third entry of `BLITZ_SUBS`, belongs in
the error check area, which is copied into a program only with runtime
error checking on. A C function goes there with `BLITZ_ERRCHK`, and a
thunk for it with `BLITZ_ERRCHK_THUNK`. An error is raised with
`BLITZ_RUNERR(message)`: it shows the message through the program's
runtime error handler, the debugger or the error requester. A message
used only by check routines goes into the error check area with
`BLITZ_ERRCHK_MESSAGE`. Normal code must never refer to anything in that
area, because without runtime error checking it is not there; the build
refuses a library that does.

## Calling other libraries

A routine can call a routine of another Blitz library by its token, with
`BLITZ_ALIBJSR`. The C code writes a placeholder, and the compiler fills
in the real address when it copies both libraries into a program. AmigaOS
functions are called through the library base the routine asked for with
`BLITZ_LIBS`, with the normal AmigaOS headers and inline calls.

## Where to read on

The macros, with the original's macro next to each one and a description
of every helper, are documented in
[the header for libraries](https://github.com/fubar-coder/bb2-c/blob/main/libs/include/blitzlib.h)
and [its helpers](https://github.com/fubar-coder/bb2-c/blob/main/libs/include/blitzhelp.h).
The best examples are the libraries themselves, each with a comment at
the top that explains its conventions:
[the libraries of Blitz Basic 2](https://github.com/fubar-coder/bb2-c/tree/main/libs/acidlibs)
on GitHub.
