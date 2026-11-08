---
title: Compiling from the command line
weight: 2
description: The options of blitz2-ab3, how it chooses between BB2 and AB3 mode, where the compile options come from, and what errors look like.
next: /docs/ab3-compared
---

Before this page you should have AmiBlitz3's files set up
([Setting up AmiBlitz3's files]({{< relref "/docs/ab3-mode/setup" >}})).

From the Shell, blitz2-ab3 compiles one source to an executable without
opening the editor. The option names are those of AmiBlitz3's own
command line, so scripts written for AmiBlitz3 work with it.

## A first compile

Save this as a text file `hello.ab3`:

```
a$ = Trim$("  Hello  ")
NPrint "[", a$, "]"
End
```

`Trim$` exists only in AmiBlitz3's libraries. Compile and run it:

```
1> blitz2:blitz2-ab3 -s hello.ab3 -e RAM:hello
1> RAM:hello
[Hello]
```

The name ends in `.ab3`, so blitz2-ab3 compiled it in AB3 mode, with
AmiBlitz3's libraries. Force BB2 mode on the same file and the compiler no
longer knows `Trim$`:

```
1> blitz2:blitz2-ab3 -s hello.ab3 -e RAM:hello -mode bb2
hello.ab3:1: Array not yet Dim'd
```

## The options

```
blitz2-ab3 -s SOURCE -e EXE [-r] [-mode ab3|bb2] [-w LIST] [-werr LIST]
```

| Option | Meaning |
|---|---|
| `-s`, `-source` *file* | The source to compile: a text file, or a source tokenised by the editor. |
| `-e`, `-exe` *file* | The executable to write. |
| `-r`, `-release` | A release executable: no runtime error checking, no debugger, no debug information. |
| `-mode` *ab3* or *bb2* | Compile in this mode, whatever the source looks like. This option is an addition of the port; AmiBlitz3 does not have it. |
| `-w` *list* | The warnings that are shown ([Warnings](#warnings)). An addition of the port. |
| `-werr` *list* | The warnings that are errors ([Warnings](#warnings)). An addition of the port. |
| `-h`, `-help` | Show the options. |

AmiBlitz3's other options, such as `-d`, `-run`, `-dump` or `-save`, are
refused with a message that names the option; nothing is compiled.

Started without options, or with only a file name, blitz2-ab3 opens the
editor, exactly like blitz2, and the editor works in BB2 mode.

The command line needs Kickstart 2.0 or newer, and the program `ab3cli`
must be in the same directory as blitz2-ab3. Both are in the port's
`blitz2` directory after the build.

## How the mode is chosen

The main source decides the mode for the whole program, including every
file it includes:

* **AB3 mode** for a text source whose first line is AmiBlitz3's embedded
  header (`; XTRA`), for any source whose name ends in `.ab3`, and for a
  tokenised source whose `.xtra` file has AmiBlitz3's format.
* **BB2 mode** for every other source.
* `-mode` overrides both.

A text file included by the program is tokenised in the program's mode. A
*tokenised* include of the other mode cannot be read correctly, because
its tokens mean other commands: the compile stops with an error that names
the file.

## Where the compile options come from

AmiBlitz3 keeps the options of a text source in its embedded header, a
block of comment lines at the start between `; XTRA` and `; /XTRA`, with
lines such as `; RuntimeDebug = 1` or `; Residents = "all.res"`. blitz2-ab3
takes the options from there. A source without a header takes them from
its `.xtra` file, if it has one, and otherwise uses the compiler's
defaults. `-r` switches runtime error checking off in any case.

Relative file names, of the source, the executable and the includes, are
relative to the Shell's current directory.

## Errors and return codes

A compile error is printed with the file name, the line and the message,
and no executable is written. Inside an include, the include's name and
line are given. For example, a source `broken.bb` with a syntax error in
its second line:

```
1> blitz2:blitz2-ab3 -s broken.bb -e RAM:broken
broken.bb:2: Syntax Error
```

The return code tells a script what happened:

| Return code | Meaning |
|---|---|
| 0 | Compiled, the executable is written. |
| 5 | Compiled with warnings, the executable is written. |
| 10 | The source did not compile: an error in it, a warning that is an error, or a library file or resident that cannot be loaded. |
| 20 | Wrong or unsupported options, or a source file that does not exist. |

As with blitz2, only the first error is reported.

{{< callout type="info" >}}
Try it: compile `hello.ab3` once as above and once with `-r`, and compare
the sizes of the two executables. Then break a line of it and compile it
again to see the error.
{{< /callout >}}

## Warnings

Besides errors, the compiler knows warnings: something in the source
that is probably a mistake, but does not stop the compile and does not
change the executable. Each warning has a name. There is one so far,
`resclash`: two residents define the same constant with different
values, and the value of the first resident is used
([The editor and the compiler]({{< relref "/docs/how-blitz2-works/editor-and-compiler#warnings" >}})).

A warning is printed with the name of the source, the word `warning`
and, at the end, the name of the warning. For a clash of residents, the
two resident files are named as well. This source `wclash.ab3` loads two
residents that both define `#c`, the first as 1, the second as 2:

```
; XTRA
; Residents       = "TEST:src/wclash1.res,TEST:src/wclash2.res"
; /XTRA
NPrint #c
End
```

Compiled, it gives the warning, and the program prints 1:

```
1> blitz2:blitz2-ab3 -s wclash.ab3 -e RAM:wclash
wclash.ab3: warning: Clash in Residents: #c (TEST:src/wclash1.res, TEST:src/wclash2.res) [resclash]
```

The executable is written, and the return code is 5, the AmigaDOS code
for a warning. A script continues after it with the Shell's default
`FailAt 10`; one that sets `FailAt 5` or lower stops there.

Which warnings are shown, and which of them are errors, starts from the
source's own setting, **WARNINGS** in the editor's compiler options; a
source without that setting shows all warnings and makes none of them an
error. Two options change it for this compile:

* `-w` *list* chooses the warnings that are shown.
* `-werr` *list* chooses the warnings that are errors. A warning that is
  not shown is never an error.

A list is made of items separated by commas, applied from left to right:
`all` and `none` replace the set, a name adds that warning to it, and a
name with a minus in front, such as `-resclash`, removes it. Several
`-w` or `-werr` options are applied one after the other. For example:

| Options | Effect |
|---|---|
| `-w -resclash` or `-w all,-resclash` | The clash of residents is not reported. |
| `-w none` | No warnings are reported. |
| `-werr resclash` | The clash of residents is an error. |
| `-werr all` | Every warning that is shown is an error. |

A warning that is an error is printed like an error, and nothing is
compiled; the return code is 10:

```
1> blitz2:blitz2-ab3 -s wclash.ab3 -e RAM:wclash -werr resclash
wclash.ab3: Clash in Residents: #c
```

An unknown name or an empty item in a list is an option error, with
return code 20: blitz2-ab3 names the item, lists the known names and
shows its options.
