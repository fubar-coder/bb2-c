# ted-c – TED 1.1 (editor of Blitz Basic 2) in C

C port of `ted.s` (TED 1.1, here `programs/ted/`) following the method of the compiler port
`programs/blitz2`. As of: **phase 2c** – all routines translated (2b),
integration review (2c: [review-2c.md](review-2c.md),
comdata interface [comdata.md](comdata.md)); host and
Amiga checks green.  **Phase 3** (emulator A/B tests,
[PHASE3-PLAN.md](PHASE3-PLAN.md), log
[PHASE3-LOG.md](PHASE3-LOG.md)): stage 1 completed – all
13 tests (start, round trips, typing, join, block, search/replace,
compiling, error line, `.xtra`, options) equal with TED-C and reference TED
under blitz2 1.95; stage 2a completed – the same tests with
blitz2-c (Rc/Cc), all 52 comparisons equal (bb2 `ted-c/results/ted-ab/report.txt`);
next up stage 2b (broader tests).

```
cmake --preset amiga && cmake --build --preset amiga && ctest --preset host
#  -> build/amiga/blitz2/ted (C build), build/amiga/programs/ted/ted.sym
#  -> build/amiga/programs/ted/ref/ted, ted.lst, ted.map, ted.labels  (vasm build of ted.s = reference)
#  checks in the build (bb2: ctest): refcheck listing-verify, hunkcheck, refcheck data, refcheck labels, checklink
#  the amiga preset also builds the host side (bb2: separate preset host):
#  -> core library (src/core) + unit tests on the host (programs/ted/tests/host;
#     instead of bb2tok.py fixtures: tests/data/ted-oracle.sha256 of the original TED)
ctest --preset emulator -R ted_ab      # phase 3: TED A/B (tests/ted-ab/), instead of bb2's
#  tools/tedab.py gate/plan/run/report; selection with -DTED_AB_FILTER=...,
#  -DTED_AB_COMBOS=... (README); bb2 ran its sessions via tools/amiberry.cmake,
#  here tools/lib/amiga_emulation
```

| File | Contents |
|---|---|
| [reference.md](reference.md) | reference build, tools, binary includes (rawed.obj, filereq, data) and decisions |
| [overview.md](overview.md) | chunks, architecture, cross-section of the surveys, risks |
| `survey-01…07.md` | interface survey per chunk |
| [xrefs.md](xrefs.md) | cross-references between the chunks (bb2 `tools/xrefs.py`) |
| [PLAN.md](PLAN.md) | file layout, headers, glue, host core, verification |
| `SURVEY-BRIEF.md` | brief of the surveys (bb2 only, not taken over) |
| [CONVENTIONS.md](CONVENTIONS.md) | translation conventions (phase 2b) |
| `PHASE2B-BRIEF.md` | brief phase 2b: ownership, check, split, open decisions (bb2 only, not taken over) |
| [deviations.md](deviations.md) | all `DIFF:` locations |
| [changes.md](changes.md) | changes to shared headers |
| [comdata.md](comdata.md) | register interface comdata/callbacks: original ↔ glue ↔ C |
| [review-2c.md](review-2c.md) | phase 2c: sample at instruction level, results |
| [PHASE3-PLAN.md](PHASE3-PLAN.md) | plan of the emulator A/B tests |
| [PHASE3-LOG.md](PHASE3-LOG.md) | log phase 3 (tools, emulator runs, deviations from the plan) |

| Directory | Contents |
|---|---|
| `programs/ted/src/core/` | core without AmigaOS headers (also host): `tedtypes.h`, `tedcore.h`, `hooks.h`, `state.h/.c`, per chunk `loadsave`, `lines`, `services`, `fileio`, `tokit`, `lineedit`, `keymap` |
| `programs/ted/src/` | Amiga part: `ted.h`, `structs.h`, `glue.h`, `glue_start.c`, `glue.c`, per chunk `main`, `edit`, `mouse`, `reqs`, `screen`, `input`, `data`, plus `rawkey`, `filereq` (core `shared/filereq/filereq_core.c`, environment `filereq_env.h`) |
| `programs/ted/tests/host/` | host tests (snitch, `test_*.cpp`; bb2: `test_*.c`), stubs of the hooks, fixture generation |
| `tests/ted-ab/amiga/` | phase 3: `teddrive` (driver, route D), `teddrivetest` (gate D), `tedprobe` (gate 0), `tdscript.h` (script syntax) |
| `tests/ted-ab/scripts/` | phase 3: test scripts `*.drv` (teddrive); bb2's gate scripts not taken over |
| `tests/ted-ab/host/` | host tests of `tedab`/`teddump` (bb2: `tedab.py`/`teddump.py`) and the script syntax |
| `results/ted-ab/` | phase 3: results in bb2 (not taken over); here the report `build/<preset>/tests/ted-ab/ted_ab.txt` |
| `tools/` | bb2: `tedlisting.py`, `xrefs.py`, `checkhunk.py`, `checkdata.py`, `checklabels.py`, `checklink.cmake`; here `tools/refcheck`, `tools/hunkcheck`, `programs/ted/checklink.cmake`; phase 3: `tests/ted-ab/runner/` `tedab` (runner), `teddump` (screen dumps) |
