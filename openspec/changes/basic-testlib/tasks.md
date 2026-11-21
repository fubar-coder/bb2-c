# Tasks

## 1. Open points

- [x] 1.1 Spike 3, the form of a case with 1.95, 2.10 and the port; verify: `~/.local/share/bb2-c-work/tr-spike3-caseform-report.md`, the user's choice (subroutine) in `task-decisions.md`
- [x] 1.2 The user decides how the library gets its control data; verify: "In den Treiber" in `task-decisions.md`, design.md D5
- [x] 1.3 Short spike of the end enforcement of D3 (`Dc.l tc_end_`@` before the case label, the label in the end macro) with 1.95, 2.10 and the port, both passes, a missing and an extra end, alignment after `Return`; verify: a report in `~/.local/share/bb2-c-work/` and design.md D3 confirmed or changed
- [x] 1.4 Check the macro names and library number 40 against the AmiBlitz3 token list and library numbers; verify: the result named in design.md (D10, Context)

## 2. Library

- [ ] 2.1 Reserve library number 81 in every `libs/variants/*/libnums.h` and the documentation of the library numbers; verify: the number check of the build and `blitzlib check` pass in a clean rebuild without warnings
- [ ] 2.2 Write the library in `libs/userlibs/` (start with the result file's name, block functions, checks with namespace and number, string check, precondition with the return through the block call, announcement of a runtime error, case end, finit with `incomplete`/`end`, buffering in Blitz mode and under Forbid, attribution of procedure checks to the running case), starting from the prototype of `spike/testlib`; verify: host tests of the C logic (entry format, verdicts, buffering, attribution) pass with and without `-DHOST_SANITIZE=ON`
- [ ] 2.3 Register test of the routines (the register contract plus `d2`-`d7`/`a2`) and a test that free memory is unchanged around a check; verify: both run in the emulator with `port` and `clib`

## 3. Macros

- [ ] 3.1 The macro text (`TestCase`/`EndTestCase` as in D3, `TestBatchInit`/`EndTestBatchInit`, `TestBatchEnd`/`EndTestBatchEnd`, `Check...` with namespace number and optional name, `Require`, `ExpectError`); block `If` only apart from the last statement of `TestCase`; verify: a program with every macro compiles with 1.95, 2.10 and the port with the same entries
- [ ] 3.2 A build check of the macro names against the token lists of all library sets of the build, and against AmiBlitz3's list when the originals are fetched (label `originals`), with a test with deliberately broken input (a macro named like a token); verify: the check fails on the broken input and passes on the real text

## 4. Staging and documentation

- [ ] 4.1 Stage the library for every configuration of the runner (1.95: `blitz2:DIR/` and `blitz2:userlibs/DIR/`; 2.10 and the port: `blitz2:userlibs/DIR/`); verify: a test with one passing and one failing check gives the expected result files with `ref`, the RWE reference, `port` and `clib`
- [ ] 4.2 Document how a test source is written (blocks, names, checks in cases, procedures, init and end, preconditions, the rule for register- and string-buffer-dependent acts) in `tests/README.md` and `docs/testing.md`; verify: the examples in the text compile as a test of the build
