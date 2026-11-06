---
title: Documentation
---

The BB2 C port is a translation of Blitz Basic 2 into C: the compiler, the editor and
the libraries that provide the commands. The chapters below explain how to
build and use it, how it works inside, and where it differs from the
original. They are in reading order: the first chapters are for everyone who
wants to use the port, the last ones for people who write libraries or work
on the port itself.

{{< cards >}}
  {{< card link="introduction" title="Introduction" subtitle="What the BB2 C port is, how to build it and how to try it in an emulator." >}}
  {{< card link="how-blitz2-works" title="How blitz2 works" subtitle="A program on its way from source text to an executable." >}}
  {{< card link="libraries" title="Libraries" subtitle="The commands each library provides." >}}
  {{< card link="compared-with-original" title="Differences from the original" subtitle="Where the port deliberately differs from Blitz Basic 2, for example fixed bugs." >}}
  {{< card link="ab3-mode" title="AB3 mode" subtitle="Compiling AmiBlitz3 sources." >}}
  {{< card link="ab3-compared" title="AB3 compared with blitz2" subtitle="What the AB3 mode changes." >}}
  {{< card link="blitz2-internals" title="blitz2 internals" subtitle="Writing libraries in C and reading the compiler's source." >}}
  {{< card link="project-structure" title="Project structure" subtitle="The parts of the project, the tests and the checks of the build." >}}
{{< /cards >}}
