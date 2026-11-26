---
title: Differences from the original
weight: 6
description: Where the BB2 C port behaves differently from Blitz Basic 2.10, and how to find out for yourself.
prev: /docs/libraries
next: /docs/compared-with-original/important-fixes
---

The BB2 C port is Blitz Basic 2.10, not a new version of it, and its goal
is Blitz Basic 2.10 without the bugs of the original. The translation is
complete: a program compiled with the port gives the same results as
with the original compiler, the libraries do what the original libraries
do, and the editor behaves like the original editor. The executables
themselves may differ, where the compiler makes faster code, such as
four functions it compiles inline. On that base the bugs of the original
are fixed one at a time, area by area. A program that never ran into one
of them behaves as it did with the original.

The port has no mode that keeps the original's bugs. If you need the
original behaviour, bug for bug, use the original compiler and libraries.

Every difference is written down, and every known bug of the original
that is not fixed yet has an issue and, in most cases, a test. This
chapter tells you what the differences are and how to check the port
against the original yourself.

## What differs

* **Fixed bugs.** Bugs of the original are fixed deliberately, each one
  on its own, with its own issue and a test of the correct behaviour. A
  program that ran into such a bug behaves differently with the port,
  usually in the way it was always meant to.
  Most fixes concern rare cases: an error without the debugger, a value at
  the edge of its range, a resource that is missing.
* **The expression type system.** The port decides the type of every
  operation in an expression by its own operands and the type the result
  is wanted in, where the original went from left to right. Programs that
  mix Quicks with other types can get other, mostly more exact, results.
* **New errors.** A few fixes make the port refuse a source that the
  original compiled, mostly with a wrong or random result, or stop a
  program with a runtime error where the original went on with a wrong
  argument. Each is described with its fix, with a program and what to
  change; the table of
  [Important Fixes & Breaking Changes]({{< relref "/docs/compared-with-original/important-fixes" >}}?level=3,4#all-fixes)
  shows them as levels 3 and 4.
* **Extensions.** The compiler understands a little more than the
  original, but only where the original refused the source. A source that
  the original compiled is never given a new meaning. It also compiles
  `Int`, `Frac`, `Len` and `Asc` faster where it can, with the same
  results. `LoadShape` loads animations that the original stopped with
  an error.
* **Inside the port.** The C code differs from the original assembler code
  in many details that no program can see. These are listed for developers
  and are not part of this chapter.
* **Registers for assembler code.** A library command keeps the registers
  Mark Sibly's library documentation promises: a4, a5, a6 and a7, and a
  function also a3. Other registers, such as d0, d1, a0 and a1, may be
  changed even where the original happened to keep them. A Basic program
  cannot see this; inline assembler around a command and assembler
  libraries that call a command can. The commands that known libraries
  and programs rely on keep their registers as before (EventWindow,
  EMouseX, EMouseY, EventCode, RawStatus, InitSound, Rnd, WBWidth,
  WBHeight, WBDepth).

## The editor

The port's editor is TED 1.1, the editor of Blitz Basic 2, translated
like the compiler. It is checked against a build of the original editor's
source: both are given the same key presses and their screens, requesters
and saved files are compared. Apart from the fixed bugs of the original
([Editor]({{< relref "/docs/compared-with-original/editor" >}})),
the differences that exist are internal and cannot be seen from the
outside.

## The full lists

The pages of this chapter describe the differences a program can notice.
The complete lists, including the internal ones, are kept for developers
in the repository: the
[deviations of the compiler](https://github.com/fubar-coder/bb2-c/blob/main/docs/blitz2/deviations.md),
the [deviations of the libraries](https://github.com/fubar-coder/bb2-c/blob/main/docs/libs/deviations.md)
and the
[deviations of the editor](https://github.com/fubar-coder/bb2-c/blob/main/docs/ted/deviations.md).
Every fixed bug has an issue on GitHub with the details and the test
that shows the fix; the pages link them.

{{< cards >}}
  {{< card link="important-fixes" title="Important Fixes & Breaking Changes" subtitle="All fixed bugs in one table, by what they can change in an existing program." >}}
  {{< card link="language" title="Language" subtitle="Fixed bugs in how the compiler reads a source: arrays, NewTypes, procedures, names, include files and macros." >}}
  {{< card link="expressions" title="Expressions" subtitle="Fixed bugs in the value of an expression: operators, numbers, comparisons and Quicks in Long computations." >}}
  {{< card link="expressions/type-system" title="Expression type system" subtitle="How the original and the port type each operation of an expression, with the results that differ, by example." >}}
  {{< card link="compiled-programs" title="Compiled programs" subtitle="Fixed bugs in the generated program: AmigaOS functions and objects, the end of a program, the executable." >}}
  {{< card link="running-blitz2" title="Running blitz2" subtitle="Fixed bugs of blitz2 as a program: loading the libraries, starting and ending, its memory, and the editor's compiler menu." >}}
  {{< card link="editor" title="Editor" subtitle="Fixed bugs of the editor TED: blocks, JOIN, accented letters, loading, saving, options and low memory." >}}
  {{< card link="libraries" title="Libraries" subtitle="Fixed bugs of the command libraries, of AmigaOS calls and of the RWE userlibs, one page per library." >}}
  {{< card link="extensions" title="Extensions" subtitle="What the compiler and the libraries accept beyond Blitz Basic 2." >}}
  {{< card link="bugs-not-fixed-yet" title="Bugs not fixed yet" subtitle="The known bugs of the original that the port still shows, and how they are tracked and tested." >}}
  {{< card link="comparing" title="Comparing with the original yourself" subtitle="Compile your own program with both compilers and compare the results." >}}
{{< /cards >}}
