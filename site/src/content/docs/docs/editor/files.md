---
title: Files
description: Loading and saving sources in the editor, the files SAVE writes
  next to a source, tokenised and plain text files, the file requester,
  printing, and the other items of the PROJECT menu.
sidebar:
  order: 4
---

Before this page you should have saved and loaded a source once
([Your first program](/docs/introduction/first-program/#saving-and-loading)).

The PROJECT menu has the items for files and for the editor as a whole.
This page explains what LOAD reads and what SAVE writes, which can be
more than one file, and then the other items of the menu.

## Loading

**LOAD** (Amiga-L) asks for a file with the file requester ("Name of
file to load") and replaces the text in the editor with it. **NEW**
clears the editor for a new source. If the text has changed since it
was last saved, both first ask "This will erase altered file!
Continue?"; CANCEL keeps the text.

The editor reads two kinds of files and tells them apart by their
content, not by their name:

* **A source saved by the editor under blitz2**, which is tokenised:
  its lines hold the tokens of the commands, each line ended with a 0
  byte. It is loaded as it is.
* **Plain text**, with lines ended by line feeds, as any other editor
  writes it. Under blitz2, each line is tokenised as it is loaded, so
  the source can be compiled at once. A TAB becomes spaces up to the
  next tab stop, a line longer than 128 characters is split into lines
  of up to 128, and accented letters are kept.

Under blitz2, LOAD also reads the source's `.xtra` file, if there is
one, and so sets the compiler options that were saved with the source.
After loading, the title bar shows the file name, and the cursor is at
the start of the first line.

## What SAVE writes {#what-save-writes}

**SAVE** (Amiga-S) asks for the name with the file requester ("Name to
save file as"), which shows the name the source was loaded or saved
with last. For a source `hello.bb2`, SAVE then writes up to four files:

| File | When | What it is |
|---|---|---|
| `hello.bb2.bak` | when `hello.bb2` exists | The previous version: the old file is renamed, and an older `.bak` deleted. |
| `hello.bb2` | always | The source: tokenised with "Tokenise" on, as under blitz2; plain text with it off, as in the editor on its own. |
| `hello.bb2.xtra` | under blitz2 | The compiler options of this source ([Residents and the .xtra file](/docs/how-blitz2-works/residents/)). |
| `hello.bb2.info` | with "Create Icons" on, and no icon yet | An icon for the source, whose default tool is `blitz2:blitz2`. An existing icon is never overwritten. |

The `.bak` file is the only way back to the version before the last
save, so keep it until you are sure. When the source cannot be written,
for example because the disk is full, the editor shows "Error writing
file!", and it writes neither the `.xtra` file nor the icon. The
previous version is still there as the `.bak` file.

To save a source under blitz2 as plain text, for another editor or a
program on your host, switch "Tokenise" off in DEFAULTS before saving
and on again afterwards
([Options](/docs/editor/options/)), or mark the whole
source as a block and write it with BLOCK TO DISK, which always writes
plain text
([Blocks](/docs/editor/blocks/#blocks-on-disk)).
A plain text file holds every accented letter as its own Latin-1 byte,
so it is saved exactly as it was loaded.

The other messages of loading and saving are "Couldn't open file!",
when the file cannot be opened, and "Error reading file!".

## The file requester

LOAD, SAVE, BLOCK TO DISK and INSERT FROM DISK use the editor's own
file requester, the one that the command `FileRequest$` opens in a
program ([freqlib](/docs/libraries/commands/freqlib/)).
Its title says what it is asking for.

* The list shows the files of the drawer in `PATH:`, with the drawers
  marked `<DIR>`. The slider at the right scrolls it.
* A click on a drawer opens it; **PARENT** goes to the drawer above.
  **DRIVES** adds the drives, volumes and assigns to the top of the
  list.
* A double click on a file chooses it. You can also type the drawer
  into `PATH:` and the name into `FILE:`, and click **OK**. **CANCEL**,
  or the close gadget, cancels.
* **CD** makes the drawer shown the editor's current directory, so that
  CLI and the programs you run from the editor start there.
* The window can be made larger to show more files at once.

A name has room for 63 characters and a path for 159. The requester
refuses a longer one: it stays open and shows "Name too long" or "Path
too long" in the title bar.

## Printing

**PRINT** sends the whole source to the printer. A requester "Print
Text" asks for the device, `PRT:` at first; PRINT starts, CANCEL does
not. Under blitz2 the commands are printed in bold, with the printer's
codes for bold on and off around each one. The device can also be a
file name, such as `RAM:listing.txt`; the file then holds the text with
those codes.

## The other items of the PROJECT menu

| Item | Key | What it does |
|---|---|---|
| DEFAULTS | Amiga-? | Opens the editor's options ([Options](/docs/editor/options/)). |
| ABOUT | | Shows the version and the credits of Blitz Basic 2. |
| CLI | Amiga-Z | Opens a Shell in a window of its own, "Blitz 2 CLI Window". Type `EndCLI` to close it and get back to the editor. |
| CLOSE WORKBENCH | | Closes the Workbench screen, if no program has a window on it, to free its memory. |
| OPEN WORKBENCH | | Opens the Workbench screen again. |
| QUIT | Amiga-Q | Leaves the editor; under blitz2 it ends blitz2 as well. If the text has changed since it was saved, it first asks, as LOAD does. |

When you quit, the editor also saves its options, if you changed them
in DEFAULTS during this session.

*Based on the Blitz Basic 2 manual by Acid Software, chapter 1, "Using
Ted the Blitz2 Editor".*
