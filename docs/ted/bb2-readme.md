# ted-c – TED 1.1 (editor of Blitz Basic 2) in C

C port of `ted.s` (TED 1.1) following the method of the compiler port
`../blitz2-c`. As of: **phase 2c** – all routines translated (2b),
integration review (2c: [notes/review-2c.md](notes/review-2c.md),
comdata interface [notes/comdata.md](notes/comdata.md)); host and
Amiga checks green.  **Phase 3** (emulator A/B tests,
[notes/PHASE3-PLAN.md](notes/PHASE3-PLAN.md), log
[notes/PHASE3-LOG.md](notes/PHASE3-LOG.md)): stage 1 completed – all
13 tests (start, round trips, typing, join, block, search/replace,
compiling, error line, `.xtra`, options) equal with TED-C and reference TED
under blitz2 1.95; stage 2a completed – the same tests with
blitz2-c (Rc/Cc), all 52 comparisons equal (`results/ted-ab/report.txt`);
next up stage 2b (broader tests).

```
cmake --preset amiga && cmake --build --preset amiga && ctest --preset amiga
#  -> build/amiga/ted, ted.sym (C build)
#  -> build/amiga/ref/ted, ted.lst, ted.map, ted.labels  (vasm build of ted.s = reference)
#  ctest: ref_verify, checkhunk, checkdata, checklabels, checklink
cmake --preset host && cmake --build --preset host && ctest --preset host
#  -> core library (src/core) + unit tests on the host (fixtures from the
#     CD sources and bb2tok.py in build/host/fixtures)
python3 -I tools/tedab.py gate         # phase 3: prepare gate session (without --go nothing starts)
python3 -I tools/tedab.py plan --test '0*'               # sessions (each ≤ 50 s)
python3 -I tools/tedab.py run --test 00-start [--go]    # one session, without window (tools/amiberry.cmake)
python3 -I tools/tedab.py report --all                  # report over all sessions
```

| File | Contents |
|---|---|
| `notes/reference.md` | reference build, tools, binary includes (rawed.obj, filereq, data) and decisions |
| `notes/overview.md` | chunks, architecture, cross-section of the surveys, risks |
| `notes/survey-01…07.md` | interface survey per chunk |
| `notes/xrefs.md` | cross-references between the chunks (`tools/xrefs.py`) |
| `notes/PLAN.md` | file layout, headers, glue, host core, verification |
| `notes/SURVEY-BRIEF.md` | brief of the surveys |
| `notes/CONVENTIONS.md` | translation conventions (phase 2b) |
| `notes/PHASE2B-BRIEF.md` | brief phase 2b: ownership, check, split, open decisions |
| `notes/deviations.md` | all `DIFF:` locations |
| `notes/changes.md` | changes to shared headers |
| `notes/comdata.md` | register interface comdata/callbacks: original ↔ glue ↔ C |
| `notes/review-2c.md` | phase 2c: sample at instruction level, results |
| `notes/PHASE3-PLAN.md` | plan of the emulator A/B tests |
| `notes/PHASE3-LOG.md` | log phase 3 (tools, emulator runs, deviations from the plan) |

| Directory | Contents |
|---|---|
| `src/core/` | core without AmigaOS headers (also host): `tedtypes.h`, `tedcore.h`, `hooks.h`, `state.h/.c`, per chunk `loadsave`, `lines`, `services`, `fileio`, `tokit`, `lineedit`, `keymap` |
| `src/` | Amiga part: `ted.h`, `structs.h`, `glue.h`, `glue_start.c`, `glue.c`, per chunk `main`, `edit`, `mouse`, `reqs`, `screen`, `input`, `data`, plus `rawkey`, `filereq` (core `../shared/filereq/filereq_core.c`, environment `filereq_env.h`) |
| `tests/host/` | host tests (`test_*.c`), stubs of the hooks, fixture generation |
| `tests/amiga/` | phase 3: `teddrive` (driver, route D), `teddrivetest` (gate D), `tedprobe` (gate 0), `tdscript.h` (script syntax) |
| `tests/emu/` | phase 3: test scripts `*.drv` (teddrive) and gate scripts |
| `tests/tools/` | host tests of `tedab.py`/`teddump.py` and the script syntax |
| `results/ted-ab/` | phase 3: results (text only; binary files under `build/ted-ab/`) |
| `tools/` | `tedlisting.py`, `xrefs.py`, `checkhunk.py`, `checkdata.py`, `checklabels.py`, `checklink.cmake`; phase 3: `tedab.py` (runner), `teddump.py` (screen dumps) |
