# AmiBlitz3 changes to the acidlibs, part a–l (relative to Blitz Basic 2.10)

What AmiBlitz 2.x/3 changed in the acidlibs `2dlib` … `lmullib` of Blitz
Basic 2 (our port: `libs/acidlibs/<lib>.c`, notes `docs/libs/*.md`), as
the basis for deciding which changes we take over. Documentation only;
nothing here is taken over yet. Part m–z: `docs/ab3/acidlibs-m-z.md`.

## Sources and method

* **Baseline:** the CD's `BLITZ2/ACIDLIBS` (BigLib, 83 libraries; same
  declarations as our build), plus our C port's `BUG(orig)` comments and
  the `bug-orig` issues of `fubar-coder/bb2-c`.
* **AmiBlitz3:** repository `AmiBlitz/AmiBlitz3` (HEAD
  `AmiBlitz/AmiBlitz3@b1a9dee1`, 2026-10-25; the first 602 commits up to
  `AmiBlitz/AmiBlitz3@c95cce17` are the SourceForge SVN, revision numbers
  from the `git-svn` mirror).
  * Binaries, two states compared with the CD:
    * `BlitzLibs/AcidLibs` of `AmiBlitz/AmiBlitz3@ea8c25c7` (SVN r1,
      2009-02-10). This is already AmiBlitz 2.x and carries all pre-git
      changes; they are listed as "≤ 2009" with this commit.
    * The BigLibs of HEAD (last changed by
      `AmiBlitz/AmiBlitz3@c79fee82`, 2026-10-16): AB3 split the old
      acidlibs into `BlitzLibs/baseLibs`, `csLibs` (chipset/Blitz mode
      libraries) and `utilLibs`, and renumbered many libraries
      (`AmiBlitz/AmiBlitz3@dbc34db6`, 2026-04-24). Libraries were matched
      by their first token name, not by number.
  * Sources (`Sourcecodes/Amiblitz3/BlitzLibs/{,ACIDLIBS/,Blitzlibs/}*.ab3`,
    tokenised `.bb2` before `AmiBlitz/AmiBlitz3@9f5e7b8c`, SVN r444,
    2014-03-02) and their history, for the reasons of a change. Many are
    disassemblies (In_Go Reassembler 2002–2004, IRA/ADis 2016–2026); a
    source is not always what the shipped binary was built from (e.g.
    `asllib.ab3`, `gadgetslib.ab3` are the BUM sources, the binary is
    the AB2 one).
  * Changelogs: `Docs/Changelog.guide` at `AmiBlitz/AmiBlitz3@ea8c25c7`,
    `Docs/Miscellaneous/Changelog.guide` at HEAD.
* **Method:** every library extracted with `biglib extract` (all
  numbers probed) from the CD, the 2009 and the HEAD BigLibs; tokens
  listed with `blitztokens` (CD `BLITZ2` + the single library),
  declarations compared with `blitzlibcompare`, code compared as a raw
  `m68k-amigaos-objdump` disassembly diff. Where the code differs, the
  AB3 source/commit was read for the reason and checked against the
  issues of the library. "Not fixed" below means the original code is
  still there (checked in the AB3 source or binary); "not checked" means
  only the commit level was looked at.
* **Classes:** **fix** (of a BB2 bug), **extension** (new commands or
  options, compatible), **incompatible** (changed declarations, token or
  library numbers, behaviour; needs OS3.x/68020/AB3 compiler or
  debugger), **cosmetic** (comments, reformatting, disassembly into
  source, directory moves; only counted).
* **AB3-internal:** several AB3 "fixes" repair regressions of AB3's own
  reassemblies or earlier AB3 changes (gameiolib, intuifontlib,
  linklistlib `ClearList`, `MULU.L` on the 68000, iffmakelib). They are
  named, but do not count as fixes of BB2.

### Changes affecting all libraries

* `AmiBlitz/AmiBlitz3@dbc34db6` (2026-04-24), `@7960b75c`, `@d349b6f6`:
  library renumbering. In part a–l: editlib 186 → 79, environlib
  193 → 188, gameiolib 190 → 39; the ALibJsr calls between libraries
  (blitzoslib → fontlib, iffmakelib → environlib, dhandlerlib →
  debuglib) were adapted. **Incompatible** with BB2 tokenised sources and
  with third-party libraries that call these libraries by token.
* Cosmetic, not listed per library (about 120 commits in a–l):
  `AmiBlitz/AmiBlitz3@0e2ff1db` (SVN r2, sources added),
  `@87438cdd` (r279), `@9f5e7b8c` (r444, `.bb2` → `.ab3`),
  `@86444d9b` (r582), `@9d854a53`, `@b449b041`, `@8880f56b`,
  `@693db8cd`, `@a6f54bc9` (directory moves), `@f16939fb` (2025-11-05,
  hard coded ALibJsr numbers for globalalloc/free → constants),
  `@f4ae616a` (2023-12-10, `myasllib`/`mybbgtlib` renamed `asllib`/
  `gadtoolslib`), and the many "disassembled …", "added comments",
  "source improvements without changes" commits.

## Libraries without relevant AB3 changes

Code identical to the CD (HEAD binary; at most 16 bytes of padding at the
end), source commits only cosmetic (disassembly, comments, help texts):
2dlib (sis2dlib), animlib (`AmiBlitz/AmiBlitz3@115dea76`/`@f6ef04b0`
2023: source only, help texts), arexxlib, audiolib, bblitlib, blitlib,
blitzcoplib, blitzkeyslib, chipbaselib, cliargslib, clrlib, collslib,
datalib, displaylib, errtraplib, fadelib, ffplib, floatquicklib, freqlib,
iffiolib, ilbmifflib, inputoutputlib, intlib, intuifontlib (see below).
All their `bug-orig` issues therefore apply to AB3 unchanged.

Code identical, but with an AB3-internal history:

* **gameiolib** – `AmiBlitz/AmiBlitz3@53927190` (2026-02-05) "added
  port checking" broke `Joyx(1)`/`Joyy(1)` (AB3 issue `AmiBlitz/AmiBlitz3#125`);
  `AmiBlitz/AmiBlitz3@dc94e999` (2026-09-30) went back to the original
  code. Only the library number changed (190 → 39, incompatible). Our
  #415 (`Gameb(0)` POTGO) not fixed.
* **intuifontlib** – `AmiBlitz/AmiBlitz3@75883a7e` (2022-08-26) "fixed a
  bug in intuifontlib" repaired AB3's own reassembly
  (`@41c38d2c`); the binary is the original again. #197, #198 not fixed.

## Libraries with changes

### arrayslib (system library $C400)

| AB3 commit | Date | Class | What | Our issue |
|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@ea8c25c7` SVN r1 | ≤ 2009 | extension, incompatible | Two new entries `$C405`/`$C406` (`newarr2`/`newlocarr2`): re-`Dim` that keeps the contents (AllocMem + CopyMem, refuses if the largest free block < size+200000). Only called by the AB3 compiler. | – |
| `AmiBlitz/AmiBlitz3@f9c423e5` | 2020-07-15 | (AB3-internal) | Kick 1.3 check for memory pools, `MULU.L` on the 68000 removed (AB3 regression). | – |
| `AmiBlitz/AmiBlitz3@23a4611b`, `@590b856d`, `@22751b30` | 2026-06 … 09 | incompatible | List creation/deletion moved to linklistlib (ALibJsr `$4024`/`$4025`); list header 40 instead of 32 bytes (pool pointer, element size, mode); `Dim List a(0)` = dynamic list in a memlib pool. Global arrays now via globalalloc. | #59 (?) – size 0 is redefined as "dynamic list", the old linking loop is gone |

Not fixed: #58 (`arrmult` still `SWAP` instead of shift; the "Dim only
32768 elements on 68000" fix of 3.11.0 is in the compiler).

### asllib (#myasllib, 80)

| AB3 commit | Date | Class | What | Our issue |
|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@ea8c25c7` SVN r1 | ≤ 2009 | extension | `ASLMultiSelect`, `ASLGetFile`, `ASLNextFile`, `ASLFreeRequest`, `ASLNumFiles` (multi-select file requester, needs dos.library; requester freed in finit). | – |
| `AmiBlitz/AmiBlitz3@ea8c25c7` SVN r1 | ≤ 2009 | fix (?) | AB2 changelog: "fixed crash when trying to select another screenmode (bug in myasllib)"; the binary patches the flag handling of `ASLScreenRequest` (flag 1 takes a value). | – (?) |
| `AmiBlitz/AmiBlitz3@94bfce8f` | 2026-06-03 | cosmetic | token id → `#allocstring` in the (unused) source. | – |

Not fixed (2009 = HEAD binary, reassembly in r444): #60
(`ASLScreenRequest` returns the freed requester), #65 (font name buffer
still 96 bytes). #61–#64, #403 not checked in the binary (the BUM source
in the repository still has them).

### banklib (76)

| AB3 commit | Date | Class | What | Our issue |
|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@5d668e8e` | 2026-04-16 | extension | `New`, `Dispose` (moved from LotanSystem); library gets init code. | – |

#72 not checked.

### bitmaplib (160)

| AB3 commit | Date | Class | What | Our issue |
|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@ea8c25c7` SVN r1 | ≤ 2009 | extension | `SystemBitMap` (mode: give bitmaps a layer + RastPort), `BitMapRastPort`; bitmap object gets a layer pointer. Needs layers.library. | – |
| `AmiBlitz/AmiBlitz3@899c4511` | 2020-07-03 | fix (?) | "memory alignment bugfix in bitmaplib (works now with 68000 again)" – most likely an AB2/AB3 regression. | – (?) |
| `AmiBlitz/AmiBlitz3@64435749` | 2026-04-09 | extension | `BitMapDepth` (from ElmoreHardwareLib `Depth`). | – |

Not fixed: #76 (size still `MULU` of words in `extbitmap`/free).
#78–#80 not checked.

### blitzoslib (117)

| AB3 commit | Date | Class | What | Our issue |
|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@d5ed6ea5`, `@dbc34db6` | 2026-03/04 | incompatible | ALibJsr targets adapted to the renumbered fontlib (and the shifted fontlib routines); error stubs rearranged (reassembly). Needs AB3's library set. | – |

Otherwise unchanged; #81–#91, #94 not fixed.

### brexxlib (132)

| AB3 commit | Date | Class | What | Our issue |
|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@ea8c25c7` SVN r1 | ≤ 2009 | extension | `RawKeyState`, `RawQualifier`; `Type` accepts a qualifier (changelog 2.27). | – |
| same | ≤ 2009 | incompatible | The new tokens are inserted before `Tape`/`FreeMacroKey`: their token numbers move ($12/$13 → $14/$15). | – |

#95–#104 not checked.

### crunchlib (#romuluscrunchlib, 74)

| AB3 commit | Date | Class | What | Our issue |
|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@4bc3227a` | 2025-08-29 | extension | `ZX0Decrunch` (ZX0/LZ77, code © Emmanuel Marty). Library now in `utilLibs`. | – |

Imploder/CrM/PP code only relocated (?); #112–#120 not fixed (?).

### datelib (78)

| AB3 commit | Date | Class | What | Our issue |
|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@ea8c25c7` SVN r1 | ≤ 2009 | fix | `Date$`: the day → day/month/year conversion is rewritten as a year loop (365/366) and a month table with February 28/29, instead of the 4-year-cycle division. Day 366 of a cycle no longer becomes 32/12. Leap years only by "divisible by 4" (the 100/400 test is dead code). | #120 |
| same | ≤ 2009 | cosmetic | Help texts (`datelib_new`). | – |

Not fixed: `NumDays` unchanged (#122–#125). #121 (`WeekDay`) not checked
(code changed around `totday`, still `SUBQ.w #2`).

### debuglib (system library $D500)

| AB3 commit | Date | Class | What | Our issue |
|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@ea8c25c7` SVN r1, `@1d7d56e7` SVN r598, `@7f5ba4d1` SVN r599 | ≤ 2009 … 2016-05-10 | extension, incompatible | Bernd's task trap handler (`#tasktrap`, new entry `$D50A` `dochangetrap`, changed trap register handling, "UPrint"); reassembled 2016. Part of the AB2/AB3 debugger interface. | – |
| `AmiBlitz/AmiBlitz3@7cde5dce`, `@820697d9` | 2023-08-25, 2026-10-13 | incompatible | Debugger data structures changed (ComDataVersionID `$EEE9`), token base passed via `tc_UserData`. Only works with AB3's debugger/compiler. | – |

Not fixed: #127 (`dofindoff` unchanged), #126 not checked.

### dhandlerlib (1)

| AB3 commit | Date | Class | What | Our issue |
|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@ea8c25c7` SVN r1 | ≤ 2009 | incompatible | 40 KB variant (AB2 debugger handler). | – |
| `AmiBlitz/AmiBlitz3@6ec7746b` SVN r597 | 2016-05-05 | incompatible | Reassembled original again (876 bytes); process name "AmiBlitz Debug Process" instead of "Blitz ][ Debug Proc"; the wait loop uses dos `Delay(1)` instead of graphics `WaitTOF`; ALibJsr numbers of debuglib adapted. | – |

#409, #410 not checked.

### editlib (186 → 79)

| AB3 commit | Date | Class | What | Our issue |
|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@ea8c25c7` SVN r1 | ≤ 2009 | extension | `ReadLine$` (fast read up to 2000 chars), `GetRegD0`/`PutRegD0`. | – |
| `AmiBlitz/AmiBlitz3@9cabe785` | 2022-11-16 | incompatible | `GetRegD0`/`PutRegD0` moved to environlib (token change). | – |
| source since `AmiBlitz/AmiBlitz3@0e2ff1db`, in the HEAD binary | ≤ 2026 | extension | `Input$`, `Input` (aliases of `Edit$`/`Edit`). | – |
| `AmiBlitz/AmiBlitz3@dbc34db6` | 2026-04-24 | incompatible | library number 186 → 79. | – |

Not fixed: #131 (buffer still 80 bytes). #128–#130, #132 not checked.

### environlib (193 → 188)

| AB3 commit | Date | Class | What | Our issue |
|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@ea8c25c7` SVN r1 | ≤ 2009 | extension | `Processor` recognises the 68060 (AttnFlags bit 7) and FPU values. | – |
| `AmiBlitz/AmiBlitz3@fd03e2d9`, `@b5f4789e` | 2019-12-09, 2022-03-31 | (AB3-internal) | fixes of AB3's 68060/FPU detection. | – |
| `AmiBlitz/AmiBlitz3@11ca65bd` … (source comment 2019-11-25) | 2019 | extension | `ProcessorFPU`. | – |
| `AmiBlitz/AmiBlitz3@5432585d` | 2022-09-29 | fix (partial), incompatible | `VBRcheck`/`VBRmove`/`VBRclear` replace `cr_vbr` (token removed); the VBR is read from the 68010 on. | #42 (environlib part only; errtraplib, runerrlib, compiler unchanged) |
| `AmiBlitz/AmiBlitz3@9cabe785` | 2022-11-16 | extension | `GetD0`, `PutD0`, `GetRegD0`, `PutRegD0`, `PushD0` (from wzlib/editlib). | – |
| `AmiBlitz/AmiBlitz3@5bb94fff` | 2025-11-04 | extension | `UsedMem` (memlib's allocation counter). | – |
| `AmiBlitz/AmiBlitz3@721df645` | 2025-12-10 | extension | `NTSC`, `VPos`, `DispHeight` (from vallib). | – |
| `AmiBlitz/AmiBlitz3@64435749`, `@5d668e8e`, `@7960b75c` | 2026-04 | extension | `StrAddr`, `Null`, `ActiveScreenPtr`, `ActiveWindowPtr`, `ScreenWidth/Height`, `ChipFree`, `FastFree`, `LargestFree`, library bases, `ExecBase`, `KickVersion` (from Elmore/Lotan libraries). | – |
| same + `AmiBlitz/AmiBlitz3@dbc34db6` | 2026-04 | incompatible | Token order changed (`WBExtViewMode` before `Processor`, `ExecVersion` moved), library number 193 → 188. | – |

### exitslib (system library $C800)

| AB3 commit | Date | Class | What | Our issue |
|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@ea8c25c7` SVN r1 | ≤ 2009 | extension | Entry `$C801` `addexithandler` (chain of exit handlers, changelog 2.24); used by the AB3 compiler. | – |

### fileiolib (124)

| AB3 commit | Date | Class | What | Our issue |
|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@ea8c25c7` SVN r1 | ≤ 2009 | extension | `WriteType`, `ReadType`, `CopyType`, `ClearType` (whole NEWTYPEs incl. strings); `FileSeek` can be used as a function (`!acommand{#long}`); error messages 9/10. | – |

Core code from the BUM source (2009 = HEAD binary); #138, #140, #141,
#143, #144 not checked in the binary.

### fontlib (142)

| AB3 commit | Date | Class | What | Our issue |
|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@f9d146cb`, `@0e7af991` | 2022-11-23 | extension | diskfont.library is opened only when a font is not in ROM (topaz needs no diskfont). | – |
| `AmiBlitz/AmiBlitz3@6cee6b72` | 2025-08-27 | extension | `CludgeBlitzFont` (font file in memory, no diskfont.library); separate error messages for a wrong size / proportional font. The `BlitzFont` object token moves $0D → $0E (?). | – |
| `AmiBlitz/AmiBlitz3@99f38fed` | 2025-11-12 | fix | `bitchar`: `SUB.b tf_LoChar` is followed by `BCS` instead of `BMI`, so characters ≥ LoChar+128 (umlauts) are drawn. | #148 |
| `AmiBlitz/AmiBlitz3@d5ed6ea5` | 2026-03-17 | incompatible | routine numbers shifted (blitzoslib adapted). | – |

Not fixed: #150 (unsuitable font still not closed before the error).
#149, #151 not checked.

### gadgetslib (140)

| AB3 commit | Date | Class | What | Our issue |
|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@ea8c25c7` SVN r1 | ≤ 2009 | extension | `SetPropInfo`, `MoveProp`, `GetPropCurrent`, `ArrowGadget` (BOOPSI, freed with `DisposeObject`); PropInfo block 28 bytes. | – |
| same | ≤ 2009 | incompatible (?) | `SetTextGadget` (token $22) is gone; `SetPropInfo` takes its token. | – |
| same | ≤ 2009 | fix (?) | AB2 changelog: "memaccess after memrelease in gadgetslib fixed" – no matching issue of ours. | – (?) |

Not fixed: #163 (`freegadget` still overwrites the type with the size,
now `MOVEQ #28`). #164–#167, #952 not checked.

### gtlib (#mygadtoolslib, 141; AB3 "gadtoolslib")

| AB3 commit | Date | Class | What | Our issue |
|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@ea8c25c7` SVN r1 (David McMinn, NCS, 2000–2004) | ≤ 2009 | extension | `GTActivateGadget`, `GTNewLookProp`, `GTListAddress`, `GTUserData`, `GTChangeCycle`, `GTGetInternal`, `GTUnderscore`, `GTEventSeconds`, `GTEventMicros`, `GTScaleX/Y`; all creation commands return the gadget; GTBevelBox frame type, GTButton/GTShape initial toggle state, GTGadPtr option "last gadget". | – |
| same | ≤ 2009 | incompatible | `GTGimmeCludge` renamed `GTGZZPosition`; GTGetString/GTGetInteger/GTGetAttrs/GTEnable/GTDisable use `GT_Get/SetGadgetAttrsA` on V39+ (other behaviour on OS3); internal gadget list per GTList. | – |
| same | ≤ 2009 | fix | Cycle/MX labels: the pointer array is sized by counting the `|`, kept in the internal list and freed with the GTList/gadget. | #174 |
| same | ≤ 2009 | fix (?) | GTShape: own `Image` structures per gadget (no static image, PlanePick from the depth), `CreateGadget` result checked in `_createstd`. | #173, #327 (?) |
| same | ≤ 2009 | fix (?) | GTPalette's colour now goes into its own default tag list; optional values reset by the forms without them. | #168, #169 (?) |
| same | ≤ 2009 | incompatible (?) | GTGadPtr/GTSetAttrs use the internal list (first/last gadget of an Id handled per McMinn's notes 2000–2001). | #175 (?) |
| `AmiBlitz/AmiBlitz3@3941e6f3` SVN r584, `@50187254` SVN r583 | 2016-05-03 | cosmetic | source updated to the binary. | – |
| `AmiBlitz/AmiBlitz3@e93d1c9e` | 2026-04-19 | extension | `RetrieveCallHook` (from RILVCallHook). | – |

Not fixed: #176 (`handlestd` still adds the borders of screen 0 without
a check). #170 (?): GTBevelBox still inserts only the tag
`GTBB_Recessed` with data 0. #171, #172, #328 not checked.

### iffmakelib (114)

| AB3 commit | Date | Class | What | Our issue |
|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@925282d3`, `@d349b6f6` | 2026-06-09/22 | incompatible | Only the ALibJsr to environlib (`ExecVersion`) adapted to the new numbers ("unknown token" bug of AB3 3.13). | – |

Code otherwise identical: #180–#184, #326 not fixed.

### ldivlib, lmullib (system libraries $CB00, $CA00)

| AB3 commit | Date | Class | What | Our issue |
|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@6c659503`, `@4a68340a` | 2026-04-18/21 | extension (68020) | New entry `$CB02`/`$CA02`: quick division/multiplication with `DIVS.L`/`MULS.L`, used by the AB3 compiler when 68020 optimisation is on. | – |

Original routines unchanged: #199 (lmullib), #200 (ldivlib) not fixed.

### linklistlib (128)

| AB3 commit | Date | Class | What | Our issue |
|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@ea8c25c7` SVN r1 | ≤ 2009 | extension | `SetItem`, `CountList`, `CountItem`, `NextItemFast`, `FillArray`, pointer lists (`CreateListPtr`, `NextPtrItem` … `FreePtrList`, 14 commands). | – |
| `AmiBlitz/AmiBlitz3@f9c423e5`, `@8a783d5c`, `@dce9f699` | 2020-07-15, 2024-03-14, 2026-09-24 | (AB3-internal) | Kick 1.3/68000 fixes of AB3's pools, ClearList crash of AB3's own free option. | – |
| `AmiBlitz/AmiBlitz3@0989b2d3` | 2025-04-14 | incompatible | `FillArray` removed (now in sortlib); later tokens shift. | – |
| `AmiBlitz/AmiBlitz3@97b92dca`, `@36fae0df` | 2026-05-18/29 | extension | `GetItem`, `SwapItems`, `IsList`; reverse order for `SortList`; free memory option for `ClearList`. | – |
| `AmiBlitz/AmiBlitz3@23a4611b` | 2026-06-22 | incompatible | Creates/frees the lists itself (from arrayslib), 40-byte header, dynamic lists in memlib pools (own pooling below Kick 3.0). | – |

Not fixed: #201 (`SortList` still decides by `CMP`/`BMI`).

## Summary

* 46 libraries in part a–l. **25 unchanged** in code (the 24 of the
  list above plus gameiolib, which changed only its number); changes in
  **21**: arrayslib, asllib, banklib, bitmaplib, blitzoslib, brexxlib,
  crunchlib, datelib, debuglib, dhandlerlib, editlib, environlib,
  exitslib, fileiolib, fontlib, gadgetslib, gtlib, iffmakelib, ldivlib,
  lmullib, linklistlib.
* Table rows by class (a row with two classes counts for both): **fix**
  9 (4 certain: datelib #120, environlib #42 partially, fontlib #148,
  gtlib #174; 5 uncertain), **extension** 26, **incompatible** 20
  (incl. 2 uncertain and the library renumbering), **cosmetic** 3 rows
  plus about 120 commits (only counted); in addition 5 rows / 7 commits
  of AB3-internal fixes of AB3 regressions.
* Most of AB2/AB3's work on these libraries is new commands (gtlib,
  linklistlib, environlib, fileiolib, asllib) and the integration with
  AB3's compiler, debugger, memlib pools and library numbering. Real
  fixes of BB2 bugs are few; the large majority of our `bug-orig` issues
  for a–l are still present in AB3.
* Candidates to take over (as deliberate deviations after the port):
  fontlib #148 (one instruction), datelib #120 (the year loop; leap years
  as in AB3 only by 4), environlib #42 (reading the VBR on the 68010 –
  better done for all four places of the issue), gtlib #174 (and, after
  a closer look, #173/#327/#168/#169 from McMinn's version).
* Mapping our issue → AB3 fix: #120 → `AmiBlitz/AmiBlitz3@ea8c25c7`;
  #42 (environlib) → `AmiBlitz/AmiBlitz3@5432585d`; #148 →
  `AmiBlitz/AmiBlitz3@99f38fed`; #174 → `AmiBlitz/AmiBlitz3@ea8c25c7`;
  uncertain: #59, #168, #169, #173, #175, #327 → `@ea8c25c7`/`@23a4611b`.
