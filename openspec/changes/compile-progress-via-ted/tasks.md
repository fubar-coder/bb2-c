# Tasks

Group 1 is analysis; groups 2 to 4 are one commit each, the fix commit
(group 3) with `Fixes #980`. Every commit: clean `cmake --build --preset
amiga` without warnings, `ctest --preset all` with a prev build
(`-DBLITZ2_PREV=<base>`): the executables, library files and the files TED
writes stay byte-identical (`prev_results`, corpus `prev`); `ted_ab`
differences only as named in the task.

## 1. Interface check

- [ ] 1.1 List every `comdata` access of the original compiler (`archive/sources/blitz2-1.95/blitz2.s` and the CD's compiler, `docs/v2.10/`) and of blitz2-c (`programs/blitz2/src`, `CD_*` in `main.h`): confirm no offset at or above 98 is read, so the marker and entry of D2 are invisible to the original. Verify: the list with file:line in the commit message of group 2; if an offset >= 98 is read, stop and report (D2 changes).
- [ ] 1.2 Record what the title area looks like after a compile from TED-C with the original compiler (C5 dumps of `tests/ted-ab/scripts/06-compile.drv` and of the other scripts that compile): whether the stripe is visible in any dump. Verify: a note in the group 3 commit message naming the dumps that show the stripe (or none).

## 2. TED-C: the progress call

- [ ] 2.1 Append the marker `'TEDp'` and the progress entry after `req_window` in `struct mydata` (`programs/ted/src/input.c`, `ted.h`), with a register stub in TED-C's glue (`d0` bar, `d1` column, design D3); `static_assert` the offsets 98 and 102. Verify: build; `CDOFF`-style asserts pass; TED-C's other `comdata` offsets unchanged (asserts).
- [ ] 2.2 The routine (design D4): returns if the screen is not open; else `COMPLEMENT`, write mask 1, an 8 x 1 `RectFill` at x = 8 x column in rows `bar` and `9 - bar`, draw mode and mask restored. Verify: host test of the cell geometry if TED-C's host core can run it, otherwise covered by 3.3; `ctest --preset all` with `ted_ab`: R5, C5, Cc, Rc unchanged (the compilers do not call it yet).
- [ ] 2.3 Document the call in the `comdata` description (`docs/ted/` interface notes, `docs/blitz2/` editor interface): offsets 98/102, registers, meaning, the marker rule. Verify: the docs name the offsets and registers as the asserts do.

## 3. blitz2-c: report instead of draw (fix of #980)

- [ ] 3.1 `programs/blitz2/src`: `CD_PROGMARK` 98 and `CD_PROGRESS` 102 in `main.h`, `ed_progress(bar, column)` through `callregs`; `compile` checks the marker once per compile; `nextline` calls `ed_progress` where it XORed (main: `hi - 1`; include: `col`) and calls nothing without the marker; `botline` and the reads of `combitmap`/`bmapmul` for drawing removed; `BUG(orig) (#980)` becomes `Fixed (#980)`. Verify: build; corpus and CD executables byte-identical (prev).
- [ ] 3.2 `shared/edstub` / `tedstub`: pass-file options to announce the call, to set `bmapmul`, and to put guard bytes before and after the bitmap; the stub logs the reported cells and checks the guards at the end. Verify: `tedstub` host or emulator self-test with the options; without them the stub behaves as before (every test that uses `tedstub` passes).
- [ ] 3.3 bug-orig test `tests/bug-orig/compiler/compiler-racing-stripe-wide.bas` with `; @fixed #980` (design D6): wide `bmapmul`, guards, an include so that every bar moves; once without the call (guards untouched), once with the call (guards untouched, cells as the original would XOR). Record the original's observation (`.orig`, the guard before the bitmap overwritten). Verify: FIXED in `ctest --preset all`; REPRODUCED on the commit before.
- [ ] 3.4 TED A/B: C5-Cc of the scripts that compile stay equal; for a dump that shows the stripe (1.2), R5-Rc gets `; @fixed #980 R5-Rc` and the differences are only the bar. Verify: `ctest --preset all`, `ted_ab` report: C5-Cc equal, R5-Rc FIXED #980 (differs) only where expected.
- [ ] 3.5 `docs/libs/deviations.md`: the #980 row becomes an intended deviation (kind F: the progress is reported to TED-C, the original TED gets no bar); commit message names the changed results (none of the executables; `ted_ab` R5-Rc if 3.4 applies) and `Fixes #980`. Verify: the row and the message.

## 4. Final checks

- [ ] 4.1 `ctest --preset bug-orig`: no verdict changed except the new FIXED test (known DIFFERS in TODO.md). Verify: the verdict list compared with the run before the change.
- [ ] 4.2 Manual run in Amiberry with `tools/amiga_run` (visible): compile a long program with an include from TED-C with blitz2-c and with the original compiler; the bar moves the same way. Verify: screenshots of both, named in the report.

## Workflow follow-up

- Coordinator: site entry on the fixed-bugs page (compiler area), label `fixed-in-port` on #980, the row in `docs/v2.10/blitz2.md`, the issue closed after the push.
- Archive the change after acceptance.
