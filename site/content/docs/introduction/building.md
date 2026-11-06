---
title: Building the BB2 C port
weight: 1
description: From a fresh clone to the blitz2 directory - the tools you need, the cross compiler, and the commands.
prev: /docs/introduction
next: /docs/how-blitz2-works
---

Before this page you should know what the port is
([Introduction]({{< relref "/docs/introduction" >}})).

There are no ready-made downloads of the port, so this page is where
everything starts. At the end you have a `blitz2` directory with the
compiler, the editor and the libraries, ready to be copied to an Amiga
or an emulator.

{{< callout type="info" >}}
**What you need**

* A Linux machine on x86-64. This is the platform the port is built and
  tested on.
* CMake 4.2.3 or newer, Ninja, Git, and a C and C++ compiler for your
  machine.
* The cross compiler m68k-amigaos-gcc, version 16.2-rc21 or newer, with
  its assembler vasm.
{{< /callout >}}

{{% steps %}}

### Install the build tools

CMake, Ninja, Git and the host compiler come with every Linux
distribution. On Debian and Ubuntu:

```
sudo apt install cmake ninja-build git gcc g++
```

The port needs CMake 4.2.3 or newer; Ubuntu 26.04 has it. On an older
distribution, such as Ubuntu 24.04, install a newer CMake from Kitware's
binaries at [cmake.org](https://cmake.org/download/), with `pip install
cmake`, or with `snap install cmake --classic`.

### Install the cross compiler

The cross compiler is the m68k-amigaos-gcc of the AmigaPorts project.
Its [releases on GitHub](https://github.com/AmigaPorts/m68k-amigaos-gcc/releases)
have ready archives; take the one for `x86_64-linux` of version 16.2-rc21
or newer. It contains the compiler, the libraries of AmigaOS and the
assembler vasm, which the build needs as well.

Unpack it, for example under `/opt`, and put its `bin` directory into
your `PATH`:

```
sudo tar xJf m68k-amigaos-gcc-16.2-rc23-x86_64-linux.tar.xz -C /opt
export PATH=/opt/m68k-amigaos-gcc-16.2/bin:$PATH
```

Instead of the `PATH`, you can tell the build where the cross compiler is
with `-DAMIGA_TOOLCHAIN_PREFIX=/opt/m68k-amigaos-gcc-16.2` when you
configure, or with the environment variable of the same name. Without
either, the build looks for `m68k-amigaos-gcc` in the `PATH` and then in
`/opt/amiga` and `/opt/m68k-amigaos`.

### Get the sources

```
git clone https://github.com/fubar-coder/bb2-c.git
cd bb2-c
```

### Configure

```
cmake --preset amiga
```

This prepares the build in `build/amiga/`. It checks that the cross
compiler is recent enough and stops with a message if it is not.

### Build

```
cmake --build --preset amiga
```

The build makes the tools for your machine first, then the Amiga
programs and libraries. It checks every result in the same step that
makes it: a file that fails a check counts as not built, and the build
stops with an error. A clean build has no warnings.

### Look at the result

The `blitz2` directory is `build/amiga/blitz2/`:

{{< filetree/container >}}
  {{< filetree/folder name="build/amiga/blitz2" >}}
    {{< filetree/file name="blitz2" >}}
    {{< filetree/file name="ted" >}}
    {{< filetree/file name="acidlibs" >}}
    {{< filetree/file name="deflibs" >}}
    {{< filetree/file name="blitz2-ab3" >}}
    {{< filetree/file name="ab3cli" >}}
  {{< /filetree/folder >}}
{{< /filetree/container >}}

`cmake --install build/amiga` copies the same files into
`build/amiga/install/blitz2/`, a directory with nothing else in it.

{{% /steps %}}

{{< callout type="info" >}}
Try it: run the tests that need no emulator. They take seconds and show
that the tools and the libraries were built correctly:

```
ctest --preset host
```

Tests that need the emulator or the original material are reported as
skipped.
{{< /callout >}}

## Other build variants

The preset `amiga-debug` builds the same code with debug symbols and
without stripping, into `build/amiga-debug/`. An IDE such as CLion can
use the presets as its CMake profiles; the repository's
[development notes](https://github.com/fubar-coder/bb2-c/blob/main/docs/development.md)
explain that, and the second profile for the tools of the host side.
