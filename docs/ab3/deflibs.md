# AmiBlitz3 changes to the deflibs (relative to Blitz Basic 2.10)

What AmiBlitz 2.x/3 changed in the AmigaOS call libraries of Blitz Basic 2
(`blitz2:deflibs`; our port: `libs/deflibs/*.c`, notes
`docs/libs/deflibs.md`), as the basis for deciding which changes we take
over. Documentation only; nothing here is taken over yet.

## Sources and method

* **Baseline:** the CD's `BLITZ2/DEFLIBS` (39 libraries). Our build
  `build/amiga/blitz2/deflibs` has the same declarations (ctest
  `declarations_deflibs`), so "BB2" below means both.
* **AmiBlitz3:** repository `AmiBlitz/AmiBlitz3` (GitHub; the first 602
  commits up to `AmiBlitz/AmiBlitz3@c95cce17` are the SourceForge SVN).
  * `BlitzLibs/Deflibs` (the AmiBlitz BigLib holding *all* libraries,
    command libraries and OS call libraries), 12 readable states from
    `AmiBlitz/AmiBlitz3@ea8c25c7` (SVN r1, 2009-02-10, already AmiBlitz
    2.x, so it carries all pre-git changes) to
    `AmiBlitz/AmiBlitz3@d3a79260` (2021-09-21).
    `AmiBlitz/AmiBlitz3@1db85b0c` is corrupt (load file broken; removed
    by `AmiBlitz/AmiBlitz3@034e2bd0`, restored by
    `AmiBlitz/AmiBlitz3@8e7928a5`).
  * `BlitzLibs/OsLibs` (from `AmiBlitz/AmiBlitz3@513e8d72`, 2021-11-08:
    the OS call libraries moved out of `Deflibs` into their own BigLib),
    12 states up to `AmiBlitz/AmiBlitz3@43fc2a3b` (2026-10-07, the last
    change; repository HEAD `b1a9dee1`, 2026-10-25).
  * The generator of the OS call libraries:
    `Sourcecodes/Amiblitz3/Tools/AB3FDConvert.ab3` (an `fdconvert.bb2`
    descendant), and `Docs/Miscellaneous/Changelog.guide`.
* **Method:** every state extracted with `git show`; AB3's BigLib files
  lack the end mark (long 0) that our `biglib`/`blitzlibcompare` want,
  so a throw-away script listed for every OS call library (header starts
  with 4 zero bytes) the entries (name, help text, base library, LVO,
  registers) and the init/finit code. Libraries are matched by their
  AmigaOS name, not by number (AB3 renumbered many). The registers were
  also checked against the NDK 3.2 `.fd` files of the toolchain
  (`/opt/m68k-amigaos/m68k-amigaos/ndk/lib/fd`).
* Not covered: the command libraries in `Deflibs` (acidlibs/userlibs
  material), the compiler side (how AB3 handles a base of 0), and AB3's
  third-party OS libraries in detail.

Classes: **fix** (of a BB2 bug), **extension** (new library or call),
**incompatible** (removed/moved call, changed token number = library
number/index, changed library number, changed behaviour), **cosmetic**
(help text only). "(?)" marks an uncertain point.

## Library init and finit (all libraries)

BB2's init loops forever if `OpenLibrary`/`OpenResource` fails (#44), and
the resource inits pass a name shifted by 6 bytes (#45; see
`docs/libs/deflibs.md`).

AB3's generator (`AB3FDConvert`, already in the 2009 state) writes:

```
init:  MOVE.l  4.w,a6
       LEA     libname(pc),a1      ; resources: displacement $001D
       MOVEQ   #0,d0
       JSR     OpenLibrary(a6)     ; resources: OpenResource
       RTS                         ; no TST/BEQ loop
finit: MOVE.l  a1,d0               ; libraries; resources: RTS
       TST.l   d0
       BEQ     skip
       MOVEA.l 4,a6
       JSR     CloseLibrary(a6)
skip:  RTS
```

| Library | Change | Class | Our issue | AB3 |
|---|---|---|---|---|
| the other 25 OpenLibrary libraries of BB2 that AB3 kept | init returns the base or 0 instead of looping; finit skips `CloseLibrary` for base 0 | fix (behaviour: a missing library no longer hangs at startup; the program now runs with base 0 and crashes at the first call unless it checks (?)) | #44 | `AmiBlitz/AmiBlitz3@ea8c25c7` (SVN r1, pre-git) |
| datatypes.library (239) | same, the 2009 state still had the BB2 code | fix | #44 | `AmiBlitz/AmiBlitz3@5153d097` (2021-09-03) |
| battclock, battmem, card, disk, misc, potgo resources | loop removed, but the LEA displacement is the fixed `$001D` of `AB3FDConvert` ("Right, now create Resource fixed"): it points 21 bytes behind the start of the name (behind its end or to an odd address in the padding), so `OpenResource` gets a wrong/empty name and always fails | partial fix of #44 (no hang); #45 **not** fixed (base is always 0) | #44, #45 | `AmiBlitz/AmiBlitz3@ea8c25c7` (pre-git), unchanged to HEAD |
| ciaa, ciab resources | unchanged BB2 code: loop and name `"esource"` | not fixed | #44, #45 | – |
| dos 249, graphics 250 (no init) | unchanged | – | – | – |

The correct displacement for the resource template would be `$0008`
(name right behind `RTS` of init and finit); a fix of #45 for us is not
in AB3.

## exec.library (253, `execlib.c`)

| Change | Class | Our issue | AB3 |
|---|---|---|---|
| `CachePreDMA_`, `CachePostDMA_`: `flags` in **d0** instead of d1 (BB2 passes it in d1; `exec_lib.fd`: `(a0/a1,d0)`) | fix | none (new finding, no `BUG(orig)` yet) | `AmiBlitz/AmiBlitz3@ea8c25c7` (pre-git) |
| `ObtainQuickVector_` (-786) added | extension | – | `AmiBlitz/AmiBlitz3@ea8c25c7` |
| `NewMinList_` (-828, V45) added | extension | – | `AmiBlitz/AmiBlitz3@5153d097` |
| `ChildFree_`, `ChildOrphan_`, `ChildStatus_`, `ChildWait_` (-738…-756, never implemented) removed; tokens from index 109 shift | incompatible | – | `AmiBlitz/AmiBlitz3@5153d097` |
| `FindSemaphore_` help `(sigSem)` → `(name)` (BB2's help text is wrong) | cosmetic | – | `AmiBlitz/AmiBlitz3@5153d097` |

## graphics.library (254 and 250, `graphicslib.c`, `graphicslib2.c`)

| Change | Class | AB3 |
|---|---|---|
| `GfxInternal1_`, `GfxInternal2_` (private) removed from 254; `EraseRect_`, `ExtendFont_` moved from 250 to the end of 254 | incompatible (token numbers of 254 from index 102 and of all of 250 change) | `AmiBlitz/AmiBlitz3@ea8c25c7` (pre-git) |
| `GfxInternal3_`, `GfxSpare1_`, `GfxSpare2_` removed from 250 | incompatible | `AmiBlitz/AmiBlitz3@ea8c25c7` |
| `WriteChunkyPixels_` (V40) added to 250 | extension | `AmiBlitz/AmiBlitz3@ea8c25c7` |

## Other libraries of BB2 with changes

| Library (number, our file) | Change | Class | AB3 |
|---|---|---|---|
| amigaguide (248, `amigaguidelib.c`) | `AGARexxHost_` (private) removed, all tokens shift by one | incompatible | `AmiBlitz/AmiBlitz3@ea8c25c7` |
| amigaguide | `LoadXRef_`, `ExpungeXRef_` added | extension | `AmiBlitz/AmiBlitz3@ea8c25c7` |
| icon (235, `iconlib.c`) | `GetIcon_`, `PutIcon_` (obsolete V33 calls, not in the 3.x `.fd`) removed, tokens shift by two | incompatible | `AmiBlitz/AmiBlitz3@ea8c25c7` |
| icon | 9 V44 calls (`DupDiskObjectA_` … `ChangeToSelectedIconColor_`) | extension | `AmiBlitz/AmiBlitz3@ea8c25c7` |
| workbench (220, `wblib.c`) | 7 V44 calls (`OpenWorkbenchObjectA_` … `MakeWorkbenchObjectVisibleA_`) | extension | `AmiBlitz/AmiBlitz3@ea8c25c7` |
| workbench | `UpdateWorkbench_` (-30, inserted at index 0, so all tokens shift) and `WhichWorkbenchObjectA_` | extension + incompatible (token shift) | `AmiBlitz/AmiBlitz3@5153d097` |
| datatypes (239, `datatypeslib.c`) | `RLDispatch_` (private) removed, tokens shift by one | incompatible | `AmiBlitz/AmiBlitz3@5153d097` |
| datatypes | 8 V44/V45 calls (`FindMethod_` … `StartDragSelect_`) | extension | `AmiBlitz/AmiBlitz3@5153d097` |
| diskfont (251, `diskfontlib.c`) | 12 V45+ calls (`GetDiskFontCtrl_` … `ObtainCharsetInfo_`) | extension | `AmiBlitz/AmiBlitz3@5153d097` |
| intuition (255, `intuitionlib.c`) | `ShowWindow_`, `HideWindow_`, `IntuitionControlA_` (V47) | extension | `AmiBlitz/AmiBlitz3@5153d097` |
| layers (232, `layerslib.c`) | `LayerOccluded_`, `HideLayer_`, `ShowLayer_`, `SetLayerInfoBounds_` (V45+) | extension | `AmiBlitz/AmiBlitz3@5153d097` |
| rexxsyslib (222, `rexxsyslib.c`) | 7 V45+ calls (`CreateRexxHostPort_` … `GetRexxBufferFromMsg_`) | extension | `AmiBlitz/AmiBlitz3@5153d097` |
| utility (221, `utilitylib.c`) | `VSNPrintf_`, `Strncpy_`, `Strncat_`, `SDivMod64_`, `UDivMod64_` (V47) | extension | `AmiBlitz/AmiBlitz3@5153d097` |
| gadtools (236, `gadtoolslib.c`) | `SetDesignFontA_`, `ScaleGadgetRectA_` (V47) inserted before `GT_GetGadgetAttrsA_`, whose token shifts | extension + incompatible (token shift) | `AmiBlitz/AmiBlitz3@5153d097` dropped gadtools from `Deflibs` by mistake, `AmiBlitz/AmiBlitz3@d3a79260` ("added missing gadtools.library") brought it back with the new calls |
| dos (252, `doslib.c`) | help of `ReadArgs_`/`FindArg_`: `template` → `arg_template` | cosmetic | `AmiBlitz/AmiBlitz3@ea8c25c7` |
| locale (231, `localelib.c`) | help of `FormatDate_`, `FormatString_`, `ParseDate_`: `fmtTemplate` | cosmetic | `AmiBlitz/AmiBlitz3@ea8c25c7` |
| utility | help of 5 calls (`packTable`, `nameSpace`, `tagList`, `lastObject`) | cosmetic | `AmiBlitz/AmiBlitz3@ea8c25c7` |

Declarations unchanged to HEAD (apart from the init code above): asl
247, bullet 244, commodities 240, expansion 237, iffparse 234, keymap
233, mathffp 230, mathieeedoubbas 229, mathieeedoubtrans 228,
mathieeesingbas 227, mathieeesingtrans 226, mathtrans 225, dos 249, and
the resources battclock 246, battmem 245, card 243, ciaa 242, ciab 241,
disk 238, misc 224, potgo 223. dos 252 has no new calls (the 3.2 update
did not touch dos.library, changelog V3.8.1). Apart from
`CachePreDMA_`/`CachePostDMA_`, all registers and LVOs of BB2 match the
NDK 3.2 `.fd` files; AB3 HEAD has no mismatch either.

## Libraries of BB2 removed by AB3

| Library (BB2 number) | AB3 | Class |
|---|---|---|
| CrM.library (200, `crmlib.c`) | gone in the 2009 state; number 200 reused for `wbstart.library` (1 call), from `AmiBlitz/AmiBlitz3@dbc34db6` (2026-04-24) for `render.library` | incompatible |
| gadgets/colorwheel.gadget (219, `colorwheellib.c`) | gone in the 2009 state; a rebuilt version (name `sys:classes/gadgets/colorwheel.gadget`) only lies unused in `BlitzLibs/Decompiled/oslibs/` (`AmiBlitz/AmiBlitz3@a6f54bc9`); number 219 used for `triton.library` since `AmiBlitz/AmiBlitz3@d4ed2485` (2024-02-22) | incompatible |
| xprzmodem.library (2, `xprzmodemlib.c`) | gone in the 2009 state | incompatible |

## New OS call libraries in AB3

Extensions, all with the AB3 init template (no loop). Numbers are the
HEAD ones; many were renumbered by `AmiBlitz/AmiBlitz3@dbc34db6`
("reordered … to get some free numbers between amigalibs and blitzlibs",
2026-04-24), `bsdsocket` 26 → 212 by `AmiBlitz/AmiBlitz3@4592c2d5`,
`camd` 5 → 205 and `mpega` 6 → 206 by `AmiBlitz/AmiBlitz3@bd537dff`.

| Library (HEAD number, calls) | First in |
|---|---|
| FileID 213 (6), asyncio 193 (13), bfbplaymaster 204 (15), bsdsocket 212 (46), camd 205 (34), cybergraphics 195 (22), disassembler 211 (2), identify 203 (8), mpega 206 (7), muimaster 201 (25), newicon 208 (6), openurl 197 (12), preferences 210 (10), render 200 (46), reqtools 215 (21), ted 216 (13), thxplay 209 (17), translator 214 (1), ttengine 190 (16), wizard 42 (30), xpkmaster 192 (14), zlib 191 (20), wbstart (200, removed again in `AmiBlitz/AmiBlitz3@dbc34db6`) | `AmiBlitz/AmiBlitz3@ea8c25c7` (SVN r1, pre-git) |
| guigfx 202 (27), jpeg 194 (10) | `AmiBlitz/AmiBlitz3@a790336c` (SVN r3) |
| muimaster: 4 V3.9 calls added and removed again | `AmiBlitz/AmiBlitz3@f53adb11` (SVN r503), reverted by `AmiBlitz/AmiBlitz3@34f55d11` (SVN r550) |
| hvlplay 207 (7) | `AmiBlitz/AmiBlitz3@a6f54bc9` |
| triton 219 (35) | `AmiBlitz/AmiBlitz3@d4ed2485` |
| bsdsocket: 55 new calls, all help texts renamed | `AmiBlitz/AmiBlitz3@91440d7e` |
| Warp3D 198 (84) | `AmiBlitz/AmiBlitz3@fe8a299e` |
| sndfile 199 (28) | `AmiBlitz/AmiBlitz3@dbc34db6` |
| gadgets/chooser 30 (7) | `AmiBlitz/AmiBlitz3@3b51db9a` |
| c2p 196 (16), gadgets/clicktab 29 (5), gadgets/listbrowser 31 (16), gadgets/radiobutton 28 (5), gadgets/textfield 27 (2) | `AmiBlitz/AmiBlitz3@43fc2a3b` |

## Summary

Counted per row of the tables above (a row with two classes counts in
both):

| Class | Count | Items |
|---|---|---|
| fix | 4 | init without loop for libraries (#44, pre-git, datatypes in 2021); the same for 6 resources (partial: #44 only); `CachePreDMA_`/`CachePostDMA_` flags register (#959) |
| extension | 14 changes to BB2 libraries + 35 new libraries at HEAD (36 with `wbstart`) | V40–V47 calls in exec, graphics, amigaguide, icon, workbench (2), datatypes, diskfont, intuition, layers, rexxsyslib, utility, gadtools |
| incompatible | 12 | removed calls (exec `Child*`, graphics `GfxInternal*`/`GfxSpare*`, icon `GetIcon_`/`PutIcon_`, amigaguide `AGARexxHost_`, datatypes `RLDispatch_`), token shifts (also workbench, gadtools, graphics `EraseRect_`/`ExtendFont_` moved), 3 libraries removed with numbers reused |
| cosmetic | 4 | help texts in dos, locale, utility, exec `FindSemaphore_` |

Mapping of our issues:

* **#44** (init loops forever): fixed in AB3 for every library
  (`AmiBlitz/AmiBlitz3@ea8c25c7`, datatypes
  `AmiBlitz/AmiBlitz3@5153d097`) and for 6 of the 8 resources; ciaa/ciab
  unchanged. AB3's fix replaces the hang by base 0; what a program does
  then depends on the compiler (?).
* **#45** (resource name shifted): **not fixed** in AB3; the 6 rebuilt
  resources point further off (`$001D`), ciaa/ciab keep the BB2 code.
* **#959**: **`CachePreDMA_`/`CachePostDMA_` pass `flags` in d1**
  instead of d0 (`libs/deflibs/execlib.c`), fixed in AB3 before 2009.

Candidates to take over, once fixing original bugs is allowed: the
library init/finit without loop (#44), the `CachePreDMA_`/`CachePostDMA_`
registers, and a correct resource name (#45, our own fix: AB3 has none).
The new calls are possible as an extension only if appended at the end of
a library, so that BB2's token numbers stay; AB3's removals and moves
should not be taken over.
