# Plan phase 3: emulator A/B tests of TED-C

Basis: [PLAN.md](PLAN.md) §6 (c), [reference.md](reference.md),
[overview.md](overview.md) §4, [deviations.md](deviations.md) and the
existing emulator infrastructure in bb2 `compilerlauf/` (runner
`tools/bb2compile.py`, stub and requester tools in `tedstub/` – here `tests/corpus/amiga/`,
`compilerlauf/README.md`,
`compilerlauf/tedstub/README.md`)
as well as bb2 `ci-exploration.md` (bb2 only, not taken over).

State at planning (26.10.2026): phase 2b completed, `ctest --preset
amiga` (ref_verify, checkhunk, checkdata, checklabels, checklink) and
`ctest --preset host` (11 tests, among them `test_loadsave`: 724 files,
0 errors) green. TED-C has **never run** yet – phase 3 is the first
start on an Amiga.

Goal: **same observable behaviour** of TED-C (`build/amiga/blitz2/ted`)
and reference TED (`build/amiga/programs/ted/ref/ted`, vasm build of `ted.s`), measured
in the emulator with both compilers, without operator, never blocking.
Not a goal: comparison with the CD binary of TED (reference.md §1, separate
step), performance measurement.

---

## 1 Starting point

| Component | Path | State |
|---|---|---|
| Reference TED | `build/amiga/programs/ted/ref/ted` (57060 bytes) + `ted.lst`/`ted.map`/`ted.labels` | present, `ref_verify` green |
| TED-C | `build/amiga/blitz2/ted` (69432 bytes), link map `build/amiga/programs/ted/ted.map`, symbols `build/amiga/programs/ted/ted.sym` | built, untested |
| blitz2 v1.95 (vasm) | `compilerlauf/blitz2-195` (here `build/amiga/programs/blitz2/ref/blitz2`) | proven in the corpus |
| blitz2-c | `build/amiga/blitz2/blitz2` | byte-equal with 1.95 in the corpus (724/724) |
| Libraries/helpers | `archive/cd/ultimate-blitz-basic-2.1/BLITZ2/{DEFLIBS,ACIDLIBS,USERLIBS,DEFAULTDBUG,HELP,HELP.DAT}` | copied by `bb2compile.prepare()` (except `help`, `help.dat`) |
| Emulator start | `bb2/tools/amiberry.cmake` (bb2; here `tools/lib/amiga_emulation`) (without window; script to `AB3C:build/amiberry/run.script`, `user-startup` runs it, end via `RES:done` and `uaequit`, time limit terminates Amiberry; until 27.10.2026 `../ab3c/tools/amiberry_run.py`) | proven; configuration via `--config`/`AMIBERRY_CONFIG` |
| Lock | bb2 `libs-c/build/.amiberry.lock` (`bb2compile.LOCK`) | one emulator at a time |
| Synthetic inputs | `tests/corpus/amiga/reqclose.h` (IntuiMessages into the UserPort, `-DREQCLOSE_INPUT`: old input.device variant), starter `reqtrap.c` (AutoRequest/EasyRequest patch, `SystemTags`) | proven for requesters; **no** tool for operating TED |
| Oracles without TED | bb2 `compilerlauf/tools/bb2tok.py` (tokenizer like TED), `tedstub` (compiles without UI), `corpus_check.mask_trap1()` | present |

To be newly built are the companion process `teddrive` (§4), the runner
`tedab.py` (§5), the dump decoder `teddump.py` (§6.1) and an
emulator configuration with a native Workbench screen (§3).

## 2 A/B setup

### 2.1 Four combinations

| Code | TED | Compiler | Role |
|---|---|---|---|
| **R5** | reference (`build/amiga/programs/ted/ref/ted`) | blitz2-195 | **reference** (everything original, only vasm-assembled) |
| **C5** | TED-C (`build/amiga/blitz2/ted`) | blitz2-195 | **mandatory**: checks TED-C against the original compiler (register interface `comdata`, callbacks `menurout`/`loadxtra`/`savextra`/`cleanup` in asm code) |
| **Cc** | TED-C | blitz2-c | **mandatory**: target combination (everything C); two `-fbaserel` programs each with its own a4 (overview.md §4 risk 2) |
| **Rc** | reference | blitz2-c | control: on a deviation Cc↔R5 separates the cause (compiler or editor); blitz2-c is already confirmed against 1.95 in the corpus, hence lower priority |

Each comparison changes only one variable: **R5↔C5** (editor with the
original compiler) is the main A/B comparison, **Rc↔Cc** the same with the
C compiler, **R5↔Rc** confirms the compiler. Expectation: all four equal
except for the masks in §6. Stage 1 (§7.1) runs only R5 and C5; Rc/Cc are
added as soon as R5↔C5 is clean.

### 2.2 Setup in the emulator

Like `bb2compile.prepare()` (`compilerlauf/tools/bb2compile.py`), but with
real TED (the runner already has `--ted PATH` for this), under
`../ab3c/build/amiberry/ted-ab/TAG/` (emulator: `AB3C:build/amiberry/ted-ab/TAG/`):

```
TAG/blitz2/       blitz2, ted, deflibs, acidlibs, userlibs/, defaultdbug, help, help.dat
TAG/L/            blitzeditor.opts (given per test or empty)
TAG/orig/         unchanged test sources (+ .xtra)
TAG/work/         working copies that TED loads and saves (copied anew per run)
TAG/out/          dumps, logs, result files, executables
TAG/teddrive      the companion process
```

* **blitz2 starts TED itself**: `blitz2:blitz2 AB3C:…/work/NAME.bb2` →
  blitz2 loads `blitz2:ted`, calls hunk+4 (`outside`) and then `.tomain`
  with the file name; TED loads the file (as in the tedstub flow,
  `tedstub/README.md` "Ablauf"). Exit via the menu QUIT (`escape`)
  → TED returns → blitz2 ends.
* **No standalone TED**: without blitz2, `tokens = 0` (deviations.md,
  line `findtoke`), tokenised sources would be read via address 0.
  All tests run via blitz2.
* **Isolate `l:blitzeditor.opts`** (`defname`, src/core/state.c:60):
  `loaddefs`/`savedefs` read/write `L:` of the system disk. The run
  sets `Assign L: AB3C:…/TAG/L` + `Assign L: SYS:L ADD` (own
  directory first, writing goes there) and at the end `Assign L: SYS:L`.
  Without a given file TED starts with `defstart` (data from the listing,
  `checkdata`). Before the first run check that `SYS:L/blitzeditor.opts`
  does **not** exist on the disk (otherwise it would apply to tests without their own
  file – `List SYS:L/blitzeditor.opts` in the gate run, §3.3).
* Test sources: `.bas` from `tests/corpus/tests` are generated as in
  `prepare()` with `bb2tok.tokenise()`, CD sources copied from `archive/cd/ultimate-blitz-basic-2.1/`
  (with `.xtra`).

## 3 Emulator: native chipset is mandatory

### 3.1 Finding

TED opens its screen **as a copy of the Workbench mode**:
`calcscreen` (ted.s:4619, src/screen.c `calcscreen`) reads
`GetScreenData(…, WBENCHSCREEN)` and takes over width, height and
`ViewPort.Modes`; `extnewscreen` (src/data.c) additionally sets
`SA_LikeWorkbench` (only `SA_Depth` = 2 is overridden). Then TED draws
**directly** into `BitMap->Planes[0/1]` (CPU, `print3`) and with the blitter
via `$DFF000` (`eraseline`, `scrollup`/`scrolldown`, `cls`; src/screen.c).

The shared test configuration `~/Amiberry/Configurations/A1200-AB3.uae`
has an **RTG Workbench** (uaegfx, `gfxcard_size=16`, 1680×1050, see
ci-exploration.md §2.1). TED would clone an RTG screen there: the
bitplanes would be in graphics card memory, the blitter cannot
address them (drawing into nowhere or into foreign chip RAM), and with 1050
lines the file requester hits the known overflow from 605 pixels
window height (reference.md §3.2, `libs/acidlibs/freqlib.c`). **TED is not testable with
this configuration** – this applies equally to both TEDs, so it is
not an A/B error, but a blocker.

### 3.2 Test configuration `A1200-AB3-AGA.uae` (B1 solved)

Created by the user (27.10.2026): `~/Amiberry/Configurations/A1200-AB3-AGA.uae`
= `A1200-AB3.uae` without RTG card (`gfxcard_size`/`gfxcard_type` removed)
with its own hardfile `Workbench_OS3.2-AB3-AGA.hdf`; Workbench mode **PAL
High Res Laced, 256 colours (depth 8)**. Selection via `AMIBERRY_CONFIG`
(`tedab.py` sets it itself, `--config` overrides). Configuration and
hardfile belong to the user and are not changed by the tools;
there is no separate `A1200-TED.uae`. Smoke test of the
user: `run.script`, `RES:`, `AB3C:`, `BB2:` work.

| Setting | Value | Remark |
|---|---|---|
| `chipset=aga`, `ntsc=false` | as `A1200-AB3.uae` | PAL/AGA |
| Workbench mode | native, PAL High Res Laced, depth 8 | TED clones width/height/mode (`calcscreen`), depth always 2 (`SA_Depth`); gate 0 records the actual values (O-P2) |
| RTG card | none | no screen can end up on uaegfx |
| `joyport0` | `mouse` (unchanged) | route D does not need the mouse position. **Risk:** a click of the host mouse into the Amiberry window generates real MOUSEBUTTONS messages for TED – do not click into the emulator window during a session |
| `chipmem_size` | 8 (unchanged) | more than a real A1200; changes only `AvailMem` (masked) |
| `cpu_speed=max`, JIT | as before | timing via "TED is waiting" (§4.4), not via time |

### 3.3 Gate 0 (before all tests)

A run without TED: a small probe program (`tedprobe`, own program
`tests/ted-ab/amiga/tedprobe.c`, target `build/amiga/tests/ted-ab/amiga/tedprobe`) writes `GetScreenData(WBENCHSCREEN)` (width, height, `ViewPort.Modes`,
`GetVPModeID`, depth), `TypeOfMem(Planes[0])` (must be `MEMF_CHIP`),
`AvailMem(MEMF_CHIP)` and the presence of `SYS:L/blitzeditor.opts`
to `out/gate0.txt`. The runner aborts if the mode is not native
or the size is not the expected one. The same result is in every
screen dump (header, §6.1), so that dumps of different runs are only compared with
the same geometry.

## 4 Companion process `teddrive`

### 4.1 Structure

`teddrive SCRIPT COMMAND [ARGS]` – built like `reqclose` from
`tests/corpus/amiga/reqtrap.c` (there: AutoRequest/EasyRequestArgs
patched, command via `SystemTags`, second process via
`CreateNewProcTags`, end via stop flag and signal). New is the second
process: instead of the autonomous requester loop of `reqclose.h` it plays back
a **script**. Taken over from `reqclose.h` are the proven parts:
window search under `LockIBase` in the window lists of all screens,
`rc_valid`, `rc_send` (Forbid + LockIBase, check window again,
`PutMsg` of a static `ExtIntuiMessage` with its own reply port and
`IDCMPWindow`), gadget search via IntuiText, log buffer. For this,
split `reqclose.h` so that the helpers can be included without `rc_main`
(owned by compilerlauf; `reqclose` must afterwards be byte-equal or
unchanged green with `reqclosetest`), or the helpers as a copy with
an origin note in `ted-c/tests/amiga/` – decision when implementing
(O-P1, §10).

Implemented (deviations from this section, justification in
[PHASE3-LOG.md](PHASE3-LOG.md)): invocation `teddrive SCRIPT PREFIX COMMAND
[ARGS]` instead of `ENV:TEDDRIVE`/`ENV:TEDDRIVELOG` (log `PREFIXlog`, result
`PREFIXresult`, dumps `PREFIXNAME.scr`); the AutoRequest/EasyRequest patch
comes from the unchanged `reqtrap` (`reqtrap teddrive …`), `REQUESTER:` lines
are therefore in the `.stdout` file; script syntax in
`tests/ted-ab/amiga/tdscript.h` (checked on the host against `tedab.py`).

Location: `tests/ted-ab/amiga/teddrive.c` (+ `teddrive.h`), CMake target in preset
`amiga` (`build/amiga/tests/ted-ab/amiga/teddrive`), compilable like `reqtrap` without C
runtime. The AutoRequest patch stays: DOS requesters ("Please insert
volume …") and alerts via EasyRequest must not stop the run; their
text appears as `REQUESTER: …` in the log.

Script: path in `ENV:TEDDRIVE` (or first argument); log appended to
`ENV:TEDDRIVELOG` (file on `AB3C:`, `Open`/`Seek`/`Write`/`Close` per
line like the tedstub log, survives hangs and crashes).

### 4.2 How TED reads input – what must be injected

TED reads **only IDCMP** of the current window: `getinput`/`getinput2`
(ted.s:5845/5833, src/input.c) fetch from `pickwindow()->UserPort`
(`req_window`, otherwise `mywindow`), `handled0` (ted.s:5866) evaluates:

| Class | what TED reads | Injection |
|---|---|---|
| `IDCMP_RAWKEY` | `Code` (key-up ignored), `Qualifier` → `rawed_convert` (RawKeyConvert with its own InputEvent, F1–F10 → `$81–$8A`, cursor → `$1C–$1F`, Help → `$8B`) | **direct IntuiMessage** (like `rc_send`): `Class`, `Code`, `Qualifier`; text via `MapANSI` (keymap.library V36) into rawkey+qualifier, so that the configured keyboard layout matches (TED uses it via console.device) |
| `IDCMP_MENUPICK` | only `Code` (`FULLMENUNUM`), `menu`/`extmenu` convert menu/item themselves (src/edit.c `menu`), `NextSelect` ignored | **direct IntuiMessage**, `Code = menu | item<<5 | 31<<11` |
| `IDCMP_GADGETUP/DOWN` | `IAddress` → `GadgetID`, `GadgetType`, `Activation & ENDGADGET` | **direct IntuiMessage** with `IAddress` = gadget (like reqclose) |
| Return / Esc in the requester | `handled0`: Return → first ENDGADGET (bool), Esc → last ENDGADGET | RAWKEY `$44` / `$45` – also checks TED's own path |
| `IDCMP_MOUSEBUTTONS` | only `Code` (SELECTDOWN/UP); the **position** is read by `getmousexy` (ted.s:4015) from `myscreen->MouseX/MouseY` | the pointer must really be there → `input.device` `IND_WRITEEVENT` `IECLASS_NEWPOINTERPOS` (`IESUBCLASS_PIXEL`, screen = TED's screen), then the button as RAWMOUSE event or direct MOUSEBUTTONS message |
| Amiga keyboard shortcuts (`COMMSEQ`, e.g. Amiga-S) | generated by Intuition from RAWKEY+Amiga | only via `input.device` (RAWKEY with `IEQUALIFIER_RCOMMAND`) |

From this, two routes:

* **Route D (direct, stage 1/2)**: keys, menu selection, gadgets, requester
  answers as IntuiMessages into the UserPort. Deterministic, without
  input.device; reqclose has proven exactly this technique in the test emulator.
* **Route I (input.device, stage 3)**: only for mouse position, clicks into the
  text area/the prop gadgets and Amiga shortcuts. The former input.device
  variant of reqclose paralysed the input task in the test emulator – that
  was on the **RTG** Workbench (tedstub/README.md "reqclose", last
  point); re-check on the native screen: **gate I** = small
  `teddrivetest` (own screen, set pointer, click, shortcut, result
  "expected"/"MISMATCH" like `reqclosetest`), route I only if the gate
  is green.

### 4.3 Requesters

| Requester | Detection | Answer |
|---|---|---|
| TED's own (`openreq`, src/reqs.c: window `reqwindow` **without title** on TED's screen): erase, error, find, repall, goto, defaults, about | new window on TED's screen; identifier = list of the gadget texts and IDs (IntuiText), e.g. erasereq, errorreq from src/data.c | `gad ID` (GADGETUP on ENDGADGET → `handled0` closes), `key $44`/`$45` |
| blitz2 requesters via comdata `openreq` (version requester "OKEE DOKEE", "COMPILING", errors via `error` comdata+12) | as above (run through TED's `openreq`; gadgets are shifted once by +6/+12, relevant for deviations O6) | version requester: `gad 1` or Return |
| File requester (freqlib core, `ezfreq`) | window title = title text: "Name of file to load", "Name to save file as" (src/core/state.c:72/73), "Name of Executable to create" (blitz2) | `freq ok` = GADGETUP on "OK" (like `reqclose ok TITLE`; path/name are preset with `filepath`/`filename` of the loaded file); `freq ok NAME` first writes the name into the buffer of the name string gadget |
| unexpected | any other new window | log with all gadgets (like `rc_dumpgadgets`), test = FAIL, rescue sequence §4.5 |

**String/integer gadgets** (search/replace text `findreqsg1/2`, line in the
goto requester, tab stop/colours in the defaults requester): typing via route D
does not reach them (the editing is done by Intuition). `str ID "text"`
writes under Forbid + LockIBase into `StringInfo->Buffer`, sets
`NumChars`, `BufferPos = DispPos = 0` and with `GACT_LONGINT` also
`LongInt` (TED reads e.g. `int3 = defaultreqsg3.LongInt`, src/data.h:79),
then `RefreshGList`; afterwards GADGETUP on the string gadget or on the
button. Same for A and B, hence neutral for the comparison.

### 4.4 Synchronisation: "TED is waiting"

Every step waits until TED has processed the previous input – never
fixed pauses. The reply (`ReplyMsg`) is **not** sufficient: `handled0` replies
before executing the routine. Criterion `idle` (read under `Forbid`):

* task of the window (`UserPort->mp_SigTask`) in `TS_WAIT`,
* `tc_SigWait` contains `1 << UserPort->mp_SigBit` (TED is in
  `WaitPort` of `getinput`, not in `Delay` of `refme`/`actgad` – there
  it waits for the DOS signal),
* port empty, no own message unanswered,
* this three times in a row at a 40 ms interval (`Delay(2)` like reqclose).

Busy polls (`mousedown` dragging, compiling with STOP query via
`getinput2`) are never `idle`; there the script waits for the result
(new window, window gone, process ended).

### 4.5 Script language (proposal)

```
; comment
timeout 15                 time limit for every following wait (s)
wait screen                TED's screen open (title "Blitz Basic 2"/"File: ") and idle
wait req LABEL [LABEL…]    new window on TED's screen with these gadget texts, TED idle
wait freq TITLE            file requester with this title
wait gone                  topmost requester window closed, TED idle
wait idle
wait exit                  blitz2 ended (SystemTags returns)
key CODE [QUAL]            RAWKEY (hex), then idle
type "TEXT"                MapANSI → RAWKEY sequence, idle after every key; \n = Return
menu M I                   MENUPICK (menu M, item I; COMPILER = menu 4)
gad ID                     GADGETUP in the topmost TED window
str ID "TEXT"              fill string gadget (§4.3)
freq ok|cancel [NAME]
pointer X Y | click X Y    route I only (gate I)
dump NAME                  screen to out/NAME.scr (§6.1)
state NAME                 internal state to out/NAME.st (§6.2, stage 2)
mem NAME                   AvailMem(MEMF_ANY/CHIP) into the log
log "TEXT"
```

Every line is written to the log **before** execution (`step N …`),
the result afterwards (`ok`, `timeout`, seen windows with gadgets). If
a `wait` runs into the time limit: rescue sequence – Esc into every TED window,
`freq cancel`, menu QUIT (`menu 0 9`), on the erase requester gadget 1
(`erase2` only ends with `d6 == 1`, src/reqs.c) – then `RESULT FAIL step reason`; if that blocks too, the
emulator watchdog ends the session (§5).

Result per run: `out/RUN.result` with exactly one line `OK`,
`FAIL step reason` – written by teddrive; if the file is missing after the
session, the runner sets `TIMEOUT` (or `CRASH` if the run started
but did not finish and the session ended early).

## 5 Runner `tedab.py` and sessions

Location bb2 `ted-c/tools/tedab.py` (here `tests/ted-ab/runner/`; imports `compilerlauf/tools/bb2compile.py`
for `prepare` parts, `LOCK`, `AB3C`, `amiga_path`, and `corpus_check.mask_trap1`).

```
python3 -I tools/tedab.py [--combos R5,C5,Rc,Cc] [--test GLOB] [--timeout 90] [--keep]
  -> results/ted-ab/COMBO/TEST/…   (files, dumps, logs, .result)
  -> results/ted-ab/report.txt     (matrix test × comparison: SAME/DIFF/KNOWN/FAIL/TIMEOUT)
```

* **Run** = (test, combination). Script part per run:
  ```
  Echo "RUN t03-join C5 start" >>AB3C:…/out/session.log
  Assign blitz2: AB3C:…/C5/blitz2
  Assign L: AB3C:…/C5/L
  Assign L: SYS:L ADD
  Copy AB3C:…/orig/x.bb2#? AB3C:…/C5/work/ QUIET
  SetEnv TEDDRIVE AB3C:…/tests/t03-join.drv
  SetEnv TEDDRIVELOG AB3C:…/C5/out/t03-join.log
  Stack 40000
  AB3C:…/teddrive blitz2:blitz2 AB3C:…/C5/work/x.bb2 >AB3C:…/C5/out/t03-join.stdout
  Echo "rc=$RC" >>AB3C:…/C5/out/t03-join.stdout
  Assign L: SYS:L
  Assign blitz2:
  Echo "RUN t03-join C5 end" >>AB3C:…/out/session.log
  ```
* **Session** = several runs one after another in **one** emulator start
  (boot ≈ 15 s, `tools/amiberry.cmake`), `--timeout 90` (like
  `bb2compile.BATCH_TIMEOUT`). Based on the measured run time
  (initial value: 10 s per run), the runner packs at most as many runs that ≤ 60 s are
  expected; the combinations of a test are in the same session
  (same system state for A and B).
* **Hangs**: watchdog = time limit of `tools/amiberry.cmake` (terminates Amiberry,
  partial results remain in `AB3C:`). The runner reads `session.log`: the
  run with `start` without `end` becomes `TIMEOUT`, the rest are repeated in a
  new session (pattern `bb2corpus.run_session`). After two
  TIMEOUTs of the same test in the same combination no third attempt.
* Runs do not change the system permanently: `L:` is reset, all
  files are on `AB3C:`; a crashed TED leaves at most
  memory (next run = new session, fresh boot).

## 6 What is compared

| Subject | Source | Comparison | Mask/note |
|---|---|---|---|
| saved source | `work/NAME.bb2` after SAVE | A↔B **byte-equal**; additionally against oracle (round trip: = original; typed text: = `bb2tok.tokenise(expected text)`) | – |
| `.bak` | `work/NAME.bb2.bak` (filesave: Delete + Rename, src/main.c `filesave`) | existence and contents = old file | – |
| `.xtra` | `savextra` callback when saving | A↔B byte-equal; R5↔Rc shows whether blitz2-c writes the same options | – |
| `.info` | `srcinfo` (396 bytes, reference.md §3) with `makeicons` | byte-equal (icon position is not set by TED) | – |
| Executable | COMPILER → CREATE EXECUTABLE | A↔B byte-equal; additionally = executable of tedstub for the same source and the same compiler | with "Runtime Errors": `mask_trap1` (address of the editor line after `trap #1`/`#4` depends on memory); preferably `.xtra` with the option switched off (like `bb2compile --no-runerr`, bytes 2–5) |
| Error line → cursor | `error` (comdata+12) → errorreq, then `menurout` → d0 → `gotolined0` (src/edit.c) | requester identifier equal; afterwards status line "Line:"/"Column:" and cursor in the dump equal and = expected line | – |
| Screen | `dump` (§6.1) after defined steps | bitplanes byte-equal | field "Free" (status line, `printmem`: `statsloc+38`, 5 characters) |
| Requester sequence | teddrive log: every new window with title and gadget texts | sequence equal | do not compare window pointers/addresses |
| Options file | `TAG/L/blitzeditor.opts` after DEFAULTS → OK (`savedefs`, 50 bytes) | byte-equal, = expected image from `defstart` with change | – |
| Screen title | `Screen->Title` in the dump header (`newsctit`: "File: NAME", help text after Help) | equal | – |
| Memory balance | `mem` before the start and after `wait exit` | difference A↔B equal (leaks like `join` FreeMem without `lenitem` occur in both) | not absolute values (program sizes differ); stage 2 |
| Return of blitz2 | `rc=` in `.stdout` | equal | – |

### 6.1 Screen dump

**Mechanism**: teddrive waits for `idle`, then under `Forbid` (TED
draws only in its own task, which is waiting; the blitter is free after
`WaitBlit`/`DisownBlitter`): find TED's screen (window of the
blitz2 process, title), `BitMap = Screen->RastPort.BitMap`, write file
`out/NAME.scr`:

```
Header (text, 1 line per field): magic TEDSCR1, Width, Height, Depth,
  BytesPerRow, Rows, ModeID, MouseX, MouseY, Title, number of windows on
  the screen (with position/size), TypeOfMem(Planes[0])
then Depth × BytesPerRow × Rows bytes (plane 0, plane 1)
```

(640×512×2: 80 KB per dump.) This is robust, needs no addresses in TED
and is the same for both TEDs. Open TED requesters are windows on
the same screen and are in the picture too – desired (requester contents
equal), position is in the header.

**Evaluation** (bb2 `ted-c/tools/teddump.py`, here `tests/ted-ab/runner/teddump.c`): PNG for humans; text
decoding of the editor area with the TED fonts (`chars8.b`/
`chars.b` from `archive/sources/ted-1.1`,
like `bin2c.py`; position: from line `topskip` = 12, `fonthite` 8 or 6,
character `(c&15) + (c>>4)*128 + r*16` or `*96`, reference.md §3) → text
per line + attribute (inverse = block/cursor, plane 1), so that a DIFF appears as a
readable line difference; status line (`statsloc`, texts
"Line:", "Column:", "Block") decoded, "Free" masked. Comparison:
planes byte-equal after mask, otherwise text diff + difference image.

**chmap** (TED's character map, `chmap`, chcols × edrows, chip RAM) is
thus not needed: TED maintains it in parallel to the bitplanes (same
characters; DIFF "chmap blits": in TED-C with the CPU instead of the blitter), the
decoding delivers the same information for the visible area. Whoever wants it directly: §6.2.

### 6.2 Internal state (stage 2, optional)

For debugging: `state NAME` reads TED's variables (`chmap` contents,
`cursx`/`cursy`/`cursoff`/`curstop`, `numlines`, `btopx…bboty`,
`allalt`, `tabstop`, `tokenise`) and the line list from `firstitem`
(`struct item`: length + bytes – the in-memory image of the source, comparable
without saving).

TED publishes no addresses (`comdata` is known only to blitz2). Entry
via the **VBlank server `myint`** (ted.s:306, src/main.c:22;
`AddIntServer(INTB_VERTB, &myint)`): under `Disable`, teddrive walks
the server list `SysBase->IntVects[INTB_VERTB].iv_Data`, takes nodes with
`ln_Type = NT_INTERRUPT`, `ln_Name = NULL`, `is_Data = NULL`, computes
the hunk base `= &myint − OFF(myint)` and checks it with an identifier in the
same hunk (`deftitname` "Blitz Basic 2 - Ted Version 1.1" or another
constant datum located in the same hunk). The offsets are generated
by the runner per TED as a description `TED.desc`:

* Reference: `build/amiga/programs/ted/ref/ted.labels` (one hunk; e.g. `myint` `$632`,
  `chmap` `$5E5A`, `cursx` `$5E3E`, `curstop` `$5E54`, `firstitem`
  `$5E60`, `numlines` `$5E6C`, `defstart` `$5E8C`),
* TED-C: `build/amiga/programs/ted/ted.map` / `ted.sym` (`-fbaserel`: variables and
  `myint` in the data hunk; offsets relative to its start).

Field widths according to `structs.h`/`state.h` (the Amiga layout is bound to the original via
`_Static_assert`). Stage 1 does not need this.

## 7 Test list

Sources: `T` = `tests/corpus/tests/NAME.bas` (tokenised by the runner with
`bb2tok`), `S` = `tests/ted-ab/src/syntaxerr.bb2`
(error in line 2, `b=(a+`), `K` = CD source from `archive/cd/ultimate-blitz-basic-2.1/`. Every test
starts with `wait req "OKEE DOKEE"` → `gad 1` (version requester of
blitz2) and `wait screen`, ends with QUIT (`menu 0 9`) and if needed the erase
requester, `wait exit`; in between `dump` at the named places.

Menu numbers from `menutable` (src/input.c): PROJECT 0 (NEW 0, LOAD 1, SAVE 2,
DEFAULTS 3, ABOUT 4, PRINT 5, CLI 6, CLOSE WB 7, OPEN WB 8, QUIT 9), EDIT 1
(COPY 0, KILL 1, BLOCK SAVE 2, BLOCK LOAD 3, FORGET 4, INSERT LINE 5,
DELETE LINE 6, JOIN 8, BLOCK TAB 9, UNTAB 10, MARK INDENT 11), SOURCE 2
(TOP 0, BOTTOM 1, GOTO 2), SEARCH 3 (FIND 0, NEXT 1, PREVIOUS 2,
REPLACE 3, REPLACE ALL 4), COMPILER 4 (blitz2; CREATE EXECUTABLE = item 2
as in tedstub). Keys: F1/F2 block start/end (`$50`/`$51` → 129/130),
F3 save block, F4 copy block, Help `$5F`, cursor `$4C–$4F`.

### 7.1 Stage 1 (R5 ↔ C5, route D only)

| No | Test | Source | Steps | Compared |
|---|---|---|---|---|
| 00 | Start/exit | T `t01_int` | load, `dump start`, QUIT | dump, requester sequence, rc; **smoke test** – only when it runs in C5 is the rest worthwhile |
| 01 | Round trip | T `t01_int`, T `t08_tokquirk`, K 2–3 small tokenised sources (e.g. `bui5/makefile/includes/finish_mouse.bb`, `bui6/mapslib/mikesmaplib.bb`) | load, SAVE (`freq ok`), QUIT | `.bb2` = original, `.bak`, `.xtra`, `.info`, dump |
| 02 | Typing + saving | T `t01_int` | BOTTOM, `type` several lines (keywords, strings, `;` comment, `$` hex, tab), SAVE | `.bb2` A↔B and = `bb2tok.tokenise` of the expected text; dumps after typing |
| 03 | Join/split | T `t02_float` | cursor to line n, column k, Return (split); JOIN (`menu 1 8`); SAVE | file, dumps, memory balance (join leak BUG(orig), equal in A and B) |
| 04 | Copy/delete block | T `t04_arr` | F1/F2 over 3 lines, cursor further, COPY (`menu 1 0`) or F4, KILL (`menu 1 1`), FORGET; SAVE | file, dumps (block marking inverse) |
| 05 | Search/replace | T `t03_str` | FIND (`menu 3 0`), `str` search/replace text, NEXT; REPLACE; REPLACE ALL (repall requester), SAVE | file, dumps, requester sequence |
| 06 | Compiling via menu | T `t01_int` (`.xtra` without runtime errors) | `menu 4 2`, `freq ok` ("Name of Executable to create"), wait for end of "COMPILING", QUIT | executable A↔B and = tedstub result (corpus `results/blitz2-195-…`), requester sequence |
| 07 | Error line | S | `menu 4 2`, `freq ok`, `wait req` errorreq → `gad 1`, `dump` | cursor/status line = line 2, dump A↔B |
| 08 | loadxtra/savextra | K source with its own `.xtra` | load (loadxtra), SAVE under a new name (`freq ok NAME2`), QUIT; second run loads NAME2 and saves again | `NAME2.xtra` = original `.xtra`; second round trip unchanged |
| 09 | Options | – (new file) | DEFAULTS (`menu 0 3`), `str` tab stop 4, OK; QUIT; second run with this `L/blitzeditor.opts` | options file A↔B, dump in the second run (tab width) |

### 7.2 Stage 2 (all four combinations, broader)

* Round trip over a larger selection of the 724 CD sources (pack sessions
  full; the host round trip `test_loadsave` is already green – what counts here is
  the real input/output path with `dos.library`, `.bak`, `.info`).
* Text files (9 plain-text `.bb2` of the CD) with `tokenise` on: result =
  `bb2tok tok` taking `readline` into account (last line without LF stays
  untokenised, BUG(orig), fileio.c:215).
* Font 6 pixels (`fonthite` via DEFAULTS), window split,
  scrolling past the window end (`scrollup`/`scrolldown`, blitter), TOP/BOTTOM/GOTO.
* Block to file / from file (BLOCK SAVE/LOAD, file requester with
  `blokpath`/`blokname`), BLOCK TAB/UNTAB, MARK INDENT.
* NEW with confirmation (erase requester yes/no), LOAD of a second file.
* Help key on a token (title bar = help text, in the dump header); not
  with the right Amiga key (starts `blitz2:help`).
* Long lines (127/128 characters, wrap in `readline`), umlauts (`germs`,
  bit 30), Latin-1 `$FD–$FF` (cmp.b dispatch, overview.md §4 risk 6).
* Compiling all `corpus/tests/*.bas` via the menu (executables against
  `compilerlauf/results/…`), error lines from `reference/195.tsv`
  (sources with `error`, line → cursor).
* `state` comparison (§6.2) and memory balance for all stage 1 tests.
* Out of memory: `outmem` during editing (longjmp to `memcont`) –
  only with a memory eater in the companion process, own session.

### 7.3 Stage 3 (route I, after gate I)

Mouse click sets cursor, dragging marks block (also upwards – in the original
"currently FUCKED", src/mouse.c `mousedown`), click into the label column
(`gomouse`), prop gadgets (`gadget`, busy poll), Amiga shortcuts (Amiga-S,
Amiga-L, Amiga-?), CLOSE WB/OPEN WB.

## 8 Cases with special expectation

Basic rule: `BUG(orig)` is reproduced in C → **A and B equal** (also
equally wrong). Deviations only for the `DIFF:` lines of
[deviations.md](deviations.md) with "observable: yes/only …".

| Case | Source | Expectation in the test |
|---|---|---|
| `tabstop = 0` (DEFAULTS accepts 0, BUG(orig) src/main.c:748) → `divu` by 0 in `readline` | PHASE2B-BRIEF O3; since phase 2c real `divu.w` (src/glue.c `divu_w`) | both: trap (Guru 5, Division by zero) → run CRASH/TIMEOUT. **KNOWN-BOTH**, own run at the end of a session |
| damaged options file, colour value ≥ 4 (`swapd0bord`) | deviations.md "`swapd0bord`" | both swap bytes with the original strides behind `defaultreqbo14`; the data there are other variables in C → consequences may differ. **KNOWN-DIFF**, stage 2 only |
| CLI (`mycli` → `openio`) | since 2c: Z flag of `WBenchToFront` via `callregs` (src/mouse.c) | "equal" |
| `outmem` before the first main loop (`memstack == 0`) | since 2c: `memcont_nostack` (src/glue.c) sets a7 = 0 and runs memcont like the original | both crash (crash location may differ: the called C routines use the stack differently) – stage 2 only, KNOWN-BOTH |
| `join`: FreeMem without `lenitem` (9 bytes per join), `freethis`/`freeall` via `add.b` from `numchars` ≥ 247 | lines.c:407, tokit.c:303 | same memory balance A↔B (leak in both) |
| `join` without length check, `split` appends prefix length (I.3/I.4) | lines.c:382, lines.c:1036 | only with short lines in stage 1; long lines in stage 2 with expectation "equal" (memory overwriting may hit both equally – with CRASH in both: KNOWN-BOTH) |
| `gotolined0` without range check (GOTO/error line ≥ `numlines`, I.13) | lines.c:602, edit.c:110 | not in stage 1; in stage 2 expectation "both equal" (NULL walk) – KNOWN-BOTH |
| ASCII block gets a 0 byte per line (`tokenise` off) | loadsave.c:52 | BLOCK SAVE without tokenising: file contains `text\0\n` in A and B |
| UNTAB deletes arbitrary characters | loadsave.c:102 | file equal in A and B, ≠ "sensible" result |
| `readline`: last line without LF untokenised, byte ≥ `$80` = token also in text | fileio.c:215 | oracle `bb2tok` with the same exception (like `test_loadsave`) |
| block ends at umlaut (`getbchar` N flag) | fileio.c:715 | equal in A and B |
| loading an empty file: old lines lost (no `freeall`) | loadsave.c:237 | memory balance equal |
| memory error while writing: `allalt` stays, `.xtra`/`.info` anyway | loadsave.c:317 | only with a full volume, stage 2 optional |
| executable with "Runtime Errors" | tedstub/README.md "Bekannt" | addresses after `trap #1`/`#4` differ → mask; empty "OKAY!" requester when running (only relevant if programs are started – not here) |
| file requester from 605 lines | reference.md §3.2 | excluded by 640×512; never test screens > 604 lines |
| hourglass animation (`myroutine`, VBlank) | src/main.c | sprite, not in the bitplanes – no influence on dumps |

The report distinguishes: `SAME`, `DIFF`, `KNOWN-DIFF` (line in
deviations.md), `KNOWN-BOTH` (both equally faulty/crashed),
`FAIL`/`TIMEOUT` (run not complete). Expectations are per test in
`tests/emu/NAME.expect` next to the script `tests/emu/NAME.drv` (here `tests/ted-ab/scripts/`).

## 9 Prerequisites and blockers

| # | Point | Kind | What needs to be done |
|---|---|---|---|
| B1 | ~~RTG Workbench of the test configuration~~ | **solved** (27.10.) | the user's `A1200-AB3-AGA.uae` (§3.2), native PAL hires laced Workbench, depth 8; gate 0 checks it in every session (§3.3) |
| B2 | companion process `teddrive` | implemented (route D) | `tests/ted-ab/amiga/teddrive.c`, self-test `teddrivetest` (gate D); `reqclose.h` included unchanged (O-P1) |
| B3 | runner `tedab.py`, decoder `teddump.py`, test scripts | implemented (gate, test 00) | §5, §6.1; host tests `tests/tools/` (ctest host `tedab_tools`, `tdscript`); further `tests/emu/*.drv` after test 00 |
| B4 | mouse position/shortcuts only via input.device, which paralysed the input task in the test emulator (RTG) | risk for stage 3 | gate I on the native screen; stage 1/2 manage without |
| B5 | synchronisation "idle" not tried (TS_WAIT/SigWait) | risk | first try with test 00 on R5 (reference must run reliably), log every wait time |
| B6 | TED-C has never been started | risk | test 00 first with R5 (check the tool), then C5; on crash: Enforcer/MuForce in the test image would be helpful (hits into the log), otherwise session TIMEOUT and log step as a clue |
| B7 | `help`, `help.dat` are not copied by `bb2compile.prepare()` | done | `tedab.py` copies them into every combination |
| B8 | `SYS:L/blitzeditor.opts` on the hardfile unknown | small | gate 0 reports it; `Assign L:` order (§2.2) |
| B9 | ~~O1/O3~~: done in phase 2c (both like the original: crash or trap, §8) | – | – |
| B10 | ~~README~~: updated in phase 2c | – | – |
| B11 | run time: boot ≈ 15 s, TED start and requester timing unknown | planning | measure after test 00, derive session size in `tedab.py` from it |
| B12 | CI: hardfile/ROM require a licence, emulator run only local/self-hosted (ci-exploration.md §2.2/2.3) | framework | phase 3 locally; CI later with the same configuration |

## 10 Procedure (work packages)

1. **Configuration** (B1, done: the user's `A1200-AB3-AGA.uae`),
   `tedprobe` → gate 0 green (native mode, planes in chip RAM, height
   below 605).
2. **teddrive** route D + `dump` + log + `.result`; decision **O-P1**
   (split or copy reqclose.h). Self-test `teddrivetest` (own
   window with gadgets/string gadget, checks RAWKEY/MENUPICK/GADGETUP/`str`/
   `idle`) – gate D, analogous to `reqclosetest`.
3. **tedab.py** (staging, sessions, repetition after hang, report) and
   **teddump.py**.
4. Test 00 with **R5** (tool trial on the original), then **C5**; only then
   01–09 (stage 1).
5. Add Rc/Cc (stage 2a), then the broader tests (stage 2b), `state`
   (§6.2).
6. Gate I (input.device on native screen) → stage 3.
7. Result in [deviations.md](deviations.md) (new observed DIFFs, column
   "observable?" with test number), state in `README.md`.

Decided points (coordinator, 27.10.2026):

* **O-P1**: `reqclose.h` is **not** copied and not split:
  `teddrive.c` includes the file unchanged from
  `compilerlauf/tedstub/src` (here `tests/corpus/amiga/`; one source like `shared/filereq/filereq_core.c`) and
  uses the log buffer (`rc_cat`/`rc_hex`/`rc_dec`), `rc_valid`, `rc_label`,
  `rc_copytitle` and the static message buffers including reply port
  (`rc_msg`/`rc_busy`/`rc_port`/`rc_replies`); the rest (own loop
  of reqclose) stays unused (`-Wno-unused-function`). Sending with
  qualifier (`td_send`) is a variation of `rc_send` in teddrive.c.
  `reqclose` itself is untouched (byte-equal).
* **O-P2**: what counts is the actual Workbench mode (expected
  640×512 laced, depth 8); gate 0 records it, dumps are only compared with
  the same geometry (`teddump.compare`).
* **O-P3**: results in `ted-c/results/ted-ab/` like `compilerlauf/results/`
  – text only (logs, `.result`, decoded dumps, checksums, `report.txt`);
  binary dumps and saved files under `ted-c/build/ted-ab/` (not
  in git).

Implementation and deviations from the plan: [PHASE3-LOG.md](PHASE3-LOG.md).
