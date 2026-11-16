# libs-c – Blitz Basic 2 libraries in C

The libraries of Blitz Basic 2 (system libs like `ldivlib`, later the
command libraries) as readable C sources from which **functionally equivalent
library files in BB2 format** can be built. The basis are the
BB2 originals in `archive/aminet/amibl2src/bblibsrc/` (readable: `build/<preset>/bblibsrc/`).

Two ways (see agreement):

* **Original libraries** (`archive/cd/ultimate-blitz-basic-2.1/BLITZ2/DEFLIBS`, `ACIDLIBS`): with these a
  compiler must produce byte-identical programs. Nothing is rebuilt here.
* **C libraries** (`libs/`): same format, same tokens,
  same calling convention, **functionally the same** – checked in the emulator
  against the original. The machine code is different (GCC instead of
  hand-written assembler), so programs produced with them are not byte-identical.

## Building and checking

```
cmake --preset amiga && cmake --build --preset amiga
#  -> build/amiga/libs/acidlibs/libNAME.a   static library (basis, later usable directly with ld)
#  -> build/amiga/libs/acidlibs/NAME.obj    library file in BB2 format
build/amiga/host/tools/blitzlib/blitzlib extract archive/cd/ultimate-blitz-basic-2.1/BLITZ2/ACIDLIBS 64735 ldivlib.orig
build/amiga/host/tools/blitzlib/blitzlib compare ldivlib.orig build/amiga/libs/acidlibs/ldivlib.obj
ctest --preset emulator -R libs_emulator          # Amiberry, see below
```

## What a library looks like in C

A BB2 library was a Blitz program: the macros from `libmacs.res`
(`archive/cd/ultimate-blitz-basic-2.1/EXTRAS/BLITZLIBS/LIBMACS.BB`) generated header and
token table with `Dc` in front of the assembler code. `libs/include/blitzlib.h` reproduces each
of these macros and generates **the same bytes** – as data in the
section `.blitzlib.head`, which `libs/blitzlib.ld` places at offset 0 of the only
code hunk:

```c
/* !syslibheader{#ldivlib,$cb00,$cb02,0,0,0,0,$4000} */
BLITZ_SYSLIBHEADER(LIB_ldivlib, 0xcb00, 0xcb02, 0, 0, 0, 0, 0x4000);
BLITZ_NULLSUB(qdivide, 0, 0);           /* $CB00 quickdiv */
BLITZ_NULLSUB(ldivide, 0, 0);           /* $CB01 longdiv */
BLITZ_LIBFIN();

LONG ldivide(LONG a BL_REG(d0), LONG b BL_REG(d1)) { ... }
```

* **Registers:** arguments as GCC register parameters (`BL_REG(d0)`),
  result in d0. a5 (globals) and a4 (locals) are fixed registers
  (`-ffixed-a4 -ffixed-a5`) and accessible in C as `blitz_globals`/`blitz_locals`
  (global register variables).
* **Check routines** (only with debugger): `BLITZ_ERRCHK` places them behind
  `BLITZ_ERROR_AREA`. References from normal code to there point, without
  debugger, behind the end of the code (the originals partly do that too, e.g.
  memlib err1 → mess1); until 27.10.2026 `mkblitzlib.py` warned about it, since then
  nobody checks this automatically any more (here `blitzlib check` warns again).
* **Order in the error check area:** the assembler routines of `blitzlib.h`
  (`BLITZ_THUNK`, `BLITZ_ERRCHK_THUNK`, `BLITZ_ERRCHK_MESSAGE`,
  `BLITZ_ERRCHK_AB`, `BLITZ_ASM_ROUTINE` ...) are C functions whose body is
  only the assembler text, so GCC switches to their section itself; thunks,
  messages and check functions may come in any order (#1056). Top-level
  `__asm__` that switches the section is invisible to GCC (the GNU as of this
  toolchain has no `.pushsection`/`.popsection`): if it ends in another
  section than the one GCC believes current, the next function of that
  section gets no section directive and lands where the asm left off.
  Therefore a library writes no section directive of its own (#1057):
  assembler in the error check area is `BLITZ_ERRCHK_ASM`, assembler in the
  normal code `BLITZ_ASM_ROUTINE` or, for several routines, handlers with
  BRA entries in front and data, `BLITZ_ASM_BLOCK`. The build step of every
  library refuses a `.section`, `.text`, `.data` or `.bss` in the
  assembler of its sources (`cmake/check-asm-sections.cmake`); only
  dhandlerlib's `.section .blitzlib.head` is allowed (its code is in the
  header section, where GCC cannot place a function). `blitzlib check`
  reports a check thunk whose C function is in the normal code.
* **Fidelity:** algorithms are taken over bit-exactly (e.g. division by 0,
  overflow), not replaced by C operators.

Further macros in `libs/include/blitzlib.h` (details in the comments there):

| Macro | Purpose |
|---|---|
| `BLITZ_THUNK`, `_Z`, `_PRE`, `_POST`, `_POP` | entry point that saves registers / sets CCR / clears stack arguments, calls a C function |
| `BLITZ_ALIBJSR(tok, r)`, `_FORM`, `BLITZ_BLIBJSR` | call a routine of another library (d0/d1/a0/a1 via `struct bl_regs`); like every ALibJsr/BLibJsr macro it has a3 (`blitz_string`) as in/out operand, because a string function moves a3 |
| `BLITZ_ALIBJSR_A6(tok, base, x)` | the same with a6 loaded (e.g. floatquicklib `$D300` needs the ffplib base), d0 there and back |
| `BLITZ_BASEJSR(base, off, r)`, `BLITZ_BASEJSR1/2(base, off, x0[, x1])` | `JSR off(a6)` with a6 = base (mathffp, memlib table, system library); 1/2 = operands in d0/d1 |
| `BLITZ_ALIBJSR_CCR(tok, r, "cc")`, `BLITZ_ALIBJSR_CCR_D0A0(tok, "cc", &d0, a0)` | call of a routine that answers in the CCR (iffiolib: Z = ok); result `Scc` directly after the call |
| `BLITZ_ALIBJSR_D4`, `BLITZ_BLIBJSR_D4(tok, d0, d1, d2, d3)` | call with arguments in d0–d3 (e.g. spriteslib ShowSprite) |
| `BLITZ_ENTRY_C(f)` | code label as `char e_f[]` for table entries with address arithmetic |
| `BLITZ_AMIGAONLY(code)`, `BLITZ_BLITZONLY`, `BLITZ_BLITZONLY_CHK(chk)` | address + 1 in `!subs`: Amiga mode only / Blitz mode only (err1 = 1 or check + 1) |
| `BLITZ_ERRCHK_AB(entry, blitz, amiga)` | error check with Blitz/Amiga variant (`$a001` prefix) in the error check area |
| `BLITZ_ERRCHK_THUNK(entry, cfunc, save)` | thunk of an error check in the error check area; `save` = registers that the original check preserves |
| `BLITZ_ERRCHK_MESSAGE(label, "Text")` | error message as assembler data in the error check area (GCC allows no code and data in the same named section) |
| `BLITZ_ERRCHK_ASM(name, "text")` | assembler in the error check area (checks that test the CCR or need the caller's return address, error branches); the text has its own labels |
| `BLITZ_RUNERR(msg)` | runtime error (`TRAP #0`) |
| `BLITZ_INLINE`, `BLITZ_AB_ROUTINE`, `BLITZ_ASM_ROUTINE` | inline routine, Blitz/Amiga variant, assembler routine |
| `BLITZ_ASM_BLOCK(name, "text")` | assembler in the normal code with its own labels: several routines, handlers with BRA entries in front, data in the code hunk |

`libs/include/blitzhelp.h` (included by `blitzlib.h`) contains small
shared helpers: `BL_SETW`/`BL_SETB` (word/byte operation, rest of the
register stays), `BL_EXTL`, `BL_SWAP`, `bl_divu_w`/`bl_divs_w` (DIVU/DIVS
as an instruction, overflow leaves the register as is), `BL_POPL`/`BL_POPW` (local
cursor on the a2 argument stack), `LVO_SPFix` … `LVO_SPCeil` (mathffp) and
`TOK_globalloc` … or `MEMBASE_localloc` … (frequently called routines of the
system libs), `bl_mulu_w`/`bl_muls_w` (MULU.w/MULS.w as an instruction, otherwise
GCC partly calls `__mulsi3`), custom chip accesses `BL_CUSTOM`, `BL_CUSTOMW`/`BL_CUSTOML`
(registers relative to the chip base), `BL_BLITBUSY`/`BL_BLITWAIT`,
`BL_INTENA`/`BL_INTS_OFF`/`BL_INTS_ON` as well as tokens of frequently called
command libraries (`TOK_allocst`/`TOK_freest`, `TOK_setint`/`TOK_clrint`/`TOK_vwait`,
`TOK_execversion`, `TOK_iffopen` … `TOK_iffread`, `TOK_ilbminit` …
`TOK_ilbmbitmap`, `TOK_getpal`/`TOK_getpal2`, `TOK_convertkey` …
`TOK_qconvertkey`, `TOK_showsprite`, `TOK_setevfilter`, `TOK_extbitmap`).
`libs/include/blitzexec.h` has `BL_FREESIGNAL(sig)` (FreeSignal with
zero-extended signal number like the originals; the NDK inline takes a
BYTE). What only one library needs stays in its source.

## Tools

| File (bb2 `libs-c/`) | Purpose | here |
|---|---|---|
| `tools/detok.py` | make tokenised BB2 sources (`bblibsrc`) readable | `programs/ted/tools/tedsrc`, target `bblibsrc` |
| `cmake -P tools/mklibnums.cmake` | generate `include/libnums.h` from the original `libnums.bb` (`LIB_ldivlib` = `#ldivlib`) | not taken over (`libs/include/libnums.h` is in git) |
| `tools/biglib.py` | extract libs from BigLib files (`extract`), show them (`dump`), compare declarations (`compare`), replace (`replace`), reassemble (`join`) | `blitzlib` (`extract`, `join`), `blitzlib compare` (compare) |
| `tools/mkdeflibs.py` | one-time generator: `deflibs/NAME.c` from `cd/blitz2/deflibs` (see below, [deflibs.md](deflibs.md)) | not taken over |
| `tools/srcdecl.py` | evaluate the declarations (libmacs macros) of a Blitz source on the host and compare them with a lib binary (`dump`/`compare`/`report`), see [cmdlibs-decl.md](cmdlibs-decl.md) | not taken over |
| `tools/amiga_test.py` | `tests/libs_tests` (snitch) in Amiberry, optionally with a filter; serial, timeout 30 s | `libs/tests/emulator.cmake`, ctest `libs_emulator` |
| `tools/disasm.py` | disassemble the code of a library, entry points and relocs marked | not taken over |
| `tools/asmcmp.py` | compare assembler routines original/C build instruction by instruction | `refcheck asm` (`tools/refcheck`) |

`amiga_test.py` used `tools/amiberry.cmake` of bb2 (emulator without window,
configuration `A1200-AB3.uae`, 68020, drive `AB3C:`) and put the test files under
`ab3c/build/amiberry/bb2-libs-c/`; here `tools/lib/amiga_emulation` (drive `TEST:`).

## State: all 25 system libs

All declaration parts are identical to the originals
(`blitzlib compare`); all tests in the emulator pass. The tests
are a snitch program (`libs/tests/libs_tests`, same setup as ab3c);
the test logic itself is C:

| Test case (tag) | File | Libraries | Checks |
|---|---|---|---|
| `libtest ldivlib`/`lmullib` `[libtest]` | `libtest.c` | ldivlib, lmullib | 20 729 cases per routine |
| `[sys]` | `systest.c` | memlib, datalib, chipbaselib, strings1lib, staticslib, varslib, exitslib, clrlib | 48 |
| `[mathstr]` | `systest_mathstr.c` | strings2lib, ffplib, strcomplib, floatquicklib, modlib, maxlenlib | 56 864 |
| `[runtime]` | `systest_runtime.c` | arrayslib, maxslib, errtraplib, rundebuglib | 113 |
| `[system]` | `systest_system.c` | wbstartuplib, intlib, switchlib, runerrlib, debuglib | 178 |
| `[cmd]` | `systest_cmdlibs.c` | memacclib, sortlib, mathtranslib, utillib, environlib (command libraries, see below); the exceptions of the register rule; the small assembler entries of vallib, scrolllib, gameiolib, mouselib, collslib, shapeslib, fontlib, windowslib and blitzkeyslib | 3 187 |
| `[blit]` | `systest_blit.c` | blitlib, qblitlib, bblitlib, stencillib, 2dlib ClipBlit (the blitter commands in Blitz and Amiga mode on bitmaps and shapes in chip memory: shifts, handles, excess planes, modes, collisions, clipping) | 2 670 |
| `[gfx]` | `systest_gfx.c` | 2dlib (Line, Box, Boxf, Circle, Ellipse, Circlef, Ellipsef, Poly, Polyf in Blitz and Amiga mode: clipping, origin, colours), palettelib (PalRGB, AGAPalRGB, FadePalette, ShowPalette, Use Palette, RGB, Red, Green, Blue), blitzcoplib (Show, ShowF, ShowB), spriteslib (ShowSprite), displaylib (DisplayBitMap, DisplaySprite, DisplayPalette), scrolllib (Scroll) | 4 692 |
| `[tracker]` | `systest_tracker.c` | trackerlib (PlayModule, the player's interrupt routine once per frame on a generated module with every effect, StopModule, finit; Paula's registers redirected to RAM) | 21 532 |

A test program loads one library set (the C build or the original) and
writes every observation as a case; the cases are compared with the
expected data `libs/tests/expected/NAME.txt` (`tests/README.md`,
"Libraries").

```
ctest --preset emulator -R libs_emulator          # all
cmake ... -DFILTER="[mathstr]" -P libs/tests/emulator.cmake   # one group (README)
```

Reference logs: bb2 `libs-c/tests/ref-logs/` (snitch run, structure comparison,
instruction comparison; not taken over).

Every call is additionally checked that the C version preserves at least
all registers that the original preserves. Routines that remain
assembler are compared by `ctest --preset host` (`asmcmp_*`, list in
`libs/acidlibs/CMakeLists.txt`) instruction by instruction with the original.

**Assembler remains** only where C cannot express it: saving/setting the
stack pointer (exitslib, parts of runerrlib, errtraplib), exception and
trap handlers with RTE (errtraplib, rundebuglib, runerrlib, debuglib),
jump tables and data blocks with a fixed layout (memlib, debuglib),
inline routines and Blitz/Amiga prefixes (chipbaselib, intlib). Everything else
is C.

**Not executable in the emulator** (only structure/instruction comparison): everything
that switches on Blitz mode (switchlib goblitz) or is only reached from the
debugger/trap state machine (parts of debuglib,
runerrlib, rundebuglib TRAP #0), as well as the Workbench start of wbstartuplib.
The details are in the header comments of the libraries.

**Binary newer than the source:** `switchlib` (keyboard via the
device list) and `debuglib` (additional token `$D509`, extended
handler table) – the reference is the binary of the CD.

**For the comparison with AmiBlitz3:** not a bug, only kept: `MOD` on Quick/Float uses the integer parts – the
library returns `Quotient<<16|Rest` for Quick, a long for Float, but the
compiler reads the result as word or long, blitz2.s:9459/9463 – which is
the intended integer semantics of MOD. Fixed since (intended deviations): division by 0 gives
the largest number with the dividend's sign, `$7FFFFFFF` or `-$7FFFFFFF`
(#200), with runtime error checking it is "GURU! - Division by Zero" as
for a Word division (#200);
`newlist` wrote, for size 0 (e.g. dimension −1), 8 bytes behind the block
(#59); Quick 0 became the Float `$00000080` and Quick −32768 the Float
+32768 (#146, #147); `arrmult` added its second partial product via SWAP
and let an array of 4 GB or more go on modulo 2^32, now such a `Dim` fails
with "Out of Memory!", with runtime error checking with "Array Size out
of Range" (#58); `lmultiply` and `qmultiply` added partial
products via SWAP, on an overflow the product is now the one modulo 2^32
(#199, next section).

### lmullib: the products of the original (#199)

The Long and Quick multiplication (`*` on `.l` and `.q`, lmullib's
`longmult` $CA01 and `quickmult` $CA00) works on the magnitudes of the
factors with three or four 16x16 bit MULU. The original added the
partial products SWAPped instead of shifted by 16, so the high word of a
partial product also landed in the low word of the result. The port gives
the product modulo 2^32 (Quick: bits 16..47 of the 64 bit product), with
the sign applied last. With A, B the magnitudes and ah, al, bh, bl their
16 bit halves, the original's result differs from it exactly in these
cases:

* Long: when `ah*bl` or `al*bh` is 65536 or more. The magnitude was too
  large by the sum of the high words of these two products, at most
  65534 (`196608*30000` gave 1603272705 instead of 1603272704). Then A*B
  is 2^32 or more: a product that fits in 32 bits never changes, and not
  every one that does not fit changes (`65536*65536` is the same).
* Quick: when the product of the integer parts `ah*bh` is 65536 or more,
  i.e. the true product is at least twice outside the Quick range. The
  value was too large in magnitude by the high word of `ah*bh`, at most
  16384/65536 = 0.25 (`300*300` gave `$5F90.0001` instead of
  `$5F90.0000`).

Word and byte multiplications use MULS and were never affected. A program
that needs the original's results, e.g. a calculation that lets a Long
overflow on purpose and must give the same numbers as before, can compute
them with these functions (from the analysis of #199, checked in the
emulator with 3000 pairs of mixed sizes and signs against the original
library and the fixed one: the same results with both). Every
multiplication in them has factors below 65536, which both libraries
compute exactly; a call per product is far slower than the operator.

```
; the product as the original lmullib gave it (Long)
Function.l OldMul{a.l,b.l}
  n.l=0
  If a<0 Then a=-a : n=1-n
  If b<0 Then b=-b : n=1-n
  ah.l=(a LSR 16) AND $FFFF : al.l=a AND $FFFF
  bh.l=(b LSR 16) AND $FFFF : bl.l=b AND $FFFF
  c1.l=ah*bl : c2.l=al*bh
  p.l=al*bl+(((c1+c2) AND $FFFF) LSL 16)+((c1 LSR 16) AND $FFFF)+((c2 LSR 16) AND $FFFF)
  If n Then p=-p
  Function Return p
End Function

; the same for Quick
Function.q OldQMul{a.q,b.q}
  ra.l=Peek.l(&a) : rb.l=Peek.l(&b) : n.l=0
  If ra<0 Then ra=-ra : n=1-n
  If rb<0 Then rb=-rb : n=1-n
  ah.l=(ra LSR 16) AND $FFFF : al.l=ra AND $FFFF
  bh.l=(rb LSR 16) AND $FFFF : bl.l=rb AND $FFFF
  hh.l=ah*bh
  p.l=(((al*bl) LSR 16) AND $FFFF)+ah*bl+al*bh+(hh LSL 16)+((hh LSR 16) AND $FFFF)
  If n Then p=-p
  r.q=0 : Poke.l &r,p
  Function Return r
End Function
```

For a factor known in advance the correction is shorter: for a
non-negative `x`, `x*69069+(((x LSR 16)*3533) LSR 16)` is the original's
`x*69069` (69069 = 65536 + 3533; the corpus programs `t158_2dfuzz` and
`t159_polyfuzz` compute it from 16 bit products in their `rn{}`).
AmiBlitz3's own lmullib keeps the original routines (only its 68020
entry `$CA02`, which its compiler uses with 68020 optimisation, multiplies
with `MULS.L`; the AmiBlitz3 mode of `blitz2-ab3` loads that library;
`docs/ab3/acidlibs-a-l.md`), and AmiBlitz3 lets a
library of its userlibs directory replace a loaded library of the same
number (`AmiBlitz/AmiBlitz3@7ab0350b`, test `ab3_cli` case `userlib`, also
run with AmiBlitz3's compiler).

## State: command libraries (acidlibs)

The "simple" command libraries, ordered by size. Declarations
with `blitzlib compare` against the CD binary
(`blitzlib extract archive/cd/ultimate-blitz-basic-2.1/BLITZ2/ACIDLIBS NUM NAME.orig`), function
checked twice:

* **BASIC level:** test programs `tests/corpus/tests/` t09–t13, t20, t21, t30, t31
  as well as `tests-runerr/` t32–t36 (only with error checking)
  (every token, edge cases); ctest `corpus`/`corpus_runerr` (`tests/corpus/runner`) compiles
  them with original compiler + original libs and with the C port + C `acidlibs`
  (the CMake target `acidlibs` picks up every `libs/acidlibs/*.c`) and compares
  the outputs, with and without runtime error checking.
* **Routine level:** `[cmd]` in `libs/tests/libs_tests` (results, memory,
  a3, register preservation as with the system libs).

| No | Library | Tokens | Declarations | Tests | Assembler |
|---:|---|---|---|---|---|
| 180 | memacclib | Poke Peek Peeks$ Call | identical | t09, `[cmd]` | `Call` (JMP with all registers) |
| 110 | sortlib | Sort SortUp SortDown | identical | t10, `[cmd]` | CCR of mathffp `SPCmp` → -1/0/1 |
| 192 | mathtranslib | ACos … HTan, `^` (Float) | identical | t11, `[cmd]` | inline routines `JSR lvo(a6)` |
| 194 | utillib | CreateMsgPort DeleteMsgPort Wait TaskName$ | identical | t12, `[cmd]` | – |
| 193 | environlib | WBWidth … WBExtViewMode, cr_vbr | identical | t13, `[cmd]` | `MOVEC VBR` |
| 78 | datelib | SystemDate Date$ NumDays DateFormat Days … Secs | identical | t30 | DIVU/DIVS as an instruction (overflow behaviour) |
| 128 | linklistlib | FirstItem AddItem … SortList (14 of the binary) | identical | t31, t32–t36 (runtime errors) | error texts as data, thunk of the error checks |
| 185 | stringfunclib | Left$ Right$ Mid$ … Replace$ Cvi/Cvl/Cvq | identical | t20 | inline `Chr$`, `Len`; thunks |
| 120 | vallib | Val Int Sgn Frac … Rnd Abs QAngle | identical | t21 | inline QFrac QWrap QAbs VPos True False |
| 124 | fileiolib | OpenFile ReadFile WriteFile CloseFile KillFile Fields Put Get FileOutput FileInput Lof FileSeek Loc Eof DosBuffLen WriteMem ReadMem Exists | identical | t50, t53, t54–t57 (runtime errors) | BRA table of the I/O handlers; thunks |
| 106 | cliargslib | NumPars Par$ FromCLI ParPath$ | identical | t51, t58–t59 (runtime errors) | – (thunks) |
| 76 | banklib | Bank InitBank FreeBank LoadBank SaveBank BankLoc BankSize AllocMem FreeMem | identical | t52 | – (thunks) |
| 164 | inputoutputlib | DefaultInput DefaultOutput PopInput PopOutput (+ 7 channel routines) | identical | t41, t43–t45 (runtime errors) | channel dispatcher `JSR off(a6)`, BRA.w heads of the handlers, trap of defin |
| 187 | printlib | Print NPrint Str$ Format FloatMode UStr$ | identical | t40 | BRA.w head of the Str$ handler |
| 186 | editlib | Edit$ Inkey$ Edit | identical | t41, t42, t46–t49 (runtime errors) | – |
| 74 | crunchlib (`#romuluscrunchlib`) | Implode Deplode CrMDecrunch PPDecrunch | identical | t60, host comparison (unicorn) | – |
| 92 | arexxlib | CreateRexxMsg DeleteRexxMsg FillRexxMsg … RexxEvent IsRexxMsg RexxError | identical | t61 | – |
| 90 | seriallib | OpenSerial WriteSerial … SerialEvent ReadSerialMem WriteSerialMem | identical | t62, t63 (runtime errors), t64 (reception with remote station, `tests/corpus/runner/serial.c`) | – |
| 158 | screenslib | Screen WbToScreen LoadScreen FindScreen … ShowBitMap ScreenTags | identical | t81, t86–t89 (runtime errors) | inline ViewPort/Show/Hide/Beep/MoveScreen/SMouseX/Y |
| 142 | fontlib | LoadBlitzFont BitMapOutput Colour Locate CursX CursY BitMapInput | identical | t82, t84 (BitMapInput not testable) | BRA.w heads of the handlers, `doinput1` |
| 134 | intuifontlib | LoadFont (binary without `Text`) | identical | t82, t85 | `loadchk` (CCR check) |
| 130 | rawkeylib | – (convertkey, unconvert, qconvertkey for other libs) | identical | indirectly (t72 via windowslib) | – |
| 138 | windowslib | Window WaitEvent Event … WindowTags (75) | identical | t70–t72, t75, t76, t78, t79 | BRA table of the handlers; inline RastPort |
| 136 | menuslib | MenuTitle SetMenu MenuItem SubItem ShapeItem ShapeSub MenuGap SubItemOff MenuState MenuColour MenuChecked | identical | t73, t71, t77 | – |
| 108 | mouselib | Mouse Pointer MouseArea MouseX MouseY MouseXSpeed MouseYSpeed PositionMouse | identical | t74 | call to ShowSprite with d0–d3 |
| 162 | iffiolib | (none; routines +1…+8 for other libs) | identical | indirectly t100, t103 | – (thunks, CCR result via tst.w) |
| 161 | ilbmifflib | ILBMInfo ILBMWidth ILBMHeight ILBMDepth ILBMViewMode DecodeILBM | identical | t100, t103 | – (thunks; ALibJsr with Z flag) |
| 114 | iffmakelib | SaveBitmap SaveScreen SaveShape SavePalette | identical | t100, t104, t107 | – (thunks) |
| 132 | brexxlib | Record PlayBack SaveTape LoadTape AbsMouse RelMouse MouseButton ClickButton MacroKey TapeTrap QuietTrap Type QuickPlay XStatus PlayWait FreeMacroKey | identical | t101, t106 | – (thunks; input handler in C) |
| 117 | blitzoslib | InitPage FreePage AddBox … BlitzEvent SpotBox (31 tokens) | identical | none (original crashes in the test setup) | BLibJsr calls via inline asm (BCALL), DIVS with overflow |
| 116 | audiolib | DiskPlay DiskBuffer LoadSound Sound Volume Filter LoopSound InitSound SoundData PeekSound DecodeSound SetPeriod | identical | t110, t114, t115, t119 (DiskPlay not testable) | inline SoundData/PeekSound, `IFFCALL` |
| 96 | trackerlib | LoadModule PlayModule StopModule | identical | t111, t116, t117 | DBF wait loops, $A001 prefixes |
| 82 | medlib | LoadMedModule StartMedModule PlayMed StopMed JumpMed SetMedVolume GetMedVolume GetMedNote GetMedInstr SetMedMask DecodeMedModule | identical | t112, t118 | – |
| 88 | speaklib | Speak SetVoice Translate$ PhoneticSpeak VoiceLoc | identical | t113 (init errors only) | – |
| 140 | gadgetslib | TextGadget … SetTextGadget (GadgetList) | identical | t90, host comparison | – |
| 141 | gtlib (`#mygadtoolslib`) | GTButton … GTFreeGadget (binary state) | identical | host comparison | AddGList with position -1 as long |
| 80 | asllib (`#myasllib`) | ASLFileRequest$ ASLFontRequest ASLScreenRequest ASLPathRequest$ | identical | host comparison | – |
| 152 | freqlib | FileRequest$ | identical | host comparison | data block (bytes + relocs) of the requester |
| 160 | bitmaplib | BitMap LoadBitMap ScreensBitMap CopyBitMap ShapesBitMap CludgeBitMap | identical | t120, t124, t125 | – |
| 156 | palettelib | LoadPalette Cycle RGB Red Green Blue PalRGB SetCycle StopCycle AGARGB … MixPalette | identical (binary: Red/Green/Blue #long) | t121–t123, t126–t129 | Blitz/Amiga prefixes ($a001), RGB jumps directly into SetRGB4 |
| 157 | fadelib | FadeIn FadeOut ASyncFade DoFade FadeStatus | identical | t123 | – |
| 150 | blitlib | Blit BlitMode CookieMode EraseMode InvMode SolidMode Block BlitColl | identical | t130, t134–t136 | Blitz/Amiga prefixes, inline modes |
| 146 | bblitlib | Buffer BBlit UnBuffer BBlitMode ShowStencil FlushBuffer | identical | t131, t134, t137 | Blitz/Amiga prefixes, MULS/MULU inline |
| 148 | qblitlib | Queue QBlit UnQueue QBlitMode FlushQueue | identical | t132, t134, t138 | Blitz/Amiga prefixes, inline FlushQueue |
| 122 | stencillib | Stencil SBlit SBlitMode | identical | t133, t134, t139 | Blitz/Amiga prefixes |
| 154 | shapeslib | LoadShape MakeCookie ShapeWidth ShapeHeight Handle AutoCookie CopyShape GetaShape SaveShapes LoadShapes MidHandle DecodeShapes InitShape | identical | t140, t144, t145, t146, t149 | – (thunks; Blitz/Amiga prefix GetaShape) |
| 118 | shapetrixlib | Scale XFlip YFlip Rotate | identical | t141 (XFlip timing-dependent, see notes) | – |
| 112 | spriteslib | GetaSprite ShowSprite InFront InFrontF InFrontB SaveSprites LoadSprites SpriteMode | identical | t142, t147 | – |
| 104 | collslib | SetCollOdd SetColl SetCollHi ShapesHit ShapeSpriteHit SpritesHit RectsHit SColl PColl DoColl | identical | t143, t148 | – |
| 100 | scrolllib | Scroll Plot Point Cls BlockScroll | identical | t150, t159 | – |
| 119 | 2dlib (#sis2dlib) | Circle Circlef Box Boxf Line FreeFill AllocFill ReMap FloodFill Poly Polyf BitPlanesBitMap ClipBlit ClipBlitMode BitMapWindow BitMapOrigin | identical | t151–t155, t158 | yofill (FloodFill) as original assembler |
| 121 | polylib (#chunkylib) | ChunkyCLS ChunkyPlot ChunkyVlin ChunkyPoly BitMapPoly RenderToBitmap RenderToChunky RenderPoly | identical | t156, t157 | – |
| 190 | gameiolib | Joyx Joyy Joyr Joyb Gameb | identical | t160, t165 (runtime errors) | – |
| 98 | blitzkeyslib | BlitzKeys RawStatus BlitzRepeat BlitzQualifier HandleRawKey BlitzKey | identical | t161 | BRA head of the input handler |
| 94 | animlib | LoadAnim InitAnim NextFrame Frames | identical | t162, t168 | inline Frames; err1–7 stubs |
| 144 | blitzcoplib | Slice CopLoc CustomCop Show ShowF ShowB CopLen ColSplit ShowBlitz FreeSlices DisplayOff DisplayOn SetBPLCON0 SetBLWait (binary without Help) | identical | t163, t166 | $A001 heads |
| 143 | displaylib | InitCopList CreateDisplay DisplayBitMap … DisplayGenlock (24) | identical | t164, t167, t169 | error stubs, _checkcoplist |
| 33 | literallib | – (127 dummy tokens `$9081`-`$90FF`: the names of the literal characters, #1018) | new: not on the CD (also the userlib `literal/literallib.obj`) | `tokens_port`, `test_blitzlib`, tests/blitz2 `literal-stray` | the first byte `$90` (libraries 32 and 33) is reserved for it, decision of 2026-11-11 (`docs/token-table.md`) |
| 1 | dhandlerlib | – (init/finit: the debugger process, Ctrl-LAlt-C) | identical (lib number `LIB_dhandlerlib`) | `asmcmp_dhandlerlib`; the binary is the original's but the cache flush of #409 | everything but the header and token table (process segment template, relocation order) |

Findings and `BUG(orig)` peculiarities per library: [cmdlibs-port.md](cmdlibs-port.md).
All deviations of the C versions from the originals (DIFF, conventions,
compiler): [deviations.md](deviations.md).

Open: the remaining command libraries (deflibs/acidlibs). Integrating the
C libraries into a real compiler run (C port `programs/blitz2`) is done for the
test corpus (`tests/corpus/runner`, configuration clib).

## deflibs: AmigaOS calls

`blitz2:deflibs` (39 libraries with the declarations `Open_`, `AllocMem_` …,
numbers 2, 200, 219–255) exists as C source under `libs/deflibs/NAME.c`,
generated once by bb2 `tools/mkdeflibs.py`; since then the C files are the
source. Per call one line `BLITZ_OSCALL(baselib, lvo, "Name_", "(help)",
regs…)`, header `BLITZ_OSLIBHEADER`, init/finit in C (`libs/deflibs/deflib.h`,
including the `BUG(orig)` endless loop and the wrong name pointer of the
resources).

```
cmake --build --preset amiga --target deflibs   # -> build/amiga/blitz2/deflibs
ctest --preset emulator -R corpus               # configuration clib with C deflibs
```

All 39 declaration parts are identical to the original (`blitzlib compare`,
ctest `declarations_deflibs`), and so is the order. Details and test results:
[deflibs.md](deflibs.md).
