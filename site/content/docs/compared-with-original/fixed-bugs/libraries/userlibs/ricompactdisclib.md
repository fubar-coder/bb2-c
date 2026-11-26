---
title: ricompactdisclib
weight: 13
description: The bugs of Blitz Basic 2 in the RWE userlib ricompactdisclib that the BB2 C port fixes.
---

{{< fixed-bugs-library >}}

## Changes you may notice

### Track lengths, volume and the SCSI flag of ricompactdisclib {#compactdisc-results}

[ricompactdisclib]({{< relref "/docs/libraries/userlibs/ricompactdisclib" >}})
plays audio CDs through cd.device, the CD drive of the CD32, or through
another device. Three of its fixes change values that a working program
may see.

**`CDTrackLength 1`.** `CDTrackLength` gives the length of a track in
seconds, from the start of that track to the start of the next one. For
track 1 the original returned the start of track 2, so the gap at the
beginning of the CD before track 1, normally 2 seconds, was counted in.
With the port, track 1 is measured like every other track and comes out
that much shorter.

**`CDVolume`.** `CDVolume volume,lengthoffade` lets the drive fade to
`volume` over `lengthoffade` frames of 1/75 second. The original gave
the device the two values the other way round: `CDVolume 32767,0`, full
volume at once, asked for a fade to silence that took 32767 frames,
more than 7 minutes. The port passes them as they are named.

**`OpenCD`.** `OpenCD devicename$,unit,scsiflag` opens another device,
and with a SCSI flag other than 0 the library reads the table of
contents and plays tracks with SCSI commands. The original kept the
flag, the name and the unit for every later `OpenCD`: `OpenCD
devicename$,unit` after one with the flag still used SCSI commands, and
`OpenCD` without arguments opened the device of the call before instead
of unit 0 of cd.device, as the guide describes. This program opens the
emulator's SCSI device first with and then without the flag, then
cd.device:

```
DEFTYPE .l
r=OpenCD("uaescsi.device",0,-1)
NPrint "open scsi ",r
If r
  t=CDReadTOC
  NPrint "tracks scsi ",CDNumTracks
  CloseCD
EndIf
r=OpenCD("uaescsi.device",0)
NPrint "open ",r
If r
  t=CDReadTOC
  NPrint "tracks ",CDNumTracks
  CloseCD
EndIf
r=OpenCD
NPrint "open default ",r
If r
  CloseCD
EndIf
End
```

**With the original**

```
open scsi -1
tracks scsi 2
open -1
tracks 2
open default -1
```

**With the port**

```
open scsi -1
tracks scsi 2
open -1
tracks 0
open default 0
```

The emulator's SCSI device answers SCSI commands only, and there is no
cd.device. With the port, the second `OpenCD` uses cd.device's commands,
which this device does not answer, so no table of contents is read, and
`OpenCD` without arguments tries cd.device and fails.

**What to change.** Give the SCSI flag to every `OpenCD` that needs it,
and the device name to every `OpenCD` for another device. A program that
took the 2 seconds off `CDTrackLength 1` itself, or swapped the arguments
of `CDVolume` to make it work, drops that.

Issues:

* [CDTrackLength 1 returns the start of track 2, not its distance to the start of track 1 (#584)](https://github.com/fubar-coder/bb2-c/issues/584)
* [CDVolume puts the volume into io_Length and the fade time into io_Offset, the reverse of cd.device's CD_ATTENUATE (#982)](https://github.com/fubar-coder/bb2-c/issues/982)
* [OpenCD devicename$,unit keeps the scsiflag of an earlier OpenCD (#577)](https://github.com/fubar-coder/bb2-c/issues/577)

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in ricompactdisclib {#ricompactdisclib-fixes}

These fixes concern
[ricompactdisclib]({{< relref "/docs/libraries/userlibs/ricompactdisclib" >}}),
the audio CD commands of Red When Excited. `CDTrackLength 1`, `CDVolume`
and the SCSI flag of `OpenCD` are described in
[Changes you may notice](#compactdisc-results).

* Programs that use the library start on Kickstart 1.x. The library
  builds its message port and its requests itself, with functions that
  every Kickstart has; the original called functions of Kickstart 2.0
  there, and every program with the library crashed before its first
  line ([#1138](https://github.com/fubar-coder/bb2-c/issues/1138)).
* `OpenCD` returns 0 when the device cannot be opened. The original
  returned a number left over from freeing its request, 200 on
  Kickstart 3.1
  ([#575](https://github.com/fubar-coder/bb2-c/issues/575)).
* `OpenCD devicename$,unit` refuses a device name of 40 characters or
  more with 0, as the guide asks for a name of less than forty. The
  original copied it into a buffer of 40 bytes and overwrote the
  library's data
  ([#576](https://github.com/fubar-coder/bb2-c/issues/576)).
* With runtime error checking, `OpenCD` while a device is open stops with
  "CD device already open: CloseCD first". The original opened the new
  device and lost the one opened first, which stayed open
  ([Runtime errors of the libraries]({{< relref "/docs/compared-with-original/new-errors/runtime-errors#compactdisc-checks" >}}),
  [#578](https://github.com/fubar-coder/bb2-c/issues/578)).
* `CDPlayTrack` uses a request of its own while the music plays. In the
  original every other command, such as `CDUpdateInfo`, `CDStatus`,
  `CDPause` or `CDVolume`, filled and sent the request of the track that
  was still playing
  ([#1156](https://github.com/fubar-coder/bb2-c/issues/1156)).
* `CDStop`, `CloseCD`, the next `CDPlayTrack` and the end of the program
  stop a play and wait until the device has taken it back. The original
  did not wait, and the device's reply stayed behind
  ([#574](https://github.com/fubar-coder/bb2-c/issues/574)).
* `CDPlayTrack` returns -1 when the play was sent and 0 without a device.
  The original returned whatever value the system had left
  ([#1157](https://github.com/fubar-coder/bb2-c/issues/1157)).
* With the SCSI flag, `CDPlayTrack` plays the tracks it is asked for,
  taken from the table of contents that `CDReadTOC` read, and returns -1
  if the drive took the command, else 0. The original ignored its
  arguments and always played the same blocks, test values of the
  author ([#580](https://github.com/fubar-coder/bb2-c/issues/580)).
* With the SCSI flag, `CDPlayTrack` no longer crashes a 68000 with an
  address error
  ([#579](https://github.com/fubar-coder/bb2-c/issues/579)).
* `CDReadTOC` returns 0 and keeps no table of contents when reading it
  fails or there is no memory for it. The original returned -1 and left
  the track functions reading whatever the table held
  ([#581](https://github.com/fubar-coder/bb2-c/issues/581)).
* `CDNumTracks`, `CDFirstTrack`, `CDLastTrack` and `CDTrackLength` return
  0 before `CDReadTOC`, and with the SCSI flag they read the SCSI table of
  contents. The original read memory at address 0 there, and with the
  flag only `CDNumTracks` worked
  ([#582](https://github.com/fubar-coder/bb2-c/issues/582)).
* `CDTrackLength` of the last track ends at the end of the CD's music,
  and with runtime error checking a track the CD does not hold stops
  with "Track number out of range". The original read the last track's
  end from memory the table of contents does not fill
  ([Runtime errors of the libraries]({{< relref "/docs/compared-with-original/new-errors/runtime-errors#compactdisc-checks" >}}),
  [#583](https://github.com/fubar-coder/bb2-c/issues/583)).
* `CDTrackLength` is right for track 44 and the tracks after it
  ([#585](https://github.com/fubar-coder/bb2-c/issues/585)).
* With runtime error checking, `CDTrackMins` and `CDTrackSecs` with an
  offset other than 0 or 1 stop with "Offset must be 0 or 1". The
  original read other memory of the library
  ([Runtime errors of the libraries]({{< relref "/docs/compared-with-original/new-errors/runtime-errors#compactdisc-checks" >}}),
  [#586](https://github.com/fubar-coder/bb2-c/issues/586)).
* `CDUpdateInfo` returns 0 without an open device. The original returned
  a meaningless address
  ([#587](https://github.com/fubar-coder/bb2-c/issues/587)).
* `CDStatus` asks the device for its whole status record. The original
  asked for a shorter one, and a device that writes the whole record
  wrote 8 bytes behind the library; on Workbench 3.1 the program then
  ended with an alert
  ([#948](https://github.com/fubar-coder/bb2-c/issues/948)).

{{% fixed-bugs-library part="elsewhere" %}}
