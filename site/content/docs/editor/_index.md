---
title: The editor
weight: 2
description: TED, the editor of Blitz Basic 2 - its screen, typing and moving, blocks, search and replace, the files it writes, its options, and all its keys and menus.
prev: /docs/introduction/first-program
next: /docs/editor/editing
---

Before this page you should have typed and run a program in the editor
([Your first program]({{< relref "/docs/introduction/first-program" >}})).

The editor of the BB2 C port is TED 1.1, the editor of Blitz Basic 2.
Under blitz2 it is the whole user interface: you write the program in
it, compile and run it from its COMPILER menu, and come back to it when
the program ends. It also runs on its own, as a plain text editor. This
chapter describes the editor itself: how to type and move around, how to
work with blocks of text, how to search and replace, which files it
writes, and how to set it up. The COMPILER menu, which blitz2 adds, is
described in
[The editor and the compiler]({{< relref "/docs/how-blitz2-works/editor-and-compiler" >}}).

## Two ways to start it

**Under blitz2**, which is the normal case, blitz2 loads the editor and
gives it the list of all commands of the loaded libraries. The editor
then keeps every line *tokenised*: when you leave a line, it turns the
keywords and commands into short numbers and shows them in the token
colour
([Tokenised sources]({{< relref "/docs/how-blitz2-works/tokens" >}})).
This is also what makes the HELP key work, the label column at the right
edge of the screen, and the `.xtra` file with the compiler options next
to each saved source.

**On its own**, the editor is started from the Shell as `blitz2:ted`,
with the name of a file to edit if you like:

```
1> blitz2:ted RAM:notes.txt
```

The name ends at the first space; a name with spaces cannot be given
here, but LOAD can open such a file. Without blitz2 there is no COMPILER
menu and no list of commands, so the editor keeps the text as it is:
the option "Tokenise" is off and cannot be switched on, the HELP key
only beeps, and there is no label column. Everything else in this
chapter works the same way in both cases.

## The screen

The editor opens a screen of its own, as wide and as high as the
Workbench screen and in the same display mode, with four colours. From
top to bottom it shows:

* **The title bar** of the screen, with the name of the file you are
  editing. The HELP key shows its help text here, and the file
  requester its messages.
* **The text**, one line of the source per row. A line holds up to 128
  characters. When it is wider than the screen, the editor scrolls the
  text sideways as the cursor comes near the edge. To the right of the
  text is the vertical scroll bar, below it the horizontal one; drag
  them to move through the source.
* **The label column**, under blitz2, at the right edge of the screen.
  It appears as soon as the source has a label, a line that starts with
  a `.`, and lists the labels in the order they appear in the source.
  Click a label to go to its line
  ([Typing and moving around]({{< relref "/docs/editor/editing#the-label-column" >}})).
* **The status line** at the bottom, with the line and the column of
  the cursor, the largest block of free memory in kilobytes, and the
  word `block` while a block is marked.

The menus are on the right mouse button, as on every Amiga screen, and
most of their items also have a shortcut with the right Amiga key: hold
it down and press the letter, as with Shift. The pages of this chapter
write such a shortcut as **Amiga-L**.

{{< callout type="warning" >}}
The editor has no undo. A deleted line, a killed block or a replace is
final, and the only way back is the file you saved last; SAVE keeps the
version before that as a `.bak` file
([Files]({{< relref "/docs/editor/files#what-save-writes" >}})).
{{< /callout >}}

## The pages of this chapter

{{< cards >}}
  {{< card link="editing" title="Typing and moving around" subtitle="The keys of the text, the mouse, line commands, jumping to a line or a label, and the HELP key." >}}
  {{< card link="blocks" title="Blocks" subtitle="Marking text with the mouse or the keyboard, copying, moving, deleting and indenting it, and blocks on disk." >}}
  {{< card link="search" title="Finding and replacing" subtitle="The Find/Replace requester, searching forwards and backwards, and replacing in one line or in the whole source." >}}
  {{< card link="files" title="Files" subtitle="Loading and saving, the files SAVE writes, the file requester, printing and the PROJECT menu." >}}
  {{< card link="options" title="Options" subtitle="The DEFAULTS requester: font, tokenising, icons, scroll margins, tab stop and colours, and the file that keeps them." >}}
  {{< card link="keys-and-menus" title="Keys and menus at a glance" subtitle="Every menu item and every key of the editor in tables, for looking things up." >}}
{{< /cards >}}

The editor behaves like the original TED 1.1, except for the original's
bugs, which the port fixes. Those fixes are listed on the page
[Editor]({{< relref "/docs/compared-with-original/editor" >}})
of the fixed bugs.

*Parts of this chapter are based on the Blitz Basic 2 manual by Acid
Software, chapter 1, "Using Ted the Blitz2 Editor".*
