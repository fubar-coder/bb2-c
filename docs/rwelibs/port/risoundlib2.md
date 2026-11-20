# risoundlib2 (45): CludgeSound and AHI

Reflective Images. The successor of risoundlib (`port/risoundlib.md`):
the same `!libheader {#risoundlib,…}` and CludgeSound as its first
command, plus sound output through AHI (ahi.device): `OpenAHI
unit[,deviceID]`, `CloseAHI`, `PlaySoundAHI sound[,volume[,balance]]`,
`PlayRawAHI address,length,frequency[,volume[,balance]]`,
`AHIOutputRequest [x,y,width,height[,modeid[,frequency]]]` (AHI's audio
mode requester), `AHIRequestedID`, `AHIRequestedFreq`. init creates a
message port and an `AHIRequest` (80 bytes); OpenAHI opens ahi.device
(version 4) with it and keeps `io_Device` as the library base for the
requester calls (`AHI_AllocAudioRequestA`, `AHI_AudioRequestA`);
CloseAHI closes the device, finit also deletes request and port. CludgeSound is risoundlib's;
PlaySoundAHI plays a sound at 3579545 / its period.

* **Source:** `rwelibsrc/soundlib/risoundlib2.bb` (= `.bak`), in the
  `BB22` format (README §4.1; `bbsrc bb22`). Readable:
  `build/amiga/rwelibsrc/soundlib/risoundlib2.bb`.
* **Reference:** no binary exists; the reference build of the source
  (README §3.1, REFERENCE `refbuild`, 2216 bytes). Its `.xtra` loads the
  residents `bb2objtypes.res`, `lvo.res`, `amigalibs.res` and `AHI.res`;
  `AHI.res` exists nowhere and is made from the AHI SDK of the toolchain
  (`libs/userlibs/rwe/residents.cmake`, README §3.1): ahi.device's
  functions as `#AHI_Name=-offset` (as the source writes them) and the
  constants of `devices/ahi.i`. Declarations equal (ctest
  `rwe_declarations_risoundlib2`).
* **C file:** `libs/userlibs/rwe/risoundlib2.c`; test
  `libs/tests/rwe/risoundlib2.c` (snitch case "rwe risoundlib2").

## Number and variant

risoundlib2 has risoundlib's number 45 (`#risoundlib`) and its command
CludgeSound (same token, index 0): the two cannot be in one set of
libraries (a number twice, `rwe_numbers`), so not both in `rwe-1999`. Design (the smallest
that keeps both): the variant `rwe-1999-ahi`
(`libs/variants/rwe-1999-ahi/libnums.h`) has the numbers of `rwe-1999`
unchanged – its acidlibs and deflibs are `rwe-1999`'s byte for byte
(ctest `variant_rwe_1999_ahi`) – and risoundlib2 replaces risoundlib in
it: `RWE_OWN_VARIANTS` got an optional third field, the RWE library the
variant replaces (`risoundlib2|rwe-1999-ahi|risoundlib`). The library is
built to `build/amiga/variants/rwe-1999-ahi/userlibs/rwe/risoundlib2.obj`
and used with the RWE userlibs of `rwe-1999` except `risoundlib.obj`.
ctest `rwe_numbers_rwe-1999-ahi`: no number twice in that set;
`rwe_numbers_rwe-1999-ahi_clash` (must fail): risoundlib2 with
`rwe-1999`'s acidlibs, deflibs and risoundlib (45 twice) – the reason
for the variant. The readable source is detokenised with the same set.
Not chosen: a new number for risoundlib2 (no source or list gives one,
and CludgeSound would still be there twice), or risoundlib2 instead of
risoundlib in `rwe-1999` (the BSS2 binary is risoundlib).

## C

All C, one thunk per entry (keeps d1/a0/a1; the originals change d0, d1,
a0, a1 and a6, AHIOutputRequest d2–d5 too). exec and ahi.device's
vectors through `BLITZ_BASEJSR` where the original passes the whole d0
on (OpenDevice, SendIO, DoIO, the AHI calls); memlib through
`BLITZ_ALIBJSR`. The tag list of the requester is the original's (in its
code) with the original's offsets, so AHIOutputRequest writes the same
longs. The five entries of AHIOutputRequest with fewer arguments take
the registers the original's entries keep (d2–d5) as parameters. The
code behind the `ahidevice` check (PlaySoundAHI, PlayRawAHI, CloseDevice
in finit) is translated although it is never reached.

## BUG(orig), kept

* `ahidevice` is never set (only tested and cleared): PlaySoundAHI and
  PlayRawAHI always return at once and play nothing, finit/CloseAHI
  never call CloseDevice, and OpenAHI's "already open" check never
  holds – a second OpenAHI opens the open request again
  (#924).
* AHIOutputRequest: the `!subs` of the forms with 1, 2 and 4 arguments
  point to the wrong entries – 4 arguments go to the "AudioID" entry
  (x, y, width, height replaced by -1, the mode id is d4 as it was), 1
  argument to "AudioID, Frequency" (the argument replaced, d4/d5 as they
  were), 2 to "X,Y,WIDTH,HEIGHT" (d2/d3 as they were)
  (#928). The values go to the offsets 4,
  12, 16, 20, 24, 28 of the tag list: 16 and 24 are the tag ids of
  `AHIR_InitialWidth` and `AHIR_InitialHeight` – width and the mode id
  become tag ids (mode id 0, passed by the 0- and 2-argument forms - the
  1- and 4-argument forms leave d4 as it is -, is `TAG_DONE` and
  ends the list), height and frequency their data; the tags
  `AHIR_InitialAudioID`/`AHIR_InitialMixFreq` keep -1 and 0
  (#931). The current screen of
  `!libs {#screenslib,#ua0}` is not used (`AHIR_Screen` stays 0)
  (#929). Every call allocates a
  requester; none is ever freed (#930).
* AHIRequestedID and AHIRequestedFreq return the data of the
  requester's initial tags (always -1 and 0), not what the user chose
  (`ahiam_AudioID`, `ahiam_MixFreq` of the requester)
  (#934).
* Never reached (behind the `ahidevice` check): PlaySoundAHI gives the
  length in words (+6) as `io_Length` in bytes
  (#932), and sends the one request
  with SendIO again without waiting for the one before or taking its
  reply from the port; finit leaves everything open while the request is
  in use (#933).

## Fixed

Intended deviations from the original (`docs/libs/deviations.md`,
kind F):

* CludgeSound does not free the old sample when audiolib's `"cL"` mark
  (+14) is set (DecodeSound's in-place data of chip memory, which memlib
  never allocated); the original passed it to memlib globfree ("Unable to
  Free Memory") (#945; test
  `tests/bug-orig/rwe/risoundlib2-cludgesound-frees-decoded`).
* CludgeSound clears +14, audiolib's `"cL"` mark of DecodeSound data,
  as risoundlib does; the original replaced that `MOVE.w #0,14(a0)` by
  the write of the rate at +16 (#926), so after a DecodeSound of chip
  data the mark stayed on CludgeSound's copy and audiolib never freed it
  (#925; test `tests/bug-orig/rwe/risoundlib2-cludgesound-cl-mark-kept`,
  `libs_emulator` cases `[fixed #925]` of `rwe_risoundlib2`).
* CludgeSound's fixes of risoundlib (`port/risoundlib.md`, "Fixed"), in
  the same routine: the rate is `samplesPerSec` (#832); a rate of 0
  gives 0 (#830; test `tests/bug-orig/rwe/risoundlib2-rate-zero-hangs`);
  the chunk walk skips the IFF pad bytes and stays inside the FORM
  (#828); the old sample is freed after the checks and its pointer
  cleared, a failed call leaves the sound as it was (#831); the whole
  BODY is copied, an empty one or one of 131072 bytes or more gives 0
  (#829).
* CludgeSound writes nothing behind audiolib's 16 byte sound object. The
  original stored the sample rate at +16 for PlaySoundAHI: the high word
  of the next object's sample pointer, behind the last object whatever
  follows. PlaySoundAHI takes its frequency from the object's period
  instead, 3579545 / period (two `DIVU.w`; 0 for a period of 0), which
  works for every sound object and gives the pitch `Sound` plays (22050
  Hz becomes 22096 Hz through the period's rounding) (#926; test
  `tests/bug-orig/rwe/risoundlib2-cludgesound-rate-behind-object`,
  `libs_emulator` cases `[fixed #926]` of `rwe_risoundlib2`).
* CloseAHI closes the device and keeps the request and the port, which
  finit deletes; OpenAHI without a request (init could not make one)
  returns 0, and finit makes no CheckIO without one. The original's
  CloseAHI was finit: a later OpenAHI wrote `ahir_Version` to address 48
  and called OpenDevice with request 0, the same without a request from
  init (#927; test `tests/bug-orig/rwe/risoundlib2-openahi-after-closeahi`).

## Test

Each run loads one library set (the C build or the original); the
scenario writes its observations as cases, compared with the expected
data `libs/tests/expected/rwe_risoundlib2.txt` recorded from the
original.

The test machine's Workbench has no ahi.device; the test puts its own
"ahi.device" into the device list with priority 10, before one that a
Workbench may have in `DEVS:` (whose requester would wait for the user).
It logs Open/Close/AbortIO and every BeginIO with the request's fields
(command, data, length, type, frequency, volume, position, `ahir_Link`,
priority), completes a request at once (quick, or replied to the port)
or keeps it while the scenario lets a sound "play on" until AbortIO, and
marks `ahir_Link` and the priority of a completed request (AHI's
autodoc: "the other fields are trashed"). It copies the tag list of
`AHI_AllocAudioRequestA`, checks the requester and tags of
`AHI_AudioRequestA` (a confirmed requester gets a choice in
`ahiam_AudioID`/`ahiam_MixFreq`), counts `AHI_FreeAudioRequest` and the
other vectors, and can fail OpenDevice, the allocation and the
requester. AHIOutputRequest gets a used screen object in a0 (or 0).
Observed: d0, what the device saw, the replies in the library's port,
the kept registers. Covered: every command with OpenDevice failing;
OpenAHI (both forms, again while open), PlaySoundAHI/PlayRawAHI in all
forms, sounds that play on and ones that end at once, CloseAHI and finit
while one plays, AHIOutputRequest in all six forms with markers in d2–d5
(the tag list also checked for its values: the bugs above), with and
without a used screen, the allocation failing, the requester cancelled,
AHIRequestedID/Freq, CloseAHI (open, in use – CheckIO 0 –, again),
finit, init again, init without a message port; CludgeSound (+16
written by the original only, #926; +14 cleared by the port only, #925),
then risoundlib's CludgeSound cases (`libs/tests/rwe/risoundcludge.h`,
the same routine). Not covered: the real ahi.device; OpenAHI after
CloseAHI or without a request (the original writes to address 48: the
Blitz test `risoundlib2-openahi-after-closeahi`); finit without a
request (CheckIO of address 0).
