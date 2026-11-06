---
title: AB3 mode
weight: 5
description: blitz2-ab3, the compiler with a second mode that compiles AmiBlitz3 sources with AmiBlitz3's own libraries.
prev: /docs/compared-with-original/comparing
next: /docs/ab3-mode/setup
---

AmiBlitz3 is the later development of Blitz Basic 2: the same code line,
continued for many years by other people, with a larger language and
many more libraries. Its sources do not compile with Blitz Basic 2, and
its tokenised files mean other commands
([Tokenised sources]({{< relref "/docs/how-blitz2-works/tokens" >}})).

The BB2 C port has a second compiler program for this, **blitz2-ab3**.
It has two modes:

* **BB2 mode** compiles Blitz Basic 2 sources exactly as blitz2 does: the
  same libraries, the same tokens, the same executables.
* **AB3 mode** compiles AmiBlitz3 sources with AmiBlitz3's token list and
  AmiBlitz3's own library files, as AmiBlitz3 v3.14.0 ships them.

blitz2-ab3 decides for each source which mode it needs. blitz2 itself has
no AB3 mode and is not affected by it in any way.

## What AB3 mode is and is not

AB3 mode compiles from the Shell, one source per run, with the options of
AmiBlitz3's command line. Started without options, blitz2-ab3 opens the
editor like blitz2, and the editor always works in BB2 mode: AmiBlitz3
sources are compiled from the Shell, not from the editor.

AB3 mode uses AmiBlitz3's libraries as binary files; they are not part of
the port and not translated to C. You need AmiBlitz3's files for it
([Setting up AmiBlitz3's files]({{< relref "/docs/ab3-mode/setup" >}})).

AB3 mode does not yet support all of AmiBlitz3's language. Many of
AmiBlitz3's own test programs compile and run with the same results as
with AmiBlitz3; programs that use floating point on the FPU or 68020
instructions in inline assembler do not compile yet. What is supported
and what is missing is described in the chapter
[AB3 compared with blitz2]({{< relref "/docs/ab3-compared" >}}).

{{< cards >}}
  {{< card link="setup" title="Setting up AmiBlitz3's files" subtitle="The AmiBlitz3 libraries and residents AB3 mode needs, and where to get them." >}}
  {{< card link="command-line" title="Compiling from the command line" subtitle="The options of blitz2-ab3, how the mode is chosen, and what errors look like." >}}
{{< /cards >}}
