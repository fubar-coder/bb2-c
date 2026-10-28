# Phase 3 – work log (emulator A/B tests)

Running log of the implementation of [PHASE3-PLAN.md](PHASE3-PLAN.md).
Rules for the emulator (coordinator): one emulator at a time
(`bb2compile.LOCK`), one session for many tests (60 s, at most 90 s,
otherwise split), nothing may block (every wait point with a time limit,
inputs synthetic), every driver with its own self-test gate up front,
all host checks before every run, **no emulator start without approval
by the coordinator**, new drivers at first only with the gates.

## 27.10.2026 – tools (host only, no emulator run)

Starting point checked: `ctest --preset amiga` 5/5, `ctest --preset host`
11/11 green.

### Gate 0: `tedprobe` (`tests/amiga/tedprobe.c`)

`tedprobe OUTPUT` writes "key value" lines: system versions,
`GetScreenData(84 bytes, WBENCHSCREEN)` like TED's `calcscreen` and the
TED geometry resulting from it (`ted_edrows_f8/f6`, `ted_height_f8/f6`),
Workbench screen (`GetVPModeID`, mode name, `BMA_DEPTH`, `BMA_FLAGS`,
`BytesPerRow`, `TypeOfMem(Planes[0])`, interleaved), free memory,
`SYS:L/blitzeditor.opts` and `L:blitzeditor.opts`, stack, keyboard layout
and `MapANSI` of a sample. Last line `gate0 OK` (rc 0) or
`gate0 FAIL` (rc 5, WARN): planes not in chip RAM, depth < 2, TED's
screen taller than 604 lines, Workbench narrower than 640.

### `teddrive` (route D) and gate D

* `tests/amiga/teddrive.c`: `teddrive SCRIPT PREFIX COMMAND [ARGS]`, flow
  and script language in the header comment. The main process starts the command with
  `SystemTags`, a second process plays the script. Program screen =
  first new screen with a backdrop window; target of every input = youngest
  open requester window, otherwise the main window (TED's `pickwindow`).
  "idle" = task of the target port in `TS_WAIT` with the port signal in
  `tc_SigWait`, port empty, all own messages answered, three times
  at a 20 ms interval. Every wait point has a time limit (`timeout`,
  default 15 s); on an error immediately `FAIL step reason` into the
  result file, then the rescue sequence (at most 8 rounds/15 s:
  main window → menu QUIT, file requester → Cancel/CLOSEWINDOW,
  TED requester → gadget 1 as ENDGADGET, otherwise Esc).
* **O-P1** (decision of the coordinator): `reqclose.h` included
  unchanged (include path `compilerlauf/tedstub/src`, CMake cache
  `TEDSTUB_SRC`), no copy, no split. `reqclose` is untouched.
* `tests/amiga/tdscript.h`: the pure script syntax (words, strings with
  `\n \t \xHH`, numbers, label comparison) without AmigaOS headers – compiled on the
  host (`tests/tools/tdscript_host.c`, ctest `tdscript`) and
  checked against `tedab.split_line` (same words for every line of every
  `.drv` file).
* `tests/amiga/teddrivetest.c` (gate D): reads inputs like TED (own
  screen with `SA_LikeWorkbench` + depth 2, backdrop main window with menu,
  requesters as title-less windows, `Wait` only on the port of the youngest
  window, reply before processing, keys via `RawKeyConvert`).
  Two scripts: `tests/emu/gate-d.drv` (RAWKEY, `type` with MapANSI,
  MENUPICK, `wait req`, `str` text and LONGINT, GADGETUP, `wait gone`,
  busy step 1 s – no input may come during that time –, a
  wait point that must expire (`expectfail`), two dumps with a
  rectangle in colour 3, QUIT) and `tests/emu/gate-d-rescue.drv` (fails
  deliberately in step 6, the rescue must end the program).
  Self-limited by a 20 s timer.

### `tedab.py`, `teddump.py`

* `tools/teddump.py`: read/write dump format, TED geometry from the
  screen height, text decoding of the editor area (character + attribute:
  colour 1–3, inverse a–c) and of the status line (plane 1 only, `qprint`),
  mask "Free" (`statsloc+38`, 5 characters), comparison (SAME/DIFF with
  text diff), PNG (standard library only).
* `tools/tedab.py`: `lint`, `plan`, `gate`, `run`, `report`. **Without `--go`
  nothing starts** (only staging + print script). With `--go` first the
  host checks (build and ctest of both presets, lint), then **exactly one**
  session under `bb2compile.LOCK`, `--timeout` 60 (above 90 refused),
  `AMIBERRY_CONFIG` = `A1200-AB3-AGA.uae`. Session script: assign `TEDAB:`
  (short lines, shell ≤ 255 characters), **reboot lock** (`TEDAB:booted`:
  a crash with reboot would otherwise run the script a second
  time), gate 0 → gate D (main, rescue) → runs; every gate with
  `If WARN`/`If VAL $tdg GT 0` before the rest.
* Host tests `tests/tools/test_teddump.py`, `test_tedab.py` with
  artificial data (dumps with real TED fonts, gate results,
  session log, A/B comparison, staging into a temp directory); ctest
  host `tedab_tools`. State: `ctest --preset host` 13/13, `ctest --preset
  amiga` 5/5 green; `teddrive`, `teddrivetest`, `tedprobe` without warnings.

### Bugs of the new tools found and fixed in the host test

* teddrive: script loop aborted at the first empty line.
* teddrive: `\x` without two hex digits skipped the closing `"`
  (self-test `tdscript`); now C and Python equal: only `\xHH` with two
  hex digits.
* teddrive: reply port was deleted even with an unanswered message –
  now it stays in that case (log line).
* tedprobe: the `MapANSI` buffer length is in bytes (autodoc), not in pairs.
* teddump: the space glyph was decoded as a control character (26 empty
  glyphs in the font) – printable codes now take precedence.

### Deviations from the plan (§4/§5)

| Plan | implemented | Reason |
|---|---|---|
| script/log via `ENV:TEDDRIVE`/`ENV:TEDDRIVELOG` | arguments `SCRIPT PREFIX` | no global variables to clean up between runs |
| AutoRequest patch in teddrive | `reqtrap teddrive …` (unchanged `reqtrap`) | proven code without copy; `REQUESTER:` is in `.stdout` instead of the teddrive log |
| `tedprobe` part of teddrive | own program | gate 0 without driver; the dump header contains the same screen fields |
| idle at a 40 ms interval | 20 ms (`Delay(1)`) | typing: per key reply + 3 idle checks |
| `Copy orig → work` in the emulator | working copies prepared by the host, one directory per run | fewer commands in the session |
| `joyport0=none`, `chipmem_size=4` | user's configuration unchanged (`mouse`, 8) | requirement: do not change the configuration; risk host mouse click (§3.2) |

Open for test 00: label of the version requester (according to the tedstub log
`" OKEE DOKEE "`, comparison without spaces at the edge), duration per run
(estimate 12 s).

## Emulator runs

### Run 1 (27.10.2026): gates only – `tedab.py gate --go`

Approval by the coordinator, run exactly once. Emulator rc 0,
session log: `SESSION start, GATE0 OK, GATED OK, SESSION end`.
Results: `results/ted-ab/` (gate/, session.log, report.txt),
dumps `build/ted-ab/gd.gd-*.scr`.

* **Gate 0 OK**: Kickstart/Workbench 47.13 (OS 3.2), AGA (`ChipRevBits0`
  $1F), PAL. Workbench 640×512, **depth 8**, mode ID $00029004
  "PAL:HighRes Interlace", planes in chip RAM (`TypeOfMem` $703).
  TED geometry: screen 640×509, `edcols` 78, `edrows` 60 (font 8) or
  81 / height 513 (font 6) – both below 605. Chip free 3.8 MB, fast 133 MB.
  `SYS:L/blitzeditor.opts` and `L:blitzeditor.opts` do not exist (tests
  without their own options file start with `defstart`). Stack 40000 arrives in the
  program. Keyboard layout `d`; MapANSI needs one key for all
  samples.
* **Gate D main OK** (result `OK`, 10/10 events "expected", 0
  deviations, 5.2 s): RAWKEY, `type "Ab1\n"` → RawKeyConvert "Ab1\r"
  (shift qualifier from MapANSI), MENUPICK, `str` text/LONGINT, GADGETUP,
  `wait gone`, busy step (return after 1.1 s, no input
  in between), `expectfail` after 2 s, dumps 640×512×2 with rectangle.
  Times: key ≈ 0.16 s, menu/gadget ≈ 0.16 s, `wait screen` 0.22 s.
* **Gate D rescue OK** (result `FAIL step 6 timeout`, rescue: gadget 1,
  menu QUIT, "rescue: program ended", 2.9 s).

Surprises / findings:

* **Interleaved bitmaps**: the Workbench bitmap is interleaved
  (`BytesPerRow` 640 = 80 × 8), and so is the screen cloned with `SA_LikeWorkbench` + depth 2
  (`bytesperrow` 160, `interleaved 1` in the dump).
  TED computes with `bmapmul = BytesPerRow` and `Planes[0/1]`; this fits
  interleaved (line distance 160, blitter modulo `BytesPerRow − edcols`),
  but is an environment that TED never saw in 1991. Applies equally to R5 and
  C5 (no A/B difference), dumps compare only the same
  geometry. To be kept in mind with picture errors in test 00.
* `cybergraphics.library` is loaded (OS 3.2 hardfile), although no
  RTG card is configured; without effect on the native Workbench.
* The main window of teddrivetest was logged with `idcmp $00000000`,
  although messages arrived: the snapshot fell into
  `OpenWindowTags` (window already in the list, IDCMP not yet set).
  Log only, no effect.
* Evaluation error in `tedab.py` (not in the emulator): `eval_gate` looked for
  the dumps in `results/` instead of `build/ted-ab/` and reported "gates
  FAILED"; fixed, report regenerated (`gates OK`). Log line of
  `expectfail` without a space before the time – fixed.

Afterwards (host only): on success teddrive additionally writes `PREFIXok`;
`tedab.py` runs the further combinations of a test only if the
first (reference) was OK (`If EXISTS …/ok`, otherwise `RUN … skipped`); test 00
with `timeout 10`. ctest amiga 5/5, host 13/13.

### Run 2 (27.10.2026): test 00 – `tedab.py run --test 00-start --go`

Approval by the coordinator, exactly once. Emulator rc 0, session ≈ 26 s
(with boot). Gates green again (teddrive with `ok` marker). **R5 OK, C5 OK**
– TED-C ran for the first time: blitz2 loads it, version requester
(gadget 1 " OKEE DOKEE "), screen "Blitz Basic 2 - Ted Version 1.1"
640×512×2, source loaded and displayed, QUIT without confirmation, blitz2 rc 0
in both. Steps in the teddrive log: version requester after 0.30 s,
screen idle 0.58 s, QUIT until program end 0.74 s (R5 and C5 equal).

Decoded start dump (`results/ted-ab/runs/00-start/*/start.txt`):
source text accurate to the line, keywords (tokens) in colour 3, status line
"Line:1 Column:1 Largest Mem (K):…". The display on the
interleaved screen is correct (finding from run 1 without consequences).

A/B: **DIFF, 22 bytes – only the screen title**: it contains the path of the
loaded file, and that differed per combination
(`TEDAB:runs/00-start/R5/…` vs. `…/C5/…`). Status line "Largest Mem"
different (63721 vs. 63829 K) – covered by the mask "Free".
Otherwise byte-equal. Requester sequence equal, rc equal. The window flags of the
version requester in the log ($300A vs. $1300A, `WFLG_RMBTRAP`) are a
timing artefact of the snapshot: both TEDs set the bit directly
after `openreq` (`menusoff`, ted.s:272/276); no difference in behaviour.

Fixed (host only): every run gets its directory as `TEDRUN:`
(same paths in all combinations, hence the same title);
`collect` no longer deletes binary files of earlier tests.

### Preparation of stage 1 (host only)

* Test scripts `tests/emu/01a…01d` (round trip: t01_int, t08_tokquirk,
  CD `finish_mouse.bb`, `mikesmaplib.bb`), `02-type`, `03-join`,
  `04-block`, `05-find`, `06-compile`, `07-error`, `08-xtra`
  (`let.bb2` with its own `.xtra`), `09-opts` (without file, DEFAULTS,
  tab stop gadget 6 = 8, OK = gadget 26, `savedefs` to `TEDRUN:L`).
  Gadget IDs/labels from `src/data.c` (findreq 1–8, repall 1–3,
  erasereq/defaultreq " OK "/" CANCEL ", errorreq " OKAY ").
* teddrive: `freq ok "NAME" "PATH"` (path gadget = topmost
  string gadget). Needed for 06/07, because the presetting of blitz2's
  file requester is unknown; the position of the freq gadgets is checked before the
  session with 06/07 in the log of test 01.
* tedab: `@roundtrip` (saved file and `.bak` = original), estimate
  per run from the measurements (3–10 s instead of 10–12 s); all runs of a
  session depend on the success of the first run (tool problem → rest
  skipped instead of watchdog), further combinations of a test on the
  success of the reference. Plan stage 1: 4 sessions (~49/55/59/53 s).

### Run 3 (27.10.2026): stage 1, session 1 – round trips 01a–01d

`tedab.py run --test '0[1-9]*' --session 1 --go`, approval by the
coordinator, exactly once. Emulator rc 0, 46.6 s with boot. Gates green.
CD sources staged only as copies (md5 of the CD files before/after
equal).

* **01a (t01_int), 01b (t08_tokquirk), 01c (CD finish_mouse.bb): R5 OK,
  C5 OK, R5↔C5 SAME** – saved file, `.bak`, `.info` (396 bytes,
  `srcinfo`), `.xtra` (1206 bytes, rewritten by blitz2 `savextra`),
  dumps "start"/"saved", requester sequence, rc. Oracle `@roundtrip`:
  saved file and `.bak` = original in all six runs.
  Run ≈ 1.7 s (teddrive time), of which file requester 0.4 s.
* File requester "Name to save file as": 288×472 on TED's screen, 10
  gadgets; OK/DRIVES/PARENT/CD/CANCEL, string gadgets anchored at the bottom
  (`GFLG_RELBOTTOM`): "PATH:" at −49, file name at −33. teddrive's
  assignment (path = topmost, file = bottommost string gadget) is correct –
  `freq ok "NAME" "PATH"` for 06/07 is thus confirmed.
* **01d (CD mikesmaplib.bb): R5 FAIL step 4 timeout, C5 skipped.**
  Cause in the test script, not in TED: the `.xtra` names residents
  `blitzlibs:libmacs.res`/`blitzoffs.res`; `blitzlibs:` does not exist →
  two DOS requesters "Nochmal versuchen|Abbrechen" (answered by reqtrap with
  "Abbrechen", `REQUESTER:` in stdout) and two blitz2
  error requesters " OKAY " that the script did not expect. The rescue
  answered both, afterwards TED reopened its screen (different
  geometry: label column, prop gadget at x 531) and ended with QUIT.
  Script adapted (twice `wait req "OKAY"` / `gad 1`); 01d stays as
  round trip with error path (comdata `error`, `loadxtra`).

Host only afterwards: `--test` takes a list of patterns; estimates after
measurement (01/08: 3 s, 02: 9 s, 03: 4 s). Next session: 01d, 02, 03
(~57 s planned).

### Runs 4 and 5 (27.10.2026): session A (01d + 03) and B (02)

Requirement of the coordinator: planned session length ≤ 50 s (`PLAN_LIMIT` in
tedab.py, planner budget 50 − boot − gates). Both sessions started once
each; reports under `results/ted-ab/sessions/`.

* **A (37.8 s): 01d R5/C5 OK, SAME; 03 R5/C5 OK, SAME.** 01d with the two
  expected OKAY requesters (missing residents) and the rebuild of the
  screen, round-trip oracle fulfilled. 03: split and JOIN yield the
  original file again (saved file = `.bak`), cursor afterwards line 4.
  Memory before/after: −8 bytes in R5 **and** C5 (join leak BUG(orig),
  9 bytes FreeMem without `lenitem`, rounded to 8) – equal, as expected.
* **B (18.7 s): gate D FAIL, test 02 not run.** teddrivetest
  received, instead of the synthetic inputs, ten times real RAWKEY $21 ("s")
  with qualifier $0200 (`IEQUALIFIER_REPEAT`): a key held in the emulator
  with autorepeat. teddrive itself worked correctly (program ended
  via the foreign inputs, "program exited"), the gate blocked the rest as
  intended. Cause presumably host keyboard: Amiberry grabs
  the focus on start; if someone types at that moment, the key stays pressed in the
  emulator (no key-up). Not a tool or TED error.
  Remedy (host only): tedprobe reads the keyboard matrix
  (`keyboard.device` `KBD_READMATRIX`, line `keys_down`) and lets gate 0
  fail while a key is pressed – then no input to teddrivetest or TED
  begins. Please do not type during the sessions.

### Headless emulator (coordinator, 27.10.2026)

From now on Amiberry runs by default in a nested virtual
KWin (`kwin_wayland --virtual --socket tedab-PID --width 1280 --height 1024
--exit-with-session inner.sh`; in the inner script `DISPLAY`,
`XDG_ACTIVATION_TOKEN`, `DESKTOP_STARTUP_ID` removed,
`SDL_VIDEO_DRIVER=wayland`, rc of amiberry_run into a file, because kwin's
rc is not the inner one). No focus stealing, no host inputs. Outer
time limit = session timeout + 20 s, afterwards the process group is
terminated. `--visible` = old way. Implemented in `tedab.py`
(`run_emulator`, `inner_script`, `kwin_command`; files under
`build/ted-ab-run/`), `../ab3c` and `compilerlauf/tools` unchanged. The
keyboard matrix check in gate 0 stays.

### Runs 6–8 (27.10.2026): sessions C (02), D (04, 05), E (06) – headless

Once each, emulator rc 0, gates green, `keys_down` empty.

* **C (43.5 s): 02 R5/C5 OK, SAME.** TED's Return does not split the line,
  but opens a new line below it (the text typed first is
  before the old line 0) – equal in both; test description corrected.
  Oracle: saved file = `bb2tok.tokenise` of the expected text
  including the empty line after the last Return (949 bytes, R5 and
  C5). Typing ≈ 0.16 s per key, run 8.4 s.
  Addendum to 03: there too Return opened an empty line, JOIN removed
  it again – hence result = original.
* **D (41.2 s): 04 R5/C5 OK, SAME; 05 R5/C5 OK, SAME.** 04: block lines
  1–3 marked inverse, COPY + KILL move it behind "Dim m.l(3,4)".
  05: find requester (8 gadgets), NEXT, REPLACE, REPLACE ALL with
  repall requester "FROM TOP" – all `NPrint` → `Print` (same
  file size, both tokens).
* **E (33.4 s): 06 R5/C5 OK, SAME.** blitz2's file requester "Name of
  Executable to create", `freq ok "t01_int" "TEDRUN:work"`, COMPILING
  requester, back in the editor after 1.4 s. Executable 10680 bytes in R5 and
  C5 **byte-equal with `compilerlauf/results/corpus-ref/t01_int`**
  (blitz2 1.95 + tedstub, without runtime errors). Plus `.info` of the
  executable (383 bytes) and newly written `.xtra` (1226 bytes), both
  A↔B equal.

### Run 9 (27.10.2026): session F (07, 08) – headless

35.1 s, rc 0, gates green. **07 R5/C5 OK, SAME; 08 R5/C5 OK, SAME** – but:

* 07: error requester " OKAY " after compiling, afterwards the
  cursor (inverse cell) is on line 2 (`b=(a+`) – correct. The status line
  still shows "Line:1": TED does not update it after the jump to the
  error line. Equal in R5 and C5 (behaviour of the original, no
  DIFF).
* **08: nothing was saved** – neither `let2.bb2` nor `.bak`/`.info`,
  `.xtra` unchanged (savextra not called), in R5 **and** C5; no
  error requester (so no `doopen` error) → the file requester apparently returned
  "cancel" or a different name. Difference to 01a (saving
  worked): teddrive wrote the name into the file name gadget, which the
  requester activates on opening with `ActivateGadget` (the string gadgets
  of gate D are never active). Tool error (test oracle not fulfilled) →
  stop as required, session G not started.
  Host only: teddrive now logs the buffers of all string gadgets
  (on a new window, after `str`, before the button in `freq`, with "(active)"
  for `GFLG_SELECTED`); 08 makes a dump after saving (title =
  file name). Next session: 00 (repetition with `TEDRUN:`), 08
  (diagnosis), 09.

### Expectations on the host (coordinator: "result OK alone is not a pass")

`tedab.py`: `; @expect KIND ARGS` (`same`, `bak`, `exists`, `absent`,
`tok NAME EXPECTFILE` = `bb2tok.tokenise`, `equals NAME PATH`, `lexists`),
`@roundtrip` = `same` + `bak`; a test without `@roundtrip`/`@expect`/
`@nofiles` is rejected. A run is only passed if teddrive reports OK
**and** all expectations hold, otherwise `FAIL (expectations)`.
Expectations entered for all tests (01: file, `.bak`, `.info`,
`.xtra`; 02/04/05: text oracle `tests/emu/0x-*.expect` via bb2tok; 03:
= original; 06: executable = `compilerlauf/results/corpus-ref/t01_int`;
07: no executable, source unchanged; 08: `let2.bb2` = `let.bb2`,
`.xtra`/`.info` present; 09: `L/blitzeditor.opts`). Checked against the
existing results 01–07: all fulfilled; the old 08 becomes FAIL.

### Run 10 (27.10.2026): session H (00, 08, 09) – headless

35.4 s, rc 0, gates green.

* **00 R5/C5 OK, SAME** – with `TEDRUN:` the screen title is equal too.
* **09 R5/C5 OK, SAME** – DEFAULTS (screen is closed/reopened,
  teddrive follows), gadget 6 (previously "2") = "8", OK; `L/blitzeditor.opts`
  50 bytes, equal in R5 and C5.
* **08 R5/C5 FAIL (expectations)** (A/B equal). Diagnosis from the new
  log: TED presets the file name gadget with the **full** name
  `TEDRUN:work/let.bb2`, `PATH:` is empty (the requester returns
  PATH + name). `freq ok "let2.bb2"` replaced this with a relative
  name; TED saved `let2.bb2` (+ `.xtra`, `.info`) **relative to the
  current directory** of the session script (title "File: let2.bb2") –
  not found on any host drive, so in the emulator, presumably `SYS:`
  of the user's hardfile (sessions F and H, same name, overwritten).
  Tool error with side effect on the hardfile – reported.
  Fixed (host): 08 with full name; lint forbids `freq ok NAME` with
  a relative name without PATH; every run with `CD TEDRUN:work` (relative
  paths end up in the captured run directory), afterwards `CD TEDAB:`; the session
  writes its start directory to `session.cd`. Cleanup as a
  one-off prologue `tests/emu/cleanup-let2.ados` (`--pre`): deletes only
  `let2.bb2`, `let2.bb2.xtra`, `let2.bb2.info` relative to the start directory
  (= where TED wrote them), list before/after to `cleanup.txt` – only with
  approval.

### Run 11 (27.10.2026): session I (08 + location) – headless

Requirement: **delete nothing** (the hardfile belongs to the user). Prologue
read-only (`tests/emu/locate-let2.ados` via `--pre`: `CD` and
`List let2.bb2#? QUICK` to `cleanup.txt`).

* **08 R5/C5 OK, SAME, expectations fulfilled**: `let2.bb2` = `let.bb2`,
  `let2.bb2.xtra` and `.info` written (with the full name
  `TEDRUN:work/let2.bb2` in the file name gadget).
* `cleanup.txt`: the start directory of the session is **`Workbench:`** (=
  `SYS:` of the user's hardfile `Workbench_OS3.2-AB3-AGA.vhd`); there are
  four files from sessions F and H: `let2.bb2`, `let2.bb2.bak`
  (H renamed F's file), `let2.bb2.xtra`, `let2.bb2.info` (together
  1 KB, 10 blocks). Not deleted – decision of the user.

## State after stage 1 (27.10.2026)

Consolidated report `results/ted-ab/report.txt` (`tedab.py report
--all` over `results/ted-ab/sessions/*.session.log`; the log of the first
stage 1 session `s0-01` restored from the output of the time, since
archiving was only introduced afterwards):

| Test | R5 | C5 | R5↔C5 | Oracle |
|---|---|---|---|---|
| 00 Start/exit | OK | OK | SAME | – (`@nofiles`) |
| 01a–01d Round trip (2 corpus, 2 CD sources, one with missing residents) | OK | OK | SAME | file and `.bak` = original, `.info`/`.xtra` present |
| 02 Typing | OK | OK | SAME | = bb2tok(expected text) |
| 03 Return + JOIN | OK | OK | SAME | = original; join leak −8 bytes in both |
| 04 Block | OK | OK | SAME | = bb2tok(block moved) |
| 05 Search/replace | OK | OK | SAME | = bb2tok(all `NPrint` → `Print`) |
| 06 Compiling | OK | OK | SAME | executable = `compilerlauf/results/corpus-ref/t01_int` |
| 07 Error line | OK | OK | SAME | no executable; cursor line 2 (status line stays "Line:1" – original) |
| 08 `.xtra` | OK | OK | SAME | `let2.bb2` = `let.bb2`, `.xtra`/`.info` |
| 09 Options | OK | OK | SAME | `L/blitzeditor.opts` (50 bytes) |

Result: **TED-C behaves observably like the reference in all stage 1 tests**
(screen byte-equal except for the masked memory display,
files byte-equal, requester sequence and rc equal). No new line for
`abweichungen.md`. Observed original behaviour (equal in both):
Return does not split a line, but opens a new one below; the
status line is not updated after the jump to the error line;
file name gadget preset with the full name, `PATH:` empty.

Tool state: 11 emulator sessions, all planned ≤ 50 s (actually
19–47 s), no hangs, no crash. One session blocked by a held
host key (gate D) – since then headless in the nested KWin and
keyboard matrix check in gate 0. One tool error with a side effect
(relative file name in 08 → four files in `Workbench:`), fixed; the
files are still there.

Open for stage 2a: Rc/Cc (blitz2-c with both TEDs).

## Stage 2a (27.10.2026): Rc/Cc – blitz2-c with both TEDs, headless

Preparation (host only): default combinations of a test = all four
(`--combos` selects, in this order: reference first, `Rc` before
`Cc`); every session archives log and report itself to
`results/ted-ab/sessions/sNN-…` (archive names renamed to two digits,
so that `report --all` reads in the right order). blitz2-c:
`build/amiga/blitz2` from the checked-in state (`ninja: no work to do`).

Six sessions (`--combos Rc,Cc --session 1…6`), once each, each planned ≤ 49 s,
all gates green, no hangs:

| Session | Tests | Rc | Cc | Rc↔Cc |
|---|---|---|---|---|
| s09 | 00, 01a, 01b, 01c | OK | OK | SAME |
| s10 | 01d, 02 | OK | OK | SAME |
| s11 | 03, 04 | OK | OK | SAME |
| s12 | 05, 06 | OK | OK | SAME |
| s13 | 07, 08 | OK | OK | SAME |
| s14 | 09 | OK | OK | SAME |

All expectations fulfilled – also 06: the executable generated via TED's menu with **blitz2-c**
is byte-equal with `corpus-ref/t01_int` (blitz2 1.95).

Consolidated (`report --all`, all four combinations): at first DIFF in
R5↔Rc and C5↔Cc for 01–07 – evaluation artefact: since
session H teddrive appends ` buffer "…"` to string gadget lines, `requesters()` took
that as a label (the stage 1 logs 01–07 come from the older teddrive).
Parser corrected (label = first "…" after the size, buffer is not
compared, host test for it). Afterwards **52 of 52 comparisons SAME** (13
tests × R5↔C5, Rc↔Cc, R5↔Rc, C5↔Cc).

Result of stage 2a: TED-C and reference TED behave equally with
blitz2-c too; blitz2-c behaves with both TEDs like blitz2 1.95
(screen, files, requester sequence, rc, executable). The two
`-fbaserel` programs each with its own a4 (overview.md §4 risk 2) show
no problem in these tests.

## Stage 2b (27.10.2026) – headless, R5/C5

Preparation (host only):
* `tests/emu/gen2b.py` generates the families: `20a–20i` round trips over
  CD sources (tokenised, `.xtra` without residents, 96 bytes to 120 KB),
  `21a/21b` text sources (= `build/host/fixtures/tok/716/717.bb2` of the
  host test), `29-cmp-*` compiling t02–t13 via the menu
  (= `compilerlauf/results/corpus-ref`). By hand: `22-scroll` (BOTTOM,
  TOP, 62× down, GOTO 30), `23-font6`, `24-blockfile` (BLOCK
  SAVE/LOAD), `25-tab` (TAB/UNTAB/MARK INDENT), `26-newload` (NEW with
  erase requester, LOAD), `27-help` (Help key), `28-long` (line > 127
  characters, umlauts, $FD–$FF, typed umlauts; source
  `tests/emu/src/long_umlaut.bas`, source kind `E`).
* tedab: `@opts` (stage options file to `L/`), expectation `opts`
  (contents of the options file; generator checked against TED's own file from 09:
  byte-equal), `@font 6`, planner refuses sessions over
  50 s; lint in the preflight reads the `.drv` as Latin-1 (previously
  `--go` aborted on UTF-8 reading of `28-long.drv`, without emulator start).
* **09 corrected**: TED takes over the number fields of the DEFAULTS
  requester only on the GADGETUP of the string gadget itself (like Return; in
  `defaults()` OK = gadget 26 leaves the loop before `tabstop =
  int3`, asm and C equal). Stage 1 had therefore saved tabstop 2
  (equal in R5 and C5, SAME, but test goal missed – the weak
  expectation `lexists` did not notice it). Now `gad 6` after `str`,
  expectation `opts … tabstop=8`.

Sessions (`--test '09*,2*' --combos R5,C5 --session N`):

| Session | Tests | R5 | C5 | R5↔C5 |
|---|---|---|---|---|
| s15 | 09 (new), 20a, 20b | OK | OK | SAME (09: options file with tabstop 8) |
| s16 | 20c–20f | OK | OK | SAME |
| s17 | 20g, 20h | OK | OK | SAME |
| s18 | 20i (120 KB, 4762 lines), 21a | OK | OK | SAME |
| s19 | 21b | OK | OK | SAME |
| s20 | 22 | OK | OK | SAME |
| s21 | 23, 24 | **CRASH** (R5) | not run | – |

**s21: the reference TED (R5) crashed on start with the options file
`fonthite 6`** – before the version requester. teddrive stdout:
`REQUESTER: EasyRequest title "Software-Fehler" … gadgets "Später
neustarten|Gleich neustarten" -> answered 0` (reqtrap: 0 = right button
= reboot). The emulator rebooted, the reboot lock took effect
("SESSION second boot - skipped"), session ended cleanly (rc 0), C5 and
24 not run. Stop as required.

Hypothesis (not confirmed): with font 6 TED's `calcscreen` yields a
screen of **513** lines (81 × 6 + 6 + 9 + 12, gate 0
`ted_height_f6 513`), the Workbench has **512**. If OS 3.2 limits the
cloned screen to 512 lines, `OpenWindow` of the
main window fails (top 12 + height 501 = 513) → `wfail` → `cfail` = known
BUG(orig) survey-05 I.1 (O2: reproduced): `jsr closescreen(a6)` with
a6 = ExecBase calls `Exception()` → crash. Then the case would be
KNOWN-BOTH (TED-C reproduces the bug deliberately).

### Emulator without window instead of nested KWin (coordinator, 27.10.2026)

The KWin route (section "Headless emulator") is replaced. Amiberry 8.4
(SDL3) runs with `SDL_VIDEO_DRIVER=offscreen`, `SDL_AUDIO_DRIVER=dummy`
and without `DISPLAY`/`WAYLAND_DISPLAY` entirely without a window; focus stealing and
host inputs are thus ruled out, a compositor and the watchdogs
for the Plasma session are no longer needed. Shared for all tools in
`bb2/tools/amiberry.py`: derived configuration under
`build/amiberry/amiberry.uae` (Amiberry directory relocated,
`use_gui=no`, `sound_output=none`), own XDG directories, outer
time limit + 20 s with termination of the process group. Check run: `tedab.py gate
--go` with `A1200-AB3-AGA.uae` – gate 0 OK (640×512, depth 8, mode
$00029004, chip planes), gate D main and rescue OK.

### Emulator run as CMake script with `uaequit` (coordinator, 27.10.2026)

`bb2/tools/amiberry.cmake` replaces `../ab3c/tools/amiberry_run.py` and the
Python part of `tools/amiberry.py` (now only a call of the script).
The session script ends with `AB3C:build/amiberry/bb2-tools/uaequit`
(`tools/uaequit.c`, built on the first run): uaelib trap `$F0FF60`,
function 13 `ExitEmu`. Amiberry terminates itself shortly afterwards (the call
still returns, hence as the last command); `execute_process(TIMEOUT)`
terminates hangs. Check runs: `amiga_test.py` (7 test cases), `bb2compile.py
--run t01_int`, `tedab.py gate --go` (gate 0 and D OK), `serialrun.py`
(TCP port via `OPTIONS`, `t64_serialrx` equal).

### `sound_output=interrupts` instead of `none` (coordinator, 27.10.2026)

With the emulator without window, in every session every TED run after the
first hung at step 4 (`wait screen` after "OKEE DOKEE": the start requester
did not close) – with the original TED as with TED-C, reproducible
(session `01c`, `06`, `08` with R5/C5: only `01c`/R5 OK). The cause was
`sound_output=none` in the derived configuration (presumably without
emulated audio interrupts). With `sound_output=normal` and with
`sound_output=interrupts` (interrupts emulated, no output; together with
`SDL_AUDIO_DRIVER=dummy`) all six runs are OK. `tools/amiberry.cmake`
now sets `interrupts`. The corpus (`corpus_check`, 201 checks) was
not affected by this.

