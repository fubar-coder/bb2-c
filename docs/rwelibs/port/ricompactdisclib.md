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
(200 bytes, built by hand) on a port of init; the commands return 0
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
d0`); memlib's
globalloc/globfree through `BLITZ_ALIBJSR`. `CloseCD`'s `ALibJsr
CDStop` is a C call. The data from `trackname` to `MyCDInfo` keep the
original's order and sizes (one packed struct), so that a long device
name and the unlimited `CDTrackMins`/`CDTrackSecs` offsets hit the same
bytes; `MyCDInfo` is a whole struct CDInfo (34 bytes, #948). Every entry is a thunk keeping d1/a0/a1; the original's changes of a6, d7 (CDReadTOC) and
d1/d2 (CDTrackLength) are not reproduced.

**Fixed:**

* `OpenCD name$,unit[,scsiflag]` refuses a name of 40 characters or more
  (the manual: "less than forty characters"): 0, nothing copied, nothing
  opened. The original copies it into the 40-byte `username` without a
  limit (over toc, toclen, temptoc, qcodeinfo, ioreq ...). (#576)
* `CDPlayTrack` with scsiflag writes its command block byte by byte; the
  original writes two longs and a word to odd addresses, an address
  error on a 68000. (#579)
* `CDNumTracks`, `CDFirstTrack`, `CDLastTrack` and `CDTrackLength`
  return 0 without a TOC; with scsiflag they use the SCSI TOC of
  `CDReadTOC` (first and last track from its header, the length of a
  track from the LBAs of its descriptor and the next one or the
  lead-out, frames / 75; 0 for a track it does not hold). The original
  reads toc without a test (address 0 before any `CDReadTOC`) and with
  scsiflag counts only `CDNumTracks` from the SCSI TOC. (#582)
* `CDReadTOC` returns 0 and leaves no TOC when the summary read, the
  globalloc of the TOC or the read of the entries fails (the old TOC is
  freed first). The original checks neither the summary read (it reads
  the entries with the last track of an earlier summary and returns -1)
  nor the globalloc, and keeps the uncleared buffer of a failed read.
  toclen = (last + 2) × 8 + 32 for 6-byte entries, as in the
  original. (#581)
* The reply port and the request are built by hand with exec V33
  functions (AllocMem, AllocSignal, FindTask; FreeMem, FreeSignal), one
  path for all Kickstarts, in the shape of ritrackdisklib's (#941): the
  port cleared, a signal bit, `PA_SIGNAL`, the message list set up by
  hand; the request 200 cleared bytes with `NT_REPLYMSG`, the reply port
  and its length. The original calls CreateMsgPort (in init),
  CreateIORequest, DeleteIORequest and DeleteMsgPort (exec V36) without a
  version check: on Kickstart 1.x every program with the library crashes
  before its first line. (#1138)
* `OpenCD` returns 0 when OpenDevice fails, and the freed request is
  forgotten. The original keeps the deleted request in ioreq and returns
  the d0 of DeleteIORequest (200 on Kickstart 3.1). (#575)
* `OpenCD name$,unit` opens without scsiflag; `OpenCD` without
  arguments opens cd.device unit 0 without scsiflag (the manual:
  "ommiting the optional parameter of this command will cause the
  default cd.device unit 0 to be opened"). The original keeps the name,
  the unit and scsiflag of the `OpenCD` before (only init clears scsiflag
  and sets the name back, not the unit, so a second run from the editor
  opens cd.device with the unit of the first). (#577)
* `CDTrackLength 1` is the start of track 2 minus the start of track 1,
  as for every other track; the original returns the start of track 2
  (the 2 seconds of the pregap counted). (#584)
* `CDTrackLength` takes the minutes of the start alone. From track 44 the
  original's entry offset (n−1) × 6 exceeds 255 and its high byte stays
  in the word that `MULU.w #60` multiplies (`MOVE.b (a0),d0` replaces the
  low byte only): the start was wrong. (#585)
* `CDTrackLength n` (start of track n+1 − start of track n in seconds,
  minutes × 60 + seconds) of the last track ends at the lead-out of the
  summary; the original takes the entry behind it, which CD_TOCMSF does
  not write. With runtime error checking a track outside the TOC's first
  to last track is refused ("Track number out of range"; the manual: "The
  track# should be checked to make sure that it exists on the compact
  disc"); the release code is unchanged there (track 0 reads 65539 bytes
  behind toc, D-005). The library gets an error check area for it
  (`ricompactdisclib.declarations-intended.txt`). (#583)
* With runtime error checking `CDTrackMins offset` and `CDTrackSecs
  offset` refuse an offset other than 0 or 1 ("Offset must be 0 or 1";
  the manual: "The optional parameter offset can take the value of 0 or
  1"). The release code is the original's (D-005): `ASL.b #2` shifts the
  low byte only, the index is the whole word, the high bytes of d0 stay
  in the result, no limit. They and `CDTrackPlaying` read the Q-code
  without an open device (the data of the last `CDUpdateInfo`). (#586)
* With runtime error checking `OpenCD` (all three forms) refuses to open
  while a device is open ("CD device already open: CloseCD first"; the
  manual: "you must call CloseCD between calls to OpenCD"). The release
  code is the original's (D-005): the open request is lost and stays
  open. (#578)
* `CDUpdateInfo` returns 0 without an open device; the original returns
  the caller's a0 + 5. (#587)
* `CDVolume volume,fade` sends CD_ATTENUATE with the volume in
  `io_Offset` and the fade time in `io_Length` (frames), as cd.device
  wants them. The original swaps them, so `CDVolume 32767,0` asked for
  silence with a fade of 32767 frames. (#982)
* `CDStatus` asks for a whole struct CDInfo (34 bytes) into a `MyCDInfo`
  of that size. The original asks for 26 bytes into 13 words, and a
  device that writes the whole struct (uaescsi.device) writes 8 bytes
  behind `MyCDInfo`, the end of the library: on Workbench 3.1 the
  unloading of the program stops with the alert 8100 0005. (The port had
  8 bytes of room there since #946, which kept the pointers of its SCSI
  buffers from being overwritten; it is the end of the struct now.)
  (#948)
* `CDPlayTrack track#,numtracks` with scsiflag sends PLAY AUDIO(12)
  (`$A5`, as the author began it) from the SCSI TOC of `CDReadTOC`: the
  LBA of track# and the length to the start of the track behind the last
  one played (the lead-out at the most). It returns -1 when the command
  worked, else 0: also without a `CDReadTOC`, for a track the TOC does
  not hold and for 0 tracks (the manual: "A return value of true means
  that the command succeeded"). The original ignores its arguments,
  plays the author's test values (blocks 14000 to 15000) and returns
  DoIO's error code. Only `CDPlayTrack`, `CDReadTOC` and the TOC
  functions speak SCSI; `CDStop`, `CDPause` and the others send
  cd.device commands also with scsiflag (a SCSI play is a completed
  command: `CDStop` does not stop it). (#580)
* `CDPlayTrack` sends CD_PLAYTRACK (SendIO) with a request of its own,
  made with the library's request and given the device and unit after
  OpenDevice; `CDStop`, `CloseCD` (and finit) and the next `CDPlayTrack`
  abort a play that is out and take it back with WaitIO, and io_Command
  is not written. The original sends its one request: on a real
  cd.device the play stays in flight until the track ends, every other
  command (`CDUpdateInfo`, `CDStatus`, `CDPause`, `CDVolume`) fills the
  same request and sends it again, `CDStop` writes CD_PLAYTRACK into it
  and calls AbortIO without WaitIO, and the reply stays at the port
  (the next play links the same message again; `CloseCD` frees the
  request while it may still be in use). (#1156, #574)

**BUG(orig)** (kept):

* `CDPlayTrack` without scsiflag returns the d0 SendIO leaves (SendIO
  has no result; 0 with uaescsi.device), not true or false as the manual
  says. (#1157)

**Test** (`libs/tests/rwe/ricompactdisclib.c`, snitch case "rwe
ricompactdisclib"): each run loads one library set (the C build or the
original) with memlib; the scenario writes its observations as cases,
compared with the expected data
`libs/tests/expected/rwe_ricompactdisclib.txt` recorded from the
original. The library talks to a device of the test, `rwecd.device`
(MakeLibrary + AddDevice): the emulator's `uaescsi.device` answers the
SCSI path, but not cd.device's CD_TOCMSF and CD_QCODEMSF, and it ends
every play at once (measured 2026-11-20). It
plays a CD of 9, 50 or 20 tracks (CD_INFO with a Status word, CD_TOCMSF
summary and entries, a moving CD_QCODEMSF position, HD_SCSICMD READ TOC
header and track descriptors), replies a play (SendIO) at once or, in
hold mode, keeps it until AbortIO, logs every request (command, flags,
length, offset, where the data pointer points, the SCSICmd and its
command bytes), opens, closes and AbortIOs, the requests begun while a
held play is out and the replies left at the library's port, refuses
unit 3 and sets an io_Error on request (also for one request only).
Observed
are d0, the requests, the Q-code bytes behind `CDUpdateInfo`'s address
and the kept registers; at the end all memory must be back (a check of
the program's own). Cases:
every command without a device; OpenCD of cd.device (not in the
emulator: the failure path), of unit 3, with the name kept, while open
and in all three forms; all CD commands with on/off values 0-3 and -1;
io_Errors of CD_EJECT, CD_INFO, CD_TOCMSF, a failing summary read only;
CDReadTOC repeated (old TOC freed); a play in progress stopped, closed
and its request used again; the TOC functions before any CDReadTOC
(address 0); CDTrackLength 1-9 and 40-50 (the track 44 bug); offsets 0, 1,
4, 5, −1..−5 and set high bits for CDTrackMins/Secs; the SCSI path
(PLAY AUDIO(12) bytes before and after READ TOC, with tracks inside, at
the ends of and outside the TOC, READ TOC, the TOC functions, the
flag kept); CloseCD twice; finit with a device open and again; init
again; init without a port (AllocSignal fails) and OpenCD without a
request (AllocMem of 200 bytes fails). Not covered: a real drive, names over
40 bytes (bug-orig test `ricompactdisclib-opencd-name-overflow`),
CDTrackLength 0 or behind the last track (unwritten memory), the SCSI
`CDPlayTrack` on a 68000 (bug-orig test
`ricompactdisclib-playtrack-scsi-odd` on the a600; the library test runs
on the a1200), globalloc failing.
