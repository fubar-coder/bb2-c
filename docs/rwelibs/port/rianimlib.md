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

**C:** `libs/userlibs/rwe/rianimlib.c`, C behind thunks; the ANIM7
decoder of a plane (long and short data) is an `asm` block.
UnpackIFF (form 1: d0 source, d1 bitmap#, d2 -1,
d3 offset) and ILBMPalette (d0 source, d1 palette#) are called through
ALibJsr; RIPackLib's token list is that of the BSS2 binary. The chunk
search (InitFindChunk, FindChunk, FindNextChunk) is the code of
Pack-Library9, with its globals. The thunks keep d1/a0/a1 (GCC the
rest): RINextAnimFrame keeps everything but d0, as the original. Not ported,
without effect: `GetChunkHeader` (called by nothing), `_libfinit` (an
RTS no header refers to).
The `MULU`/`DIVU` of the original are `bl_mulu_w`/`bl_divu_w` (DIVU
with its overflow); the row steps have an `asm` barrier (GCC would
compute the loops' end pointer with `__mulsi3`, no libgcc in a library).
The ANIM7 plane decoder is assembler (`ANIM7_PLANE`, one block for the
long and the short form): in C, with a3-a5 fixed, gcc kept the column
or the op pointer on the stack and the frame of the benchmark `banim`
took 105761 cycles against the original's 98231. The block is the
original's loop (`'unpackaplane`) with the op's sign as the copy test and
the column's end behind each op, inline in `planes7` (no call per
plane): 97311 cycles, 0.99 of the original. The counts are those of the
C code (a count of 0 is 256, #508).

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
* The chunk walks (FindChunk, RIAnimFrameCount, the frame pointers,
  RINextAnimFrame's steps over the frame, ANHD and CMAP) add the pad
  byte of an odd length when it is 0; a non-zero byte there is taken as
  the next chunk (as ilbmifflib, #189; that chunk starts at an odd
  address, and RIAnimInit refuses such an ANIM, #1119 below). The original added length + 8 only: behind an
  odd-length chunk (an ANNO, a CMAP of an odd number of colours) the walk
  was one byte off, BMHD, frames, CMAP and DLTA were missed (#505).
  RINextAnimFrame pays a test of the length's bit 0 per step, about 20
  cycles on a 68000, three steps per frame.
* ANIM7 decodes the short (word) form when bit 0 of the ANHD bits is
  clear, with the words of the row as its columns, (width + 15) / 16.
  The original tested the operation (always 7) instead of the bits:
  every ANIM7 was decoded as long data, garbage for short data, and its
  `UnpackAnim7Short` was unreachable (#504). An ANIM7 frame pays a test
  of the bit and a compare per plane.
* ANIM7's long form has the long columns of the word-aligned row,
  rounded down: (width + 15) / 32; an ANIM7 long frame of a width up to
  16 (one word) decodes no column (RINextAnimFrame returns -1: the frame
  is applied, a row of one word has no long column). The original took
  width >> 5 (`ASR.w`): the rest of a width that is no multiple of 32
  was dropped, also a long column the row has (a width of 56: 4 words, 2
  longs, one decoded), a width below 32 gave `DBRA` -1, 65536 columns
  read from the delta and written far behind the planes, a width of
  32768 or more a negative count, 64512 columns for 32768 instead of
  1024 (#1115; bug-orig tests `rianimlib-anim7-long-columns`,
  `rianimlib-anim7-long-columns-rest` and
  `rianimlib-anim7-long-columns-wide`). RIAnimInit computes the count; an
  ANIM7 frame pays one compare (+43 cycles of an ANIM7 frame of the
  benchmark `banim`, 97440.8 to 97484.2, 0.04 %).
  **Limit (kept):** a row with an odd number of words (a width of 33 to
  48, 65 to 80, ...) has no long column for its last word: its 16
  pixels are not updated by ANIM7 long frames. The format's description
  (the ANIM7 appendix of 1992) does not say what the last long column of
  such a row holds; xanim truncates there as the original, FFmpeg
  decodes one more column and writes the upper word of each long.
  Truncating cannot write data the file does not hold there (user
  decision L1 Q2 (a) of 2026-11-20).
* RINextAnimFrame skips chunks of the ANIM that are no FORM on its way
  to the next frame (an `ANNO` between two frames); the original took
  such a chunk as the end of the animation and stopped there, or went on
  with the third frame with AnimLoop on (#1116; bug-orig test
  `rianimlib-nextframe-skip-chunk`). Free per frame: only the frame's
  FORM test that fails (a chunk that is no FORM, or the end) leads into
  the skip.
* RINextAnimFrame's steps over `ANHD` and `CMAP` stay inside the frame:
  a frame too short for its `ILBM` and the `ANHD` fields up to the bits,
  or an `ANHD` or `CMAP` whose length leaves no chunk header in the
  frame, returns 0. The original stepped by the length and read the next
  chunk ID and length anywhere in memory (#1117; bug-orig test
  `rianimlib-nextframe-anhd-length`: an `ANHD` length that ends at the
  `DLTA` of the next frame played that delta). Per frame a compare of the
  frame's length and two per step.
* RIAnimInit refuses an ANIM in which a chunk, of its top level or
  inside a frame, starts at an odd address (behind an odd length
  without its pad byte: the non-zero byte there taken as the next
  chunk), and FindChunk (RIAnimDepth, RIAnimWidth, RIAnimHeight) stops
  at such a chunk. The long reads of its ID and length were an address
  error on a 68000; a 68020 read them (#1119; bug-orig test
  `rianimlib-chunk-odd-address-68000` on the A600). One scan of the
  ANIM at RIAnimInit, stepping as RINextAnimFrame does; nothing per
  frame.
* RINextAnimFrame refuses (returns 0, no plane played) a frame whose
  `DLTA` leaves the frame, is shorter than its header of plane offsets
  (8 longs for ANIM5, 16 for ANIM7), or has an op list offset (ANIM7: or
  a data list offset) of a plane it plays at or behind its end. The
  original took the offsets as they were and read op lists and data
  from anywhere in memory (#1120; bug-orig test
  `rianimlib-delta-plane-offsets`: offsets that point at the next
  frames played their ops). Once per frame, before the first plane:
  about 0.6 % of an ANIM5 frame and 0.4 % of an ANIM7 frame of the
  benchmark `banim` (+195 and +463 cycles).
* RINextAnimFrame refuses (returns 0, no plane played) an ANIM7 frame
  whose `DLTA` has an odd data list offset for a plane it plays: the
  word or long reads of that data list were an address error on a
  68000 (#1121; bug-orig test `rianimlib-anim7-odd-data-offset-68000`
  on the A600). The frame is refused on every CPU, as the ANIM of #1119:
  a file a 68000 cannot play is refused on a 68020 too. A bit test per
  plane in the check of #1120: +130 cycles of an ANIM7 frame of `banim`
  (0.13 %), an ANIM5 frame unchanged.
* With runtime error checking RINextAnimFrame refuses an ANIM7 frame
  when a plane pointer of the bitmap plus RIAnimInit's offset is odd:
  "Animation position on an odd byte for ANIM7" (`nextchk`, error check
  area only). The offset is odd for an x whose byte offset x >> 3 is odd
  (x from 8 to 15, 24 to 31, ...) or an odd xy_offset; the ANIM7
  decoders write words and longs through those pointers, an address
  error on a 68000. ANIM5 writes bytes and plays at any offset. The
  check finds the coming frame as RINextAnimFrame does and reads the
  operation of its ANHD; the release code is unchanged (#1122, an
  invalid argument, D-005; bug-orig tests
  `rianimlib-nextframe-odd-x-anim7-68000` and
  `rianimlib-nextframe-odd-offset-anim7-68000` on the A600).

* InitFindChunk on data that is not a FORM but has the wanted ID at +8
  (a chunk pointed at directly, such as an IFF LIST of type ANIM) pushed
  a1 and returned without popping it: the RTS jumped to the caller's a1,
  and the original crashes. A return through a1 cannot be written in C
  (correction, technically unavoidable): the C build returns -1 as if a1
  had been popped and the search goes on with the chunk (#513; bug-orig
  test `rianimlib-initfindchunk-stack`, `fixed-orig = 513`).

Kept, without effect: the `DIVU.w #64` that undoes the compiler's
bitmap# × 64 in RIAnimInit's x,y form overflows (no change) from 64 ×
65536 on; the number is kept as a byte anyway (named in #514).

**Kept on purpose (not fixed, decision):**

* BUG(orig) (#508): a run or copy count of 0 is 256 (DBRA on the byte). Kept on
  purpose (won't fix, user decision W5 Q4 of 2026-11-19): encoders never
  write a count of 0 (the ANIM format and its op 7 appendix do not define
  it; Kent's skip.c writes no run of 0, and the reference decoders -
  Kent's unvscomp.asm, FFmpeg, xanim, AROS anim.datatype - read it as 0
  rows), and a test would cost every run and copy op of RINextAnimFrame,
  the game path (D-007). The bug-orig test `rianimlib-delta-count-zero`
  stays REPRODUCED.
* BUG(orig) (#1120, the part not fixed): the ops of a `DLTA`'s columns, and the ANIM7 data they take, are not
  bounded by the chunk: a corrupt op list reads on behind the `DLTA` and
  writes its deltas into the planes or behind them (user decision "C2 findings" of 2026-11-19: a check per op would
  cost every delta op of RINextAnimFrame, the game path, D-007; bug-orig
  test `rianimlib-delta-ops-past-chunk`, REPRODUCED). The plane offsets
  are checked (above).

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
+8 that is no FORM (the DIFF), odd chunk lengths (fixed #505; bug-orig
test `rianimlib-chunk-pad-byte`), depth 0, run counts
of 0, ANIM7 long frames of a width that is no multiple of 32 or of 32768
and more (fixed #1115; bug-orig tests `rianimlib-anim7-long-columns`,
`rianimlib-anim7-long-columns-rest` and
`rianimlib-anim7-long-columns-wide`).
