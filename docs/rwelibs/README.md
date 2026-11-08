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
  vs acidlibs' polylib, `docs/v2.10/libs.md` §4; resolved by the variant
  `rwe-1999-eval`, polylib at 87 = `#renderlib`, `port/rievallib.md`);
  among the RWE libraries only risoundlib and its successor risoundlib2
  (both `#risoundlib` = 45: variant `rwe-1999-ahi`,
  `port/risoundlib2.md`). Several sources carry
  literal test numbers (5, 8, 10, 19, 22, 39, 195, 197, 198), some of
  them used by other libraries [S, AB3].
* **License:** `ROBNORTHFAST.bb` contains Rob Northen's PRO-PACK unpacker
  "Copyright (c) 1991,92 Rob Northen Computing, U.K. All Rights
  Reserved." – not covered by RWE's release, **not to be ported**. The
  Protracker 2.3A play routine (ptlib, CIA tracker) is Peter "Crayon"
  Hanning's / Mushroom Studios', released as "Public Domain" with the
  Protracker 2.3A disk; ported on that basis (decision 2026-10-30,
  `archive/README.md`). Further credits
  in the sources (Ott/The Farm: bug reports; Delirium Softdesign: a
  busy-wait fix in the CIA tracker library; Nigel Hughes: an addition to
  RIShapesLib) [S].
* **Tokens:** with the token list of blitz2 1.95 alone 100 of the 123
  sources are readable; with the token tables of the binaries added
  (1773 tokens, provisional) 120. Of the other three, two are no token
  problem: they are saved in another file format, **`BB22`** (section
  4.1; 9 files in all; `tools/bb22` converts them to TED's format); the third uses `GetPubScreen` (`$CF12`) of a
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
| Arrays | 7 (`#riarrayslib`: RWE's list, REDHELP.DAT, a program using it; clash `om3dlib`, AB3) | `ArraysLib` | RI | – | – | ArraysLib (the only version; reference build, 3.3), ported 2026-10-30 (`port/riarrayslib.md`) |
| CompactDisk | 14 (`#ricompactdisclib`, lit. 14) | `CDlib/cd`, `cd2`, `cd3`, `cdSCSI` | – | 1995-07-03 | ≈ 96 % OL `RICOMPACTDISKLIB` | open |
| Commodities | 89 | `Commoditieslib/CommoditiesLib2` | – | – | **=** BSS2/AB3 (2604 B) | CommoditiesLib2 |
| CopperFX | 11 (bin.) | `Copperlib/CopperFX3` | RI | 1995-03-08 | **=** BSS2/AB3 (1856 B) | CopperFX3 |
| DOS | 33 | `DOSLib/ridoslib`, `ridoslib2` | – | – | ridoslib2 **=** BSS2/AB3 (2416 B) | ridoslib2 |
| Debug | 51 | `Debuglib/Debuglib4` | LES | 1995-02-27 | **=** OL `RIDEBUGLIB` (1380 B); BSS2's differs | Debuglib4 (BSS2: check) |
| LES debug | 49 (`#riLESDEBUGlib`) | `Debuglib/LESDEBUGlib`, `LESDebuglib/LESDEBUGlib` | – | 1996-09-17 / 1996-12-01 | the older **=** AB3 `RILESDEBUGLib` (11216 B; the code is another version); the newer ≈ 94 %; BUM `RILESDEBUGLIB` older (26 commands) | LESDebuglib (reference build with the font cut from AB3's binary, ported 2026-10-30, `port/rilesdebuglib.md`) |
| Disasm | 50 (bin.) | `Disasmlib/Disasm-library.bb2` | LES | 1994 | **=** AB3 `RIDisAsm` (9228 B) | Disasm-library |
| Encrypt | 52 | `Encryptlib/Encrypt-Library` | RI (McNamara) | 1996-01-21 | **=** BSS2/AB3 (636 B); BUM/OL (680 B) same declarations | Encrypt-Library |
| Eval | 121 (bin.; clash with acidlibs: variant `rwe-1999-eval`) | `EvalLib4` | RI | 1995-11-17 | ≈ 83 % BSS2 `RIEVALLIB` | BSS2 binary (ported 2026-10-30, `port/rievallib.md`) |
| FastSerial | 36 | `FASTSerial/fastseriallib`, `2`, `3` | – | – | 3 **=** BSS2/AB3 (1920 B) | fastseriallib3 |
| FNS | 93 | `FNSLib/fns-library11`; `FNS2` (lit. 39, 3 entries) | RI | 1995-11-14 | 11: names equal BSS2, 1 help text differs | fns-library11 (check) |
| FX | 66 | `FXLib/FX-Library13` | RI (McNamara) | 1996-01-21 | **=** BSS2/AB3 (4632 B) | FX-Library13 |
| GFX | 65 | `GFXLib/GFX-Library2` | – | 1996-01-21 | **=** BSS2/AB3 (2036 B) | GFX-Library2 |
| GTMenu | 37 | `GTMenuLib/GTMenuLib`, `2`, `3` | RI | 1996-03-10 | ≈ 92 % BSS2 | open |
| Linked list | 12 (bin.) | `Listlib/listlib4` | RI | – | ≈ 92 % OL `RILISTLIB` | open |
| MapEd | 8 | `MapEd/MapEd_Bit`, `MapEd_Bit2` | RI | – | – | MapEd_Bit2 (reference build): file date 1997-01-26 (MapEd_Bit: the copy date 1996-07-26), the 65535-byte map fix and 9 commands more; MapEd_Bit does not compile (3.1); ported 2026-10-30 (`port/rimapeditorlib.md`) |
| MapShapes | 6 | `MapEd/MapShape`, `MapShape2` | RI | 1996-12-22 | – | MapShape2 (reference build): its history 1996-07-08/12-22 (FreeMapShapes, the blit fix, the group commands); MapShape is the older subset; ported 2026-10-30 (`port/rimapshapeslib.md`) |
| ReqTools | 54 (`#neilsreqtoolslib`) | `RTLib` | Neil O'Rourke | 1997-01-29 | ≈ 97 % `BUM10A/BLITZLIBS/OTHERLIBS/NEILSREQTOOLSLIB` | open |
| Registry | 3 | `Registry`, `Registry/Registry2`, `Registry3` | RWE | 1997 | – | Registry3 (reference build, `BB22`), ported 2026-10-30 (`port/riregistrylib.md`, tested with a mock library): file date 1997-07-10, Registry2's 27 commands + 8; Registry (13) older. Needs `registry.library` at run time, which exists nowhere |
| Registry support | – (`#registrysupplib`) | `Registry/regsupplib` | RWE | 1997 | – | not ported: needs `registry.inc` (tags `#SS_ApplicationName`, `#SS_ApplicationParameters`) and `registrysupp.library`, neither of which exists anywhere (REDRegistry was only given out as pre-release) |
| Snow | lit. 195 | `Snow/SnowLib3` | RI | 1994 | – | SnowLib3 (reference build), ported 2026-10-30 (`port/risnowlib.md`); number 195 (the literal: free in rwe-1999, Acid's `#systemcallslib` is not in it) |
| Blitz Bombers | 30 | `blitzbomberslib/blitzbombers86`, `87`, `88` | RI | 1996-10-04 | – | not ported (decision 2026-10-29): needs `bomberman:shapeoffsets.bb` of the game, not in the archive |
| Blitz Bombers 2 | – (`#blitzbomberslibtwo`) | `blitzbomberslib/blitzbombersTwo00` | RI | – | – | blitzbombersTwo00 (reference build; ported 2026-10-30, `port/blitzbomberslibtwo.md`); number 31 (ours: RWE's `RESERVEDLIB_8`, next to `#blitzbomberslib` 30) |
| BB blit | 19 (lit., = `#bb_blitlib`) | `blitzbomberslib/BB_BlitLib4` | RI (McNamara) | 1996-01-30 | – | BB_BlitLib4 (reference build; ported 2026-10-30, `port/bb_blitlib.md`) |
| BB map | 34 (`#ribbmaplib`, RWE's list) | `blitzbomberslib/bbmaplib2` | RI | 1995-09-30 | – | bbmaplib2 (reference build; ported 2026-10-30, `port/ribbmaplib.md`) |
| (tests) | lit. 5 / – | `blitzbomberslib/TestBlit`, `PredatorBlit` | – | – | – | not a library / test |
| C2P | lit. 8 | `c2plib`, `c2plibA` | RI | 1995 | – | not ported: a preliminary Chunky (its one command `C2P` is chunkylib's `ChunkyToPlanar` on a 320×256×8 bitmap; chunkylib's history starts with it, 10/8/95), and 8 is MapEd's in RWE's list |
| Chunky | 24 | `chunkylib`, `chunkylib2` | RI | 1995-08-14 | – | chunkylib (reference build), ported 2026-10-30 (`port/richunkylib.md`): chunkylib2 replaces the C2P code by `IncBin "c2p"` from `TEMP:`, a file that exists nowhere (and drops chunkylib's checks), so it does not compile |
| DataTypes | 25 | `datatypeslib` … `datatypeslib6` | RI / LES (Matty) | 1995 | 6 ≈ 95 % BSS2 | open |
| (template) | – | `emptylib` | RI | 1995 | – | not a library |
| LVCallHook | 27 (bin.) | `lvcallhook/LVCallHook3` | RWE | 1996 | **=** AB3 `RILVCallHook` (1172 B) | LVCallHook3 |
| Misc | 47 | `misclib/misclib`, `misclib2` | – | 1996-05-23 | misclib2: names equal AB3 `Rimisclib`, 2 help texts differ; BSS2 ≈ 72 % | open |
| (argument tests) | lit. 198 | `myaudiolib`, `myctrlclib` | – | – | – | test |
| Display | 29 | `mydisplaylib` | RI | – | – | mydisplaylib (reference build), ported 2026-10-30 (`port/ridisplaylib.md`) |
| NonVolatile | 35 (clash `mazfpulib`, AB3) | `nonvolatile` | RI | 1994 | **=** BSS2/AB3 (640 B) | nonvolatile |
| Pack | 73 | `packlib/Pack-Library8`, `8_Mc`, `9` | RI | 1996-05-09 | 8 **=** BUM (2564 B), 8_Mc **=** OL (2388 B); 9 newer, ≈ 88 % | open (9 newer than the binaries) |
| PopUp window | – (`#popupwinlib`) | `popuplib/popupwinlib` (+ `test2`) | RI | 1994 | – | popupwinlib (reference build; = its `.bak` without a debug `ILLEGAL`), ported 2026-10-30 (`port/popupwinlib.md`); number 16 (ours: RWE's `RESERVEDLIB_10`; AmiBlitz3's port took 24, RWE's `#richunkylib`) |
| Protracker | 58 (`#ritrackerlib`, RWE's list) | `ptlib/ptlib` … `ptlib7`, `ptlibmatty` | Protracker 2.3A routine, adapted | 1995-04-08 | – | ptlib6 (reference build), ported 2026-10-30 (`port/ritrackerlib.md`: why ptlib6; decision to port: `archive/README.md`); not the acidlibs `trackerlib` (96: NoiseTracker in the vertical blank, Module objects, `LoadModule`…) but its own library: `TrackInit address`, `TrackPlay` (called by the program: the CIA interrupt code is dead), `TrackStop`, `TrackDMA`, `Tracker`, `TrackerChannels`, `TrackerPause` (`TrackerTimer` only in ptlib4/7/matty); no binary; the number only in RWE's list |
| CIA tracker | 56 (`#ciatrklib`; clash `xbciatrackerlib`, AB3) | `ptlib/CIATrackerLib1` (+ `SamplePrograms/TrackerTest`) | Neil O'Rourke, Delirium Softdesign | – | ≈ 88 % OL `XBONESCIATRKLIB` | CIATrackerLib1 (ported 2026-10-30, `port/ciatrklib.md`) |
| Req | 79 | `reqlib/ReqLib`, `2`, `3` | – | – | all three **=** OL/AB3 `RIREQLIB` (964 B) | ReqLib2 (built identically, 3.1) |
| ARexx | 133 (`#myconst=133`) | `rexxlib/…` (9 versions) | RI; `NEWAREXXLIB`: RWE (Matty) | 1997-02-09 | ≈ 76 % BSS2 `RIAREXXLIB` | open |
| Intuition | 15 (`#riintuilib`, RWE's list) | `riintuilib`, `riintuilib2` | RI | 1995 | – | riintuilib2 (reference build), ported 2026-10-30 (`port/riintuilib.md`): Rast objects (`IPBitMap`) and `IPOutput` redirecting Print through inputoutputlib replace riintuilib's own `IPPrint`/`IPRastPort` |
| PRO-PACK | lit. 197 | `ROBNORTHFAST` | Rob Northen Computing (All Rights Reserved) | 1991/92 | – | **not ported** (license) |
| Serial | lit. 22 | `seriallib/Serial-Library` | – | – | – | Serial-Library (reference build), ported 2026-10-30 (`port/riseriallib.md`); number 22 (the literal: RWE's list gives it to `#riblitlib`, of which no source exists, so it is free in rwe-1999) |
| Shapes | 46 | `shapeslib/RIShapesLib`, `RIShapesLib2` (+ `Test`, `Comparison`) | RI, Nigel Hughes | – | ≈ 92 % BUM `RISHAPESLIB`; RIShapesLib2 in the `BB22` format | open |
| Shapez | 21 | `shapezlib` | RI | 1995 | – | shapezlib (reference build), ported 2026-10-30 (`port/rishapezlib.md`) |
| Sort | 13 | `sortlib/RISortlib2` | RI | 1996-08-30 | ≈ 80 % BUM/BSS2/OL `RISORTLIB` (one file) | open (source newer than the binary) |
| Sound | 45 | `soundlib/risoundlib`, `risoundlib2` | – | – | risoundlib **=** BSS2/AB3 (496 B); risoundlib2 (8 entries) – | risoundlib (BSS2 binary); risoundlib2 (`BB22`, AHI, resident `AHI.res`; reference build) ported 2026-10-30 in the variant `rwe-1999-ahi` instead of risoundlib (same number and CludgeSound, `port/risoundlib2.md`) |
| STC decrunch | 18 | `stcdecrunchlib` | – | – | **=** BSS2/AB3 (576 B) | stcdecrunchlib |
| StoneCracker | 17 | `stclib` | – | – | **=** BSS2/AB3 (1136 B) | stclib |
| Thread | 44 (`#rithreadlib`, RWE's list) | `threadlib` | RI | 1995 | – | threadlib (reference build), ported 2026-10-30 (`port/rithreadlib.md`) |
| ToolTypes | 59 (bin.) | `tooltypeslib/ToolTypes4` | RI (McNamara) | 1995-04-05 | **=** BSS2/AB3 (3564 B) | ToolTypes4 |
| TrackDisk | 91 | `trackdisklib/trackdisk`, `trackdisk2` | – | – | both **=** BSS2/AB3 (1892 B) | trackdisk2 (both build identically, 3.1) |
| WB sprite | lit. 10 | `wbspritelib/WBSprite-library2` | RI | 1994 | – | WBSprite-library2 (reference build), ported 2026-10-30 (`port/riwbspritelib.md`); number 10 (the literal, free in rwe-1999) |
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
   for the comparison (declarations as with `blitzlib compare`, code as
   with `refcheck asm`). If several versions equal the same binary (Req,
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

**Checked with the CD** [B]: `corpusrun cd` compiled `EXTRAS/LIBSDEV/USERLIBSRC/`
with blitz2 2.10 (`blitz2-cd-reference`): 9 third-party libraries are
**byte-identical** to their binaries on the CD (AaronsIcon, ElmoreDos,
ElmoreFuncs, ElmoreMath, ElmoreSys, FuzziesReq, RomulusConsole,
RomulusCrunch, RomulusRequest). The RI sources there are older versions
(e.g. RIEncrypt without the optional parameters) – no binary of them is
on the CD.

**The RWE sources** (`corpusrun cd` over a staging directory, raw material
`.cache/rwelibs-analysis/refbuild/`): with the files of
`EXTRAS/BLITZLIBS/` (libmacs, libnums, residents) and `sbra.res` =
`BUM10A/BLITZLIBS/ASMMACS.RES` ("donated by Steve Matty – RI/RWE –
August '96": the `!SBRA`… short branch macros), the reconstructed
residents and library numbers (below) 84 of the 123 sources compile,
**16 are byte-identical** to a binary [B] (compared as files; the
`corpusrun cd` table hashes with the trap blocks zeroed and misses
`Debuglib4`):

| Source | Identical to |
|---|---|
| `AppLib/applib2` | BSS2/AB3 `RIAPPLIB` |
| `Commoditieslib/CommoditiesLib2` | BSS2/AB3 `RICOMMODITIESLIB` |
| `Copperlib/CopperFX3` | BSS2 `RICOPPERFXLIB` |
| `Debuglib/Debuglib4` | OL `RIDEBUGLIB` |
| `DOSLib/ridoslib2` | BSS2 `RIDOSLIB` |
| `Encryptlib/Encrypt-Library` | BSS2/AB3 `RIENCRYPTLIB` |
| `FASTSerial/fastseriallib3` | BSS2 `RIFASTSERIALLIB` |
| `FXLib/FX-Library13` | BSS2/AB3 `RIFXLIB` |
| `GFXLib/GFX-Library2` | BSS2/AB3 `RIGFXLIB` |
| `nonvolatile` | BSS2 `RINONVOLATILELIB` |
| `packlib/Pack-Library8` | BUM `RIPACKLIB` |
| `reqlib/ReqLib2` | OL/AB3 `RIREQLIB` (so ReqLib2 is the reference, not 1 or 3) |
| `tooltypeslib/ToolTypes4` | BSS2/AB3 `RITOOLTYPESLIB` |
| `trackdisklib/trackdisk`, `trackdisk2` | BSS2/AB3 `RITRACKDISKLIB` (both the same code) |
| `zonejoylib/ZoneJoy-Library4` | BSS2/AB3 `RIZONEJOYLIB` (so 4, not 3) |

So a reference build is possible: the binaries are blitz2 2.10 output of
these sources. Four things were needed:

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
  of the CD uses INCDIR with a path. For the reference build the INCDIR
  lines are replaced by `;` (INCDIR generates no code) [B, S].
* `sbra.res` (and other residents of RWE) exist nowhere; `ASMMACS.RES`
  serves for `sbra.res`.
* **Residents reconstructed** [B, S]: `lvo.res`, `custom.res` and `datatypes.res`
  exist nowhere; they only hold constants, so they are made from the
  NDK of the toolchain as Blitz sources and saved with *Create
  Resident* (tedstub `resident=`) by blitz2 2.10 (in the build:
  `tools/blitzres`, below):
  * `lvo.res`: `#_Name=-offset` of every public function of the OS 3.1
    libraries (FD files: exec, dos, graphics, intuition, icon, wb,
    layers, gadtools, utility … – 1060 names, no name with two
    offsets). The 50 sources that load it use 87 of them.
  * `custom.res`: `#custom=$dff000`, the registers of
    `hardware/custom.i` (`#dmacon=$96` …) and `#color00`…`#color31`
    (CopperFX3 uses `color00`).
  * `datatypes.res`: the datatypes.library offsets (with `_SetDTAttrs`
    for `SetDTAttrsA`, as the sources write it) and the constants of
    `datatypes/*.i` (`#PDTA_Remap` …). Not in `lvo.res`: two residents
    with the same name give "Clash in Residents" (`checkclash`), and
    the sources that use datatypes functions all load both.
  * `AHI.res` (only `soundlib/risoundlib2.bb`; its `.xtra` loads
    `bb2objtypes.res`, `lvo.res`, `AHI.res`, `amigalibs.res`):
    ahi.device's functions as `#AHI_Name=-offset` – without the `_` of
    `lvo.res`, as the source writes them (`JSR AHI_AudioRequestA(a6)`) –
    from `lvo/ahi.i` (the NDK has no AHI FD file; `blitzres --lvo`), and
    the constants of `devices/ahi.i` (`#AHIR_InitialAudioID`,
    `#AHIST_M8S`, `#AHI_DEFAULT_ID` … – 180 names, none of them in
    `lvo.res`, `amigalibs.res` or `bb2objtypes.res`; `CMD_WRITE` and
    `TAG_END` come from `amigalibs.res`). From the AHI SDK the
    toolchain installs (AHI 6, 2005; not checked in, like the NDK) –
    RWE's was of 1997 (AHI 4), so the V6 names are extra; the 12 names
    the source uses are older. The structure offsets (`ahir_Type` …) are
    no EQUs in `ahi.i`; the source defines them itself.
  Left out: `_Date2Amiga`, `bpl1mod`, `bpl2mod` – TED tokenises the
  keyword behind the digit (`AMIGA`, `BPL`), no source uses them. A
  source may define a constant of a resident again (FASTSerial
  `#custom`): blitz2 does not check that. Checked: the method gives the
  contents of `BLITZOFFS.RES`, `LIBMACS.RES` and `BB2OBJTYPES.RES` of the
  CD (only unused node bytes differ – memory junk in the originals); the
  values used by applib2, CopperFX3, ridoslib2, fastseriallib3 and
  nonvolatile are confirmed by their byte-identical binaries. Not yet
  confirmed: `datatypes.res` (no datatypes source equals a binary, the
  closest is another version), `AHI.res` (risoundlib2 has no binary:
  its reference build is its reference).
* **Library numbers** [B]: the numbers the sources use but no libnums
  defines (`#ricopperfx`, `#ridoslib` …) appended to the staged
  `LIBNUMS.BB` – 28 names, the number from AmiBlitz3's libnums or of the
  binary with the same declarations; the 14 also in the CD's libnums
  agree. Confirmed by the identical binaries (`#ricopperfx`=11,
  `#ridoslib`=33, `#rifastseriallib`=36, `#rinonvolatilelib`=35,
  `#ridebuglib`=51). The numbers of the libraries without a binary
  (`#ritrackerlib`, `#riintuilib`, `#rithreadlib`, `#ribbmaplib`,
  `#popupwinlib`, `#blitzbomberslibtwo` …): 3.3.
* **`#riarrayslib` = 7** [B, S], from a program that uses the library:
  its only command `FillArray` has index 0, so its token is `$8000 |
  number<<7 | 1`. `Debuglib/defaultdbug50.bb2` (RWE's debugger, in
  the archive) has the token `$8381` (library 7) in `… winstatus(),0`
  and `… memwin(),0`, where AmiBlitz3's `REDDebugger.ab3` writes
  `FillArray winstatus(),0` (AmiBlitz3 moved `FillArray` into its
  `linklistlib`). The code of `FillArray` (ArraysLib built with 7) is
  unchanged in the CD's `BSS2/REDDEBUGGER/PROGRAMS/REDDEBUGGER` and
  `BSS3/REDMAPEDITOR/PROGRAMS/REDMAPEDITOR`. 7 is also Oliver Michel's
  `OM3D` (CD `INTERNET/DOOM4BB2`, AmiBlitz3 `#om3dlib`) and AmiBlitz3's
  `LotanCP2`.

`optidec.bin` (`IncBin` of `stclib`, `stcdecrunchlib`) is in the
archive next to them; with the INCDIR lines gone, `corpusrun cd` runs these jobs
in the staging root (where `libmacs.bb` is), so it is staged there too.
Both compile; their binaries contain `optidec.bin` unchanged but are
other versions (644/1296 bytes against 648/1276).

**Not yet buildable** (38 sources) [B]: the library numbers above (15
sources, 8 of them ptlib); the `BB22` files (blitz2 cannot read them,
6); further includes (`registry.inc`, `bomberman:shapeoffsets.bb`,
`Blitz2:Debugger/Font.bin` – since 2026-10-30 the font cut from AB3's
binary –, `c2p`, `dt_table.bin`, `FNS:Personal.FNS`, 9); programs, not
libraries, with tokens of other libraries (`defaultdbug50`, `TrackerTest`,
`Comparison`, `Test`, `test2`, 5); single errors (`func-library4`,
`MapEd_Bit`, `emptylib`). 68 sources compile but match no binary – as far as
checked older or newer versions (`func-library7`: the binary has one
`JSR _UnLock(a6)` more).

**Ported (stage A, 2026-10-29):** the 15 libraries of the table above
are real C in `libs/userlibs/rwe/` (variant `rwe-1999`): declarations
equal to the binary, behaviour compared with it in Amiberry; notes per
library in `port/`. Stage B with a binary as the reference (AMOS
functions, Sound, StoneCracker, FNS) and Anim followed the same day;
the 12 libraries whose reference is the reference build of their source
(3.2 "Source": LVCallHook, Misc, STC decrunch, CompactDisk, GTMenu,
Linked list, ReqTools, DataTypes, ARexx, Shapes, Sort, Disasm) on
2026-10-30, ptlib (Protracker, ptlib6) and the CIA tracker
(CIATrackerLib1; both with the Protracker 2.3A play routine, ported on
the basis of its release as "Public Domain", decision 2026-10-30;
reference build) the same day, and LES debug (LESDebuglib; `Font.bin`
cut from AmiBlitz3's binary, `archive/README.md`; reference build) too.
Also on 2026-10-30: Eval (121, the BSS2 binary, in the variant
`rwe-1999-eval` with polylib at 87) and stage D (3.3), the 16 libraries
without any binary, with the reference build of their source:
Registry, MapShapes, Arrays, MapEd, WB sprite, Intuition, PopUp
window, BB blit, Shapez, Serial, Chunky, Display, Blitz Bombers 2, BB
map, Thread, Snow. Then risoundlib2 (Sound with AHI, the resident
`AHI.res` from the toolchain's AHI SDK; reference build; in the variant
`rwe-1999-ahi`, where it replaces risoundlib: both are number 45 with
the command CludgeSound). All 53 libraries of `RWE_LIBS` are ported; not
ported: Registry support and Blitz Bombers (includes that exist
nowhere), PRO-PACK (license), C2P (a preliminary Chunky), the tests
and templates (section 2).

**In the build** (2026-10-29; `libs/userlibs/rwe/refbuild.cmake`,
`residents.cmake`, README "Userlibs", "Tests"): the reference build is
no longer a throw-away run. The residents are written by the host tool
`tools/blitzres` directly in the format of *Create Resident* (no
emulator), from the same NDK files with the same rules; their contents
equal those blitz2 2.10 makes of them as Blitz source (ctest
`rwe_residents_cd` in Amiberry, compared without the unused node bytes). The staging (target `rwe_refstage`,
`build/<preset>/libs/userlibs/rwe/refstage/`) is the one above with
two changes: the `INCDIR` lines are neutralised on the bytes of the
sources (`tools/bblines`) – a text round trip through TED tokenises
other lines with a different token list (`Pack-Library8`: the label
`allocmem` became a token) – and the RWE numbers come from the
variant's `libnums.h` (its own `LIB_` defines), tokenised and appended
to the bytes of the CD's `LIBNUMS.BB` (its text has a line longer than
TED takes). `FNS:` is staged with `PERSONAL.FNS` of BSS2's examples,
so `fns-library11` compiles too. ctest `rwe_refbuild` (Amiberry, about
20 s) compiles the sources of all `RWE_LIBS` entries to
`build/<preset>/libs/userlibs/rwe/ref/NAME.obj` – all compile (the
19 of stages A/B, those of 3.2 "Source" and 3.3; a `BB22` source is
converted by `tools/bb22` first) – and
`rwe_refbuild_NAME` checks that the 15 of stage A equal their binaries
byte for byte. An entry whose reference is the reference build of its
source (stage B/C "Source", 3.3) writes `refbuild` as REFERENCE: its
`rwe_declarations_NAME` and its pair in `libs_emulator` use
`ref/NAME.obj` (with the fixture of `rwe_refbuild`).

### 3.2 Which version is the reference (stages B and C)

For the libraries whose source and binary differ (stage B: same
declarations, other code; stage C: other declarations too) the
differences were analysed and weighed (raw material
`.cache/rwelibs-analysis/versions/NAME.md`): fixes or features on one
side only, dated header comments (the archive's 1996-07-26 and the CD's
1997-10-02 are copy dates, only later dates are edit dates), version
strings, the order of the source versions, the release docs (REDBlitzLibs
V2.5 of BSS2). Binaries built by AmiBlitz (hunk ending `7000 4E75` plus
16 × `$01`) are later changes of the AmiBlitz3 team, not RWE's. Decision
of 2026-10-29 – the reference of the port:

| Library | Newer | Reference |
|---|---|---|
| AMOS functions | AB3 (BSS2 + AB3 fixes, e.g. ChangeDir `MOVE.L oldlock`) | the AB3 binary |
| Disasm | the AB3 binary is an AB3 derivative (THOR's disassembler.library) | the RWE source |
| LVCallHook | source (LVCallHook3, 1997-01-14) | source |
| Sound | BSS2 (risoundlib + `sloop`/`srep`) | binary |
| Misc | source misclib2 newer than BSS2; AB3 an AB3 extension | source |
| LES debug | source LESDebuglib (1996-12-02) | source |
| FNS | BSS2 (a wrong guard removed) | binary |
| StoneCracker | BSS2 (`S404` checks) | binary |
| STC decrunch | unclear | source |
| Anim | source (BSS2 = source without the unfinished `RIDiskAnimInit`) | binary (the source does not compile, below) |
| CompactDisk | source cdSCSI | source |
| Eval | BSS2 (`{}` indirection, a fix) | binary |
| GTMenu | GTMenuLib3 (4 bytes differ from BSS2) | source |
| Linked list | source listlib4 | source |
| ReqTools | source RTLib (Matty 1997-01-29) | source |
| DataTypes | source datatypeslib6 (13 checks more than BSS2) | source |
| CIA tracker | source 1.02b, between OL (1.02a) and BUM9B (1.03) | source |
| ARexx | source NEWAREXXLIB | source |
| Shapes | source RIShapesLib2 | source |
| Sort | source RISortlib2 | source |
| Pack | BSS2 (Pack-Library9 + IFF size functions, a fix) | binary |

"Source" means: the reference build of that source with blitz2 2.10 (the
binary only for the parts it shares); "binary": the source is brought to
the binary's state first. Anim (changed 2026-10-29): `Anim-Library3`
does not compile – its unfinished `RIDiskAnimInit` uses `diskanimflag`
and `animfile`, defined nowhere ("Constant Not Found", line 153) – and
without that command it is the BSS2 binary but for a padding word, so
the BSS2 binary is the reference (`RWE_REFBUILD_SKIP`). LES debug
(changed 2026-10-30): `Blitz2:Debugger/Font.bin` exists nowhere, but its
2048 bytes are in the binaries (the same in BUM's and AB3's, at the
IncBin's place); only these bytes are taken over
(`libs/userlibs/rwe/rilesdebuglib-font.bin`, cut from AB3's GitHub
binary, `archive/README.md`), the reference build stages them as
`Font.bin`, and LESDebuglib's reference build is the reference. Neither source builds to a binary (the older one's build, a
9524 byte file, equals neither BUM's 8476 nor AB3's 11632 bytes). Also
found: Pack-Library8_Mc is not identical to the OL binary (an older
build with `$VER`); GTMenuLib3, datatypeslib6
and CIATrackerLib1 have the declarations of their binaries; the BB22
files convert to TED's format and build (`versions/tools/bb22.py`, a
throw-away converter: a C tool for it is still to do); the constants of
`datatypes.res` that datatypeslib6 uses are confirmed.

### 3.3 Numbers of the libraries without a binary (stage D)

RWE's "Official Blitz Library Number List" of 1997-02-02 (a copy on the
CD of Amiga MA Magazine 1998 #7, discmaster.textfiles.com …/www/rwe/
liblist_nf.html; AmiBlitz3's `sorted_libnums.ab3` is a copy with RI lines
lost) gives: `#riintuilib` 15, `#ribbmaplib` 34, `#rithreadlib` 44,
`#ritrackerlib` 58 (used by the port of ptlib6), `#riarrayslib` 7, MapEd 8, MapShapes 6, Registry 3,
BB blit 19, Chunky 24, Display 29, Shapez 21. Not in it: Snow (literal
195, probably a test number), Serial (literal 22 – the list gives 22 to
`#riblitlib`), WB sprite (literal 10), PopUp window, Blitz Bombers 2. C2P
(literal 8, like MapEd) is a preliminary RIChunkyLib (its `.xtra` writes
`ChunkyLib.obj`). No binary of any of them exists anywhere. Contexts for
variant names: REDMapEditor (MapEd, MapShapes, partly Shapez), REDRegistry,
Blitz Bombers (BB blit, BB map, Blitz Bombers 2; Serial a guess). Raw
material `.cache/rwelibs-analysis/research/`.

**In the reference build** (2026-10-30): the 16 of them that compile are
`RWE_LIBS` entries with REFERENCE `refbuild` (the newest source, section
2, column Reference); `libnums.h` of `rwe-1999` gives each its number
under the constant its `!libheader` uses (`#rimapeditorlib`,
`#rimapshapeslib`, `#riregistrylib`, `#richunkylib`, `#ridisplaylib`,
`#rishapezlib` as RWE's list names them). The literal numbers 10 (WB
sprite), 19 (BB blit), 22 (Serial) and 195 (Snow) are free in the set
(acidlibs, deflibs, all RWE userlibs) and kept; PopUp window and Blitz
Bombers 2 get RWE's reserved slots 16 and 31 (our choice, no number of
theirs is known). Left out: C2P (superseded by Chunky, 8 is MapEd's).
No stage D source uses tokens of a library without a binary.

## 4 Tokens

Token = `$8000 | number<<7 | (index + 1)` (checked with acidlibs'
sortlib 110 against `tests/data/ted-tokens.txt`). For reading the
sources a token list is needed that knows the libraries they call: the
token tables of the binaries give 1773 tokens beyond the 1.95 list
(provisional: per number the version with the most entries; for numbers
used by several libraries – 7, 10, 15, 35, 40, 41, 56, 60, 66, 69, 75,
107, 109 – names may be wrong). For the port this has to become a
generated list from exactly the reference binaries (step 2).

Done (2026-10-30): the readable sources (target `rwelibsrc`,
`build/<preset>/rwelibsrc/`) take their tokens from the library files of
their variant built here – `blitz2`, the variant's `acidlibs` and
`deflibs` (`build/<preset>/variants/rwe-1999/`, for `EvalLib4` those of
`rwe-1999-eval`) and all RWE userlibs (`…/userlibs/rwe/*.obj`, for
`EvalLib4` with its own `rievallib.obj`) – no longer from CD/AmiBlitz3
binaries named per source (the former USES field of `RWE_LIBS`). So
they need only the Aminet archive; the tokens resolve the same: all 52
files are identical to those read with the CD binaries (the C builds
have the declarations of their references, `rwe_declarations_NAME`).
The reference build still stages the CD's own libraries (3.1).

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
| 10 (7 files) | 16 bytes: `BB22`, then the words 10, column, row, 0, top, 2 (e.g. `applib3`: 10, 13, 25, 0, 441, 2) | 2 bytes (always 0), tokenised text, 0 byte |
| 8 (`Anim-Library3` + `.bak`) | 14 bytes: `BB22`, then 8, column, row, 0, top | 1 byte (always 0), tokenised text, 0 byte |

The tokenised text is that of TED's format (a byte with bit 7 and the
byte after it are a token); in none of the files is the second byte of a
token 0, and there is no LF in a line, so the lines are the same whether
split at every 0 byte or token by token. All 9 files split into lines
exactly to their end [B].

| File | Version | column, row, top | Lines | Longest (bytes) | Line top + row |
|---|---|---|---|---|---|
| `Anim-Library3.bb` | 8 | 19, 1, 96 | 786 | 69 | 97 (19 characters) |
| `Anim-Library3.bb.bak` | 8 | 9, 45, 49 | 733 | 69 | 94 (9 characters) |
| `applib3.bb` | 10 | 13, 25, 441 | 791 | 64 | 466 |
| `applib3.bb.bak` | 10 | 20, 8, 441 | 791 | 64 | 449 (20 characters) |
| `Registry3.bb` | 10 | 0, 54, 571 | 626 | 93 | 625 (the last, empty) |
| `NEWAREXXLIB.bb` | 10 | 0, 6, 606 | 1039 | 92 | 612 |
| `RIShapesLib2.bb` | 10 | 8, 7, 170 | 1151 | 102 | 177 |
| `risoundlib2.bb` (= `.bak`) | 10 | 13, 32, 315 | 494 | 75 | 347 |

(lines counted from 0, characters as TED shows the line)

**The header words are the cursor** [B]: the cursor is in line
top + row (from 0) at column `column` of the line as shown. Evidence:
`applib3.bb` and its `.bak` differ only in the header words and in two
bytes – the offsets in line 462 (`ADD     8(a1),d2`, the `.bak` has
`6(a1)`) and line 466 (`MOVE    6(a1),d2`, the `.bak` has `8(a1)`); after
the edit the cursor is in line 466 at column 13, just behind the digit
typed at column 12. In the `.bak` (the state saved before) and in
`Anim-Library3` (both versions) the cursor stands exactly at the end of
line top + row (column = its length as shown: `MOVE.l  msgid,d2`,
`ilbmbuffer: Dc.l  0`, `      RTS`); in `Registry3` at the start of the
last (empty) line. That top is the first line shown in the window and
row the line of the cursor in it is a conclusion: the sum is the line,
and row goes up to 54 (a window of 55 lines and more, an interlaced or
overscan screen) [Schluss].

**The editor's state in the file** [B, Schluss]: the BB22 sources still
have a normal `.xtra` next to them (same format and size as those of the
TED sources, 1206–1226 bytes); blitz2's `.xtra` holds only compiler
things (the options, the CLI arguments, the object maxima per library:
`savextra_c`), no cursor. The BB22 header adds the editor's part – where
the cursor and the window were – which a TED source loses. AmiBlitz3
later took both into the source, as an "Embedded .xtra Header" of
comment lines at its start, with the compiler options and
`CursorLine`/`CursorColumn` (e.g. `Sourcecodes/Amiblitz3/Shared/
library_offsets.ab3`); BB22 looks like an early step of that: RWE's 1997
editor kept the editor's state in the file, the compiler options stayed
in the `.xtra` [Schluss].

**Not known:** the word at +10 (always 0; a horizontal scroll offset?),
the word at +14 of version 10 (always 2 – the size of the line prefix,
which version 8 lacks with its 1-byte prefix?) and the meaning of the
prefix bytes (always 0; flags per line, e.g. folding or marks?) [Schluss].
The `.bak` files show how the editor saves: like TED it renames the old
file to `.bak` – `NEWAREXXLIB.bb.bak` (March 1997) and `Registry3.bb.bak`
(February 1997) are still in TED's format, `NEWAREXXLIB.bb` is its `.bak`
plus 3 inserted lines, `Registry3.bb` its `.bak` plus 5 blocks of
inserted lines (nothing removed); `risoundlib2.bb.bak` equals the file
(saved twice) [B].

Which editor wrote it is not known:
none of the editors on the CD (TED 1.1, TED 2.1, SuperTED 2.1/2.23/2.50)
contains `BB22`, nor do AmiBlitz3's PED and compiler (sources and history
searched) [B, AB3]. That it comes from RWE's own later editor (the files
are RWE's, 1997) is a conclusion [Schluss].

**The reader:** `tools/bb22` (logic `tools/lib/bb22`) converts a `BB22`
file into TED's format, `bb22 IN OUT`; `bb22 --info IN` shows the
version, the cursor, the line count and the longest line. It is strict:
another version, a word at +10 other than 0 or at +14 other than 2, a
prefix byte other than 0, a line without its 0 byte before the end of the
file, a token byte without its second byte or followed by a 0 byte, an LF
in a line – each is refused with a message (offset, line), nothing is
written. The converted files are read by teddetok with the tokens of the
CD's compiler, acidlibs, deflibs and BSS2's `RIPACKLIB` (`Anim-Library3`
and `RIShapesLib2` call the Pack library, 73): ctest `bb22_rwelibsrc`
(skipped without the archive or the CD); `NEWAREXXLIB` and `RIShapesLib2`
then compile with blitz2 2.10.

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
   *Create Executable* with libmacs; 16 RWE sources build
   byte-identically with blitz2 2.10 (3.1), with `lvo.res`,
   `custom.res` and `datatypes.res` reconstructed from the NDK, `AHI.res`
   from the AHI SDK of the toolchain. Open: `datatypes.res` and `AHI.res`
   unconfirmed, seven library numbers.
2. Which libraries belong together as one product (e.g. the three Blitz
   Bombers libraries, MapEd/MapShapes, Registry/support)?
3. The libraries with literal test numbers (5, 8, 10, 19, 22, 39, 195,
   197, 198): which number do they get as userlibs?
4. ~~The Protracker play routine in ptlib: whose is it, and may it be
   taken over?~~ Peter "Crayon" Hanning / Mushroom Studios, released as
   "Public Domain" with Protracker 2.3A (1993-01-01); ptlib and the CIA
   tracker are ported (decision 2026-10-30, `archive/README.md`; it
   replaces the one of 2026-10-29 not to port ptlib). No replacement
   around Frank Wille's ptplayer. AmiBlitz3 has no ptlib either (its Protracker
   support: Paul Reece's EMLLib, 60, binary only; `ptplay.include`
   around `ptplay.library`).
