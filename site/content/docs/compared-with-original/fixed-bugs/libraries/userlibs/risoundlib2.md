---
title: risoundlib2
weight: 43
description: The bugs of Blitz Basic 2 in the RWE userlib risoundlib2 that the BB2 C port fixes.
---

{{< fixed-bugs-library >}}

## Other fixes {#other-fixes}

In the lists of fixes below, each point says first what the port does
and then what the original did, unless the introduction of a list says
otherwise.

### Fixes in risoundlib2 {#risoundlib2-fixes}

These fixes concern
[risoundlib2]({{< relref "/docs/libraries/userlibs/risoundlib2" >}}),
the second sound library of Reflective Images, with its commands for
sound output through AHI. Its `CludgeSound` also has the fixes in the
list [Fixes in risoundlib]({{< relref "/docs/compared-with-original/fixed-bugs/libraries/userlibs/risoundlib#risoundlib-fixes" >}}).

* `OpenAHI` remembers the device it opened, so `PlaySoundAHI` and
  `PlayRawAHI` play through AHI, `CloseAHI` and the end of the program
  close the device, and a second `OpenAHI` returns 0 while the device
  is open. In the original none of this happened: the two play commands
  returned at once and played nothing, the device was never closed, and
  a second `OpenAHI` opened it again
  ([#924](https://github.com/fubar-coder/bb2-c/issues/924)).
* `OpenAHI` works again after `CloseAHI`: `CloseAHI` closes the device
  and keeps the library's request for the next `OpenAHI`. The original's
  `CloseAHI` freed the request, and a later `OpenAHI` wrote to memory
  address 48 and called the device without a request
  ([#927](https://github.com/fubar-coder/bb2-c/issues/927)).
* A sound that `PlaySoundAHI` sent and that still plays is stopped by
  the next `PlaySoundAHI` or `PlayRawAHI`, by `CloseAHI` and at the end
  of the program, and the library takes its request back. The original
  sent its one request again while AHI still held it
  ([#933](https://github.com/fubar-coder/bb2-c/issues/933)).
* `PlaySoundAHI` plays the whole sample. The original gave AHI the
  length in words as the length in bytes, which would have played the
  first half
  ([#932](https://github.com/fubar-coder/bb2-c/issues/932)).
* `CludgeSound` of this library writes nothing behind the sound object,
  and `PlaySoundAHI` takes the pitch from the sound's period, the pitch
  `Sound` plays it at. The original stored the sample rate over the start
  of the next sound object, which damaged that sound's sample address
  ([#926](https://github.com/fubar-coder/bb2-c/issues/926)).
* `AHIOutputRequest` gives the requester the position, size, audio mode
  and frequency it is called with. The original wrote width, height, mode
  and frequency into the wrong places of the requester's settings, and a
  mode of 0 ended them early
  ([#931](https://github.com/fubar-coder/bb2-c/issues/931)).
* The forms of `AHIOutputRequest` with 1, 2 and 4 arguments take the
  audio mode, the mode and the frequency, and the position and size. The
  original ran the code of another form for each, so their arguments
  were lost or taken as other values
  ([#928](https://github.com/fubar-coder/bb2-c/issues/928)).
* `AHIOutputRequest` opens the requester on the screen the program
  uses. The original never told AHI the screen
  ([#929](https://github.com/fubar-coder/bb2-c/issues/929)).
* `AHIRequestedID` and `AHIRequestedFreq` return the audio mode and the
  frequency the user chose in the last requester confirmed. The original
  returned the values the requester started with
  ([#934](https://github.com/fubar-coder/bb2-c/issues/934)).
* `AHIOutputRequest` frees its requester again after each call. The
  original made a new one for every call and never freed one
  ([#930](https://github.com/fubar-coder/bb2-c/issues/930)).
* Programs with the library start on Kickstart 1.x, where the AHI
  commands do nothing and `OpenAHI` returns 0. The original called
  functions of Kickstart 2.0 there and crashed at the start
  ([#1139](https://github.com/fubar-coder/bb2-c/issues/1139)).

### More fixes {#more-fixes}

Each point says first what the original did and then what the port
does.

* `CludgeSound` of the second sound library kept the mark by which
  `DecodeSound` keeps such data from being freed, so the copy of the sample it makes was never freed before the
  program ended; the mark is cleared
  ([#925](https://github.com/fubar-coder/bb2-c/issues/925)).

{{% fixed-bugs-library part="elsewhere" %}}
