# The libraries of Red When Excited: inventory

> As of 29.10.2026. Static analysis: sources read as text, token tables
> of the binaries compared; nothing compiled or run.
> Evidence: **[B]** binary / token table, **[S]** source, **[AB3]**
> AmiBlitz3 (SourceForge `amiblitz3`, git of the SVN), **[Schluss]**
> conclusion, not proven. Plan: `TODO.md` §4; origin and license:
> `archive/README.md`.

## 1 Summary

* **Sources:** `archive/aminet/amibl2src/rwelibsrc/` (from
  `RWELibSourc.lha` in `amibl2src.lha`, what RWE released in September
  1999): 287 files, 123 Blitz sources, of them 117 library sources (with
  `!libheader`) of **about 50 libraries**, many in several versions; the
  rest examples, tests and a debugger [S].
* **Authors** (by the headers): mostly **Reflective Images** (Stephen
  McNamara, Steven Matty; 1994–1996), some marked **Leading Edge
  Software** (Matty: datatypes, Disasm, Debuglib), **Red When Excited**
  (Registry 1997, LVCallHook 1996, the ARexx library of 1997), **Neil
  O'Rourke** (ReqTools interface, CIA tracker library) [S].
* **Binaries** to compare with: the CD (`BSS2/REDBLITZLIBS/LIBRARIES/USERLIBS/`
  = Blitz Support Suite 2 of RWE, the latest; `BUM9B`/`BUM10A/BLITZLIBS/RILIBS/`,
  `EXTRAS/BLITZLIBS/OTHERLIBS/`) and AmiBlitz3 (`BlitzLibs/UserLibs/userlibs/`,
  32 RI/RWE files, 27 of them byte-identical to the CD, never changed in
  its history) [B, AB3].
* **29 sources have the declarations of a binary exactly** (same names in
  the same order, same help texts): for 22 libraries there is a source
  version whose binary exists (section 3) [B].
* **About 25 libraries exist only as source** (no binary on the CD nor
  in AmiBlitz3), e.g. Registry, the Blitz Bombers libraries, ptlib,
  MapEd, threadlib [B].
* **Only one RI/RWE binary has no source:** `RWEMathLib` (23,
  `BUM10A/BLITZLIBS/TESTLIBS`), besides the four later acidlibs of
  `BSS2/REDBLITZLIBS/LIBRARIES/BASIC/` (documented only,
  `docs/v2.10/libs.md` §4) [B].
* **Numbers:** taken from AmiBlitz3's `sorted_libnums.ab3` (it lists
  every RI library); **clash with acidlibs/deflibs only 121** (RIEvalLib
  vs acidlibs' polylib, `docs/v2.10/libs.md` §4). Several sources carry
  literal test numbers (5, 8, 10, 19, 22, 39, 195, 197, 198), some of
  them used by other libraries [S, AB3].
* **License:** `ROBNORTHFAST.bb` contains Rob Northen's PRO-PACK unpacker
  "Copyright (c) 1991,92 Rob Northen Computing, U.K. All Rights
  Reserved." – not covered by RWE's release, **not to be ported**. The
  Protracker 2.3A play routine (ptlib) names no authors. Further credits
  in the sources (Ott/The Farm: bug reports; Delirium Softdesign: a
  busy-wait fix in the CIA tracker library; Nigel Hughes: an addition to
  RIShapesLib) [S].
* **Tokens:** with the token list of blitz2 1.95 alone 100 of the 123
  sources are readable; with the token tables of the binaries added
  (1773 tokens, provisional) 120. Of the other three, two are no token
  problem: they are saved in another file format, **`BB22`** (section
  4.1; 9 files in all); the third uses `GetPubScreen` (`$CF12`) of a
  later screenslib that is not on the CD [B, AB3].

## 2 Libraries

Number: `!libheader` constant resolved by AmiBlitz3 (`#name` →), a
literal number in the source (lit.), or – where the constant is in no
table – the number of the matching binary (bin.). Binary: **=** the
declarations are identical (names, order, help texts), **≈ n %** only the
name sets overlap that much (Jaccard), – none. BSS2 =
`BSS2/REDBLITZLIBS/LIBRARIES/USERLIBS/`, OL = `EXTRAS/BLITZLIBS/OTHERLIBS/`,
BUM = `BUM9B`/`BUM10A/BLITZLIBS/RILIBS/`, AB3 = AmiBlitz3. Date: the
latest date in the header comments.

| Library | Number | Source versions (`rwelibsrc/…`) | Author (header) | Date | Binary | Reference |
|---|---|---|---|---|---|---|
| AMOS functions | 75 | `AMOSFunclib/func-library4`…`7` | RI | 1996-01-11 | 7 **=** BSS2 `RIAMOSFUNCLIB` (4768 B); 6: names equal, 1 help text differs; AB3's file differs from BSS2 | func-library7 |
| Anim | 57 | `ANIMLib/Anim-Library3` | RI (McNamara) | 1994-11-07 | ≈ 88 % BSS2 `RIANIMLIB` | open |
| App | 61 | `AppLib/AppLib`, `applib2`, `applib3` | – | – | applib2 **=** BSS2/AB3 `RIAPPLIB` (2892 B); applib3 in the `BB22` format | applib2 (applib3: check) |
| Arrays | – (`#riarrayslib` in no table) | `ArraysLib` | RI | – | – | open |
| CompactDisk | 14 (`#ricompactdisclib`, lit. 14) | `CDlib/cd`, `cd2`, `cd3`, `cdSCSI` | – | 1995-07-03 | ≈ 96 % OL `RICOMPACTDISKLIB` | open |
| Commodities | 89 | `Commoditieslib/CommoditiesLib2` | – | – | **=** BSS2/AB3 (2604 B) | CommoditiesLib2 |
| CopperFX | 11 (bin.) | `Copperlib/CopperFX3` | RI | 1995-03-08 | **=** BSS2/AB3 (1856 B) | CopperFX3 |
| DOS | 33 | `DOSLib/ridoslib`, `ridoslib2` | – | – | ridoslib2 **=** BSS2/AB3 (2416 B) | ridoslib2 |
| Debug | 51 | `Debuglib/Debuglib4` | LES | 1995-02-27 | **=** OL `RIDEBUGLIB` (1380 B); BSS2's differs | Debuglib4 (BSS2: check) |
| LES debug | 49 | `Debuglib/LESDEBUGlib`, `LESDebuglib/LESDEBUGlib` | – | 1996-09-17 / 1996-12-01 | the older **=** AB3 `RILESDEBUGLib` (11216 B); the newer ≈ 94 % | open (newer source than the binary) |
| Disasm | 50 (bin.) | `Disasmlib/Disasm-library.bb2` | LES | 1994 | **=** AB3 `RIDisAsm` (9228 B) | Disasm-library |
| Encrypt | 52 | `Encryptlib/Encrypt-Library` | RI (McNamara) | 1996-01-21 | **=** BSS2/AB3 (636 B); BUM/OL (680 B) same declarations | Encrypt-Library |
| Eval | 121 (bin.; clash with acidlibs) | `EvalLib4` | RI | 1995-11-17 | ≈ 83 % BSS2 `RIEVALLIB` | open |
| FastSerial | 36 | `FASTSerial/fastseriallib`, `2`, `3` | – | – | 3 **=** BSS2/AB3 (1920 B) | fastseriallib3 |
| FNS | 93 | `FNSLib/fns-library11`; `FNS2` (lit. 39, 3 entries) | RI | 1995-11-14 | 11: names equal BSS2, 1 help text differs | fns-library11 (check) |
| FX | 66 | `FXLib/FX-Library13` | RI (McNamara) | 1996-01-21 | **=** BSS2/AB3 (4632 B) | FX-Library13 |
| GFX | 65 | `GFXLib/GFX-Library2` | – | 1996-01-21 | **=** BSS2/AB3 (2036 B) | GFX-Library2 |
| GTMenu | 37 | `GTMenuLib/GTMenuLib`, `2`, `3` | RI | 1996-03-10 | ≈ 92 % BSS2 | open |
| Linked list | 12 (bin.) | `Listlib/listlib4` | RI | – | ≈ 92 % OL `RILISTLIB` | open |
| MapEd | 8 | `MapEd/MapEd_Bit`, `MapEd_Bit2` | RI | – | – | open |
| MapShapes | 6 | `MapEd/MapShape`, `MapShape2` | RI | 1996-12-22 | – | open |
| ReqTools | 54 (`#neilsreqtoolslib`) | `RTLib` | Neil O'Rourke | 1997-01-29 | ≈ 97 % `BUM10A/BLITZLIBS/OTHERLIBS/NEILSREQTOOLSLIB` | open |
| Registry | 3 | `Registry`, `Registry/Registry2`, `Registry3` | RWE | 1997 | – | open |
| Registry support | – (`#registrysupplib`) | `Registry/regsupplib` | RWE | 1997 | – | open |
| Snow | lit. 195 | `Snow/SnowLib3` | RI | 1994 | – | open |
| Blitz Bombers | 30 | `blitzbomberslib/blitzbombers86`, `87`, `88` | RI | 1996-10-04 | – | open |
| Blitz Bombers 2 | – (`#blitzbomberslibtwo`) | `blitzbomberslib/blitzbombersTwo00` | RI | – | – | open |
| BB blit | 19 (lit., = `#bb_blitlib`) | `blitzbomberslib/BB_BlitLib4` | RI (McNamara) | 1996-01-30 | – | open |
| BB map | – (`#ribbmaplib`) | `blitzbomberslib/bbmaplib2` | RI | 1995-09-30 | – | open |
| (tests) | lit. 5 / – | `blitzbomberslib/TestBlit`, `PredatorBlit` | – | – | – | not a library / test |
| C2P | lit. 8 | `c2plib`, `c2plibA` | RI | 1995 | – | open |
| Chunky | 24 | `chunkylib`, `chunkylib2` | RI | 1995-08-14 | – | open |
| DataTypes | 25 | `datatypeslib` … `datatypeslib6` | RI / LES (Matty) | 1995 | 6 ≈ 95 % BSS2 | open |
| (template) | – | `emptylib` | RI | 1995 | – | not a library |
| LVCallHook | 27 (bin.) | `lvcallhook/LVCallHook3` | RWE | 1996 | **=** AB3 `RILVCallHook` (1172 B) | LVCallHook3 |
| Misc | 47 | `misclib/misclib`, `misclib2` | – | 1996-05-23 | misclib2: names equal AB3 `Rimisclib`, 2 help texts differ; BSS2 ≈ 72 % | open |
| (argument tests) | lit. 198 | `myaudiolib`, `myctrlclib` | – | – | – | test |
| Display | 29 | `mydisplaylib` | RI | – | – | open |
| NonVolatile | 35 (clash `mazfpulib`, AB3) | `nonvolatile` | RI | 1994 | **=** BSS2/AB3 (640 B) | nonvolatile |
| Pack | 73 | `packlib/Pack-Library8`, `8_Mc`, `9` | RI | 1996-05-09 | 8 **=** BUM (2564 B), 8_Mc **=** OL (2388 B); 9 newer, ≈ 88 % | open (9 newer than the binaries) |
| PopUp window | – (`#popupwinlib`) | `popuplib/popupwinlib` (+ `test2`) | RI | 1994 | – | open |
| Protracker | – (`#ritrackerlib`) | `ptlib/ptlib` … `ptlib7`, `ptlibmatty` | Protracker 2.3A routine, adapted | 1995-04-08 | – | open (authors of the routine) |
| CIA tracker | 56 (`#ciatrklib`; clash `xbciatrackerlib`, AB3) | `ptlib/CIATrackerLib1` (+ `SamplePrograms/TrackerTest`) | Neil O'Rourke, Delirium Softdesign | – | ≈ 88 % OL `XBONESCIATRKLIB` | open |
| Req | 79 | `reqlib/ReqLib`, `2`, `3` | – | – | all three **=** OL/AB3 `RIREQLIB` (964 B) | ReqLib2 (built identically, 3.1) |
| ARexx | 133 (`#myconst=133`) | `rexxlib/…` (9 versions) | RI; `NEWAREXXLIB`: RWE (Matty) | 1997-02-09 | ≈ 76 % BSS2 `RIAREXXLIB` | open |
| Intuition | – (`#riintuilib`) | `riintuilib`, `riintuilib2` | RI | 1995 | – | open |
| PRO-PACK | lit. 197 | `ROBNORTHFAST` | Rob Northen Computing (All Rights Reserved) | 1991/92 | – | **not ported** (license) |
| Serial | lit. 22 | `seriallib/Serial-Library` | – | – | – | open |
| Shapes | 46 | `shapeslib/RIShapesLib`, `RIShapesLib2` (+ `Test`, `Comparison`) | RI, Nigel Hughes | – | ≈ 92 % BUM `RISHAPESLIB`; RIShapesLib2 in the `BB22` format | open |
| Shapez | 21 | `shapezlib` | RI | 1995 | – | open |
| Sort | 13 | `sortlib/RISortlib2` | RI | 1996-08-30 | ≈ 80 % BUM/BSS2/OL `RISORTLIB` (one file) | open (source newer than the binary) |
| Sound | 45 | `soundlib/risoundlib`, `risoundlib2` | – | – | risoundlib **=** BSS2/AB3 (496 B); risoundlib2 (8 entries) – | open |
| STC decrunch | 18 | `stcdecrunchlib` | – | – | **=** BSS2/AB3 (576 B) | stcdecrunchlib |
| StoneCracker | 17 | `stclib` | – | – | **=** BSS2/AB3 (1136 B) | stclib |
| Thread | – (`#rithreadlib`) | `threadlib` | RI | 1995 | – | open |
| ToolTypes | 59 (bin.) | `tooltypeslib/ToolTypes4` | RI (McNamara) | 1995-04-05 | **=** BSS2/AB3 (3564 B) | ToolTypes4 |
| TrackDisk | 91 | `trackdisklib/trackdisk`, `trackdisk2` | – | – | both **=** BSS2/AB3 (1892 B) | trackdisk2 (both build identically, 3.1) |
| WB sprite | lit. 10 | `wbspritelib/WBSprite-library2` | RI | 1994 | – | open |
| ZoneJoy | 64 | `zonejoylib/…` (6 versions) | RI | 1995-08-12 | 3 and 4 **=** BSS2/AB3 (3656 B); 1 and 2 **=** BUM/OL (3396 B); the two 020 versions (1994) ≈ 76 % | ZoneJoy-Library4 (built identically, 3.1) |

Also in `rwelibsrc/`: `Debuglib/defaultdbug50.bb2` (a debugger, not a
library; uses `GetPubScreen` of a later screenslib, section 4), `optidec.bin`, documentation
(`RIShapesLib.doc`, `RIPackLib.doc`, `ptlib/Docs`), icons and `.bak`
copies [S].

## 3 What "reference" means here

For the acidlibs the reference was the binary of the 2.10 release. For
these libraries there is no release that contains all of them; the CD
and AmiBlitz3 have binaries of about half of them, in partly several
builds. Proposed rule (to be decided in step 3 of `TODO.md` §4):

1. **A source version whose declarations equal a binary** (22 libraries,
   column "Reference" above): that version, the binary is the reference
   for the comparison (declarations as with `blitzlibcompare`, code as
   with `asmcmp`). If several versions equal the same binary (Req,
   TrackDisk, ZoneJoy), the code decides.
2. **A source newer than every binary** (LES debug, Pack, Sort, maybe
   more): the newest source; the binary only as a comparison for the
   unchanged parts.
3. **Only a source** (about 25 libraries): the newest version; a
   reference build from the Blitz source would be needed to compare –
   whether blitz2 can build a library from `libmacs.bb` sources is step
   3.
4. **Not ported:** `ROBNORTHFAST` (license); examples, tests and the
   template (`emptylib`, `myaudiolib`, `myctrlclib`, `TestBlit`,
   `PredatorBlit`, `popuplib/test2`, `shapeslib/Test`, `Comparison`,
   `ptlib/SamplePrograms`) – they may serve as test programs.

### 3.1 Reference build with blitz2 2.10 (step 3)

**How libraries were built** (`EXTRAS/LIBSDEV/DOCUMENTATION`, Acid's
"Blitz2 Library Developers Documents"): the source is compiled with
blitz2 by *Create Executable*; the executable *is* the library file
("create an executable by the name of 'Blitzlibs:userlibs/examplelib.obj'").
The macros come from `libmacs` (resident `libmacs.res` or `XINCLUDE
"libmacs.bb"`); runtime errors must be off and no BASIC token (`End`)
may be in it [Doku].

**Checked with the CD** [B]: `cdrun` compiled `EXTRAS/LIBSDEV/USERLIBSRC/`
with blitz2 2.10 (`blitz2-cd-reference`): 9 third-party libraries are
**byte-identical** to their binaries on the CD (AaronsIcon, ElmoreDos,
ElmoreFuncs, ElmoreMath, ElmoreSys, FuzziesReq, RomulusConsole,
RomulusCrunch, RomulusRequest). The RI sources there are older versions
(e.g. RIEncrypt without the optional parameters) – no binary of them is
on the CD.

**The RWE sources** (`cdrun` over a staging directory, raw material
`.cache/rwelibs-analysis/refbuild/`): with the files of
`EXTRAS/BLITZLIBS/` (libmacs, libnums, residents) and `sbra.res` =
`BUM10A/BLITZLIBS/ASMMACS.RES` ("donated by Steve Matty – RI/RWE –
August '96": the `!SBRA`… short branch macros) 34 of the 123 sources
compile, **10 are byte-identical** to a binary [B]:

| Source | Identical to |
|---|---|
| `Commoditieslib/CommoditiesLib2` | BSS2/AB3 `RICOMMODITIESLIB` |
| `Encryptlib/Encrypt-Library` | BSS2/AB3 `RIENCRYPTLIB` |
| `FXLib/FX-Library13` | BSS2/AB3 `RIFXLIB` |
| `GFXLib/GFX-Library2` | BSS2/AB3 `RIGFXLIB` |
| `packlib/Pack-Library8` | BUM `RIPACKLIB` |
| `reqlib/ReqLib2` | OL/AB3 `RIREQLIB` (so ReqLib2 is the reference, not 1 or 3) |
| `tooltypeslib/ToolTypes4` | BSS2/AB3 `RITOOLTYPESLIB` |
| `trackdisklib/trackdisk`, `trackdisk2` | BSS2/AB3 `RITRACKDISKLIB` (both the same code) |
| `zonejoylib/ZoneJoy-Library4` | BSS2/AB3 `RIZONEJOYLIB` (so 4, not 3) |

So a reference build is possible: the binaries are blitz2 2.10 output of
these sources. Two things were needed:

* **`INCDIR` + an absolute nested include fails in blitz2 2.10**:
  `INCDIR "BlitzLibs:"` + `XINCLUDE "LibMacs.bb"` gives "Can't open
  Include" at the XINCLUDE line. INCDIR itself works; the failing open is
  line 278 of `LIBMACS.BB`, `INCLUDE "blitzlibs:libnums.bb"`: `makeiname`
  (`blitz2.s:11947`) always puts `incdir` in front of the name, also of
  an absolute one (with `:`) – "BlitzLibs:blitzlibs:libnums.bb"; the
  error is reported at the outer line (#341). Besides, `incdir`
  (`blitz2.s:14862`) is never reset: it carries over into the next
  compile of the same blitz2 session (#342). Both are original bugs; the
  C port has them as well (experiments with both compilers in Amiberry,
  `.cache/…/refbuild/`, and the bug-orig tests of the issues). No source
  of the CD uses INCDIR with a path. For the reference build the INCDIR lines are replaced by
  `;` (INCDIR generates no code) [B, S].
* `sbra.res` (and other residents of RWE) exist nowhere; `ASMMACS.RES`
  serves for `sbra.res`.

**Not yet buildable** (89 sources) [B]: missing constants of residents
that exist nowhere – `lvo.res` (`JSR _RemPort(a6)`, `_CloseLibrary`,
`_Wait` …), `custom.res` (`custom`, `#dmacon` …), `datatypes.res`,
`AHI.res`; library numbers no libnums defines (`#ritrackerlib`,
`#ricompactdisclib`, `#rigtmenulib` as written in the sources …); the
`BB22` files (blitz2 cannot read them); further includes (`optidec.bin`,
`registry.inc`, `bomberman:shapeoffsets.bb`). 24 sources compile but
match no binary (older or newer versions, or other options).

## 4 Tokens

Token = `$8000 | number<<7 | (index + 1)` (checked with acidlibs'
sortlib 110 against `tests/data/ted-tokens.txt`). For reading the
sources a token list is needed that knows the libraries they call: the
token tables of the binaries give 1773 tokens beyond the 1.95 list
(provisional: per number the version with the most entries; for numbers
used by several libraries – 7, 10, 15, 35, 40, 41, 56, 60, 66, 69, 75,
107, 109 – names may be wrong). For the port this has to become a
generated list from exactly the reference binaries (step 2).

`$CF12` (`Debuglib/defaultdbug50.bb2`, `*scr.Screen=GetPubScreen(0,"Workbench")`)
is entry 18 of screenslib (158). The screenslib of 2.10 ends with entry
15 (`ScreenTags`, `$CF0F`); AmiBlitz3's has 20, 16 to 20 are
`FrontScreen`, `NewCloseScreen`, `GetPubScreen`, `ReleasePubScreen`,
`ScreenPens2` (`BlitzLibs/tokenlist.txt`). So `defaultdbug50` was written
for a later acidlibs – one that is not on the CD [B, AB3]. The other two
"tokens" (`$B900`, `$AA00`) were bytes of a file header (4.1).

### 4.1 The `BB22` format

9 files of `rwelibsrc/` do not have TED's format (lines of tokenised
text, each ended by a 0 byte) but begin with `BB22` [B]:
`ANIMLib/Anim-Library3.bb` (+ `.bak`), `AppLib/applib3.bb` (+ `.bak`),
`Registry/Registry3.bb`, `rexxlib/NEWAREXXLIB.bb`,
`shapeslib/RIShapesLib2.bb`, `soundlib/risoundlib2.bb` (+ `.bak`) – all
saved between June and September 1997 – shortly before the date of the
CD's files (2 October 1997), yet none of the CD's editors knows the
format (below).

| Version (word +4) | Header | Line |
|---|---|---|
| 10 (5 files) | 16 bytes: `BB22`, then the words 10, a, b, 0, c, 2 (e.g. `applib3`: 10, 13, 25, 0, 441, 2) | 2 bytes (always 0 so far), tokenised text, 0 byte |
| 8 (`Anim-Library3`) | 14 bytes: `BB22`, then 8, a, b, 0, c | 1 byte, tokenised text, 0 byte |

All five files of version 10 split into lines exactly to their end. The
meaning of the words a, b, c (cursor? first line shown?) and of the
line prefix is not known. Which editor wrote it is not known either:
none of the editors on the CD (TED 1.1, TED 2.1, SuperTED 2.1/2.23/2.50)
contains `BB22`, nor do AmiBlitz3's PED and compiler (sources and history
searched) [B, AB3]. That it comes from RWE's own later editor (the files
are RWE's, 1997) is a conclusion [Schluss].

For reading these sources a reader for `BB22` is needed (teddetok only
splits at 0 bytes: the text survives, the line structure does not; the
inventory of section 2 is not affected, it only uses the `!name` lines).

AmiBlitz3's `BlitzLibs/tokenlist.txt` (`NAME.obj(NUM):INDEX token`) has
the same names for the RI libraries it contains; it knows none of the
source-only libraries [AB3].

## 5 Documentation and examples of the release

`BSS2/REDBLITZLIBS/DOCUMENTATION/DOCS.CAT` (93 933 bytes) and
`BSS2/REDBLITZLIBS/EXAMPLES/EXAMPLES.CAT` are archives of RWE's
installer: one StoneCracker 4.04 (`S404`) packed file after the other,
each behind its name (`REDBlitzLibs.guide.stc` …). `BSS1/INSTALL`
unpacks them with `BSS1/INSTALLFILES/PROGRAMS/CONCAT DECAT ARCHIVE DIR`;
the same call in Amiberry gives [B]:

* **`DOCS.CAT`**: 30 AmigaGuides (284 605 bytes) –
  `REDBlitzLibs.guide` (overview: "COPYRIGHT 1996 Red When Excited Ltd.
  All Rights Reserved", the RIBlitzLibs renamed REDBlitzLibs with "many
  new commands since the public release of RIBlitzLibs" and bug fixes;
  needs Kickstart 1.3, Blitz Basic 2 V1.7 or later, V1.9 for string
  functions) and one manual per library, all "**V2.5**": AmosFunc, Anim,
  App, ARexx, Commodities, CompactDisk, CopperFX, DataTypes, Debug, DOS,
  Encrypt, Eval, FastSerial, FNS, FX, GFX, GTMenu (twice:
  `RIGTMenuLib.guide`, `RIGTMenusLib.guide`), Misc, NonVolatile, Pack,
  Shapes, Sort, Sound, STCDecrunch, STC, ToolTypes, TrackDisk, ZoneJoy.
  So the release BSS2 consists of these 28 libraries – the binaries of
  `BSS2/REDBLITZLIBS/LIBRARIES/USERLIBS/` [Schluss].
* **`EXAMPLES.CAT`**: 69 files, 30 example programs (`RI…Lib1.bb` …,
  tokenised, with `.xtra`) and their data (fonts, shapes, IFF, an anim).
* AmiBlitz3's `Docs/UserLibs/RI*.guide` are later edits of these
  manuals (e.g. RISortLib V2.6 "Updated by mrodfr", 2007; links to
  AmiBlitz3); its examples are there only as `.ab3` today [AB3]. The
  originals are the ones of the CD.

Use: the manuals describe the behaviour the port has to have, the
examples are test programs. They are RWE's commercial documentation
("Unauthorised duplicaton … prohibited"), not part of the release of
1999 – reference only, not in the repository. Unpacked copies:
`.cache/rwelibs-analysis/bss2/`. `BSS2/REDDEBUGGER/EXAMPLES/TOKENLIST`
(also `S404` inside) is a program (`TOKENLIST.BB` compiled), not a token
list.

## 6 Method

Raw material (git-ignored): `.cache/rwelibs-analysis/` with `INDEX.md`
(scripts and outputs; throw-away, Python with `python3 -I`, not in the
repository).

1. All `*.OBJ` of the CD and `BlitzLibs/UserLibs/userlibs/*.obj` of
   AmiBlitz3 read with `tools/lib/blitzlib` (number, size, entries with
   name and help text).
2. The sources made readable with `teddetok` and the extended token list.
3. Per source: `!libheader`, `!name` entries (order, help text), the
   header comments (authors, dates).
4. Matching: equal declarations (names in order and help texts) =
   "**=**"; otherwise the overlap of the name sets.
5. Numbers from AmiBlitz3 `Sourcecodes/Includes/System/sorted_libnums.ab3`
   and `Sourcecodes/Amiblitz3/BlitzLibs/libnums.ab3`; clashes with the
   number lists of `BLITZ2/ACIDLIBS` and `BLITZ2/DEFLIBS`.

## 7 Open questions

1. ~~How were these libraries built, can blitz2 do it here?~~ Yes:
   *Create Executable* with libmacs; 10 RWE sources build
   byte-identically with blitz2 2.10 (3.1). Open: the residents RWE
   used that exist nowhere (`lvo.res`, `custom.res`, `datatypes.res`,
   `AHI.res`) and the missing library numbers.
2. Which libraries belong together as one product (e.g. the three Blitz
   Bombers libraries, MapEd/MapShapes, Registry/support)?
3. The libraries with literal test numbers (5, 8, 10, 19, 22, 39, 195,
   197, 198): which number do they get as userlibs?
4. The Protracker play routine in ptlib: whose is it (the readme of
   Protracker 2.3A names the authors), and may it be taken over?
