---
title: Build and use a library variant
weight: 1
description: Choose the library variant a source needs, build it, put its acidlibs, deflibs and userlibs into a blitz2 directory of their own, and use that directory in an emulator or on an Amiga.
prev: /docs/how-to
---

Most sources compile with the libraries the build puts into the `blitz2`
directory. A source written with the RWE userlibs, the libraries of
Reflective Images and others, needs another set: a *library variant*,
the same libraries with other library numbers
([Library variants]({{< relref "/docs/blitz2-internals/variants" >}})
explains why the numbers matter). The build makes every variant of the
port. This page shows which one to take, how to make a `blitz2`
directory with it, and how to use that directory.

## Which variant you need

| Variant | Take it for | What blitz2 loads |
|---|---|---|
| bb2-2.10 | sources of Blitz Basic 2 without RWE userlibs; the default | the `acidlibs` and `deflibs` of the build's `blitz2` directory |
| rwe-1999 | sources that use the RWE userlibs | its own `acidlibs` and `deflibs`, and the RWE userlibs |
| rwe-1999-eval | sources that use the expression evaluator rievallib | its own `acidlibs` and `deflibs`, the RWE userlibs of rwe-1999 and rievallib |
| rwe-1999-ahi | sources that use the newer sound library risoundlib2 | its own `acidlibs` and `deflibs`, the RWE userlibs of rwe-1999 with risoundlib2 instead of risoundlib |

The command reference lists every RWE userlib with its commands
([RWE userlibs]({{< relref "/docs/libraries/userlibs" >}})). If you do
not know which libraries a source uses, bbsrc lists the library numbers
of the commands it does not know
([Find the user libraries a source needs]({{< relref "readable-source#find-the-user-libraries-a-source-needs" >}})).

{{< callout type="warning" >}}
A variant gives some commands other tokens. A source saved by the editor
with one variant shows other commands, or `?????`, when it is loaded with
another. Keep each source together with the variant it was written for.
{{< /callout >}}

## Make a blitz2 directory for the variant

The examples use rwe-1999 and a new directory `~/bb2-rwe/blitz2`. Every
command runs on the host, from the repository's root.

{{% steps %}}

### Build the port

The normal build makes all variants and their userlibs:

```
cmake --preset amiga && cmake --build --preset amiga
```

Each variant other than the default one is in
`build/amiga/variants/<name>/`:

```
$ ls build/amiga/variants/rwe-1999
acidlibs  deflibs  userlibs
```

To build only the libraries of one variant, name its targets:
`cmake --build --preset amiga --target rwe-1999-acidlibs rwe-1999-deflibs`.

### Copy the programs

`cmake --install` writes a complete `blitz2` directory, with the default
libraries, below the directory you give it:

```
cmake --install build/amiga --prefix ~/bb2-rwe
```

The result is `~/bb2-rwe/blitz2/` with `blitz2`, `ted`, `blitz2-ab3`,
`ab3cli`, `acidlibs` and `deflibs`.

### Replace the libraries

Copy the variant's `acidlibs` and `deflibs` over the default ones, and its
userlibs into a directory below `userlibs`:

```
d=~/bb2-rwe/blitz2
v=build/amiga/variants/rwe-1999
cp $v/acidlibs $v/deflibs $d/
mkdir -p $d/userlibs/rwe
cp $v/userlibs/rwe/*.obj $d/userlibs/rwe/
```

For rwe-1999-eval and rwe-1999-ahi, take the `acidlibs` and `deflibs`
from their own directories, and add or replace the one library there
([Install the RWE userlibs]({{< relref "install-rwe-userlibs" >}})).

{{% /steps %}}

Keep the build's own `build/amiga/blitz2` for sources without RWE
userlibs. Two directories side by side are the simplest way to switch:
the assign `blitz2:` decides which one blitz2 uses.

## Use it

### In an emulator

Give the emulated Amiga `~/bb2-rwe/blitz2` as a drive instead of the
build's `blitz2` directory, with the volume name `BB2C`
([Running it in an emulator]({{< relref "/docs/introduction/emulator" >}})).
Then, in the Shell:

```
Assign blitz2: BB2C:
blitz2:blitz2
```

### On an Amiga

Copy the directory to a disk of the Amiga, for example to
`Work:bb2-rwe/`, and assign `blitz2:` to it before you start blitz2:

```
Assign blitz2: Work:bb2-rwe
blitz2:blitz2
```

Put the `Assign` line into `S:User-Startup` to have it after every start.

{{< callout type="info" >}}
blitz2 reads `acidlibs` and `deflibs` only when it starts. After you
change them, or the assign `blitz2:`, quit blitz2 and start it again.
RELOAD ALL LIBS in the COMPILER menu reloads only the userlibs.
{{< /callout >}}

## Check that it works

`Flip$` is a command of the RWE userlib rimisclib; it returns a string
with its characters in reverse order. Save this as a text file
`flip.bb`:

```
NPrint Flip$("Hello")
End
```

Compile and run it from the Shell with blitz2-ab3, which compiles a
Blitz Basic 2 source exactly as blitz2 does:

```
1> blitz2:blitz2-ab3 -s flip.bb -e RAM:flip
1> RAM:flip
olleH
```

With the default libraries the compiler does not know `Flip$` and takes
it for an array:

```
1> blitz2:blitz2-ab3 -s flip.bb -e RAM:flip
flip.bb:1: Array not yet Dim'd
```

The same check runs without a window, from the host, with the runner the
tests use ([Running your own script in the emulator]({{< relref "/docs/project-structure/tests#running-your-own-script-in-the-emulator" >}})).
With `flip.bb` and this `run.script` in a directory `mysources`:

```
Assign blitz2: BB2C:
Cd SRC:
blitz2:blitz2-ab3 >TEST:out.txt -s flip.bb -e RAM:flip
RAM:flip >>TEST:out.txt
```

the runner prints `olleH`:

```
build/amiga/host/tools/amiga_run/amiga_run \
    --mount-ro BB2C=$HOME/bb2-rwe/blitz2 --mount-ro SRC=mysources mysources/run.script
```

To make a variant of your own, with numbers you choose, see
[Making your own]({{< relref "/docs/blitz2-internals/variants#making-your-own" >}}).
