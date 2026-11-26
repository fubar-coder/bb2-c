---
title: Important Fixes & Breaking Changes
weight: 1
width: wide
description: All fixed bugs of Blitz Basic 2 in one table, sorted by what they can change in an existing program, with the few fixes that change correct programs first.
prev: /docs/compared-with-original
aliases: [/docs/compared-with-original/fixed-bugs/]
---

Before this page you should know what the chapter means by a fixed bug
([Differences from the original]({{< relref "/docs/compared-with-original" >}})).

The BB2 C port fixes the bugs of Blitz Basic 2 one at a time, each with
its own issue on GitHub and a test that checks the correct behaviour. A
fix changes only the case that was wrong: a program that never ran into
the bug behaves as before, and in most cases is compiled to exactly the
same executable. Nearly all of these fixes only matter to a program that
ran into the bug: it crashed, hung or got a wrong result, and with the
port it does not. A few fixes, though, can change a program that worked.
This page lists every fixed bug in one table, so that you can see which
fixes can concern your sources without reading all the pages of fixed
bugs.

{{< fix-overview part="breaking" >}}

## Bugs an ordinary program can meet

Most of the fixed bugs need unusual conditions to show: a name of more
than 127 characters, a library file that cannot be read, a system with
256 MB of free memory. The bugs below, though, strike in ordinary
programs, with a wrong result or a crash and no error message. If one of
your programs behaved strangely with Blitz Basic 2, look at these first.
Each of them is shown on its page with a short program, what the
original made of it, and what the port does.

In the compiler ([Compiling]({{< relref "/docs/compared-with-original/compiler" >}})):

* [Numbers like `1.` and `1.0e5`]({{< relref "/docs/compared-with-original/compiler#float-literals" >}})
  were refused.
* [Arrays with 256-byte elements]({{< relref "/docs/compared-with-original/compiler#arrays-256" >}})
  had their elements 64 KB apart.
* [A List array indexed by another array]({{< relref "/docs/compared-with-original/compiler#list-array-index" >}}),
  as in `a(b(1))`, read the wrong element.
* [A field of a NewType defined later]({{< relref "/docs/compared-with-original/compiler#newtype-defined-later" >}})
  got a wrong size.
* [`End Statement` could close a Function]({{< relref "/docs/compared-with-original/compiler#end-of-procedure" >}}),
  and a string Function ended that way crashed.
* [An AmigaOS function inside a calculation]({{< relref "/docs/compared-with-original/compiler#amigaos-call-in-expression" >}}),
  as in `a+(b+(c+Write_(…)))`, overwrote values of the calculation.
* [Object numbers above a raised maximum]({{< relref "/docs/compared-with-original/compiler#object-numbers-above-32k" >}}),
  such as Shape 1025, addressed memory before the object table.
* [Text after `IncDir "…"`]({{< relref "/docs/compared-with-original/compiler#text-after-path" >}}),
  such as `: a=2`, was never compiled.
* [`IncDir` stayed for the next compile]({{< relref "/docs/compared-with-original/compiler#incdir-next-compile" >}})
  in the same editor session.
* [An include name with a device after `IncDir`]({{< relref "/docs/compared-with-original/compiler#include-with-device" >}})
  could not be opened.
* [A program with the Blitz OS commands hung at its end]({{< relref "/docs/compared-with-original/compiler#os-commands-end" >}}),
  even if it never called one of them.
* [Data of odd length at the end of a program]({{< relref "/docs/compared-with-original/compiler#odd-end" >}})
  crashed it at `End`.

In the libraries ([Libraries]({{< relref "/docs/compared-with-original/libraries" >}})):

* [`Date$` showed 1 January after a leap year as 32 December]({{< relref "/docs/compared-with-original/libraries/commands/datelib#date-after-leap-year" >}}):
  1 January 2025 came out as `32/12/2024`.
* [Quicks were printed with a stray point or one too low]({{< relref "/docs/compared-with-original/libraries/commands/printlib#quick-print" >}}):
  2.99998 came out as `2.`.
* [`Format` rounded wrongly and lost the minus sign]({{< relref "/docs/compared-with-original/libraries/commands/printlib#format" >}}):
  -7 with `"###"` came out as `7`.
* [Umlauts were not drawn]({{< relref "/docs/compared-with-original/libraries/commands/fontlib#umlauts" >}})
  by the text commands.
* [Text after a string function lost its last character]({{< relref "/docs/compared-with-original/libraries#text-after-string-function" >}}):
  `FileRequest$(...)+".bak"` or `ProgDir$+"/data"` held a 0 byte.
* [`Loc` and `Lof` were one too high after `Eof`]({{< relref "/docs/compared-with-original/libraries/commands/fileiolib#loc-lof-after-eof" >}}):
  a file of 3 bytes had the length 4.
* [A GadTools gadget took over the values of the one before]({{< relref "/docs/compared-with-original/libraries/commands/gtlib#gadtools-values" >}}).
* [Closing a window lost the events of the others]({{< relref "/docs/compared-with-original/libraries/commands/windowslib#closewindow-events" >}}).
* [`WSize` and `WMove` hung]({{< relref "/docs/compared-with-original/libraries/commands/windowslib#wsize-wmove" >}})
  at the size limits of a window or the edge of the screen.
* [Window titles were never freed]({{< relref "/docs/compared-with-original/libraries/commands/windowslib#window-titles" >}}).
* [Decoded palettes were black on Kickstart before 3.0]({{< relref "/docs/compared-with-original/libraries/commands/palettelib#palette-kickstart-13" >}}).
* [A missing AmigaOS library hung the program at its start]({{< relref "/docs/compared-with-original/libraries/amigaos#missing-os-library" >}}).
* [`NextArg$` did not end a quoted word after `""`]({{< relref "/docs/compared-with-original/libraries/userlibs/rimisclib#nextarg-doubled-quote" >}}):
  `"a""b" c` gave the one word `a"b c`.
* [The tracker library did not play in CIA mode]({{< relref "/docs/compared-with-original/libraries/userlibs/ciatrklib#tracker-cia-mode" >}}):
  a program that played a Protracker module crashed on a 68000 at its
  start, or stayed silent.
* [Samples without a loop ended in a buzz]({{< relref "/docs/compared-with-original/libraries/userlibs/ciatrklib#tracker-sample-end" >}})
  in Protracker modules.
* [Voices left out by `TrackerChannels` played the wrong notes]({{< relref "/docs/compared-with-original/libraries/userlibs/ritrackerlib#trackerchannels-voices" >}}):
  each voice after a left-out one played the notes of the voice before
  it.
* [`BlockToShape` copied only part of a wide shape]({{< relref "/docs/compared-with-original/libraries/userlibs/bb_blitlib#blocktoshape-width" >}}):
  of a shape 32 pixels wide, only the first half of its planes.
* [`RTEZGetLong` offered the number of the call before]({{< relref "/docs/compared-with-original/libraries/userlibs/neilsreqtoolslib#rtezgetlong-previous" >}}),
  and returned it when the user cancelled.
* [`BankLimit n` returned an address]({{< relref "/docs/compared-with-original/libraries/userlibs/riamosfunclib#banklimit-result" >}})
  instead of the number of banks.
* [`FindVolume` returned an address instead of -1]({{< relref "/docs/compared-with-original/libraries/userlibs/riamosfunclib#findvolume-result" >}}),
  so a comparison with -1 never found the volume.
* [`PutIconObject` reported a failed write as success]({{< relref "/docs/compared-with-original/libraries/userlibs/ritooltypeslib#puticonobject-result" >}})
  and dropped the icon.
* [`SetToolValue` replaced a tool type whose name only began with the name given]({{< relref "/docs/compared-with-original/libraries/userlibs/ritooltypeslib#settoolvalue-prefix" >}}):
  `SetToolValue "FL","x"` replaced `FLAG`.
* [`StoreNVData` returned 0 for a stored item]({{< relref "/docs/compared-with-original/libraries/userlibs/rinonvolatilelib#storenvdata-result" >}}),
  and an error code, a true value, when it could not store it.

In the editor ([Editor]({{< relref "/docs/compared-with-original/editor" >}})):

* [BLOCK UNTAB removed text, not only indentation]({{< relref "/docs/compared-with-original/editor#block-untab" >}}):
  lines not indented by the tab stop lost their first characters.
* [The last line of a text source was not tokenised]({{< relref "/docs/compared-with-original/editor#last-line-not-tokenised" >}})
  when the file had no line feed at its end.
* [Umlauts in a text source joined lines]({{< relref "/docs/compared-with-original/editor#umlauts-in-text-source" >}})
  and were shown as keywords.
* [JOIN of two long lines lost the end of the text]({{< relref "/docs/compared-with-original/editor#join-long-lines" >}})
  when they were longer than 128 characters together.
* [Accented letters broke saved blocks, copied blocks and printouts]({{< relref "/docs/compared-with-original/editor#latin1-without-tokenise" >}})
  in the editor on its own.
* [ß, é and other accented letters were lost]({{< relref "/docs/compared-with-original/editor#latin1-in-strings" >}}):
  only the six umlauts could be typed into a string.
* [A tab stop of 0 crashed the editor]({{< relref "/docs/compared-with-original/editor#tab-stop-zero" >}}),
  and the options turned every value of 80 and more into 0.
* [The editor did not start with the small font on a 512-row Workbench]({{< relref "/docs/compared-with-original/editor#small-font-screen" >}}).

## The five levels

Every fix has one of five levels. The level answers one question: can a
program that ran correctly with Blitz Basic 2 behave differently with the
port, and how hard is that to notice before it happens? A new runtime
error ranks above a new compile error, since the compiler shows the one
at once and the other only comes when the program runs into it.

{{< fix-overview part="levels" >}}

## All fixes

Each row names the topic and the component the fix is in: blitz2 for the
compiler, also for blitz2 as a program (loading the libraries, starting
and ending), ted for the editor, the name of the library for a command
library or an RWE userlib, and the AmigaOS library, such as
`exec.library`, for a call of an AmigaOS function. The Fix column gives
the commands concerned and one short sentence; the sentence leads to the
description of the fix, and the issue to its details and its test on
GitHub. The table starts
sorted by level and, inside a level, by issue number. With JavaScript,
the column headers sort the table and the two lists above it show only
one level or one topic.

{{< fix-overview part="table" >}}
