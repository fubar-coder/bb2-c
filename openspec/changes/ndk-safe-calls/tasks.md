# Tasks

Every task leaves `ctest --preset host` green; the groups that change
library files also `ctest --preset all` against a prev build
(`cmake --preset amiga -DBLITZ2_PREV=ON`), with the changed library files
named in `PREV_INTENDED`. The compilers' output, `ted` and the files TED
writes stay byte-identical.

## 1. Checks before any code

- [ ] 1.1 Confirm that CMake (`file(STRINGS)`, regular expressions) reads
  the register macros of the NDK's `inline/*.h` and gives the audit's 35
  functions (design D4, Open Question 3). Verify: the list equals the
  audit's table; else switch to the tool variant before group 2.
- [ ] 1.2 List the NDK inline headers the libraries include (`rg
  '#include <proto/' libs shared`) and every use of an affected function
  (the audit's seven call sites, plus a fresh scan of the preprocessed
  compile commands). Verify: no call site is missing in the design.
- [ ] 1.3 Confirm no library is compiled with `-fbaserel` (Open Question 4).
- [ ] 1.4 Measure the function form of `EZRequest` (Open Question 1) and the
  byte difference of gtlib with the safe macro (Open Question 2).

## 2. Shadow headers and inline varargs wrappers

- [ ] 2.1 The list file and the CMake generator for the shadow headers
  (design D1, D4), the include path before the NDK in `blitz_library`.
  Verify: blitz2 and TED build byte-identical.
- [ ] 2.2 The varargs wrappers `static inline` (design D2). Verify: a
  library test object with `proto/intuition.h` has only the used wrapper,
  byte-identical to the NDK's, no warning; no symbol of an unused `__X_va`
  wrapper in any library; the changed libraries in `PREV_INTENDED`; the
  saving (about 11 KB) in the commit message.

## 3. The configure check

- [ ] 3.1 The comparison of the NDK's a3/a4/a5 functions with the list
  (design D4) at configure. Verify: the test with a list lacking one name
  fails with that name; one with a name too many fails; the full list
  passes.

## 4. Safe macros, poison, behaviour-neutral reworks

- [ ] 4.1 Safe macros for `AutoRequest`, `TextFit`, `ChangeExtSpriteA`
  (design D3) and their call sites: rundebuglib, rilvcallhooklib (the
  thunk's `/a3` goes), riwbspritelib (the own `__asm` goes). Verify:
  library tests unchanged; bytes named in `PREV_INTENDED`; the kind-F entry
  for #1028 in `docs/libs/deviations.md`.
- [ ] 4.2 The poison of every affected function the libraries do not use.
  Verify: a clean rebuild has no warning; the compile test with a poisoned
  call fails as expected (design D5).

## 5. Fixes (one commit each, bug-fix workflow)

- [ ] 5.1 #516 riapplib `AddAppIconA`: safe macro for a3 and a4, library
  test pair `[orig]` / `[fixed #516]`, `Fixes #516`, the `fix-scope` run
  named in the report.
- [ ] 5.2 #1027 rimisclib `EZRequest`/`EZRequestArgs`: per the measurement
  of 1.4, test pair, `Fixes #1027`, `fix-scope` run named.
- [ ] 5.3 gtlib (#328): `GT_SetGadgetAttrsA`/`GT_GetGadgetAttrsA` get the
  safe macro; `KEEP_A3` removed or kept per 1.4; a rework, bytes named in
  `PREV_INTENDED`. At the end the list holds no `pending` function (a test
  of the final list).

## 6. Docs

- [ ] 6.1 `docs/libs/cmdlibs-port.md` "OS calls with a parameter in a3, a4
  or a5": the build check and the rule for hand-written `__asm` calls;
  `tests/README.md`; `TODO.md` (the items of this change done).
- [ ] 6.2 Report for the coordinator: website entries and labels for
  #516 and #1027 (steps 3-5 of the bug-fix workflow).
