---
title: Inside the compiler source
weight: 6
description: How the C source of blitz2 and its editor is organised, how it is tied to the original assembler source, and how AB3 mode hooks in.
next: /docs/project-structure
---

Before this page you should know how the compiler works from the outside
([How blitz2 works]({{< relref "/docs/how-blitz2-works" >}})).

The compiler and the editor of the BB2 C port are translations of the
original assembler sources of Blitz Basic 2 and TED, made to be read next
to them. This page explains how the C source is organised, so that you
can find your way from a statement of Blitz Basic to the code that
compiles it, and from there to the original.

## Faithful rather than elegant

The translation keeps the original's algorithms, data layouts and even
its quirks. Functions and global variables carry the names of the
original's labels, and each function has a comment with the line of the
original source it translates, such as `/* blitz2.s:13837 doendif2 */`.
With the original source next to it (it is in the repository), you can
compare any routine line by line.

Two comment markers explain where the C code is not a plain translation:

* **`BUG(orig)`** marks a bug of the original that is not fixed yet,
  usually with the number of its issue. Once the bug is fixed, the
  comment becomes `Fixed` with the issue's number.
* **`DIFF`** marks a place where C cannot do what the assembler did, for
  example a jump that leaves several functions at once, and says what the
  original did instead.

## The parts of the compiler

The compiler's source is split the way the original source runs: each C
file translates one stretch of the original, in order. Roughly, from the
start: program start, menus and the connection to the editor; the
compile loop and the dispatch of statements; statements, residents and
the object buffer; the inline assembler and the loading of libraries;
the parser, error messages and writing the executable; expressions and
code generation; library calls and includes; the built-in statements in
two parts; and the data, requesters and options. Every C file has a
header that is its contract with the others.

The machine code the compiler copies into programs, its *templates*, is
kept as arrays with the original bytes. Assembler is only used where C
cannot express the original at all, in two small glue files: calling
code with given register contents, the entry points that the editor and
the system call, and the trap handler of the debugger's direct mode.

The compiler addresses its own global variables relative to register a4.
Every entry point from outside sets a4 first.

{{< callout type="info" >}}
Try it: find the code that compiles `End`. Its token is `$8002`, the
second built-in word, so it is the second entry of the table of statement
handlers in the compile loop's file, `doend`. Follow `doend` to the
routine it calls for the end of the program.
{{< /callout >}}

The source of the compiler is
[programs/blitz2/src](https://github.com/fubar-coder/bb2-c/tree/main/programs/blitz2/src),
the original
[blitz2.s](https://github.com/fubar-coder/bb2-c/blob/main/archive/sources/blitz2-1.95/blitz2.s)
on GitHub.

## blitz2 and blitz2-ab3

blitz2 and blitz2-ab3 are built from one core: the translated files above,
compiled once. Wherever AB3 mode behaves differently, the core calls an
*extension point*, a function with a fixed contract. blitz2 links a
version of these functions that does nothing and leaves everything to
Blitz Basic 2's code; blitz2-ab3 links the AB3 mode files, which handle a
case when the program is in AB3 mode and leave it otherwise. The build
checks that no AB3 code is in the core itself. That is how blitz2 stays
exactly Blitz Basic 2, and how blitz2-ab3 in BB2 mode gives the same
executables as blitz2.

## The editor

The editor is organised the same way, with one difference: its core, the
part that edits and tokenises lines, has no calls to the operating system.
That core is also compiled for the host, where the tests exercise it
directly, and it is the basis of the tools that tokenise and detokenise
sources on the host. The rest, screen, input, requesters and files, is the
Amiga part.

## The reference builds

The build also assembles the original sources of the compiler and the
editor, with a modern assembler, into reference programs with listings
and maps. They are what the port is compared with: the editor's data is
checked against its reference build after every link, and every code
label of the original editor must appear in the C code. The originals
themselves are never changed.
