# ricompactdisclib (CompactDisk, library 14)

**What it does:** audio CDs through cd.device or a compatible device:
`OpenCD [devicename$,unit[,scsiflag]]`/`CloseCD`, `CDPlayTrack
track,numtracks` (SendIO), `CDStop` (AbortIO), `CDPause`, `CDRewind`/
`CDFastForward`/`CDNormalSpeed`/`CDSpeed` (CD_SEARCH), `CDVolume`
(CD_ATTENUATE), `CDDoor` (CD_EJECT), `CDMotor`, `CDFlush`, `CDStatus`
(CD_INFO's Status), `CDReadTOC` (CD_TOCMSF into a buffer of memlib's
globalloc), `CDNumTracks`/`CDFirstTrack`/`CDLastTrack`/`CDTrackLength`
from that TOC, `CDUpdateInfo` (CD_QCODEMSF) and `CDTrackMins`/
`CDTrackSecs`/`CDTrackPlaying` from its Q-code. One IOStdReq
(CreateIORequest, 200 bytes) on a port of init; the commands return 0
without an open device, else DoIO's d0. With `scsiflag` <> 0
`CDPlayTrack`, `CDReadTOC` and `CDNumTracks` use HD_SCSICMD (PLAY
AUDIO(12), READ TOC) instead. All entries but `OpenCD`, `CloseCD`,
`CDTrackMins`, `CDTrackSecs` and `CDTrackPlaying` are Amiga mode only.

**Source and reference:** `rwelibsrc/CDlib/cdSCSI.bb` (1997-01-05),
the newest of the four CD sources and newer than every binary (README
3.2: the BSS2/BUM/AB3 binary V1.4 is cd3 without `scsiflag`/`CDMotor`,
cdSCSI is cd3 plus the SCSI path). The reference is the reference build
of that source by blitz2 2.10 (`ref/ricompactdisclib.obj`, ctest
`rwe_refbuild`); the C build has its declarations
(`rwe_declarations_ricompactdisclib`), the code is not byte-identical.
`#debug=0`: no `CDTOCPos`. Library number `LIB_ricompactdisclib` (14).

**C:** everything (`libs/userlibs/rwe/ricompactdisclib.c`). exec through
`proto/exec.h`, but through `BLITZ_BASEJSR` where the original passes
the whole d0 of the call on: DoIO, SendIO, AbortIO, OpenDevice (`TST.l
d0`) and DeleteIORequest (OpenCD's failure result); memlib's
globalloc/globfree through `BLITZ_ALIBJSR`. `CloseCD`'s `ALibJsr
CDStop` is a C call. The data from `trackname` to `MyCDInfo` keep the
original's order and sizes (one packed struct), so that a long device
name and the unlimited `CDTrackMins`/`CDTrackSecs` offsets hit the same
bytes. `CDPlayTrack`'s SCSI path writes its two longs to odd addresses
with volatile long accesses, like the original. Every entry is a thunk
keeping d1/a0/a1; the original's changes of a6, d7 (CDReadTOC) and
d1/d2 (CDTrackLength) are not reproduced.

**BUG(orig)** (kept):

* `OpenCD name$,unit` copies the name into the 40-byte `username`
  without a limit (a longer one overwrites toc, toclen, temptoc,
  qcodeinfo, ioreq ...) and keeps name, unit and scsiflag for later
  `OpenCD`s; only init clears scsiflag (and sets the name back).
* `OpenCD` while open: the old request stays open, lost. If OpenDevice
  fails, the request is deleted but stays in ioreq, and `OpenCD` returns
  d0 of DeleteIORequest (200 on Kickstart 3.2), not 0.
* `CDStop` (also in `CloseCD`) is AbortIO only, no WaitIO; it sets
  io_Command first.
* `CDPlayTrack` with scsiflag ignores its arguments: blocks 14000 to
  15000 (test values), io_Offset = 1, and the longs are written to odd
  addresses - an address error on a 68000.
* `CDReadTOC` with scsiflag fills only its own buffer: `CDFirstTrack`,
  `CDLastTrack`, `CDTrackLength` still use toc (address 0 before any
  `CDReadTOC` without scsiflag).
* `CDReadTOC`: the summary read is not checked, nor the globalloc of the
  TOC; toclen = (last + 2) × 8 + 32 for 6-byte entries.
* `CDTrackLength n` = start of track n+1 − start of track n in seconds
  (minutes × 60 + seconds), for track 1 the start of track 2. From track
  44 the offset (n−1) × 6 exceeds 255 and its high byte stays in the
  word that `MULU.w #60` multiplies (`MOVE.b (a0),d0` replaces the low
  byte only): the start is wrong. The last track takes the entry behind
  it, which CD_TOCMSF does not write; track 0 reads 65539 bytes behind
  toc.
* `CDUpdateInfo` without an open device returns the caller's a0 + 5.
* `CDTrackMins`/`CDTrackSecs offset`: `ASL.b #2` shifts the low byte
  only, the index is the whole word, the high bytes of d0 stay in the
  result; no limit. They and `CDTrackPlaying` do not check for an open
  device.

**Test** (`libs/tests/rwe/ricompactdisclib.c`, snitch case "rwe
ricompactdisclib"): C build and reference build side by side, each with
its own memlib, against a device of the test, `rwecd.device`
(MakeLibrary + AddDevice) - there is no CD drive in the emulator. It
plays a CD of 9, 50 or 20 tracks (CD_INFO with a Status word, CD_TOCMSF
summary and entries, a moving CD_QCODEMSF position, HD_SCSICMD READ TOC
header), logs every request (command, flags, length, offset, where the
data pointer points, the SCSICmd and its command bytes), opens, closes
and AbortIOs, refuses unit 3 and sets an io_Error on request. Compared
are d0, the requests, the Q-code bytes behind `CDUpdateInfo`'s address
and the kept registers; at the end all memory must be back. Cases:
every command without a device; OpenCD of cd.device (not in the
emulator: the failure path), of unit 3, with the name kept, while open
and in all three forms; all CD commands with on/off values 0-3 and -1;
io_Errors of CD_EJECT, CD_INFO, CD_TOCMSF; CDReadTOC repeated (old TOC
freed); CDTrackLength 1-9 and 40-50 (the track 44 bug); offsets 0, 1,
4, 5, −1..−5 and set high bits for CDTrackMins/Secs; the SCSI path
(PLAY AUDIO(12) bytes, READ TOC, CDNumTracks from the SCSI data, the
flag kept); CloseCD twice; finit with a device open and again; init
again; init without a port (CreateMsgPort fails) and OpenCD without a
request (CreateIORequest fails). Not covered: a real drive, names over
40 bytes, CDTrackLength 0 or behind the last track (unwritten memory),
the TOC functions before any CDReadTOC (address 0), the SCSI
`CDPlayTrack` on a 68000 (the emulator has a 68020), globalloc failing.
