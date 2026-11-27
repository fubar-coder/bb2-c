---
title: brexxlib
description: The bugs of Blitz Basic 2 in the command library brexxlib that the
  BB2 C port fixes.
sidebar:
  order: 13
aliases:
  - /docs/compared-with-original/fixed-bugs/libraries/commands/brexxlib/
---

::fixed-bugs-library

## Changes for existing programs

### A simulated event ended a running `Record` or `PlayBack` {#simulated-event-ends-tape}

A *tape* holds mouse and keyboard events: `Record` records what the user
does onto it, and `PlayBack` plays it back. The commands `Type`,
`AbsMouse`, `RelMouse`, `MouseButton` and `ClickButton` make such events
themselves, as if the user typed or moved the mouse; they can be used
while a tape records or plays, for example to type a text into the
program that a tape drives.

**What happened.** Each of these commands ended a running recording or
playback, whether its event was executed or went onto a tape with
`TapeTrap`. `XStatus` was 0 from then on, and the rest of a tape that was
playing was lost. This program moves the pointer while it records:

```
Record 0
VWait 10
AbsMouse 40,30
VWait 10
NPrint "status ",XStatus
Record
NPrint "stopped ",XStatus
End
```

**With the original**

```
status 0
stopped 0
```

**With the port**

```
status 1
stopped 0
```

The recording goes on after `AbsMouse`, with the same time base, and the
pointer move is on the tape; `Record` without a tape ends it.

**What to change.** A program that used one of these commands to stop a
recording or a playback ends it with `Record` or `PlayBack` without a
tape number.

Issue: [Appending to a tape clears the mode, Record and PlayBack stop (#96)](https://github.com/fubar-coder/bb2-c/issues/96).

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Tapes and simulated input {#tapes}

`Record`, `PlayBack`, `PlayWait` and `LoadTape` record mouse and
keyboard events onto tapes and play them back. `Type`, `AbsMouse`,
`RelMouse`, `MouseButton` and `ClickButton` make such events
themselves. One more fix of them changes what an existing program sees:
[a simulated event ended a running `Record` or `PlayBack`](#simulated-event-ends-tape).

In this list, each point says first what the port does and then what
the original did.

* `TapeTrap` without a tape number ends the redirection of the simulated
  events to a tape; the original went on trapping them and chained the
  next events into the program's string memory instead of executing them
  ([#95](https://github.com/fubar-coder/bb2-c/issues/95)).
* `ClickButton` releases the button it pressed. The original released
  the left button after an executed `ClickButton 1`, and under `TapeTrap`
  with `QuietTrap On` it recorded `ClickButton 0` as the left button
  pressed and the right one released
  ([#97](https://github.com/fubar-coder/bb2-c/issues/97)).
* `RelMouse` makes a relative mouse event (RAWMOUSE), as a real mouse
  does; the original made a pointer position event (POINTERPOS), which
  moved the pointer the same way but gave tapes, saved tapes and
  programs that look at the events another kind of event
  ([#98](https://github.com/fubar-coder/bb2-c/issues/98)).
* `PlayWait` without a tape number waits until the running playback has
  finished, as its help says; the original refused this form with
  "Syntax Error". `PlayWait Tape#` works as in the original, which plays
  `Tape#` in place of a running playback and waits for its end, but
  returns at once for an empty tape, where the original waited for ever
  ([#99](https://github.com/fubar-coder/bb2-c/issues/99);
  [`PlayWait` without a tape number](/docs/compared-with-original/extensions/#playwait-without-tape)).
* `PlayBack` plays a recorded event in front of the first input event
  that came after its time; the original compared the times only with
  the first of the input events that arrived together, so it could play
  a tape event behind input that came after it
  ([#100](https://github.com/fubar-coder/bb2-c/issues/100)).
* `LoadTape` of a file shorter than the 8 bytes of the start time leaves
  the tape empty; the original took the bytes of the file as the start
  time ([#101](https://github.com/fubar-coder/bb2-c/issues/101)).
* `Type` skips a character that has no key; the original typed the key
  code $FFFF with the Shift key
  ([#102](https://github.com/fubar-coder/bb2-c/issues/102)).
* `XStatus` returns 0, 1 or 2; right after `Record`, `PlayBack` or an
  executed simulated event the original returned 32768 more, until the
  system had handled the next input event
  ([#103](https://github.com/fubar-coder/bb2-c/issues/103)).
* Without a free signal, `PlayWait` returns at once instead of waiting
  for ever, and the end of the program frees no signal that the library
  did not get
  ([#104](https://github.com/fubar-coder/bb2-c/issues/104)).

::fixed-bugs-library{part="elsewhere"}
