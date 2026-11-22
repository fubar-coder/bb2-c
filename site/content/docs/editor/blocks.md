---
title: Blocks
weight: 2
description: Marking a block of text with the mouse, the function keys or MARK INDENT, and copying, moving, deleting, indenting, saving and loading blocks.
---

Before this page you should know how to type and move the cursor in the
editor ([Typing and moving around]({{< relref "/docs/editor/editing" >}})).

A block is a marked part of the text: from one character to another,
over as many lines as you like. The editor shows it inverted, and the
status line shows the word `block` while one is marked. There is one
block at a time. The items that need a block (COPY, KILL, BLOCK TO DISK,
BLOCK TAB and BLOCK UNTAB) are ghosted in the EDIT menu while there is
none, and their keys beep.

## Marking a block

**With the mouse**, point at the first character, hold the left button
down and drag to the last one, then let go. While you drag, moving the
pointer to the top or the bottom edge of the text scrolls it up or
down, and to the left or the right edge, sideways, so a block can be
longer than the screen. Marking a new block removes the old one; a
click without dragging only moves the cursor and keeps the block.

**With the keyboard**, put the cursor on the first character of the
block and press **F1**, then move the cursor behind the last character
and press **F2**. The block ends in front of the cursor: with the cursor
at the start of a line, it ends with the line above. F2 at the place of
F1 removes the block, and F2 without F1 beeps.

**MARK INDENT** (Amiga-A) marks a part of a structured program in one
step: the cursor line and all lines around it, above and below, that are
indented at least as far as the cursor line. Empty lines are included,
and the block ends at the first line above and below that is indented
less. With the cursor on the `If` line of

```
For i=1 To 10
  If i>5
    NPrint i
  EndIf
Next
```

MARK INDENT marks the three lines from `If` to `EndIf`. On an empty line
it beeps.

**FORGET** (Amiga-W) removes the mark. The text stays as it is.

## Copying, moving and deleting

| Item | Key | What it does |
|---|---|---|
| COPY | Amiga-C or F4 | Inserts a copy of the block at the cursor. The block stays marked. |
| KILL | Amiga-K or Shift + F3 | Deletes the block from the text. |
| FORGET | Amiga-W | Removes the mark of the block. |

COPY refuses with a beep when the cursor is inside the block. There is
no clipboard: KILL deletes the text, and only a saved file has it
still. To move a block, copy it to the new place first and then kill
the original, which stays marked after the copy:

{{% steps %}}

### Mark the block

Drag over it with the mouse, or use F1 and F2.

### Copy it

Put the cursor where the block is to go, outside the block, and press
F4 or Amiga-C.

### Delete the original

Press Amiga-K or Shift + F3. The marked original goes, the copy stays.

{{% /steps %}}

## Indenting a block

**BLOCK TAB** (Amiga-]) indents every line of the block by one tab
stop: it puts as many spaces as the option "Tab Stop" says in front of
each line. **BLOCK UNTAB** (Amiga-[) takes them away again: it removes
spaces at the start of each line, at most one tab stop's worth. A line
indented by fewer spaces loses those it has, and a line that starts with
a character other than a space stays as it is. Both work on whole lines,
from the first line of the block to the last, wherever the block starts
and ends in them.

## Blocks on disk

**BLOCK TO DISK** (F3) writes the block to a file. The file requester
asks for its name ("Name to save block as"). The file is always plain
text, with the lines of the block separated by line feeds and the
commands written out as their names, so that any other editor can read
it. The block stays marked.

**INSERT FROM DISK** loads a file and inserts it at the cursor, as if
it were a block you copy there. The file requester asks for its name
("Name of block to load"). The file can be plain text or a source saved
by the editor; under blitz2, plain text is tokenised as it is loaded.
When the cursor is not at the start of a line and the first line of the
file does not fit behind the text in front of the cursor, within the
128 characters of a line, the editor refuses with a beep and nothing
changes. A marked block is removed.

The two commands remember the drawer and the name of the last block
file, separately from the name of the source.

*Based on the Blitz Basic 2 manual by Acid Software, chapter 1, "Using
Ted the Blitz2 Editor".*
