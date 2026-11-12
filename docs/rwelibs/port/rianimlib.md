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
`RIAnimFrameCount` counts the chunks of the ANIM, `RIAnimDepth`,
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

**C:** `libs/userlibs/rwe/rianimlib.c`, everything C but the thunk of
RINextAnimFrame (assembler: it saves d1–d7/a0–a6 like the original and
restores them unless the C function reports the case where the original
does not, see below). UnpackIFF (form 1: d0 source, d1 bitmap#, d2 -1,
d3 offset) and ILBMPalette (d0 source, d1 palette#) are called through
ALibJsr; RIPackLib's token list is that of the BSS2 binary. The chunk
search (InitFindChunk, FindChunk, FindNextChunk) is the code of
Pack-Library9, with its globals. The other thunks keep d1/a0/a1;
RINextAnimFrame keeps everything but d0, as the original. Not ported,
without effect: `GetChunkHeader` (called by nothing), `_libfinit` (an
RTS no header refers to), `UnpackAnim7Short` (unreachable, see below).
The `MULU`/`DIVU` of the original are `bl_mulu_w`/`bl_divu_w` (DIVU
with its overflow); the row steps have an `asm` barrier (GCC would
compute the loops' end pointer with `__mulsi3`, no libgcc in a library).

**BUG(orig)** (kept):

* RINextAnimFrame returns without restoring its 14 saved registers when
  a frame has no DLTA after its ANHD (and CMAP): the RTS takes the saved
  d1 as return address. Reproduced by the thunk (#515).
* InitFindChunk on data that is not a FORM but has the wanted ID at +8
  pushes a1 and returns without popping it: the RTS jumps to the
  caller's a1. **DIFF:** the C build returns -1 as if a1 had been popped
  (the original crashes; not reproducible in C) (#513).
* The chunk walks (FindChunk, RIAnimFrameCount, the frame pointers) add
  length + 8 without the pad byte of odd lengths (#505) and compare
  addresses signed (#506); RIAnimFrameCount counts every chunk of the
  ANIM, not only the frames (#510).
* RIAnimInit searches the BMHD from the first frame's chunks up to the
  end of the ANIM (a BMHD chunk of the ANIM behind the first frame is
  found) (#511); if no ILBM or BMHD is found it keeps the ANIM's address
  and end (RIAnimFrameCount counts it) and the frame pointers of the
  animation before (#512).
* RIAnimInit x,y: the offset is ebwidth × y (MULU.w, low words) + x >> 3
  (ASR.l); the `DIVU.w #64` that undoes the compiler's bitmap# × 64
  overflows (no change) from 64 × 65536 on (#514).
* ANIM7: the test for the short (word) form tests the operation
  (always 7), so the long form is always used and the short code is
  unreachable (#504); the columns are width >> 5 (ASR.w) – the rest of a
  width that is not a multiple of 32 is dropped, a width below 32 gives
  65536 columns (DBRA). ANIM5 likewise with width >> 3 (#507).
* The planes loop counts a byte up to the depth: depth 0 gives 256
  planes, more than 8 read behind the bitmap's plane pointers (#509). A
  run or copy count of 0 is 256 (DBRA on the byte) (#508).

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
Not covered: a frame without DLTA (the crash), a chunk with the ID at
+8 that is no FORM (the DIFF), odd chunk lengths, depth 0, run counts
of 0, widths below 8 or 32.
