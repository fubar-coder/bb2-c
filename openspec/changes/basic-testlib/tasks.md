# Tasks

## 1. Open points

- [x] 1.1 Spike 3, the form of a case with 1.95, 2.10 and the port; verify: `~/.local/share/bb2-c-work/tr-spike3-caseform-report.md`, the user's choice (subroutine) in `task-decisions.md`
- [x] 1.2 The user decides how the library gets its control data; verify: "In den Treiber" in `task-decisions.md`, design.md D5
- [x] 1.3 Short spike of the end enforcement of D3 (`Dc.l tc_end_`@` before the case label, the label in the end macro) with 1.95, 2.10 and the port, both passes, a missing and an extra end, alignment after `Return`; verify: a report in `~/.local/share/bb2-c-work/` and design.md D3 confirmed or changed
- [x] 1.4 Check the macro names and library number 40 against the AmiBlitz3 token list and library numbers; verify: the result named in design.md (D10, Context)

## 2. Library

- [x] 2.1 Reserve library number 81 in every `libs/variants/*/libnums.h` and the documentation of the library numbers; verify: the number check of the build and `blitzlib check` pass in a clean rebuild without warnings
- [x] 2.2 Write the library in `libs/userlibs/` (start with the result file's name, block functions, checks with namespace and number, string check, precondition with the return through the block call, announcement of a runtime error, case end, finit with `incomplete`/`end`, buffering in Blitz mode and under Forbid, attribution of procedure checks to the running case), starting from the prototype of `spike/testlib`, writing its entries with the codec of `shared/basictest/` (`testing/basic-test-interface`); verify: host tests of the C logic (entry format, verdicts, buffering, attribution) pass with and without `-DHOST_SANITIZE=ON`
- [x] 2.3 Register test of the routines (the register contract plus `d2`-`d7`/`a2`) and a test that free memory is unchanged around a check; verify: both run in the emulator with `port` and `clib`

## 3. Macros

- [x] 3.1 The macro text of `shared/basictest/macros.bas` (from `basic-test-interface`) with the library: every macro in a hand-written driver of the form of the interface spec; verify: a program with every macro compiles with 1.95, 2.10 and the port with the same entries, and a comparison gives -1 for true in all three (the mapping of `!Check{K,COND}` onto `TLCheck K,-1,(COND)<>0` depends on it)
- [x] 3.2 A build check of the macro names against the token lists of all library sets of the build, and against AmiBlitz3's list when the originals are fetched (label `originals`), with a test with deliberately broken input (a macro named like a token); verify: the check fails on the broken input and passes on the real text

## 4. Staging and documentation

- [ ] 4.1 (done by the implementation of `basic-test-runner`, after its task 3.1; design.md "Interface") Stage the library for every configuration of the runner (1.95: `blitz2:DIR/` and `blitz2:userlibs/DIR/`; 2.10 and the port: `blitz2:userlibs/DIR/`); verify: a test with one passing and one failing check gives the expected result files with `ref`, the RWE reference, `port` and `clib`
- [ ] 4.2 Document how a test source is written (blocks, names, checks in cases, procedures, init and end, preconditions, the rule for register- and string-buffer-dependent acts) in `tests/README.md` and `docs/testing.md`; verify: the examples in the text compile as a test of the build

## 5. Typed checks and the findings in the interface (decided 2026-11-21, task-decisions.md "basic-testlib: typed checks, findings of implementer A")

- [x] 5.1 The delta of `testing/basic-test-interface` in this change: `!CheckQEq`, `!CheckFltNear`, their preprocessed forms, `TLCheckQ`, `TLCheckFlt`, the entries `checkq`, `checkflt`, the last field of `checkstr` (`-`/`cut`), the rule that a condition is a long, and the findings of design.md that are behaviour (preconditions and the batch init, entries of calls out of order, limits); verify: `openspec validate basic-testlib --strict`
- [x] 5.2 The codec (`shared/basictest/`): `checkq`, `checkflt` and the field of `checkstr`, writer and reader; verify: host tests with round trip and broken input, the README example and its test
- [x] 5.3 The library: `TLCheckQ`, `TLCheckFlt` (mathffp.library opened by `TLBegin`), the cut field, the macros `CheckQEq`, `CheckFltNear`; verify: host tests of the logic, an emulator run with 1.95, 2.10, the port and both library sets (a quick 1.5 against 1.5 and 1.0, floats within and outside the delta, a cut string)
- [x] 5.4 design.md records the decisions and the author rule for `!Check`; verify: the text

## 6. Length and CRC32 of string values (decided 2026-11-21, task-decisions.md "String values: length and CRC32")

- [x] 6.1 The delta of `testing/basic-test-interface` in this change: each string value of `checkstr` followed by the length and the CRC32 of the whole string (the field order, the encodings, the standard CRC-32 of IEEE 802.3), what a cut value is and what the reader refuses; verify: `openspec validate basic-testlib --strict`
- [ ] 6.2 The codec (`shared/basictest/`): `basictest_crc32` usable by the library and the host (no allocation, no division, no libgcc), writer and reader with the new fields; verify: host tests with known CRC vectors (`123456789` gives `$CBF43926`), round trips and broken input, with and without `-DHOST_SANITIZE=ON`, the build check of the writer, the README example and its test
- [ ] 6.3 The library: `TLCheckStr` computes length and CRC32 over the whole strings before the values are cut; verify: host tests of the logic, an emulator run with 1.95, 2.10, the port and both library sets (a short string, a pair of 400 bytes that is cut, equal values with equal CRC32s)
- [ ] 6.4 design.md records the decision; verify: the text
