---
title: Editor
weight: 4
next: /docs/compared-with-original/new-errors
description: Fixed bugs of TED, the editor of Blitz Basic 2, in the block commands, in Join and Delete Line, and in loading text sources, with examples of the ones ordinary users meet.
---

These bugs are in TED, the editor of Blitz Basic 2 that blitz2 loads and
that also runs on its own as a text editor. They concern what the editor
does with the text in memory and in the files it saves, not the programs
that are compiled from it. The first two sections show the ones an
ordinary user can run into, each with a short source; the other fixes
follow as a list, one sentence each with a link to its issue.

The menu items are named as the editor's menus show them: BLOCK UNTAB,
BLOCK TAB, JOIN and DELETE LINE are in the EDIT menu. "Tokenise" and
"Tab Stop" are settings of the editor's default options; when the editor
runs under blitz2, "Tokenise" is on and the editor keeps the lines of a
source as tokens; on its own, it keeps them as text.

## Block commands

### BLOCK UNTAB removed text, not only indentation {#block-untab}

BLOCK UNTAB is meant to take one level of indentation away from every
line of the marked block. The original did not look at the characters it
removed: it cut the first characters of each line, as many as the tab
stop, whatever they were. A line that was not indented that far lost the
start of its text, and a line no longer than the tab stop became empty.
With the tab stop set to 0, every line of the block became empty.

**What happened**

With the tab stop 2, mark these three lines as a block and choose BLOCK
UNTAB:

```
If a=1
  NPrint "one"
EndIf
```

The original leaves

```
 a=1
NPrint "one"
dIf
```

The indented line is right, but `If` and `En` are gone from the other
two. Saved like this, the source no longer compiles as it did.

**With the port**

```
If a=1
NPrint "one"
EndIf
```

BLOCK UNTAB removes only spaces at the start of a line, up to the tab
stop. A line indented by fewer spaces loses those it has, and a line
that does not start with a space stays as it is. With the tab stop 0,
nothing changes.

Issue: [BLOCK UNTAB removed the first characters of every line, whatever they were (#348)](https://github.com/fubar-coder/bb2-c/issues/348).

## Loading text sources

### The last line of a text source was not tokenised {#last-line-not-tokenised}

The editor loads plain text as well as tokenised sources: a source
written with another editor, or saved as text, is tokenised line by line
while it is loaded, if "Tokenise" is on (which it is under blitz2). The
original tokenised only the lines that end with a line feed. Many text
files have no line feed after their last line, and that line stayed text.
The same happened to a line too long for the editor, more than 127
characters: the editor splits it, and the first part stayed text.

**What happened**

A text source of two lines, with no line feed after the last one:

```
a=1
NPrint a
```

After loading it, the first line is tokenised and the second is not.
On the screen the line looks the same, but `NPrint` is not shown in the
token colour, and Help does not know it. When the source is saved, the
line is saved as the six letters of `NPrint` instead of the
token of `NPrint`, and the compiler, which reads the tokens, does not
take it as the command either. Only changing the line in the editor
tokenised it.

**With the port**

Both lines are tokenised, and so is the first part of a split long
line. `NPrint` is shown in the token colour, Help knows it, and the saved
file contains its token, like any other line.

A tokenised source, whose lines already hold tokens, is loaded as it is.

Issue: [The last line of a text source without a final line feed was not tokenised (#376)](https://github.com/fubar-coder/bb2-c/issues/376).

## Other fixes

### Blocks

* Typing inside a block marked over several lines, in a source of more
  than 128 lines, stretched the block to the end of its last line, so
  that COPY, KILL or BLOCK TO DISK acted on more text than was marked, or
  left the end of the block past column 127; the block keeps its end
  ([#349](https://github.com/fubar-coder/bb2-c/issues/349)).
* BLOCK TAB without tokenising, as in the editor on its own, added an
  invisible 0 byte to the end of every line of the block, which showed up
  as an extra empty line after each of them when the file was saved and
  loaded again; the lines get only their indentation
  ([#347](https://github.com/fubar-coder/bb2-c/issues/347)).
* DELETE LINE on the only line of a one-line block removed the block but
  left the block items of the EDIT menu enabled, "Block" in the status
  line and the start mark of the block set; the block ends completely
  ([#353](https://github.com/fubar-coder/bb2-c/issues/353)).

### Joining lines

* Every JOIN lost 8 to 16 bytes of memory until the next reboot; the
  memory of the joined line is freed completely
  ([#364](https://github.com/fubar-coder/bb2-c/issues/364)).
* A JOIN that moved the start of a block on the next line past column 127
  left an inverted "ghost block" from that line to the end of the text,
  which the keys of KILL and BLOCK TO DISK acted on; the block is removed
  and nothing else is marked
  ([#346](https://github.com/fubar-coder/bb2-c/issues/346)).
