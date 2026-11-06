---
title: AB3 compared with blitz2
weight: 6
description: What changes when blitz2-ab3 compiles a source in AB3 mode, what is not supported yet, and where it differs from AmiBlitz3 itself.
prev: /docs/ab3-mode/command-line
next: /docs/ab3-compared/language
---

AmiBlitz3 grew out of Blitz Basic 2, so most of a Blitz Basic 2 program
is also an AmiBlitz3 program. But over the years AmiBlitz3 added to the
language, renumbered and extended the libraries, and changed some rules.
This chapter describes what AB3 mode does differently from blitz2, so
that you know what to expect when you compile an AmiBlitz3 source with
blitz2-ab3.

One rule holds throughout: all of this applies only in AB3 mode. A Blitz
Basic 2 source, compiled by blitz2 or by blitz2-ab3 in BB2 mode, is
compiled exactly as before, to the same executable.

AB3 mode follows AmiBlitz3 v3.14.0. Where AmiBlitz3's own compiler has a
bug, AB3 mode does what AmiBlitz3 obviously means, and the last page of
this chapter lists those cases.

{{< cards >}}
  {{< card link="language" title="Language additions" subtitle="New syntax, line joining, escapes in strings, character literals and AmiBlitz3's procedures." >}}
  {{< card link="directives" title="Directives: OPTIMIZE, SYNTAX and runtime errors" subtitle="AmiBlitz3's compiler directives and what AB3 mode does with them." >}}
  {{< card link="libraries-and-includes" title="Libraries, tokens and includes" subtitle="AmiBlitz3's libraries and token list, and how includes are found." >}}
  {{< card link="not-supported" title="What AB3 mode does not support yet" subtitle="The AmiBlitz3 features that are missing, and how the compiler tells you." >}}
  {{< card link="differences-from-amiblitz3" title="Where AB3 mode differs from AmiBlitz3" subtitle="Cases where AmiBlitz3's own compiler gives another result, and why." >}}
{{< /cards >}}
