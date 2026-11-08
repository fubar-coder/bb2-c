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
requester calls (`AHI_AllocAudioRequestA`, `AHI_AudioRequestA`); finit
(and CloseAHI) deletes request and port. CludgeSound is risoundlib's,
with the sample rate also stored at +16 of the sound object (for
PlaySoundAHI's frequency).

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
* CloseAHI is finit: it deletes the request and the port. A later OpenAHI
  writes `ahir_Version` to address 48 and calls OpenDevice with request 0;
  the same if init could not make its request
  (#927).
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
* CludgeSound: risoundlib's bugs (#831, #832, #830, #828, #829); the rate
  goes to +16, behind audiolib's 16 byte sound object – into the next
  object's sample pointer, behind the last into whatever follows the
  objects (#926); +14, audiolib's
  "cL" mark of DecodeSound data, is no longer cleared
  (#925); the old sample is freed without looking at that mark, so
  DecodeSound's in-place data of chip memory goes to memlib globfree
  ("Unable to Free Memory") (#945; test
  `tests/bug-orig/rwe/risoundlib2-cludgesound-frees-decoded`).
* Never reached (behind the `ahidevice` check): PlaySoundAHI gives the
  length in words (+6) as `io_Length` in bytes
  (#932), and sends the one request
  with SendIO again without waiting for the one before or taking its
  reply from the port; finit leaves everything open while the request is
  in use (#933).

## Test

The emulator's Workbench has an ahi.device in `DEVS:` (its requester
would wait for the user), so the test puts its own "ahi.device" into the
device list with priority 10, before it: it logs Open/Close/BeginIO/
AbortIO, copies the tag list of `AHI_AllocAudioRequestA`, checks the
requester and tags of `AHI_AudioRequestA`, counts the other vectors, and
can fail OpenDevice, the allocation and the requester. Compared with the
reference build: d0, what the device saw, the registers. Covered: every
command with OpenDevice failing; OpenAHI (both forms, again while open),
PlaySoundAHI/PlayRawAHI in all forms (nothing reaches the device),
AHIOutputRequest in all six forms with markers in d2–d5 (the tag list
also checked for its values: the bugs above), the allocation failing,
the requester cancelled, AHIRequestedID/Freq (-1, 0), CloseAHI (open, in
use – CheckIO 0: nothing freed –, again), finit, init again, init
without a message port; CludgeSound (+16 written, +14 kept). Not
covered: the real ahi.device and playing (unreachable); OpenAHI after
CloseAHI or without a request (writes to address 48); finit without a
request (CheckIO of address 0); risoundlib's CludgeSound cases (the same
code, `libs/tests/rwe/risoundlib.c`).
