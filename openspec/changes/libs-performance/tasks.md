Each numbered group is one implementer round; each task is one commit
unless it says otherwise. Every commit that changes a library file names the
changed results (`PREV_INTENDED`) and the measured cycles before and after.

## 1. Benchmark and preset

- [x] 1.1 Decide with the first measurement where the benchmark sources live (design.md, Open Questions); record the original's value next to each budget as the target; write the decision into design.md
- [x] 1.2 Bring the measurement method of `perf-analysis/` (bench sources, CIA-B timer cascade, empty loop subtraction, cycle-exact A500 configuration) into the repository as a test, in C/CMake and Blitz sources only (no Python); one run at a time
- [x] 1.3 Add the `perf` preset to `CMakePresets.json` (label `perf`, not in `all`, `host` or `emulator`) and let the test skip with 77 without Amiberry or data
- [x] 1.4 Add the budget file with the measured values of the current build per command plus a tolerance, covering the 100 commands slower than the originals; a broken budget (a value below the measurement) must make the test fail with the command and both values; add that test with deliberately broken input
- [x] 1.5 Document the preset in `tests/README.md` and `docs/testing.md`

## 2. Register rule

- [ ] 2.1 Verify the exceptions in the generated code: disassemble `EventWindow`, `RawStatus`, `Rnd`, `WBWidth`, `WBHeight`, `WBDepth` as aliases of the C functions and write down which registers each keeps by nature; decide per entry: alias, reduced thunk or assembler block (the list is in `analyse-thirdparty-regs.md` section 5)
- [ ] 2.2 Change the KEEPS judge and the statements of the library tests to the new rule (a4 to a7, a3 for functions, the exceptions' registers and the upper word of d0 where stated); keep the result and change cases; update the summary and `tests/README.md` ("Kept registers"); add a test with deliberately broken input (a function that loses a6 fails; a statement that changes d1 passes)
- [ ] 2.3 Replace the thunks of the user-level statements and functions without exception by aliases of the C functions, one commit per library group; keep internal tokens, nullsubs and error checks; leave banklib `AllocMem` as it is (#1059); measure with the benchmark; run `ctest --preset all` and `ctest --preset bug-orig`
- [ ] 2.4 Implement the exceptions as decided in 2.1, each with a KEEPS statement of exactly the needed registers; `InitSound` keeps a0 (this is the fix of #1058: its own commit with the test of the expected behaviour and the line `Fixes #1058`); `EMouseX`, `EMouseY` and `EventCode` become assembler entries `move.w x,d0 : rts` as in the original (they keep a0 and the upper word of d0 by their code, design.md decision 3); `ExecVersion` returns a zero-extended long
- [ ] 2.5 Add the kind-B deviation to `docs/libs/deviations.md` with the rule, the exceptions and their callers, and name a6; update `docs/libs/cmdlibs-port.md` and the website page of differences from the original; give the fix-scope result if a fix commit is involved
- [ ] 2.6 Experiment `__entrypoint` (m68k-amigaos-gcc: the function saves no callee-saved register) for the user-level statements and functions that 2.3 made aliases: first count how many of them save d2-d7/a2 and measure the gain in the benchmark; d2-d7 and a2 need not be kept (rule), but `__entrypoint` also gives a6 free (seen: gcc used a6 unsaved), so add a build check that refuses an `__entrypoint` routine whose code writes a6 without restoring it, with a test with deliberately broken input; the KEEPS test checks a6 at run time; never for internal tokens (they keep d2-d7); adopt it only where the measurement shows a gain (user decision 2026-11-15)

## 3. Speed profile

- [ ] 3.1 Add strings1lib and strings2lib to `ACIDLIBS_SPEED` (`libs/acidlibs/CMakeLists.txt`); measure string assignment, concat, compare, Len; name the changed library files
- [ ] 3.2 Add medlib and trackerlib to `ACIDLIBS_SPEED`; measure PlayMed and PlayModule per frame; name the size change of `acidlibs`

## 4. Long and quick division

- [x] 4.1 Replace the `divide` routine of ldivlib by an assembler block: a fast path for a divisor that fits in 16 bits (two `divu`, about 300 cycles instead of the original's 2661 for Long) and the original's loop (about 14 instructions, `ldivlib` of the CD) for a 32-bit divisor; the same for Quick division (16.16) and the Long and Quick Mod; keep the entries, the sign rules and the #200 fix bit for bit; no error check is added (#1048); measure Long, Quick and Mod division and tighten the budgets (user decision 2026-11-15: faster than the original where the 68000 allows it)
- [x] 4.2 Run the library tests, the corpus and bug-orig; check division results of the corpus unchanged

## 5. String copy loops

- [ ] 5.1 Replace the copy loop of `copstring` (strings1lib) by the unrolled assembler loop of the original; keep the contract of the internal token (d0-d7)
- [ ] 5.2 Replace the copy loop of `stringass` (strings2lib) likewise; put the allocation path of `assign` in its own `noinline` function so that the common path saves no registers; keep d1-d7 and a2 of `lastring`/`astring`; both entries need no thunk then (task 8.3)
- [ ] 5.3 Measure assignment, concat, compare, Len, Mid$, Left$, Right$, Asc, Chr$, Inkey$; tighten the budgets

## 6. Small entries

- [ ] 6.1 Peek and Poke (memacclib), Plot and Point (scrolllib) as assembler routines of the original
- [ ] 6.2 Joyx, Joyy, Joyb, Joyr (gameiolib) and MouseX, MouseY, MouseXSpeed, MouseYSpeed (mouselib)
- [ ] 6.3 RawStatus (blitzkeyslib, keeping a1 as decided in 2.1), QLimit and Rnd (vallib, keeping a0 and a1 for Rnd)
- [ ] 6.4 DoColl, SColl, PColl (collslib), ShapeWidth, ShapeHeight, Handle (shapeslib), Colour and Locate (fontlib)
- [ ] 6.5 GadgetHit, MenuHit, ItemHit, SubHit, EventQualifier (windowslib) as assembler entries of the same form as EMouseX (task 2.4)
- [ ] 6.6 Sgn, Int, Abs (vallib): the float case in its own `noinline` function, or an assembler entry if that is not enough
- [ ] 6.7 For each routine of 6.1 to 6.6 check that a library test case covers its entry conditions; add cases where the original leaves one undefined; tighten the budgets

## 7. Blit core and sortlib

- [ ] 7.1 Measure Blit, QBlit, BBlit/UnBuffer, ClipBlit, Block, BlitColl first; decide per file between the original's set-up and plane loop as an assembler block and `always_inline` helpers with register state; do not fix #1045 here, but do not prevent a cost-free fix
- [ ] 7.2 Implement the decision for blitlib, qblitlib, bblitlib, 2dlib ClipBlit and stencillib, one commit per library
- [ ] 7.3 sortlib: one scan loop per element type (byte, word, long, quick inline; float and string through a call); keep the results of every element type (library test cases per type, stable order where the original is stable)
- [ ] 7.4 Measure and tighten the budgets for the blit and Sort commands

## 8. Internal tokens

- [x] 8.1 Analysis first: which registers and flags the third-party callers of the internal tokens `globalloc`, `globfree` ($C002/$C003, 41/37 libraries), `allocst`, `freest` ($CF01/$CF02) and `fltoqu` ($D301) rely on across the call, the same method as `analyse-thirdparty-regs.md`; #1059 (the Z flag after AllocMem) belongs here - done (2026-11-15): `~/.local/share/bb2-c-work/analyse-internal-token-callers.md`
- [ ] 8.2 Cut the thunks of the internal tokens to the contract of `thunks.md` table 1.4 and the 8.1 analysis: no thunk for Mod ($D400-04), qutofl ($D300), the power routine, arrmult/newarr and maxlen; clrloc, newmem and localstat save d0-d1 only; third-party contracts (8.1): globalloc keeps a0-a1 and leaves Z clear after success (#1059 is fixed separately), globfree keeps d1/a0, allocst keeps a0-a1 (d1 can go), freest keeps d0/a0-a1 (d1 can go; d0 for LotanArray ArSet), fltoqu keeps a0-a1 (d1 can go), longmult keeps a0 (no thunk; the routine must not use a0/a1); or write these entries as assembler that does not touch those registers; each with a KEEPS statement whose reason names the third-party caller
- [ ] 8.3 `strcomp` ($CE00, keeps d0-d7), `freelast` ($C005, keeps d0 and the flags), the procedure prologue routines and the conversions `qutofl`/`fltoqu` ($D300/$D301, the original's code: `BCHG #7,d0`, the mathffp calls through `jsr -xx(a6)` and `NEG.l d0`, never touching a0/a1, so that the third-party callers of 8.1 keep a0-a1 without a thunk; user decision 2026-11-15) as assembler entries that do not touch the registers of their contract, so they need no thunk; `copstring` and `lastring`/`astring` get the same in group 5
- [ ] 8.4 Measure string compare, procedure calls with locals, Mod and the Quick/Float conversions; tighten the budgets

## 9. Closing

- [ ] 9.1 Check `libs/register-contract`, `libs/performance`, `testing/performance-budgets` and the `testing/library-capture` delta against the implementation; adjust the text where decisions changed it
- [ ] 9.2 Run `ctest --preset all`, `ctest --preset bug-orig`, `ctest --preset originals` (needs the CD) and `ctest --preset perf`; run `openspec validate libs-performance`; record the final measurement table next to the old one in `~/.local/share/bb2-c-work/perf-analysis/`
