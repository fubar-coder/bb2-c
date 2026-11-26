---
title: medlib
weight: 39
description: The bugs of Blitz Basic 2 in the command library medlib that the BB2 C port fixes.
---

{{< fixed-bugs-library >}}

## Changes for existing programs

### `GetMedVolume` silenced the channel it read {#getmedvolume-clears}

`GetMedNote`, `GetMedInstr` and `GetMedVolume` tell what a channel of a
MED module plays, for example for graphics that follow the music.
`GetMedNote` and `GetMedInstr` give 0 once they have been read, until the
channel plays its next note, so that a program can see when a new note
starts.

**What happened.** The original's `GetMedVolume` also set its value to 0
after reading it. But that value is the volume the player gives the
channel in every step of the music: after the first `GetMedVolume` the
channel was silent until a note with an instrument number or a volume
command set the volume again. A program that shows the volume in every
frame silenced its own music:

```
LoadMedModule 0,"song.med"
StartMedModule 0
Repeat
  VWait
  PlayMed
  NPrint GetMedVolume(0)
Until Joyb(0)
StopMed
End
```

With the original, every note on channel 0 sounded only until the next
call of `PlayMed`, and the program printed its volume
once and then 0. With the port, the channel plays as it does without
`GetMedVolume`, and the program prints the volume of the note in every
frame.

**What to change.** A program that used the 0 after the first read to see
that a new note has started reads `GetMedNote` instead, which still gives
0 once it has been read. A program that reads the volume only to show it
needs no change.

Issue: [GetMedVolume clears the volume, the channel goes silent (#212)](https://github.com/fubar-coder/bb2-c/issues/212).

## Other fixes {#other-fixes}

In the lists of this part, each point says first what the original did
and then what the port does, unless the introduction of a list says
otherwise.

### Music players (MED and tracker modules) {#music-players}

Two more fixes of them change what an existing program does:
[`GetMedVolume` silenced the channel it read](#getmedvolume-clears) and
[A pattern break in the last song position played on behind the end]({{< relref "/docs/compared-with-original/fixed-bugs/libraries/commands/trackerlib#tracker-break-song-end" >}}).

* `StartMedModule` of a module with MIDI dumps read the address of a
  further module from a wrong place and could change foreign memory as
  if it were one; it reads the right address
  ([#207](https://github.com/fubar-coder/bb2-c/issues/207)).
* `SetMedMask` with bits above 3 set let the notes of tracks 4 to 15
  through to the player, which wrote them behind its data and to
  addresses read from that data as audio registers; it keeps only the channels 0 to 3
  ([#209](https://github.com/fubar-coder/bb2-c/issues/209)).
* A channel switched off with `SetMedMask` did not skip its notes, so
  every later channel played the notes of the channel before it; the
  notes of a switched-off channel are skipped
  ([#334](https://github.com/fubar-coder/bb2-c/issues/334)).
* With a channel mask with gaps, such as `SetMedMask %0101`, the higher
  channels did not get the repeat part of their samples; every playing
  channel gets it ([#208](https://github.com/fubar-coder/bb2-c/issues/208)).
* With runtime error checking, `JumpMed` without a playing module is the
  error "No Module Playing"; the original wrote the position into an
  exception vector of the processor
  ([#210](https://github.com/fubar-coder/bb2-c/issues/210);
  [`JumpMed` without a playing module]({{< relref "/docs/compared-with-original/new-errors/runtime-errors#jumpmed-no-module" >}})).
* A finetune outside -8 to 7, from the instrument or the command 15xx,
  made the MED player read a period table from outside its table; 15xx
  ignores such a value, and the instrument's is taken as 0
  ([#211](https://github.com/fubar-coder/bb2-c/issues/211)).
* The MED position jump Bxx could take a byte of another track of the
  same line as part of the position, and jumped to position 0 or read
  before the play sequence; it jumps to position xx
  ([#203](https://github.com/fubar-coder/bb2-c/issues/203)).
* The MED pattern break 1Dxx could likewise take a byte of another track
  as part of the line number, and then played the memory behind the
  block; it goes to line xx of the next block
  ([#1066](https://github.com/fubar-coder/bb2-c/issues/1066)).
* In a MED module with more than 8 tracks, the player let the commands of
  tracks 8 to 11 run, and the commands 03 and FFD there wrote into its
  data of other tracks; tracks 8 to 11 are ignored like the other tracks
  behind track 3, as a MED player does
  ([#1067](https://github.com/fubar-coder/bb2-c/issues/1067)).
* The MED tremolo 7xy with a speed of 0 ran the vibrato code in the first
  step of its line; it stays a tremolo
  ([#204](https://github.com/fubar-coder/bb2-c/issues/204)).
* The MED note delay and retrigger 1Fxy took the retrigger interval from
  a value the delayed note had left, so the note could sound twice; it
  uses its own y
  ([#205](https://github.com/fubar-coder/bb2-c/issues/205)).
* MED synth and hybrid sounds with a period below 113 went to the audio
  hardware unlimited, because the limit was set in the wrong place; the
  period is limited to 113
  ([#206](https://github.com/fubar-coder/bb2-c/issues/206)).
* `GetMedVolume`, `GetMedNote` and `GetMedInstr` changed the pointer at
  which a string expression builds its parts, so a line such as
  `a$="V"+Str$(GetMedVolume(0))` gave a cut string or crashed the
  program; they leave the pointer alone
  ([#1061](https://github.com/fubar-coder/bb2-c/issues/1061)).

{{% fixed-bugs-library part="elsewhere" %}}
