# ritrackdisklib (TrackDisk, library 91)

**What it does:** sector access to floppy units 0-3 through
trackdisk.device (or another device: `OpenDisk unit#,devicename$`):
`OpenDisk`/`CloseDisk`, `ReadSector`/`WriteSector unit,sector,buffer
[,numsectors]`, `FormatTrack`, `WriteBoot` (a default boot block
`"DOS",0,-1`), `BootProg` (checksum + write sectors 0-1), `MotorOn`/
`MotorOff`, `DiskStatus` (CheckIO), `DiskIO` (the IOStdReq), `DiskMotor`
(CIAB bit). One IOStdReq per unit (CreateIORequest, 200 bytes), one
reply port created by init and deleted by finit. Results: DoIO's d0 with
io_Error as low byte, 0 without port/request; `DiskStatus`, `DiskIO`,
`BootProg`, `DiskMotor` are Amiga mode only (odd code address in
`!subs`).

**Source and reference:** `rwelibsrc/trackdisklib/trackdisk2.bb`
(`trackdisk.bb` has the same code), reference BSS2 `RITRACKDISKLIB.OBJ`
(1892 bytes) = the blitz2 2.10 build of the source. The C build has the
same declarations (`rwe_declarations_ritrackdisklib`); the code is not
byte-identical. Library number `LIB_ritrackdisklib` (91) is in the
bb2-2.10 libnums.

**C:** everything (`libs/userlibs/rwe/ritrackdisklib.c`): exec through
`proto/exec.h`, except `DoIO`, which goes through `BLITZ_BASEJSR`
because the original returns all of DoIO's d0 (the NDK inline returns a
`BYTE`); `DiskMotor` reads CIAB PRB with a volatile access. The routines
that branch into each other in the original (`_install1` →
`_formatsector2` → `okok`, `_bootprog` → `_writesector2`) are C calls.
The data keep the original's order and sizes (58 byte name buffer,
request table, port, default boot block), so that the overflows below
hit the same things. Every entry is a thunk keeping d1/a0/a1 (the
original keeps them on the paths without port or request); the
original's changes of a6 and d4-d7 are not reproduced.

**BUG(orig)** (kept):

* `OpenDisk unit,name$` copies the name into the 58-byte buffer
  `trackname` without a limit (a longer one overwrites the request table
  and the port) and keeps it for later `OpenDisk` without a name.
* `OpenDisk` of an open unit replaces its request (the old one stays
  open); a negative unit passes the check (`CMP #3: BGT`), but the table
  routines ignore it: the request is lost, or `DeleteIORequest(0)` if
  OpenDevice failed.
* finit closes units 0-2, not 3; init clears only three table entries.
* `WriteBoot unit,bootdata` overwrites bootdata (d1 = sector 0) and
  takes the buffer from d2, which is no argument (whatever the caller
  left there; the C routine takes d2 as a register parameter for the same
  effect). `WriteBoot unit` writes the 8-byte default boot block and the
  1016 bytes behind it (the end of the library and beyond).
* `FormatTrack`/`WriteBoot` (TD_FORMAT): length = number × 512 as for
  sectors, not tracks × 11 × 512.
* `BootProg`: the checksum is the plain sum with carry of all 256 longs
  including the old checksum (not cleared, not complemented); the root
  block long (+8) is cleared afterwards.
* `DiskMotor` returns -1 (the byte $FF) when CIAB PRB bit 7 (/MTR,
  active low) is set, i.e. motor off; the help text says "-1 if motor is
  on".

**Test** (`libs/tests/rwe/ritrackdisklib.c`, snitch case "rwe
ritrackdisklib"): C build and CD original side by side against a device
of the test, `rwetest.device` (MakeLibrary + AddDevice): a RAM disk of
32 sectors per set that logs every request (command, flags, length,
offset, buffer, unit, checksum of the data written) and refuses units 2
and -2. Compared are d0, the request the device saw, the disk image and
the buffers, BootProg's checksum, the open/close counts and the kept
registers. Cases: every command before OpenDisk, OpenDisk of units 5, 4
(with name), 0-3, -1 and -2, the kept name, a second OpenDisk of an open
unit, sector commands with and without count (offset overflow of
`sector << 9`), device errors (positive and negative io_Error),
FormatTrack with the track's upper word set, both WriteBoot forms (the
default block compared by its 8 bytes), BootProg twice, DiskStatus,
DiskIO, DiskMotor, CloseDisk of open/closed/invalid units, finit (unit 3
stays open) and init again (unit 3's stale entry). Then the real
trackdisk.device on the empty DF0: of the emulator: OpenDisk, MotorOn,
DiskMotor, MotorOff, DiskMotor, ReadSector (29 = no disk), DiskStatus,
CloseDisk. Not covered: trackdisk.device with a disk (no floppy image;
the hardfiles are read-only), a device name over 56 characters.
