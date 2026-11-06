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

{{< hextra/feature-grid >}}
  {{< hextra/feature-card
    title="Introduction"
    subtitle="What the BB2 C port is, how to build it and how to try the results in an emulator."
    icon="play"
    link="docs/introduction"
  >}}
  {{< hextra/feature-card
    title="How blitz2 works"
    subtitle="A program on its way through the compiler, from the source text to an executable."
    icon="light-bulb"
    link="docs/how-blitz2-works"
  >}}
  {{< hextra/feature-card
    title="blitz2 internals"
    subtitle="How the libraries the compiler reads are built, and the library variants."
    icon="cog"
    link="docs/blitz2-internals"
  >}}
  {{< hextra/feature-card
    title="Libraries"
    subtitle="The commands each library provides."
    icon="collection"
    link="docs/libraries"
  >}}
  {{< hextra/feature-card
    title="AB3 mode"
    subtitle="A second mode of the compiler that compiles the sources of AmiBlitz3."
    icon="switch-horizontal"
    link="docs/ab3-mode"
  >}}
  {{< hextra/feature-card
    title="Differences from the original"
    subtitle="The port behaves like Blitz Basic 2. Where it deliberately differs, for example a fixed bug, it is listed here."
    icon="scale"
    link="docs/compared-with-original"
  >}}
{{< /hextra/feature-grid >}}
