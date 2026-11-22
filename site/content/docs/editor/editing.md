---
title: Typing and moving around
weight: 1
description: The keys of the editor's text, the mouse and the scroll bars, inserting, deleting, splitting and joining lines, jumping to a line or a label, and the HELP key.
prev: /docs/editor
---

Before this page you should know what the editor's screen shows
([The editor]({{< relref "/docs/editor" >}})).

This page covers everything you do with the text line by line: typing,
moving the cursor, the commands for whole lines, jumping to a place in
the source, and looking up a command with the HELP key.

## Typing

The editor is always in insert mode: a character you type goes in at
the cursor, and the rest of the line moves one column to the right.
A line holds up to 128 characters; a character that typing pushes past
the last column is lost.

Under blitz2, the editor tokenises a line when you leave it: keywords
and commands are stored as tokens and shown in the token colour, the
rest stays text. So a command shows its colour only once the cursor is
on another line. A command that keeps the text colour is not known to
the compiler: it is misspelt, or its library is not loaded.

You can type the umlauts ä ö ü Ä Ö Ü and every other Latin-1 character,
such as ß, é or £, and the editor shows each as itself. Accented letters
belong in strings and comments; outside them, the compiler stops with
"Syntax Error"
([ß, é and other accented letters]({{< relref "/docs/compared-with-original/fixed-bugs/editor#latin1-in-strings" >}})).

## The keys of the text

| Key | What it does |
|---|---|
| Cursor keys | Move the cursor by one character or one line. |
| Shift + Cursor up / down | Move the cursor a screen up or down. |
| Shift + Cursor left | Moves the cursor to the start of the line. At the start of the line already, it moves it under the first character of the line above, to the indentation of that line. |
| Shift + Cursor right | Moves the cursor behind the last character of the line. |
| Return | Opens a new, empty line below the cursor line and puts the cursor at its start. It does not split the line. |
| Shift + Return | Splits the line at the cursor: the text from the cursor on moves to a new line below. |
| Backspace | Deletes the character left of the cursor. At the start of a line it does nothing; JOIN joins two lines. |
| Del | Deletes the character under the cursor. |
| Tab | Inserts spaces up to the next tab stop; the text behind the cursor moves along. |
| Shift + Tab | Moves the cursor back to the previous tab stop and deletes what lies between, so that the text behind the cursor moves left with it. |
| HELP | Shows the command at the cursor with its arguments in the title bar (see below). |

The tab stops are every two columns, or as many as the option "Tab
Stop" says
([Options]({{< relref "/docs/editor/options" >}})). Tab inserts
spaces, not a TAB character; a TAB in a text file you load is turned
into spaces in the same way.

The function keys F1 to F4 work on blocks and are described on the page
[Blocks]({{< relref "/docs/editor/blocks" >}}).

## The mouse and the scroll bars

A click with the left mouse button in the text puts the cursor there.
Holding the button down and dragging marks a block
([Blocks]({{< relref "/docs/editor/blocks#marking-a-block" >}})).

The vertical scroll bar at the right of the text moves through the
lines of the source, the horizontal one below the text through the
columns. Drag the knob, and the cursor goes along.

The editor scrolls by itself when the cursor comes near an edge of the
screen: it keeps the cursor six rows away from the top and the bottom,
and twelve columns away from the left and the right edge, as long as
there is text to scroll to. The options "Vertical Scroll Margin" and
"Horizontal Scroll Margin" change these distances.

## Commands for whole lines

These items are in the EDIT menu:

| Item | Key | What it does |
|---|---|---|
| INSERT LINE | Amiga-I | Inserts an empty line above the cursor line; the cursor stays in the empty line. |
| DELETE LINE | Amiga-D | Deletes the cursor line. |
| DELETE RIGHT | Amiga-Y | Deletes the text from the cursor to the end of the line. |
| JOIN | Amiga-J | Appends the next line to the cursor line, directly behind its last character. |

JOIN takes the next line as it is, with its leading spaces. It refuses
with a beep on the last line, where there is nothing to join, and when
the two lines would be longer than 128 characters together; both lines
then stay as they are.

## Going to a place in the source

The SOURCE menu moves the cursor through the source:

| Item | Key | What it does |
|---|---|---|
| TOP | Amiga-T | Moves the cursor to the first line. |
| BOTTOM | Amiga-B | Moves the cursor to the last line. |
| GOTO LINE | Amiga-G | Asks for a line number and moves the cursor to that line. |

GOTO LINE opens a small requester, "LINE TO GOTO:". Type the number and
press Return, or click OK. A number of 0 or one behind the last line
gives a beep, and the cursor stays where it was. The status line always
shows the number of the cursor line, so you can note a place and come
back to it.

Searching for a text is another way to get to a place
([Finding and replacing]({{< relref "/docs/editor/search" >}})).

### The label column

Under blitz2, every line whose first character other than a space is a
`.` is a label, as `.mainloop` in this source:

```
.mainloop
  NPrint "again"
  Goto mainloop
```

As soon as the source has a label, the editor makes the text area
narrower and shows a column at the right edge of the screen, twelve
characters wide, with the names of the labels in the order they appear
in the source, one per row, without the `.`. Click a name, and the
cursor goes to the line of that label. The column shows as many labels
as the screen has rows. When the last label is deleted, the column goes
away again.

## The HELP key

Under blitz2, the HELP key shows the command at the cursor in the title
bar of the screen, with the arguments it takes as its library declares
them: with the cursor on `NPrint`, the title bar shows
`NPrint Expression[,Expression...]`. Square brackets mark optional
arguments. With the cursor behind a command, on one of its arguments,
the editor takes the last command before the cursor in the line. When
there is none, the title bar shows the file name again.

The key works for every command of every loaded library, also for
userlibs, and also on the line you are typing. In the editor on its
own, which has no list of commands, HELP only beeps.

With the right Amiga key held down, HELP also starts the program
`blitz2:help` with the number of the command's token, the help viewer
of the original Blitz Basic 2. The port does not include that program.

*Based on the Blitz Basic 2 manual by Acid Software, chapter 1, "Using
Ted the Blitz2 Editor".*
