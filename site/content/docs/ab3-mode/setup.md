---
title: Setting up AmiBlitz3's files
weight: 1
description: The AmiBlitz3 libraries and residents that AB3 mode needs, where to get them, and the Blitz3 assign.
prev: /docs/ab3-mode
---

Before this page you should know what AB3 mode is
([AB3 mode]({{< relref "/docs/ab3-mode" >}})) and have the port's
`blitz2` directory in your Amiga or emulator.

AB3 mode compiles with AmiBlitz3's own library files. They are not part of
the BB2 C port; you take them from AmiBlitz3 itself.

## Which version

AB3 mode is made for **AmiBlitz3 v3.14.0** and tested with its files. Use
that release: the libraries of another version have other commands and
other token numbers, and a source written for one does not fit the other
([Tokenised sources]({{< relref "/docs/how-blitz2-works/tokens" >}})).

AmiBlitz3 is free software under the Apache License 2.0. Its release
archive is on GitHub:
[AmiBlitz3 v3.14.0](https://github.com/AmiBlitz/AmiBlitz3/releases/tag/v3.14.0),
the file `AmiBlitz3.14.lha`. The library files in it are the same as in
the repository at that release.

## What AB3 mode reads

AB3 mode looks for everything below the assign `Blitz3:`, in the same
places as AmiBlitz3:

{{< filetree/container >}}
  {{< filetree/folder name="Blitz3:" >}}
    {{< filetree/folder name="BlitzLibs" >}}
      {{< filetree/file name="baseLibs" >}}
      {{< filetree/file name="utilLibs" >}}
      {{< filetree/file name="OsLibs" >}}
      {{< filetree/file name="csLibs" >}}
      {{< filetree/folder name="UserLibs" state="closed" >}}
      {{< /filetree/folder >}}
      {{< filetree/folder name="Residents" >}}
        {{< filetree/file name="all.res" >}}
      {{< /filetree/folder >}}
    {{< /filetree/folder >}}
    {{< filetree/folder name="Sourcecodes" state="closed" >}}
      {{< filetree/folder name="Includes" state="closed" >}}
      {{< /filetree/folder >}}
    {{< /filetree/folder >}}
  {{< /filetree/folder >}}
{{< /filetree/container >}}

* **The four library files** `baseLibs`, `utilLibs`, `OsLibs` and
  `csLibs` replace blitz2's `acidlibs` and `deflibs`. All four must be
  there; if one is missing, the compile stops with "Library file not
  found:" and the name of the file.
* **`UserLibs/`** holds further libraries, all in this one directory (not
  in subdirectories as with blitz2). It may be missing or empty. A
  library here with the number of a library in the four files replaces
  that library, as in AmiBlitz3.
* **`Residents/`** is where a resident named in a source's header without
  a path is looked for. Many AmiBlitz3 sources name `all.res`.
* **`Sourcecodes/Includes/`** is where AmiBlitz3's include files are
  looked for, if a source includes them.

{{% steps %}}

### Unpack AmiBlitz3

Unpack `AmiBlitz3.14.lha` on the Amiga, or on the host into the directory
your emulator shows as a drive. It creates the directory `AmiBlitz3.14`
with `BlitzLibs`, `Sourcecodes` and the rest of AmiBlitz3.

### Assign Blitz3:

In the Shell, assign `Blitz3:` to that directory, for example

```
Assign Blitz3: Work:AmiBlitz3.14
```

Put the line into `S:User-Startup` to have it after every start.

### Keep blitz2: as it is

blitz2-ab3 stays in the port's directory, next to the helper program
`ab3cli`, which it loads from its own directory. For Blitz Basic 2
sources it uses `blitz2:` and the libraries there, exactly as blitz2
does, so keep that assign as well.

{{% /steps %}}
