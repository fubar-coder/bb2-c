---
title: Keys and menus at a glance
weight: 6
description: Every menu item of the editor with its shortcut, every key of the text and the function keys, in tables for looking things up.
next: /docs/how-blitz2-works
---

Before this page you should have read at least the first page of this
chapter ([The editor]({{< relref "/docs/editor" >}})); the other pages
explain what the items below do.

This page lists the editor's menus and keys in one place. A shortcut
**Amiga-L** means the right Amiga key held down with L. The menus are
in the order of the menu bar.

## PROJECT

| Item | Key | Described in |
|---|---|---|
| NEW | | [Loading]({{< relref "/docs/editor/files#loading" >}}) |
| LOAD | Amiga-L | [Loading]({{< relref "/docs/editor/files#loading" >}}) |
| SAVE | Amiga-S | [What SAVE writes]({{< relref "/docs/editor/files#what-save-writes" >}}) |
| DEFAULTS | Amiga-? | [Options]({{< relref "/docs/editor/options" >}}) |
| ABOUT | | [The other items of the PROJECT menu]({{< relref "/docs/editor/files#the-other-items-of-the-project-menu" >}}) |
| PRINT | | [Printing]({{< relref "/docs/editor/files#printing" >}}) |
| CLI | Amiga-Z | [The other items of the PROJECT menu]({{< relref "/docs/editor/files#the-other-items-of-the-project-menu" >}}) |
| CLOSE WORKBENCH | | [The other items of the PROJECT menu]({{< relref "/docs/editor/files#the-other-items-of-the-project-menu" >}}) |
| OPEN WORKBENCH | | [The other items of the PROJECT menu]({{< relref "/docs/editor/files#the-other-items-of-the-project-menu" >}}) |
| QUIT | Amiga-Q | [The other items of the PROJECT menu]({{< relref "/docs/editor/files#the-other-items-of-the-project-menu" >}}) |

## EDIT

The items marked with a star need a block and are ghosted without one.

| Item | Key | Described in |
|---|---|---|
| COPY \* | Amiga-C, F4 | [Copying, moving and deleting]({{< relref "/docs/editor/blocks#copying-moving-and-deleting" >}}) |
| KILL \* | Amiga-K, Shift + F3 | [Copying, moving and deleting]({{< relref "/docs/editor/blocks#copying-moving-and-deleting" >}}) |
| BLOCK TO DISK \* | F3 | [Blocks on disk]({{< relref "/docs/editor/blocks#blocks-on-disk" >}}) |
| INSERT FROM DISK | | [Blocks on disk]({{< relref "/docs/editor/blocks#blocks-on-disk" >}}) |
| FORGET | Amiga-W | [Marking a block]({{< relref "/docs/editor/blocks#marking-a-block" >}}) |
| INSERT LINE | Amiga-I | [Commands for whole lines]({{< relref "/docs/editor/editing#commands-for-whole-lines" >}}) |
| DELETE LINE | Amiga-D | [Commands for whole lines]({{< relref "/docs/editor/editing#commands-for-whole-lines" >}}) |
| DELETE RIGHT | Amiga-Y | [Commands for whole lines]({{< relref "/docs/editor/editing#commands-for-whole-lines" >}}) |
| JOIN | Amiga-J | [Commands for whole lines]({{< relref "/docs/editor/editing#commands-for-whole-lines" >}}) |
| BLOCK TAB \* | Amiga-] | [Indenting a block]({{< relref "/docs/editor/blocks#indenting-a-block" >}}) |
| BLOCK UNTAB \* | Amiga-[ | [Indenting a block]({{< relref "/docs/editor/blocks#indenting-a-block" >}}) |
| MARK INDENT | Amiga-A | [Marking a block]({{< relref "/docs/editor/blocks#marking-a-block" >}}) |

## SOURCE

| Item | Key | Described in |
|---|---|---|
| TOP | Amiga-T | [Going to a place in the source]({{< relref "/docs/editor/editing#going-to-a-place-in-the-source" >}}) |
| BOTTOM | Amiga-B | [Going to a place in the source]({{< relref "/docs/editor/editing#going-to-a-place-in-the-source" >}}) |
| GOTO LINE | Amiga-G | [Going to a place in the source]({{< relref "/docs/editor/editing#going-to-a-place-in-the-source" >}}) |

## SEARCH

| Item | Key | Described in |
|---|---|---|
| FIND | Amiga-F | [The Find/Replace requester]({{< relref "/docs/editor/search#the-findreplace-requester" >}}) |
| NEXT | Amiga-N | [Searching]({{< relref "/docs/editor/search#searching" >}}) |
| PREVIOUS | Amiga-P | [Searching]({{< relref "/docs/editor/search#searching" >}}) |
| REPLACE | Amiga-R | [Replacing]({{< relref "/docs/editor/search#replacing" >}}) |
| REPLACE ALL | | [Replacing]({{< relref "/docs/editor/search#replacing" >}}) |

## COMPILER

blitz2 adds this menu; the editor on its own does not have it. Its
items are described in
[The editor and the compiler]({{< relref "/docs/how-blitz2-works/editor-and-compiler#the-compiler-menu" >}}).

| Item | Key |
|---|---|
| COMPILE AND RUN | Amiga-X |
| RUN | Amiga-M |
| CREATE EXECUTABLE | Amiga-E |
| COMPILER OPTIONS | Amiga-O |
| CREATE RESIDENT | Amiga-; |
| VIEW NEWTYPES | Amiga-\- |
| CLI ARGUMENT | Amiga-= |
| CALCULATOR | Amiga-H |
| RELOAD ALL LIBS | Amiga-\\ |
| LOAD DEBUG MODULE | Amiga-. |

## Keys of the text

| Key | What it does |
|---|---|
| Cursor keys | One character or one line. |
| Shift + Cursor up / down | A screen up or down. |
| Shift + Cursor left | To the start of the line; at the start, to the indentation of the line above. |
| Shift + Cursor right | Behind the last character of the line. |
| Return | A new, empty line below the cursor line. |
| Shift + Return | Splits the line at the cursor. |
| Backspace | Deletes the character left of the cursor. |
| Del | Deletes the character under the cursor. |
| Tab | Spaces up to the next tab stop. |
| Shift + Tab | Back to the previous tab stop, deleting what lies between. |
| HELP | The command at the cursor and its arguments, in the title bar. |
| Right Amiga + HELP | As HELP, and starts `blitz2:help` for the command. |
| F1 | Start of a block at the cursor. |
| F2 | End of the block in front of the cursor. |
| F3 | BLOCK TO DISK. |
| Shift + F3 | KILL. |
| F4 | COPY. |

The keys are described on
[Typing and moving around]({{< relref "/docs/editor/editing#the-keys-of-the-text" >}})
and [Blocks]({{< relref "/docs/editor/blocks" >}}). F5 to F10 do
nothing.

In a requester, when none of its text fields is active, Return clicks
the first of its buttons that closes it, such as OK, NEXT or PRINT, and
Esc the last one, usually CANCEL.

## The mouse

| Action | What it does |
|---|---|
| Click in the text | Puts the cursor there. |
| Drag in the text | Marks a block; at an edge of the text, it scrolls. |
| Drag a scroll bar | Moves through the lines or the columns. |
| Click in the label column | Goes to the line of that label. |
| Right button | The menus. |
