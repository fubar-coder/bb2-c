---
title: Fixed original bugs
weight: 1
description: The bugs of Blitz Basic 2 that the BB2 C port fixes deliberately, and which programs notice the difference.
prev: /docs/compared-with-original
---

Before this page you should know what the chapter means by a difference
([Differences from the original]({{< relref "/docs/compared-with-original" >}})).

The bugs of the original are fixed one at a time, area by area; this page
lists the ones fixed so far. Each fix changes only the case that was
wrong; a program that never ran into the bug behaves exactly as before,
and in most cases is compiled to exactly the same executable. Every entry
links its issue, which describes the bug in detail and names the test
that checks the correct behaviour. Nearly all of these tests run with the normal
emulator tests, so a fix that is undone later is noticed. The bugs that
are not fixed yet are on their own page
([Bugs not fixed yet]({{< relref "/docs/compared-with-original/bugs-not-fixed-yet" >}})).

## The compiler

These fixes change what the compiler does or the code it generates.

| Effect of the bug in the original | With the port |
|---|---|
| [Large executables did not load on AmigaOS 2.0 and later (#943)](https://github.com/fubar-coder/bb2-c/issues/943): with very many relocations the executable's relocation table had a form newer AmigaOS versions refuse. | The table is split into groups AmigaOS accepts. Only such very large programs are written differently. |
| [Calls of AmigaOS functions with an argument in register a4 or a5 crashed (#958)](https://github.com/fubar-coder/bb2-c/issues/958), or destroyed the program's variables, because those registers hold the variable bases. | The compiler saves a4 and a5 around such a call. Only these few calls get different code. |
| [A string addition from the seventh argument of a library command on got a wrong length (#971)](https://github.com/fubar-coder/bb2-c/issues/971), for example the text of a window; also a string addition nested that deeply. | The length is right. |
| [Pad bytes contained random values (#969)](https://github.com/fubar-coder/bb2-c/issues/969): the bytes skipped by `Even`, by the alignment of data and code, and the space reserved with `Ds` held whatever the compiler's memory held. | They are zero, so they no longer depend on the compiler's memory. |
| [A constant defined without a value held a random value (#972)](https://github.com/fubar-coder/bb2-c/issues/972): a line `#c` alone was accepted, and `#c` could be anything. | Its value is 0. |
| [A NewType that contained a field of its own type got a random size (#973)](https://github.com/fubar-coder/bb2-c/issues/973). | The compiler reports "Illegal Type"; a pointer to the own type stays allowed. |
| [A `~` inside a string literal started a constant expression (#966)](https://github.com/fubar-coder/bb2-c/issues/966): `"a~1+2~b"` became `a3b`, a single `~` in a string an error. | Inside strings `~` is an ordinary character. |
| [The Macro Buffer option had no effect, and blitz2 could crash when it ended (#336)](https://github.com/fubar-coder/bb2-c/issues/336) if a source had another Macro Buffer size. | The option sets the largest macro text, and the buffer is freed correctly. |
| [An error inside a macro definition could crash the compiler (#965)](https://github.com/fubar-coder/bb2-c/issues/965) instead of showing the error. | The error is shown. |
| [The last constant of a resident pointed behind the resident (#34)](https://github.com/fubar-coder/bb2-c/issues/34), so every compile freed memory that did not belong to it and could crash the system. | The list ends where it should. |
| [Resident files contained bytes nobody had set (#970)](https://github.com/fubar-coder/bb2-c/issues/970), so CREATE RESIDENT gave a different file each time. | Those bytes are zero; the same source gives the same resident. |
| [`IncBin` kept a file or directory in use after an error (#38)](https://github.com/fubar-coder/bb2-c/issues/38). | The file is closed; the errors are the same. |
| [On a 68010 processor the trap vectors were taken from the wrong table (#42)](https://github.com/fubar-coder/bb2-c/issues/42) when the system had moved them, which affects runtime errors and the debugger. | The moved table is used, on the 68010 as on later processors. |
| [Started from the Workbench, blitz2 did not go back to its start directory when it ended (#21)](https://github.com/fubar-coder/bb2-c/issues/21): it ended in its own directory, or in that of the project icon it was started with. | The start-up code restores the start directory when blitz2 ends. |
| [Started from the Workbench, blitz2 could crash at its very end (#337)](https://github.com/fubar-coder/bb2-c/issues/337): it let the Workbench unload it before it had returned, which goes wrong when the freed memory is reused at once or a memory checker such as MungWall overwrites it. | blitz2 has ended before the Workbench unloads it. |

## Runtime errors without the debugger

Every library has an area of error checks that is left out when a
program is compiled without runtime error checking. Several libraries had
error handling that jumped into that area, or kept the text of an error
message there. Without runtime error checking, such an error jumped to
memory behind the library, or reported a message that pointed into
nowhere. The port puts this code and these messages where they belong, so
the error is reported with its proper text in both cases. With runtime
error checking on, nothing changes.

The fixes concern:

* [memory errors, "Out of Memory!" (#956)](https://github.com/fubar-coder/bb2-c/issues/956)
* [fonts, "Can't Open Font" and others (#955)](https://github.com/fubar-coder/bb2-c/issues/955)
* [screens, "Can't open Screen" and others (#251)](https://github.com/fubar-coder/bb2-c/issues/251)
* [displays and copper lists (#412)](https://github.com/fubar-coder/bb2-c/issues/412)
* [shapes, `DecodeShapes` of address 0 (#270)](https://github.com/fubar-coder/bb2-c/issues/270) and [the shape error messages (#953)](https://github.com/fubar-coder/bb2-c/issues/953)
* [sprite files (#954)](https://github.com/fubar-coder/bb2-c/issues/954)
* [`SetPeriod` without a sound, and the sound error messages (#70)](https://github.com/fubar-coder/bb2-c/issues/70)
* [animations, "Can't Open Anim File" (#398)](https://github.com/fubar-coder/bb2-c/issues/398)
* [music modules, "Error Accessing Module File" (#957)](https://github.com/fubar-coder/bb2-c/issues/957)
* [IFF pictures (#192)](https://github.com/fubar-coder/bb2-c/issues/192) and [saving IFF files, "Can't open File" (#185)](https://github.com/fubar-coder/bb2-c/issues/185)
* [Workbench arguments without memory at the start of a program (#305)](https://github.com/fubar-coder/bb2-c/issues/305)

## Command libraries

| Effect of the bug in the original | With the port |
|---|---|
| [`Dim List` with size 0 wrote behind its memory block (#59)](https://github.com/fubar-coder/bb2-c/issues/59). | Nothing is written behind the block. |
| [`Date$` showed the 1st of January after a leap year as "32/12" (#120)](https://github.com/fubar-coder/bb2-c/issues/120), and the day, month and year functions were wrong for that day. | The date is the 1st of January. |
| [Characters far above the font's first character were not drawn (#148)](https://github.com/fubar-coder/bb2-c/issues/148), among them the umlauts in the standard font. | They are drawn. |
| [`GTPalette` with a colour ignored the colour and broke a following tag list (#168)](https://github.com/fubar-coder/bb2-c/issues/168). | The colour is set. |
| [GadTools gadgets took over optional values of an earlier gadget (#169)](https://github.com/fubar-coder/bb2-c/issues/169): a gadget created without, say, its start value got the previous gadget's. | Without the value, a gadget starts with the default. |
| [All `GTShape` gadgets showed the images of the last one (#173)](https://github.com/fubar-coder/bb2-c/issues/173), and GadTools' own image was overwritten. | Each gadget keeps its own images. |
| [`GTShape` wrote to low memory when the gadget could not be created (#327)](https://github.com/fubar-coder/bb2-c/issues/327). | A failed gadget is left alone. |
| [Cycle and MX gadgets had room for a limited number of labels without a check, and the memory was never freed (#174)](https://github.com/fubar-coder/bb2-c/issues/174). | The labels are counted, and the memory is freed with the gadget. |
| [Faded and decoded palettes were wrong or black on Kickstart before 3.0 (#224)](https://github.com/fubar-coder/bb2-c/issues/224). | The colours are right. |
| [A screen mode with a monitor part, such as PAL or NTSC, opened on the default monitor (#250)](https://github.com/fubar-coder/bb2-c/issues/250). | The whole mode is used. |
| [`WindowTags` with a long tag list overwrote code of the library (#317)](https://github.com/fubar-coder/bb2-c/issues/317). | A long list is copied only in part, the rest is linked. |
| [Window titles were copied and never freed (#318)](https://github.com/fubar-coder/bb2-c/issues/318): every window and every new title cost memory until the program ended. | The copies are freed with the window or the next title. |
| [`WMove` and `WSize` hung when Intuition limited the new position or size (#320)](https://github.com/fubar-coder/bb2-c/issues/320). | They wait only briefly. |
| [Closing one window discarded the events of all other windows (#322)](https://github.com/fubar-coder/bb2-c/issues/322). | Only the closed window's events are discarded. |

## AmigaOS calls

| Effect of the bug in the original | With the port |
|---|---|
| [A program hung at its start if an AmigaOS library it calls was missing (#44)](https://github.com/fubar-coder/bb2-c/issues/44). | It starts; only a call of the missing library then crashes. |
| [A program that calls a resource, such as the battery clock or the CIA resources, hung at its start (#45)](https://github.com/fubar-coder/bb2-c/issues/45). | The resource is opened. |
| [`CachePreDMA_` and `CachePostDMA_` passed their flags in the wrong register (#959)](https://github.com/fubar-coder/bb2-c/issues/959). | The flags reach the system. |

## RWE userlibs

These fixes concern the
[RWE userlibs]({{< relref "/docs/libraries/userlibs" >}}).

| Effect of the bug in the original | With the port |
|---|---|
| [`RIAnimInit`, `RIAnimDepth`, `RIAnimWidth` and `RIAnimHeight` crashed on some data that is not a `FORM ANIM` (#513)](https://github.com/fubar-coder/bb2-c/issues/513), such as an IFF `LIST` of type `ANIM`: the chunk search returned with an unbalanced stack. | The commands return normally; for such a `LIST`, which holds no animation they can read, `RIAnimDepth` returns 0. |
