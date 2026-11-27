---
title: User libraries
weight: 3
description: The commands of the userlibs that the BB2 C port builds - the RWE userlibs and the test library testlib - one page per library, with their forms and argument types.
---

The port builds two kinds of userlibs:

* **The RWE userlibs** are the libraries of Reflective Images and others
  whose sources Red When Excited released in 1999.
  [The RWE userlibs]({{< relref "/docs/blitz2-internals/rwe-userlibs" >}})
  explains which variant they need and how to install them.
* **testlib**, user library 81, is the port's test library: the
  routines behind the macros that a
  [test that judges itself]({{< relref "/docs/project-structure/self-judging-tests" >}})
  checks its results with. Only the tests use it;
  [The test library]({{< relref "/docs/project-structure/test-library" >}})
  describes its macros.

Each page below belongs to one of these libraries and lists its commands
in alphabetical order, in the same form as the
[command libraries]({{< relref "/docs/libraries/commands" >}}), whose
page also explains the notation. The library numbers are those of the
variant the library is built for.

## The libraries

{{< library-list what="commands" >}}
