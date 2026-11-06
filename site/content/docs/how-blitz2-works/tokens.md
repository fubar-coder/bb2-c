---
title: Tokenised sources
weight: 2
description: What a token is, how library commands get their token numbers, and why those numbers matter.
---

Before this page you should know how the editor and the compiler work
together ([The editor and the compiler]({{< relref "/docs/how-blitz2-works/editor-and-compiler" >}})).

A Blitz Basic 2 source file is not plain text. The editor replaces every
keyword and every library command by a token, a number of two bytes, and
keeps only names, numbers, strings and operators as text. The compiler
reads these tokens directly. This page explains how that works and why it
matters when libraries change.

## What the editor does with a line

You type a line as text. When you leave it, the editor tokenises it: it
looks up every word in its list of known names and replaces each one it
finds by its token. Everything else stays as it was. In memory and in the
saved file, a line is a sequence of bytes ended by a zero byte:

* a byte below `$80` is a character: a letter of a variable name, a
  digit, a quote, an operator, a space;
* a byte of `$80` or more starts a token, which is two bytes long.

The editor stores the German umlauts, whose character codes are above
`$80`, as control bytes below `$20`, so they cannot be mistaken for
tokens; the compiler turns them back into the real characters inside
strings.

The program

```
a$="Hello"
Print Left$(a$,2)
End
```

is stored as these bytes (the port's own tokeniser produced them):

```
61 24 3D 22 48 65 6C 6C 6F 22 00    a$="Hello"
DD 81 20 DC 81 28 61 24 2C 32 29 00 Print Left$(a$,2)
80 02 00                            End
```

`Print` became `DD 81`, `Left$` became `DC 81` and `End` became `80 02`.
The variable `a$`, the string, the numbers and the brackets are still
text. When the editor shows the line again, it turns each token back into
its name.

The file has no header and no other structure: it is the lines, one after
the other. The extension (`.bb2`, `.bb`) means nothing to the editor or
the compiler. Tabs become spaces when a source is loaded.

## Two kinds of tokens

**Built-in words** are the words the compiler itself knows: statements
like `If`, `For`, `NEWTYPE`, `Macro` and `End`, the mnemonics of the
inline assembler, and a few more. Their tokens begin with `$80`, like
`80 02` for `End`. Everything that is not a built-in word comes from a
library, even such basic commands as `Print` and `Left$`.

**Library tokens** belong to the commands of the libraries. Every library
has a number, and the token of a command is made from that number and the
command's position in the library:

```
token = $8000 + library number × 128 + position
```

`Print` is the first command of the library with the number 187, so its
token is `$8000 + 187 × 128 + 1 = $DD81`. `Left$` is the first command
of library 185: `$DC81`. The editor learns these names and tokens at
start, when blitz2 loads its libraries and adds their commands to the
list of known names.

## Why the library numbers matter

A source does not store the name `Print`. It stores "command 1 of library
187". That has three consequences:

* **A command is only recognised if its library is loaded.** If the
  editor loads a source with a token that no loaded library has, it
  cannot show a name for it and shows `?????` instead. The same happens
  with a source written for a library you do not have.
* **The numbers must never change in an existing set of libraries.** If a
  library got another number, every source that uses it would suddenly
  mean other commands, or none. So the library numbers are fixed, and a
  library you write yourself needs a number no other library uses. A set
  of libraries with other numbers is a *library variant*
  ([blitz2 internals]({{< relref "/docs/blitz2-internals" >}})).
* **Sources of AmiBlitz3 use other numbers.** AmiBlitz3 renumbered many
  libraries and added a great many commands, so a tokenised AmiBlitz3
  source means something else to blitz2. This is why AB3 mode uses
  AmiBlitz3's own token list ([AB3 mode]({{< relref "/docs/ab3-mode" >}})).

## Text sources

The compiler itself always works on tokenised lines. The command line of
blitz2-ab3 can also compile a source saved as plain text: it tokenises
the text while loading, the same way the editor does, with the names of
the libraries it has loaded. In the editor, a source is always kept
tokenised.
