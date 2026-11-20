# ritrackdisklib (TrackDisk, library 91)

**What it does:** sector access to floppy units 0-3 through
trackdisk.device (or another device: `OpenDisk unit#,devicename$`;
`OpenDisk unit#` always opens trackdisk.device):
`OpenDisk`/`CloseDisk`, `ReadSector`/`WriteSector unit,sector,buffer
[,numsectors]`, `FormatTrack`, `WriteBoot` (a bootable standard boot
block, or 1024 bytes of the program's), `BootProg` (checksum + write sectors 0-1), `MotorOn`/
`MotorOff`, `DiskStatus` (CheckIO), `DiskIO` (the IOStdReq), `DiskMotor`
(the motor state as the library knows it). One IOStdReq per unit (200 bytes), one reply port made by
init and freed by finit; both are built by hand with exec V33 functions,
so the library runs on every Kickstart (fixed #941). `DiskStatus`,
`DiskIO`, `BootProg`, `DiskMotor` are Amiga mode only (odd code address
in `!subs`).

**Results** (as the original's; the manuals write `success=` without
values, #1155): `OpenDisk` and `CloseDisk` return -1 when they opened or
closed the unit, 0 otherwise. `ReadSector`, `WriteSector`,
`FormatTrack`, `WriteBoot`, `BootProg`, `MotorOn` and `MotorOff` return
DoIO's d0 with the request's `io_Error` as its low byte: the byte is
**0 when the command worked** and the trackdisk.device error otherwise
(28 write protected, 29 no disk in the drive, -4 a length the device
refuses, ...); they return 0 too when the unit is not open or there is
no reply port, so 0 does not tell "done" from "not open", and
`If ReadSector(...)` is true on an error. `WriteBoot unit` returns
`TDERR_NoMem` (31) without chip memory for its buffer. `DiskStatus`
returns CheckIO's result (the request when it is done, else 0),
`DiskIO` the unit's request (0 if it is not open), `DiskMotor` -1 or 0
(#857).

**Source and reference:** `rwelibsrc/trackdisklib/trackdisk2.bb`
(`trackdisk.bb` has the same code), reference BSS2 `RITRACKDISKLIB.OBJ`
(1892 bytes) = the blitz2 2.10 build of the source. The C build has the
same declarations (`rwe_declarations_ritrackdisklib`); the code is not
byte-identical. Library number `LIB_ritrackdisklib` (91) is in the
bb2-2.10 libnums.

**C:** everything (`libs/userlibs/rwe/ritrackdisklib.c`): exec through
`proto/exec.h`, except `DoIO`, which goes through `BLITZ_BASEJSR`
because the original returns all of DoIO's d0 (the NDK inline returns a
`BYTE`). The routines
that branch into each other in the original (`_install1` →
`_formatsector2` → `okok`, `_bootprog` → `_writesector2`) are C calls.
Every entry is a thunk keeping d1/a0/a1 (the
original keeps them on the paths without port or request); the
original's changes of a6 and d4-d7 are not reproduced.

No bug of the original is kept; the fixed ones:

* #941: the original's init called `CreateMsgPort`, the commands
  `CreateIORequest`/`DeleteIORequest`, finit `DeleteMsgPort` – exec V36
  functions – without checking exec's version: on Kickstart 1.x every
  program that used the library crashed at start-up. The port
  (`newport`: `AllocMem` of a cleared `MsgPort`, `AllocSignal`,
  `FindTask(0)`, `PA_SIGNAL`, an empty message list; `freeport`) and the
  requests (`newreq`: `AllocMem` of 200 cleared bytes, `NT_REPLYMSG`,
  `mn_ReplyPort`, `mn_Length`; `freereq`) are built by hand, the same
  path on every Kickstart (W7 Kickstart decision, question 1 (a)).
  Tests: bug-orig `rwe/ritrackdisklib-v36-calls` (a500), the regression
  test `ritrackdisklib-floppy-key` reads a disk in DF1 on the a500,
  a500-204 and a1200 with these requests.
* #856: the original's `BootProg` stored as checksum the plain sum with
  carry of the 256 longs, including the old checksum long (not cleared,
  not complemented), and cleared the root block long (+8) after the sum:
  Kickstart did not boot the block. The root block long is cleared
  first, the checksum long cleared, the longs added with end-around
  carry and the complement stored (the block sums to `$FFFFFFFF`). Test:
  the `libs_emulator` cases `[fixed #856]` (BootProg twice: the block,
  its checksum and sum, the data the device was given).
* #857: the original's `DiskMotor` (help text "Returns -1 if motor is
  on"; no unit argument, in no manual) returned `SNE` of bit 7 of CIAB
  PRB (`$BFD100`, /MTR). trackdisk.device leaves that bit 1 after every
  request, with the motor on and off (measured on Kickstart 1.3 and 3.1:
  it deselects the drives): the original returned -1 always, the
  inversion the issue proposed would return 0 always, and the motor
  state cannot be read cleanly from the hardware. Decided (R10, the
  answer to the DiskMotor question: option (a)): **the result is the
  library's own knowledge**, not what other programs do with the
  drives: a unit's motor is on after a `MotorOn` or a `ReadSector`,
  `WriteSector`, `FormatTrack`, `WriteBoot` or `BootProg` that worked
  (trackdisk.device switches the motor on for them and leaves it on),
  off after a `MotorOff` that worked and after `CloseDisk`; init clears
  the states. `DiskMotor` returns -1 if the motor of a unit the library
  has open is on, else 0. Test: the `libs_emulator` case `[fixed #857]`
  (DiskMotor after MotorOff on the empty DF0: 0); the disk tests after
  MotorOn and MotorOff.
* #858: `FormatTrack unit,track,buffer[,num]` and `WriteBoot` shared
  the sector commands' length, num × 512 bytes, with `TD_FORMAT`.
  trackdisk.device formats whole tracks only (5632 bytes = 11 sectors
  of a DD disk) and refuses every other length with -4
  (`IOERR_BADLENGTH`; measured on Kickstart 1.3 and 3.1): `FormatTrack`
  without a count and both forms of `WriteBoot` never wrote anything,
  `FormatTrack …,n` worked for multiples of 11 only, and then formatted
  n/11 tracks. Decided (R10, the user's answer to the count question:
  option (b)): **the count of `FormatTrack` stays in sectors** (only
  multiples of 11 work: 11 formats one track, 22 two; a program that
  passes 11 for one track keeps working, the declaration's name
  `numtracks` notwithstanding), the form without a count formats one
  whole track (11 sectors: the buffer must hold 5632 bytes), and
  `WriteBoot` writes the boot block, sectors 0-1, with `CMD_WRITE`
  (the manual: "writes 1k of data to the bootblock"; a `TD_FORMAT` of
  track 0 would wipe sectors 2-10). Test: the `libs_emulator` cases
  `[fixed #858]` (length and command of FormatTrack and WriteBoot).
* #864: the original's `WriteBoot unit,bootdata` loaded the sector 0
  into d1 over its argument bootdata and wrote from d2, which is no
  argument (whatever the caller left there). It writes bootdata (d1)
  now. Test: the `libs_emulator` cases `[fixed #864]` (buffer, head and
  data of `WriteBoot 0,bootdata`).
* #859, #860: the original's `OpenDisk unit,name$` copied the name into
  a buffer of 58 bytes (`"trackdisk.device"` and room behind it) without
  a limit, so a name of 57 characters or more overwrote the request
  table and the port (#860), and kept it there, so a later `OpenDisk
  unit` without a name opened that device again (#859; the manual
  documents `OpenDisk(unit#)` as trackdisk.device). The name goes to
  `OpenDevice` as it is (a Blitz string ends with its 0), `OpenDisk
  unit` always opens trackdisk.device; the buffer is gone. A program
  that relied on the kept name opens trackdisk.device now. Tests:
  bug-orig `rwe/ritrackdisklib-opendisk-name-overflow` (#860),
  `rwe/ritrackdisklib-opendisk-kept-name` (#859), the `libs_emulator`
  case `[fixed #859]` (OpenDisk 2 without a name after names were
  given: trackdisk.device has no unit 2 on the test machine).
* #861: the original's `OpenDisk` checked the unit with a signed
  compare (`CMP #3: BGT`): a negative unit passed, the table routines
  ignored it, and the request was lost (OpenDevice ok, the device stayed
  open) or `DeleteIORequest(0)` was called (OpenDevice failed). A unit
  outside 0-3 is refused before the request is made (an unsigned
  compare); OpenDisk returns 0. Test: the `libs_emulator` cases
  `[fixed #861]` (units -1 and -2).
* #862: the original's `OpenDisk` of a unit that was open made a new
  request and opened the device again: the old request stayed open and
  was lost (CloseDisk and finit closed only the new one). An open unit
  is left alone now and `OpenDisk` returns 0 (as seriallib's
  `OpenSerial`, #260). Tests: bug-orig `rwe/ritrackdisklib-opendisk-open-unit`
  (`OpenDisk 0` twice on DF0: -1, 0), the `libs_emulator` cases
  `[fixed #862]` (a second OpenDisk of unit 1).
* #863: the original's finit closed units 0-2 only (unit 3 stayed open,
  its request leaked, and the port it names was deleted), and its init
  cleared only three table entries (a second init kept unit 3's stale
  request). init clears and finit closes all four units; with finit
  fixed the init half shows only if finit did not run. Test: the
  `libs_emulator` cases `[fixed #863]` (finit with unit 3 open, init
  again).
* #865: the original's `WriteBoot unit` wrote its 8-byte default block
  (`"DOS",0` and the long -1) and the 1016 bytes behind it in memory
  (the end of the library and whatever followed): no valid checksum, a
  root block pointer and boot code from unrelated bytes. Decided (R10,
  the answer to the default boot block question: option (b), a bootable
  standard boot block): `WriteBoot unit` reads sectors 0-1 of the disk
  first and keeps the flag byte (byte 3, the file system: 0 OFS, 1 FFS,
  …) of a boot block that starts with `"DOS"`, 0 for any other block or
  when the read fails; it writes `"DOS"` with that byte, the checksum
  (as BootProg's, #856), root block 880 and boot code of its own at
  offset 12: `FindResident("dos.library")`, the resident's `rt_Init` in
  a0 with d0 = 0, or d0 = -1 when there is none (38 bytes, the bytes and
  their instructions in `bootcode` of the C file). The buffer is 1024
  bytes of chip memory, freed again; without it the result is
  `TDERR_NoMem` (31). Test: the `libs_emulator` cases `[fixed #865]`
  (the block on the disk: head, sum `$FFFFFFFF`, boot code; on a disk
  with `"DOS",3` the flag 3 kept); the disk tests boot it.
* #1154: the original never sent `CMD_UPDATE`. trackdisk.device keeps
  the sectors written with `CMD_WRITE` in its track buffer and writes
  them to the disk only when it changes the track (measured: a sector
  written, the motor off, the unit closed, and the sector is not on the
  image); so the last track a program wrote - for `BootProg` and
  `WriteBoot` the boot block itself - was lost when the disk was taken
  out, unless another opener moved the head first. `MotorOff` sends
  `CMD_UPDATE` before `TD_MOTOR` 0, and `CloseDisk` and finit switch the
  motor off through it; not after every write (a program writing sector
  by sector would rewrite the track eleven times). A program that keeps
  the unit open and the motor on until the disk is changed still loses
  the track. Test: the `libs_emulator` cases `[fixed #1154]` (the test
  device's count of `CMD_UPDATE` at MotorOff, CloseDisk and finit); the
  disk tests read the written sectors from the image.

**Test** (`libs/tests/rwe/ritrackdisklib.c`, snitch case "rwe
ritrackdisklib"): each run loads one library set (the C build or the CD
original); the scenario writes its observations as cases, compared with
the expected data `libs/tests/expected/rwe_ritrackdisklib.txt` recorded
from the original. The library talks to a device of the test,
`rwetest.device` (MakeLibrary + AddDevice): a RAM disk of 32 sectors
that logs every request (command, flags, length, offset, buffer, unit,
checksum of the data written) and refuses units 2 and -2. Observed are
d0, the request the device saw, the disk image and
the buffers, BootProg's checksum, the open/close counts and the kept
registers. Cases: every command before OpenDisk, OpenDisk of units 5, 4
(with name), 0-3, -1 and -2, the kept name, a second OpenDisk of an open
unit, sector commands with and without count (offset overflow of
`sector << 9`), device errors (positive and negative io_Error),
FormatTrack with the track's upper word set, both WriteBoot forms (the
default block observed by its 8 bytes), BootProg twice, DiskStatus,
DiskIO, DiskMotor, CloseDisk of open/closed/invalid units, finit with
unit 3 open and init again (unit 3's entry). Then the real
trackdisk.device on the empty DF0: of the emulator: OpenDisk, MotorOn,
DiskMotor, MotorOff, DiskMotor, ReadSector (29 = no disk), DiskStatus,
CloseDisk. Not covered: trackdisk.device with a disk (no floppy image;
the hardfiles are read-only), a device name over 56 characters.
