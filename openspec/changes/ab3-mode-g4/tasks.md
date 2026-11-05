# Tasks

## 1. Side table and definition

- [ ] 1.1 `ab3proc.c`: side table per `struct proc *` (first optional index, FAST flag, NewType return node) and the slot layout function (offsets and total bytes from `params[]`: 2 bytes for byte/word, 4 for long/quick/float, 8 for a string; parameters 1-6 only their string slots) (D1, D3)
- [ ] 1.2 Extension points `ext_procfast`, `ext_procoptional`, `ext_procmax` in `dofunc2`; BB2 behaviour in `ext_bb2.c` (D2). Verify: the `@`, `FAST` and 7-parameter definitions of the probes compile in AB3 mode; BB2 mode unchanged (corpus with both compilers byte-identical)
- [ ] 1.3 `ext_procrettype` in `dofunc`: `Function.nt` in AB3 mode, the type node for the pointer-assignment rule (D2)

## 2. Parameters 7-10 on the stack

- [ ] 2.1 Callee: copy parameters 7-10 from their slots into the locals at entry; strings at every position through the slot table (pointer from the slot for registers > 5) (D3)
- [ ] 2.2 Epilogue with `ext_procstackbytes`; `afunction`/`handleproc` subtract it from `stackuse` (D3)
- [ ] 2.3 Caller: arguments 7-10 of a procedure call through `maineval2`'s push path, strings with AmiBlitz3's 8-byte slot, every push counted in `stackuse` (D3). Verify: probes p10 (10 parameters, statement and function, `.w`/`.b` beyond 6), p11 (nested call: `p7=7 p8=100`), p12 (optional and string beyond 6)

## 3. Omitted arguments

- [ ] 3.1 `ext_procdefaults` on a missing `,` in `fetchpees_p` (tail) with the default code of D4; `notparerr` before the first optional parameter. Verify: p01 (defaults per type), p02 (string default), p07 (mandatory after optional), p08 (not enough: our error, no hang), p20 (NewType pointer -1)
- [ ] 3.2 The empty path `name{}` in `handleproc` and `afunction` (D4), including 7-10 only-optional parameters. Verify: p03 (statement and function), p22 (eight only-optional parameters filled)
- [ ] 3.3 The error cases: p04 (`s{1,,3}`), p05 (`s{1,}`), p09 (too many arguments), p13 (11 parameters)

## 4. FAST

- [ ] 4.1 `linkput` without `clrloc` for FAST; `procfixer`'s error when FAST and `nstatics != 0` (D5). Verify: p14 (FAST function and statement), p17 (locals not cleared: `42`, `42`), p15/p16 (local string, local NewType: the error)

## 5. NewType return type

- [ ] 5.1 Verify p18 (compiles in ours, AmiBlitz3 refuses the `Function Return 0`: a Differences row) and p19 (`*p.nt = mk{}`, prints `5`)

## 6. Tests and documentation

- [ ] 6.1 `tests/ab3/cli` cases from the probes (sources in the session scratchpad `g4/probe/`, `g4b/probe/`), registered in `tests/ab3/CMakeLists.txt`; the running cases also in `ab3_cli_ref` against AmiBlitz3 (`REF_EXPECT` where ours differs on purpose: p11, p22, p18); AmiBlitz3's hanging p08 not in `ab3_cli_ref`
- [ ] 6.2 BB2-mode cases: `@` keeps the pointer flag ("Not Enough Parameters"), 7 parameters refused, `FAST` as a name; `inc_datetime` still passes with real optional parameters
- [ ] 6.3 AmiBlitz3's sources: `function_problem` (prints `5`), `serial_read` (compiles), `error.include.ab3` as an include case (compile only); the measured share of the 68 test programs updated in `docs/ab3/variant.md`
- [ ] 6.4 `docs/ab3/variant.md`: G4 section (rules, slot layout, defaults) and "Differences" rows (parameters 7-10 on the stack, `name{}` with 7+ optional parameters, no `Function Return` NewType check, our error texts, "Not enough parameters" without the hang); `TODO.md` G4 done
- [ ] 6.5 Full check: a clean build without warnings, `blitz2` and BB2-mode results byte-identical (bb2-equivalence), `ctest --preset all` and `ctest --preset bug-orig` with the known verdicts

## Workflow follow-up

- Archive the change with `/opsx:archive ab3-mode-g4`. This creates
  `ab3-compat/procedures` in `openspec/specs/`.
