---
title: blitz2 internals
weight: 7
description: Writing libraries in C, library variants, the RWE userlibs, and reading the compiler's source.
prev: /docs/ab3-compared/differences-from-amiblitz3
next: /docs/blitz2-internals/library-anatomy
---

This chapter is for readers who want to go further than writing
programs: to write libraries of their own, to build the libraries with
other numbers, or to read and change the compiler. It builds on the
chapter [How blitz2 works]({{< relref "/docs/how-blitz2-works" >}}), which
you should know first.

The libraries of the BB2 C port are written in C. The chapter explains
how such a library is built up, the rules for writing one, and then takes
you through a library of your own, from the first line to a program that
calls it. The original way of writing Blitz libraries, as Blitz Basic
programs with inline assembler and a set of macros, is not covered: those
macros come with the original Blitz Basic 2 CD and are not part of this
project.

{{< cards >}}
  {{< card link="library-anatomy" title="Anatomy of a Blitz library" subtitle="The header, the table of commands, the code and the error check area." >}}
  {{< card link="c-libraries" title="Libraries in C" subtitle="Compiler options, register parameters, strings, check routines." >}}
  {{< card link="own-userlib" title="Writing your own userlib" subtitle="A tutorial: from an empty file to a program that calls your command." >}}
  {{< card link="variants" title="Library variants" subtitle="The same libraries with other numbers, and how to make a variant." >}}
  {{< card link="rwe-userlibs" title="The RWE userlibs" subtitle="The libraries of Red When Excited and Reflective Images, built as userlibs." >}}
  {{< card link="compiler-source" title="Inside the compiler source" subtitle="How the C source of blitz2 and its editor is organised." >}}
{{< /cards >}}
