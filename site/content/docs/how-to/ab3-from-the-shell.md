---
title: Compile an AmiBlitz3 source from the Shell
weight: 3
description: Compile and run an AmiBlitz3 source with blitz2-ab3 in AB3 mode - the files it needs, the assigns, a compile of one of AmiBlitz3's own test programs, release executables, scripts and return codes, and a run from the Linux host without a window.
---

blitz2-ab3, the port's second compiler, compiles AmiBlitz3 sources from
the Shell, without the editor: this is *AB3 mode*
([AB3 mode]({{< relref "/docs/ab3-mode" >}})). This page takes one
source through it, from the files you need to a script that compiles
it. Every option is explained on
[Compiling from the command line]({{< relref "/docs/ab3-mode/command-line" >}}).

## What you need

* The port's `blitz2` directory, with `blitz2-ab3` and the helper program
  `ab3cli` side by side. The build puts both into `build/amiga/blitz2/`.
* The library files of **AmiBlitz3 v3.14.0**, unpacked from its release
  archive `AmiBlitz3.14.lha`
  ([Setting up AmiBlitz3's files]({{< relref "/docs/ab3-mode/setup" >}})).
* An Amiga or an emulated one with Kickstart 2.0 or newer.

The example is the source `trim.ab3`, one of AmiBlitz3's own test
programs (`Sourcecodes/Amiblitz3/Tests/` in
[AmiBlitz3's repository](https://github.com/AmiBlitz/AmiBlitz3)). It is a
text file that starts with AmiBlitz3's embedded header, the block of
comment lines between `; XTRA` and `; /XTRA` that holds its compile
options, and ends with a few lines of program:

```
; XTRA
; Embedded .xtra Header
...
; /XTRA
b$ = " hallo   "
NPrint Len(b$)
;a$ = StripTrail$(b$,31)
a$ = Trim$(b$)
NPrint ">" + b$ + "<"
NPrint ">" + a$ + "<   "
End
```

`Trim$` is a command of AmiBlitz3's libraries; Blitz Basic 2 does not
have it.

## Compile and run it

{{% steps %}}

### Make the files visible

In an emulator, give the Amiga three drives: the port's `blitz2`
directory as `BB2C:`, the directory into which you unpacked AmiBlitz3
(it holds `AmiBlitz3.14`) as `AB3:`, and the directory with your sources
as `SRC:`
([Running it in an emulator]({{< relref "/docs/introduction/emulator" >}})
shows how to add a drive). On an Amiga, copy the files to a disk instead.

### Make the assigns

blitz2-ab3 finds the Blitz Basic 2 files through `blitz2:` and
AmiBlitz3's files through `Blitz3:`. In the Shell:

```
Assign blitz2: BB2C:
Assign Blitz3: AB3:AmiBlitz3.14
Cd SRC:
```

`Blitz3:` must be the directory that holds `BlitzLibs`.

### Compile

`-s` names the source, `-e` the executable to write:

```
1> blitz2:blitz2-ab3 -s trim.ab3 -e RAM:trim
```

The source has the embedded header, so blitz2-ab3 compiles it in AB3
mode, with AmiBlitz3's libraries. Without an error it prints nothing.

### Run it

```
1> RAM:trim
9
> hallo   <
>hallo<
```

{{% /steps %}}

## A release executable

The embedded header of `trim.ab3` switches runtime error checking on.
`-r` compiles without it, and without the debugger, whatever the header
says. The executable is smaller:

```
1> blitz2:blitz2-ab3 -s trim.ab3 -e RAM:trim-r -r
1> List RAM:trim RAM:trim-r LFORMAT "%n %l"
trim 12292
trim-r 9704
```

## A source that does not look like AmiBlitz3

blitz2-ab3 chooses AB3 mode for a source with the embedded header, for a
source whose name ends in `.ab3`, and for a tokenised source whose
`.xtra` file is AmiBlitz3's. Any other source is compiled in BB2 mode,
where AmiBlitz3's commands are unknown. A text file `trimtest.bas` with
the program lines alone:

```
b$ = " hallo   "
a$ = Trim$(b$)
NPrint ">" + a$ + "<"
End
```

fails in BB2 mode, since `Trim$` is taken for an array; `-mode ab3`
compiles it:

```
1> blitz2:blitz2-ab3 -s trimtest.bas -e RAM:t2
trimtest.bas:2: Array not yet Dim'd
1> blitz2:blitz2-ab3 -s trimtest.bas -e RAM:t2 -mode ab3
1> RAM:t2
>hallo<
```

## In a script

blitz2-ab3 returns 0 when the executable is written, 5 when it is
written with warnings, 10 when the source does not compile, and 20 for
wrong options or a source that cannot be read. The Shell's `$RC` holds
the code of the last command. An AmigaDOS script stops at the first
command that returns 10 or more, unless it raises the limit with
`FailAt`; this one goes on, and writes every result to a file:

```
FailAt 21
Assign blitz2: BB2C:
Assign Blitz3: AB3:AmiBlitz3.14
Cd SRC:
blitz2:blitz2-ab3 >RAM:out.txt -s trim.ab3 -e RAM:trim
Echo "rc=$RC" >>RAM:out.txt
blitz2:blitz2-ab3 >>RAM:out.txt -s nosuch.ab3 -e RAM:t3
Echo "rc=$RC" >>RAM:out.txt
```

`RAM:out.txt` then reads:

```
rc=0
blitz2-ab3: cannot read the source nosuch.ab3
rc=20
```

## From the Linux host, without a window

The runner the tests use starts the emulator without a window, runs an
AmigaDOS script and prints the files the script wrote to `TEST:`
([Running your own script in the emulator]({{< relref "/docs/project-structure/tests#running-your-own-script-in-the-emulator" >}})).
It needs the emulator set up for the tests. With the sources in a
directory `mysources`, AmiBlitz3 unpacked into `~/ab3` (so that
`~/ab3/AmiBlitz3.14` holds `BlitzLibs`), and this
`mysources/ab3.script`:

```
Assign blitz2: BB2C:
Assign Blitz3: AB3:AmiBlitz3.14
Cd SRC:
blitz2:blitz2-ab3 >TEST:out.txt -s trim.ab3 -e RAM:trim
RAM:trim >>TEST:out.txt
```

the runner compiles and runs the program and prints its output:

```
build/amiga/host/tools/amiga_run/amiga_run --mount-ro BB2C=build/amiga/blitz2 \
    --mount-ro AB3=$HOME/ab3 --mount-ro SRC=mysources mysources/ab3.script
```

## If it does not compile

| Message | Cause |
|---|---|
| `trim.ab3: Library file not found: Blitz3:BlitzLibs/csLibs` (return code 10) | one of AmiBlitz3's four library files is missing in `Blitz3:BlitzLibs/`, or `Blitz3:` is not assigned, or not to the directory that holds `BlitzLibs` (then the first file, `baseLibs`, is named) |
| `trimtest.bas:2: Array not yet Dim'd` at a command of AmiBlitz3 | the source was compiled in BB2 mode: add `-mode ab3` |
| `blitz2-ab3: cannot read the source NAME` | the source does not exist; a relative name is relative to the Shell's current directory |

Only the first error of a source is reported, with its file and line.
