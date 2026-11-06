---
title: BB2 C port
layout: hextra-home
---

<div class="hx:mt-6 hx:mb-6">
{{< hextra/hero-headline >}}
  Blitz Basic 2, rebuilt in C
{{< /hextra/hero-headline >}}
</div>

<div class="hx:mb-12">
{{< hextra/hero-subtitle >}}
  The Blitz Basic 2 compiler, its editor and its libraries,&nbsp;<br class="hx:sm:block hx:hidden" />translated to C and built for the Amiga with a modern toolchain
{{< /hextra/hero-subtitle >}}
</div>

<div class="hx:mb-6">
{{< hextra/hero-button text="Get started" link="docs/introduction" >}}
</div>

<div class="hx:mt-6"></div>

{{< hextra/feature-grid cols="2" >}}
  {{< hextra/feature-card
    title="Building the BB2 C port"
    subtitle="You want to try it: build the compiler, the editor and the libraries from the sources on Linux."
    icon="cog"
    link="docs/introduction/building"
  >}}
  {{< hextra/feature-card
    title="Your first program"
    subtitle="You wrote Blitz Basic 2 programs and want to again: type, run and compile a program in the emulator."
    icon="play"
    link="docs/introduction/first-program"
  >}}
  {{< hextra/feature-card
    title="Writing your own userlib"
    subtitle="You want to add commands: write a Blitz library in C, build it and call it from a program."
    icon="puzzle"
    link="docs/blitz2-internals/own-userlib"
  >}}
  {{< hextra/feature-card
    title="AB3 mode"
    subtitle="You use AmiBlitz3: compile its sources from the Shell with AmiBlitz3's own libraries."
    icon="switch-horizontal"
    link="docs/ab3-mode"
  >}}
{{< /hextra/feature-grid >}}

<div class="hx:mt-16 hx:mb-6">
{{< hextra/hero-section heading="h2" >}}Advanced topics{{< /hextra/hero-section >}}
</div>

{{< hextra/feature-grid cols="2" >}}
  {{< hextra/feature-card
    title="How blitz2 works"
    subtitle="A program on its way through the compiler, from the line you type to the executable."
    icon="light-bulb"
    link="docs/how-blitz2-works"
  >}}
  {{< hextra/feature-card
    title="Differences from the original"
    subtitle="The port behaves like Blitz Basic 2. Where it deliberately differs, for example a fixed bug, it is listed here."
    icon="scale"
    link="docs/compared-with-original"
  >}}
{{< /hextra/feature-grid >}}
