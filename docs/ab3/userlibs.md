# AmiBlitz3 changes to the RWE and third-party userlibs (relative to RWE 1999)

What AmiBlitz 2.x/3 changed in the userlibs our port has
(`libs/userlibs/rwe/*.c`, notes `docs/rwelibs/`), as the basis for
deciding which fixes we take over. Documentation only; nothing here is
taken over by this document (some AB3 changes were taken over earlier,
when the reference of the port was chosen, `docs/rwelibs/README.md` 3.2;
they are marked "already in our reference").

## Sources and method

* **Baseline:** RWE's library sources of September 1999
  (`archive/aminet/amibl2src/rwelibsrc/`), the CD's binaries
  (`BSS2/REDBLITZLIBS/LIBRARIES/USERLIBS`, for a few libraries
  `BUM9B`/`BUM10A`/`EXTRAS`), the reference build `rwe_refbuild`
  (`build/amiga/libs/userlibs/rwe/ref/`) and our port with its
  `BUG(orig)` issues (`fubar-coder/bb2-c`, label `bug-orig`, `lib:<name>`).
* **AmiBlitz3:** repository `AmiBlitz/AmiBlitz3` (GitHub; the first 602
  commits up to `AmiBlitz/AmiBlitz3@c95cce17` are the SourceForge SVN,
  revision numbers from the git-svn mirror `amiblitz3-code-git`). The
  RI libraries appear in three forms:
  * inside the BigLib `BlitzLibs/Deflibs`, unchanged from
    `AmiBlitz/AmiBlitz3@a790336c` (SVN r3, 2009-02-11 – already AmiBlitz
    2.x, so it carries all pre-git changes) to
    `AmiBlitz/AmiBlitz3@a6f54bc9` (2021-11-18); then in `utilLibs`,
    `baseLibs` and `csLibs` (from `AmiBlitz/AmiBlitz3@23b6a30d`,
    2022-04-28; HEAD `43fc2a3b`, 2026-10-07, the last BigLib change);
  * as single files `BlitzLibs/UserLibs/userlibs/*.obj`, all added by
    `AmiBlitz/AmiBlitz3@87438cdd` (SVN r279, 2012-02-25), copied to
    `BlitzLibs/Decompiled/thirdpartylibs/` by
    `AmiBlitz/AmiBlitz3@9d854a53` (2021-09-04, byte-identical), later
    `*_new.obj` rebuilds and `_notused/`;
  * sources: AB3's own disassemblies or conversions
    (`Sourcecodes/Amiblitz3/BlitzLibs/Blitzlibs/ri*.ab3`,
    `NeilsCIATrackerLib.ab3`, `others/popupwinlib.ab3`; before 2024 in
    `DEFLIBS/`).
  * `Docs/Changelog.guide` (`AmiBlitz/AmiBlitz3@ea8c25c7`) and
    `Docs/Miscellaneous/Changelog.guide` (HEAD), AB3 GitHub issues.
* **Method:** every binary state taken with `git show`
  (`git log --all --name-only` for the paths, the BigLibs per commit),
  libraries out of the BigLibs with `blitzlib extract`, compared by SHA-256
  against the CD binaries and each other; token lists with `bbsrc tokens`
  (library number = (token − $8000) >> 7); where the bytes differ,
  `od`/`m68k-amigaos-objdump` on the differing words; for AB3 sources
  `git show`/`git diff` of the commit. Matched against our issues by
  their text. The code of large rewrites (rilesdebuglib 2023) was not
  compared instruction by instruction ("not analysed").
* **AB3-built binaries** end their hunk with `7000 4E75` plus 16 × `$01`
  (`docs/rwelibs/README.md` 3.2); the rebuilds from disassemblies
  (`*_new.obj`, `Ripacklib25.obj`, `Rifnslib.obj_new`) are 16–20 bytes
  longer than the originals for that reason alone.

**Classes:** **fix** – of an original bug (our issue if one matches);
**extension** – new commands or behaviour, old programs unaffected;
**incompatible** – changed or removed tokens, library numbers or
behaviour (also when it only follows another AB3 change); **cosmetic** –
comments, source conversion, moves (only counted).

## Common background: the RIAmosFuncLib renumbering

`AmiBlitz/AmiBlitz3@d7225ca7` (2023-11-03, source) and
`AmiBlitz/AmiBlitz3@548dcebe` (2024-03-12, binary) moved `Min`/`Max` out
of RIAmosFuncLib (library 75) into vallib (library 120, tokens
$BC10/$BC11). The entries behind them moved up: every token from
`CopyByte` on is one or two lower (`CopyByte` $A58A → $A589 …
`SaveExec` $A5A5 → $A5A3). Tokenised sources using these commands read
differently (AB3 issue `AmiBlitz/AmiBlitz3#117`, "AB 3.10.1 cannot use RIAmosFuncLib", closed,
is of this kind (?)), and every library that calls RIAmosFuncLib through
ALibJsr (the call encodes the entry number) had to be rebuilt: RIFNSLib,
RIPackLib, RIGfxLib below. Those rebuilds change nothing else.

## Per library

### riamosfunclib (75)

| AB3 commit | Date | Class | What | Our issue |
|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@a790336c` SVN r3 | ≤ 2009-02-11 | fix | `DeviceName$` rewritten: registers kept, task through `dol_Task` → `mp_SigTask`, `UnLockDosList` with the flags of `LockDosList` (13, the original 12), terminating 0 behind the ":" (already in our reference) | – (`#493`, `#494` stay) |
| `AmiBlitz/AmiBlitz3@a790336c` SVN r3 | ≤ 2009-02-11 | cosmetic | help text `(volumename$)` | – |
| `AmiBlitz/AmiBlitz3@87438cdd` SVN r279 | 2009-02-11 … 2012-02-25 | fix | `ChangeDir`: `MOVE oldlock,d1` (word) → `MOVE.L` – the previous lock is really unlocked. Only in the single `.obj` (our reference); the BigLib kept `MOVE.W` until 2024-03-12 | – (`#490`, `#491` stay) |
| `AmiBlitz/AmiBlitz3@37d8fde0` | 2023-12-13 | incompatible | shipped library replaced by an older RWE variant (`MemBank`, `Cache`, `PLoad`, `CludgeShapes`, `CludgeSound`; no `CachesOn`…`SaveExec`, 28 entries); undone 3 months later | – |
| `AmiBlitz/AmiBlitz3@548dcebe` | 2024-03-12 | incompatible | r279's code re-disassembled and rebuilt without `Min`/`Max` (moved to vallib, `AmiBlitz/AmiBlitz3@d7225ca7`): tokens from `CopyByte` on renumbered (see above) | – |
| `AmiBlitz/AmiBlitz3@89ce66e7`, `@0c191809`, `@996139a9` | 2024-03-19 … 2026-05-29 | cosmetic | 3 × source comments | – |

All the bugs of our list (`#483`–`#503`) are still in AB3's code.

### rimisclib (47)

| AB3 commit | Date | Class | What | Our issue |
|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@a790336c` SVN r3 | ≤ 2009-02-11 | extension | `NextArgChar$`, `BInstr` – not AB3's code but an "unreleased rimisclib" of RWE (changelog 2.27b4), the commands of our source `misclib2.bb` (our reference) | – |
| `AmiBlitz/AmiBlitz3@c8b1ed15` | 2025-10-23 | incompatible | rebuilt from a disassembly; `FromBlitz` renamed `FromIDE`, the task name it compares "Blitz ][ Program Process" → "AmiBlitz3 Program Process" | – |
| `AmiBlitz/AmiBlitz3@d7a3a07c` | 2025-10-23 | cosmetic | "init and finit were swapped" – a mistake of the rebuild one commit earlier, not of the original; net no change | – |
| `AmiBlitz/AmiBlitz3@5d668e8e` | 2026-04-16 | extension | `IsEven`, `SearchString` (from NeilsSupportLib), `CtrlC` (from LotanSystem) appended | – |

### rilesdebuglib (49)

| AB3 commit | Date | Class | What | Our issue |
|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@a790336c` SVN r3 | ≤ 2009-02-11 | extension (?) | an AmiBlitz-built binary (11704 bytes in the BigLib, 11632 as r279 `.obj`) with the 31 entries of RWE's older `Debuglib/LESDEBUGlib.bb`, "yet another version" of the code (`docs/rwelibs/port/rilesdebuglib.md`); whose changes is unknown | – |
| `AmiBlitz/AmiBlitz3@b64ab995`, `@84e8174b`, `@7cde5dce`, `@ec1e5c49`, `@c579ae0e` | 2023-08-25 … 12-01 | cosmetic | 5 × comments in the disassembly | – |
| `AmiBlitz/AmiBlitz3@37d8fde0` | 2023-12-13 | incompatible | rebuilt from the disassembly (8248 bytes): `FLocate`, `FPrint`, `FNPrint`, `FInverse`, `FYOffset`, `FPrintDetoke` removed, `DetokenizeLine` added, tokens renumbered; code not analysed | – |
| `AmiBlitz/AmiBlitz3@3c24a4c3` | 2026-10-08 | extension | `OutTextStyle` (font style) appended | – |
| `AmiBlitz/AmiBlitz3@c79fee82` | 2026-10-16 | cosmetic | moved to `baseLibs` | – |

Our reference (`LESDebuglib.bb`, 1996-12-01) has `FindLineToken` and
`GetDataItem`, which no AB3 state has.

### neilsreqtoolslib (54)

| AB3 commit | Date | Class | What | Our issue |
|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@a790336c` SVN r3 | ≤ 2009-02-11 (AmiBlitz 2.27) | fix (?) | AmiBlitz-built patch of the CD's V1.70b binary (1994, not our 1997 source): changelog "nielsrtlib: rtezstringrequest enforcerfix". In the routine before an `rtEZRequestA` call the string passed in d0 is copied into an own 120-byte buffer which the tag points at, instead of the original's store of d0 into the library's code/data; what it fixed exactly is not verified | – (not `#459`: that is RTEZGetString's default string) |

### ridoslib (33 → 58)

| AB3 commit | Date | Class | What | Our issue |
|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@bed4b8fa` | 2026-08-17 | cosmetic | disassembled ("source code recovered by disassembly") | – |
| `AmiBlitz/AmiBlitz3@66d3c4be` | 2026-08-19 | incompatible | rebuilt: code identical to BSS2, library number 33 → 58 (shipped `AmiBlitz/AmiBlitz3@22751b30` 2026-09-16, libnums `AmiBlitz/AmiBlitz3@32533190`); 58 is `#ritrackerlib` in RWE's number list | – |

### rigtmenulib (37)

| AB3 commit | Date | Class | What | Our issue |
|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@9636a803` | 2026-02-13 | cosmetic | disassembled; `riGTMenuLib_new.obj` has BSS2's code unchanged (only the AB3 hunk end); not shipped (the BigLib keeps BSS2's) | – (`#690` stays) |

### ripacklib (73)

| AB3 commit | Date | Class | What | Our issue |
|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@97a26996` | 2024-03-19 | incompatible | shipped library replaced by an older Pack-Library (`DeIce`; no `UnPackPBM`, `IFFDepth`, `IFFWidth`, `IFFHeight`, `IFFViewmode`) | – |
| `AmiBlitz/AmiBlitz3@40ab4662` | 2024-11-11 | incompatible | BSS2 (V2.5) disassembled and rebuilt; only the 6 ALibJsr calls into RIAmosFuncLib renumbered | – |

### rifnslib (93)

| AB3 commit | Date | Class | What | Our issue |
|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@b34d9a85` | 2025-01-24 | incompatible | disassembled and rebuilt; only the 3 ALibJsr calls into RIAmosFuncLib renumbered (AB3 issue `AmiBlitz/AmiBlitz3#107` "riFNSLib does not work anymore") | – |

### rigfxlib (65)

| AB3 commit | Date | Class | What | Our issue |
|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@97a26996` | 2024-03-19 | incompatible | source v1.0 → v2.5 (= BSS2); the `csLibs` build is BSS2 with the ALibJsr to `BSave` renumbered | – |

### ritrackdisklib (91)

| AB3 commit | Date | Class | What | Our issue |
|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@60e25c02` | 2025-04-15 | fix (WIP) | source only, not built (binary still BSS2): init checks exec ≥ 36 and otherwise uses `CreatePort`/`CreateExtIO`/`DeleteExtIO`/`RemPort` – but through `JSR _xxx(a6)` on exec, where those amiga.lib routines are no LVOs (?); AB3 issue `AmiBlitz/AmiBlitz3#109` (open) | `#941` |

### rilvcallhooklib (27)

| AB3 commit | Date | Class | What | Our issue |
|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@e93d1c9e` | 2026-04-19 | incompatible | `RetrieveCallHook` moved into gadtoolslib (library 141, token $C6AE), the library retired (`_notused/`). AB3's binary itself is an older RWE version than our source, not an AB3 change | – |

### rireqlib (79 in AB3)

| AB3 commit | Date | Class | What | Our issue |
|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@5d668e8e` | 2026-04-16 | incompatible | removed ("relies on req.library"); the binary was the CD's `EXTRAS` one, never changed | – |

### ridisasmlib (50)

| AB3 commit | Date | Class | What | Our issue |
|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@a790336c` SVN r3 | ≤ 2009-02-11 | extension | AmiBlitz derivative: `DisAssemble$` tries THOR's `disassembler.library` first and falls back to RWE's code (`docs/rwelibs/port/ridisasmlib.md`); declarations equal | – |

### ciatrklib (56)

| AB3 commit | Date | Class | What | Our issue |
|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@a790336c` SVN r3 | ≤ 2009-02-11 | extension | AB3 ships "CIA Tracker Library v1.055 (07.02.2002)" (`Xbciatrackerlib.obj`, now in `csLibs`), a later Crossbones release, not AB3's code: 41 commands (our 1.02b: 21; BUM10A: 38), `ModuleSampleName$`/`GetSampleName$` with `$`, `AllocatePatternMem`, `GetChannelInfo`, `TrackerPatternToMem`; never changed after | – |
| `AmiBlitz/AmiBlitz3@ad8e70a9` | 2020-07-15 | cosmetic | source `NeilsCIATrackerLib.ab3` added: Neil O'Rourke's V1.3 (the ancestor), not built | – |
| `AmiBlitz/AmiBlitz3@f16939fb` | 2025-11-05 | cosmetic | libjsr constants in that source | – |

AB3 issue `AmiBlitz/AmiBlitz3#3` (2019, "Mungwall hit when freeing a module", closed without
a fix; AB3 recommends ptplayerlib) concerns the 3 bytes added to the
module size – the area of our `#442`, `#434` (?).

### popupwinlib (16)

| AB3 commit | Date | Class | What | Our issue |
|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@0e2ff1db` SVN r2, `@9f5e7b8c` SVN r444 | 2009-02-11, 2014-03-02 | cosmetic | source only (never built or shipped): RWE's `popupwinlib.bb` with the `!SBxx` macros written as plain branches and `#popupwinlib=24` (our 16) | – |

### Libraries AB3 never changed

* **Binary identical to BSS2 in every AB3 state; sources only converted
  or disassembled (cosmetic):** rianimlib, riapplib, riarexxlib,
  ricommoditieslib, ricompactdisclib, ricopperfxlib (`csLibs`),
  ridatatypeslib, ridebuglib, riencryptlib, rievallib, rifastseriallib,
  rifxlib, rinonvolatilelib, rishapeslib, risortlib, risoundlib
  (`csLibs`), ristcdecrunchlib, ristonecrackerlib, ritooltypeslib,
  rizonejoylib. Library numbers unchanged.
* **rilinkedlistlib:** only the CD's older `EXTRAS/RILISTLIB.OBJ` (no
  `SizeOfListItem`), parked in `_notused/` by
  `AmiBlitz/AmiBlitz3@dbc34db6` (2026-04-24); never shipped.
* **Not in AB3 at all:** bb_blitlib, blitzbomberslibtwo, riarrayslib,
  ribbmaplib, richunkylib, ridisplaylib, riintuilib, rimapeditorlib,
  rimapshapeslib, riregistrylib, riseriallib, rishapezlib, risnowlib,
  risoundlib2, rithreadlib, ritrackerlib, riwbspritelib (AB3's
  arrayslib, displaylib, seriallib, trackerlib, chunkylib are Acid's or
  other authors' libraries of those names).

Cosmetic commits over all (not listed above): the bulk conversions and
moves `AmiBlitz/AmiBlitz3@9f5e7b8c` (SVN r444, `.bb2` → `.ab3`),
`@693db8cd`, `@417d4a73`, `@7eaf7048`, `@8c0efae0`, `@8880f56b`,
`@3ea5c208`, `@97a26996` (documentation).

## Summary

| Class | Count | |
|---|---|---|
| fix | 4 | riamosfunclib `DeviceName$` and `ChangeDir` (both already in our reference), neilsreqtoolslib enforcer fix (?), ritrackdisklib Kick 1.x (WIP, not built) |
| extension | 6 | rimisclib ×2, rilesdebuglib ×2, ridisasmlib, ciatrklib v1.055 – of these only rimisclib `IsEven`/`SearchString`/`CtrlC`, rilesdebuglib `OutTextStyle` and the ridisasmlib derivative are AB3's own code |
| incompatible | 11 | riamosfunclib ×2, rimisclib, rilesdebuglib, ridoslib, ripacklib ×2, rifnslib, rigfxlib, rilvcallhooklib, rireqlib |
| cosmetic | 17 (+ 8 bulk conversion/move commits) | comments, disassemblies, rebuilds without code change, moves |

* AB3 fixed hardly any original bug of the RI libraries: apart from the
  two RIAmosFuncLib fixes of the AmiBlitz 2 era (both already in our
  reference) and an unverified ReqTools patch, the only targeted fix is
  the unfinished Kick 1.3 work on RiTrackDiskLib (our `#941`, AB3 issue `AmiBlitz/AmiBlitz3#109`).
  None of our other `bug-orig` issues of these libraries is fixed in any
  AB3 state.
* Most AB3 activity since 2023 is disassembly and reorganisation:
  libraries rebuilt from disassemblies without code change, commands
  moved between libraries (`Min`/`Max`, `RetrieveCallHook`, `CtrlC`,
  `IsEven`, `SearchString`), libraries retired (RIReqLib, RILVCallHook)
  or renumbered (RIDosLib 33 → 58). These are incompatible with RWE's
  token and library numbers and are no candidates for taking over.
* Candidates to look at: the `#941` fix (but AB3's WIP calls amiga.lib
  routines through exec and is not usable as is); the NeilsReqToolsLib
  patch, to be analysed against our 1997 source (it may be a bug of the
  1994 binary only); AB3's own extensions (rimisclib, rilesdebuglib) only
  if we want AB3 compatibility.

## Mapping our issues → AB3

| Our issue | AB3 | State |
|---|---|---|
| `#941` ritrackdisklib V36 calls on Kick 1.x | `AmiBlitz/AmiBlitz3@60e25c02`, AB3 issue `AmiBlitz/AmiBlitz3#109` | WIP source, not built |
| (none) riamosfunclib `ChangeDir` word lock | `AmiBlitz/AmiBlitz3@87438cdd` (r279) | in our reference |
| (none) riamosfunclib `DeviceName$` | `AmiBlitz/AmiBlitz3@a790336c` (r3) | in our reference |
| `#442`, `#434` (?) ciatrklib module size + 3 | AB3 issue `AmiBlitz/AmiBlitz3#3` | closed without a fix |
