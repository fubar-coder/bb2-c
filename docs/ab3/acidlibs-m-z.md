# AmiBlitz3 changes to the acidlibs m–z (relative to Blitz Basic 2.10)

What AmiBlitz 2.x/3 changed in the command and system libraries of
`blitz2:acidlibs` from `mathtranslib` to `windowslib` (our port:
`libs/acidlibs/<lib>.c`), as the basis for deciding which changes we take
over. Documentation only; nothing here is taken over yet. Part a–l:
`docs/ab3/acidlibs-a-l.md`.

## Sources and method

* **Baseline:** the CD's `BLITZ2/ACIDLIBS` (BigLib, 1995). Every library
  was extracted with `blitzlib extract` (by the number of our `.obj`) and
  compared with `blitzlib compare`; our build has the same declarations,
  so "BB2" below means both. For the code: `archive/aminet/amibl2src/
  bblibsrc/<lib>` (detokenised with a throw-away script), and the
  `BUG(orig)`/`DIFF` comments of our C files. Our issues: GitHub
  `fubar-coder/bb2-c`, label `bug-orig` + `lib:<name>`.
* **AmiBlitz3:** repository `AmiBlitz/AmiBlitz3` (GitHub; the first 602
  commits up to `AmiBlitz/AmiBlitz3@c95cce17`, 2016-06-01, are the
  SourceForge SVN; SVN numbers from the `git-svn-id` of the SVN import).
  * Binaries: the BigLib `BlitzLibs/AcidLibs` (2009–2020), then
    `BlitzLibs/baseLibs`, `csLibs` and `utilLibs` (the acidlibs were
    split between them). A throw-away script extracted all m–z libraries
    from each of the 84 states of these files and recorded every change
    of a library (hash and size); that gives the date of each binary
    change below. Libraries are matched by number, renumbered ones
    (`mathtranslib`, `stringfunclib`, `utillib`) by their token names
    (`bbsrc tokens`).
  * Sources: `Sourcecodes/Amiblitz3/BlitzLibs/*.bb2` (from
    `AmiBlitz/AmiBlitz3@0e2ff1db`, SVN r2, 2009-02-11: the libraries
    AmiBlitz had changed, tokenised), later `ACIDLIBS/*.ab3` and now
    `Blitzlibs/*.ab3` (mostly disassemblies of the binaries, 2016–2026);
    `Docs/Miscellaneous/Changelog.guide`.
  * The first commit `AmiBlitz/AmiBlitz3@ea8c25c7` (SVN r1, 2009-02-10)
    is already AmiBlitz 2.x: it carries all pre-git changes (dated with
    AmiBlitz's releases and changelogs before git in
    [acidlibs-a-l.md](acidlibs-a-l.md#dating-the-pre-git-changes); a
    date "≤2007-05" means the library of 3.00 beta 4 is already the one
    of r1, without a changelog entry for the change). Its
    windowslib descends from the RedBlitzLibs windowslib of the
    Blitz Support Suite (`BSS2/REDBLITZLIBS/LIBRARIES/BASIC/WINDOWSLIB.OBJ`
    on the same CD, 1997: `AddWaitEvent`, `Wline`, ... are there).
* **Classes:** **fix** (of a BB2 bug; our issue if it matches), **ext**
  (extension), **incompat** (changed declarations/token numbers/
  behaviour, needs OS3.x/FPU/CGX, or depends on the AB3 compiler),
  **cosmetic** (disassembly, comments, renames without effect; only
  counted). AB3-internal fixes of AB3's own regressions are listed as
  "fix (AB3)". "(?)" marks an uncertain reading.
* Breadth over depth: for unchanged-looking code the statement "not
  addressed" means the AB3 source still has the same code at the place
  our issue names (checked for the issues mentioned), not a full review.

## Overview of the binaries

Identical to the CD binary in every AB3 state (only disassembled,
commented or moved between BigLibs): **maxlenlib, maxslib, medlib,
modlib, qblitlib, seriallib, shapeslib, shapetrixlib, speaklib,
spriteslib, stencillib, strcomplib, trackerlib**. All our issues of
these libraries (e.g. medlib #203–#212/#334, seriallib #252–#262,
shapeslib #263–#270, trackerlib #289–#296) are not addressed by AB3.

`utillib` is identical except for the library number. **polylib** does
not exist in AmiBlitz at all (already missing in SVN r1).

Changed in SVN r1 against the CD: memacclib, memlib, menuslib, printlib,
runerrlib, screenslib, staticslib, stringfunclib, strings1lib,
strings2lib, vallib, varslib, wbstartuplib, windowslib. Later binary
changes: see the libraries.

## Per library

### mathtranslib

| AB3 commit | SVN | Date | Class | What | Issue |
|---|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@92eeb2a9`, binary `AmiBlitz/AmiBlitz3@2e84e002` | – | 2020-07-23/08-19 | cosmetic (?) | reassembled from a disassembly; 16 bytes longer, declarations unchanged, no functional difference found | – |
| `AmiBlitz/AmiBlitz3@dbc34db6` (+ `@b043e8fa`) | – | 2026-04-24 | incompat | library number 192 → 44: all 13 tokens change | – |

Not addressed: #202 (init loops forever without mathtrans.library).

### memacclib

| AB3 commit | SVN | Date | Class | What | Issue |
|---|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@ea8c25c7` | r1 | AB2 ≤2.42 (≤2004-08) | ext | `PeekUB`, `PeekUW` (unsigned peeks; changelog 2.27 names them as new commands of wizardlib) | – |

`AmiBlitz/AmiBlitz3@522425fb` (2023) only disassembled it. Not
addressed: #214 (Peeks$ with a negative length).

### memlib

| AB3 commit | SVN | Date | Class | What | Issue |
|---|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@ea8c25c7` | r1 | AB2 2.42–2.43beta26 (2004–2005; library state ≤2007-05) | incompat | rewritten (804 → 1604 bytes): block header 16 bytes instead of 8 (doubly linked blocks, 2.43beta25: "faster memlib thru double link lists. freeing stringmem blitzmem is about 10* faster"), new entry `$c007` (allocret); memory from exec pools (`CreatePool`/`AllocPooled`, V39 – needs Kickstart 3.0; the memlib of 2.43, 2005-07-11, already calls them; puddle size 2000 → 16384 in 2.43beta26), blocks 8-byte aligned (2.42). Changelogs `news2.42.txt`, `news2.43.txt` | – |
| `AmiBlitz/AmiBlitz3@d98d3123`, `@91f3fd7b` | r94, r98 | 2009-09/11 | incompat | "mungwall" (16-byte walls before/after every block, "Mungwall Hit" errors, 32 bytes more per allocation), "better pointer checking"; memory still from exec pools (`CreatePrivatePool`/`AllocPooled`, V39) | – |
| `AmiBlitz/AmiBlitz3@344a7006`, `@f9c423e5`, `@ad8e70a9` | – | 2020-07-06/15 | fix (AB3) | pooling only with V39+, else AllocMem again (Kick 1.3 works again) | – |
| `AmiBlitz/AmiBlitz3@5bb94fff` | – | 2025-11-04 | ext | tracks the allocated size, `$c008` = `UsedMem` (command in environlib) | – |
| `AmiBlitz/AmiBlitz3@b5daf0a9`, `@925282d3`, `@c46a5e9e`, `@23a4611b`, `@eff2302a` | – | 2026-06 | ext / incompat | own pooling for Kick < 3.0, pool API `$c009`–`$c00c` for other libraries; the dynamic lists of arrayslib/linkedlistlib now allocate through it (logic moved between libraries) | – |

Our #215/#956 (error paths, messages in the error check area): AB3's
error code is a different, new design; no mapping.

### menuslib

| AB3 commit | SVN | Date | Class | What | Issue |
|---|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@ea8c25c7` | r1 | AB2 2.42 (2004) | fix | `textcalc`: an empty item/subitem text gets length 1 before the copy (`ALibJsr $cf01`); an unused message "No string with length 0 allowed" was added | – |

Not addressed: #216–#220, #416.

### mouselib

| AB3 commit | SVN | Date | Class | What | Issue |
|---|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@33905811`, `@53927190`, `@75ee467e`, `@c71df476`, `@9636a803` | – | 2026-01-30…02-13 | ext | optional port parameter for several mouse commands (mouse in port 1, two pointers); later commits fix bugs of this change. Tokens unchanged, declarations (forms) differ | – |

Not addressed: #223 (Mouse On swaps the JOY0DAT counters; the AB3 source
still reads `$dff00a` as X).

### palettelib

| AB3 commit | SVN | Date | Class | What | Issue |
|---|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@99f38fed` | – | 2025-11-12 | fix | `SetCycle` check (debugger only): colour range 0–31 without AGA, 0–255 with AGA (before: always 0–255) | – |
| `AmiBlitz/AmiBlitz3@99f38fed` | – | 2025-11-12 | incompat (?) | the new binary is built from AB3's source (= `bblibsrc/palettelib.aga`), not from the CD binary: `Red`/`Green`/`Blue` take `#word` again (CD: `#long`); `make4bitrgbs` uses the top byte (CD: low byte, which makes #224) – so #224 is gone as a side effect (?), but the CD binary's screen check/masking in `AGARed`/`AGAGreen`/`AGABlue` is not in it (?) | #224 (?) |
| `AmiBlitz/AmiBlitz3@2fbc3b8f`, `@dbc34db6` | – | 2026-04 | cosmetic | relinked for renumbered environlib | – |

Not addressed: #225–#229.

### polylib

Not in AmiBlitz (missing since SVN r1) – **incompat** (removed). Our
#417–#422 do not apply to AB3.

### printlib

| AB3 commit | SVN | Date | Class | What | Issue |
|---|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@ea8c25c7` | r1 | AB2 2.43beta25 (≤2005-07) | ext | `UPrint`, `UNPrint` (Print/NPrint ignoring `Format`, like the CD's `UStr$`) | – |
| `AmiBlitz/AmiBlitz3@ea8c25c7` | r1 | AB2 2.4 (2003) (?) | incompat | float output via the FPU (68881 code) when the compiler's FPU mode is set (asked from strings2lib `$c704`) | – |
| `AmiBlitz/AmiBlitz3@27971a6d` | – | 2025-04-02 | cosmetic | FPU mode read once in init (source only; the HEAD binary is still the 2009 one) | – |

Not addressed (code not compared in detail): #233–#242.

### rawkeylib

| AB3 commit | SVN | Date | Class | What | Issue |
|---|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@114e39b9` | – | 2022-07-07 | ext | raw codes `$7A`/`$7B` (NM_WHEEL_UP/DOWN, WinUAE) are returned as they are instead of being converted | – |

Not addressed: #245.

### rundebuglib

| AB3 commit | SVN | Date | Class | What | Issue |
|---|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@11b5984d` | – | 2022-08-10 | fix | requester text pens: the error text was white on white on AmigaOS 1.3 | – |

Not addressed: #6, #43, #246.

### runerrlib

| AB3 commit | SVN | Date | Class | What | Issue |
|---|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@ea8c25c7` | r1 | AB2 2.43 (≤2005-07) | ext / incompat | two new internal entries for the AB3 compiler: `$c31f` updatestack (stack limit from the task's `tc_SPLower`), `$c320` checkpointer ("Error pointer adress <=0" for a2 < 4) | – |

`AmiBlitz/AmiBlitz3@11b5984d` (2022) only disassembled it. Not
addressed: #42.

### screenslib

| AB3 commit | SVN | Date | Class | What | Issue |
|---|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@ea8c25c7` | r1 | AB2 2.4, 2.43beta3 (≤2005-07) | incompat | `Screen`, `ScreenTags` etc. are functions (`!acommand{#long}`): a failed open returns 0 instead of the runtime error "Can't open Screen"; `FindScreen` returns 0 instead of an error; `CloseScreen` returns a result | #251 (?) (other approach: no error at all) |
| `AmiBlitz/AmiBlitz3@ea8c25c7` | r1 | AB2 2.43 (≤2005-07) | ext | `FrontScreen`, `NewCloseScreen`, `GetPubScreen`, `ReleasePubScreen`, `ScreenPens2` | – |
| `AmiBlitz/AmiBlitz3@ea8c25c7` | r1 | AB2 2.43beta3 (≤2005-07) | ext / incompat | `SA_FullPalette` and `SA_Interleaved` always set; depth up to 32: for depth > 8 the mode comes from `cybergraphics.library` (`BestCModeIDTagList`), opened in init; depth 0 uses the viewmode as a full 32-bit display ID | #250 (?) (only for depth 0) |
| `AmiBlitz/AmiBlitz3@41c38d2c` | – | 2022-06-07 | fix (AB3) | cybergraphics.library only opened on Kick ≥ 3.0 | – |
| `AmiBlitz/AmiBlitz3@7960b75c`, `@2fbc3b8f`, `@dbc34db6` | – | 2026-04 | cosmetic | relinked for renumbered environlib | – |

Not addressed: #247, #249; #248 not checked.

### scrolllib

| AB3 commit | SVN | Date | Class | What | Issue |
|---|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@e93d1c9e` (binary `@6c659503`) | – | 2026-04-18/19 | ext | `QPlot` moved here from quicklib | – |

`AmiBlitz/AmiBlitz3@e406ee49`, `@99f38fed` only disassembled it. Not
addressed: #393.

### sortlib

| AB3 commit | SVN | Date | Class | What | Issue |
|---|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@7aa3f9df`, `@d83ff14c` | – | 2025-04-14 | ext | `ArraySort`, `ArrayFill` (was `FillArray` in linkedlistlib), `ArraySize`, `ArrayElements`, `ArrayDataType` | – |
| same | – | 2025-04-14 | incompat | `ArraySort` inserted as token 2: `SortUp` `$B702`→`$B703`, `SortDown` `$B703`→`$B704` | – |
| `AmiBlitz/AmiBlitz3@4fdee2d7` | – | 2025-10-07 | fix (AB3) | `ArrayFill` token config | – |

Not checked: #275, #276 (the sort routines come from the disassembly
`AmiBlitz/AmiBlitz3@fefe8d0b` (?)).

### staticslib

| AB3 commit | SVN | Date | Class | What | Issue |
|---|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@ea8c25c7` | r1 | AB2 2.43 (≤2005-07) | ext / incompat | statics in one allocation instead of one per entry; 8 new internal entries `$c502`–`$c509` (loadtemp/savetemp, temporaries for the AB3 compiler) | – |

### stringfunclib

| AB3 commit | SVN | Date | Class | What | Issue |
|---|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@ea8c25c7` | r1 | AB2 ≤2.42 (≤2004-08) | ext | `Hexb$`, `Hexw$`, `Binb$`, `Binw$`, `PeekMax$`; `Mid$` gets forms `Mid$(s$,start)` and `Mid$(s$,start,len,replace$)` | – |
| `AmiBlitz/AmiBlitz3@f8ea9265`, `@2cb489dc` | – | 2020-11-07/16 | ext | `Trim$` | – |
| `AmiBlitz/AmiBlitz3@dbc34db6` | – | 2026-04-24 | incompat | library number 185 → 6: all tokens change | – |

Not addressed: #285 (Right$ offset still `0(a3,d0.w)`), #287 (Instr
start not checked), #288 (Cvi/Cvl/Cvq without length check); #286, #424
not checked.

### strings1lib

| AB3 commit | SVN | Date | Class | What | Issue |
|---|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@ea8c25c7` | r1 | AB2 2.43beta2 (2004-10) | ext / incompat | string buffer resizable (AB3 "StringBuffer" option): internal `$c602` (buffer base), `$c603` (setstringbase) | – |
| same | r1 | AB2 2.43beta2 (2004-10) | fix | the string copy stops at the end of the buffer (minus 300 bytes) instead of overrunning it | – |

### strings2lib

| AB3 commit | SVN | Date | Class | What | Issue |
|---|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@ea8c25c7` | r1 | AB2 2.27 (library state 2.43, 2005-07) | ext / incompat | internal `$c702` (string assignment for libraries), `$c703`/`$c704` set/get FPU mode (mode 2 executes an FPU instruction); 4 bytes slack per string allocation | – |
| `AmiBlitz/AmiBlitz3@91f3fd7b` | r98 | 2009-11-06 | incompat (?) | binary revised again (560 → 620 bytes; matches the changelog 2.27 note on `$c702`/`$c704`) | – |

### switchlib

| AB3 commit | SVN | Date | Class | What | Issue |
|---|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@6ec7746b` | r597 | 2016-05-05 | fix | null pointer check (enforcer hit on CGX systems at the start of debugging) | – |

The reconstruction commits before it (`AmiBlitz/AmiBlitz3@083f557b` …
`@e3926f0f`, r585–r596) are cosmetic.

### utillib

| AB3 commit | SVN | Date | Class | What | Issue |
|---|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@dbc34db6` | – | 2026-04-24 | incompat | library number 194 → 107 (code identical) | – |

Not addressed: #297–#300.

### vallib

| AB3 commit | SVN | Date | Class | What | Issue |
|---|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@ea8c25c7` | r1 | ≤2007-05 | ext | `Vallong`, `RndSeed` | – |
| same | r1 | ≤2007-05 | incompat | `True`/`False` return `#long` (CD `#word`); `Rnd` is a new seeded generator (start value from VHPOSR once, then deterministic) instead of mixing beam/CIA counters on every call | – |
| same | r1 | AB2 2.4 (2003; library state ≤2007-05) (?) | incompat | `Val` with 68881 code (partly executed unconditionally) | – |
| `AmiBlitz/AmiBlitz3@d5ede9fb`, `@c99bd364` (binary `@6f78a2ed`/`@1db85b0c`/`@16491369`) | – | 2019-11/12 | fix (AB3) | `Val`: separate FPU and non-FPU paths, chosen by the compiler's FPU mode | – |
| `AmiBlitz/AmiBlitz3@d7225ca7`, `@37d8fde0` | – | 2023-11/12 | ext | `Min`, `Max` (from riAmosFuncLib) | – |
| `AmiBlitz/AmiBlitz3@a7e05802` | – | 2024-03-26 | fix (AB3) | `Int`: type constants broken by AB3's source conversion | – |
| `AmiBlitz/AmiBlitz3@721df645` | – | 2025-12-10 | incompat | `NTSC`, `DispHeight`, `VPos` moved to environlib; `True`/`False`/`QAngle` move from `$BC0E`–`$BC10` to `$BC0B`–`$BC0D` | – |
| `AmiBlitz/AmiBlitz3@489c5b4a` | – | 2026-04-19 | ext | `WWrap`, `WLimit` from quicklib (source only, not in the HEAD binary) | – |

Not addressed: #301 (Sgn of a float), #302–#304 (Val: byte past "-",
"$", "%", word length, decimal places; same code in both paths).

### varslib

| AB3 commit | SVN | Date | Class | What | Issue |
|---|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@ea8c25c7` | r1 | AB2 ≤2.42 (≤2004-08) | incompat (?) | init saves all registers and finit restores them at program end, a0 (argument pointer) is passed on for wbstartuplib; no memory for the globals → `ILLEGAL` instead of a clean return | – |

`AmiBlitz/AmiBlitz3@57282d93`, `@c9481779` (2026) only disassembled
and commented it.

### wbstartuplib

| AB3 commit | SVN | Date | Class | What | Issue |
|---|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@ea8c25c7` | r1 | ≤2007-05 | ext | internal entry `$bf01` GetWBStartupMsg (for dos.include), message pointer cleared in finit | – |
| `AmiBlitz/AmiBlitz3@9636a803` | – | 2026-02-13 | fix (AB3) | CLI start: argument pointer a0 checked for 0 (start from the IDE without arguments, after AB3's changed process creation) | – |

Not addressed: #305–#308.

### windowslib

| AB3 commit | SVN | Date | Class | What | Issue |
|---|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@ea8c25c7` | r1 | ≤2007-05 | fix | `CloseWindow` replies and drops only the messages/events of the closed window (`unlinkonlywin`) | #322 |
| same | r1 | ≤2007-05 | fix | `WMove`/`WSize` no longer wait until the position/size matches | #320 |
| same | r1 | ≤2007-05 | fix | `WindowTags …,&TagList` links the user's list with `TAG_MORE` instead of copying it into 32 longs (and no longer writes the title length behind it) | #317, #329 |
| same | r1 | ≤2007-05 | fix (?) | `WTitle` frees the previous window title (in `CloseWindow` the free is commented out) | #318 (partly) |
| same | r1 | ≤2007-05 | incompat | `Window`/`WindowTags` are functions: return the window, 0 on failure instead of the runtime error | – |
| same | r1 | ≤2007-05 | incompat | `WaitEvent` waits on a signal mask (own signal bits via `AddWaitEvent`/`DelWaitEvent`/`WaitEventMask`, returns their code); event records 24 → 32 bytes (seconds/micros) | – |
| same | r1 | AB2 2.4 … 2.43b40 (2003–2005; library state ≤2007-05) | ext | `EventiAddress`, `WaitEventSigBits`, `EventSeconds`, `EventMicros`, `BlitzMsgPort`, `EventDoubleclick`, `WindowBackFillHook`, `EventTest`, `EventRawKey`, `TextFit`, `WinFontHeight`, `SetMenuVerifyHandler`, `EventVanillaKey`, `WBottomSize`, `WRightSize`; optional colour for `WScroll`, screen title optional in `WTitle`, `WindowFont` as a function. Dated: `EventDoubleclick` fixes and `WindowBackFillHook` 2.4/2.41, `TextFit`, `WinFontHeight` 2.43b31, `EventVanillaKey`, `SetMenuVerifyHandler` 2.43b40 (`news2.4.txt`, `news2.42.txt`, `news2.43.txt`); `BlitzMsgPort`, `EventVanillaKey`, `SetMenuVerifyHandler` documented in the 2.43 `WINDOWSLIB.guide` (2005-07-26, `archive/bernd/ab2.42_upd_to_2.43/amiblitz2.43/redhelp/acidguides/`) | – |
| same | r1 | AB2 2.43beta3 (≤2005-07) | incompat | `Window` form with `BitMap#` (SuperBitMap) removed ("superbitmap dont work"); `WLine` renamed `Wline`; NewWindow always gets `WFLG_NW_EXTENDED`. `news2.43.txt` 2.43beta3: "This dont work and cause only crashes if you use it,so its remove now" | #319 (?) |
| `AmiBlitz/AmiBlitz3@19cd5498` | – | 2023-02-14 | ext | `WindowIsOpen` | – |
| `AmiBlitz/AmiBlitz3@d39916e8`, `@472fd6b5` | – | 2024-03-14/04-05 | fix (AB3) | `BitMap#` form of `Window` back | – |
| `AmiBlitz/AmiBlitz3@64435749`, `@1cae33b6` (binary `@22751b30`) | – | 2026-04/09 | ext | `WaitEvent [IDCMP]` (from ElmoreSysLib's WaitFor), ReAction event handling in `WaitEvent` | – |
| `AmiBlitz/AmiBlitz3@bd97e060`, `@03516ca1` | – | 2026-09-24/10-13 | ext | `WindowAddress`, `TextLength` | – |
| `AmiBlitz/AmiBlitz3@268e962d` | – | 2026-10-06 | cosmetic | `Wline` → `WLine` again | – |

Not addressed: #310–#315, #319 (the re-added `BitMap#` form still sets
`bmapbase` for later windows), #321, #323; #316 (?) (a `WindowTags`
without tags still uses the static `usertags`, now with the
`TAG_MORE` of the last call).

## Cosmetic commits (only counted)

About 120 commits touch these libraries without a functional change:
disassembly and conversion to `.ab3` (e.g. `AmiBlitz/AmiBlitz3@9f5e7b8c`
r444 for all sources, switchlib r585–r596, shapeslib, spriteslib,
trackerlib, memacclib, maxlenlib/maxslib, modlib, mouselib, qblitlib,
scrolllib, sortlib, utillib, varslib, rawkeylib, rundebuglib, runerrlib,
stringfunclib), comments and source documentation, moves and renames of
directories/BigLibs (`AmiBlitz/AmiBlitz3@87438cdd` r279,
`@9d854a53`, `@b449b041`, `@8880f56b`, `@c4484065`/`@11b5984d` csLibs),
relinking after renumbering other libraries (`@7960b75c`, `@2fbc3b8f`,
`@dbc34db6` for palettelib/screenslib/windowslib), and "updated
biglibs" commits.

## Summary

Counted entries of the tables above (a row naming several commits
counts once; rows with two classes count for both):

| Class | Count | Notes |
|---|---|---|
| fix (of a BB2 bug) | 9 | menuslib empty text, palettelib SetCycle check, rundebuglib OS1.3 pens, strings1lib overrun, switchlib CGX null pointer, windowslib ×4 |
| fix (AB3, own regressions) | 7 | memlib Kick 1.3, screenslib CGX, sortlib, vallib ×2, wbstartuplib, windowslib BitMap# |
| ext | 24 | |
| incompat | 23 (+ polylib removed) | incl. renumbered libraries (mathtranslib, stringfunclib, utillib), moved tokens (sortlib, vallib), removed polylib, FPU/CGX/V39 dependencies, AB3 compiler interfaces |
| cosmetic | 5 rows + ~120 commits | |

Our issues → AB3 changes:

* #322 → `AmiBlitz/AmiBlitz3@ea8c25c7` (r1): CloseWindow keeps other windows' events.
* #320 → `AmiBlitz/AmiBlitz3@ea8c25c7` (r1): WMove/WSize without the wait loop.
* #317, #329 → `AmiBlitz/AmiBlitz3@ea8c25c7` (r1): WindowTags `&TagList` via `TAG_MORE`.
* #318 (partly, ?) → `AmiBlitz/AmiBlitz3@ea8c25c7` (r1): WTitle frees the old title.
* #251 (?) → `AmiBlitz/AmiBlitz3@ea8c25c7` (r1): Screen returns 0 instead of the error (another approach than our fix).
* #250 (?, only depth 0) → `AmiBlitz/AmiBlitz3@ea8c25c7` (r1): full 32-bit display ID.
* #224 (?) → `AmiBlitz/AmiBlitz3@99f38fed`: side effect of rebuilding palettelib from the older source.

Everything else of our m–z issues is not addressed by AB3. Candidates
worth a look for our port, once original bugs may be fixed: the
windowslib fixes (#317/#329, #320, #322), the menuslib empty-text guard
and the strings1lib buffer limit. Most other AB3 changes depend on the AB3
compiler (strings1lib/strings2lib/staticslib/runerrlib internal entries,
FPU mode), on newer OS versions/CGX, or change token numbers.
