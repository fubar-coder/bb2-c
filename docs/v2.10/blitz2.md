# Compiler: blitz2 v1.95 (source) vs v2.10 (CD)

> As of 28.10.2026. Static comparison of binaries, nothing executed.
> Evidence: **[B]** binary comparison (commands in [README.md](README.md),
> section "Method"), **[S]** source (`blitz2.s:NNN`), **[Schluss]**
> conclusion, not proven.

Compared:

* **1.95** = the reference build of this repository
  (`cmake --build --preset amiga --target blitz2_ref`,
  `build/amiga/programs/blitz2/ref/blitz2`, vasm `-devpac`, with
  `blitz2.map` and `blitz2.labels`), assembled from
  `archive/sources/blitz2-1.95/blitz2.s` (`version equ 195`).
* **2.10** = `archive/cd/ultimate-blitz-basic-2.1/BLITZ2/BLITZ2`
  (86 216 bytes, SHA-256 `0c08492e…4d88c2b`, "Blitz Basic 2 Version 2.10",
  "Release Version"). `BUM9A/BLITZ2/BLITZ2` and `BUM10B/BLITZ2/BLITZ2` are
  byte-identical copies; there is no other compiler binary on the CD.

## Changelog 1.95 → 2.10

This is the complete list of what differs between the archived source
(1.95) and the released compiler 2.10, by category. Every byte of the two
code hunks was compared (section 1); the list is exhaustive for the
compiler binary [B].

| Category | Changed / added / removed in 2.10 |
|---|---|
| Version | About requester text "Blitz Basic 2 Version 1.95" → "**2.10**" (`verreqla4`, `$2820`–`$2823`; built with `version equ 210`) |
| Behaviour / bug fixes | **userlibs path**: `loadblitzlibs` (`$7094`, `blitz2.s:6400`) checks for a trailing `/` instead of `:` → libraries in `blitz2:userlibs/<dir>/` are found (1.95: `blitz2:userlibs//<dir>` = `blitz2:<dir>`); issue #2 |
| Generated code | `Select … Case` on byte values: the templates `selpushb`/`selcomb` are emitted as `ANDI.W #255,D0` (`0240 00FF`) instead of the vasm build's `AND.W #255,D0` (`C07C 00FF`) – same behaviour, different bytes in the executable (section 3.1) |
| Commands / keywords (built-in tokens `$8001`–`$80D3`) | none (keyword list `tokens` byte-identical) |
| Compiler directives, compile-time features | none |
| Inline assembler (mnemonics, addressing modes, encodings it generates) | none (tables and `asmit`/`getea` logic identical; only the compiler's *own* instructions are encoded differently) |
| Compiler options, option requester, `.xtra` format | none (`optspnts`, option gadgets, `loadxtra`/`savextra` identical) |
| Error messages | none (all texts identical) |
| Menus, requesters, other texts | none except the version text |
| Library loading (`loadlibs`, order acidlibs → deflibs → userlibs), residents, debugger interface, `edstruct`/TED interface | none (apart from the userlibs path) |
| Data tables (umlaut table `gs`, operator tables, type tables) | none |
| Routines added / removed | none (same layout, all 2319 labels at the same offsets) |
| Assembler-only differences | 154 instructions of the compiler itself encoded by Devpac instead of vasm (`ANDI/ORI/SUBI/ADDI` vs `AND/OR/SUB/ADD #imm`, swapped `exg` operands), 1 padding word – no effect |

Everything the CD documentation describes as "new to version 2.1"
(`BLITZ2/RELEASENOTES`: online help, restructured library handling –
acidlibs/deflibs/userlibs –, "bug free parameter passing", AGA and 3.0
support) is **already in the archived 1.95 source** (the 2.10 binary is that
source); the release notes describe the
step from Blitz 2.0x to the 1.95/2.1 code line, not a difference between
the source and 2.10 [B, Doku]. Online help is done by TED/`blitz2:help`;
AGA/3.0 support lives in the libraries [Schluss].

## 1 Result

**Compiler 2.10 is `blitz2.s` as archived (1.95), assembled with Devpac,
with `version equ 210` and one changed source line.** [B]

| | 1.95 (vasm build) | 2.10 (CD) |
|---|---|---|
| hunks | 1 × HUNK_CODE | 1 × HUNK_CODE |
| code size | `$11F60` = 73 568 bytes | `$11F60` = 73 568 bytes |
| relocations (hunk 0 → 0) | 3149 (2752 as HUNK_RELOC32SHORT `$3F7`, 397 as HUNK_RELOC32) | 3149 (HUNK_RELOC32 only) |
| relocation offsets | – | **the same 3149 offsets** |
| differing code bytes | – | 314 bytes in 154 contiguous places (159 instructions/data items) |
| file size | 80 724 | 86 216 (only because of the relocation format) |

Of the 154 places, **151 are pure assembler encodings** (154 instructions;
Devpac vs vasm, same instruction, same length, same flags), **1 is
padding**, and **2 are real changes**:

| Offset | Source | 1.95 | 2.10 | Meaning |
|---|---|---|---|---|
| `$2820`–`$2823` | `blitz2.s:870–877 verreqla4` | `31 2E 39 35` "1.95" | `32 2E 31 30` "2.10" | version text of the About requester. The digits are computed from `version` at assembly time (`tver`/`tver2`, lines 871–877); `version` is used nowhere else (grep: lines 18, 871, 873). So 2.10 was built with `version equ 210`. |
| `$7094` | `blitz2.s:6400` (`loadblitzlibs`) | `0C18 003A` `cmp.b #':',(a0)+` | `0C18 002F` `cmp.b #'/',(a0)+` | **bug fix**: the path of the userlibs directories no longer gets a second `/` (issue #2, section 4) |
| `$11F5E` | end of the hunk (after `menu9 .ok rts`, `blitz2.s:16015`) | `4E71` | `0000` | longword padding of the hunk (vasm pads with NOP, Devpac with 0) |

Everything else – keyword list (`tokens`, `$87A`), option table
(`optspnts`, `$F936`), `.xtra` handling, error messages, menus, requesters,
umlaut table `gs` (`$7BEE`), code templates, the inline assembler tables –
is byte-identical, apart from the encodings in section 3. **There are no
new or removed routines, keywords, tokens, options or error messages.**

### 1.1 Alignment by label

Because size, layout and relocation offsets are identical, **every label of
the 1.95 build is at the same offset in 2.10**: the mapping 1.95 → 2.10 is
the identity for all 2319 labels of `build/amiga/programs/blitz2/ref/blitz2.labels`
(and all lines of `blitz2.map`). Examples, checked in the 2.10 file
(file offset = code offset + `$20`):

| Label | Offset | 2.10 bytes there |
|---|---|---|
| `edstruct` (`blitz2.s:274`) | `$085E` | (referenced by TED) |
| `tokens` (`blitz2.s:283`) | `$087A` | `00000892 0001 "NEWTYPE"` – first keyword entry (`next.l`, `nummer.w`, name) |
| `libsdir` (`blitz2.s:750`) | `$229C` | `"blitz2:userlibs/"` |
| `gs` (`blitz2.s:7181`) | `$7BEE` | 26 × `00`, then `E4 F6 FC C4 D6 DC` |
| `optspnts` (`blitz2.s:15293`) | `$F936` | `0001 0000FD8E 00010FB9 00000000 0002 …` |

The table `optspnts` is at code offset `$F936` (file offset `$F956`) in
both builds (`docs/formats.md` §1.1).

So the C port, whose functions carry `blitz2.s:NNN label` references, is
aligned with 2.10 by these labels without further work.

## 2 The real change: `loadblitzlibs` (blitz2.s:6392–6405)

1.95 source and vasm build:

```
007082 loadblitzlibs  lea     undobuffer,a0
007088                lea     libsdir,a1           ; "blitz2:userlibs/"
00708E .loop0         move.b  (a1)+,(a0)+
007090                bne.s   .loop0
007092                subq    #2,a0
007094                cmp.b   #':',(a0)+           ; 0C18 003A
007098                beq.s   .yup
00709A                cmp.l   #undobuffer,a0
0070A0                beq.s   .yup
0070A2                move.b  #'/',(a0)+
0070A6 .yup           move.l  a0,-(a7)
```

2.10: the same, except `$7094`: `0C18 002F` = `cmp.b #'/',(a0)+`. With the
fixed `libsdir` (`blitz2:userlibs/`, never written) 2.10 builds
`blitz2:userlibs/<dir>/…`, 1.95 `blitz2:userlibs//<dir>/…` (AmigaDOS: `//`
= parent directory, i.e. `blitz2:<dir>`). 2.10 no longer tests for `:` at
all; with the constant `libsdir` this makes no difference [Schluss].

The C port had the 1.95 behaviour (`if (*a0++ != ':')`, `BUG(orig)`);
since 29.10.2026 it tests `'/'` as 2.10 does (`asmlibs.c`
`loadblitzlibs()`, `DIFF`; #2 fixed).

## 3 Assembler encodings (no functional difference)

Devpac encodes `and/or/sub/add #imm,Dn` (word, long, byte) as
`ANDI/ORI/SUBI/ADDI` (`$02xx/$00xx/$04xx/$06xx`), vasm (also with
`-devpac`; no option changes this, tried: `-opt-speed`, `-opt-allbra`)
as `AND/OR/SUB/ADD` with immediate source (`$Cx7C/$8x7C/$9x7C/$Dx7C`).
`exg Ax,Ay` has its operands swapped (`exg a4,a5` = vasm `$C94D`, Devpac
`$CB4C`; already known, `docs/blitz2/survey-01.md`). Both forms are the
same length, set the same flags and give the same result. Counted [B]:
`or` 64, `and` 59, `sub` 20, `add` 8, `exg` 3 = 154 instructions (in 151
places; + the 3 places of section 1). The full list is in the appendix.

### 3.1 Consequence for generated programs

Most of the places are code of the compiler itself. **Two are code
templates that blitz2 copies into the compiled program** [B]:

| Template | Offset | 1.95 (vasm) | 2.10 (Devpac) | Copied by |
|---|---|---|---|---|
| `selpushb` (`blitz2.s:13015`) `and #255,d0` | `$CD4E` | `C07C 00FF` | `0240 00FF` | `blitz2.s:13193` (`lea selpushb(pc),a0 / lea selpushbf(pc),a1`) |
| `selcomb` (`blitz2.s:13023`) `and #255,d0` | `$CD5A` | `C07C 00FF` | `0240 00FF` | `blitz2.s:13146` |

These are used by `Select`/`Case` on byte expressions. A program compiled
by 2.10 therefore differs from one compiled by the 1.95 reference build
(and by the C port, which takes the vasm bytes: `programs/blitz2/src/tokens1.c:77`
and `:81`, `0xC07C, 0x00FF`) by these 2 bytes per use – functionally the
same. This is probably the "one assembler encoding" by which the corpus
run of bb2 found v1.95 and v2.10 output to differ (`docs/architecture.md`,
header) [Schluss]. Found by searching all `lea A(pc),aN / lea B(pc),aM`
template pairs of `blitz2.s` for differing bytes and by searching the C
port for vasm-specific encodings (`grep -E "0x(C.7C|8.7C|9.7C|D.7C|C.BC|D.BC|C94D|CB4C)"`
in `programs/blitz2/src`: only `tokens1.c:77/81` and `main.c:887`
`exga4a5 0xCB4C` – the latter already uses the Devpac encoding and is never
copied [`docs/blitz2/survey-01.md`]).

## 4 Bug-orig issues of the compiler in 2.10

Since all code except `$7094` is identical, every compiler bug documented
from the 1.95 source is present in 2.10 unchanged – except #2. The port
fixes some of them; those rows say "fixed in the port". [B]

| Issue | State | Title | In 2.10 |
|---|---|---|---|
| #1 | closed | [compiler/Assembler] Inline assembler expression over 239 characters: node size stored as a byte | **fixed in the port** |
| #2 | closed | [compiler/Libs] Userlibs path becomes "blitz2:userlibs//dir" (parent directory) | **fixed in 2.10** |
| #3 | closed | [compiler/Libs] Read error (-1) while loading a library is not detected | **fixed in the port** |
| #4 | closed | [compiler/Macros] Macro parameters are collected without checking against the end of macrobuff | unchanged |
| #5 | closed | [compiler/Macros] `@` (expansion counter of the macro) is written into the macro line without overflow check | unchanged |
| #6 | open | [compiler/rundebuglib] Executables with runtime error checking show an empty requester at program end without `End` | unchanged |
| #7 | closed | [compiler] Constant without a name (`#=…`) is accepted without error | **fixed in the port** |
| #8 | closed | [compiler/Options] AllocMem for the arrow images of the options requester unchecked | **fixed in the port** |
| #9 | closed | [compiler/Expression] Operator `=<` is compiled as `<>` | **fixed in the port** |
| #10 | closed | [compiler/Constants] Number with a point but no fractional digits (`1.`) is a syntax error | **fixed in the port** |
| #11 | closed | [compiler/Constants] Exponent after fractional part 0 (`1.0e5`) is not evaluated | **fixed in the port** |
| #12 | open | [compiler/Constants] consttype2 jumps to `.quick2` with unloaded d0 for type Word/unknown | unchanged |
| #13 | open | [compiler/Constants] Quick constant with fractional part 0 or negative fractional part becomes a float constant | unchanged |
| #14 | closed | [compiler/Code generation] pushput does not free the save buffer when the object buffer is full | **fixed in the port** |
| #15 | closed | [compiler/Code generation] Multiplication by 256 generates `lsl #8` twice (×65536) | **fixed in the port** |
| #16 | closed | [compiler/IncDir] Path without length check (buffer `incdir` 96 bytes) | **fixed in the port** |
| #17 | closed | [compiler/IncDir] Rest of the line (also after `:`) is taken into the path | **fixed in the port** |
| #18 | closed | [compiler] Failure of AllocSignal for the PANIC signal is not detected | unchanged |
| #19 | closed | [compiler/WB start] Short-read .info file is not closed | **fixed in the port** |
| #20 | closed | [compiler/WB start] Without a readable .info the editor is supposed to load "<name>.info" as the program | **fixed in the port** |
| #21 | closed | [compiler/WB start] Old current directory is never restored | **fixed in the port** |
| #22 | closed | [compiler] Return value of blitz2 is lost in wb_exit | **fixed in the port** |
| #23 | closed | [compiler/Debugger] Loading a debug module without an old module leaves the busy pointer | **fixed in the port** |
| #24 | closed | [compiler/Run] PANIC removes the program process with RemTask without cleanup | **won't fix**: a clean abort of a running program is not worth the complexity (2.10 unchanged) |
| #25 | closed | [compiler/Run] Failure of CreateProc leaves the stop requester open | **fixed in the port** |
| #26 | closed | [compiler] flushlibs: the 256 MB request would never be freed if it succeeded | **fixed in the port** |
| #27 | closed | [compiler] checklib: result of Examine unchecked | **fixed in the port** |
| #28 | closed | [compiler/Libs] Directory name under blitz2:userlibs copied without length check | **fixed in the port** |
| #29 | closed | [compiler/Parser] makename4: identifier copied to namebuff without length check | **fixed in the port** |
| #30 | closed | [compiler/Parser] findlab with name length 0 compares 65536 bytes | **fixed in the port** |
| #31 | closed | [compiler] UsePath: path without length check (buffer 128 bytes) | **fixed in the port** |
| #32 | closed | [compiler/Options] New buffer sizes: fallback to the previous sizes is reported as success | unchanged |
| #34 | open | [compiler/Resident] Last constant of a resident points past the section | unchanged |
| #35 | closed | [compiler/Resident] Type without fields: fixtypes follows the raw next pointer | **fixed in the port** |
| #36 | open | [compiler/Resident] Duplicate constants in several residents are never detected | unchanged |
| #37 | closed | [compiler] `Dcb` with count 0 repeats 2^32 times | **fixed in the port** |
| #38 | open | [compiler] IncBin does not free the lock or file on errors | unchanged |
| #39 | closed | [compiler] `For a$=…` with a global string variable passes the type check | **fixed in the port** |
| #40 | closed | [compiler/Expression] Operator stack: overflow is detected only after writing | unchanged |
| #41 | closed | [compiler/Executable] Symbol names in HUNK_SYMBOL with up to 3 bytes past the label node | unchanged |
| #42 | open | [compiler/environlib/errtraplib/runerrlib] On the 68010 the VBR is not read | unchanged |
| #325 | closed | [compiler/Include] makeiname: IncDir + file name into namebuff (128 bytes) without a length check | **fixed in the port** |
| #335 | closed | [compiler] Object libraries reached only from library code get no array; End frees their objects from address 0 | **fixed in the port** |
| #336 | open | [compiler/Macros] macrobuff is freed with the current Macro Buffer size, not the allocated one | unchanged |
| #337 | closed | [compiler/WB start] wb_exit calls Permit after ReplyMsg, before blitz2 has returned | **fixed in the port** |
| #341 | closed | [compiler/Include] makeiname prepends IncDir also to absolute names (with ':'), so Include "dev:file" fails after IncDir | **fixed in the port** (BB2 mode only; AB3 mode keeps its INCDIR rule) |
| #342 | closed | [compiler/IncDir] incdir is not reset between compiles (leaks into the next source in the same blitz2 session) | **fixed in the port** |
| #343 | closed | [compiler/Assembler] A folded operation on a character constant (`#">"*8`) leaves quoteflag set: `#" "` on an earlier line becomes 0 (`eval.c` eval5, test `bug_orig_compiler_asmfixer_quoteflag`) | **fixed in the port** (RILESDEBUGLIB on the CD unchanged) |
| #344 | open | [compiler/Code generation] Amiga library call in an expression: parameter registers d2–d5 holding intermediate values are not saved (test `bug_orig_compiler_amigalib_param_regs`) | unchanged |
| #937 | closed | [compiler/Arrays] List array indexed by an element of a normal array (`a(b(1))`): the nested calcvar resets `.listoff`, the index is multiplied by the bare element size (`parse.c` calcvar, test `bug_orig_compiler_calcvar_listoff`) | **fixed in the port** |
| #939 | open | [compiler/Objects] Object addresses are table + (WORD)(n << shift) (`lsl.w` + `add.w`/`adda.w`): objects with n × size ≥ 32 KB are addressed before the object table (`tokens1.c` getmaxlib/dofree/douse, `eval.c` addrcode/doaddr, `libcall.c` toreg; the range forms of shapeslib LoadShapes/DecodeShapes and spriteslib LoadSprites likewise) | unchanged |
| #940 | closed | [compiler/Procedures] End Statement / End Function and Statement Return / Function Return are not checked against the kind of procedure: a string Function ended with End Statement pops its return address (`tokens2.c` doend, stateret/funcret, endstate) | **fixed in the port** |
| #942 | closed | [compiler/Macros] A `:` right after the macro name stays in the macro text: `Macro a:1:End Macro` expands to `:1` (`tokens1.c` domacro) | **fixed in the port** |
| #943 | open | [compiler/Executable] All relocations go into one HUNK_RELOC32 block: an executable with more than 65535 relocations does not load on AmigaOS 3.2 (Kickstart 1.3 loads it) (`parse.c` executable writer, test `bug_orig_compiler_reloc32_one_block`) | unchanged |
| #964 | closed | [compiler/Macros] A macro text of 65536 bytes or more is cut: its length is stored in a word (`domacro` `.cdone`, `freemacs`; `tokens1.c` domacro, test `bug_orig_compiler_macro_textlen_word`) | **fixed in the port** |
| #967 | closed | [compiler/Macros] A one-line If … Then inside a macro stays open until the end of the calling line (`domacro`, `doif2`, `doline`, `doendif3`; `tokens1.c` domacro, `ab3_cli` case `macthen_bb2`) | **not a bug**: the programmer writes the `End If`; AB3 mode follows AmiBlitz3 (2.10 unchanged: `blitz2.s:12866–12872`, `13275`, `2792–2806`, `13830–13836`; the only difference is the encoding of `and #255,d0` at `doendif3+$10`, same meaning) |
| #968 | closed | [compiler/Macros] A macro body line ending in End swallows the rest of the macro (`getcharb`, `domacro`, `readinc`; `tokens1.c` domacro, tests `bug_orig_compiler_macro_end_line`, `bug_orig_compiler_macro_end_line_include`) | **fixed in the port** |
| #974 | closed | [compiler/NewType] a by-value field of a forward-declared NewType (named before only by a pointer field) takes the line number of that pointer field as its size; the later NEWTYPE does not correct the layout (`tokens2.c` typemode, doend3) | **fixed in the port** |
| #977 | closed | [compiler/Labels] A constant of value 0 is taken as a label not defined yet | **fixed in the port** |
| #979 | closed | [compiler/IncBin] An IncBin of odd length at the end of the program puts the end code on an odd address | **fixed in the port** |
| #993 | closed | [compiler/Libraries] loadalib does not check the read of the reloc data: a short library file gets random relocs | **fixed in the port** |
| #994 | closed | [compiler/Code buffer] A compile error between pushdo and pushput loses the code save buffer | **fixed in the port** |
| #995 | closed | [compiler/Residents] A macro text of 32767 bytes or more breaks the resident (length rounded in a word; `resmacs`, `fixmacs`) | **fixed in the port** |
| #997 | open | [compiler/Assembler] copyconst copies an operand text into namebuff without a length check (`asmlibs.c` copyconst, test `bug_orig_compiler_copyconst_overflow`) | unchanged |

The `BUG(orig)` comments in `programs/blitz2/src` refer to the same code
and all of them apply to 2.10 as well, except the bugs the port has
fixed (see the table); #2, the one bug 2.10 fixed, is a `DIFF` in the
port (`asmlibs.c:1254`, the path separator as in 2.10).
Issues of the libraries (`rundebuglib` #6/#43, `deflibs` #44/#45 …) are
covered in [libs.md](libs.md); those of TED in [ted.md](ted.md) §1.2.

## 5 Data and tables

All identical (no byte differences outside the places of sections 1 and
3) [B]:

* keyword list `tokens` (`$87A`, 210 entries `$8001`–`$80D3`, see
  `docs/formats.md` §5.2),
* option requester and option table `optspnts` (`$F936`),
* error messages, menu texts, requesters, `(C)opyright 1995 - Acid Software`,
  `Release Version` (`final equ -1`, release build in both),
* `libsdir`, `blitz2:acidlibs`, `blitz2:deflibs`, `blitz2:ted`,
  `blitz2:defaultdbug`,
* only the version text differs ("1.95" → "2.10").

## 6 What porting "2.10" means

1. `programs/blitz2/src/main.h:36` `#define blitz2version 195` → `210`
   (the text is computed in `main.c:736–740` like in the original).
2. `asmlibs.c loadblitzlibs()`: test `'/'` instead of `':'` (and drop the
   `BUG(orig)` comment, close #2 as "fixed in 2.10").
3. Optional, for byte-identical output with 2.10: the two templates
   `selpushb`/`selcomb` with `0x0240` (`tokens1.c:77/81`). Then the corpus
   comparison against the vasm reference build (`tests/corpus`, config
   `ref`) would differ in these programs; the reference for the output
   would have to become the CD binary (`BLITZ2/BLITZ2`), or the comparison
   would have to accept the encoding.

Effort: a few lines; the acceptance is the normal one (`ctest --preset all`).
The reference build (`blitz2_ref`) could be checked against the CD binary
with a "same modulo encodings" comparison (like `tools/asmcmp`, which already
compares instructions with `objdump`): differences only at `$2820`–`$2823`
and `$7094` – a cheap guard that the archived source really is 2.10 minus
these changes.

## Per-routine table

Every global label (routine or data block) in which at least one byte
differs. Offset = in both versions (identical layout). Class: `enc` =
bytes that are only assembler encodings, `real` = semantic change, `pad` =
hunk padding. All other 2225 labels are byte-identical. Full table of all
2319 labels with both offsets: `.cache/v2.10-analysis/blitz2/labelmap_195_210.tsv`
(see [README.md](README.md) §3). Where two labels share an offset the table
names one of them (`ressbasef` = `exga4a5`, `selpushsf` = `selcomb`).

| Label | Source | Offset | Differing bytes | Class |
|---|---|---|---|---|
| `movemchk` | `asmmac:346` | `$001C06` | 4 | enc:4 |
| `exgchk` | `asmmac:451` | `$001D42` | 6 | enc:6 |
| `aslchk2` | `asmmac:485` | `$001DA8` | 2 | enc:2 |
| `verreqla4` | `blitz2.s:870` | `$00280A` | 3 | real:3 |
| `ressbasef` | `?:0` | `$002A30` | 2 | enc:2 |
| `calcvbr` | `blitz2.s:1734` | `$00356A` | 2 | enc:2 |
| `makeblitzlibs` | `blitz2.s:1816` | `$00365C` | 4 | enc:4 |
| `hexascii` | `?:0` | `$003A14` | 4 | enc:4 |
| `expandmax` | `?:0` | `$003A3A` | 10 | enc:10 |
| `maketrap` | `blitz2.s:2868` | `$0044A8` | 2 | enc:2 |
| `handletoken` | `?:0` | `$0047A2` | 4 | enc:4 |
| `dolibtoke` | `?:0` | `$004878` | 8 | enc:8 |
| `sizespec` | `blitz2.s:3283` | `$0049EA` | 2 | enc:2 |
| `endininfo` | `?:0` | `$004D44` | 2 | enc:2 |
| `gettokeps` | `?:0` | `$004D7C` | 2 | enc:2 |
| `dolibjsr` | `blitz2.s:3571` | `$004DD4` | 2 | enc:2 |
| `doblibjsr` | `blitz2.s:3582` | `$004DF4` | 2 | enc:2 |
| `stamigalib` | `?:0` | `$0051D2` | 6 | enc:6 |
| `alloxadj` | `?:0` | `$0052AA` | 2 | enc:2 |
| `menu5` | `?:0` | `$0055BA` | 2 | enc:2 |
| `doputreg` | `blitz2.s:4665` | `$005C40` | 4 | enc:4 |
| `dosetreg` | `blitz2.s:4707` | `$005CD6` | 4 | enc:4 |
| `getreg2` | `blitz2.s:4731` | `$005D22` | 2 | enc:2 |
| `getan` | `blitz2.s:4740` | `$005D3E` | 2 | enc:2 |
| `getan3` | `blitz2.s:4744` | `$005D4E` | 2 | enc:2 |
| `getmovem` | `blitz2.s:4753` | `$005D6A` | 2 | enc:2 |
| `getea` | `?:0` | `$005DC2` | 16 | enc:16 |
| `getrest` | `blitz2.s:4961` | `$005FD4` | 6 | enc:6 |
| `asmit` | `blitz2.s:5072` | `$00612C` | 10 | enc:10 |
| `asmfixer` | `?:0` | `$006410` | 2 | enc:2 |
| `doea` | `?:0` | `$00657C` | 4 | enc:4 |
| `fetchlibps` | `?:0` | `$006772` | 6 | enc:6 |
| `fetchpees3` | `blitz2.s:5624` | `$0067F2` | 2 | enc:2 |
| `handlep` | `?:0` | `$006836` | 2 | enc:2 |
| `pushap` | `?:0` | `$00685C` | 2 | enc:2 |
| `handlevara` | `?:0` | `$006894` | 2 | enc:2 |
| `sendtype` | `?:0` | `$006914` | 2 | enc:2 |
| `getap` | `blitz2.s:5822` | `$006A10` | 2 | enc:2 |
| `tokecode` | `?:0` | `$006CE2` | 6 | enc:6 |
| `loadblitzlibs` | `?:0` | `$007082` | 1 | real:1 |
| `fixlibs` | `?:0` | `$0071FC` | 4 | enc:4 |
| `aligna0` | `blitz2.s:6624` | `$007322` | 4 | enc:4 |
| `savefile` | `?:0` | `$007524` | 6 | enc:6 |
| `addd0a0` | `blitz2.s:8061` | `$009004` | 2 | enc:2 |
| `makelmul` | `?:0` | `$009052` | 2 | enc:2 |
| `calcvar2` | `?:0` | `$009122` | 2 | enc:2 |
| `maineval2` | `?:0` | `$0099A6` | 4 | enc:4 |
| `eval5` | `blitz2.s:8983` | `$009AFA` | 4 | enc:4 |
| `bittst` | `blitz2.s:9572` | `$00A2D8` | 4 | enc:4 |
| `ritetod0` | `?:0` | `$00A352` | 4 | enc:4 |
| `ritefromd0` | `?:0` | `$00A36E` | 2 | enc:2 |
| `getelement` | `blitz2.s:9643` | `$00A386` | 4 | enc:4 |
| `notit` | `?:0` | `$00A428` | 2 | enc:2 |
| `fetchpi` | `blitz2.s:9700` | `$00A44E` | 2 | enc:2 |
| `chkhex` | `?:0` | `$00A850` | 2 | enc:2 |
| `fetchhex` | `blitz2.s:9866` | `$00A886` | 2 | enc:2 |
| `fetchbin` | `blitz2.s:9883` | `$00A8C0` | 2 | enc:2 |
| `doused` | `?:0` | `$00A9EC` | 2 | enc:2 |
| `doaddr` | `?:0` | `$00AA8C` | 4 | enc:4 |
| `libfunction` | `?:0` | `$00AB3C` | 4 | enc:4 |
| `amigalib` | `blitz2.s:10183` | `$00AC1A` | 8 | enc:8 |
| `qmark` | `blitz2.s:10406` | `$00AED4` | 2 | enc:2 |
| `varcont` | `blitz2.s:10479` | `$00AF66` | 2 | enc:2 |
| `afunction` | `?:0` | `$00AFDE` | 2 | enc:2 |
| `stvar` | `?:0` | `$00B0F6` | 2 | enc:2 |
| `variable2` | `blitz2.s:10641` | `$00B150` | 6 | enc:6 |
| `fetchnum` | `?:0` | `$00B224` | 2 | enc:2 |
| `fetchpoint2` | `blitz2.s:10737` | `$00B272` | 2 | enc:2 |
| `getsign` | `blitz2.s:10804` | `$00B322` | 4 | enc:4 |
| `fetchdec` | `blitz2.s:10833` | `$00B3A6` | 2 | enc:2 |
| `fetchnum3` | `blitz2.s:10856` | `$00B3DC` | 4 | enc:4 |
| `muld1` | `?:0` | `$00B6DE` | 6 | enc:6 |
| `makevsize` | `?:0` | `$00B904` | 2 | enc:2 |
| `userjsr` | `?:0` | `$00BABE` | 2 | enc:2 |
| `toreg` | `?:0` | `$00BD06` | 14 | enc:14 |
| `makeregop` | `?:0` | `$00BF18` | 2 | enc:2 |
| `ifchar` | `?:0` | `$00BF52` | 2 | enc:2 |
| `getmaxel` | `blitz2.s:12196` | `$00C340` | 2 | enc:2 |
| `doread` | `?:0` | `$00C57C` | 2 | enc:2 |
| `collif` | `?:0` | `$00CCA8` | 2 | enc:2 |
| `selpushb` | `blitz2.s:13015` | `$00CD4E` | 2 | enc:2 |
| `selpushsf` | `?:0` | `$00CD5A` | 2 | enc:2 |
| `dofunc2` | `?:0` | `$00D304` | 2 | enc:2 |
| `procfixer` | `?:0` | `$00D4F8` | 4 | enc:4 |
| `doendif3` | `blitz2.s:13830` | `$00D79E` | 2 | enc:2 |
| `doend` | `blitz2.s:13876` | `$00D832` | 2 | enc:2 |
| `endstate` | `?:0` | `$00D9E8` | 2 | enc:2 |
| `donext` | `?:0` | `$00DBB2` | 2 | enc:2 |
| `dofor` | `?:0` | `$00DCFC` | 2 | enc:2 |
| `dolet2` | `?:0` | `$00DEA6` | 4 | enc:4 |
| `typeshow` | `blitz2.s:15140` | `$00EF38` | 2 | enc:2 |
| `optreqprep` | `options.src:1` | `$0102B0` | 2 | enc:2 |
| `menu7` | `?:0` | `$011A24` | 2 | enc:2 |
| `menu9` | `?:0` | `$011EE8` | 2 | pad:2 |


## Appendix: all encoding differences

Offsets are code-hunk offsets (file offset + `$20`). "1.95" = vasm build,
"2.10" = CD. Label = nearest preceding global label of `blitz2.labels`.

| Offset | Source | Label | 1.95 | 2.10 | Instruction |
|---|---|---|---|---|---|
| `$01C20` | `asmmac:355` | `movemchk+$1A` | `C07C0813` | `02400813` | `and #$813,d0` |
| `$01C34` | `asmmac:363` | `movemchk+$2E` | `C07C0E0B` | `02400E0B` | `and #$e0b,d0` |
| `$01D48` | `asmmac:453` | `exgchk+$6` | `827C0088` | `00410088` | `or #$88,d1` |
| `$01D5C` | `asmmac:459` | `exgchk+$1A` | `827C0048` | `00410048` | `or #$48,d1` |
| `$01D64` | `asmmac:461` | `exgchk+$22` | `827C0040` | `00410040` | `.data or #$40,d1` |
| `$01DE4` | `asmmac:505` | `aslchk2+$3C` | `C27C0100` | `02410100` | `and #$100,d1` |
| `$02A30` | `blitz2.s:1141` | `exga4a5` | `C94D` | `CB4C` | `exga4a5 exg.l a4,a5` |
| `$03574` | `blitz2.s:1737` | `calcvbr+$A` | `C07C000F` | `0240000F` | `and #15,d0` |
| `$036FE` | `blitz2.s:1866` | `makeblitzlibs+$A2` | `C07C00DF` | `024000DF` | `and #255-32,d0` |
| `$0370C` | `blitz2.s:1870` | `makeblitzlibs+$B0` | `C07C00DF` | `024000DF` | `and #255-32,d0` |
| `$03A1A` | `blitz2.s:2110` | `hexascii+$6` | `C47C000F` | `0242000F` | `and #15,d2` |
| `$03A1E` | `blitz2.s:2111` | `hexascii+$A` | `D47C0030` | `06420030` | `add #48,d2` |
| `$03B82` | `blitz2.s:2241` | `expandmax+$148` | `807C0020` | `00400020` | `.notnum or #32,d0` |
| `$03B94` | `blitz2.s:2245` | `expandmax+$15A` | `907C0030` | `04400030` | `.fkit sub #48,d0` |
| `$03BA0` | `blitz2.s:2249` | `expandmax+$166` | `907C0027` | `04400027` | `sub #39,d0` |
| `$03C02` | `blitz2.s:2282` | `expandmax+$1C8` | `D07C0030` | `06400030` | `add #48,d0` |
| `$03C10` | `blitz2.s:2287` | `expandmax+$1D6` | `D07C0030` | `06400030` | `add #48,d0` |
| `$044A8` | `blitz2.s:2868` | `maketrap` | `827C4E40` | `00414E40` | `maketrap or #$4e40,d1` |
| `$047CA` | `blitz2.s:3123` | `handletoken+$28` | `C27C7F00` | `02417F00` | `and #$7f00,d1` |
| `$047DA` | `blitz2.s:3129` | `handletoken+$38` | `907C0077` | `04400077` | `sub #tnum-fnum,d0` |
| `$048A0` | `blitz2.s:3194` | `dolibtoke+$28` | `C47C00FF` | `024200FF` | `and #255,d2` |
| `$048B0` | `blitz2.s:3199` | `dolibtoke+$38` | `C67C0007` | `02430007` | `and #7,d3` |
| `$04950` | `blitz2.s:3245` | `dolibtoke+$D8` | `C27C0007` | `02410007` | `and #7,d1` |
| `$04958` | `blitz2.s:3248` | `dolibtoke+$E0` | `827C504F` | `0041504F` | `or #$504f,d1` |
| `$049F8` | `blitz2.s:3286` | `sizespec+$E` | `827C7000` | `00417000` | `or #$7000,d1` |
| `$04D5E` | `blitz2.s:3532` | `endininfo+$1A` | `803C0020` | `00000020` | `or.b #32,d0` |
| `$04D82` | `blitz2.s:3545` | `gettokeps+$6` | `C27C7F80` | `02417F80` | `and #userand,d1` |
| `$04DE8` | `blitz2.s:3577` | `dolibjsr+$14` | `827C8000` | `00418000` | `or #$8000,d1` |
| `$04E08` | `blitz2.s:3588` | `doblibjsr+$14` | `827CC000` | `0041C000` | `or #$c000,d1` |
| `$05232` | `blitz2.s:3920` | `stamigalib+$60` | `827C201F` | `0041201F` | `or #$201f,d1` |
| `$0523A` | `blitz2.s:3922` | `stamigalib+$68` | `C27C0007` | `02410007` | `.addreg and #7,d1` |
| `$05242` | `blitz2.s:3925` | `stamigalib+$70` | `827C205F` | `0041205F` | `or #$205f,d1` |
| `$052BE` | `blitz2.s:3962` | `alloxadj+$14` | `D4BC00000400` | `068200000400` | `add.l #1024,d2` |
| `$05654` | `blitz2.s:4151` | `menu5+$9A` | `C6BCFFFFFFFC` | `0283FFFFFFFC` | `and.l #$fffffffc,d3` |
| `$05C58` | `blitz2.s:4671` | `doputreg+$18` | `827C2F00` | `00412F00` | `or #$2f00,d1` |
| `$05CC4` | `blitz2.s:4702` | `doputreg+$84` | `827C1000` | `00411000` | `or #$1000,d1` |
| `$05D00` | `blitz2.s:4720` | `dosetreg+$2A` | `827C2000` | `00412000` | `or #$2000,d1` |
| `$05D12` | `blitz2.s:4726` | `dosetreg+$3C` | `827C2040` | `00412040` | `or #$2040,d1` |
| `$05D22` | `blitz2.s:4731` | `getreg2` | `807C0020` | `00400020` | `getreg2 or #32,d0` |
| `$05D42` | `blitz2.s:4741` | `getan+$4` | `807C0020` | `00400020` | `or #32,d0` |
| `$05D52` | `blitz2.s:4745` | `getan3+$4` | `907C0030` | `04400030` | `sub #48,d0` |
| `$05D6C` | `blitz2.s:4754` | `getmovem+$2` | `987C0030` | `04440030` | `sub #48,d4` |
| `$05E20` | `blitz2.s:4813` | `getea+$5E` | `807C0020` | `00400020` | `.more or #32,d0` |
| `$05E66` | `blitz2.s:4833` | `getea+$A4` | `987C0030` | `04440030` | `sub #48,d4` |
| `$05EA8` | `blitz2.s:4856` | `getea+$E6` | `987C0030` | `04440030` | `sub #48,d4` |
| `$05EAE` | `blitz2.s:4859` | `getea+$EC` | `807C0020` | `00400020` | `.notan or #32,d0` |
| `$05EF2` | `blitz2.s:4886` | `getea+$130` | `807C0020` | `00400020` | `.notsx or #32,d0` |
| `$05F70` | `blitz2.s:4928` | `getea+$1AE` | `807C0020` | `00400020` | `or #32,d0` |
| `$05F80` | `blitz2.s:4932` | `getea+$1BE` | `807C0020` | `00400020` | `or #32,d0` |
| `$05FA4` | `blitz2.s:4944` | `getea+$1E2` | `807C0020` | `00400020` | `or #32,d0` |
| `$05FE0` | `blitz2.s:4964` | `getrest+$C` | `807C0020` | `00400020` | `or #32,d0` |
| `$05FFE` | `blitz2.s:4972` | `getrest+$2A` | `907C0030` | `04400030` | `sub #48,d0` |
| `$06024` | `blitz2.s:4983` | `getrest+$50` | `807C0020` | `00400020` | `or #32,d0` |
| `$06130` | `blitz2.s:5073` | `asmit+$4` | `907C803C` | `0440803C` | `sub #$8000+fnum,d0` |
| `$0615A` | `blitz2.s:5085` | `asmit+$2E` | `807C0020` | `00400020` | `or #32,d0` |
| `$0619E` | `blitz2.s:5105` | `asmit+$72` | `C07C000F` | `0240000F` | `.tstsize and #15,d0` |
| `$063AE` | `blitz2.s:5255` | `asmit+$282` | `C07CF000` | `0240F000` | `and #$f000,d0` |
| `$063CA` | `blitz2.s:5264` | `asmit+$29E` | `C47C000F` | `0242000F` | `and #15,d2` |
| `$0655A` | `blitz2.s:5374` | `asmfixer+$14A` | `C67C0007` | `02430007` | `and #7,d3` |
| `$065A0` | `blitz2.s:5409` | `doea+$24` | `C07C000F` | `0240000F` | `and #15,d0` |
| `$065B2` | `blitz2.s:5415` | `doea+$36` | `CE7C000F` | `0247000F` | `and #15,d7` |
| `$0677A` | `blitz2.s:5574` | `fetchlibps+$8` | `C07CFF00` | `0240FF00` | `and #$ff00,d0` |
| `$0679C` | `blitz2.s:5588` | `fetchlibps+$2A` | `C47C0007` | `02420007` | `and #7,d2` |
| `$067CC` | `blitz2.s:5603` | `fetchlibps+$5A` | `C27C00FF` | `024100FF` | `and #255,d1` |
| `$067F2` | `blitz2.s:5624` | `fetchpees3` | `C07C000F` | `0240000F` | `fetchpees3 and #15,d0` |
| `$06838` | `blitz2.s:5664` | `handlep+$2` | `C27C0040` | `02410040` | `and #64,d1` |
| `$0687C` | `blitz2.s:5686` | `pushap+$20` | `827C1000` | `00411000` | `or #$1000,d1` |
| `$06894` | `blitz2.s:5700` | `handlevara` | `C47C0007` | `02420007` | `and #7,d2` |
| `$0691E` | `blitz2.s:5747` | `sendtype+$A` | `827C7000` | `00417000` | `or #$7000,d1` |
| `$06A10` | `blitz2.s:5822` | `getap` | `C47C0007` | `02420007` | `getap and #7,d2` |
| `$06D26` | `blitz2.s:6072` | `tokecode+$44` | `C07C00FF` | `024000FF` | `and #255,d0` |
| `$06D60` | `blitz2.s:6090` | `tokecode+$7E` | `C07CF000` | `0240F000` | `and #$f000,d0` |
| `$06DD8` | `blitz2.s:6129` | `tokecode+$F6` | `C07CF000` | `0240F000` | `and #$f000,d0` |
| `$07246` | `blitz2.s:6552` | `fixlibs+$4A` | `C07C000F` | `0240000F` | `and #15,d0` |
| `$0727A` | `blitz2.s:6571` | `fixlibs+$7E` | `C07C00FF` | `024000FF` | `and #255,d0` |
| `$07322` | `blitz2.s:6624` | `aligna0` | `C149` | `C348` | `aligna0 exg.l a0,a1` |
| `$07328` | `blitz2.s:6626` | `aligna0+$6` | `C149` | `C348` | `exg.l a0,a1` |
| `$075DE` | `blitz2.s:6849` | `savefile+$BA` | `C6BCFFFFFFFC` | `0283FFFFFFFC` | `and.l #$fffffffc,d3` |
| `$07688` | `blitz2.s:6897` | `savefile+$164` | `987C0013` | `04440013` | `sub #19,d4` |
| `$0768E` | `blitz2.s:6899` | `savefile+$16A` | `C87CFFFC` | `0244FFFC` | `and #$fffc,d4` |
| `$09012` | `blitz2.s:8065` | `addd0a0+$E` | `C67C0007` | `02430007` | `and #7,d3` |
| `$090F6` | `blitz2.s:8148` | `makelmul+$A4` | `827CD080` | `0041D080` | `or #$d080,d1` |
| `$09294` | `blitz2.s:8283` | `calcvar2+$172` | `827CD5C0` | `0041D5C0` | `or #$d5c0,d1` |
| `$09A2A` | `blitz2.s:8919` | `maineval2+$84` | `827C7000` | `00417000` | `or #$7000,d1` |
| `$09A2E` | `blitz2.s:8920` | `maineval2+$88` | `D27C0200` | `06410200` | `add #$0200,d1` |
| `$09BB0` | `blitz2.s:9047` | `eval5+$B6` | `827C203C` | `0041203C` | `or #$203c,d1` |
| `$09BBC` | `blitz2.s:9050` | `eval5+$C2` | `827C1000` | `00411000` | `or #$1000,d1` |
| `$0A2E6` | `blitz2.s:9576` | `bittst+$E` | `827C56C0` | `004156C0` | `or #$56c0,d1` |
| `$0A2F4` | `blitz2.s:9580` | `bittst+$1C` | `827C4880` | `00414880` | `or #$4880,d1` |
| `$0A35C` | `blitz2.s:9627` | `ritetod0+$A` | `827C2000` | `00412000` | `or #$2000,d1` |
| `$0A364` | `blitz2.s:9629` | `ritetod0+$12` | `D27C0201` | `06410201` | `add #$201,d1` |
| `$0A37C` | `blitz2.s:9639` | `ritefromd0+$E` | `827C2000` | `00412000` | `or #$2000,d1` |
| `$0A3CE` | `blitz2.s:9663` | `getelement+$48` | `C27CF000` | `0241F000` | `and #$f000,d1` |
| `$0A3D2` | `blitz2.s:9664` | `getelement+$4C` | `827C0C1F` | `00410C1F` | `or #$c1f,d1` |
| `$0A43A` | `blitz2.s:9694` | `notit+$12` | `827C4640` | `00414640` | `or #$4640,d1` |
| `$0A45A` | `blitz2.s:9704` | `fetchpi+$C` | `827C203C` | `0041203C` | `or #$203c,d1` |
| `$0A860` | `blitz2.s:9853` | `chkhex+$10` | `C07CFFDF` | `0240FFDF` | `and #$ffdf,d0` |
| `$0A8A0` | `blitz2.s:9873` | `fetchhex+$1A` | `907C0030` | `04400030` | `sub #48,d0` |
| `$0A8D4` | `blitz2.s:9889` | `fetchbin+$14` | `907C0030` | `04400030` | `sub #48,d0` |
| `$0AA0A` | `blitz2.s:10003` | `doused+$1E` | `C67C0007` | `02430007` | `and #7,d3` |
| `$0AAC2` | `blitz2.s:10073` | `doaddr+$36` | `C27CFFF8` | `0241FFF8` | `and #$fff8,d1` |
| `$0AAD4` | `blitz2.s:10077` | `doaddr+$48` | `C27CF1FF` | `0241F1FF` | `and #$f1ff,d1` |
| `$0ABA0` | `blitz2.s:10145` | `libfunction+$64` | `827C7000` | `00417000` | `or #$7000,d1` |
| `$0ABD6` | `blitz2.s:10160` | `libfunction+$9A` | `827C2000` | `00412000` | `or #$2000,d1` |
| `$0ACA2` | `blitz2.s:10233` | `amigalib+$88` | `827C201F` | `0041201F` | `or #$201f,d1` |
| `$0ACAA` | `blitz2.s:10235` | `amigalib+$90` | `C27C0007` | `02410007` | `.addreg and #7,d1` |
| `$0ACB2` | `blitz2.s:10238` | `amigalib+$98` | `827C205F` | `0041205F` | `or #$205f,d1` |
| `$0ACEC` | `blitz2.s:10252` | `amigalib+$D2` | `827C2000` | `00412000` | `or #$2000,d1` |
| `$0AEE6` | `blitz2.s:10412` | `qmark+$12` | `827C203C` | `0041203C` | `or #$203c,d1` |
| `$0AF74` | `blitz2.s:10485` | `varcont+$E` | `827C4880` | `00414880` | `or #$4880,d1` |
| `$0B082` | `blitz2.s:10576` | `afunction+$A4` | `827C2000` | `00412000` | `or #$2000,d1` |
| `$0B11C` | `blitz2.s:10628` | `stvar+$26` | `827C7000` | `00417000` | `or #$7000,d1` |
| `$0B172` | `blitz2.s:10651` | `variable2+$22` | `827C200A` | `0041200A` | `or #$200a,d1` |
| `$0B194` | `blitz2.s:10660` | `variable2+$44` | `C47C00FF` | `024200FF` | `.more and #255,d2` |
| `$0B1D2` | `blitz2.s:10681` | `variable2+$82` | `827C1000` | `00411000` | `.word or #$1000,d1` |
| `$0B232` | `blitz2.s:10719` | `fetchnum+$E` | `807C0020` | `00400020` | `or #32,d0` |
| `$0B2C4` | `blitz2.s:10762` | `fetchpoint2+$52` | `807C0020` | `00400020` | `or #32,d0` |
| `$0B32C` | `blitz2.s:10807` | `getsign+$A` | `987C0030` | `04440030` | `sub #48,d4` |
| `$0B340` | `blitz2.s:10812` | `getsign+$1E` | `907C0030` | `04400030` | `sub #48,d0` |
| `$0B3C6` | `blitz2.s:10847` | `fetchdec+$20` | `907C0030` | `04400030` | `sub #48,d0` |
| `$0B422` | `blitz2.s:10875` | `fetchnum3+$46` | `827C303C` | `0041303C` | `or #$303c,d1` |
| `$0B43E` | `blitz2.s:10884` | `fetchnum3+$62` | `827C203C` | `0041203C` | `or #$203c,d1` |
| `$0B708` | `blitz2.s:11123` | `muld1+$2A` | `927C0008` | `04410008` | `sub #8,d1` |
| `$0B716` | `blitz2.s:11128` | `muld1+$38` | `827CE188` | `0041E188` | `or #$e188,d1` |
| `$0B730` | `blitz2.s:11135` | `muld1+$52` | `827CC0FC` | `0041C0FC` | `or #$c0fc,d1` |
| `$0B90C` | `blitz2.s:11276` | `makevsize+$8` | `927C8000` | `04418000` | `sub #$8000,d1` |
| `$0BACE` | `blitz2.s:11451` | `userjsr+$10` | `C27C00FF` | `024100FF` | `and #255,d1` |
| `$0BD88` | `blitz2.s:11688` | `toreg+$82` | `C47C0007` | `02420007` | `.skipad and #7,d2` |
| `$0BDE0` | `blitz2.s:11721` | `toreg+$DA` | `C27C0007` | `02410007` | `and #7,d1` |
| `$0BDF4` | `blitz2.s:11729` | `toreg+$EE` | `827CB07C` | `0041B07C` | `or #$b07c,d1` |
| `$0BE4C` | `blitz2.s:11761` | `toreg+$146` | `C47C0007` | `02420007` | `and #7,d2` |
| `$0BE60` | `blitz2.s:11772` | `toreg+$15A` | `C27C0007` | `02410007` | `and #7,d1` |
| `$0BE6C` | `blitz2.s:11776` | `toreg+$166` | `C47C0E00` | `02420E00` | `and #$e00,d2` |
| `$0BEF2` | `blitz2.s:11823` | `toreg+$1EC` | `827C1000` | `00411000` | `.notst or #$1000,d1` |
| `$0BF42` | `blitz2.s:11853` | `makeregop+$2A` | `C47C0007` | `02420007` | `.skip and #7,d2` |
| `$0BF62` | `blitz2.s:11867` | `ifchar+$10` | `907C003C` | `0440003C` | `sub #60,d0` |
| `$0C382` | `blitz2.s:12220` | `getmaxel+$42` | `827CB07C` | `0041B07C` | `or #$b07c,d1` |
| `$0C5BC` | `blitz2.s:12405` | `doread+$40` | `C23C000F` | `0201000F` | `and.b #15,d1` |
| `$0CCCA` | `blitz2.s:12956` | `collif+$22` | `C07C0003` | `02400003` | `and #3,d0` |
| `$0CD4E` | `blitz2.s:13015` | `selpushb` | `C07C00FF` | `024000FF` | `selpushb and #255,d0` |
| `$0CD5A` | `blitz2.s:13023` | `selpushsf` | `C07C00FF` | `024000FF` | `selcomb and #255,d0` |
| `$0D42C` | `blitz2.s:13574` | `dofunc2+$128` | `827C1000` | `00411000` | `.wgot or #$1000,d1` |
| `$0D59A` | `blitz2.s:13688` | `procfixer+$A2` | `C87C7FFF` | `02447FFF` | `and #32767,d4` |
| `$0D5A6` | `blitz2.s:13694` | `procfixer+$AE` | `C27C000F` | `0241000F` | `and #15,d1` |
| `$0D7AE` | `blitz2.s:13834` | `doendif3+$10` | `C07C00FF` | `024000FF` | `and #255,d0` |
| `$0D91E` | `blitz2.s:13942` | `doend+$EC` | `827C7200` | `00417200` | `or #$7200,d1` |
| `$0DAFE` | `blitz2.s:14075` | `endstate+$116` | `C27C0007` | `02410007` | `and #7,d1` |
| `$0DBDC` | `blitz2.s:14144` | `donext+$2A` | `927C0012` | `04410012` | `sub #18,d1` |
| `$0DD90` | `blitz2.s:14269` | `dofor+$94` | `C47C00FF` | `024200FF` | `and #255,d2` |
| `$0DFAA` | `blitz2.s:14410` | `dolet2+$104` | `C47C00FF` | `024200FF` | `and #$ff,d2` |
| `$0E0A4` | `blitz2.s:14473` | `dolet2+$1FE` | `827C1000` | `00411000` | `or #$1000,d1` |
| `$0EF4A` | `blitz2.s:15143` | `typeshow+$12` | `927C000B` | `0441000B` | `sub #11,d1` |
| `$102C4` | `options.src:7` | `optreqprep+$14` | `D0BC00000020` | `068000000020` | `add.l #(arrowf-uparrow)/2,d0` |
| `$11B44` | `blitz2.s:15828` | `menu7+$120` | `D47C0030` | `06420030` | `add #48,d2` |
