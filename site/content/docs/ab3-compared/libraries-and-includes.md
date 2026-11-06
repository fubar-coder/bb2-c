---
title: Libraries, tokens and includes
weight: 3
description: Which libraries and token list AB3 mode compiles with, how userlibs and residents are found, and how includes of an AmiBlitz3 program work.
---

Before this page you should know where AB3 mode finds AmiBlitz3's files
([Setting up AmiBlitz3's files]({{< relref "/docs/ab3-mode/setup" >}})).

## AmiBlitz3's libraries, not blitz2's

In AB3 mode blitz2-ab3 loads AmiBlitz3's four library files from
`Blitz3:BlitzLibs/` instead of blitz2's `acidlibs` and `deflibs`, and then
every file in `Blitz3:BlitzLibs/UserLibs/`. The files are used unchanged,
as AmiBlitz3 ships them. The two sets are never mixed: one run of
blitz2-ab3 compiles one program, with the libraries of its mode only.

Two rules differ from blitz2:

* **The userlibs are one flat directory**, without the subdirectories
  blitz2 uses.
* **A userlib replaces a library with the same number.** In blitz2 two
  libraries must not share a number; in AB3 mode, as in AmiBlitz3, a
  userlib can take the place of a library of the four files.

## AmiBlitz3's token list

The source is tokenised and compiled with AmiBlitz3's token list: its
compiler's own words, which differ from Blitz Basic 2's in a few places,
followed by the commands of the loaded libraries. For the same libraries
the list is the same as AmiBlitz3's own, so a command has the same token
in both. This is why a source tokenised by blitz2's editor cannot be used
in an AB3 program, and the reverse
([Tokenised sources]({{< relref "/docs/how-blitz2-works/tokens" >}})).

## Residents

A resident named in the source's embedded header without a path, such as
`all.res`, is loaded from `Blitz3:BlitzLibs/Residents/`. A name with a
device or a directory is used as it is. A resident that cannot be loaded
stops the compile with "Can't Load Resident" and its name.

## Includes

An AmiBlitz3 program usually includes text files: `INCLUDE "file"` or
`XINCLUDE "file"`. In AB3 mode:

* **The kind of file comes from its name.** A name ending in `ab3` or
  `ab2` is read as text and tokenised; one ending in `bb2` or `.bb` is
  read as a tokenised file. Any other name is the error "You can only
  include tokenized files".
* **The file is searched in AmiBlitz3's order**: in the includes
  directory `Blitz3:Sourcecodes/Includes`, the directory of the main
  source, the directory of the current include, and the includes
  directory again with the name's file part only. A path set with
  `INCDIR` is put in front of the name. In each
  place the name is tried as given, then with its extension replaced by
  `.ab3`, `.ab2` and `.bb2`, then with these appended. So
  `INCLUDE "mylib"` finds `mylib.ab3`.
* **`XINCLUDE`** of a file that is already included is skipped; `INCLUDE`
  includes it again. A file that includes itself, directly or through
  others, is an error.
* **A text include is tokenised in the program's mode.** A tokenised
  include whose mode differs from the program's stops the compile with an
  error that names it.

An error in an include is reported with the include's name and its line.
