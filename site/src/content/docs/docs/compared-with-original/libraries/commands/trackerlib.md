---
title: trackerlib
description: The bugs of Blitz Basic 2 in the command library trackerlib that
  the BB2 C port fixes.
sidebar:
  order: 59
aliases:
  - /docs/compared-with-original/fixed-bugs/libraries/commands/trackerlib/
---

::fixed-bugs-library

## Changes you may notice

### A pattern break in the last song position played on behind the end {#tracker-break-song-end}

A tracker module plays its patterns in the order of its song positions.
The song length says how many positions the song uses, and after the last
one the song starts again at its restart position. A pattern break
(effect D) or a position jump (effect B) ends a pattern early and goes on
at the next position.

**What happened.** After a D or B in the last song position, the
original's `PlayModule` compared the next position with a byte of the
pattern data instead of the song length, and took the restart position
from the byte after it. So it could miss the end of the song and play
on into the unused positions behind it, with whatever patterns they
name. A module with two song
positions, patterns 0 and 1, a break `D00` in row 16 of pattern 1 and
pattern 2 in the unused position 2 behind them, plays like this:

| | Positions played |
|---|---|
| With the original | 0, 1 (to row 16), 2, and on behind the end |
| With the port | 0, 1 (to row 16), 0, 1, … |

**What to change.** Nothing for a module that is to loop as it does in
the tracker: it now loops at its song length, as at the end of a pattern
without a break. A module that relied on playing on behind its end needs
a song length that covers all the positions it is to play.

Issue: [After a pattern break or position jump the song end is compared with the wrong byte (#292)](https://github.com/fubar-coder/bb2-c/issues/292).

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Music players (MED and tracker modules) {#music-players}

Two more fixes of them change what an existing program does:
[`GetMedVolume` silenced the channel it read](/docs/compared-with-original/libraries/commands/medlib/#getmedvolume-clears) and
[A pattern break in the last song position played on behind the end](#tracker-break-song-end).

* `LoadModule` of a drawer or a missing file kept a block of memory, and
  for a drawer a lock on it, after its runtime error, which mattered to
  programs that catch the error with `SetErr`; both are freed before the
  error ([#296](https://github.com/fubar-coder/bb2-c/issues/296)).
* `PlayModule` never read the last song position, and a new highest
  pattern number at position 126 made it search 64 KB further and clear
  memory behind the module; it finds the highest pattern from all 128
  positions ([#289](https://github.com/fubar-coder/bb2-c/issues/289)).
* A module with sample numbers above 31 made the tracker player play
  memory behind its table of samples; `PlayModule` clears such numbers in
  the patterns once ([#295](https://github.com/fubar-coder/bb2-c/issues/295)).
* The tracker player took the finetune of a sample as part of its volume,
  so a volume slide on it jumped to full volume; the sample keeps its
  real volume ([#290](https://github.com/fubar-coder/bb2-c/issues/290)).
* A sample whose repeat starts at 64 KB or later looped from the wrong
  place, because the offset wrapped at 64 KB; it loops from the right
  one ([#291](https://github.com/fubar-coder/bb2-c/issues/291)).
* The tracker effect C (set volume) set only the audio hardware, so a
  following volume slide started from the sample's volume again; it also
  sets the volume the player keeps for the channel
  ([#293](https://github.com/fubar-coder/bb2-c/issues/293)).
* The tracker effects C and F did not limit parameters of $80 and above,
  so `F80` made the song play a new row in every frame and `C80` gave full
  volume; they limit them as all others
  ([#294](https://github.com/fubar-coder/bb2-c/issues/294)).

::fixed-bugs-library{part="elsewhere"}
