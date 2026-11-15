## Context

Sources: `~/.local/share/bb2-c-work/perf-analysis/decisions.md` (all
decisions), `report.md` (the measurements), `thunks.md` (register
analysis), `~/.local/share/bb2-c-work/analyse-thirdparty-regs.md`
(third-party and AB3 callers). Facts used here:

- 537 `BLITZ_THUNK` entries; 445 of 661 entries pay 86 cycles or more
  before the C function starts, the originals nothing (`report.md`, cause T).
- gcc saves d2-d7/a2/a6 on every path of a function that contains a
  `BLITZ_BASEJSR`/`BLITZ_ALIBJSR` (cause R), e.g. vallib Sgn/Int/Abs.
- The compiler's generated code relies on a3 to a7 only (`thunks.md` 1.2).
  Internal tokens have their own contracts (1.4): operators keep d2-d7,
  strcomp/copstring d0-d7, `lastring`/`astring` d1-d7 and a2, the procedure
  prologue routines d0-d5, freelast d0 and the flags, error checks all.
- The experiment with `-fcall-saved-d1/a0/a1` failed because gcc 16.2
  miscompiles tail calls with it (the epilogue restores d1/a0/a1 before the
  `jmp`); it is not a path (`thunks.md` 0 and 5).
- The callers that need registers: `analyse-thirdparty-regs.md` sections 0
  and 5 (eight entries).

## Goals / Non-Goals

**Goals:** the C libraries come as close to the originals' speed as the
decisions allow, without changing what any command does; the register rule
is the documented one; the speed is measured by a test that fails on a
regression.

**Non-Goals:** no change of observable behaviour; no new error checks; no
fix of the issues listed in the proposal; no `-fcall-saved` ABI flag; no
other optimisation level (the libraries stay at `-Os` except the speed
profile); no byte comparison of the changed library files with the prev
build.

## Decisions

1. **The register rule is Sibly's.** Every user-level entry keeps a4, a5, a6,
   a7 and (functions) a3; nothing is guaranteed for d0, d1, a0, a1, and the
   upper word of d0 of a word function is undefined. Alternative: a general
   rule "functions keep a0/a1", suggested by the three CD programs under
   inline assembler; the user decided for a list of exceptions instead.
2. **The default is an alias of the C function.** gcc keeps d2-d7 and a2-a6,
   which covers the rule. The thunk is removed for user-level statements and
   functions only. Internal tokens (operators, strcomp, copstring,
   lastring/astring, the prologue routines, freelast, clrloc, newmem,
   localstat), nullsubs and all error checks (`BLITZ_ERRCHK_THUNK`) keep
   their thunks.
3. **Exceptions only for known callers.** Per entry the analysis names the
   caller. Where gcc's output keeps the register by nature the entry is an
   alias and the KEEPS test states the register: if a compiler change makes
   gcc use it, the test fails instead of a third-party library. Otherwise
   (InitSound; possibly EventWindow and RawStatus) a reduced thunk that saves
   exactly the needed registers, or an assembler block when the entry is hot
   (Rnd, RawStatus). Each exception is verified by disassembling the
   generated code, not by reading the C.
   A getter of two instructions whose behaviour a caller relies on
   (EMouseX, EMouseY, EventCode) is an assembler entry as in the original,
   `move.w x,d0 : rts`, not an alias (user decision 2026-11-15). The code
   is as short as the C function's, and the guarantee (only d0.w is
   written) is in the code instead of depending on gcc's choice; the KEEPS
   test stays as a check. The other event getters of windowslib
   (GadgetHit, MenuHit, ItemHit, SubHit, EventQualifier) get the same form
   with the small entries.
4. **Word functions** keep the upper word of d0 only where a caller needs it
   (EMouseY keeps it; EventCode needs zero or kept; both are assembler
   entries, point 3): the entries with
   `BLITZ_THUNK_POST(.., "move.w d0,2(sp)")` that have no such caller become
   aliases. `ExecVersion` returns a zero-extended long (the analysis: a
   caller compares the word as a long).
5. **Assembler blocks are the regular fix for hot routines** (precedents:
   SColl, Scale). A block is the original's algorithm (from the CD's
   library, extracted with `blitzlib extract`), with the fixes of decided
   bugs kept at the entry (the ldivlib fix of #200). A block keeps the
   entry's rule of Decision 1 or its exception. The order is the decided one:
   the ldivlib loop, the string copy loops, the small entries, the blit core
   and sortlib. Each step is measured by the benchmark before and after.
6. **Speed profile** adds strings1lib, strings2lib (measured +8 and +4
   bytes, -510 cycles per 21 character assignment), medlib (-630 per PlayMed
   frame, +776 bytes) and trackerlib (-220 per PlayModule frame, +212 bytes)
   to `ACIDLIBS_SPEED` (`libs/acidlibs/CMakeLists.txt:29`). The rest stays
   `-Os`.
7. **Rare paths out of line.** Where the cost is gcc's register save on a rare
   path (Sgn/Int/Abs float case, the allocation in `assign`), the rare part
   becomes a `noinline` function; an assembler entry is the alternative if
   that is not enough.
8. **The KEEPS judge.** The kept set of a routine is a4, a5, a6, a7 (a3 for
   functions) plus the registers of its stated exception. Today the judge
   states the intersection of what the original keeps (`tests/README.md`,
   "Kept registers"); the new one states the rule, so a library may keep
   fewer of the accidental registers without the test failing, and the
   exception entries still fail if they lose a needed register. The result
   and change cases stay. The summary of the registers the original keeps
   stays as information. The existing `KEEPS` statements (about 1016) are
   reduced mechanically.
9. **Benchmark `perf`.** A test runs one `For i=1 To N` loop per command in
   the cycle-exact A500 configuration of the measurement (`cpu_cycle_exact`,
   `cpu_memory_cycle_exact`, `blitter_cycle_exact`, `cpu_speed=real`,
   Kickstart 1.3, runtime error checking off), reads a CIA-B timer cascade,
   subtracts the empty loop. A budget file states per command the allowed
   cycles per call. The ctest label is `perf`; the preset `perf` is separate
   and not part of `all` (one configuration takes about 3 minutes, one
   emulator session at a time). The method repeats to 1 cycle per call.
10. **Acceptance.** Library bytes change on purpose, so `prev_results` and
    the corpus `prev` configuration are run with `-DPREV_INTENDED`; each
    commit names the library files and executables whose bytes change.
    What must stay identical: every compiler output that links no changed
    library, TED's files, and the behaviour measured by the library tests
    (`libs_emulator`, KEEPS under the new judge), the corpus, the bug-orig
    tests, and the perf budgets.
11. **Where the benchmark lives (task 1.1, decided with the first
    measurement of 2026-11-15).** In `tests/perf/`, beside the corpus,
    whose machinery it uses:
    - `tests/perf/bench/*.bas`: the Blitz sources of the measurement
      (`perf-analysis/bench/`, generated there by a throw-away script, now
      plain sources kept by hand), one program per area, tokenised by
      `tedsrc tok` like the corpus. `bmod` is reworked: an `empty` loop
      like the others, the PlayModule interrupt as a loop of the kind
      `frame` (the cycles per PAL frame the handler takes from the empty
      loop: 142102 x (1 - E / loop), E the empty loop), the timed one-call
      loops of the start and stop commands dropped (a resolution of 1000
      cycles).
    - The run: not a new runner on `tools/lib/amiga_emulation` but
      `corpusrun pass --observe` (`tests/perf/perf.cmake`), which already
      compiles with blitz2 in Amiberry, runs on Kickstart 1.3 (`rcput`),
      stages the module files, takes the `machine=`/`uae=` lines of the
      cycle-exact A500 and skips without Amiberry; the measurement of
      `report.md` used it too. One session (`parallel=1`), the test
      `RUN_SERIAL`.
    - The arithmetic and the budgets in C: `results perf` with the logic
      in `tools/lib/perfbudget` (the Python `res4.py` of the analysis
      ported), tested by `tools/tests/test_perfbudget.c`.
    - The budget file `tests/perf/budgets.txt`; the original's values
      come from the target `perf-original` (the same pass with the CD's
      libraries, not a test).
    The first measurement in the repository (clib, then the CD's
    libraries, one session each, 216 s and 168 s) reproduces `report.md`
    to the cycle for the unchanged commands (e.g. Long division 5673.1
    against 2661.0, Peek.b 217.1 against 129.0, Sort 653474 against
    77694); it differs where main changed since `b59cbe2e` (SColl 136.4,
    from the SColl rework; DisplaySprite 1578.5). Of 119 commands 101 are
    slower than the original by more than 2 %, 15 equal, 3 faster (Float
    assignment, QAbs, String$).

## Risks / Trade-offs

- A caller outside the analysed set (`analyse-thirdparty-regs.md` section 7:
  static analysis of 566 library images) relies on a register the new rule
  drops. Mitigation: the deviation lists the rule and the exceptions; the
  site page says what a library author may rely on; KEEPS tests for the
  exceptions.
- A toolchain update may make gcc use a register an alias entry keeps by
  nature. Mitigation: its KEEPS test fails first (Decision 3).
- Without the thunk a tail call of the entry becomes a direct jump into the
  C function; the `-fcall-saved` failure shows tail calls interact with
  register contracts. Mitigation: exceptions are verified in the
  disassembly with the shipped flags.
- Assembler blocks are less maintainable and gcc cannot check them.
  Mitigation: small blocks from the original's code, covered by the library
  tests (they run original and C build against the same expected data) and
  by new cases where the original leaves an entry condition undefined.
- Timing in an emulator: Amiberry's cycle model was not checked against
  hardware (`report.md`, open questions). Budgets compare library sets on
  the same emulator; they are not absolute hardware numbers.
- #1048: an error check in ldivlib/lmullib would add a jsr/rts per division
  in release programs. Nothing here adds one.

## Migration Plan

One group of tasks per implementer round, one commit per topic, in the order
of tasks.md. Each commit builds, passes `ctest --preset all` and
`ctest --preset bug-orig`, and names its changed results. The benchmark and
the rule (groups 1 to 3) come first, because the small-entry blocks and the
budgets need them. Rollback is one commit per entry group.

## Open Questions

- Where the benchmark sources live (decided in task 1.1, Decision 11):
  `tests/perf/`.
- Budgets (decided): each command's budget is the current measurement plus
  a tolerance (a guard against slowing down) and is tightened after each
  step; the original's measurement is recorded next to it as the target.
  The goal is the original's speed or better (the user's rule: no change
  may make correct programs slower).
- Whether EventWindow and RawStatus keep their registers by nature
  (task 2.1; if not: reduced thunk or, for RawStatus, an assembler entry).
- #1058 (decided): the InitSound fix is done in this change, task 2.4, in
  one commit with its test and `Fixes #1058`.
