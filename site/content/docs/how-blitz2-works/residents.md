---
title: Residents and the .xtra file
weight: 5
description: The companion file that keeps the compiler options of a source, and residents, files of precompiled types, macros and constants.
next: /docs/libraries
---

Before this page you should know how the editor and the compiler work
together ([The editor and the compiler]({{< relref "/docs/how-blitz2-works/editor-and-compiler" >}})).

A Blitz Basic 2 source rarely travels alone. Next to it there is usually
a second file with the same name and `.xtra` appended, and the source may
need *residents*, files with declarations made in advance. This page
explains both.

## The .xtra file

When the editor saves a source, say `game.bb2`, it also writes
`game.bb2.xtra`. When it loads a source, it looks for that file and, if it
is there, hands it to the compiler. The `.xtra` file holds the compiler
options of this source:

* the switches of the compiler options, among them **Runtime Error
  Debugger**, **Make Smallest Code**, **Create Debug Info for Executable
  Files** and **Create Icons for Executable Files**;
* the sizes of the compiler's buffers (Object, Libs, System, Data, Macro
  and String Buffer);
* the residents the source uses;
* the path and name of the executable last made with CREATE EXECUTABLE;
* the argument line set with CLI ARGUMENT;
* the Object Maximums of the libraries that manage objects.

The options therefore belong to the source, not to the compiler. If you
copy a source without its `.xtra` file, the compiler keeps whatever
options it has at that moment, and the program may compile differently:
with other buffer sizes, without its residents, or with runtime error
checking switched the other way.

{{< callout type="info" >}}
When you pass a source on, pass its `.xtra` file with it.
{{< /callout >}}

## Residents

A resident is a file of declarations compiled in advance: NewTypes,
macros and constants, but no code. A program that uses a large set of
declarations, for example the structures and constants of AmigaOS, can
load them from a resident instead of including them as source in every
program.

You make a resident with CREATE RESIDENT: the compiler compiles the
current source and saves the NewTypes, macros and constants it defines.
To use it, enter its file name in one of the **Resident** fields of the
COMPILER OPTIONS of another source. A source can use up to eight
residents; they are stored in its `.xtra` file and loaded when the source
is loaded or the options change.

Two residents must not define the same name: the compiler reports
"Clash in Residents" when they do. A resident that cannot be read gives
"Can't Load Resident".

{{< callout type="info" >}}
Try it: put a few constants into a new source (`#width=320`,
`#height=256`), choose CREATE RESIDENT and save it. Then enter that file
in the Resident field of another source's options and use `#width` in it
without defining it.
{{< /callout >}}

A resident is an AmigaOS load file with one hunk that holds the
declarations as they are stored in the compiler's memory. Since the
macros in it are stored tokenised, a resident, like a source, belongs to
the set of libraries it was made with ([Tokenised sources]({{< relref "/docs/how-blitz2-works/tokens" >}})).
