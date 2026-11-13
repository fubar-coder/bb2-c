# Design

## Context

Observed state on main:

- 247 `ID.orig` files under `tests/bug-orig/` (Blitz programs and TED
  scripts). A fixed test (`; @fixed #N`) has no `ID.orig` and no record test
  (`tests/bug-orig/CMakeLists.txt:556-557` for TED); `check-fixed-orig.cmake` refuses
  a stale one. A fixed test is judged FIXED in `all`
  (`openspec/specs/testing/bug-orig/spec.md`).
- Recording: `corpusrun pass` / `tedab run` with `BUGORIG_RECORD=1`
  (`tests/corpus/runner/cmd_pass.cpp:51`, `tests/ted-ab/runner/run.c:312`)
  runs only the `ref` configuration and writes `ID.orig`, no judgement.
- `tests/bug-orig/amiga/inject.c` has the option `pad` (every AllocMem of the
  process gets 64 bytes more, `inject.c:66`), a way to change the memory
  layout without changing the program. The mount of `BB2:` also changes
  the layout (`blitzcoplib-maxslice-zero`, `TODO.md:853-860`).
- The judge of the libs tests (`tools/lib/libscmp`, `libscmp_accept.c`) already
  implements "accept two runs or say test not deterministic", and
  `libscmp_scope.c` the fix-scope over qualified cases. Their input format is
  the libs case file (`libscmp.h:1-40`).
- Review of 2026-11-12 (recommendation 12): bug-orig is the only committed
  recording of the original that is never checked again.

## Goals / Non-Goals

**Goals:** bug-orig follows the shared model (recording separate from
testing, agreement of two runs, the original's side kept after a fix,
fix-scope); the known varying files are named with a reason.

**Non-Goals:** changing a verdict (FIXED, REPRODUCED, DIFFERS, BROKEN keep
their meaning and exit statuses); the library bug-orig tests of
`libs/tests/bugorig.c` (C build only, no `.orig`); the corpus (change
`corpus-test-model`); a bug-preserving mode.

## Decisions

### D1. The bug-orig notions are the shared terms; `ID.orig` / `ID.expect` already are the pair

A bug-orig test judges observations (sections). `ID.orig` is the original's
recorded observation: the `[orig]` value, run only against the original
(label `originals`). `ID.expect` is the correct behaviour, a pattern with
wildcards over the sections that matter: before the fix it is what nobody
shows yet, after the fix it is the `[fixed #N]` value, run only against the
port (every `all`). So the pair already exists; the difference is only that
a fix deletes `ID.orig`. Under the model it stays:

| State | `[orig]` | `[fixed #N]` / expected |
|---|---|---|
| open bug | `ID.orig` (the port matches it: REPRODUCED) | `ID.expect` (not shown yet: the test fails on purpose) |
| fixed (`@fixed #N`) | `ID.orig` (compared in `originals` only) | `ID.expect` (the port must show it: passes in `all`) |

The validity rule (`ID.orig` must not meet `ID.expect`, else BROKEN) holds
in both states and is now also checked for fixed tests. Consequence: the
fixed tests whose `ID.orig` was deleted get it back by one run of
`bug-orig-record` (task 5.1), a one-time migration whose diff is reviewed.
The "unqualified case" of the model (original and port agree) has no
bug-orig counterpart: a test exists because they differ from the expected
behaviour.

### D2. The record test becomes compare-only

`bug_orig_record_ID` keeps its name and label (`originals`, needs the CD,
the reference build for RWE tests) and loses `BUGORIG_RECORD=1`: it runs the
`ref` configuration, matches the observation against `ID.orig` with the
existing matcher (`tests/corpus/runner/bugorig.h`, wildcards included),
fails naming the differing section and first differing line, and never
opens a file for writing. The runner gets the pass key `record=check` (what
the CMake files write for the test) instead of the environment variable;
writing is the target's mode (D3) and reachable only from there.

### D3. Recording target with two layouts

`bug-orig-record` (build target, not ALL, not a test; `FILTER=ID` as
`libs-capture`) runs the `ref` configuration twice per test: the second time
with the `inject.c` option `pad`. It hands both observations to the C tool
(D4). On agreement it writes `ID.orig`; on a difference it prints
`test not deterministic: ID` with the differing sections and lines and
writes nothing for that test. A run limited by the filter leaves the other
files untouched. The proof is limited to the memory layout (date, time and
emulator timing are not varied), as in `libs-capture`.
For Blitz programs `pad` is an inject option of the program run. The
`ref` run of a TED test runs the original TED, which has no inject layer;
task 3.4 checks whether `tedab` can vary the layout (for example through the
`BB2:` mount, as `blitzcoplib-maxslice-zero` shows) and otherwise the TED
recording is accepted on two identical runs, which the documents state.

Alternative rejected: running twice inside the compare-only test on every
`originals` run: it doubles the cost of 247 tests for a property that
changes only with the CD.

### D4. The judge is a C module reusing the libs judge

New `tools/lib/bugorigcmp` (logic) with `results bug-orig-accept RUN1 RUN2
OUT`. It projects each observation onto cases (D6) and reuses the
equal-or-report logic of `libscmp_accept` for the two runs, so "differing
cases" read as in the libs tests; it adds no second accept mechanism. When
the existing `ID.orig` has wildcard lines, the rewrite keeps them where the
pattern matches both runs; a pattern that does not match is reported, never
dropped silently.

### D5. Wildcards with a stated reason

A line of `ID.orig` that varies legitimately is a wildcard line (`*` at the
end, or `...`) directly preceded by a comment line `# varies: REASON` (the
format already has `# NOTE`). A wildcard line without it is an error of the
runner (exit status 2 with file and line). Task 4.1 gives the four known
files theirs: `blitzcoplib-maxslice-zero`, `bb_blitlib-createshape-count0`,
`rigfxlib-cpucls-no-plane-crash`, `rishapeslib-read-palette-colour-count`
(#820); the reason is taken from the issue or the `TODO.md` entry, and the
wildcard covers only the varying line. Where a whole section varies with
the layout, the test pins the layout with `mount=BB2` as
`blitzcoplib-maxslice-zero` does, rather than wildcarding the section.

### D6. Projection onto cases for fix-scope

`libscmp_fix_scope` judges `struct libscmp_file` objects of cases. A
bug-orig test is projected in memory onto one (the case format of
`libscmp.h`, header `# capture bug-orig 1`), one case per section:

- fixed test: `ID|SECTION||1 [orig]` from `ID.orig`, `ID|SECTION||1 [fixed #N]`
  from `ID.expect`, N from the header (`; @fixed #N` of `ID.bas`, the
  directive of `ID.drv`);
- open test: the unqualified key `ID|SECTION||1` from `ID.expect` and the
  unqualified key `ID|SECTION|orig|1` from `ID.orig`.

A value is the section's lines with LF as `\xHH` (per the format); a long
section is `bytes N fnv H` as the libs format has it. Base (from `git
archive`, as `cmake/fix-scope.cmake` already exports `libs/tests/expected`)
and branch are projected by the same code and `libscmp_fix_scope_dirs`
judges the two sets. A fix of an open bug is the header line plus a
possibly edited `ID.expect`; the open test's cases become the pair of issue
N and pass. A branch that only adds tests of new open bugs reports their
cases as added and needs review, as new libs cases do; the documents say so.
The code of `libscmp_fix_scope` is not changed.
Because `libscmp` is being changed by `libs-capture-host-blocks` (host-side
block hashing), task 1.1 re-reads the interface of main and the adapter
uses only `libscmp_parse`, `libscmp_issues_*` and `libscmp_fix_scope`.

### D7. Selection

`tools/testinfo/rules.txt` selects the bug-orig tests for
`tests/bug-orig/**/*.orig` (`:71`); that stays valid now that fixed tests
read `.orig` too. No change of `testing/test-selection`.

## Risks / Trade-offs

- The second run doubles the emulator cost of `bug-orig-record` (247 tests,
  run on purpose); filterable.
- TED recordings may not be able to vary the layout (D3).
- Fixed tests need the CD in `originals` for their compare test (not in
  `all`).
- A wildcard reason is text; a wrong reason is not machine-checked.

## Open Questions

Recorded in `~/.local/share/bb2-c-work/questions-for-user.md` under
"## Offene Fragen": whether every fixed test must keep an `ID.orig`
(planned: yes), and whether the TED recording may skip the layout variation
(planned: yes, stated in the docs).

## Findings during the implementation

- Task 1.1 (2026-11-13): the fix-scope interface of `tools/lib/libscmp` on
  main (after `libs-capture-host-blocks`) is unchanged:
  `libscmp_parse`, `libscmp_issues_add`, `libscmp_issues_from_log`,
  `libscmp_issues_free`, `libscmp_fix_scope` and `libscmp_fix_scope_dirs`
  exist with the signatures D6 assumes (`libscmp.h`, "fix-scope"). The
  change only added the raw block observations (`NAME.bin`,
  `libscmp_parse_run`, ...), which bug-orig does not use. The adapter calls
  `libscmp_fix_scope` on one projected file per tree (base, branch), not
  `libscmp_fix_scope_dirs`, which reads `NAME.txt` files of a directory.
- The projection of D6 as written does not pass a fix with
  `libscmp_fix_scope` unchanged: the open test's unqualified key
  `ID|SECTION|orig|1` has no `[fixed #N]` case in the branch, so its
  removal is reported. The projection of a fixed test therefore keeps that
  key as the pair `ID|SECTION|orig|1 [orig]` (the value of `ID.orig`) /
  `ID|SECTION|orig|1 [fixed #N]` (the fixed text `judged by ID.expect`),
  and the expectation is `ID|SECTION||1 [fixed #N]` (from `ID.expect`)
  without an `[orig]` case of its own. An open test stays as D6 says
  (`ID|SECTION||1` from `ID.expect`, `ID|SECTION|orig|1` from `ID.orig`).
  With that, adding `; @fixed #N` and editing `ID.expect` are cases of
  issue N, a changed `ID.orig` of a fixed test is the `[orig]` case paired
  with `[fixed #N]`, and any change of an open test or of a test fixed by
  another issue is reported.
- Tasks 3.3/3.4 (2026-11-13): inject's `pad` does not give the second
  recording run another layout without changing what the test observes.
  The first implementation ran the compiler and the program under
  `inject pad`; the recording of the fixed tests (task 5.1) showed the
  cost: a program that was not under inject now is (`inject: rc=N`
  lines, the shell's `failed returncode` line gone, `inject: cannot load`
  for a program the bug makes unloadable: `compiler-loadalib-relocread`,
  `compiler-procend-kind`, `palettelib-make4bitrgbs-lowbyte`,
  `rundebuglib-printerr-numeric-code`), and pad itself changes behaviour
  (an `AllocMem(0)` succeeds with the 64 bytes:
  `compiler-loadalib-readerr`). The second run therefore varies the
  session's **Stack** instead (pass key `layout=stack`, 16384 bytes more
  for the compiler and the program; tedab's `stack=` the same for
  teddrive): every command's stack is allocated before it runs, so what
  the compiler and the program allocate lies elsewhere, and nothing else
  of the run changes. A trial showed it works: `shapeslib-messages-errarea`
  prints other bytes of uninitialised memory, `compiler-procend-kind`
  crashes instead of returning a stray return code. It needs no wrapper,
  so it also works on Kickstart 1.x and for the TED tests (which the
  first version could not vary). This replaces "the pad option of
  inject.c" of D3 and of the spec `testing/bug-orig` ("where the runner
  can vary it"); question in `questions-for-user.md`. The files recorded
  before with the pad variant (both runs agreed) were kept.
- The target runs both recording runs of up to
  `BUG_ORIG_RECORD_PARALLEL` tests at the same time (a pipeline of
  `cmake -P` processes that write nothing to stdout; the emulator slots
  limit the sessions), accepts each test with `results bug-orig-accept`,
  and fails at the end if a test was refused or a run failed; the files of
  the other tests are written. A skipped run (77, e.g. an `@rwe` test
  without the RWE reference build) skips the test and is listed.
