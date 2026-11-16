# Tasks

## 1. Tests first

- [ ] 1.1 Test programs for the requirements, in both modes: a dead chain, dead recursion, an include with many procedures, and each keep rule (`Data`, a constant, a NewType, a macro, an include, `INCBIN`, conditional compilation, `DEFTYPE`, a global `Dim`, a label referenced by Goto/Gosub/Restore/`?label`/SetErr/SetInt, a label referenced by inline assembler, and AB3 mode's `OPTIMIZE 4` forward reference). Each records its output without "Make Smallest" as the expectation. They pass on main, where nothing is removed yet.
- [ ] 1.2 A check that counts which procedures are in the executable (e.g. by a marker string or symbol per procedure), so that the tests of 1.1 can state "left out" and "kept". It needs a test with broken input.

## 2. Implementation

- [ ] 2.1 The call graph in pass 1: nodes per procedure, edges from `handleproc` and `afunction`, roots by the keep rules (design decision 2), and the label references from other contexts, BASIC and inline assembler. Host tests for the graph and the walk, with inputs from memory.
- [ ] 2.2 The reachability walk between the passes, and the skip of unmarked procedures in the small pass (`dofunc2` and the procedure end). Out of memory keeps everything. Tests 1.1/1.2 pass: left out where unreachable, kept by every rule, outputs unchanged.
- [ ] 2.3 "Create Executable" and a compile with "Make Smallest" optimize; an editor run without it does not. A TED test (ted_ab or the debugger's direct mode) shows that a never-called procedure stays callable without the option.

## 3. Measure and compare

- [ ] 3.1 Compile the corpus, corpus_runerr, the CD corpus (`blitz2-cd.tsv`) and corpus_cd_ab3 as small code. Every output must be unchanged. Record the executable sizes before and after in the report, and the changed hashes in the data files with the change named.
- [ ] 3.2 Compare the size gain on AB3 programs with AmiBlitz3's measured values (`analyse-ab3-optimizers-tasktrap.md`, 1.3) where the sources build in AB3 mode.

## 4. Documents and acceptance

- [ ] 4.1 Update docs/ab3/variant.md (G8, FuncOptimize), the compiler docs, and the site page on compiler differences (optimizations without visible effect).
- [ ] 4.2 Run `ctest --preset all`, `bug-orig` and `originals`, plus `oracle` if tests/ab3 changes, with the intended executable differences named. Fix-scope must report only added cases.
