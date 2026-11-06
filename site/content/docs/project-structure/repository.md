---
title: Finding your way in the repository
weight: 1
description: The directories of the BB2 C port's repository and what you find in each of them.
prev: /docs/project-structure
---

Before this page you should know the two sides of the build
([Project structure]({{< relref "/docs/project-structure" >}})).

The top level of the repository, with what each directory holds and when
you would go there:

{{< filetree/container >}}
  {{< filetree/folder name="programs" >}}
    {{< filetree/folder name="blitz2" state="closed" >}}{{< /filetree/folder >}}
    {{< filetree/folder name="ted" state="closed" >}}{{< /filetree/folder >}}
    {{< filetree/folder name="ab3cli" state="closed" >}}{{< /filetree/folder >}}
  {{< /filetree/folder >}}
  {{< filetree/folder name="libs" >}}
    {{< filetree/folder name="acidlibs" state="closed" >}}{{< /filetree/folder >}}
    {{< filetree/folder name="deflibs" state="closed" >}}{{< /filetree/folder >}}
    {{< filetree/folder name="userlibs" state="closed" >}}{{< /filetree/folder >}}
    {{< filetree/folder name="variants" state="closed" >}}{{< /filetree/folder >}}
    {{< filetree/folder name="include" state="closed" >}}{{< /filetree/folder >}}
    {{< filetree/folder name="tests" state="closed" >}}{{< /filetree/folder >}}
  {{< /filetree/folder >}}
  {{< filetree/folder name="shared" state="closed" >}}{{< /filetree/folder >}}
  {{< filetree/folder name="host" state="closed" >}}{{< /filetree/folder >}}
  {{< filetree/folder name="tools" state="closed" >}}{{< /filetree/folder >}}
  {{< filetree/folder name="tests" state="closed" >}}{{< /filetree/folder >}}
  {{< filetree/folder name="archive" state="closed" >}}{{< /filetree/folder >}}
  {{< filetree/folder name="cmake" state="closed" >}}{{< /filetree/folder >}}
  {{< filetree/folder name="docs" state="closed" >}}{{< /filetree/folder >}}
  {{< filetree/folder name="openspec" state="closed" >}}{{< /filetree/folder >}}
  {{< filetree/folder name="site" state="closed" >}}{{< /filetree/folder >}}
{{< /filetree/container >}}

## The programs

* **[programs/blitz2](https://github.com/fubar-coder/bb2-c/tree/main/programs/blitz2)**:
  the compiler. Its source is shared by blitz2 and blitz2-ab3; the files
  of AB3 mode are the ones whose names begin with `ab3`
  ([Inside the compiler source]({{< relref "/docs/blitz2-internals/compiler-source" >}})).
* **[programs/ted](https://github.com/fubar-coder/bb2-c/tree/main/programs/ted)**:
  the editor, with its operating-system-free core in a directory of its
  own, the host tests of that core, and the host tools built on it.
* **[programs/ab3cli](https://github.com/fubar-coder/bb2-c/tree/main/programs/ab3cli)**:
  the helper that blitz2-ab3 uses in place of the editor when it compiles
  from the Shell.

## The libraries

* **[libs/acidlibs](https://github.com/fubar-coder/bb2-c/tree/main/libs/acidlibs)**
  and **[libs/deflibs](https://github.com/fubar-coder/bb2-c/tree/main/libs/deflibs)**:
  one C file per library of Blitz Basic 2, and per AmigaOS library whose
  calls are declared.
* **[libs/userlibs](https://github.com/fubar-coder/bb2-c/tree/main/libs/userlibs)**:
  further libraries, such as the RWE userlibs.
* **[libs/variants](https://github.com/fubar-coder/bb2-c/tree/main/libs/variants)**:
  the library numbers of each variant.
* **[libs/include](https://github.com/fubar-coder/bb2-c/tree/main/libs/include)**:
  the header for writing libraries in C.
* **[libs/tests](https://github.com/fubar-coder/bb2-c/tree/main/libs/tests)**:
  the program that compares each library with its original in the
  emulator.

## Shared code, tools and tests

* **[shared](https://github.com/fubar-coder/bb2-c/tree/main/shared)**:
  code compiled into several programs, such as the file requester of the
  editor and of the file requester library, the `.xtra` settings, and the
  tokeniser of AB3 mode. Each program compiles it itself, with its own
  settings.
* **[tools](https://github.com/fubar-coder/bb2-c/tree/main/tools)**: the
  host tools, in C. Their logic is in libraries of their own with tests;
  each tool is a small main program around one of them.
* **[host](https://github.com/fubar-coder/bb2-c/tree/main/host)**: the
  project that builds the host side.
* **[tests](https://github.com/fubar-coder/bb2-c/tree/main/tests)**: the
  test suites that run in the emulator, their programs and data
  ([Tests]({{< relref "/docs/project-structure/tests" >}})).

## Originals, build and documentation

* **[archive](https://github.com/fubar-coder/bb2-c/tree/main/archive)**:
  the unchanged original sources, and the place the downloaded original
  material goes. Nothing in it is ever edited.
* **[cmake](https://github.com/fubar-coder/bb2-c/tree/main/cmake)**: the
  cross compiler's settings and the helpers of the build.
* **[docs](https://github.com/fubar-coder/bb2-c/tree/main/docs)**: the
  developers' notes: the analysis of the original sources, the notes on
  the translation, and the lists of deviations. They are written for
  people who work on the port, and they are the source of the facts on
  this site.
* **[openspec](https://github.com/fubar-coder/bb2-c/tree/main/openspec)**:
  specifications of new behaviour, such as AB3 mode, and the changes that
  introduced it.
* **[site](https://github.com/fubar-coder/bb2-c/tree/main/site)**: this
  website.

## Comments that refer to the original

Many comments in the C code refer to the original: a comment such as
`blitz2.s:13837` or `ted.s:1542` names the line of the original source a
function translates, `BUG(orig)` marks a bug of the original that is kept
on purpose, and `DIFF` marks a place where the C code has to work
differently from the original. The reasons behind them are in the notes
in `docs/`.
