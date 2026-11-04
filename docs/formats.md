# Binary formats of Blitz Basic 2.1

> As of 26.10.2026. Basis: copy of the Blitz 2.1 CD under `archive/cd/ultimate-blitz-basic-2.1/`
> (4310 files; paths below in lower case). Goal: settle open questions for the planned converter
> `.bb2` → `.ab3` (cf. `ab3c/docs/bb2-converter/bb2-format.md`,
> `bb2-corpus.md`).
>
> Evidence: **[B]** binary analysis of the CD files (scripts below),
> **[S]** assembler source of Blitz 2 v1.95 and TED 1.1
> (`ab3c/tests/external/aminet/amibl2src/files/amiblitz2source/bb2src.zip.dir/blitz2.s`,
> `ted.s`, `options.src`), **[C]** AB3 source
> (`AmiBlitz3/Sourcecodes/Amiblitz3/Compiler/AmiBlitz3.ab3`). None of this
> is verified in Amiberry. CD programs were not run.

Tools (Python, call with `-I`) in bb2 `tools/` (bb2 only, not taken over):

| Script | Purpose |
|---|---|
| `hunk.py` | minimal hunk reader (Code/Data/BSS, Reloc32) |
| `xtra_dump.py [--stats] <pfad>…` | decompose `.xtra`, or statistics over all |
| `res_dump.py [-s] <datei.res>…` | types, macros, constants of a `.res` |
| `src_scan.py <builtin_tokens.txt> <wurzel> [--list]` | classify sources and check them byte by byte |
| `bb2_tokens.py <blitz2> [out] [-v]` | extract the built-in keywords from the compiler |

An important finding up front: `blitz2.s` (v1.95) and `ted.s` (TED 1.1) from
`amibl2src` match the CD. TED 1.1 announces itself with the same text
"Blitz Basic 2 - Ted Version 1.1" as `BLITZ2/TED` of the CD. The
option table, the token list and the umlaut table in compiler 2.10
have exactly the structure from `blitz2.s`. The sources therefore interpret the binary data
reliably.

---

## 1 `.xtra` companion file

### 1.1 Structure

**[S]** `loadxtra`/`savextra` and the table `optspnts` (blitz2.s
lines 15288–15450) read and write blocks: a word gives the size,
followed by the target pointers up to 0. **[B]** In compiler 2.10 the same
table is at code offset `$F936` (`optspnts`): block 1 with 2 pointers, block 2 with **14**,
block 4 with 6, `$220` with 1, `$180` with 1, then end. This gives
this layout (big endian):

| Offset | Length | Contents |
|---|---|---|
| 0 (`$000`) | 1 | `vers`: 0 = old (without CLI block), 1 = with CLI block. `savextra` always writes 1. |
| 1 (`$001`) | 1 | Executables with icon (low byte of the gadget flags, `$80` = on) |
| 2 (`$002`) | 14 × 2 | Switches, each the gadget flags (`$0080` = GFLG_SELECTED = on, `$0000` = off), in this order: |
| | | +2 Runtime Errors (error debugger), +4 Debugger, +6 Long Arrays, +8 Expand Macros, +10 Quoted Macros, +12 Make Smallest Code, +14 Debug Info for executables, +16 Auto Run, +18 Interrupt Checking, +20 Assembler Checking, +22 Overflow Checking, +24 `optreq2ga5` (meaning open, always `$80`), +26 default Interrupt Checking, +28 default Overflow Checking |
| 30 (`$01E`) | 6 × 4 | Buffer sizes in bytes: Object, Libs, System, Data, Macro, String (long). Defaults in `options.src`: 16384, 16384/32768, 4096, 4096, 8192, 10240 |
| 54 (`$036`) | 8 × 68 | Residents: each 64 bytes path (0-terminated) and 4 bytes pointer (runtime value, cleared on load: `clr.l 64(a0)`) |
| 598 (`$256`) | 192 | Path of the executable (0-terminated) |
| 790 (`$316`) | 192 | Name of the executable (0-terminated; e.g. `c:makedeflibs`, `nil:`) |
| 982 (`$3D6`) | 128 | only if `vers` ≠ 0: CLI argument (0-terminated) |
| 982 or 1110 (`$456`) | n × 4 | up to end of file: pairs `lib.w`, `max.w` (object maximum per lib), in the order of the loaded libs, **without** end marker |

Fixed part: 982 bytes (vers 0) or 1110 bytes (vers 1). If `vers` = 0,
`loadxtra` sets defaults: Auto Run on, Interrupt/Overflow as per default,
Assembler Checking off.

### 1.2 Check over all 575 `.xtra` of the CD **[B]**

`xtra_dump.py --stats cd`:

* Sizes 1054–1250 bytes. All are ≡ 2 (mod 4), so they fit without remainder
  982/1110 + 4n. No file leaves a remainder.
* `vers` = 1 in 571 files, `vers` = 0 in 4 (size 1054/1066:
  `examples/blitzmode/blits.xtra`, `amigamode/medtest.bb.xtra`,
  `arexx/roger2/3.rexx.xtra`).
* All switches are only `$0000` or `$0080`, the icon byte only `$00`/`$80`.
* Residents at offset 54 occupied (byte-exact, e.g. `blitzlibs:amigalibs.res`
  in `bss2/reddebugger/examples/tokenlist.bb.xtra`). Names occurring:
  `blitzlibs:amigalibs.res` (35), `blitzlibs:libmacs.res` (30),
  `blitzlibs:blitzoffs.res` (15), `blitzlibs:bb2objtypes.res` (8),
  `libincs:…` (10), `blitz2:residents/arexx.res` (2), `pgc.res` (2) etc.
  104 files have a value ≠ 0 in the pointer field, i.e. memory garbage.
* CLI argument set in 15 files (`'setpa c:'`, `'asl.h'`, `'commands'`
  …). Bytes 116–127 of the CLI block are 0 in **all** files.
* Pairs: lib numbers 4–201, no lib 0, unsorted (lib chain order).
  Outlier: `internet/erase/erase_v1.0.bb2.xtra` with `63337:32782`
  (`$F769`). This is probably a third-party lib or garbage. AB3 skips
  unknown libs (`findlibnoerr`, `a1` = 0 → continue).
* Buffers: 225 files with the defaults 16384/32768/4096/4096/8192/10240.
  The others have different values (e.g. Object 346, System 24, Data 0).
  Whether 0 means "default" is open.
* 15 `.xtra` have no source file next to them (e.g. `extras/libsdev/acidlibsrc/*.xtra`).

**Confidence: high.** The structure matches byte for byte in all 575 files.

### 1.3 Comparison with AB3 `read_xtrafile` **[C]**

| Point | Blitz 2.1 | AB3 |
|---|---|---|
| 1-byte block | vers, icon | same (`xtra_versionnumber`, `set_create_iconfile`) |
| 2-byte block | **14** entries | up to commit `bb485acf` (05.01.2026) **14**; since `bdae3f9e` (08.01.2026, "changed comdata structure … reorganized settings for debugger") **15**. According to comments, a `default_obsolete` ("ohne inhalt") was inserted after `set_use_debugger`; the former slot "quoted macros" is now `set_FuncOptimize`, and `set_create_dbgfile` was added at the end (checked with `git show bdae3f9e^`). From the 2nd switch on, everything is shifted by 2 bytes. |
| Buffers, residents, exe path | 24 + 544 + 384 | same |
| CLI block (vers ≠ 0) | 128 bytes text | 116 text + stack.l + version info.l + number of builds.l; the stack is raised to `MIN_STACKSIZE` |
| Pairs | up to end of file | up to end of file **or** lib 0 (`;new`). After that, `LoadXtra` (CLIModeSupport.ab3 line 364) reads the PED block `XtraExtend`. When writing, AB3 appends 4 padding bytes. |

Consequences for the converter and for AB3:

* **Shift by 2 bytes:** the current AB3 wrongly reads a
  Blitz 2 `.xtra` from offset 32 instead of 30. The buffers are then
  scrambled, and the resident names lose their first two characters
  (`blitzlibs:…` → `itzlibs:…`). Even an AB3 file with 14 entries
  (`AmiBlitz3-Original/…/Tests/68k_crash_newtype.xtra`, 1296 bytes) still
  fits the 14-entry layout. This is a candidate for chapter 11 and must be
  checked in Amiberry.
* The 12 bytes stack/version/builds are 0 in all CD files. AB3
  then sets the stack to the minimum. This is harmless.
* For the `.ab3` header lines the converter can take over the values directly:
  switches, buffers, residents, exe name, CLI argument. Not the maxima:
  the header names objects (`Max Screen`), the `.xtra` numbers libraries
  (§7.6).

### 1.4 One structure: `shared/xtra`

`shared/xtra/xtra.h` holds a source's companion settings in one small
structure, `struct xtra`, whatever they came from: a Blitz 2 `.xtra`
(this chapter), the header of a `BB22` source (`docs/rwelibs/README.md`
§4.1) or AmiBlitz3's embedded header (§7). Shared code like
`shared/filereq`: every user compiles `xtra.c` and `xtra_ab3.c` itself
– now the host tools (`tools/lib/xtra`, the tool `tools/xtra`), later
blitz2 and TED (decision of 2026-10-30). Hence pure C99 on the
freestanding headers: no libc, no 32-bit multiply or divide (no libgcc on
the 68000), no writable static data, no allocation, byte order handled
explicitly. The build compiles it for the m68k with the options of
blitz2 and TED (`-fbaserel`, object library `xtra_m68k`, C99, no
extensions); no product links it yet. Design of 2026-10-30 (rework):

* **The caller does the I/O.** It loads the source (tokenised or text)
  into memory; `xtra_detect(buf, len, path)` gives the type: `"BB22"` at
  the start → `XTRA_T_BB22`; a `$00` byte → `XTRA_T_TED` (§3.2); first
  line `; XTRA` → `XTRA_T_AB3`; other text → `XTRA_T_AB3` if the name
  ends in `.ab3` (AmiBlitz3 reads every `.ab3` so), else `XTRA_T_TEXT`.
  The caller may use another type. For TED, `BB22` and plain text the
  settings are in the companion `NAME.xtra` (`xtra_has_companion`,
  `xtra_companion_name`: path + `.xtra`, as TED appends it, §3.3); the
  caller reads it too (if it exists) and passes both buffers to
  `xtra_load(x, type, src, len, xtra, xlen, err)`.
* **No copies.** The texts (executable path and name, CLI argument,
  `LabelSearch`, the 8 residents) are views: `{in, off, len}` into the
  `.xtra` buffer (`XTRA_IN_XTRA`, `off` = the field's offset in the
  file) or into the source header (`XTRA_IN_HEADER`). Offsets, not
  pointers: the caller may move the bytes (e.g. `realloc` after
  tokenising) and passes their current place as `struct xtra_base
  {xtra, header}` to `xtra_text`, the maxima iterator and the writers.
  The numbers (icon, switches, buffers, resident pointers, editor part,
  AmiBlitz3's numbers) are parsed into members.
* **The header to keep.** `x->hdr` is the size of the source header:
  `BB22` 14/16 bytes, AmiBlitz3 the lines from `; XTRA` to `; /XTRA`
  (with its line end), 0 otherwise. The caller tokenises the rest and
  keeps `src[0..hdr)` (in front of the text or elsewhere).
* **Maxima** through an iterator (`xtra_max_begin`, `xtra_next_max`)
  over the `{lib, max}` pairs of the `.xtra` (`XTRA_MAX_LIB`) or the
  `Max NAME` lines of the header (`XTRA_MAX_NAME`, the name as written:
  a blank is `-`); no fixed table, no upper limit. Mapping names to
  library numbers needs the libraries and is the caller's business.
* **Changes.** The numbers are set directly; a text with
  `xtra_set_text(x, field, p, len)` (the caller's memory, kept by the
  caller; `XTRA_IN_SET`; refused: longer than the `.xtra` field, a `0`,
  CR or LF), the maxima with `xtra_set_maxima(x, array, n)`.
  `xtra_take_blitz2(to, from)` takes what a `.xtra` holds (not the
  maxima) into another structure, e.g. to update an `.ab3` header from
  a `.xtra`.
* **Size.** `struct xtra` is 300 bytes on the m68k (368 on a 64-bit
  host); before the rework it was 10 936 bytes (a fixed table of 256
  maxima with a 32-byte name each, texts in fixed arrays).

| Field | Contents | Blitz 2 `.xtra` | `BB22` | AB3 header |
|---|---|---|---|---|
| `type` | `XTRA_T_*` as loaded | – | – | – |
| `source` | where the settings came from (`XTRA_SRC_*`) | `BLITZ2` | – | `AB3_TEXT` |
| `hdr` | bytes of the source header to keep | – | 14/16 | `; XTRA`…`; /XTRA` |
| `vers` | 0/1 | offset 0 | – | (1) |
| `icon` | `XTRA_ON` = on | offset 1 | – | `CreateIcon` |
| `sw[14]` | switch words, file order (`XTRA_SW_*`) | offset 2 | – | 7 of them (§7.2) |
| `buf[6]` | buffer sizes (`XTRA_BUF_*`) | offset 30 | – | `StringBuffer` |
| `res_ptr[8]` | the residents' pointer fields | offset 54 + 68i + 64 | – | – |
| `text[XTRA_F_RES0+i]` | resident paths (< 64) | offset 54 + 68i | – | `Residents` |
| `text[XTRA_F_EXE_PATH/NAME]` | < 192 each | offset 598, 790 | – | `ExePath`, `ExeFile` |
| `text[XTRA_F_CLI]` | < 128 | offset 982 (vers 1) | – | `CliArgs` |
| `text[XTRA_F_LABEL_SEARCH]` | < 32 | – | – | `LabelSearch` |
| `max` | the maxima (iterator) | pairs: `lib` | – | `Max NAME`: name |
| `editor` | cursor line/column, top line (0-based, flags) | – | words +6/+8/+12 | `CursorLine`, `CursorColumn` |
| `ab3` | AmiBlitz3's own numbers, `seen` = keys present | – | – | the rest (§7.2) |

* **Byte-exact round trip.** `xtra_write_blitz2` copies a text from the
  `.xtra` as its whole field (bytes after the 0 included) and writes
  the switch words and the icon byte as they are (not just on/off), the
  pairs in file order, and the **pointer field of each resident**: a
  runtime value (blitz2's `loadxtra` clears it, `savextra` writes
  whatever is in memory – 104 CD files have garbage there, §1.2), kept
  in `res_ptr[]`; a structure built from scratch has 0 there. A text
  from elsewhere (header, caller) is written 0-terminated and padded with
  0. `vers` is written as it is (0: without CLI block). All 575 `.xtra`
  of the CD and the 124 of `rwelibsrc/` load without error and give the
  same bytes again (ctest `xtra_originals`).
* **Strict loader.** Refused: shorter than the fixed part, size not
  982/1110 + 4n, `vers` other than 0/1, a text field (residents, paths,
  CLI argument) without its 0 byte – each with code, file and offset
  (`struct xtra_error`, `xtra_strerror`). blitz2's own loader is more
  tolerant (it reads what is there and stops at end of file or at the
  first unknown library); the CD has none of these cases.
* **vers 0.** The loader leaves the file values; `xtra_apply_vers0` does
  what `loadxtra` does afterwards (Auto Run on, Interrupt/Overflow
  Checking from their defaults, Assembler Checking off).
* **`BB22`**: the loader reads the header (14/16 bytes, the rules of
  `tools/lib/bb22`) into `editor` (line = top + row, column, top) and
  the companion `.xtra`, a normal Blitz 2 `.xtra`
  (`xtra dump --source SRC`, `xtra dump FILE.xtra --bb22 SRC`).
* **Defaults** (`xtra_init`): the initial flags of the option gadgets and
  the `LongInt` of the buffer gadgets (`options.src`, C port
  `programs/blitz2/src/data.c`): icon, Runtime Errors, Debugger, Long
  Arrays, Expand Macros, Auto Run, Interrupt Checking, `optreq2ga5` and
  both defaults on; buffers 16384, 32768, 4096, 4096, 8192, 10240; vers 1;
  AmiBlitz3's own numbers as its `ParseAB3Header` sets them (stack 32768).
* **AmiBlitz3's binary `.xtra`** (15 switch words since `bdae3f9e`,
  §1.3) is only **recognised**: `xtra_is_ab3_xtra` (for the mode of a
  tokenised source in `blitz2-ab3`, `docs/ab3/variant.md`). The layout as
  `write_xtrafile` of v3.14.0 (`58a11658`) writes it: vers 1 (always),
  icon, 15 words at 2, buffers at 32, residents at 56, executable path
  and name at 600 and 792, CLI argument (116 bytes) at 984, stack,
  version, builds at 1100/1104/1108, the pairs from 1112, then 4 more
  bytes (the long behind `XTRAEndPadBytes`, `maxerrorcount` – not
  necessarily 0) and, if PED saved the file, its `XtraExtend` (44
  bytes). Recognised by: at least 1116 bytes, a multiple of 4 (Blitz 2:
  982/1110 + 4n, i.e. 2 more), vers 1, icon and the 15 words `$0000` or
  `$0080`, every buffer below 16 MB (in the 14-word layout's place the
  low word of a buffer would be the high word of the next, 16384 →
  `$40000000`), the text fields 0-terminated. An older AmiBlitz3 `.xtra`
  with 14 words (e.g. the 1296 bytes of `68k_crash_newtype.xtra`) is not
  recognised (host test `xtra_ab3_binary`). It is not loaded:
  `blitz2-ab3` takes only the mode from it and ignores its options
  (`ab3cli` prints a note, `docs/ab3/variant.md`), because AmiBlitz3
  drops loading tokenised sources – the only sources with such a
  companion – and no loader is written for it.

---

## 2 `.res` (resident files)

### 2.1 Format **[S]** **[B]**

Blitz 2.1 has the same format as AB3 (`blitz2.s` `menu5`, `loadres`).
`menu5` ("make resident", blitz2.s line 4105 ff.) writes a hunk file:
`HUNK_HEADER` with 1 hunk, `HUNK_CODE`, optionally `HUNK_RELOC32` (hunk 0)
and `HUNK_END`. Padding is done with `'ZAP',0` to a longword. The code hunk
contains, one after the other:

1. **Types** (`restypes`): `count.w`. Per type first the field records (`next.l`,
   +4 offset, +10 type number.w, +14 length.b, +15 name), then the type record
   (+4 first field, +10 length.b, +11 name). Type numbers ≥ 7 are shifted
   by `rescnt` on load, as in AB3 (`fixtypes`).
2. **Macros** (`resmacs`): `count.w`, records `next.l`, `body.l`, `len.w`,
   `count.w`, `nameend.b`, name, body (tokenised, statements joined
   with `:`).
3. **Constants** (`resconsts`): `count.w`, label records with `mode.l` = 1,
   `value.l` at +8, length.b at +$12 and name at +$13.

`loadres`/`fixtypes`/`fixmacs`/`fixconsts`/`checkclash` are, instruction for
instruction, the model for the AB3 routines of the same name. **Differences: none
found.** The token words in macro bodies use the
Blitz 2.1 numbers (e.g. `BlitWait`: `$8057` = `BTST`, `$8050` = `BNE`).
So for lib tokens the same mapping problem applies as for sources.

### 2.2 Contents of the 18 `.res` on the CD **[B]**

`res_dump.py` reads all of them up to the end of the hunk (at most 2 padding bytes left).

| File | Types | Macros | Constants | Examples |
|---|---|---|---|---|
| `extras/blitzlibs/amigalibs.res` (also `bum9b`, `bum10a`) | 359 | 98 | 4551 | types `Node`, `List`, `Resident`; macros `IsListEmpty`, `OFFSET`, `INITBYTE`, `BADDR`; `#INCLUDE_VERSION=$28`, `#NT_TASK=1` |
| `extras/blitzlibs/bb2objtypes.res` (also bum9b/10a) | 23 | 0 | 0 | `rgbcomp`, `palette`, `bitmap`, `shape`, `slice`, `window`, `screen` … |
| `extras/blitzlibs/blitzoffs.res` (also bum9b/10a) | 0 | 0 | 313 | `#_OpenIntuition=-30`, `#_AddGadget=-42` … |
| `extras/blitzlibs/copmacs.res` | 0 | 9 | 0 | `BlitWait`, `DataBlock`, `BitMaps`, `AmigaBitMaps`, `StenBitMaps`, `SetSprite0/1`, `SetJmp0/1` |
| `extras/blitzlibs/libmacs.res` | 0 | 23 | 242 | macros `pchk`, `echk`, `libheader`, `astatement`, `afunction`, `args`, `subs`, `name`, `copwait`; `#wordret=-1`, `#long=3`, `#ld0`…`#la6`, `#ud0`… |
| `bum9b/…/libmacs.res` / `bum10a/…/libmacs.res` | 0 | 23 | 249 / 252 | later versions |
| `extras/blitzlibs/libnums.res` | 0 | 23 | 240 | **contains libmacs content**, not the lib numbers (error on the CD) |
| `bum9b/…/libnums.res`, `bum10a/…/libnums.res` | 0 | 0 | 164 / 167 | `#varslib=$FFFE`, `#wbstartuplib=$FFFD`, `#memlib=$FFFA` … |
| `bum10a/blitzlibs/asmmacs.res` | 0 | 12 | 0 | `SBRA`, `SBCC` … `SBLE`, `LEASCALE` |
| `bui8/pgc/pgc/pgc.res` | 3 | 0 | 35 | types `main`, `baddytype`, `baddy`; `#_BLOCKWIDTH=$A` |

**Confidence: high** for the format; that AB3 behaves the same when loading
is only inferred from the code [C].

---

## 3 Sources `.bb2`/`.bb`

### 3.1 Inventory **[B]**

`src_scan.py`: checked were all `*.bb2`/`*.bb` and all files with
`.xtra` next to them, 734 in total.

| Kind | Count |
|---|---|
| tokenised `.bb2` | 606 |
| tokenised `.bb` | 109 |
| tokenised, other extension (`.1`, `.include`, `.h`, `.i` ×2, `.src`, none) | 7 |
| **Text** (LF, no `$00`) `.bb2` | 9, all in `internet/tritonblitz/` (`triton.bb2`, `memo.bb2` …) |
| Text, not Blitz (`.rexx` ×2, `startup-sequence`, with `.xtra` next to them) | 3 |

The 722 tokenised files have 122 837 lines and 2.78 MB.

### 3.2 Structure **[B]** confirmed

* **No header.** No file begins with `BB22` or any other
  identifier. 487 files begin with the line `;` (`3B 00`), 38 with an
  empty line (`00`). The rest begins directly with code or a comment.
* **All 722 end with `$00`.** Lines are bytes up to `$00`. The longest
  line has **128 bytes** (`bui7/marbles/source/the_game10.bb2`); TED
  limits the line to its buffer.
* **Token words:** outside of strings, every byte ≥ `$80` starts a
  word. 82 348 token words are in code, 179 in comments. All
  built-in words (< `$8100`) are in the list from section 5; there is
  no unknown one. Lib tokens come from 100 libs. The most frequent are
  lib 138 (2601), 187 (2241), 120 (1803) and **185 (1670)** as well as 186 (305).
  AB3 remaps these lib numbers via `replacedat` (bb2-corpus.md §5).
* **No tabs, CR, LF** and **no bytes ≥ `$80` in strings**. TED
  converts tabs to spaces on load (see 4.2).
* **Umlaut bytes `$18`–`$1F`: occur nowhere on the CD**, neither in
  strings nor in comments. The mapping is only attested by TED and the
  compiler (section 4).
* **Foreign control bytes:** exactly 5 × `$01` outside of strings:
  `extras/libsdev/userlibsrc/elmorehardwarelib.bb` lines 96/97 (line consisting only
  of `$01`) and `ripacklib.bb` lines 367/368 in labels `no_\x01ber`,
  `get_\x01ber` (presumably "über" from another editor). In the compiler
  `$01` is the end character of a `~…~` insertion (`getchar`, blitz2.s),
  `$02` the USEPATH return.
* `$807F` (`MOVEP`) and `$80AD` (`TAS`) do not occur. This is important
  because AB3 has `Chip`/`BSS` there (5.2).

### 3.3 `.bb` versus `.bb2`

The format is the same; the extension has no meaning. `.bb` is used
mostly for lib sources (`extras/libsdev/userlibsrc` 25,
`extras/blitzlibs/*.bb`) and older examples. TED 2.1 (`extras/newteditor`)
uses the file pattern `(#?.bb|#?.bb2)`, TED 1.1 and the compiler do not check the
extension at all. `.xtra` is appended to the full name (`x.bb.xtra`).
**Confidence: high.**

---

## 4 Editor TED

### 4.1 Umlaut table **[B]** **[S]**

`BLITZ2/TED` of the CD (TED 1.1), file offset `$5DCE`, label `germs` in ted.s
line 6022:

```
E4 1A  F6 1B  FC 1C  C4 1D  D6 1E  DC 1F
ä→$1A  ö→$1B  ü→$1C  Ä→$1D  Ö→$1E  Ü→$1F
```

The keyboard handler (ted.s line 5976) replaces the Latin-1 character of a
key with the control byte. The same table (six pairs, **without** ß/©)
is in all TEDs of the CD:

| File | Version | Offset |
|---|---|---|
| `blitz2/ted`, `bum9a/blitz2/ted`, `bum10b/blitz2/ted` | Ted Version 1.1 | `$5DCE` |
| `extras/newteditor/ted` | TED V2.1 (01.11.1995) | `$77A8` |
| `internet/newted/ted` | TED V2.1 (27.10.1995) | `$7770` |
| `bss3/superted/programs/ted`, `bum10a/ted223/ted` | SuperTED 2.50 / 2.23 | `$84BE` / `$7E34` |

`ß`→`$19` and `©`→`$18` (bb2-corpus.md §3.2) are only added in the later
`ted2.bb2` (AmiBlitz 2).

**Compiler 2.10:** the table `gs` (`ds.b 26` followed by
`E4 F6 FC C4 D6 DC`; `gs` at code offset `$7BEE`, the six bytes at `$7C08`) maps `$1A`–`$1F` to
**ä ö ü Ä Ö Ü**, so it matches TED. According to blitz2.s (`getchar3`) this applies
to every character < 32 **in quotes**: `$1A`–`$1F` become
Latin-1, `$00`–`$19` become 0. So Blitz 2.1 most likely output umlauts in string literals
as Latin-1. AB3's `gs` (`F6 E4 FC D6 C4 DC`,
ä/ö swapped) is therefore a **later deviation** and not inherited from
Blitz 2.1. For the converter this means: in strings and comments
`$1A`–`$1F` → ä ö ü Ä Ö Ü (Latin-1 `E4 F6 FC C4 D6 DC`).

### 4.2 Loading and saving **[S]**, TED 1.1

* **Loading** (`fileopen`, `readline` ted.s line 3375): a byte with
  bit 7 set takes the following byte along unchecked (token). A line ends at
  `$00` **or at LF**. If there is an LF, the file counts as ASCII; the line is
  tokenised if the option "Tokenise" is on. Tab becomes spaces
  up to the next tab stop. CR is not treated specially. Lines that are too long
  are cut off at the end of the buffer. After that TED loads `<name>.xtra`
  via the compiler routine `loadxtra` (`edstruct`).
* **Saving** (`filesave`): first the old file is renamed to
  `<name>.bak`. Then every line is written with its terminating byte:
  `$00`, or **LF if "Tokenise" is off**. Then follows `<name>.xtra`
  (`savextra`) and on request `<name>.info`. TED 2.1 additionally has "Save
  ASCII…". So text `.bb2` files are a regular TED format.
* **`BB22`:** occurs in no program of the CD, neither in TED, SuperTED,
  `blitz2` nor in the libs. It was found only as data (`$BB22` in
  `displaylib.bb2`, a sample). The header `BB22 00 08 00 13 00 01 …` from
  `amibl2src/…/RWELibSourc/…/Anim-Library3.bb` therefore does not come from
  Blitz 2.1 or TED, but probably from an editor by Reflective Images.
  The layout (cursor in the header, a prefix before each line) and the
  converter `tools/bb22`: `docs/rwelibs/README.md` §4.1.

---

## 5 Compiler `blitz2`

### 5.1 Version **[B]**

`BLITZ2/BLITZ2` of the CD (86 216 bytes, one code hunk with 73 568 bytes and 3149
relocs): "Blitz Basic 2 Version 2.10", "(C)opyright 1995 - Acid Software",
"Release Version". The commands are in `blitz2:acidlibs`, `deflibs` and
`userlibs/` (releasenotes).

### 5.2 Built-in keywords **[B]** **[S]**

Structure as `tokens` in blitz2.s: a linked list of entries
`dc.l next` (relocated), `dc.w nummer`, `name,0`, `hilfetext,0`, `even`.
Token = `$8000 + nummer`. In compiler 2.10 the list begins at code offset
`$87A` (file offset `$89A`). It is referenced from `edstruct` (`$85E`)
and from two code locations.

**Result:** 210 entries `$8001`–`$80D3`. Only `$80B6` is missing (commented out
in blitz2.s: `Ern`). The list is in bb2 `builtin_tokens.txt` (not taken over)
(`TOKEN $xxxx name`):

* `$8001`–`$803B`: language (`NEWTYPE`, `End`, `Let`, `Dim` … `CNIF`, `CSIF`,
  `CELSE`, `CEND`, `CERR`, `Then`, `Else`, `Even4`, `Even8`, `Dc`, `Ds`,
  `Even`, `Dcb`, `GetReg`, `IncBin`, `Free`, `Use`, …, `ALibJsr`, `TokeJsr`,
  `BLibJsr`)
* `$803C`–`$80B2`: 68000 mnemonics `ABCD` … `UNLK`, `EndAsm`
* `$80B3`–`$80D3`: `List`, `SetErr`, `ClrErr`, `ErrFail`, `Addr`, `MOD`, `Pi`,
  `Repeat`, `Until`, `PutReg`, `Pop`, `INCDIR`, `EndIf`, `Wend`, `SysJsr`,
  `WBStartup`, `Maximum`, `On`, `Off`, `Forever`, `Restore`, `Exchange`,
  `USEPATH`, `CloseEd`, `NoCli`, `BitTst`/`Set`/`Clr`/`Chg`, `Used`,
  `Runerrson`, `Runerrsoff`

**Comparison with AB3** (`ab3c/build/host-debug/tests/tokens.txt`): 204
same, 14 different.

| Token | Blitz 2.1 | AB3 |
|---|---|---|
| `$8014` | `Jimi` (placeholder) | `SYNTAX` |
| `$803B` | `Jimi` (placeholder) | `EXTB` |
| `$807F` | **`MOVEP`** | **`Chip`** |
| `$80AD` | **`TAS`** | **`BSS`** |
| `$80D2`/`$80D3` | `Runerrson`/`Runerrsoff` | `RunErrsOn`/`RunErrsOff` (spelling only) |
| `$80D4`–`$80DB` | – | `USELASTPATH`, `RegLoad`, `RegSave`, `Object`, `USEPATH2`, `LastRunErrsMode`, `KEEP`, `OPTIMIZE` |

A Blitz 2.1 source with `MOVEP` or `TAS` would therefore silently become
`Chip`/`BSS` in AB3. The converter must output these two tokens as text (on
the CD they do not occur). `Jimi` is only a placeholder.

---

## 6 FNS fonts (RIFNSLib)

The bitmap font format of Reflective Images' FNS library (`InstallFNS`,
`FNSLoad`; userlib `rifnslib`, `libs/userlibs/rwe/rifnslib.c`). Described
in the library's guide ("my own (rather primitive) font file format",
AmiBlitz3 `Docs/UserLibs/RIFNSLib.guide`) and checked against
`PERSONAL.FNS` and the library code (`InstallFNS` tests `FNS.`) [B, S].

| Offset | Size | Contents |
|---|---|---|
| 0 | 4 | `FNS.` – identifier, checked by InstallFNS |
| 4 | word | height in pixels (rows) |
| 6 | word | width of a character cell in pixels, a multiple of 16 |
| 8 | word | underline position: row from the top |
| 10 | word | bytes per character = width / 8 × height |
| 12 | 20 | unused (0 in `PERSONAL.FNS`) |
| 32 | 224 | one byte per character code 32–255 (offset = code): the advance – the value added to x after printing the character, i.e. the proportional width including the gap (not the cell width) |
| 256 | … | the bitmaps, one per character from code 32 (space) on, as many as the file holds; each row width/8 bytes, big-endian, bit 15 of the first word = leftmost pixel |

One plane: the library colours a character with `FNSInk`/`FNSPrefs`
while drawing (shadow, underline, "chr$(1)" control codes – the guide).
A font covers codes from 32 up to as many bitmaps as follow; the advance
bytes of codes behind the last bitmap are still read (`PERSONAL.FNS`: 8
for 123–255, no bitmaps – printing them reads past the font, a
`BUG(orig)` of the library's use, see `docs/rwelibs/port/rifnslib.md`).
The font built into the library is described in
[docs/rwelibs/fns-personal.md](rwelibs/fns-personal.md).

---

## 7 AmiBlitz3's "Embedded .xtra Header" (`.ab3`)

AmiBlitz3 keeps the companion settings of a text source (`.ab3`) not in
a `.xtra` file but in comment lines at the start of the source. Evidence
**[C]**: AmiBlitz3 on GitHub (<https://github.com/AmiBlitz/AmiBlitz3>,
Apache License 2.0), commit `b1a9dee150ef` (2026-10-25), file
`Sourcecodes/Amiblitz3/Compiler/CLIModeSupport.ab3` (last changed in
`c11b4186fd20`, 2026-06-24): `write_AB3Header` (line 1077, writes it when
saving an `.ab3`), `ParseAB3Header` (line 576, reads it in `LoadSource`,
line 437: `.bb`/`.bb2`/`.ab2` get `LoadXtra` instead), `SkipXtraHeader`
(line 401, skips it for includes, `AmiBlitz3.ab3` line 19473); the
constants in `Sourcecodes/Amiblitz3/Shared/common.ab3` lines 85–93. Our
reader and writer are own code after this description
(`shared/xtra/xtra_ab3.c`, API in `xtra.h`).

### 7.1 Layout (as `write_AB3Header` writes it)

```
; XTRA
; Embedded .xtra Header
; 
; General Info
; -------------------------------------------------------
; ExePath         = "RAM:"
; ExeFile         = "Prog.exe"
; CreateIcon      = 0
; Residents       = "all.res"
; 
; Compiler
; -------------------------------------------------------
; StringBuffer    = 10240
; MakeSmallest    = 0
; FuncOptimize    = 0
; Version         = 0.0.0
; NumberOfBuilds  = 0
; 
; Debugger
; -------------------------------------------------------
; CliArgs         = ""
; StackSize       = 32768
; RuntimeDebug    = 1
; DebugInfo       = 0
; CreateDbgFile   = 0
; OverflowCheck   = 1
; AssemblerCheck  = 0
; InterruptCheck  = 1
; AutoRun         = 1
; 
; Editor
; -------------------------------------------------------
; CursorLine      = 1
; CursorColumn    = 1
; LabelSearch     = ""
; LabelRemark     = 0
; LabelAll        = 0
; LabelCase       = 0
; LabelPosition   = 0
; 
; Blitz Objects
; -------------------------------------------------------
; Max Screen      = 5
; Max Req-Lib     = 5
; Max ChunkyBuffer= 2
; /XTRA
```

LF line ends; the empty comment lines are `; ` (with a blank). The key is
padded to 16 characters after `; `, then `= `; strings in `"` without
escapes; numbers with `Str$` (signed decimal). `Residents`: the 8
entries joined – a `,` before each further entry unless the text so far
ends with one, one trailing `,` removed (empty entries vanish, except a
leading one: `",all.res"`). `Max NAME`: one line per object type of the
loaded libraries (`getobjectentry`), a blank in NAME written as `-`, the
key padded to 18 columns (longer names without padding). The program text
follows directly after `; /XTRA`.

### 7.2 Keys

| Key | Value | AB3 default | `struct xtra` | Blitz 2 `.xtra` |
|---|---|---|---|---|
| `ExePath`, `ExeFile` | text | `"RAM:"`, `"Prog.exe"` | `text[XTRA_F_EXE_PATH]`, `[XTRA_F_EXE_NAME]` (< 192) | offsets 598, 790 |
| `CreateIcon` | 0/≠0 | 0 | `icon` | offset 1 |
| `Residents` | text, `,`-separated | none | `text[XTRA_F_RES0 + i]` (≤ 8 entries, < 64 each) | offset 54 |
| `StringBuffer` | number | 10240 | `buf[XTRA_BUF_STRING]` | offset 50 |
| `MakeSmallest` | 0/≠0 | 0 | `sw[XTRA_SW_MAKE_SMALLEST]` | offset 12 |
| `FuncOptimize` | 0/≠0 | 0 | `ab3.func_optimize` | – (AB3 binary: the former "quoted macros" word) |
| `Version` | `MAJOR.MINOR.PATCH` | 0.0.0 | `ab3.version[3]` (0..255 each) | – (AB3 binary: CLI block +120, `MAJOR<<24 \| MINOR<<16 \| PATCH<<8`) |
| `NumberOfBuilds` | number | 0 | `ab3.builds` | – (AB3 binary: CLI block +124) |
| `CliArgs` | text | `""` | `text[XTRA_F_CLI]` (< 128; AB3 allows 254) | offset 982 |
| `StackSize` | number | 32768 | `ab3.stack` | – (AB3 binary: CLI block +116) |
| `RuntimeDebug` | 0/≠0 | 1 | `sw[XTRA_SW_RUNTIME_ERRORS]` | offset 2 |
| `DebugInfo` | 0/≠0 | 0 | `sw[XTRA_SW_DEBUG_INFO]` | offset 14 |
| `CreateDbgFile` | 0/≠0 | 0 | `ab3.create_dbgfile` | – |
| `OverflowCheck` | 0/≠0 | 1 | `sw[XTRA_SW_OVERFLOW_CHECK]` | offset 22 |
| `AssemblerCheck` | 0/≠0 | 0 | `sw[XTRA_SW_ASSEMBLER_CHECK]` | offset 20 |
| `InterruptCheck` | 0/≠0 | 1 | `sw[XTRA_SW_INTERRUPT_CHECK]` | offset 18 |
| `AutoRun` | 0/≠0 | 1 | `sw[XTRA_SW_AUTO_RUN]` | offset 16 |
| `CursorLine`, `CursorColumn` | number, 1-based | 0 | `editor.line`, `.column` (0-based) | – |
| `LabelSearch` | text | `""` | `text[XTRA_F_LABEL_SEARCH]` (< 32; AB3 keeps 29) | – |
| `LabelRemark`, `LabelAll`, `LabelCase` | word | 0 | `ab3.label_*` (words as written) | – |
| `LabelPosition` | number | 0 | `ab3.label_position` | – |
| `Max NAME` | number | the library's | `max`: the lines (iterator, `XTRA_MAX_NAME`) | pairs, by library **number** |

The switches the header does not have (Debugger, Long Arrays, Expand and
Quoted Macros, `optreq2ga5`, the two defaults) and the buffers other than
the string buffer keep Blitz 2's defaults in `struct xtra` (AmiBlitz3
itself clears the obsolete words and takes its own buffer defaults:
object 16384, libs 32768, system/data 4096, macro 40000).
`x->ab3.seen` has a bit per key found (`XTRA_K_*`).

### 7.3 How AmiBlitz3 reads it (`ParseAB3Header`)

* It reads **all lines from the start of the file** until a line whose
  key is `/xtra` (or the end of the file); there is no check of the first
  line, and lines without `;` count too.
* Key: the text before the first `=`, only letters, `_`, `/` and `-` kept
  (`-` becomes a blank; blanks and digits are dropped), compared in lower
  case. Value: the text after the `=` **minus its first character**
  (the blank `write_AB3Header` puts there), trimmed of characters below
  33, one pair of `"` removed if both ends have one; numbers with
  `Vallong` (so `-1` works: `LabelAll = -1` is in AmiBlitz3's own sources).
* `Residents`: split at `,`; after the 7th entry a warning "could not
  load all residents" and the rest is dropped.
* `Max NAME`: NAME is compared with the object names of the loaded
  libraries (`getobjectentry`); unknown names are ignored.
* After the loop: `ExePath` empty → the path part of `ExeFile`;
  `MakeSmallest` ≠ 0 forces `FuncOptimize`; `StringBuffer` below 4096 and
  `StackSize` below 8191 are raised to these minima; `CursorColumn` is
  read but not used ("cursorcol is missing").
* Unknown keys are ignored.

### 7.4 Our reader (`xtra_load`, `XTRA_T_AB3`)

Stricter where AmiBlitz3 guesses, so that broken headers are reported:

* A header exists only if the **first line** is `; XTRA` (blanks and case
  free); otherwise the source has no header (AmiBlitz3's defaults,
  `hdr` 0, no error). It ends at the line `; /XTRA`; `hdr` is the offset
  of the program text behind it (the bytes the caller keeps).
* Inside, every non-empty line must start with `;` (blanks before it
  allowed, CR before LF ignored); lines without `=` (title, ruler, `; `)
  are ignored.
* Key compared case-insensitively as a whole word (`Max` followed by a
  blank and NAME; digits in NAME are kept – AmiBlitz3 drops them); the
  value is trimmed of blanks/tabs (no first character dropped: `Key=1`
  is 1, AmiBlitz3 would read nothing).
* **Unknown keys are ignored and counted** (`ab3.unknown`, the first one's
  line in `ab3.unknown_line`); a key given twice: the last value counts
  (as in AmiBlitz3). The `Max` lines stay in the header: the iterator
  returns each line (a NAME given twice twice; the last one counts), the
  NAME as written (`-` for a blank).
* Values are kept as written: texts as views into the header, booleans
  as on/off (`≠ 0`), numbers as 32 bits (decimal or `$hex`, a leading `-`
  as two's complement), no minima applied, `ExePath` not derived.
* Refused, with the line: a line without `;`, no `; /XTRA`, a number that
  is none or out of range (32 bits; maxima 0..65535), a `"` without its
  closing `"`, text longer than its `.xtra` field, more than 8
  residents, `Version` not `N.N.N` with 0..255, `Max` without NAME (or
  one of 64 KB). No limit for the number of maxima.

### 7.5 Our writer (`xtra_write_ab3`)

* **A source with header: the kept header updated.** Line by line from
  `; XTRA` to `; /XTRA`: comments, rulers, unknown keys and the line
  ends stay as they are. For the line of a known key (the last one if
  the key is there twice) the writer reads the line again and compares
  its value with the structure, as the header shows it (booleans as
  on/off: `CreateIcon = 65535` stays for an icon flag `$80`; texts by
  their bytes; the residents slot by slot); only a different value is
  replaced – the rest of the line (key spelling, padding, blanks behind
  the value, CR) kept; directly after `=` a blank is put in (AmiBlitz3
  drops the first character after `=`). Keys the header lacks are added
  in front of `; /XTRA` (in the order of 7.2, with the line end of that
  line) if their value differs from AmiBlitz3's default. Maxima set by
  the caller (`xtra_set_maxima`) replace the `Max` lines (in front of
  `; /XTRA`). Unchanged settings give the same bytes for every layout.
* **A source without header** (or one built from a `.xtra`): a new
  header with exactly the lines of `write_AB3Header` (7.1): all keys,
  the residents joined as AmiBlitz3 does, `CursorLine`/`CursorColumn` =
  0-based value + 1 (1 without an editor part), numbers signed like
  `Str$` (the label words sign-extended), LF line ends.
* Both write the values as AmiBlitz3 does (booleans 0/1, numbers like
  `Str$`), the maxima with a name (a blank as `-`; the others are counted
  as skipped); output into the caller's buffer, `xtra_ab3_size` gives the
  size needed.

### 7.6 Converting and checking

* `.ab3` → Blitz 2 `.xtra` (`xtra convert --to-xtra`): everything in the
  last column of 7.2; the AB3-only fields and the editor part are lost;
  the maxima are left out (the `.xtra` needs library numbers – mapping
  object names to libraries needs the libraries, a later step).
* Blitz 2 `.xtra` → header lines (`xtra convert --to-ab3`, with
  `--bb22 SRC` the cursor of a `BB22` source): a new header, the
  AB3-only fields with AmiBlitz3's defaults; the maxima are left out (no
  names). With `--ab3 SRC` the output is the source SRC with its header
  updated from the `.xtra` (`xtra_take_blitz2`; AmiBlitz3's own fields
  and the `Max` lines stay; a new header if SRC has none).
* `xtra check --ab3`: the header written again is byte for byte the
  file's, and a new header written from the fields and read again has
  the same fields (`--exact`: the new header is also byte for byte the
  file's – for headers in the current layout). All five AmiBlitz3
  sources of `archive/amiblitz3/github-b1a9dee` pass, three of them in
  older layouts (ctest `xtra_ab3_examples`).
* Survey **[B]** with `xtra check --ab3` (before the rework of
  2026-10-30) over the 1158 `.ab3` files of a local clone of AmiBlitz3's
  repository: 964 start with `; XTRA`, 194 have no header; 1156 read without
  error; refused are the 2 whose header was cut off without `; /XTRA`
  (the next line is program text). 477 headers were not written back
  byte for byte by the writer of that time, which always wrote the
  current layout: older layouts (358 without `LabelCase`, 63 with only
  some keys, others without `FuncOptimize`/`CreateDbgFile`) and values
  like `-1` or `65535` for booleans. The updating writer keeps all of
  them; the survey was not repeated (the clone is not in the archive).

---

## Open points

1. **AB3 reads `.xtra` with 15 two-byte switches** (since `bdae3f9e`,
   08.01.2026). Blitz 2.1, AmiBlitz 2 and older AB3 write 14. This
   shifts everything from offset 30 on by 2 bytes; resident names are also
   truncated. Check in Amiberry and record as an issue in chapter 11.
2. Meaning of `optreq2ga5` (offset 24, always `$80`) and of the
   buffer values 0 or unusually small buffer values.
3. String umlauts: that Blitz 2.1 converts `$1A`–`$1F` in string literals via
   `getchar3`/`gs` to Latin-1 is in the source of v1.95; the
   table is also in the 2.10 binary. Whether compiling strings really
   goes through `getchar3` should be settled by a test with the real 2.10 in Amiberry
   (`NPrint` with `$1A`–`$1F`).
4. `BB22` header: not present on the CD; origin (Reflective Images /
   RI editor?) still open. The format is described in
   `docs/rwelibs/README.md` §4.1 (reader `tools/bb22`).
5. Lib tokens: mapping the 100 used lib numbers to command names
   needs `acidlibs`/`deflibs` (BlitzLib format, chapter 09) or AB3's
   `replacedat`. Not part of this analysis.
6. `extras/blitzlibs/libnums.res` contains the contents of `libmacs.res`; the
   correct version is on `bum9b`/`bum10a`.
7. The `$01` bytes in `ripacklib.bb` and `elmorehardwarelib.bb`: origin
   unclear. The converter should report them.
