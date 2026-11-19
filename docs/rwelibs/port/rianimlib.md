# rianimlib (RIAnimLib, library 57)

**What it does:** plays ANIM5/ANIM7 animations that are in memory.
`RIAnimInit(address,bitmap#,palette#[,offset]|[,x,y])` finds the first
frame (the FORM ILBM in the FORM ANIM) and its BMHD, unpacks it with
RIPackLib's `UnpackIFF` and `ILBMPalette` and remembers the second and
third frame; `RINextAnimFrame(bitmap#)` applies the next frame's DLTA
(ANHD operation 5 or 65: byte vertical delta, 7: long vertical delta)
to the bitmap's planes (plus the offset), a CMAP of the frame through
`ILBMPalette`; after the last frame it goes on with the third if
`AnimLoop On` (the default), otherwise it returns 0.
`RIAnimFrameCount` counts the frames (FORM ILBM chunks) of the ANIM, `RIAnimDepth`,
`RIAnimWidth`, `RIAnimHeight(address)` read the first frame's BMHD.

**Source and reference:** `rwelibsrc/ANIMLib/Anim-Library3.bb` (BB22,
converted by `bbsrc bb22`; Reflective Images, Stephen McNamara,
1994/95, file date 1997-06-09). The source does not compile: its last
command, `RIDiskAnimInit "animfile$,datafile$,bitmap#,palette#"`, is
unfinished (it uses `diskanimflag` and `animfile`, which are defined
nowhere; `RWE_REFBUILD_SKIP` in `refbuild.cmake`). The reference is the
BSS2 `RIANIMLIB.OBJ` (= AmiBlitz3 `Rianimlib.obj`), which is the source
without the RIDiskAnimInit scaffolding (its declaration and lines 89–154
of the text: the data longs `bmap` … `ilbmbuffer`, `oops`, the routine),
byte for byte but a padding word (`.cache` analysis, README 3.2). **The
port leaves RIDiskAnimInit out** for that reason; its declarations
equal the BSS2 binary (`rwe_declarations_rianimlib`). The BUM/OL
`RIANIMLIB.OBJ` (4 entries) is the older v1.3. Library number
`LIB_rianimlib` (57) in the bb2-2.10 libnums.

**C:** `libs/userlibs/rwe/rianimlib.c`, everything C behind thunks.
UnpackIFF (form 1: d0 source, d1 bitmap#, d2 -1,
d3 offset) and ILBMPalette (d0 source, d1 palette#) are called through
ALibJsr; RIPackLib's token list is that of the BSS2 binary. The chunk
search (InitFindChunk, FindChunk, FindNextChunk) is the code of
Pack-Library9, with its globals. The thunks keep d1/a0/a1 (GCC the
rest): RINextAnimFrame keeps everything but d0, as the original. Not ported,
without effect: `GetChunkHeader` (called by nothing), `_libfinit` (an
RTS no header refers to), `UnpackAnim7Short` (unreachable, see below).
The `MULU`/`DIVU` of the original are `bl_mulu_w`/`bl_divu_w` (DIVU
with its overflow); the row steps have an `asm` barrier (GCC would
compute the loops' end pointer with `__mulsi3`, no libgcc in a library).

**Fixed** (intended deviations, `docs/libs/deviations.md`, kind F):

* RIAnimInit searches the BMHD in the first frame only; the original
  searched up to the end of the ANIM and took a BMHD chunk of the ANIM
  behind a first frame without one (#511).
* A failed RIAnimInit leaves no animation: RIAnimFrameCount and
  RINextAnimFrame return 0. The original kept the ANIM's address and end
  when no ILBM or BMHD was found (RIAnimFrameCount counted it) and the
  frame pointers of the animation before, which RINextAnimFrame went on
  playing (#512).
* RIAnimFrameCount counts the FORM ILBM chunks of the ANIM, the frames;
  the original counted every chunk (an ANNO or other chunk of the ANIM
  as a frame) (#510).
* The chunk walks compare addresses unsigned and stop at a chunk whose
  length passes the end. RIAnimInit ends the ANIM at the first chunk of
  its top level that does not fit (so RINextAnimFrame's step from frame
  to frame stays inside the ANIM) and fails if the first frame does not
  fit. The original compared signed: a corrupt length that moved the
  pointer past $7FFFFFFF made it "below" the end, and the walks went on
  reading chunk headers from there (#506).
* RINextAnimFrame skips other chunks between ANHD (and CMAP) and the
  DLTA inside the frame (a CMAP among them goes through `ILBMPalette`)
  and returns 0 for a frame without DLTA. The original returned without
  restoring its 14 saved registers: the RTS took the saved d1 as return
  address (#515).
* RIAnimInit fails for a BMHD depth of 0 or above 8, before UnpackIFF.
  The original's planes loop counted a byte up to the depth: depth 0 gave
  256 planes, more than 8 took plane pointers from behind the bitmap's 8
  and plane offsets from behind the DLTA's (#509).
* ANIM5 decodes the byte columns of the word-aligned row, (width + 15) /
  16 × 2, as the encoders write them (Kent's skip.c, FFmpeg). The
  original took width >> 3: the rest of a width that is no multiple of 8
  and the pad byte of the row were never decoded, and a width below 8 gave
  65536 columns (DBRA). RIAnimInit computes the counts and fails for a
  width of 0 (#507, its ANIM5 half).
* With runtime error checking RIAnimInit's x,y form refuses a position
  outside the bitmap (x not in 0 to its width − 1, y not in 0 to its
  rows − 1): "Animation position outside the bitmap" (`xychk`, an error
  check area the original has not; `rianimlib.declarations-intended.txt`).
  The release code is unchanged (an invalid argument, D-005): the offset
  is ebwidth × y (MULU.w, low words) + x >> 3 (ASR.l), so a negative y
  or one of 65536 and above gives an offset of up to 65535 rows, where
  UnpackIFF and every RINextAnimFrame write; the manual: "You should
  ensure that your animation will never go off screen" (#514).
* RIAnimInit fails (0) when RIPackLib's `UnpackIFF` refuses the first
  frame: a bitmap too narrow (fewer bytes per row than the frame's
  word-aligned row), too shallow or without plane 0. The original
  dropped UnpackIFF's result, returned true and played the deltas into
  that bitmap. The height is not checked (UnpackIFF unpacks as many rows
  as the bitmap has; the manual: "You must ensure that the bitmap is big
  and deep enough to actually hold the animation"). ILBMPalette's result
  is not looked at, as in the original (#1106).
* init (a nullsub behind the commands; the original has none) clears
  the animation and sets AnimLoop on. Without it a second run of a
  program with the library still loaded (from the editor) started with
  the run before's animation: RIAnimFrameCount and RINextAnimFrame
  before an RIAnimInit walked it (perhaps freed memory), and an AnimLoop
  Off of the run before stayed off (the manual's default is on)
  (#1107; `rianimlib.declarations-intended.txt`).

Kept, without effect: the `DIVU.w #64` that undoes the compiler's
bitmap# × 64 in RIAnimInit's x,y form overflows (no change) from 64 ×
65536 on; the number is kept as a byte anyway (named in #514).

**BUG(orig)** (kept):

* InitFindChunk on data that is not a FORM but has the wanted ID at +8
  pushes a1 and returns without popping it: the RTS jumps to the
  caller's a1. **DIFF:** the C build returns -1 as if a1 had been popped
  (the original crashes; not reproducible in C) (#513).
* The chunk walks (FindChunk, RIAnimFrameCount, the frame pointers) add
  length + 8 without the pad byte of odd lengths (#505).
* ANIM7: the test for the short (word) form tests the operation
  (always 7), so the long form is always used and the short code is
  unreachable (#504); the columns are width >> 5 (ASR.w) – the rest of a
  width that is not a multiple of 32 is dropped, a width below 32 gives
  65536 columns (DBRA) (#507, its ANIM7 half: what the last long column
  of a row with an odd number of words holds is not in the format's
  description, the ANIM7 appendix of 1992; xanim truncates as the
  original, FFmpeg writes the upper word of the last long: not decided
  here).
* A run or copy count of 0 is 256 (DBRA on the byte) (#508).

**Test** (`libs/tests/rwe/rianimlib.c`, snitch case "rwe rianimlib"):
each run loads one library set (the C build or the CD original); the
scenario writes its observations as cases, compared with the expected
data `libs/tests/expected/rwe_rianimlib.txt` recorded from the original.
The test builds small ANIMs in memory (64×8, 2 planes: frames with ANHD
operations 5, 5 with CMAP, 7, 65 and 3; one and two frame anims;
failing ones) and gives the set a copy, a bitmap object (ebwidth 8,
guarded plane buffers) and register markers. RIPackLib is not loaded:
UnpackIFF and ILBMPalette are stubs (`sysload_stubs`) that log their
arguments (addresses relative to the ANIM). Observed after every call: d0, the stub log, the planes with the bytes around them and
the kept registers. Cases: RIAnimDepth/Width/Height on an ANIM and on
five kinds of wrong data; RIAnimFrameCount before and after
RIAnimInit; RIAnimInit in all three forms (offset 0, offset 9, x,y with
a marker in y's upper word, bitmap# × 64, a DIVU overflow, a negative
x), failing, with a BMHD behind the first frame, one and two frames;
RINextAnimFrame before RIAnimInit, through every frame, past the end
with AnimLoop on and off; AnimLoop with a value whose low byte is 0.
Not covered: a frame with other chunks before its DLTA (the original
crashes; bug-orig test `rianimlib-nextframe-no-dlta`), a chunk with the ID at
+8 that is no FORM (the DIFF), odd chunk lengths, depth 0, run counts
of 0, ANIM7 widths below 32.
