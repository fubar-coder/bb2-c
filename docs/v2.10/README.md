# Blitz Basic 2.10 compared with the ported versions

> As of 28.10.2026. Preparation for porting the changes of Blitz Basic
> 2.10 (the release on the CD "Ultimate Blitz Basic 2.1",
> `archive/cd/ultimate-blitz-basic-2.1/`). Static comparison of binaries
> only; nothing was run in the emulator.
>
> **Ported on 29.10.2026** (section 4): the C port is 2.10 – version 210,
> the `/` test (#2), the `Select` templates as Devpac encodes them. The
> output of the compiler is compared with `BLITZ2/BLITZ2` of the CD
> (corpus, `corpus_cd`, `ted_ab`); the bug-orig tests keep the 1.95
> reference build.
>
> Evidence: **[B]** binary comparison (reproducible with the commands in
> section 3), **[S]** source, **[Doku]** text on the CD, **[Schluss]**
> conclusion, not proven.

| File | Contents |
|---|---|
| [blitz2.md](blitz2.md) | compiler 1.95 (source) vs 2.10 (CD): changelog, the two real changes, encodings, label alignment, per-routine table, bug-orig status |
| [ted.md](ted.md) | TED 1.1 (source) vs the CD's TED: changelog, per-routine table; feature/menu changelog of the later editors (TED 2.1, SuperTED 2.1/2.23/2.50) |
| [libs.md](libs.md) | `acidlibs`/`deflibs`: changelog, versions on the CD, the port's reference, token changes of the later acidlibs, their bug-orig status |

## 1 Summary

* **The compiler 2.10 is the archived `blitz2.s`** (`version equ 195`),
  assembled with Devpac, with `version equ 210` and **one changed source
  line**: `loadblitzlibs` (`blitz2.s:6400`) tests the last character of
  `libsdir` for `/` instead of `:` – this fixes issue **#2** (userlibs
  path `blitz2:userlibs//dir`). Code size, layout and all 3149 relocation
  offsets are identical; every label of the 1.95 reference build is at the
  same offset in 2.10. The other 308 of the 314 differing bytes are assembler
  encodings (Devpac `ANDI/ORI/SUBI/ADDI` for `and/or/sub/add #imm,Dn`,
  swapped `exg` operands) and padding. No new keywords, options, error
  messages or routines. [B]
* Two of the encoding differences are **code templates copied into
  compiled programs** (`selpushb`, `selcomb`: `and #255,d0` for
  `Select` on bytes). So programs compiled by 2.10 differ from those of
  the vasm reference build and of the C port by these bytes – same
  behaviour. [B]
* **The CD's TED is `ted.s` 1.1**, assembled with Devpac: same size,
  same relocations, only 48 instruction encodings + 1 padding word differ; all 1103
  labels at the same offsets. The other editors on the CD (TED 2.1
  "NewTED", SuperTED 2.1/2.23/2.50 incl. `BUM10A/TED223`) are separate,
  heavily rewritten successors without source. [B]
* **acidlibs**: one version on the CD (`BLITZ2`, `BUM9A`, `BUM10B`
  identical) – exactly the port's reference. **deflibs**: the port's
  reference `BLITZ2/DEFLIBS` (= `BUM9A/BLITZ2/DEFLIBS`, 39 AmigaOS
  libraries); the other three deflibs files on the CD contain the same
  AmigaOS libraries byte-identical (some leave out crm.library and
  colorwheel.gadget) plus third-party libraries. [B]
* The CD contains **later versions of six acidlibs** outside the release:
  the four "fixed Acidlibs" by Steve Matty mentioned in the BUM10 readme
  (`asllib`, `gtlib`, `gadgetslib`, `windowslib`, in
  `BSS2/REDBLITZLIBS/LIBRARIES/BASIC/`), the RedDebugger `debuglib`
  and Romulus' crunch library (lib 74). They are not part of 2.10 (BUM10:
  "they're not included"). They add commands (multi-select ASL requests,
  waiting on user signals, prop/arrow gadgets, GTActivateGadget …) and fix
  only a few bug-orig issues (fully: windowslib #317, #329; partly: #318,
  #169, #173), while bringing new bugs of their own (windowslib WindowTags
  crash on failure, asllib multi-select state, gtlib UserData) –
  [libs.md](libs.md) §5.
* **Bug-orig issues fixed in 2.10: only #2.** All other compiler issues
  (#1, #3–#32, #34–#42, #325), all TED `BUG(orig)` places and all library
  issues are unchanged in the 2.10 release, because the code is the same.

## 2 Inventory

All paths relative to `archive/cd/ultimate-blitz-basic-2.1/`. Search:
every file named `BLITZ2`, `TED`, `ACIDLIBS`, `DEFLIBS`, `DEFAULTDBUG`,
plus a search of all files for "Blitz Basic 2 Version", "Ted Version",
"SuperTED", "Release Version" [B].

### 2.1 Compiler

| File | Size | SHA-256 | Version | Same as |
|---|---|---|---|---|
| `BLITZ2/BLITZ2` | 86 216 | `0c08492ea83ce85e795fb9fd9ac5e8c2fc3ec63d117dbc47b865655cb4d88c2b` | "Blitz Basic 2 Version 2.10", "(C)opyright 1995 - Acid Software", "Release Version" | – |
| `BUM9A/BLITZ2/BLITZ2` | 86 216 | same | 2.10 | `BLITZ2/BLITZ2` |
| `BUM10B/BLITZ2/BLITZ2` | 86 216 | same | 2.10 | `BLITZ2/BLITZ2` ("Blitz2 compiler (unchanged from BUM9)", `BUM10B/BLITZ2/RELEASENOTES`) |

There is **only one compiler binary on the CD**: 2.10.

### 2.2 Editors

| File | Size | SHA-256 | Version | Remark |
|---|---|---|---|---|
| `BLITZ2/TED` | 59 556 | `4be00b6fb4a10f21f2179ae72ba8f2d16f1b099a6abd71924874981a53e05036` | "Blitz Basic 2 - Ted Version 1.1" | = `ted.s` 1.1 ([ted.md](ted.md) §1) |
| `BUM9A/BLITZ2/TED`, `BUM10B/BLITZ2/TED` | 59 556 | same | 1.1 | identical |
| `EXTRAS/NEWTEDITOR/TED` | 57 732 | `9a05dfa99c499e21725423039f41527e294d1e44360b8860d239bb8de42be758` | TED V2.1 (01.11.1995) | WB 2.0+ |
| `BUI5/SUPERTED/TED` | 57 732 | `2862a00d696752d28f7262ad97ad827539a74fafb9fd76d0dd822ee73273f634` | TED V2.1 (01.11.1995) | 1 byte differs from the above (menu shortcut) |
| `INTERNET/NEWTED/TED` | 57 668 | `97fcd6d3dab6ffe88c7b44415500c0a8d51b57ec856b6c4a393aad9b98d2461b` | TED V2.1 (27.10.1995) | |
| `BUM9A/BUM9EXAMPLES2/HARDDISKUSERS/TED` | 57 088 | `025dcd621e67ea3ae0f8085182eabda3ded0697a7dc312f034c2aef3043a0930` | SuperTED 2.1 | BUM9 |
| `BUM10A/TED223/TED` | 65 900 | `639f0409e1535dcbfc1bcb25ffa1a4e7a1dc52ed03fad77ddb7236190b008865` | SuperTED 2.23 | BUM10 |
| `BSS3/SUPERTED/PROGRAMS/TED` | 76 240 | `7962c8a2de55afd5c9bb91e5b090ca848b9c5da14a329968ab67335d546c53b9` | SuperTED 2.50 | BSS3 (RWE) |

### 2.3 Library files

| File | Size | SHA-256 | Libraries | Same as |
|---|---|---|---|---|
| `BLITZ2/ACIDLIBS` | 219 020 | `98aeae72f957749a4988264bd2bfcccca31b18576438d0665182d0917a340676` | 83 | `BUM9A/BLITZ2/ACIDLIBS`, `BUM10B/BLITZ2/ACIDLIBS` – **the port's reference** |
| `BLITZ2/DEFLIBS` | 55 592 | `40b1ec569a2bb9da48e4f8f68e5a9b4e962b3cf0790afdfa5f9da5f1c4e2dcf5` | 39 | `BUM9A/BLITZ2/DEFLIBS` – **the port's reference** |
| `BUM10B/BLITZ2/DEFLIBS` | 91 692 | `18917bb8e8137012c593e57dbf1c971a12fd915d6ba3dc4445fd1e5e6935bf08` | 43 | – |
| `BUM9A/BUM9EXAMPLES2/HARDDISKUSERS/DEFLIBS` | 219 256 | `119b2e156c06ae7e08c548f0682c469989906bd9e84d7e8c2867c32beb5fa88c` | 72 | – ("large deflibs", BUM9) |
| `EXTRAS/BIGDEFLIBS/DEFLIBS` | 159 232 | `d104e2956466ad038731baf46f94a523feab4b5d1a7ba4d2a171f4ecb9a01c8b` | 72 | – |

### 2.4 Debugger (not ported, for completeness)

| File | Size | SHA-256 (start) |
|---|---|---|
| `BLITZ2/DEFAULTDBUG` | 32 032 | `cb7f60ab6bb025c5…` |
| `BUM9A/BLITZ2/DEFAULTDBUG` = `BUM10B/BLITZ2/DEFAULTDBUG` | 34 092 | `4671364b6ddedaea…` ("New small debugger", BUM9) |
| `BUM9A/BUM9EXAMPLES2/HARDDISKUSERS/DEFAULTDBUG` | 154 508 | `80e11c270e4e801e…` ("New big debugger") |
| `EXTRAS/NEWDEBGGER/DEFAULTDBUG` | 152 268 | `8a5a917b7c868659…` |

The tests of this repository use `BLITZ2/DEFAULTDBUG` of the CD (e.g. `programs/ted/tests/oracle/ted-oracle.cmake`).
BUM9 replaced the small debugger because of "Crashes when using Functions
and Statements (due to the small debugger not liking the new debug
library …)" (`BUM9A/BLITZ2/RELEASENOTES`) [Doku]; acidlibs (with
`debuglib`) is the same in 2.1 and BUM9, so "new debug library" refers to
the state of the 2.1 release [Schluss]. Not examined further.

## 3 Method

1. **Reference builds with labels** (exist after a normal build):
   `build/amiga/programs/blitz2/ref/{blitz2,blitz2.map,blitz2.labels}` and
   `build/amiga/programs/ted/ref/{ted,ted.map,ted.labels}`.
2. **Code hunks** of both sides:
   ```
   m68k-amigaos-objdump -h FILE                 # one .text section, sizes
   m68k-amigaos-objcopy -O binary -j .text FILE OUT.bin
   cmp -l ref.bin cd.bin | wc -l                # differing bytes
   ```
   blitz2: both `$11F60` bytes, 314 bytes differ; TED: both `$D4FC`,
   98 bytes differ.
3. **Relocations**: hunk files parsed (HUNK_RELOC32 `$3EC`, vasm's
   HUNK_RELOC32SHORT `$3F7`); the sorted offset lists are equal (3149 and
   1245 entries).
4. **Classification**: `m68k-amigaos-objdump -D -b binary -m m68k:68000`
   of both code hunks; at every differing place the instruction of both
   sides compared after mapping `andi→and`, `ori→or`, `subi→sub`,
   `addi→add`; `exg` with swapped operands counted as equal. Every place
   mapped to the source line via `blitz2.map`/`ted.map` (tables in the
   appendices of [blitz2.md](blitz2.md) and [ted.md](ted.md)).
5. **Libraries**: BigLib files split (`long length` + load file, up to a
   long 0; `tools/lib/biglib/biglib.h`); per library number (word at
   code offset 4) SHA-256 over code hunk + relocations; token tables read
   as in `tools/lib/blitzlib/blitzlib.c` and compared by name and help
   text; all hunk files of the CD with a library header (`MOVEQ #0,d0:RTS`
   or 4 zero bytes) scanned and matched by number.
6. **Other editors / library versions**: instruction alignment
   (objdump, numbers masked, `difflib.SequenceMatcher` on the instruction
   sequence) to map labels and find changed blocks.

**Raw material** (git-ignored, not in the repository):
`.cache/v2.10-analysis/` holds the extracted code hunks, all
disassemblies, the label maps (`blitz2/labelmap_195_210.tsv`,
`ted/labelmap_ref_cd.tsv`, `ted/map_*.tsv` for the later editors), the
diff tables, the library lists and side-by-side disassemblies, the issue
texts and the throw-away scripts (Python, `python3 -I`; not in the
repository because of the project rule "no Python"). Its `INDEX.md`
explains each file and how to regenerate it (prerequisites: the CD via
`cmake -P cmake/fetch-originals.cmake` and the reference builds
`blitz2_ref`/`ted_ref`). If the comparison should become a permanent
check, it belongs into a C tool next to `refcheck asm` (`tools/refcheck`, which already
compares instructions via `objdump`).

## 4 Effort for porting 2.10

| Part | Change | Effort |
|---|---|---|
| compiler | `blitz2version` 195 → 210 (`programs/blitz2/src/main.h:36`), `loadblitzlibs` test `'/'` (`asmlibs.c`, close #2); optional `selpushb`/`selcomb` with `0x0240` (`tokens1.c:77/81`) for byte-identical output | < 1 day incl. tests; decision needed on the reference for the corpus (vasm build of 1.95 vs CD binary 2.10, see [blitz2.md](blitz2.md) §6) |
| TED | none – the shipped TED is TED 1.1 | – |
| acidlibs, deflibs | none – the port's references are the 2.1 release files | – |
| optional: later acidlib fixes | the four BUM10 "fixed Acidlibs", debuglib, crunch: not part of 2.10 | see [libs.md](libs.md) §5 |
| optional: SuperTED 2.23 | new disassembly without source | large (weeks), [ted.md](ted.md) §2.2 |

So "porting 2.10" is essentially a version bump plus one bug fix.

Done (29.10.2026): `blitz2version` 210 (`main.h`), `loadblitzlibs` tests
`'/'` (`asmlibs.c`, #2 fixed), `selpushb`/`selcomb` with `0x0240`
(`tokens1.c`), all as `DIFF` against `blitz2.s`. The C port differs from
the 1.95 state in 8 bytes (version text 3, `/` 1, templates 4). The
reference for the compiler's output is the CD binary (question 2 of
section 5); `tests/data/blitz2-cd.tsv` is generated by it (23 programs
with `Select` on bytes changed their hash). The corpus test `t14_select`
covers the templates. Not done: the later acidlibs (only documented,
question 3 of section 5), a permanent check of the archive against the
CD binary (the archive does not change). The
more interesting consequence: the archived source is not exactly the
released 2.10; #2 is a bug
that the vendor fixed in the release.

## 5 Open questions

1. Is the archived `blitz2.s` (as of 1995-09-14 in `bb2src.zip`) older or
   younger than the 2.10 binary? `version equ 195` and the missing `/`
   fix suggest a state before the release build, or a source that was
   reset later; the release notes date 2.1 to 1995 as well. Not decidable
   from the binaries; stays open, it does not matter for the port.
2. ~~Should the reference become the CD binary (2.10)?~~ Yes (29.10.2026)
   for the compiler's output (corpus, `corpus_cd`, `ted_ab`); #2 is fixed
   as in 2.10. The bug-orig tests keep the 1.95 build as the reference:
   a test is valid only if the reference shows the bug, and 2.10 shows
   all of them except #2.
3. ~~The four later acidlibs, debuglib and crunch: port them as an
   alternative (later state) or only document?~~ Only document
   (29.10.2026): they are files on the CD outside the release, in no
   acidlibs file of the CD, so not part of 2.10 ([libs.md](libs.md) §4,
   §5). Where they fix a bug-orig issue, they can serve as a model when
   that issue is fixed.
4. ~~`docs/formats.md` §1.1 names `$F918` for the option table~~:
   corrected there (`$F936`, `optspnts`).
5. `docs/architecture.md` §8 "order acidlibs/deflibs when loading in
   v2.10 – not checked": answered – the code is the same as 1.95
   (acidlibs first).
