---
title: Convert a BB22 source of RWE
weight: 5
description: Some library sources that Red When Excited released are saved in the BB22 format, which neither blitz2 nor its editor reads. Recognise such a file, convert it into the editor's format with bbsrc bb22, and make it readable.
---

Nine of the library sources that Red When Excited released in 1999 are
not saved in the format of Blitz Basic 2's editor. They begin with the
four letters `BB22`, followed by the position of the cursor and the
window, and every line has a small prefix. Neither blitz2 nor its editor
reads such a file. The tool **bbsrc** converts it into the editor's
format, a tokenised source that the editor loads and the compiler
compiles like any other. After that, the source can be turned into plain
text as every other source
([Make a tokenised source readable]({{< relref "readable-source" >}})).

The nine files are in the source archive of Red When Excited's libraries,
which `cmake -P cmake/fetch-originals.cmake` downloads into
`archive/aminet/amibl2src/rwelibsrc/`:

* `ANIMLib/Anim-Library3.bb` and its `.bak`
* `AppLib/applib3.bb` and its `.bak`
* `Registry/Registry3.bb`
* `rexxlib/NEWAREXXLIB.bb`
* `shapeslib/RIShapesLib2.bb`
* `soundlib/risoundlib2.bb` and its `.bak`

## Recognise a BB22 file

The first four bytes of the file are `BB22`:

```
$ head -c 4 archive/aminet/amibl2src/rwelibsrc/AppLib/applib3.bb
BB22
```

`bbsrc bb22-info` reads the header and tells more: the version of the
format, where the cursor stood when the file was saved, and the number
and length of the lines:

```
$ R=archive/aminet/amibl2src/rwelibsrc
$ build/amiga/host/tools/bbsrc/bbsrc bb22-info $R/AppLib/applib3.bb
version 10 (header 16 bytes, line prefix 2)
cursor: column 13, row 25, top line 441 (line 466, 0-based)
lines 791, longest 64 bytes
```

A file in any other format is refused, with the reason:

```
$ build/amiga/host/tools/bbsrc/bbsrc bb22-info $R/AppLib/applib2.bb
archive/aminet/amibl2src/rwelibsrc/AppLib/applib2.bb: offset 0: no "BB22", not a BB22 file
```

## Convert it

{{% steps %}}

### Run bbsrc bb22

`bbsrc bb22` takes the BB22 file and the name of the file to write:

```
build/amiga/host/tools/bbsrc/bbsrc bb22 $R/AppLib/applib3.bb applib3.bb
```

The result is a tokenised source in the editor's format, with the same
lines. The cursor position of the header is not part of that format and
is left out; `bbsrc bb22-info` shows it if you want to know where the
author stopped. bbsrc refuses a file it does not fully understand and
then writes nothing, so a file it writes is complete.

### Make it readable

The converted file is detokenised with the compiler and the libraries,
as on [Make a tokenised source readable]({{< relref "readable-source" >}}).
For the RWE sources, take the `acidlibs` and `deflibs` of the variant
rwe-1999:

```
v=build/amiga/variants/rwe-1999
build/amiga/host/ted-tools/tedsrc/tedsrc detok \
    --blitz2 build/amiga/blitz2/blitz2 \
    --lib $v/acidlibs --lib $v/deflibs \
    applib3.bb applib3.txt
```

The text begins as the editor shows the source:

```
#ExecBase=4

 INCDIR "Blitzlibs:"
 XINCLUDE "libmacs.bb"

!libheader {#riapplib,init,0,finit,0}

!acommand {#long}
!args {#push|#long}
```

### Add the libraries a source calls

Some of the sources use commands of other RWE userlibs. For those,
tedsrc names every line with a token it does not know, and writes no
text:

```
$ build/amiga/host/tools/bbsrc/bbsrc bb22 $R/shapeslib/RIShapesLib2.bb RIShapesLib2.bb
$ build/amiga/host/ted-tools/tedsrc/tedsrc detok --blitz2 build/amiga/blitz2/blitz2 \
    --lib $v/acidlibs --lib $v/deflibs RIShapesLib2.bb rishapeslib2.txt
tedsrc detok: RIShapesLib2.bb:785: unknown token $A488
tedsrc detok: RIShapesLib2.bb:958: unknown token $A481
tedsrc detok: rishapeslib2.txt: not written
```

The token tells the library: `$A488` is `$8000 + 73 × 128 + 8`, a
command of library 73, which the command reference lists as ripacklib
([Find the user libraries a source needs]({{< relref "readable-source#find-the-user-libraries-a-source-needs" >}})).
With that library added after `deflibs`, every token is known:

```
build/amiga/host/ted-tools/tedsrc/tedsrc detok --blitz2 build/amiga/blitz2/blitz2 \
    --lib $v/acidlibs --lib $v/deflibs --lib $v/userlibs/rwe/ripacklib.obj \
    RIShapesLib2.bb rishapeslib2.txt
```

{{% /steps %}}

To work on the source in the editor, copy the converted file to the
Amiga instead. The original's `.xtra` file, with the compiler options,
is beside it in the archive (`applib3.bb.xtra`) and is in the format
blitz2 reads; copy it along under the same name.

## All RWE library sources at once

The sources of the RWE userlibs that the port builds are also available
as text, five of the BB22 files among them, made by one build target:

```
cmake --build --preset amiga --target rwelibsrc
```

It writes one text file per source into `build/amiga/rwelibsrc/`, with
the same directories as the archive, for example
`build/amiga/rwelibsrc/shapeslib/RIShapesLib2.bb`. It takes for each
source the libraries of its variant, so no token stays unknown. It needs
only the source archive from `fetch-originals.cmake`, not the Blitz
Basic 2 CD.

## Options of bbsrc bb22 and bb22-info

| Command | Effect |
|---|---|
| `bbsrc bb22 IN OUT` | converts the BB22 file `IN` into the editor's format and writes it to `OUT` |
| `bbsrc bb22-info IN` | prints the version, the cursor of the header, the number of lines and the longest line of `IN` |

Both exit with status 0 on success, 1 if `IN` cannot be read or is
refused or `OUT` cannot be written, and 2 for a wrong command line. The
manual of bbsrc is in the repository:
[bbsrc](https://github.com/fubar-coder/bb2-c/blob/main/tools/bbsrc/README.md).
