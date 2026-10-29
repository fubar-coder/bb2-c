# Reference build of TED 1.1 and the binary includes

## 1 Reference

**The reference for the port is the vasm build of the source `ted.s` (TED 1.1)
and its listing** – not the binary from the CD. The comparison with the
CD versions (TED, compiler, libraries) is done by a separate, later step;
it is explicitly not the subject here.

```
cmake --build --preset amiga --target ted_ref
→ build/amiga/programs/ted/ref/ted        vasm build (hunk executable, 57060 bytes)
→ build/amiga/programs/ted/ref/ted.lst    vasm listing (-Lall)
→ build/amiga/programs/ted/ref/ted.map    TSV: address  file:line  bytes  source (7280 lines)
→ build/amiga/programs/ted/ref/ted.labels TSV: address  label  file:line (1103 symbols)
```

Invocation as for the compiler (`blitz2_ref`, `build/amiga/programs/blitz2/ref/blitz2`):
`vasmm68k_mot -quiet -devpac -nocase -m68000 -Fhunkexe` (`-nocase` like
`GenAm.opts`), warnings 40/41/51/62 off as in the listing of blitz2-c (CMake target `blitz2_ref`).
What gets assembled is `tedt/ted.s` (here `archive/sources/ted-1.1/ted.s`): it differs from `ted.s` only in the
include paths (line 12 `e:myoffs`, line 136 `e:wbheader` – `e:` was a
Devpac assign); all includes and binary files in `tedt/` are equal to those
in `bb2src.zip.dir/` (`archive/aminet/amibl2src/bb2src/`), the line numbers are equal (listing line =
`ted.s:NNN`).

Result: one hunk (HUNK_CODE, section `CODE` 0–`$D4FA`, `$353F`
longwords), 1245 relocations (vasm writes them as `HUNK_RELOC32SHORT`),
no errors. Listing and binary match (`tedlisting.py verify`, here `tools/vasmlisting verify`:
7280 lines, 0 deviations; runs in the build step of `ted_ref`, bb2: `ctest` `ref_verify`).

## 2 Tools (bb2 `ted-c/tools/`; here `tools/vasmlisting` with `map`, `labels`, `verify`; the rest not taken over)

| File | Purpose |
|---|---|
| `tedlisting.py LST …` | `map`, `labels`, `addr LABEL`, `line ADDR`, `bytes A B`, `carray NAME A B` (constant `UWORD` array with `ted.s:NNN` comments, for tables/data), `verify EXE` (every listing byte against the code hunk of a binary, relocated longwords excepted) |
| `xrefs.py ORIGDIR` | cross-references between the chunks → [xrefs.md](xrefs.md) |

CMake: target `ted_ref` (bb2: `ref`; in `ALL`: vasm build with listing, then `ted.map`/`ted.labels` and `verify`; until 27.10.2026 `tools/mkref.sh`), `listing`; in bb2 `ctest` ran `ref_verify`.

## 3 The binary includes

| Label (line) | File | Bytes | Address | Kind | Decision |
|---|---|---|---|---|---|
| `sleep` (316) | `sleep.bin` | 704 | `$656` | data: 8 sprite images of the hourglass ("busy"), each 88 bytes = 2 control words + 20 lines × 2 words + end word | **data** – resource |
| `pointer` (319) | `pointer.bin` | 64 | `$916` | data: sprite image of the normal pointer (14 lines, 16 px, 2 bitplanes) | **data** – resource |
| `srcinfo` (827) | `src.info` | 396 | `$1074` | data: `DiskObject` (magic `$E310`, version 1) – icon written as `NAME.info` when saving | **data** – resource |
| `rawed` (7109) | `rawed.obj` | 660 | `$73E0` | **code**: complete AmigaDOS hunk file (header 32 bytes, 624 bytes code, no relocations), entry points `rawed+32+0/4/8` | **port** (C, `rawkey.c`) |
| `filerequest` (7967) | `filereq` | 15916 | `$8CE0` | **code**: complete hunk file (14616 bytes code+data, 312 RELOC32), relocated at runtime by `flink` (7953) itself, entry point `filerequest+36` = code+4 | **port** by taking over `libs/acidlibs/freqlib.c` (see 4.2) |
| `font6` (8070) | `chars.b` | 768 | `$CDFA` | data: font 8 pixels wide, 6 high, 128 characters in 8 blocks of 16 (line r of character c at `(c&15) + (c>>4)*96 + r*16`, `fontcalc` 5214) | **data** – resource |
| `font8` (8071) | `chars8.b` | 1024 | `$D0FA` | data: font 8×8 pixels, 128 characters (`(c&15) + (c>>4)*128 + r*16`) | **data** – resource |

The pure data stays binary: it is converted into C arrays from the original files
at build time (PLAN.md §3.4), not typed in and not placed into the
repository as a copy. In TED, sprite data is **not** in chip RAM:
`mouseinit` (184) copies `pointer … finish` into an `AllocMem(MEMF_CHIP)`
block; this stays so (C: normal `const` array + the same copy).

### 3.1 `rawed.obj` – rawkey → ASCII

No source available: neither in the Aminet archive (`archive/aminet/`) nor on the CD is there
`rawed.s`/`rawmaker` (`ted.s:7107` refers, commented out, to
`rad:rawmaker.obj`); `archive/aminet/amibl2src/bb2src/rawed.obj.dsm` is a
DSM disassembly (1988 disassembler, so already present at the author's).
The same conversion routine (byte sequence from code+`$10A`) is also in
`BLITZ2/ACIDLIBS`, `DEFAULTDBUG`, `HELP` of the CD and several PD programs –
apparently a standard routine of the authors; a source for it is not present
in `bblibsrc`.

Structure (offsets relative to `rawed+32`):

| Off. | Contents |
|---|---|
| `$000` | `bra $174` – **init** (`ted.s:39 bsr rawed+32`) |
| `$004` | `bra $10A` – **convert** (`ted.s:5928 bsr rawed+32+4`) |
| `$008` | `bra $20A` – **finit** (`ted.s:128 fail5 bsr rawed+32+8`) |
| `$00C` | `"console.device",0` |
| `$01C` | `ConsoleDevice` base (l) · `$020` output buffer (w) · `$022` `InputEvent` (22 bytes, `ie_Class` = 1 = RAWKEY) |
| `$038`/`$0A1` | 2 tables of 105 bytes each (rawkey `$00–$68` without/with shift) |
| `$10A` | convert: in d0.w = rawkey, d1.w = qualifier; `$50–$59` (F1–F10) → d0+`$31` = `$81–$8A`; `$4C–$4F` (cursor) → d0−`$30` = `$1C–$1F`; `$5F` (Help) → `$8B`; otherwise `RawKeyConvert(ie, buf, 1, NULL)` (console −48), returns the one character, 0 on −1 (overflow) or without character. out: d0.w = d1.w = character; a0–a2/a6 saved |
| `$174` | init: clear 82 bytes from `$218`, `IOStdReq` (`$218`, `mn_Length` 48) + `MsgPort` (`$248`, `mp_SigTask` = `FindTask(0)`, `mp_SigBit` = `AllocSignal(-1)`), `OpenDevice("console.device", -1, io, 0)` (result not checked), base = `io_Device`; then fill both tables via convert |
| `$20A` | finit: `CloseDevice(io)` |
| `$26A` | `moveq #0,d0 / rts / $000F` – leftover (same leftover as at the end of `filereq`: random contents of the assembler buffer) |

Peculiarities (later `BUG(orig)`): the two tables are built,
but never read (neither in rawed nor in `ted.s`, which only uses `+32/+36/+40`
); the signal bit is never freed; F keys/cursor/Help ignore
the qualifier (shift/alt come via `qualifier`, which `ted.s` evaluates
itself); a failure of `OpenDevice` is not detected (then
convert calls a null base).

**Decision: port.** 624 bytes of pure Exec/console code without
tricks, only PC-relative data; directly reproducible in C with `OpenDevice`/`RawKeyConvert`
(`<devices/conunit.h>`, `proto/console.h`). Asm would
only preserve a register interface (d0/d1→d0) that is exclusively
TED-internal. The tables are built as well (same side effects:
105×2 `RawKeyConvert` calls), but marked as unused.

### 3.2 `filereq` – file requester

No source (`ted.s:7969 ;include rad:filerequester`), **but the code
is the same as in the file requester of Blitz's `freqlib`** (`FileRequest$`,
`bblibsrc/freqlib` includes `libincs:filereq.s`): code+data of `filereq`
(14616 bytes) match `BLITZ2/ACIDLIBS` of the CD from offset `$114FA` except for
7 bytes, all in padding bytes behind gadget texts (`"OK",0,$06` instead of
`"OK",0,$00` etc.) or in the leftover at the hunk end. This requester is
already translated to C in `libs/acidlibs/freqlib.c` (from the disassembly,
with all BUG(orig) notes, among others writing past the data block with
windows from 605 pixels height – this affects TED on tall screens just the same).

Integration into TED:

* `flink` (7953, once at startup from `outside`, line 76) relocates the
  embedded hunk **itself**: `a0 = filerequest+28` (hunk length), base =
  code start (`filerequest+32`), then the RELOC32 list after the code:
  `add.l basis,(code+offset)` for all 312 entries. This is
  self-modifying code in the code hunk of TED (only once, before any
  use; not reentrant, a second call would relocate
  twice).
* `ezfreq` (3502, also `comdata+24` for blitz2): `reqstruct` = {a0 path, a1
  name, a2 title, screen} (the fourth longword is set by `screenopen`, ted.s:4675 – the requester opens on TED's screen), `a0 = reqstruct`, `bra filerequest+36` (= code+4,
  skips the `lea default(pc),a0` at the code start). The same entry point
  and structure format is used by `freqlib` (`bsr freqcode+4`).

**Decision: port by reuse ("variant C", user,
implemented in phase 2a).** One source for both programs, translated by each with its
own options (`shared/filereq/filereq_core.c`, a separate translation
unit):

* `shared/filereq/filereq_core.c` – the requester core: code, data block
  `fq` (`struct fq_data`: NewWindow, gadgets, borders, IntuiTexts,
  StringInfo/PropInfo/images as NDK structures, variables and buffers in the
  layout of the freqlib, every offset checked via `static_assert`; pointers as
  static initialisers `&fq.…` = the same relocs). Interface in
  `filereq_core.h`: parameter block `ULONG fq_args[4]` = {path, name, title,
  screen} (= TED's `reqstruct`), entry point `ULONG fq_request(void)`; everything
  else `static`. Every program supplies `filereq_env.h`
  (exec declarations, `fq_divu_w` with DIVU.w semantics). Translated twice,
  because the options are incompatible: in the library `-ffixed-a3/a4/a5`,
  freestanding, one hunk; in TED `-fbaserel` (a4 = base register of the
  globals) and libnix.
* `libs/acidlibs/freqlib.c` – only the BlitzLib shell (`dofreq`/`freqchk`);
  `libs/acidlibs/filereq_env.h`: `blitzlib.h`, `blitzexec.h`,
  `fq_divu_w` = `bl_divu_w`. The declarations are identical to the original
  (`tools/blitzlibcompare`, bb2: `biglib.py compare ref/freqlib.obj …`). With `struct fq_data` instead of the
  assembler block, the data block (bytes + relocs) has stayed the same,
  but the code is shorter: for GCC the byte array `fq_data[]` had
  alignment 1, every word/longword access was assembled from single bytes
  (freqlib $4A00 → $3F20, TED's filereq code $1600 → $CA4
  bytes; host comparison in unicorn: 27 scenarios equal).
* `programs/ted/src/filereq.c` – `filerequest(reqstruct)` copies the block to
  `fq_args` and calls `fq_request()`; `ezfreq` (reqs.c) maps the
  register interface onto it (comdata+24: d0, Z). `programs/ted/src/filereq_env.h`:
  `<proto/exec.h>`, `fq_divu_w` = `divu_w` (glue.c; none of the three
  divisions can overflow).

`flink` is dropped (`DIFF:`, [deviations.md](deviations.md)). The
data block is that of the freqlib: the 7 differing padding bytes of TED's copy
are not taken over (never read; likewise in deviations.md). The
known overflow with windows from 605 pixels height writes in TED beyond
`fq.line[63]` past `fq` into the following data of the data hunk
(BUG(orig), as in freqlib). The former option `TED_FILEREQ_BLOB` (original hunk via
`.incbin`) is not built.

### 3.3 Other TED versions on the CD (not reference)

`INTERNET/NEWTED/TED`, `BUM10A/TED223/TED`, `BUI5/SUPERTED/TED`,
`BSS3/SUPERTED/PROGRAMS/TED`, `EXTRAS/NEWTEDITOR/TED`,
`BUM9A/BUM9EXAMPLES2/HARDDISKUSERS/TED` (on the CD) are other (later) editors.
`archive/aminet/amibl2src/amiblitz2source/ted2.bb2` is the AmiBlitz2 editor (Blitz source with
asm, StormWizard interface) and not a translation of `ted.s`
(no common labels like `tokit`, `comdata`); unsuitable as a reference for the
behaviour of TED 1.1.
