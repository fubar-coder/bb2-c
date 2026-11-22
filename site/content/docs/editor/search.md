---
title: Finding and replacing
weight: 3
description: The editor's Find/Replace requester, NEXT and PREVIOUS, and replacing a text in one line or in the whole source.
---

Before this page you should know how to move around in the editor
([Typing and moving around]({{< relref "/docs/editor/editing" >}})).

The SEARCH menu finds a text in the source and replaces it with
another. The editor keeps one search text and one replacement; FIND
sets them, and the other items of the menu use them.

## The Find/Replace requester

**FIND** (Amiga-F) opens the requester "Find/Replace". It has two
fields, `FIND:` for the text to look for and `REPLACE:` for the text to
put in its place, each with room for 63 characters, and a row of
buttons:

| Button | What it does |
|---|---|
| NEXT | Closes the requester and searches forwards, as the menu item NEXT. |
| PREVIOUS | Closes the requester and searches backwards, as PREVIOUS. |
| REPLACE | Closes the requester and replaces in the cursor line, as REPLACE. |
| REPLACE ALL | Closes the requester and replaces in many lines, as REPLACE ALL. |
| CANCEL | Closes the requester and keeps the texts you had before. |
| CASE SENSITIVE | A switch: on, the search tells capitals from small letters. |

The requester opens with the texts of the last search. Return in the
`FIND:` field moves to the `REPLACE:` field, and Return there searches
forwards, as NEXT. CASE SENSITIVE is off at the start of
the editor and stays as you set it.

## Searching

**NEXT** (Amiga-N) searches forwards for the search text, starting with
the line below the cursor, and puts the cursor on the first place in a
line where the text occurs. **PREVIOUS** (Amiga-P) does the same
backwards, starting with the line above the cursor. When the text is
not found up to the end or the start of the source, the editor beeps
and the cursor stays where it was; it beeps too when the search text is
empty.

Since each search starts with the next line, NEXT goes from line to
line: a second place in the same line is not visited. The search
compares the text as the editor shows it, with the commands as their
names, and finds it anywhere, also inside a longer word: `Print` also
finds `NPrint`. Without CASE SENSITIVE, `nprint` finds `NPrint` as
well.

## Replacing

**REPLACE** (Amiga-R) replaces every place of the search text in the
cursor line with the replacement, wherever the cursor is in the line.
When the line does not contain the text, the editor beeps. The usual
way is to find a place with NEXT, look at it, and press Amiga-R if it
is to change, or Amiga-N to go on.

**REPLACE ALL** (no key) replaces every place in many lines at once. It
asks where to start, in a requester "Replace all.....":

* **FROM HERE** replaces from the cursor line to the end of the source.
* **FROM TOP** replaces in the whole source.
* **CANCEL** replaces nothing.

A replacement that contains the search text is fine: the editor goes on
searching behind the text it put in, so replacing `a` with `aa` doubles
every `a` once.

{{< callout type="info" >}}
A replace cannot be undone. Before a REPLACE ALL over the whole source,
save it, so that the file still has the old text if the result is not
what you meant.
{{< /callout >}}

*Based on the Blitz Basic 2 manual by Acid Software, chapter 1, "Using
Ted the Blitz2 Editor".*
