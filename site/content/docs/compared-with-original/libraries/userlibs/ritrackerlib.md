---
title: ritrackerlib
weight: 48
description: The bugs of Blitz Basic 2 in the RWE userlib ritrackerlib that the BB2 C port fixes.
aliases: [/docs/compared-with-original/fixed-bugs/libraries/userlibs/ritrackerlib/]
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Voices left out by `TrackerChannels` played the wrong notes {#trackerchannels-voices}

[ritrackerlib]({{< relref "/docs/libraries/userlibs/ritrackerlib" >}})
plays a Protracker module on the four voices of the Amiga.
`TrackerChannels` leaves voices out, so that the program can play its
own sound effects on them while the module goes on with the others. Bit
3 of its value stands for voice 1 and bit 0 for voice 4, so
`TrackerChannels %0111` leaves out voice 1.

**What happened.** A row of a pattern holds one note for each voice,
side by side, and the player read them from left to right. It moved on
to the next note only after it had played one, so it did not move on
for a voice that was left out. Each voice after that one played the
notes of the voice before it, with their samples and effect commands,
and the notes of the last voice were not played at all. This program
plays a module with voice 1 left out:

```
LoadBank 0,"mod.tune",2
TrackInit Bank(0)
TrackerChannels %0111
SetInt 5
  TrackPlay
End SetInt
MouseWait
TrackStop
```

The table shows whose notes each voice plays. It is worked out from the
play routines of the original and of the port, not recorded from a run
of this program; the library's test compares the sound registers of all
four voices with voice 2 left out and with voices 1 and 4 left out.

| Voice | With the original | With the port |
|---|---|---|
| 1 | silent | silent |
| 2 | the notes of voice 1 | its own notes |
| 3 | the notes of voice 2 | its own notes |
| 4 | the notes of voice 3 | its own notes |

The notes of voice 4 were lost, and so were its speed, jump and break
commands, which change how the whole module plays.

**With the port** every voice that plays reads its own notes. A voice
that is left out reads its notes too, without a sound, so its speed,
jump and break commands take effect as if all four voices played.

**What to change.** Nothing. The voices that play sound as the module
was written.

Issue: [TrackerChannels: a masked voice does not advance the pattern offset – the following voices play the notes of the voice before (#568)](https://github.com/fubar-coder/bb2-c/issues/568).

### Other fixes in ritrackerlib {#ritrackerlib-fixes}

These fixes concern
[ritrackerlib]({{< relref "/docs/libraries/userlibs/ritrackerlib" >}})
alone.

* A voice left out by `TrackerChannels` stays silent and free for the
  program's own sounds, since the module no longer writes to it; the
  original still wrote the module's effects and sample loops to it, so
  it could become audible again
  ([#567](https://github.com/fubar-coder/bb2-c/issues/567)).
* The commands `F80` to `FFF` are tempo commands, as in Protracker; the
  original took them as speeds of 128 to 255 ticks per row, so the
  module nearly stopped
  ([#569](https://github.com/fubar-coder/bb2-c/issues/569)).
* A tempo command (`F20` to `FFF`) no longer writes to a timer of a CIA
  chip that the library never reserved and that another program may
  use. It does not change the speed: the library plays one tick each
  time the program calls `TrackPlay`, so the program sets the speed,
  and that is how the library works
  ([#571](https://github.com/fubar-coder/bb2-c/issues/571)).
* `TrackInit` no longer clears up to 4 bytes of memory behind a module
  whose last samples are empty
  ([#570](https://github.com/fubar-coder/bb2-c/issues/570)).
* `Tracker On` after `Tracker Off` continues the module; the original
  left it stopped until `TrackInit` or `TrackerPause 0`. A module that
  was stopped or paused before `Tracker Off` stays so
  ([#572](https://github.com/fubar-coder/bb2-c/issues/572)).
* `TrackInit` while the player is switched off prepares the module, so
  that `Tracker On` plays it; the original played the module of before,
  or memory from address 0 if there was none
  ([#573](https://github.com/fubar-coder/bb2-c/issues/573)).

{{% fixed-bugs-library part="elsewhere" %}}
