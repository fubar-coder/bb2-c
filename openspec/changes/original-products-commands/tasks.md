# Tasks

Names and place are decided (proposal, "Decisions of the user"). An enhancement, not an original bug: no bug-orig test, no `Fixes #N`; the commit names `#1036`.

## 1. Preparation

- [ ] 1.1 The library number (the proposal's planning choice): 166 free in every `libs/variants/*/libnums.h`, the CD's `LIBNUMS.BB` and `BUM9B`/`BUM10A` `LIBNUMS.BB2`, AmiBlitz3's `libnums.ab3` and `sorted_libnums.ab3`, every library file under `archive/` and AmiBlitz3's binary libraries (the method of `basic-testlib`, design D11); if not, the next free one of the design's list, named in the report. Verify: the report; `blitzlib numbers` accepts it.

## 2. Tests first

- [ ] 2.1 Test of the BASIC test model `tests/blitz2/original-products.test.bas` with `original-products.test.toml` (`extension = 1036`; what the original does instead as declared deviations; no `.expect` file, `runtime-errors` off): `LMulBB2(196608,30000)`, `LMulBB2(-196608,30000)`, `QMulBB2(300,300)` raw, the LCG from 4711 (5 numbers), the edge values (0, -1, $7FFFFFFF, $80000000, 2^k for k = 0..31, both functions, both argument orders), and checksums over the 3000 mixed pairs of #199 (`~/.local/share/bb2-c-work/analyse-199/`) computed with the functions and with `OldMul{}`/`OldQMul{}` in the same program (they must agree; the checks hold both checksums). Verify: fails on main ("Token Not Found").
- [ ] 2.2 Library capture `libtest_NAME` in `libs/tests` with the vectors of `test_lmullib` (`systest_mathstr.c:346`) and a check that each block's checksum equals the `[orig]` block of `libtest_lmullib` (design, planning choice 3); a KEEPS case (a3-a7 kept). Verify: fails on main (no library).

## 3. Implementation

- [ ] 3.1 The library (`libs/acidlibs/`, the CMake entry, the number in `libs/variants/bb2-2.10/libnums.h`): the two functions from the archive's routines (design decisions 1, 2), their declarations (`.l`/`.q` arguments and results), help texts; `blitzlib check` clean, no warnings. Verify: 2.1 passes; 2.2 passes (`libs-update` for its expected data, reviewed with `git diff`).

## 4. Checks

- [ ] 4.1 `ctest --preset host`; `ctest --preset all -R 'blitz2_|libs_tests|corpus$'`. Verify: pass.
- [ ] 4.2 Cost: `tests/perf` lines `bmath.mulbb2`, `bmath.qmulbb2` (the inputs of `bmath.lmul`/`bmath.qmul`) with the target of the original's operator (`perf-original`). Verify: `ctest --preset perf`; each at or below the original's cycles; no existing budget rises.
- [ ] 4.3 Prev comparison: only the `acidlibs` files with the new library differ. Verify: `ctest --preset all -R prev` with `-DPREV_INTENDED` naming them.

## 5. Documents

- [ ] 5.1 `docs/libs/bb2-readme.md` (the #199 section: the functions first, OldMul{} for the CD's libraries and AB3 mode), the library's head comment; site: "Getting the old products" on the page `compared-with-original/libraries/commands/lmullib.md` points to the functions, the intro and command texts of the generated page. Verify: coordinator's review; the cmdref data checked by the build; links resolve.

## 6. Commit and report

- [ ] 6.1 One commit for the library with tests, budget lines and `PREV_INTENDED`; one site commit. Message names `#1036`. Report: the measured cycles against the original operator; the files. Verify: `git show --stat`.
