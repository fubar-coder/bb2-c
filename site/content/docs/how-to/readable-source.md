---
title: Make a tokenised source readable
weight: 1
description: Turn a tokenised Blitz Basic 2 or AmiBlitz source into plain text on your own machine with bbsrc or tedsrc - which tool, which libraries, and how to find the user libraries a source needs.
prev: /docs/how-to
next: /docs/blitz2-internals
---

A Blitz Basic 2 source file is not plain text: the editor stores every
keyword and library command as a two-byte token
([Tokenised sources]({{< relref "/docs/how-blitz2-works/tokens" >}})).
Opened in an ordinary text editor, a `.bb2` file shows the names of your
variables and your strings, but no commands. The BB2 C port has two
tools that turn such a file into text on your Linux machine, without an
Amiga or an emulator: **bbsrc** and **tedsrc**. This page shows which one
to use, how to give it the right libraries, and what to do when a source
was written with libraries you do not have yet.

## The two tools

Both tools are built with the port. After the normal build
([Building the BB2 C port]({{< relref "/docs/introduction/building" >}})):

```
cmake --preset amiga && cmake --build --preset amiga
```

they are here, together with the compiler and the libraries they need:

```
build/amiga/host/tools/bbsrc/bbsrc            bbsrc
build/amiga/host/ted-tools/tedsrc/tedsrc      tedsrc
build/amiga/blitz2/                           blitz2, blitz2-ab3, acidlibs, deflibs
build/amiga/variants/                         the library variants and their userlibs
```

A token is only a number. To turn it back into a name, a tool needs the
same list of names the editor has, and the editor gets that list from the
compiler and its libraries when blitz2 starts. So both tools take the
compiler and the library files, and build the list from them the way
blitz2 does. They differ in what they do with the source:

| | `tedsrc detok` | `bbsrc detok` |
|---|---|---|
| Result | the text exactly as the editor shows it | the text line for line as it is in the file |
| Unknown token | an error: every such line is reported, no text is written | written as `{L:N}` and counted, the rest is written |
| Tabs, long lines | tabs become spaces (tab stop 2), a line wider than 128 columns is split as the editor splits it | tabs and every other character stay as they are, no line is split |
| Sources | Blitz Basic 2 sources | also sources of other editors: AmiBlitz 2, AmiBlitz3, PED |

Use **tedsrc** when you want to see a Blitz Basic 2 source as the editor
would show it, and to be sure that every token is known. Use **bbsrc**
for everything else: to read a source of unknown origin, to find out
which libraries it needs, or to read an AmiBlitz source. In both, line N
of the text is line N of the source as the editor numbers it, as long as
tedsrc splits no line.

The text is written in ISO 8859-1 (Latin-1), each line ended by a line
feed. On a terminal that expects UTF-8, convert it for viewing, for
example with `iconv -f latin1 -t utf-8 OUT.txt`.

## Detokenise a Blitz Basic 2 source

The examples use sources of the original Blitz Basic 2 CD, which are not
in the repository. One command downloads the CD and other original
material into `archive/`:

```
cmake -P cmake/fetch-originals.cmake
```

Any source of your own works the same way.

{{% steps %}}

### Name the compiler and the libraries

`--blitz2` names the compiler whose built-in words are read, each `--lib`
a library file. For a source written with the libraries of Blitz Basic 2,
these are the port's own `blitz2`, `acidlibs` and `deflibs`.

### Run tedsrc

`tedsrc detok` takes the source and the file to write:

```
CD=archive/cd/ultimate-blitz-basic-2.1
build/amiga/host/ted-tools/tedsrc/tedsrc detok \
    --blitz2 build/amiga/blitz2/blitz2 \
    --lib build/amiga/blitz2/acidlibs --lib build/amiga/blitz2/deflibs \
    $CD/BLITZ2_PDDISK_1/ZOMBIES/ZOMBIES.BB2 zombies.txt
```

The text begins as the editor shows the source:

```
;
; Zombie Appocalypse by Paul Andrews
;

WBStartup:For i=1 To 100:CloseWorkBench_:Next

NEWTYPE .obj
```

With `--check` instead of the output file, tedsrc only checks one or more
sources and writes nothing; its exit status is 0 if every token of every
file is known.

### Or run bbsrc

`bbsrc detok` takes the same libraries. Without an output file it writes
the text to the terminal:

```
build/amiga/host/tools/bbsrc/bbsrc detok \
    --blitz2 build/amiga/blitz2/blitz2 \
    --lib build/amiga/blitz2/acidlibs --lib build/amiga/blitz2/deflibs \
    $CD/BLITZ2_PDDISK_1/ZOMBIES/ZOMBIES.BB2 | less
```

{{% /steps %}}

## The order of the libraries

Give the files in the order blitz2 loads them: first the compiler, then
`acidlibs`, then `deflibs`, then the user libraries. If two files have a
command with the same token number, the file named first wins. Both
tools read single library files and BigLib files, which hold
many libraries in one file as `acidlibs` and `deflibs` do.

A source written with a
[library variant]({{< relref "/docs/blitz2-internals/variants" >}}) needs
the `acidlibs` and `deflibs` of that variant, since the variant gives some
commands other numbers. The compiler stays the same. The build puts each
variant under `build/amiga/variants/<name>/`:

| Directory | What it holds |
|---|---|
| `build/amiga/blitz2/` | `acidlibs` and `deflibs` of the default variant, bb2-2.10 |
| `variants/bb2-2.10/userlibs/` | `literal/literallib.obj`, `testlib/testlib.obj` |
| `variants/rwe-1999/` | `acidlibs`, `deflibs` and the RWE userlibs in `userlibs/rwe/` |
| `variants/rwe-1999-eval/` | `acidlibs`, `deflibs` and `userlibs/rwe/rievallib.obj` |
| `variants/rwe-1999-ahi/` | `acidlibs`, `deflibs` and `userlibs/rwe/risoundlib2.obj` |

The two variants rwe-1999-eval and rwe-1999-ahi hold only the one library
that needs them; the other RWE userlibs are those of rwe-1999
([The RWE userlibs]({{< relref "/docs/blitz2-internals/rwe-userlibs" >}})).

## Find the user libraries a source needs

Many sources on the CD and elsewhere use user libraries. With the default
libraries alone, bbsrc writes their commands as `{L:N}` and reports how
many it did not know. `--unknown` lists every such token; `> /dev/null`
leaves out the text itself:

```
f=$CD/BSS1/REDSHAPEZ/DOCUMENTATION/EXAMPLES/SERVER_WBICON.BB2
build/amiga/host/tools/bbsrc/bbsrc detok --unknown \
    --blitz2 build/amiga/blitz2/blitz2 \
    --lib build/amiga/blitz2/acidlibs --lib build/amiga/blitz2/deflibs \
    $f > /dev/null
```

```
bbsrc detok: .../SERVER_WBICON.BB2: 25 of 110 tokens unknown (14 different numbers)
{46:3} $9703 1
{46:4} $9704 1
{54:1} $9B01 2
{59:1} $9D81 2
...
{133:5} $C285 9
{133:14} $C28E 1
```

Each line is one token: `{L:N}`, the token as a hexadecimal number, and
how often it occurs. **L is the library number**, N the position of the
command in that library, counted from 1. The token is
`$8000 + L × 128 + N`; `{133:5}` is `$8000 + 133 × 128 + 5 = $C285`. In
the text, the line reads

```
      {133:5} 0,"ShapeZ Icon Server v1.00"+Chr$(10)+"By Nigel Hughes"
```

The library numbers lead to the libraries. The command reference has a
table of every library with its number:
[RWE userlibs]({{< relref "/docs/libraries/userlibs" >}}),
[Command libraries]({{< relref "/docs/libraries/commands" >}}) and
[AmigaOS libraries]({{< relref "/docs/libraries/amigaos" >}}). This source
needs the libraries 46, 54, 59 and 133: rishapeslib, neilsreqtoolslib,
ritooltypeslib and riarexxlib, all RWE userlibs of the variant rwe-1999.
Add them after `deflibs`, and with `--strict` bbsrc fails if any token is
still unknown:

```
v=build/amiga/variants/rwe-1999
build/amiga/host/tools/bbsrc/bbsrc detok --strict \
    --blitz2 build/amiga/blitz2/blitz2 \
    --lib $v/acidlibs --lib $v/deflibs \
    --lib $v/userlibs/rwe/rishapeslib.obj --lib $v/userlibs/rwe/neilsreqtoolslib.obj \
    --lib $v/userlibs/rwe/ritooltypeslib.obj --lib $v/userlibs/rwe/riarexxlib.obj \
    $f server_wbicon.txt
```

Now every command has its name:

```
      RXReplyMsg 0,"ShapeZ Icon Server v1.00"+Chr$(10)+"By Nigel Hughes"
```

tedsrc takes the same `--lib` options. Without the user libraries it writes
no text and reports every line with an unknown token instead,
for example `SERVER_WBICON.BB2:16: unknown token $C281`; the token tells
the library in the same way.

{{< callout type="info" >}}
A library number that no table lists belongs to a library the port does
not build. Such a source needs that library's file, from wherever the
author of the source got it; give it with `--lib` like the others.
{{< /callout >}}

## Make a token list once and reuse it

Building the list from a dozen files for every source is slow to type.
`bbsrc tokens` writes the list to a file, one token and its name per
line; both tools read such a file instead of `--blitz2` and `--lib`. The
files go in the same order:

```
v=build/amiga/variants/rwe-1999
build/amiga/host/tools/bbsrc/bbsrc tokens -o rwe-tokens.txt \
    build/amiga/blitz2/blitz2 $v/acidlibs $v/deflibs $v/userlibs/rwe/*.obj
```

The file begins with the compiler's own words:

```
8001 NEWTYPE
8002 End
8003 Let
```

bbsrc takes it with `--tokens`, tedsrc as the first file:

```
build/amiga/host/tools/bbsrc/bbsrc detok --tokens rwe-tokens.txt $f server_wbicon.txt
build/amiga/host/ted-tools/tedsrc/tedsrc detok rwe-tokens.txt $f server_wbicon.txt
```

`bbsrc tokens` is also the way to compare two sets of libraries: two
lists made from different variants differ in the commands whose numbers
differ.

## AmiBlitz 2 and AmiBlitz3 sources

AmiBlitz 2 and AmiBlitz3 renumbered many libraries and added a great many
commands, so their sources need their own compiler and libraries, not the
port's. bbsrc reads them, tedsrc does not.

**AmiBlitz 2 and AmiBlitz3 3.00** sources resolve with the compiler and
libraries of AmiBlitz 2.42. Its distribution from Aminet is part of what
`cmake -P cmake/fetch-originals.cmake` downloads, into
`archive/aminet/ab2.42-distri/Amiblitz2/`. The snapshots of AmiBlitz
sources in `archive/sources/` are in the repository:

```
d=archive/aminet/ab2.42-distri/Amiblitz2
build/amiga/host/tools/bbsrc/bbsrc detok \
    --blitz2 $d/Amiblitz2 --lib $d/AcidLibs --lib $d/Deflibs \
    archive/sources/amiblitz2-2.41/ped.bb2 ped.txt
```

With the port's `blitz2`, `acidlibs` and `deflibs` instead, the same
source keeps hundreds of unknown tokens, and some of the compiler's
built-in words read wrong (`Jimi` instead of `Syntax`), since AmiBlitz 2
numbered them differently too.

**AmiBlitz3 v3.14.0** sources use the token list of the port's second
compiler, blitz2-ab3, with AmiBlitz3's four BigLib files `baseLibs`,
`utilLibs`, `OsLibs` and `csLibs` (also downloaded by
`fetch-originals.cmake`, into `archive/amiblitz3/github-58a11658/BlitzLibs/`;
see [AB3 mode]({{< relref "/docs/ab3-mode" >}})). blitz2-ab3 holds two
lists of built-in words, one for each of its modes. `--last 80DB` before
`--blitz2` picks AmiBlitz3's, the one whose last word has the token
`$80DB`; without it bbsrc refuses the compiler:

```
a=archive/amiblitz3/github-58a11658/BlitzLibs
build/amiga/host/tools/bbsrc/bbsrc detok \
    --last 80DB --blitz2 build/amiga/blitz2/blitz2-ab3 \
    --lib $a/baseLibs --lib $a/utilLibs --lib $a/OsLibs --lib $a/csLibs \
    SOURCE.bb2 source.txt
```

`bbsrc tokens` takes `--last 80DB` in the same way.

## Accented letters: the `--ted` rules

AmiBlitz stores accented letters in strings and comments as plain
ISO 8859-1 characters, bytes of `$80` and above that Blitz Basic 2's
editor would take for tokens. By default bbsrc follows AmiBlitz's rules:
strings and comments are text, and only in a comment is a known token
still read as a token, since that is a line that was commented out.
`--ted` applies the editor's rules instead, where every byte from `$80`
starts a token. On an AmiBlitz source, `--ted` turns letters into
nonsense tokens:

```
Request "File envarc:ted.menus nicht gefunden","Menued stürzt ab ...       default
Request "File envarc:ted.menus nicht gefunden","Menued st{248:114}zt ab ... --ted
```

Blitz Basic 2's editor stores such a letter as a *literal character*
instead, the byte `$90` followed by the letter's code. In a string, the
default rules leave these two bytes as they are, while `--ted` writes the
letter, as tedsrc does. So for a Blitz Basic 2 source with accented
letters in its strings, use `--ted` or tedsrc. For the sources of the
Blitz Basic 2 CD, which hardly use such letters, the two rules give the
same text in all but one file.

## Options of bbsrc detok

| Option | Effect |
|---|---|
| `--blitz2 FILE` | the built-in words of this compiler |
| `--last XXXX` | for the next `--blitz2`: its list whose last token is `$XXXX` (blitz2-ab3: `80DB`) |
| `--lib FILE` | a library or BigLib file; repeat it in the compiler's order |
| `--tokens FILE` | a token list written by `bbsrc tokens` |
| `--unknown` | list every unknown token on the error output: `{L:N} $XXXX count` |
| `--strict` | fail if a token is unknown, instead of writing `{L:N}` |
| `--ted` | the editor's rules: every byte from `$80` starts a token |

At least one of `--tokens`, `--blitz2` and `--lib` is needed. The exit
status is 0 on success and 1 if a file cannot be read, a token list is
refused, the source ends in the middle of a token, `--strict` finds an
unknown token, or the output cannot be written; a half-written output
file is then removed. `bbsrc detok --help` and `tedsrc detok --help` show
the full usage. The manuals of both tools are in the repository:
[bbsrc](https://github.com/fubar-coder/bb2-c/blob/main/tools/bbsrc/README.md)
and
[tedsrc](https://github.com/fubar-coder/bb2-c/blob/main/programs/ted/tools/tedsrc/README.md).
