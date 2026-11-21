# Design

## Context

See proposal.md, "Why". What exists and what the spikes of 2026-11-21
found (`~/.local/share/bb2-c-work/tr-spike1-macros-report.md`,
`tr-spike2-testlib-report.md`, `tr-spike3-caseform-report.md`,
`tr-open-points-review.md`):

- **Which originals:** the reference of most bug-orig tests is the build
  of `blitz2.s` 1.95 (`tests/bug-orig/CMakeLists.txt:134`, `:185`); the
  506 RWE tests use the CD's 2.10 (`:256-259`). 1.95 loads a user library
  from `blitz2:DIR/` and still needs `blitz2:userlibs/DIR/` to exist
  (spike 2; `docs/v2.10/blitz2.md:122-126`).
- **Prototype** (spike 2, branch `spike/testlib`, commit 595081a3a): user
  library 40 for the spike, 6104 bytes of which about
  2.2 KB static buffers. Works with `ref`, `port` and `clib`: result file,
  crash, finit after a runtime error (`incomplete`, `end`), Blitz mode
  detection, Require. The moved copies of #265 and #263 fail exactly the
  expected checks with the original's values in the cells with the CD's
  libraries.
- **Macros** (spike 1, equal for 1.95, port and clib): `` `@ `` counts per
  macro and is fine for labels of the program's own macros; all proposed
  names are free of tokens (4876 names of the port's library sets; and of
  AmiBlitz3's 4653 tokens and 4582 library entries, task 1.4,
  `tr-task14-ab3-report.md`); a
  token as macro name is a "Syntax Error"; macro names are case sensitive;
  macro lines are joined with `:`, so `If ... Then` swallows the rest of
  the macro; `` "`1" `` keeps only the second byte of a token. Limits: 128
  characters per source line, 8192 bytes per expanded line.
- **Case form** (spike 3, equal for 1.95, port and clib): in a
  `Statement` the compiler refuses or breaks `SetErr` (26 tests), `SetInt`
  (9), `PutReg` to a local (22), a local `Dim List` (10) and top-level
  labels; a subroutine has none of these limits. The Require return keeps
  the stack, the driver's variables and loops intact in both forms. A case
  name that is a token or starts with a digit does not compile.
- **Command line:** 255 characters (`libs/acidlibs/wbstartuplib.c:76-79`,
  #308); a Workbench start (9 tests via `wbrun`) and COMPILE/RUN from the
  editor (6 tests) have none.

## Goals / Non-Goals

**Goals:**
- One library and one macro text that work with 1.95, 2.10 and the port,
  and both library sets.
- Results that do not depend on addresses: the verdict is ok / fail /
  broken; expected and actual values are information.

**Non-Goals:**
- The driver, the preprocessor, the configuration, judging, registration:
  change `basic-test-runner`.
- Moving the existing tests: change `bug-orig-basic-tests`.
- A discovery mode and a case filter in the library (user, 2026-11-21:
  the driver calls only the selected cases; the case list is checked when
  the driver is compiled).
- Tests of TED (scripts), pure compiler tests (a later form through TED's
  proxy or the compiler's command line), the corpus and the library
  capture tests keep their model.

## Decisions

**D1 Library in C, macros as text the driver defines.** Checks and the
result file are C (host tests, CLAUDE.md rule 6). The macros are a text
of the library that the driver puts in front of the test's text, so they
are the program's own macros: no resident slot, and `` `@ `` counts from 0
in each pass. The library's statements take the identification as an
argument, so they are usable without the macros.

**D2 The test source is not the main program** (user idea): the runner
copies its text into the driver (user: "Kopieren"), which calls the batch
init only if a case is selected, the selected cases, and the batch end.
`!TestBegin` does not exist for the author; a case the driver calls but
the source lacks (renamed, behind `CNIF`) fails the compile with "Label
not Found".

**D3 A case is a subroutine** (user: "G: Subroutine"). The driver jumps
over the copied text; the driver calls a case with `Gosub`. Macro text:

```
Macro TestCase                      ; !TestCase{NAME}
  Dc.l tc_end_`@                    ; never executed (the driver jumps over
                                    ;  the text); needs the end's label
  .tc_`1
  If TLCase("`1")=0 Then Return     ; last statement of the macro
End Macro
Macro EndTestCase
  TLCaseEnd : Return
  .tc_end_`@
End Macro
```

The reference of `Dc.l` enforces the end macro (user's idea): the n-th
`TestCase` refers to the label of the n-th `EndTestCase`. Spike 4
(`tr-spike4-endref-report.md`, equal for 1.95, 2.10, port and clib)
confirms the form as written: a missing end fails the compile with
"Constant Not Found", but always at the line of the last `!TestCase`, not
at the case at fault; an extra end compiles silently and cancels a missing
one. So the build-time check pairs the macros in source order (and keeps
`CNIF`/`CEND` from cutting a case); the compiler error is the second
safeguard. The `Dc.l` also keeps the case label even behind odd data (the
compiler pads before `Dc.l` but binds a label before the pad), so it stays
the first statement; no `Even` is needed. A comment after `!TestCase{...}`
or `!EndTestCase` is safe; a statement after them on the same line is lost
silently (it lands in the `Then` branch or behind the `Return`). No `If` block around
the body, so `SetInt` and the test's procedures work inside a case.
`"`1"` is safe here because the name is an identifier that is not a token.
Shared data of batch init and cases are plain globals; cases are not
isolated from each other's variables.

**D4 Check identification by the preprocessor** (user: "Wenn das stabil
funktioniert, dann OK"). The runner's preprocessor numbers the checks per
namespace (case, procedure, `init`, `end`) in source order and puts the
number into the call as a literal; it keeps a table of namespace, number,
name, source line and the text of the checked expression for the report.
Nothing of that text is in the program. No constant counter in the
program. Condition: host tests of the preprocessor (the macro argument
rules of the compiler, the cases of spike 1) and an emulator run with 1.95,
2.10 and the port show the numbering stable; otherwise the fallback is the
constant counter of spike 1 (`#tc_id`, reset by the case macro).

**D5 Control data compiled into the driver** (user: "In den Treiber").
The name of the result file is a constant of the driver given to the
library's start; the library reads no argument, so `wbstartuplib`'s list,
the 255-character line, Workbench starts and COMPILE/RUN are no concern.

**D6 Require returns through the block call.** The block function saves
the caller's registers, return address and stack pointer; a failed
`Require` makes that call return a second time with 0, and the case macro
returns from the subroutine (spikes 2 and 3, also in nested `For`,
`Select`, `Gosub`, a called procedure, with runtime error checking on). A
Require that fails inside a procedure the case calls leaks that
procedure's local strings; accepted, the case is broken anyway.

**D7 State the library keeps.** The register contract plus `d2`-`d7`
and `a2`; no allocation or free after the start (the file opened once,
static buffers, one `Write` per entry); in Blitz mode or under `Forbid`
the entries are buffered. A rule for authors in `tests/README.md`: an act
that depends on a register or on the string buffer sets it in the
statement before it, and no check stands between the two.

**D8 End entries per case and per run.** A case's end entry closes the
hole of an `End` inside a case (53 sources have more than one `End`); the
finit writes `incomplete` after a runtime error and `end`.

**D9 Checks in procedures and in init/end.** A check in a `Statement` or
`Function` of the test is numbered in the procedure's namespace and
attributed at run time to the running case; a procedure called by two
cases gives entries in both. A failed check in the batch init fails every
selected case. Helper subroutines with `Gosub` outside the cases are code
outside the blocks and refused; a helper is a procedure or a subroutine
inside a case.

**D10 Names of the macros** as proposed (`TestCase`, `EndTestCase`,
`TestBatchInit`, `Check...`, `Require`, `ExpectError`, ...); a build check
compares them with the token lists of all library sets the build has, and
with AmiBlitz3's list when the originals are fetched (user: "Nur wenn
vorhanden"; label `originals`).

**D11 Library number 81** (user: "81"). 40, used by the spike, is
STRPort in AmiBlitz3 (`libnums.ab3:333`) and LEEFUNCLIB on the CD; 81 is
free in the four variants, the CD's number lists, every AmiBlitz3 list and
every library binary under `archive/` (task 1.4; 115 is the other free
one).

## Interface

The source form, the preprocessed form, the macro text, the library
routines, the order of the driver and the result file are the spec
`testing/basic-test-interface` (change `basic-test-interface`, archived
before this change starts). The library writes its entries with the codec
of `shared/basictest/` and uses the macro text there. The staging of the
library in `tests/corpus/runner` and `tests/bug-orig/CMakeLists.txt`
(task 4.1) is done by the implementation of `basic-test-runner` after its
task 3.1, not in parallel with it.

## Risks / Trade-offs

- [The library changes what it tests: opening the file and writing
  entries shift the memory layout; spike 2 saw both bugs still
  reproduced] → no allocation after the start; a test that depends on the
  layout is a test to rewrite.
- [A wild write of a bug hits the library's state] → little state, a guard
  area of its own; a damaged state is reported as broken.
- [Cases share variables (subroutines)] → each case sets up what it uses;
  the batch init only provides data the cases read.
- [The library's DOS writes are hit by `inject`'s faults, its calls are
  seen by tests that count `JSR`s] → `inject` tests name the calls they
  fault; `JSR`-counting tests measure between labels around the act.
- [Blitz mode: buffered entries are lost when the program crashes there]
  → the missing end entry shows the crash.
- [The preprocessor misreads a check call] → host tests against the
  compiler's argument rules; the fallback of D4.

