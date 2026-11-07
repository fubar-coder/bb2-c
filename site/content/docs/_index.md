---
title: Documentation
---

The BB2 C port is a translation of Blitz Basic 2 into C: the compiler, the editor and
the libraries that provide the commands. The chapters below explain how to
build and use it, how it works inside, and where it differs from the
original. They are in reading order: the first chapters are for everyone who
wants to use the port, the last ones for people who write libraries or work
on the port itself.

## Where to start

* **You wrote Blitz Basic 2 programs and want to again.** Build the port,
  set up the emulator and write your first program
  ([Building the BB2 C port]({{< relref "/docs/introduction/building" >}}),
  [Running it in an emulator]({{< relref "/docs/introduction/emulator" >}}),
  [Your first program]({{< relref "/docs/introduction/first-program" >}})),
  then read [How blitz2 works]({{< relref "/docs/how-blitz2-works" >}})
  and [Differences from the original]({{< relref "/docs/compared-with-original" >}}).
* **You want to work on the port.** After the Introduction, read
  [Project structure]({{< relref "/docs/project-structure" >}}) and its
  [Tests]({{< relref "/docs/project-structure/tests" >}}), then
  [How blitz2 works]({{< relref "/docs/how-blitz2-works" >}}) and
  [Inside the compiler source]({{< relref "/docs/blitz2-internals/compiler-source" >}}),
  and [Comparing with the original yourself]({{< relref "/docs/compared-with-original/comparing" >}}).
* **You want to write a library.** After the Introduction and How blitz2
  works, follow the chapter
  [blitz2 internals]({{< relref "/docs/blitz2-internals" >}}) up to
  [Writing your own userlib]({{< relref "/docs/blitz2-internals/own-userlib" >}})
  and [Library variants]({{< relref "/docs/blitz2-internals/variants" >}}).
* **You use AmiBlitz3.** After the Introduction, read
  [AB3 mode]({{< relref "/docs/ab3-mode" >}}) and
  [AB3 compared with blitz2]({{< relref "/docs/ab3-compared" >}}).

## The chapters

{{< cards >}}
  {{< card link="introduction" title="Introduction" subtitle="What the BB2 C port is, how to build it and how to try it in an emulator." >}}
  {{< card link="how-blitz2-works" title="How blitz2 works" subtitle="A program on its way from source text to an executable." >}}
  {{< card link="libraries" title="Libraries" subtitle="What the libraries provide, and how to look up a command." >}}
  {{< card link="compared-with-original" title="Differences from the original" subtitle="Where the port differs from Blitz Basic 2: the fixed bugs, the bugs not fixed yet and the extensions." >}}
  {{< card link="ab3-mode" title="AB3 mode" subtitle="Compiling AmiBlitz3 sources." >}}
  {{< card link="ab3-compared" title="AB3 compared with blitz2" subtitle="What the AB3 mode changes." >}}
  {{< card link="blitz2-internals" title="blitz2 internals" subtitle="Writing libraries in C and reading the compiler's source." >}}
  {{< card link="project-structure" title="Project structure" subtitle="The parts of the project, the tests and the checks of the build." >}}
{{< /cards >}}
