---
title: How-to
weight: 9
description: Recipes for tasks with the port's tools and sources - library variants and the RWE userlibs, AmiBlitz3 sources from the Shell, readable and converted sources, and running the tests.
prev: /docs/ab3-compared/differences-from-amiblitz3
next: /docs/how-to/library-variant
---

The pages of this chapter are recipes: each one takes a single task you
may have with the BB2 C port, its tools or Blitz Basic sources, and shows
the commands that do it, with an example you can follow on your own
machine. They assume that you have built the port
([Building the BB2 C port]({{< relref "/docs/introduction/building" >}})).
Where a recipe needs background, it links the chapter that explains it.

The first three recipes are about compiling: with the libraries a source
needs, and with AmiBlitz3's libraries. The next two turn sources into
text you can read, and the last one runs the port's tests.

{{< cards >}}
  {{< card link="library-variant" title="Build and use a library variant" subtitle="Choose the variant a source needs and make a blitz2 directory with its libraries." >}}
  {{< card link="install-rwe-userlibs" title="Install the RWE userlibs" subtitle="Where blitz2 looks for userlibs, and which acidlibs and deflibs the RWE userlibs need." >}}
  {{< card link="ab3-from-the-shell" title="Compile an AmiBlitz3 source from the Shell" subtitle="Compile and run an AmiBlitz3 source with blitz2-ab3, in a script or from the Linux host." >}}
  {{< card link="readable-source" title="Make a tokenised source readable" subtitle="Turn a .bb2 file into plain text with bbsrc or tedsrc, with the right libraries." >}}
  {{< card link="bb22-source" title="Convert a BB22 source of RWE" subtitle="Turn a library source in the BB22 format into one the editor and the compiler read." >}}
  {{< card link="run-tests" title="Run a single test or one preset" subtitle="Run the tests with CTest, find a test's name, and see where results and logs go." >}}
{{< /cards >}}
