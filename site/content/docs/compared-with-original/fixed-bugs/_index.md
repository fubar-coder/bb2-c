---
title: Fixed original bugs
weight: 1
description: The bugs of Blitz Basic 2 that the BB2 C port fixes, with examples of the ones an ordinary program can run into.
prev: /docs/compared-with-original
next: /docs/compared-with-original/fixed-bugs/compiler
---

Before this page you should know what the chapter means by a difference
([Differences from the original]({{< relref "/docs/compared-with-original" >}})).

The BB2 C port fixes the bugs of Blitz Basic 2 one at a time, each with
its own issue on GitHub and a test that checks the correct behaviour. A
fix changes only the case that was wrong: a program that never ran into
the bug behaves as before, and in most cases is compiled to exactly the
same executable.

Most of the fixed bugs need unusual conditions to show: a name of more
than 127 characters, a library file that cannot be read, a system with
256 MB of free memory. A few of them, though, strike in ordinary
programs, with a wrong result or a crash and no error message. If one of
your programs behaved strangely with Blitz Basic 2, look at these first.
Each of them is shown with a short program, what the original made of
it, and what the port does.

## Bugs an ordinary program can meet

In the compiler:

* [The operator `=<` meant "not equal"]({{< relref "/docs/compared-with-original/fixed-bugs/compiler#less-or-equal" >}}):
  `If a=<b` was true whenever `a` and `b` differed.
* [`End Statement` could close a Function]({{< relref "/docs/compared-with-original/fixed-bugs/compiler#end-of-procedure" >}}):
  a string Function ended that way crashed when it returned.
* [A program with the Blitz OS commands hung at its end]({{< relref "/docs/compared-with-original/fixed-bugs/compiler#os-commands-end" >}}):
  even if it never called one of them.

In the libraries:

* [`Date$` showed 1 January after a leap year as 32 December]({{< relref "/docs/compared-with-original/fixed-bugs/libraries#date-after-leap-year" >}}):
  1 January 2025 came out as `32/12/2024`.

## All fixed bugs, by area

Every fixed bug is on one of the following pages, either with an example
or as one sentence with a link to its issue. The issue describes the bug
in detail and names the test that checks the correct behaviour. Nearly
all of these tests run with the normal emulator tests, so a fix that is
undone later is noticed.

{{< cards >}}
  {{< card link="compiler" title="Compiling" subtitle="Bugs in what the compiler makes of a source, from operators and arrays to include files and macros." >}}
  {{< card link="running-blitz2" title="Running blitz2" subtitle="Bugs of blitz2 as a program: loading the libraries, starting and ending, and the editor's compiler menu." >}}
  {{< card link="libraries" title="Libraries" subtitle="Bugs of the command libraries, of the error handling without the debugger, of AmigaOS calls and of the RWE userlibs." >}}
{{< /cards >}}

Some fixes make the compiler refuse a source that the original accepted,
or compile it to something else. Those sources, and how to change them,
are collected on their own page
([Sources that compile differently]({{< relref "/docs/compared-with-original/new-errors" >}})).
The bugs that are not fixed yet are on another page
([Bugs not fixed yet]({{< relref "/docs/compared-with-original/bugs-not-fixed-yet" >}})).
