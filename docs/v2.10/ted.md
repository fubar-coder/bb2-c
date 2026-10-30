# Editor: TED 1.1 (source) vs the TED of the CD, and the other editors

> As of 28.10.2026. Static comparison of binaries, nothing executed.
> Evidence: **[B]** binary comparison (commands in [README.md](README.md),
> "Method"), **[S]** source, **[Schluss]** conclusion, not proven.

Compared:

* **TED 1.1 (source)** = the reference build of this repository
  (`cmake --build --preset amiga --target ted_ref`,
  `build/amiga/programs/ted/ref/ted`, vasm `-devpac`, with `ted.map` and
  `ted.labels`) from `archive/sources/ted-1.1/ted.s`.
* **TED of the CD** = `archive/cd/ultimate-blitz-basic-2.1/BLITZ2/TED`
  (59 556 bytes, SHA-256 `4be00b6f…3e05036`, "Blitz Basic 2 - Ted Version
  1.1", "(C)opyright 1991 - Acid Software"). `BUM9A/BLITZ2/TED` and
  `BUM10B/BLITZ2/TED` are byte-identical. This is the editor that the
  Blitz 2.1 release (compiler 2.10) ships and loads as `blitz2:ted`
  (`BUM10B/BLITZ2/RELEASENOTES`: program disk unchanged since BUM9).

## Changelog

### TED of the 2.10 release (TED 1.1 source → `BLITZ2/TED`)

**Nothing changed** – the shipped editor is the archived `ted.s` (TED 1.1)
assembled with Devpac (section 1) [B]:

| Category | Change |
|---|---|
| Features, menus, keys, requesters, texts | none |
| Option block (`l:blitzeditor.opts`), fonts, pointer, icon | none |
| Interface to the compiler (`edstruct`/comdata, external menus) | none |
| Built-in file requester, `rawed` | none |
| Routines | none added/removed; of 1103 labels, 30 contain encoding-only differences and 1 the hunk padding (per-routine table below) |

### The later editors (not part of the 2.10 release)

For completeness, the feature changes of the successors on the CD,
read from their menu tables and strings [B] (behaviour not checked):

| Area | TED 1.1 (2.1 release) | TED 2.1 "NewTED" (10/11.1995) | SuperTED 2.1 (BUM9) | SuperTED 2.23 (BUM10) | SuperTED 2.50 (BSS3) |
|---|---|---|---|---|---|
| GUI | own menus (upper case), own requesters, own screen | GadTools menus, ASL requesters, `diskfont`, WB 2.0+ | as 2.1 | as 2.1, menu `Custom` | as 2.23 |
| Menus | PROJECT, EDIT, SOURCE, SEARCH (+ COMPILER from blitz2) | Project, Edit, Source, Search, Compiler | same + `Open New...` | same + `Custom` | same + `Quit All...`, `Iconify...`, `List...`, `Previous File...` (F9), `Next File...` (F10), document switching 1–5 |
| Project | NEW, LOAD, SAVE, DEFAULTS, ABOUT, PRINT, CLI, CLOSE/OPEN WORKBENCH, QUIT | New, Load, Save, Save As, **Save ASCII**, **Include File**, Print, Open/Close WB, NewCLI, **Prefs**, About, Quit | + Open New | + Open New | + Quit All, Iconify |
| Edit/block | COPY, KILL, FORGET, BLOCK TO DISK, INSERT FROM DISK, INSERT/DELETE LINE, DELETE RIGHT, JOIN, BLOCK TAB/UNTAB, MARK INDENT | **Cut/Copy/Paste (clipboard.device)**, Forget, Copy Marked, Kill, Save Block As, Insert/Delete Line, **Undelete Line**, Join, Delete To EOL, Block TAB/UNTAB, Mark Indent | same | + **Block Comment/Uncomment** | + List |
| Search | FIND (requester: Find/Replace, Next, Previous, Replace, Replace All, Case sensitive) | Find, Next, Previous, Replace, Replace All as menu items | same | same | same |
| Compiler menu | supplied by blitz2 | in the editor: Compile & Run, Run, Create Executable, Compiler Options, Create Resident, View NewTypes, CLI Arguments, Calculator, Reload All Libs, Choose DEBUG Module, Change Dir | same | same | same |
| Settings | `l:blitzeditor.opts` | prefs requester | `ENV:`/`ENVARC:ted.prefs`, `ted.library` (LIBS:) | same | same |
| Help | `blitz2:help` | `blitz2:help` | same | + **RedHelp** (`Blitz2:RedHelp/RED_HELP`, "Help...", "Library Help...", AmigaGuide helps) | same |
| ARexx | – | – | – | **ARexx port `TED_REXX1`…** | same |

Menu shortcuts (Amiga key) of the successors: L, ?, Q, A, I, P, X, C, V,
W, Z, K, D, U, J, Y, `]`, `[`, T, B, G, F, N, R, `#`, M, E, O, `/` or `-`
(CLI Arguments), H, `\`, `.`, e; SuperTED 2.23 adds `}` `{` (block
comment). TED 1.1 has its own key handling (`rawed`, `ted.s:5976` ff.)
without menu shortcuts for most functions [S].

## 1 Result: the CD's TED is `ted.s` 1.1, assembled with Devpac

| | TED 1.1 (vasm build) | CD `BLITZ2/TED` |
|---|---|---|
| hunks | 1 × HUNK_CODE | 1 × HUNK_CODE |
| code size | `$D4FC` = 54 524 bytes | `$D4FC` = 54 524 bytes |
| relocations | 1245 (HUNK_RELOC32SHORT) | 1245 (HUNK_RELOC32), **same offsets** |
| differing bytes | – | 98 bytes (46 contiguous places, 49 instructions/data words) |
| file size | 57 060 | 59 556 (relocation format only) |

All of them are assembler encodings (48 instructions) or padding (1 word) [B]:

* `and/add/or/sub #imm,Dn` (also `.b`/`.l`): vasm `$Cx7C/$Dx7C/$8x7C/$9x7C`
  (and `…BC`, `…3C`), Devpac `ANDI/ADDI/ORI/SUBI`: `and` 26, `add` 12, `or` 5,
  `sub` 3;
* `exg a3,a5` in `tokprint` (`ted.s:4158`, `:4160`, offsets `$45A2`, `$45A8`):
  vasm `$C74D`, Devpac `$CB4B` (operands swapped, same instruction);
* `$D4FA`, behind `font8` (`ted.s:8071`, end of the hunk): longword
  padding, vasm `4E71`, Devpac `0000`.

**There is no functional difference.** In particular the data (fonts, menus,
requesters, option block, the embedded file requester `filerequest` at
`$8CE0` and `rawed` at `$73E0`) is byte-identical. The statement in
`docs/ted/reference.md` §1 that the CD binary is "a different build" is
thus confirmed and made precise: different assembler, same source.

### 1.1 Alignment by label

Same size, same layout, same relocation offsets: **all 1103 labels of
`build/amiga/programs/ted/ref/ted.labels` are at the same offset in the CD
binary** (identity mapping). The checks of the port against the reference
build (`tools/checkdata`, `programs/ted/checkdata.tsv`) therefore hold
against the CD binary as well.

### 1.2 Bug-orig status

The port documents the bugs of the original as `BUG(orig)` places in
`programs/ted/src` (surveys in `docs/ted/`), each with its GitHub issue:
#340 (`[ted/a0toname]`) and #346–#389 (label `ted`, titles `[ted/…]`;
#360, the relocation of the embedded file requester, is not ported).
Since the CD's TED is the same code, **all of them are present in the
shipped TED unchanged** [B]. Nothing was fixed between
the archived source and the shipped editor.

## 2 The other editors on the CD (not TED 1.1)

The CD contains six further `ted` binaries. They are **later, different
editors** (TED 2.1 "NewTED", SuperTED 2.1/2.23/2.50), developed from
`ted.s` but heavily rewritten; no sources exist for them on the CD or in
the Aminet archive (`amiblitz2source/ted2.bb2` is a different editor, see
`docs/ted/reference.md` §3.3).

| File | Size | SHA-256 | Version text | Code | Relocs |
|---|---|---|---|---|---|
| `EXTRAS/NEWTEDITOR/TED` | 57 732 | `9a05dfa9…be758` | "Blitz Basic 2.1 - Ted Version 2.1", `$VER: TED V2.1 (01.11.1995)` | 49 732 | 1987 |
| `BUI5/SUPERTED/TED` | 57 732 | `2862a00d…f634` | same as above | 49 732 | 1987 |
| `INTERNET/NEWTED/TED` | 57 668 | `97fcd6d3…d2461b` | "Blitz Basic 2 - Ted Version 2", `$VER: TED V2.1 (27.10.1995)` | 49 672 | 1986 |
| `BUM9A/BUM9EXAMPLES2/HARDDISKUSERS/TED` | 57 088 | `025dcd62…0a0930` | "SuperTED v2.1", `$VER: SuperTED 2.1` | 48 632 | 2101 |
| `BUM10A/TED223/TED` | 65 900 | `639f0409…b008865` | "NuBlitz Basic v2.1 - SuperTED v2.23", `$VER: SuperTED 2.23` | 56 256 | 2398 |
| `BSS3/SUPERTED/PROGRAMS/TED` | 76 240 | `7962c8a2…46c53b9` | "NuBlitz Basic v2.1 - SuperTED v2.50" | 64 464 | 2931 |

* `BUI5/SUPERTED/TED` differs from `EXTRAS/NEWTEDITOR/TED` in **one byte**
  (file offset `$A938`): the menu shortcut of "CLI Arguments..." is `-`
  instead of `/`.
* Lineage according to the release notes: TED 2.1 = "New Teditor version
  2.1 (WB2.0+ amigas only)" (BUM9, `BUM9A/BLITZ2/RELEASENOTES`); the BUM9
  hard-disk TED already calls itself SuperTED 2.1; SuperTED 2.23 = BUM10
  ("Yes, now it's version 2.23 and it's SuperTed not NewTed!",
  `BUM10B/BUM10EXAMPLES1/BUM10DOCS/BUM10_README`); 2.50 = BSS3 (Red When
  Excited, 1996).

### 2.1 Relation to TED 1.1

All six keep the interface to the compiler: offset 0 `bsr.w edstart`,
offset 4 `outside: move #-1,returncode` (`ted.s:22–24`; bytes
`6100xxxx 33FCFFFF xxxx`), i.e. they are loaded by blitz2 2.10 as
`blitz2:ted` like TED 1.1 [B].

Instruction alignment against TED 1.1 (objdump of the code hunks, numbers
masked, `difflib` on the instruction sequence) [B]:

| Editor | instructions | identical to TED 1.1 | TED 1.1 labels found |
|---|---|---|---|
| TED 2.1 (`EXTRAS/NEWTEDITOR`) | 11 515 | 5204 (35.7 % of TED 1.1, 45.2 % of TED 2.1) | 274 of 1103 |
| SuperTED 2.1 (BUM9) | 11 249 | 4169 (28.6 %) | 269 |
| SuperTED 2.23 (BUM10A) | 12 744 | 4219 (29.0 %) | 265 |
| SuperTED 2.50 (BSS3) | 14 608 | 4165 (28.6 %) | 264 |

(TED 1.1 itself: 14 566 "instructions", of which about 3900 are the
embedded file requester and a few thousand are data decoded as code; so
roughly half of the real editor code of TED 1.1 is still instruction for
instruction in TED 2.1 [Schluss].)

Large blocks that are missing in all of them compared with TED 1.1: the
built-in file requester `filerequest` (`ted.s:7967`, about 3900
instructions), the TED requesters (`erasereq…`, `defaultreq…`), the custom
menu code (`menu1item…`/`menu3item…`). Added instead: GadTools menus,
`asl.library` requesters, screen-mode handling (`newscreen`,
`extnewscreen`), and in SuperTED much more. Strings only in SuperTED 2.23
(selection): `AREXX Port : TED_REXX1`, `rexxsyslib.library`,
`clipboard.device`, `ENV:ted.prefs`/`ENVARC:ted.prefs`, `Prefs...`,
`Block Comment`/`Block Uncomment`, `Auto In_dent`, `Choose _Font`,
`_ModeID`, `Library Help...` (`RUN >NIL: Blitz2:RedHelp/RED_HELP …`),
`LIBGUIDE`, `ted.library`; credits "Mark Tiffany", "Nigel Hughes",
"Mike Richards". TED 1.1 strings such as `l:blitzeditor.opts`,
`console.device`, `BLOCK TO DISK`, `INSERT FROM DISK` are gone.

### 2.2 Assessment

* **TED 2.10 does not exist as such**: the editor of the 2.1 release is
  TED 1.1, which this repository already ports (the port is complete with
  respect to it).
* TED 2.1 / SuperTED 2.x are separate products. Porting one of them would
  be a new disassembly project without source: about 50–64 KB of code, of
  which only 30–45 % of the instructions can be taken over from the TED 1.1
  alignment; the rest (GadTools/ASL GUI, ARexx, prefs, clipboard, help
  system) would have to be reconstructed from the binary. Effort: several
  times the size of the TED 1.1 port minus the core (tokeniser, line
  storage, which appear to be shared) [Schluss]. Not recommended as part of
  "porting 2.10".

## Per-routine table (TED 1.1 vasm build ↔ CD TED)

Global labels with at least one differing byte; all are encoding-only
(`enc`) or padding (`pad`); the other 1072 labels are byte-identical.
Full map of all 1103 labels with both offsets:
`.cache/v2.10-analysis/ted/labelmap_ref_cd.tsv`; label maps TED 1.1 →
TED 2.1 / SuperTED (from the instruction alignment, `-` = not found):
`.cache/v2.10-analysis/ted/map_*.tsv`.

| Label | Source | Offset | Differing bytes | Class |
|---|---|---|---|---|
| `mouseinit` | `ted.s:184` | `$0004AA` | 2 | enc:2 |
| `defaults` | `ted.s:401` | `$000AA2` | 10 | enc:10 |
| `cursdown` | `ted.s:1041` | `$001BEE` | 2 | enc:2 |
| `cursup` | `ted.s:1074` | `$001C68` | 2 | enc:2 |
| `split` | `ted.s:1313` | `$002002` | 2 | enc:2 |
| `return` | `ted.s:1402` | `$00215A` | 2 | enc:2 |
| `menu` | `?:0` | `$002396` | 4 | enc:4 |
| `blocksave` | `ted.s:1671` | `$002502` | 2 | enc:2 |
| `bcopyok` | `ted.s:1832` | `$00272C` | 8 | enc:8 |
| `help` | `?:0` | `$0033BA` | 6 | enc:6 |
| `getstuff` | `ted.s:2781` | `$0034A6` | 2 | enc:2 |
| `numtoascii` | `?:0` | `$00398C` | 4 | enc:4 |
| `allocline` | `ted.s:3349` | `$003BCA` | 2 | enc:2 |
| `isithere2` | `ted.s:3530` | `$003DAE` | 4 | enc:4 |
| `freeupa5` | `?:0` | `$003FA2` | 2 | enc:2 |
| `makeline` | `?:0` | `$0040FE` | 2 | enc:2 |
| `tokprint` | `ted.s:4152` | `$00458E` | 4 | enc:4 |
| `tokskip` | `ted.s:4210` | `$004626` | 4 | enc:4 |
| `scrollup` | `?:0` | `$0047B6` | 4 | enc:4 |
| `eraseline` | `ted.s:4479` | `$00495C` | 4 | enc:4 |
| `screenopen` | `ted.s:4668` | `$004C3A` | 2 | enc:2 |
| `halfcolors` | `ted.s:4917` | `$004FCC` | 2 | enc:2 |
| `colors` | `ted.s:4940` | `$005006` | 2 | enc:2 |
| `cls` | `ted.s:4986` | `$0050A8` | 2 | enc:2 |
| `print2` | `?:0` | `$005340` | 2 | enc:2 |
| `fontcalc` | `ted.s:5214` | `$0053CE` | 6 | enc:6 |
| `charlow` | `ted.s:5484` | `$00575A` | 2 | enc:2 |
| `charnot31` | `ted.s:5521` | `$0057BE` | 2 | enc:2 |
| `charnot8` | `ted.s:5555` | `$005830` | 2 | enc:2 |
| `cursrite` | `ted.s:5789` | `$005B46` | 2 | enc:2 |
| `font8` | `ted.s:8071` | `$00D0FA` | 2 | pad:2 |

## Appendix: the encoding differences TED 1.1 (vasm) ↔ CD TED

| Offset | Source | Label | vasm | CD | Instruction |
|---|---|---|---|---|---|
| `$004C2` | `ted.s:190` | `mouseinit+$18` | `D0BC00000040` | `068000000040` | `add.l #dumsleep-pointer,d0` |
| `$00B06` | `ted.s:424` | `defaults+$64` | `9C7C000A` | `0446000A` | `sub #10,d6` |
| `$00B0E` | `ted.s:427` | `defaults+$6C` | `C07C0006` | `02400006` | `and #6,d0` |
| `$00B3E` | `ted.s:444` | `defaults+$9C` | `C07C0003` | `02400003` | `and #3,d0` |
| `$00C64` | `ted.s:515` | `defaults+$1C2` | `C07C0080` | `02400080` | `and #128,d0` |
| `$00C76` | `ted.s:519` | `defaults+$1D4` | `C07C0080` | `02400080` | `and #128,d0` |
| `$01BF2` | `ted.s:1042` | `cursdown+$4` | `C07C0003` | `02400003` | `and #3,d0` |
| `$01C6C` | `ted.s:1075` | `cursup+$4` | `C07C0003` | `02400003` | `and #3,d0` |
| `$02100` | `ted.s:1378` | `split+$FE` | `D07C0009` | `06400009` | `add #lenitem,d0` |
| `$0215E` | `ted.s:1403` | `return+$4` | `C07C0003` | `02400003` | `and #3,d0` |
| `$02398` | `ted.s:1568` | `menu+$2` | `C07C000F` | `0240000F` | `and #15,d0` |
| `$0239E` | `ted.s:1570` | `menu+$8` | `CC7C00F0` | `024600F0` | `and #$f0,d6` |
| `$02506` | `ted.s:1672` | `blocksave+$4` | `C07C0003` | `02400003` | `and #3,d0` |
| `$02750` | `ted.s:1848` | `bcopyok+$24` | `C03C007F` | `0200007F` | `and.b #127,d0` |
| `$027A0` | `ted.s:1871` | `bcopyok+$74` | `D07C0009` | `06400009` | `add #lenitem,d0` |
| `$027FE` | `ted.s:1896` | `bcopyok+$D2` | `D07C0009` | `06400009` | `add #lenitem,d0` |
| `$02856` | `ted.s:1922` | `bcopyok+$12A` | `C03C007F` | `0200007F` | `and.b #127,d0` |
| `$0343A` | `ted.s:2745` | `help+$80` | `C07C0080` | `02400080` | `and #$80,d0` |
| `$03450` | `ted.s:2753` | `help+$96` | `C47C000F` | `0242000F` | `and #15,d2` |
| `$03454` | `ted.s:2754` | `help+$9A` | `D47C0030` | `06420030` | `add #48,d2` |
| `$03548` | `ted.s:2829` | `getstuff+$A2` | `DE7C000C` | `0647000C` | `add #12,d7` |
| `$0398C` | `ted.s:3146` | `numtoascii` | `C0BC0000FFFF` | `02800000FFFF` | `and.l #$ffff,d0` |
| `$039AA` | `ted.s:3156` | `numtoascii+$1E` | `D07C0030` | `06400030` | `add #48,d0` |
| `$03BCC` | `ted.s:3350` | `allocline+$2` | `D07C0009` | `06400009` | `add #lenitem,d0` |
| `$03DEA` | `ted.s:3547` | `isithere2+$3C` | `C07C00DF` | `024000DF` | `and #255-32,d0` |
| `$03E08` | `ted.s:3554` | `isithere2+$5A` | `C47C00DF` | `024200DF` | `and #255-32,d2` |
| `$0401A` | `ted.s:3720` | `freeupa5+$78` | `D07C0009` | `06400009` | `add #lenitem,d0` |
| `$04100` | `ted.s:3806` | `makeline+$2` | `D07C0009` | `06400009` | `add #lenitem,d0` |
| `$045A2` | `ted.s:4158` | `tokprint+$14` | `C74D` | `CB4B` | `exg a3,a5` |
| `$045A8` | `ted.s:4160` | `tokprint+$1A` | `C74D` | `CB4B` | `exg.l a3,a5` |
| `$046FE` | `ted.s:4278` | `tokskip+$D8` | `863C0020` | `00030020` | `or.b #32,d3` |
| `$04702` | `ted.s:4279` | `tokskip+$DC` | `843C0020` | `00020020` | `or.b #32,d2` |
| `$047F0` | `ted.s:4367` | `scrollup+$3A` | `C27C003F` | `0241003F` | `and #63,d1` |
| `$04866` | `ted.s:4402` | `scrollup+$B0` | `807C0040` | `00400040` | `or #64,d0` |
| `$049EE` | `ted.s:4517` | `eraseline+$92` | `C07C003F` | `0240003F` | `and #63,d0` |
| `$049F2` | `ted.s:4518` | `eraseline+$96` | `807C0040` | `00400040` | `or #64,d0` |
| `$04DA8` | `ted.s:4757` | `screenopen+$16E` | `907C0001` | `04400001` | `sub #1,d0` |
| `$04FFC` | `ted.s:4936` | `halfcolors+$30` | `DE7C0001` | `06470001` | `add #1,d7` |
| `$05030` | `ted.s:4956` | `colors+$2A` | `DE7C0001` | `06470001` | `add #1,d7` |
| `$050C8` | `ted.s:4993` | `cls+$20` | `C07C003F` | `0240003F` | `and #63,d0` |
| `$0534E` | `ted.s:5172` | `print2+$E` | `847C0080` | `00420080` | `or #128,d2` |
| `$053D0` | `ted.s:5215` | `fontcalc+$2` | `C07C000F` | `0240000F` | `and #15,d0` |
| `$053E6` | `ted.s:5220` | `fontcalc+$18` | `C27C00F0` | `024100F0` | `and #$f0,d1` |
| `$053F6` | `ted.s:5226` | `fontcalc+$28` | `C27C00F0` | `024100F0` | `and #$f0,d1` |
| `$05766` | `ted.s:5487` | `charlow+$C` | `C27C0003` | `02410003` | `and #3,d1` |
| `$057CA` | `ted.s:5524` | `charnot31+$C` | `C27C0003` | `02410003` | `and #3,d1` |
| `$05842` | `ted.s:5560` | `charnot8+$12` | `C27C0003` | `02410003` | `and #3,d1` |
| `$05B58` | `ted.s:5794` | `cursrite+$12` | `947C0001` | `04420001` | `sub #1,d2` |
| `$0D4FA` | end of hunk | `font8+$400` | `4E71` | `0000` | padding |
