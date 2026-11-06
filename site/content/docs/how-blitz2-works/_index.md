---
title: How blitz2 works
weight: 2
description: How the compiler, the editor and the libraries work together, from the line you type to the executable.
prev: /docs/introduction
next: /docs/how-blitz2-works/editor-and-compiler
---

Blitz Basic 2 is not a compiler in the usual sense, with source files,
object files and a linker. It is one program, blitz2, that brings its own
editor and keeps everything in memory. This chapter explains what happens
between typing a line and getting a program, so that the menu items, the
files next to your sources and the size of your executables make sense.

## The whole picture

When you start blitz2, it loads the editor and all libraries, then hands
control to the editor. Everything you do from then on happens in the
editor. The compiler only runs when you pick an item of the editor's
COMPILER menu:

```mermaid
sequenceDiagram
    actor You
    participant TED as Editor (TED)
    participant B as Compiler (blitz2)
    participant P as Your program
    You->>TED: type lines
    Note over TED: each line is tokenised<br/>when you leave it
    You->>TED: COMPILER menu, COMPILE AND RUN
    TED->>B: the lines, as they are in memory
    Note over B: one pass over the source,<br/>code and libraries into a buffer
    alt compiled
        B->>P: start the code in the buffer
        P-->>You: output in a window
    else error
        B-->>TED: error message and line
        TED-->>You: cursor on the line
    end
```

Three ideas carry the whole design:

* **The editor stores the program tokenised.** Keywords and library
  commands are kept as short numbers, not as text. The compiler never sees
  the text you typed, only these numbers and the rest of the line.
* **There is no linker.** The compiler writes machine code straight into
  one block of memory and copies the code of every library the program
  uses next to it. To run the program it simply jumps into that block; to
  create an executable it writes the block to a file.
* **The compiler runs code while it compiles.** Constant expressions are
  evaluated by generating code for them and running it. That is one reason
  why blitz2 runs only on a 68000-family processor, on an Amiga or in an
  emulator.

## The pages of this chapter

{{< cards >}}
  {{< card link="editor-and-compiler" title="The editor and the compiler" subtitle="How blitz2 and TED share one program, and what each item of the COMPILER menu does." >}}
  {{< card link="tokens" title="Tokenised sources" subtitle="What a token is, how library commands get theirs, and why the numbers matter." >}}
  {{< card link="libraries" title="Libraries: acidlibs, deflibs, userlibs" subtitle="The kinds of libraries, where they come from and what ends up in your program." >}}
  {{< card link="compiling" title="From source to executable" subtitle="Why a small program becomes an executable of several kilobytes, and what runtime error checking adds." >}}
  {{< card link="residents" title="Residents and the .xtra file" subtitle="The companion file of a source, and precompiled types, macros and constants." >}}
{{< /cards >}}
