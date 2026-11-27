---
title: Options
description: The editor's DEFAULTS requester - small font, tokenising, icons,
  scroll margins, tab stop and colours - and the file L:blitzeditor.opts that
  keeps them.
sidebar:
  order: 5
---

Before this page you should know the editor's screen and menus
([The editor](/docs/editor/)).

**DEFAULTS** (Amiga-?) in the PROJECT menu opens the requester "Default
Options". Its settings change how the editor looks and works, for every
source; they are not stored with a source. The compiler options, which
are, are a different requester in the COMPILER menu
([The editor and the compiler](/docs/how-blitz2-works/editor-and-compiler/)).

## The settings

| Setting | Default | What it does |
|---|---|---|
| Use Small Font | off | Uses a font 6 pixels high instead of 8, so that more lines fit on the screen. |
| Tokenise | on under blitz2 | Keeps the lines tokenised. Only under blitz2; in the editor on its own it is off and cannot be switched on. |
| Create Icons | on | SAVE writes an icon next to a source that has none ([Files](/docs/editor/files/#what-save-writes)). |
| Horizontal Scroll Margin | 12 | How many columns the cursor keeps from the left and the right edge before the text scrolls sideways, 0 to 31. |
| Vertical Scroll Margin | 6 | How many rows the cursor keeps from the top and the bottom edge before the text scrolls, 0 to 9. |
| Tab Stop | 2 | The distance of the tab stops, 1 to 79. It is used by Tab, Shift + Tab, BLOCK TAB and BLOCK UNTAB, and when a TAB of a text file is turned into spaces. |

Type a number into its field and press **Return**: the editor takes the
number when you leave the field with Return, not when you click OK. A
number outside its range is refused with a beep; a scroll margin then
becomes 0, and the tab stop keeps its previous value.

Switching **Tokenise** off under blitz2 turns every line into plain
text: the commands are shown in the text colour, the HELP key beeps,
and SAVE writes plain text. Switching it on tokenises all lines again.
Whatever you choose, Tokenise is on again at the next start of blitz2.

## Colours

The editor's screen has four colours. The requester sets which of them
the text uses and what each of them looks like:

* **Text Colour** is the first row of four buttons, one per colour: the
  colour of the text. By default the second colour, white.
* **Token Colour** is the third row: the colour of the commands of a
  tokenised line. By default the fourth colour, orange. The second row,
  which has no name, has no effect in the editor.
* The **row of four wide buttons** below chooses one of the colours,
  and the sliders **Red**, **Green** and **Blue** set that colour, from
  0 to 15 each. The first colour is the background, grey by default;
  the third is black.

The colours change on the screen while you move the sliders.

## OK, CANCEL and the options file

**OK** keeps the settings. With a new font, the editor opens its screen
again with the new number of rows. **CANCEL** puts back the settings
the requester opened with.

The editor stores its options in the file `L:blitzeditor.opts` and
reads them from there at every start, under blitz2 and on its own. It
writes the file when you quit the editor, if you clicked OK in DEFAULTS
during the session. To go back to the defaults of this page, delete the
file before starting the editor.

A damaged or hand-edited file does not stop the editor: a tab stop of 0
is taken as 1 and one above 79 as 79, and values the requester cannot
show are reduced to ones it can.

*Based on the Blitz Basic 2 manual by Acid Software, chapter 1, "Using
Ted the Blitz2 Editor".*
