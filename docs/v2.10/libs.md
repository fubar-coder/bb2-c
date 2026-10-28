# Libraries: acidlibs and deflibs of the CD

> As of 28.10.2026. Static comparison of binaries, nothing executed.
> Evidence: **[B]** binary comparison (method: [README.md](README.md) §3),
> **[Doku]** text on the CD, **[Schluss]** conclusion, not proven.

## Changelog

| Area | 2.10 release vs the port's reference | Later versions on the CD (not in 2.10) |
|---|---|---|
| acidlibs (83 libraries) | **no change** – identical file | asllib +5 commands (multi-select); windowslib +4 commands (wait on user signals), Window/WindowTags become functions, WScroll colour, WTitle optional; gadgetslib +4 commands (prop gadget control, BOOPSI arrow gadgets), SetTextGadget replaced; gtlib +GTActivateGadget, GTButton UserData, GTGimmeCludge → GTGZZPosition, GA_GadgetHelp in all gadgets; crunch Implode callback; debuglib RedDebugger hand-over (section 4.1, 5) |
| deflibs (39 AmigaOS libraries) | **no change** – identical file | the BUM deflibs drop crm.library and colorwheel.gadget and add third-party libraries (section 2) |
| Library sources (`bblibsrc`, Aminet) vs acidlibs | not a 2.10 change: declarations differ for linklistlib, intuifontlib, blitzcoplib (source newer: SetItem/CountList/CountItem, Text, Help), gtlib (separate development), palettelib (Red/Green/Blue `#long`), dhandlerlib (lib number) – `docs/libs/cmdlibs-decl.md` | – |
| Error messages, help texts | no change | new help texts for the new commands |

## 1 The port's references are the 2.1 release files

The port compares its libraries against `BLITZ2/ACIDLIBS` and
`BLITZ2/DEFLIBS` of the CD (`libs/tests/CMakeLists.txt:50`,
`libs/acidlibs/CMakeLists.txt:67`, ctest `declarations_*`, `asmcmp_*`,
`libs_emulator`; corpus configs in `tests/corpus/CMakeLists.txt:88–89`).

| File | Libraries | SHA-256 | Identical copies |
|---|---|---|---|
| `BLITZ2/ACIDLIBS` | 83 | `98aeae72…340676` | `BUM9A/BLITZ2/ACIDLIBS`, `BUM10B/BLITZ2/ACIDLIBS` |
| `BLITZ2/DEFLIBS` | 39 | `40b1ec56…e2dcf5` | `BUM9A/BLITZ2/DEFLIBS` |

**There is only one acidlibs on the CD** – the one of the Blitz 2.1
release, which BUM9 and BUM10 ship unchanged ("acidlibs (unchanged from
BUM9, grrr)", `BUM10B/BLITZ2/RELEASENOTES`) [B, Doku]. So the port's
acidlibs reference *is* the 2.10 state; nothing to port.

No separate copy of any of the 83 acidlibs members exists on the CD
except `switchlib` (`BSS2/REDDEBUGGER/LIBRARIES/SWITCHLIB.OBJ`, identical)
and the later versions of section 4. The library sources (`bblibsrc` of
the Aminet archive) are a different state for several libraries; that is
documented in `docs/libs/cmdlibs-decl.md` and not repeated here.

## 2 deflibs: four versions, the AmigaOS part identical

| File | Libraries | AmigaOS libraries (lib 2, 200, 219–255) | Third-party libraries |
|---|---|---|---|
| `BLITZ2/DEFLIBS` = `BUM9A/BLITZ2/DEFLIBS` (reference) | 39 | all 39 | – |
| `BUM10B/BLITZ2/DEFLIBS` | 43 | 37, **without** 200 (crm.library) and 219 (colorwheel.gadget) | 6 |
| `BUM9A/BUM9EXAMPLES2/HARDDISKUSERS/DEFLIBS` ("large deflibs", BUM9) | 72 | 37, without 200 and 219 | 35 |
| `EXTRAS/BIGDEFLIBS/DEFLIBS` | 72 | 38, without 200 | 34 |

Every AmigaOS library that is present is **byte-identical** (code and
relocations) to the reference [B]. The separate files
`EXTRAS/BLITZLIBS/AMIGALIBS/*.LIBRARY1` (and the same in
`BUM9B/BLITZLIBS/AMIGALIBS/`, `BUM10A/BLITZLIBS/AMIGALIBS/`), 38 each,
are identical to the reference as well; crm.library (lib 200) exists only
inside `BLITZ2/DEFLIBS`. So the issues of the AmigaOS part (#44, #45) are
the same in every deflibs of the CD.

The third-party libraries (numbers < 200, from `BLITZLIBS:` of the BUM
disks) are not part of the port. List (size:SHA-256 start of code+relocs;
`-` = not contained):

| Lib | First token | BUM10B | HARDDISKUSERS | BIGDEFLIBS |
|---|---|---|---|---|
| 216 | `TEDSetLabelList_` | 768:b9b9b9 | 768:b9b9b9 | - |
| 109 | `CHDir` | 7016:ff1082 | 7016:ff1082 | 3828:caccf0 |
| 103 | `Freq` | 1960:dd4be1 | 1960:dd4be1 | 1960:dd4be1 |
| 56 | `LoadTrackerModule` | 11760:3eda21 | 9852:88d13e | 9356:fa3bb7 |
| 75 | `CachesOn` | 6248:895dc9 | 6248:895dc9 | 6248:895dc9 |
| 23 | `UPeek` | 5484:1f97a5 | - | - |
| 62 | `IconInfo` | - | 1132:835527 | 1132:835527 |
| 101 | `Index` | - | 1116:5091ed | 1116:5091ed |
| 105 | `Xor` | - | 1368:47f153 | 1368:47f153 |
| 107 | `Null` | - | 2228:e66687 | 2228:e66687 |
| 53 | `Lib` | - | 3344:b0176d | 3344:b0176d |
| 55 | `PICreateRequest` | - | 2532:0ea806 | 2532:0ea806 |
| 54 | `RTEZRequest` | - | 6000:372644 | 6000:372644 |
| 67 | `Console` | - | 816:fd430a | 816:fd430a |
| 68 | `IsLocale` | - | 540:67e6de | 540:67e6de |
| 70 | `PrtText` | - | 1092:6e6c93 | 1092:6e6c93 |
| 69 | `EasyRequest` | - | 332:2feef6 | 332:2feef6 |
| 57 | `RIAnimInit` | - | 1576:e4cf90 | 1576:e4cf90 |
| 61 | `AddAppWindow` | - | 2808:efb237 | 2808:efb237 |
| 89 | `MakeCommodity` | - | 2488:52b94a | 2488:52b94a |
| 14 | `OpenCD` | - | 2768:d2b855 | 2668:f394a3 |
| 11 | `DoColSplit` | - | 1664:002b25 | 1664:002b25 |
| 52 | `Encrypt` | - | 680:8e8749 | 680:8e8749 |
| 93 | `FNSPrint` | - | 5276:d78511 | 5276:d78511 |
| 66 | `ClearBitmap` | - | 4516:9acbb3 | 4516:9acbb3 |
| 65 | `PalRed` | - | 1872:bf679d | 1872:bf679d |
| 49 | `D_FindVolume` | - | 8136:2fecb3 | - |
| 73 | `PackPBM` | - | 2564:98653f | 2388:5d3a40 |
| 46 | `CludgeShapes` | - | 4200:7a1317 | - |
| 13 | `StringSort` | - | 740:7b8bcf | 740:7b8bcf |
| 59 | `GetIconObject` | - | 2960:717ce0 | 2960:717ce0 |
| 91 | `OpenDisk` | - | 2820:bf2779 | 2820:bf2779 |
| 64 | `ZoneInit` | - | 3396:81fb77 | 3396:81fb77 |
| 10 | `Database` | - | 6136:fa1146 | - |
| 4 | `XBDiskPlay` | - | 2784:9902d3 | - |
| 72 | `Chunky` | - | 46688:847b6b | - |
| 74 | `Implode` | - | - | 4892:1f6481 |
| 111 | `IncSound` | - | - | 4536:d5a5f5 |
| 79 | `ReqFileRequest` | - | - | 964:0f9f15 |
| 12 | `InitList` | - | - | 2036:dfcaf4 |
| 51 | `AddVarTrace` | - | - | 1380:e4902c |

Lib 74 in `EXTRAS/BIGDEFLIBS/DEFLIBS` is Romulus' crunch library (the same
as `EXTRAS/BLITZLIBS/OTHERLIBS/ROMULUSCRUNCHLIB.OBJ`, section 4) and has
the **same number as acidlibs' crunchlib**. blitz2 loads acidlibs first
and `findlib` takes the first match (`docs/architecture.md` §4.7), so the
deflibs copy would be shadowed for compiling while both token tables are
linked [Schluss, not checked in the emulator].

## 3 All library files on the CD

All hunk files of the CD with one code hunk that starts with a library
header (`MOVEQ #0,d0 : RTS` or 4 zero bytes) were matched by library number
against acidlibs and deflibs [B]: 289 files, of which

* 115 identical to a library of the reference (114 AmigaOS libraries in
  the three `BLITZLIBS/AMIGALIBS` directories, `switchlib`),
* 9 with the number of a reference library but different contents
  (section 4; one of them, `BUM9B/BLITZLIBS/LIBNUMS.RES`, is a resident
  file that only looks like a library – no library),
* 165 with numbers that are in neither reference: third-party libraries
  (`EXTRAS/BLITZLIBS/OTHERLIBS`, `…/RILIBS`, `BSS2/REDBLITZLIBS/LIBRARIES/USERLIBS`,
  `BUM*/BLITZLIBS/TESTLIBS`, `BUM10B/BLITZ2/USERLIBS/BETA` …) and a few
  false positives (font files, devices). Not part of the port.

## 4 Later versions and number clashes

| Lib | Port name | CD reference (code bytes) | Other file | Code bytes | Kind |
|---|---|---|---|---|---|
| 80 | `asllib` | 1928 | `BSS2/REDBLITZLIBS/LIBRARIES/BASIC/BBASLLIB.OBJ` | 2616 | later version (BUM10 "fixed Acidlibs") |
| 141 | `gtlib` | 7592 | `BSS2/REDBLITZLIBS/LIBRARIES/BASIC/BBGTLIB.OBJ` | 8024 | later version (BUM10 "fixed Acidlibs") |
| 140 | `gadgetslib` | 4852 | `BSS2/REDBLITZLIBS/LIBRARIES/BASIC/GADGETSLIB.OBJ` | 6276 | later version (BUM10 "fixed Acidlibs") |
| 138 | `windowslib` | 10356 | `BSS2/REDBLITZLIBS/LIBRARIES/BASIC/WINDOWSLIB.OBJ` | 10968 | later version (BUM10 "fixed Acidlibs") |
| 64060 | `debuglib` | 3372 | `BSS2/REDDEBUGGER/LIBRARIES/DEBUGLIB.OBJ` | 3424 | later version (RedDebugger, BSS2) |
| 74 | `crunchlib` | 4900 | `EXTRAS/BLITZLIBS/OTHERLIBS/ROMULUSCRUNCHLIB.OBJ` (also in `EXTRAS/BIGDEFLIBS/DEFLIBS`) | 4892 | later version by a third party (same tokens, extended) |
| 121 | `polylib` | 3796 | `BSS2/REDBLITZLIBS/LIBRARIES/USERLIBS/RIEVALLIB.OBJ` | 2576 | **number clash**: a different library (ConvToPostFix, PFEvaluate …) |
| 219 | (deflibs) colorwheel.gadget | 252 | `INTERNET/TRITONBLITZ/TRITONBLITZ/BLITZLIBS/AMIGALIBS/TRITON.LIBRARY1` | 1284 | **number clash**: triton.library |

`BUM10B/BUM10EXAMPLES1/BUM10DOCS/BUM10_README` lines 51–56 [Doku]:
"Hmmm.. well Steve Matty sent me four fixed Acidlibs, but since I lost
everything on my HD I can't create a new Acidlibs.. doh (in case you
wanted to know, the fixed acidlibs are bbaslib.obj, bbgtlib.obj,
gadgetslib.obj and windowslib.obj)". These four files are exactly the
four in `BSS2/REDBLITZLIBS/LIBRARIES/BASIC/` [Schluss: same names, same
library numbers, later than the release; the BSS2 copies may be a later
state than the files Ted Bailey received]. **They were never part of an
acidlibs file, so not part of 2.10.**

Instruction similarity with the reference (objdump, numbers masked,
`difflib`) [B]: asllib 97 %, windowslib 95 %, gadgetslib 95 %, gtlib 95 %,
debuglib 98 %, crunch 98 % of the reference's instructions found again in
the same order. They are revisions of the same code, not rewrites.

### 4.1 Token changes (token table, by name and help text) [B]

* **asllib (80)**: 6 → 11 entries; new `ASLMultiSelect` (Title$,Pathname$,Filename$[,x,y,w,h]),
  `ASLGetFile` (argnum), `ASLNextFile`, `ASLFreeRequest`, `ASLNumFiles`.
* **windowslib (138)**: 75 → 79 entries; `Window` and `WindowTags` change
  their entry kind from `$0001` to `$0303`; `WScroll` gets an optional
  colour, `WTitle` an optional screen title (second form); `WLine` is
  renamed `Wline`; new `AddWaitEvent` (bitnumber,returncode.l),
  `DelWaitEvent` (bitnumber), `EventiAddress`, `WaitSigBits`.
* **gadgetslib (140)**: 39 → 43 entries; `PropGadget` with optional
  `propflags,propactivation[,typemask]`, `Redraw Window#[,Id]`;
  `SetTextGadget` is replaced at its token position by `SetPropInfo`
  (so old sources using `SetTextGadget` would show `SetPropInfo`);
  new `MoveProp`, `GetPropCurrent`, `ArrowGadget`, one more nullsub.
* **gtlib (141)**: 40 → 41 entries; `GTButton` with optional `UserData`;
  `GTGimmeCludge` renamed `GTGZZPosition` (same position); new
  `GTActivateGadget`.
* **debuglib (64060)**: same 12 entries.
* **crunch (74)**: same 4 entries; `Implode` gets an optional `?Callback`;
  `CrMDecrunch` already has two forms in the CD version, only its help text
  gains `[,Destinationadr]` and the forms call different routines.

## 5 Bug-orig issues in the later library versions

**In the 2.10 release all library issues are unchanged**: its acidlibs
and deflibs are the port's references themselves. This section shows
which issues the later, never-released versions of section 4 fix, in
case they are taken over one day. Each verdict was reached by locating
the faulty code in the CD binary and finding the matching code in the
later version through the instruction alignment. Offsets are code-hunk
offsets, CD → later version. Spot checks were repeated; nothing was run.

### 5.1 windowslib (138) – `BSS2/REDBLITZLIBS/LIBRARIES/BASIC/WINDOWSLIB.OBJ`

Error check area: `$2618` (CD) / `$287A` (later version).

| Issue | Short title | Verdict | Evidence |
|---|---|---|---|
| #310 | Menus/MenusOn/Off step 16 over 32-byte objects | unchanged | `$14DC` → `$1666` `lea 16(a2),a2; dbf d2`; `!libfin` still 32 bytes (`$1168`/`$1284`) |
| #311 | String gadget: extra GADGETUP | unchanged | `$1AFC–$1B58` → `$1D62–$1DBE`, same code |
| #312 | MENUPICK node with uninitialised qualifier/IAddress | unchanged | `$1BE0` → `$1E46`; now also visible through the new `EventiAddress` |
| #313 | Qualifier sets only the low word | unchanged | `$161A` → `$17A4` `move.w kqualifier(pc),d0; and.w #255,d0` |
| #314 | WCls clears from y=1 | unchanged | `$1700` → `$188A` |
| #315 | WScroll/WPrintScroll set the A pen to 0 | changed, still present | WPrintScroll `$1438` → `$15A2` unchanged. The new 7-argument WScroll (`$1906`) only sets the B-pen variable and then calls the old code (A pen still `moveq #0,d7`). The RastPort B pen stays at the colour afterwards. |
| #316 | WindowTags without tags reuses old usertags | unchanged (worse) | `$1E96` → `$20FA`. After a `&TagList` call, usertags holds `TAG_MORE,<old list>`, so the next call can follow a dangling pointer. |
| #317 | `&TagList` copied unbounded into 32 longs | **fixed** | The copy loop `$1E58–$1E70` is replaced at `$20C6` by `move.l #2,usertags; move.l d3,usertags+4` (TAG_MORE chains the user list) |
| #318 | Window titles allocated, never freed | partly fixed | WTitle (`$1444` → `$15B0`) now frees the old window title (maxslib `$CF02`). Screen titles, CloseWindow, Window and WindowTags still leak. |
| #319 | Flag `$80` uses the bitmap of an earlier Window | unchanged | `$1DCE` → `$202C` |
| #320 | WMove/WSize wait forever | unchanged | `$1474` → `$15FE`, `$149A` → `$1624` |
| #321 | calcwidth signed test | unchanged | `$246C` → `$26D2` `sub.b 32(a4),d0; bmi.w` |
| #322 | CloseWindow discards all windows' events | unchanged | `$2008–$2060` → `$226E–$22C6` |
| #323 | err6/err7 jump into the error check area | unchanged | e.g. `$206A` → `$2638` becomes `$22D0` → `$289A` |
| #329 | `&TagList` writes the title length behind TAG_DONE | **fixed** | the same new code as #317; a2 is no longer touched |

**New bug in the later windowslib:** if `WindowTags` cannot open the
window, the program crashes instead of reporting an error. err1 (CD
`$2610`, runtime error) became `moveq #0,d0; rts` (`$2876`).
`Window` handles this correctly: at `$20C0` it removes 6 bytes from the
stack (`addq.l #6,sp`) first. `WindowTags` jumps from `$2142` straight
to `$2876` with 6 bytes still on the stack, so its `rts` returns to a
garbage address [B, not run].

Other changes:

* `Window` and `WindowTags` now return the window pointer, and 0 on
  failure.
* WaitEvent waits with `Wait(sigmask)` instead of `WaitPort` (`$1C72`).
  Signal bits registered with `AddWaitEvent` return their code;
  `WaitSigBits` returns the mask.

### 5.2 gadgetslib (140) – `BSS2/REDBLITZLIBS/LIBRARIES/BASIC/GADGETSLIB.OBJ`

Error check area: `$1196` (CD) / `$1726` (later version).

| Issue | Short title | Verdict | Evidence |
|---|---|---|---|
| #163 | freegadget never frees StringInfo/PropInfo | unchanged | `$1130` → `$16B0` (PropInfo now 28 bytes, type still overwritten) |
| #164 | StringGadget with Borders Off writes to 0/2/36/38 | unchanged | allocborder `$1030` → `$15A6`, StringGadget `$DAA` → `$12C0` |
| #165 | StringGadget with Maxlen 0 clears 64 KB | unchanged | `$DFC` → `$1312` `subq.w #1,d5; clr.b (a0)+; dbf d5` |
| #166 | findgadget without end-of-list test | unchanged | `$C7C` → `$11CE` |
| #167 | Shape gadget with depth 0 gives PlanePick `$FF` | unchanged | `$EA8` → `$141E` |

Other changes:

* Init opens intuition.library, and a new finit frees it again.
* `ArrowGadget` uses BOOPSI (`buttongclass`/`sysiclass`).
* freegadget disposes gadgets marked `AR` (`$4152`).
* `Redraw` without an Id refreshes all gadgets.
* The code of `SetTextGadget` is gone.

### 5.3 gtlib (141) – `BBGTLIB.OBJ`

| Issue | Short title | Verdict | Evidence |
|---|---|---|---|
| #168 | GTPalette colour goes into the TAG_MORE pointer | changed, still present | `$175C` → `$18B4` still `move.w -18(a2),list+38`. The new tag `GA_GadgetHelp` (`$80030025`) is at +32, so +38 is now its value: TAG_MORE is no longer corrupted, but the colour is still ignored and GadgetHelp gets switched on. |
| #169 | Optional values stay in the static tag lists | fixed only for GTString | Cycle/MX/ListView/Scroller/Slider unchanged. GTString clears the value after CreateGadget (`$1B7E clr.l $1BA2`). |
| #170 | GTBevelBox never recessed | unchanged | `$DF8` → `$EAA` |
| #171 | Toggle GTButton writes to address 14 on failure | unchanged | `$12E2` → `$13E8`, no null test |
| #172 | GTShape takes over the previous label | unchanged | `$113A` → `$11F6` |
| #173 | GTShape overwrites GadTools' image; static images shared | partly fixed | `$1ADC` → `$1C62`. The two-shape form now allocates a 20-byte copy per gadget (freed in freecontext/GTFreeGadget). GadgetRender is still overwritten. The fallback for GadgetRender = 0 is gone, so it now writes to addresses 0–19. |
| #174 | Cycle/MX labels ≤ 63, block never freed | unchanged | `$14EC` → `$1624` |
| #175 | Last vs first gadget of an Id | unchanged | `$E40` → `$EF2` |
| #176 | Border sizes without a used screen | unchanged | `$10DA` → `$118C` |
| #327 | GTShape does not check CreateGadget | changed, still present | `$1C62`: the test `tst.l 18(a0)` is gone too, so the failure case is worse |
| #328 | a3 left on the static tag list | unchanged | six places, e.g. `$EC2` → `$F74` |

New problems in the later gtlib [B]:

* UserData comes from one global (init `$ABABABAB`) that only
  GTButton/GTCheckBox set, so other gadget kinds inherit stale UserData.
* GTFreeGadget lost its debugger check "GTList must be detached from
  window first!".
* freecontext frees SelectRender of every gadget whose SelectRender differs
  from GadgetRender [not verified for GadTools' own kinds].

Fixed: GTFreeGadget now updates the list's `previous` pointer when the
freed run ends the list (`$E5A`).

### 5.4 asllib (80) – `BBASLLIB.OBJ`

All six issues are **unchanged**:

| Issue | Evidence |
|---|---|
| #60 | `$442` → `$664` |
| #61 | `$630` → `$8C4` |
| #62 | `$59A` → `$814` |
| #63 | `$CF01` at `$754`/`$7C2`/`$848`, no free |
| #64 | `$4C2` → `$73C` |
| #65 | `$2FC` → `$526`, buffer still 96 bytes |

The new multi-select code has several new bugs [B]:

* After one `ASLMultiSelect`, every later `ASLFileRequest$`/
  `ASLPathRequest$` is multi-select and returns `fr_NumArgs` as the
  string length. The flag at `$992` is never reset; the "keep" flag at
  `$40E` is cleared only by `ASLFreeRequest`.
* `ASLFreeRequest` writes 0 over the *tag* `ASLFR_Flags2` (`$434`
  `move.l #0,$986`). This ends the tag list there, so `ASLPathRequest$`
  loses DrawersOnly.
* Cancelling an `ASLMultiSelect` leads to a double free.
* `ASLScreenRequest`/`ASLFontRequest` share the slot of a kept
  requester and overwrite it.
* `ASLGetFile` tests the NUL instead of the last character (`$486`), so
  it always appends `/` ("SYS:/file").

### 5.5 crunch (74) – `ROMULUSCRUNCHLIB.OBJ`

| Issue | Verdict | Evidence |
|---|---|---|
| #112, #114 | unchanged | the Implode/Deplode core `$180–$A08` is byte-identical |
| #115 | changed, still present | the copy loop `$A54` → `$A4E` is unchanged and is now reached only by the one-argument form |
| #116 | changed, still present (moderately sure) | the one-argument form now decrunches to buffer+14 (`$A0A lea 14(a0),a1`). It still overwrites MinSecDist−14 bytes in front of the buffer, and the data now starts at Bufferadr+14. |
| #117, #118, #119 | unchanged | the decrunchers are instruction-identical |

The two-argument `CrMDecrunch` no longer handles overlapping buffers.
`Implode` calls a user callback (d2) instead of flashing the colour and
testing the mouse.

### 5.6 debuglib (64060) – RedDebugger `DEBUGLIB.OBJ`

#126 (`$A9C` → `$A94`) and #127 (`$CC4` → `$CF6`) are **unchanged**.

New: init does `FindTask(0)` and `AllocSignal(-1)` (unchecked); finit
does `FreeSignal`, with a6 not loaded. The hand-over to the debugger
passes the task and the signal. Token table unchanged.

### 5.7 Summary

| Library | Issues | Fixed | Partly / changed, still present | Unchanged |
|---|---|---|---|---|
| windowslib | 15 | #317, #329 | #315, #318 (partly) | 11 (#316 slightly worse) |
| gadgetslib | 5 | – | – | 5 |
| gtlib | 11 | – | #168, #169 (GTString only), #173 (partly), #327 (worse) | 7 |
| asllib | 6 | – | – | 6 |
| crunchlib | 7 | – | #115, #116 | 5 |
| debuglib | 2 | – | – | 2 |

All other library issues concern libraries that exist on the CD in only
one version, so they are unchanged everywhere. The later versions also
bring new bugs (windowslib WindowTags crash, asllib multi-select, gtlib
UserData). As a replacement for the 2.1 libraries they are not clearly
better [Schluss].
