---
title: ciatrklib
description: The bugs of Blitz Basic 2 in the RWE userlib ciatrklib that the BB2
  C port fixes.
sidebar:
  order: 2
aliases:
  - /docs/compared-with-original/fixed-bugs/libraries/userlibs/ciatrklib/
---

::fixed-bugs-library

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### The tracker library did not play in CIA mode {#tracker-cia-mode}

The [CIA Protracker library](/docs/libraries/userlibs/ciatrklib/)
plays Protracker modules in two ways. By default, in CIA mode, it sets up
a timer interrupt of a CIA chip when the program starts, and that
interrupt plays the module at the right tempo by itself. After
`ChangeTrackerVBlank` the program calls `PlayTracker` 50 times a second
itself instead.

**What happened.** When the original set up the interrupt, it did not
pass the system the number of the timer's interrupt bit, but the values
it found in memory at the addresses 1 and 2. On a 68000, as in the A500,
reading a long from an odd address is an address error, so a program
that used the library crashed at its start. On other machines the
result depended on what lay at those addresses. With the system's base
above 16 MB, as on an A1200 with Zorro III fast memory, the system took
the value, but the interrupt never played the module, and the program
stayed silent.

This program loads a module whose first row holds the tracker event
`805`, lets it play in CIA mode for one second and reads the event:

```
DEFTYPE .l
r=LoadTrackerModule(0,"mod.tune")
NPrint "loaded ",r
If r<>-1 Then End
r=StartTracker(0)
For i=1 To 50 : VWait : Next
e=GetTrackerEvent
StopTracker
NPrint "event ",e
End
```

**With the original** (in the emulator, an A1200 with Zorro III fast
memory), the module never played its first row:

```
loaded -1
event 0
```

**With the port**

```
loaded -1
event 5
```

The library passes the bit of its timer, and in CIA mode the module
plays on every Amiga.

**What to change.** Nothing. A program that used `ChangeTrackerVBlank`
and `PlayTracker` only to get any sound at all can go back to CIA mode,
where tempo commands of a module take effect.

Issue: [SetCIAInt passes the longs at the addresses 1 and 2 as the ICR bit to AddICRVector (#450)](https://github.com/fubar-coder/bb2-c/issues/450).

### Samples without a loop ended in a buzz {#tracker-sample-end}

A sample of a Protracker module either loops, or it is played once and
then ends. The Amiga's sound hardware cannot simply stop a sample: it
always plays on from the loop start. For a sample without a loop,
Protracker therefore clears the first two words of the sample when it
starts a module, and the voice goes on playing those zero bytes, which
is silence.

**What happened.** `StartTracker` in the original was meant to clear
those words for every sample inside the module, but compared the
module's length with the samples' addresses, and so practically never
cleared them. After a sample without a loop, the voice went on playing
its first two bytes again and again: a buzz, or a click where the sound
stopped. This program loads a module with one sample of eight bytes,
`$11` to `$88`, without a loop, starts it and reads the sample's first
two longs:

```
DEFTYPE .l
r=LoadTrackerModule(0,"mod.tune")
If r<>-1 Then End
ChangeTrackerVBlank
r=StartTracker(0)
StopTracker
a=GetTrackerLocation(0)+2108
NPrint "first long ",Hex$(Peek.l(a))
NPrint "second long ",Hex$(Peek.l(a+4))
FreeTrackerModule 0
End
```

The number 2108 is the size of the module's header and of its one
pattern, so `a` is the address of the sample.

**With the original**

```
first long 11223344
second long 55667788
```

**With the port**

```
first long 00000000
second long 55667788
```

**What to change.** Nothing: samples without a loop end in silence, as
in Protracker. A program that reads a sample of a started module from
memory finds its first four bytes cleared.

Issue: [mt_init compares the module length with the sample addresses: the first longword of the samples is practically never cleared (#438)](https://github.com/fubar-coder/bb2-c/issues/438).

### Fixes in the Protracker play routine {#protracker-play-routine}

The [CIA Protracker library](/docs/libraries/userlibs/ciatrklib/)
and [ritrackerlib](/docs/libraries/userlibs/ritrackerlib/)
play modules with the same Protracker 2.3A play routine, which had the
same faults in both libraries. Each of the following fixes applies to
both. Each point says first what the original did and then what the
port does.

* The first portamento up or down (`1xx`, `2xx`) that a program played
  before any fine portamento (`E1x`, `E2x`) did not slide on its first
  tick; it slides by its whole value from the start
  ([#478](https://github.com/fubar-coder/bb2-c/issues/478)).
* A tremolo (`7xy`) with the ramp-down waveform (`E71`) took its shape
  from the position of the voice's vibrato; it follows the position of
  the tremolo itself
  ([#482](https://github.com/fubar-coder/bb2-c/issues/482)).
* A note with a sample offset (`9xx`) gave the sound hardware the offset
  once, but moved the voice's own start twice, so a retrigger (`E9x`), a
  note delay (`EDx`) or a following note without a sample number started
  at double the offset; the offset counts once
  ([#473](https://github.com/fubar-coder/bb2-c/issues/473)).
* A sample offset (`9xx`) on a sample of 64 KB or more played only 2
  bytes of the sample; it plays the sample from the offset
  ([#472](https://github.com/fubar-coder/bb2-c/issues/472)).
* A sample whose loop starts at 64 KB or later looped from a place 64 KB
  too early; it loops from its loop start
  ([#479](https://github.com/fubar-coder/bb2-c/issues/479)).
* An arpeggio (`0xy`) on one of the highest notes could play a much
  lower note, or a value from the player's own data; it plays at most
  the highest note of the period table
  ([#474](https://github.com/fubar-coder/bb2-c/issues/474)).
* After a stop (`F00`) on voice 1, 2 or 3, the voices after it in the
  same row read their notes from the Amiga's custom chip registers and
  went on playing random samples; they read the notes of the row
  ([#475](https://github.com/fubar-coder/bb2-c/issues/475)).
* A note period below the lowest note of the table (1 to 112, which
  Protracker does not write) played a low note or a value from the
  player's own data; it plays the highest note
  ([#481](https://github.com/fubar-coder/bb2-c/issues/481)).
* `StartTracker`, `DecodeModule` and `TrackInit` look for the module's
  highest pattern in its song positions. The original never read the
  last of the 128 positions, so a pattern used only there was not
  counted and the samples were taken from the wrong place. A new
  highest pattern at the position before it made the search read on
  through 64 KB behind the module, so that the samples were looked for
  far behind it and memory there was cleared or copied. All 128
  positions are read
  ([#477](https://github.com/fubar-coder/bb2-c/issues/477)).
* A module with sample numbers above 31 in its patterns made the sound
  hardware play memory behind the sample table; `StartTracker` and
  `TrackInit` set such numbers to 0 in the module, once
  ([#480](https://github.com/fubar-coder/bb2-c/issues/480)).
* A sample with a finetune value above 15 played periods read from far
  behind the period table; `StartTracker` and `TrackInit` keep only the
  low four bits of the value, once
  ([#476](https://github.com/fubar-coder/bb2-c/issues/476)).

### More fixes {#more-fixes}

Each point says first what the original did and then what the port
does.

* Removing the CIA interrupt of the tracker library, by
  `ChangeTrackerVBlank`, by `StartTracker` in CIA mode or at the end of
  the program, switched the audio filter, and with it the power LED, over;
  both stay as they are
  ([#449](https://github.com/fubar-coder/bb2-c/issues/449)).
* Without runtime error checking, `StartTracker` of an empty slot
  returned the slot number, a true value, although nothing plays; it
  returns 0 (False)
  ([#451](https://github.com/fubar-coder/bb2-c/issues/451)).
* Without runtime error checking, `GetModuleName$` of an empty slot gave
  a string of ten undefined characters per slot number; it is the empty
  string
  ([#437](https://github.com/fubar-coder/bb2-c/issues/437)).
* `CheckTrackerID` of a module without the mark "M.K." or "M!K!", or of
  an empty slot, returned the contents of memory address 0; it returns 0
  (False) ([#430](https://github.com/fubar-coder/bb2-c/issues/430)).
* `CheckTrackerEvent` without a new tracker event returned the value of
  the last event with undefined upper bytes, so `If CheckTrackerEvent`
  was almost always true; it returns 0 (False)
  ([#431](https://github.com/fubar-coder/bb2-c/issues/431)).
* The tracker event `8xx` came one tick after its row, and at speed 1
  never, so `WaitTrackerEvent` waited for ever; it comes on its row's
  first tick ([#435](https://github.com/fubar-coder/bb2-c/issues/435)).
* `WaitTrackerEvent` called the graphics library's wait through whatever
  library base the program happened to hold, which crashed it; the
  compiler sets up the graphics library for the call
  ([#453](https://github.com/fubar-coder/bb2-c/issues/453)).
* `LoadTrackerModule` called AmigaDOS through whatever library base the
  program happened to hold, which crashed it; the compiler sets up
  AmigaDOS for the call
  ([#443](https://github.com/fubar-coder/bb2-c/issues/443)).
* `LoadTrackerModule` loaded a file whose size ends in the byte $FD, $FE
  or $FF 253 bytes short; it loads the whole file
  ([#442](https://github.com/fubar-coder/bb2-c/issues/442)).
* After a read error `LoadTrackerModule` released the file's lock a
  second time; it releases it once
  ([#444](https://github.com/fubar-coder/bb2-c/issues/444)).
* `LoadTrackerModule` opened the file with a function of Kickstart 2.0,
  so a program crashed at it on Kickstart 1.x, and on any Kickstart it
  went on reading when the file could not be opened, such as a
  directory. It
  opens the file by its name, which works on every Kickstart, and
  reports a file that cannot be opened: the slot stays empty, and the
  result is the AmigaDOS error code
  ([#1146](https://github.com/fubar-coder/bb2-c/issues/1146)).
* `DecodeModule` took a module at an address of $80000000 or above for
  one in chip memory and left it where the sound hardware cannot play
  it; it copies such a module into chip memory
  ([#432](https://github.com/fubar-coder/bb2-c/issues/432)).
* `DecodeModule` copied a module whose size ends in $FFFD to $FFFF into
  chip memory 64 KB short; it copies the whole module
  ([#434](https://github.com/fubar-coder/bb2-c/issues/434)).
* `ModulePatterns` returned the song length, the number of song
  positions; it returns the number of patterns of the module, as
  documented ([#445](https://github.com/fubar-coder/bb2-c/issues/445)).
* `ModulePositionJump n` went on at song position n+1; it goes on at
  position n ([#447](https://github.com/fubar-coder/bb2-c/issues/447)).
* `TrackerVolume 0` and `TrackerVolume 64` changed only the volume of the
  notes that started afterwards, so a fade-out to 0 stayed audible; like
  every other volume they also change the notes that are playing
  ([#452](https://github.com/fubar-coder/bb2-c/issues/452)).
* Removing the CIA interrupt of the tracker library always removed the
  interrupt of timer A: where the library used timer B, its own
  interrupt stayed installed, and another program's interrupt on timer A
  was taken away; it removes the interrupt of its own timer
  ([#448](https://github.com/fubar-coder/bb2-c/issues/448)).
* The effect `E8x` (Karplus-Strong) on a voice without a loop rewrote
  64 KB of memory from the loop start, from address 0 on a voice that
  had not played a sample yet; it does nothing there
  ([#439](https://github.com/fubar-coder/bb2-c/issues/439)).
* `PauseModule` kept its pause whatever the program did in between.
  After a pause, the next `PauseModule` did not pause a module started
  again, but played a module that `StopTracker` had stopped, or the
  freed memory of a module that `FreeTrackerModule` had freed; two
  `PauseModule` after `StopTracker` played the stopped module again.
  `PauseModule` pauses only a module that plays and continues only the
  module it paused. `StartTracker`, `StopTracker`,
  `ChangeTrackerVBlank` and freeing the paused module, also by loading
  or decoding another module into its slot, end the pause. A program
  that used two `PauseModule` to continue a module after `StopTracker`,
  or after the module stopped itself with the effect `F00`, finds it
  stopped; `StartTracker` plays it again from the beginning
  ([#1076](https://github.com/fubar-coder/bb2-c/issues/1076)).
* After a run that called `ChangeTrackerVBlank`, the original started
  the next run of the program from the editor without the CIA
  interrupt, so a program that does not call `PlayTracker` itself
  stayed silent. Every run starts in CIA mode
  ([#1165](https://github.com/fubar-coder/bb2-c/issues/1165)).

#### `FreeTrackerModule` of the module that plays {#freetrackermodule-playing}

**What happened.** `FreeTrackerModule` freed a module that the CIA
Protracker library was still playing. The module did not stop: the
library went on reading its notes and samples from the freed memory,
which other parts of the program or the system could already be using,
also with runtime error checking.

```
DEFTYPE .l
r=LoadTrackerModule(0,"mod.tune")
If r<>-1 Then End
r=StartTracker(0)
FreeTrackerModule 0
NPrint "freed"
End
```

The original printed `freed`, and the music went on from the freed
memory.

**With the port.** With runtime error checking, `FreeTrackerModule` of
the module that plays is the runtime error "Tracker: Stop the module
before freeing it.", before anything is freed. A module stopped with
`StopTracker` can be freed. Without runtime error checking, the module
is freed as before.

**What to change.** Call `StopTracker` before `FreeTrackerModule`.

Issue: [FreeTrackerModule frees a module that is still playing (#436)](https://github.com/fubar-coder/bb2-c/issues/436).

::fixed-bugs-library{part="elsewhere"}
