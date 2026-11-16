Each numbered group is one implementer round; each task is one commit unless it says
otherwise. Every commit that changes the compiled code names the changed results
(`-DPREV_INTENDED`: the programs that use the call) and the measured cycles before and
after. The corpus outputs must not change; the library tests stay as they are.
Order: group 1 prepares, groups 2 and 3 are the priority (`Int`, `Frac`), group 4 is
`Len` and `Asc`, group 5 the candidates, group 6 the documents.

## 1. Preparation

- [x] 1.1 Probe the compiler (no change committed): for `q2=Frac(q1)`, `q2=Frac(q1)*2`, `q2=1+Frac(q1)`, `q2=(Frac(q1))`, `Poke.q a,Frac(q1)`, `If Frac(q1)>0.5` and `NPrint Frac(q1)`, print the value of `old` and `evaldepth` at the entry of `libfunction` (a temporary `fprintf`); record the table in the commit message of 3.1 and answer design.md open question 1. Verify: the table names every case.
- [x] 1.2 Read the string variable layout (the header word at `-4(var)`, a variable without memory) from `docs/`, `strings1lib.c` and `strings2lib.c`; write the layout, with file:line references, into design.md Decision 4. Verify: the text names a global, a local, a field and an unassigned variable.
- [x] 1.3 Add the table `intrinsics[]` and the branch in `libfunction` (`eval.c`) with no entry yet; the branch keys on library and sub identity. Verify: `cmake --build --preset amiga` without warnings; `ctest --preset all` and the comparison with the `prev` build show every result byte-identical.
- [x] 1.4 Add the test scaffolding: expect tests `tests/blitz2/intrinsic-*.bas` with `; @runerrs on|off` headers, the result in `ID.expect`, and a way to check the generated code (the executable holds the library call or not: a check on the compiled output, in C/CMake like the other checks; decide with the existing pattern of `tests/blitz2` and `tests/README.md`, "Extensions of blitz2"). Verify: a deliberately broken expectation (the call present where the test expects none) fails with the case named.

## 2. Int of a Quick (priority)

- [x] 2.1 `Int` with a Quick argument inline (design.md Decision 2): truncation towards zero, d0 only, no type word, no `intchk` call; update `stackuse`. Verify: `Int` of the sweep (every integer part at three fractions, boundaries -32768.0, -0.5, 0, 32767.99998) equals the value computed with integer arithmetic; the test `intrinsic-int` passes in both `@runerrs` modes.
- [x] 2.2 Tests of the cases where `Int` is not an intrinsic: a Float, a Long, a Word, a Byte argument, a string argument (with checking on: "Illegal Type" as before), an expression of type Quick (this one IS an intrinsic: say so in the test), `a=b+Int(q)` with a partial result held in a register. Verify: each compiles with the library call (or inline for the Quick ones) and prints the values of the library path; the stop with "Illegal Type" is unchanged.
- [x] 2.3 Measure `bmath.int` with the `perf` preset (before and after, three runs each); tighten the budget in `tests/perf/budgets.txt`; both compilers (`blitz2`, `blitz2-ab3`) compile the tests. Verify: `ctest --preset perf -R bmath` passes with the new budget; the cycles are named in the commit.

## 3. Frac of a Quick used as a Quick (priority)

- [x] 3.1 `Frac` inline for the case of design.md Decision 3 (argument Quick, whole right-hand side of an assignment to a Quick, nothing after the closing parenthesis); result = exact fraction word with the sign of the value; condition as found in 1.1. Verify: test `intrinsic-frac`: q1 = 1.25, -1.25, 0.5, -0.5, 3.0, -3.0, 0, 30000.0001, -30000.0001, 255.00002, a sweep of fraction words, equal to `f=Frac(q1) : q2=f` (needs #1064; until then the sweep stops below 256); both `@runerrs` modes; the code holds no `qutofl`, `fltoqu` or `Frac` call.
- [x] 3.2 Tests of the cases where `Frac` is not an intrinsic and compiles as before: Float target, Long target, Float argument, `q2=Frac(q1)*2.5`, `q2=1+Frac(q1)`, `q2=(Frac(q1))`, `Poke.q a,Frac(q1)`, `If Frac(q1)>0.5`, `NPrint Frac(q1)`, an array element as the argument (the argument is Quick: it IS an intrinsic when the context holds, and not otherwise). Verify: for each case the test states the expected call or no call, and the value equals the value of the build before this change (a Float expression value printed with enough digits).
- [x] 3.3 Measure `bmath.frac` (before and after, three runs each); tighten the budget. Verify: `ctest --preset perf -R bmath` passes; the commit names the cycles (expected: from about 3750 to under 100).
- [x] 3.4 Run `ctest --preset all` and `ctest --preset bug-orig` with the `PREV_INTENDED` of the programs that use `Frac`/`Int` of a Quick; check that the corpus outputs are unchanged and every other executable is byte-identical to the `prev` build. Verify: the runs pass; the list of the programs that changed is in the commit message.

## 4. Len and Asc of a plain string variable

- [x] 4.1 `Len(a$)` of a plain string variable (global and local) reads the header (design.md Decision 4): no string copy, no pushed length, a3 unchanged, no change of `sbasegot`/`sbgot` for this argument. Verify: test `intrinsic-len`: a long string, "", an unassigned variable, a local variable, in a nested expression `b$=a$+Chr$(Len(a$))` and `c=Len(a$)+Len(b$)`; both `@runerrs` modes; the result equals the build before.
- [x] 4.2 `Asc(a$)`: the first character, -1 for an empty string and an unassigned variable. Verify: test `intrinsic-asc` with a first character of 0, 127, 128 and 255 (zero-extended, not sign-extended), "" and an unassigned variable, same nesting cases.
- [x] 4.3 Tests of the cases that stay as before: `Len(a$+b$)`, `Len(Left$(a$,2))`, `Len("literal")`, `Len(s$(3))`, `Len(t\name$)` (a field), and `Len(a$)` where `a$` is a `NewType` field. Verify: the test states the compiled code is the same as the build before (the string is built, the library is called) and the value is right.
- [ ] 4.4 Measure `bstr.len` and `bstr.asc` with strings of 10, 100 and 1000 characters (add the lengths to the loop if the benchmark has one only; the cost must not grow with the length any more); tighten the budgets. Verify: `ctest --preset perf -R bstr` passes; the commit names the cycles.
- [ ] 4.5 Run `ctest --preset all` and `ctest --preset bug-orig` with `PREV_INTENDED` for the changed programs; check the corpus outputs. Verify: as 3.4.

## 5. Candidates (measure, do not decide)

- [ ] 5.1 For each candidate of design.md Decision 5, measure the current cost in `tests/perf` (add a bench loop where there is none: `Sgn(q)` and `Abs(q)` have lines; `Left$`/`Right$`/`Mid$`/`Instr` with a variable source, `Len`/`Asc` of an array element and a field, `Val`/`Cvi` with a variable do not) and estimate the intrinsic's cost from its instruction count. Verify: a table (candidate, cycles now, estimate, the conditions an intrinsic would need, risk) in `~/.local/share/bb2-c-work/compiler-intrinsics-candidates.md`; no compiler change.

## 6. Documents

- [ ] 6.1 Add the intrinsics to `docs/blitz2/extensions.md` (the list of compiler behaviour that has no original to compare with): the conditions of each, the result equal to the library's, and that the executables differ from the original's for programs that use them. Verify: the page names the four intrinsics and their conditions; a link check of the docs passes if the project has one.
- [ ] 6.2 Add the site entry (the page of differences from the original, if the compiler's output is described there: coordinator's decision) and the line in `docs/blitz2/` about `PREV_INTENDED` for the changed programs. Verify: the text matches the code; no claim without a reference.
- [ ] 6.3 Sync the spec: `openspec validate compiler-intrinsics` passes with the delta; after the implementation, the delta is archived into `openspec/specs/compiler/intrinsics`. Verify: `openspec validate compiler-intrinsics`.
