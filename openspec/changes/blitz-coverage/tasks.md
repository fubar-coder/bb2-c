# Tasks

## 1. Option and map

- [ ] 1.1 Add `-coverage`/`-cov` to `ab3args` and the job, refuse it with `-run` and without `-e`; verify with host tests of the argument parser and an `ab3/cli` case that checks the usage text and return codes.
- [ ] 1.2 Add `covstatement`/`covundo` in a new compiler module, map file writer (`<exe>.covmap`, D4) with file and line of main source and includes; verify with `ab3/cli` cases that compare the map of a source with includes, `:`-joined statements, a `@@` line and a macro against expect files, in both modes.
- [ ] 1.3 Verify that compiles without the option are byte-identical: build the previous commit and compare the corpus' executables and the programs `blitz2`/`blitz2-ab3` in the normal build.

## 2. Counting in the program

- [ ] 2.1 Decide the library slot and write `covlib` (finit writes `<exe>.covcnt`, D3); verify with an emulator test that a program run under Kickstart 1.3 and 3.x writes the file.
- [ ] 2.2 Emit `addq.l #1,table+4*index` with relocations in `dostatement` for BASIC statements only, none for inline assembler lines (D2); check that no code between BASIC statements depends on the condition codes (read the generator; if one does, use the flag-neutral counter of D2 there); verify with corpus programs compiled with the mode, including ones with inline assembler, that results of the programs are unchanged.
- [ ] 2.3 Test the merge (two runs add, other compile replaces), `End`, runtime error and a program with 20000 statements; verify with emulator tests that read the count file.

## 3. Report and runners

- [ ] 3.1 Write `blitzcov` and `tools/lib/blitzcov` with host tests (line = largest count, `FN`/`FNDA`, merge of several files, mismatched id, truncated count file, `--strip`).
- [ ] 3.2 Add the coverage switch to the `ab3/cli` and corpus runners and the lcov output under `<build>/coverage/lcov/<test>.info`; verify with one test of each runner that the file exists and names its Blitz sources.
- [ ] 3.3 Document the option in `docs/ab3/compiler.md` and the README, note the BB2/AB3 differences, add a `TODO.md` entry; verify that the documented commands run as written.
