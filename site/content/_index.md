---
title: bb2-c
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
    title="The original, in C"
    subtitle="Blitz Basic 2 was the BASIC many Amiga programmers used for games and tools in the 1990s. bb2-c translates the compiler, the editor and the libraries into C."
    icon="code"
  >}}
  {{< hextra/feature-card
    title="Same programs as before"
    subtitle="The translation behaves like the original. Where it differs on purpose, for example a fixed bug, the documentation says so."
    icon="badge-check"
    link="docs/compared-with-original"
  >}}
  {{< hextra/feature-card
    title="Built with a current toolchain"
    subtitle="The result runs on the Amiga (68000) and is built with a modern cross compiler instead of an assembler."
    icon="chip"
    link="docs/introduction"
  >}}
  {{< hextra/feature-card
    title="How it works"
    subtitle="How the project is laid out, and how the compiler turns a program into an executable."
    icon="light-bulb"
    link="docs/how-blitz2-works"
  >}}
  {{< hextra/feature-card
    title="Libraries"
    subtitle="How libraries are built, and what the commands of each library do."
    icon="collection"
    link="docs/libraries"
  >}}
  {{< hextra/feature-card
    title="AB3 mode"
    subtitle="A second mode of the compiler that understands the sources of AmiBlitz3."
    icon="switch-horizontal"
    link="docs/ab3-mode"
  >}}
{{< /hextra/feature-grid >}}
