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

- [x] 2.1 Verify the exceptions in the generated code: disassemble `EventWindow`, `RawStatus`, `Rnd`, `WBWidth`, `WBHeight`, `WBDepth` as aliases of the C functions and write down which registers each keeps by nature; decide per entry: alias, reduced thunk or assembler block (the list is in `analyse-thirdparty-regs.md` section 5)
- [x] 2.2 Change the KEEPS judge and the statements of the library tests to the new rule (a4 to a7, a3 for functions, the exceptions' registers and the upper word of d0 where stated); keep the result and change cases; update the summary and `tests/README.md` ("Kept registers"); add a test with deliberately broken input (a function that loses a6 fails; a statement that changes d1 passes)
- [x] 2.3 Replace the thunks of the user-level statements and functions without exception by aliases of the C functions, one commit per library group; keep internal tokens, nullsubs and error checks; leave banklib `AllocMem` as it is (#1059); measure with the benchmark; run `ctest --preset all` and `ctest --preset bug-orig`
- [x] 2.4 Implement the exceptions as decided in 2.1, each with a KEEPS statement of exactly the needed registers; `InitSound` keeps a0 (this is the fix of #1058: its own commit with the test of the expected behaviour and the line `Fixes #1058`); `EMouseX`, `EMouseY` and `EventCode` become assembler entries `move.w x,d0 : rts` as in the original (they keep a0 and the upper word of d0 by their code, design.md decision 3); `ExecVersion` returns a zero-extended long
- [x] 2.5 Add the kind-B deviation to `docs/libs/deviations.md` with the rule, the exceptions and their callers, and name a6; update `docs/libs/cmdlibs-port.md` and the website page of differences from the original; give the fix-scope result if a fix commit is involved
- [x] 2.6 Experiment `__entrypoint` (m68k-amigaos-gcc: the function saves no callee-saved register) for the user-level statements and functions that 2.3 made aliases: first count how many of them save d2-d7/a2 and measure the gain in the benchmark; d2-d7 and a2 need not be kept (rule), but `__entrypoint` also gives a6 free (seen: gcc used a6 unsaved), so add a build check that refuses an `__entrypoint` routine whose code writes a6 without restoring it, with a test with deliberately broken input; the KEEPS test checks a6 at run time; never for internal tokens (they keep d2-d7); adopt it only where the measurement shows a gain (user decision 2026-11-15)

## 3. Speed profile

- [x] 3.1 Add strings1lib and strings2lib to `ACIDLIBS_SPEED` (`libs/acidlibs/CMakeLists.txt`); measure string assignment, concat, compare, Len; name the changed library files
- [x] 3.2 Add medlib and trackerlib to `ACIDLIBS_SPEED`; measure PlayMed and PlayModule per frame; name the size change of `acidlibs`

## 4. Long and quick division

- [x] 4.1 Replace the `divide` routine of ldivlib by an assembler block: a fast path for a divisor that fits in 16 bits (two `divu`, about 300 cycles instead of the original's 2661 for Long) and the original's loop (about 14 instructions, `ldivlib` of the CD) for a 32-bit divisor; the same for Quick division (16.16) and the Long and Quick Mod; keep the entries, the sign rules and the #200 fix bit for bit; no error check is added (#1048); measure Long, Quick and Mod division and tighten the budgets (user decision 2026-11-15: faster than the original where the 68000 allows it)
- [x] 4.2 Run the library tests, the corpus and bug-orig; check division results of the corpus unchanged

## 5. String copy loops

- [x] 5.1 Replace the copy loop of `copstring` (strings1lib) by a C loop that reaches the original (longs or 8 bytes per pass, `BL_STRCOPY`; user decision 2026-11-15: C first, assembler only where C stays more than about 10 % slower); keep the contract of the internal token (d0-d7)
- [x] 5.2 Replace the copy loop of `stringass` (strings2lib) likewise, in C with a thunk that saves only d1; put the allocation path of `assign` in its own `noinline` function so that the common path saves no registers; keep d1-d7 and a2 of `lastring`/`astring`; both entries need no thunk then (task 8.3)
- [x] 5.3 Measure assignment, concat, compare, Len, Mid$, Left$, Right$, Asc, Chr$, Inkey$; tighten the budgets

## 6. Small entries

- [x] 6.1 Peek and Poke (memacclib), Plot and Point (scrolllib) as assembler routines of the original
- [x] 6.2 Joyx, Joyy, Joyb, Joyr (gameiolib) and MouseX, MouseY, MouseXSpeed, MouseYSpeed (mouselib)
- [x] 6.3 RawStatus (blitzkeyslib, keeping a1 as decided in 2.1), QLimit and Rnd (vallib, keeping a0 and a1 for Rnd)
- [x] 6.4 DoColl, SColl, PColl (collslib), ShapeWidth, ShapeHeight, Handle (shapeslib), Colour and Locate (fontlib)
- [x] 6.5 GadgetHit, MenuHit, ItemHit, SubHit, EventQualifier (windowslib) as assembler entries of the same form as EMouseX (task 2.4)
- [x] 6.6 Sgn, Int, Abs (vallib): the float case in its own `noinline` function, or an assembler entry if that is not enough
- [x] 6.7 For each routine of 6.1 to 6.6 check that a library test case covers its entry conditions; add cases where the original leaves one undefined; tighten the budgets

## 7. Blit core and sortlib

- [x] 7.1 Measure Blit, QBlit, BBlit/UnBuffer, ClipBlit, Block, BlitColl first; decide per file between the original's set-up and plane loop as an assembler block and `always_inline` helpers with register state; do not fix #1045 here, but do not prevent a cost-free fix
- [x] 7.2 Implement the decision for blitlib, qblitlib, bblitlib, 2dlib ClipBlit and stencillib, one commit per library
- [x] 7.3 sortlib: one scan loop per element type (byte, word, long, quick inline; float and string through a call); keep the results of every element type (library test cases per type, stable order where the original is stable)
- [x] 7.4 Measure and tighten the budgets for the blit and Sort commands

## 8. Internal tokens

- [x] 8.1 Analysis first: which registers and flags the third-party callers of the internal tokens `globalloc`, `globfree` ($C002/$C003, 41/37 libraries), `allocst`, `freest` ($CF01/$CF02) and `fltoqu` ($D301) rely on across the call, the same method as `analyse-thirdparty-regs.md`; #1059 (the Z flag after AllocMem) belongs here - done (2026-11-15): `~/.local/share/bb2-c-work/analyse-internal-token-callers.md`
- [x] 8.2 Cut the thunks of the internal tokens to the contract of `thunks.md` table 1.4 and the 8.1 analysis: no thunk for Mod ($D400-04), qutofl ($D300), the power routine, arrmult/newarr and maxlen; clrloc, newmem and localstat save d0-d1 only; third-party contracts (8.1): globalloc keeps a0-a1 and leaves Z clear after success (#1059 is fixed separately), globfree keeps d1/a0, allocst keeps a0-a1 (d1 can go), freest keeps d0/a0-a1 (d1 can go; d0 for LotanArray ArSet), fltoqu keeps a0-a1 (d1 can go), longmult keeps a0 (no thunk; the routine must not use a0/a1); or write these entries as assembler that does not touch those registers; each with a KEEPS statement whose reason names the third-party caller
- [x] 8.3 `strcomp` (copstring and lastring/astring keep their minimal thunks of group 5, d0-d1 and d1; the assembler variant on branch `g5-asm` would drop them for 40-100 cycles, only if a measurement asks for it) ($CE00, keeps d0-d7), `freelast` ($C005, keeps d0 and the flags), the procedure prologue routines and the conversions `qutofl`/`fltoqu` ($D300/$D301, the original's code: `BCHG #7,d0`, the mathffp calls through `jsr -xx(a6)` and `NEG.l d0`, never touching a0/a1, so that the third-party callers of 8.1 keep a0-a1 without a thunk; user decision 2026-11-15) as assembler entries that do not touch the registers of their contract, so they need no thunk; `copstring` and `lastring`/`astring` get the same in group 5
- [x] 8.4 Measure string compare, procedure calls with locals, Mod and the Quick/Float conversions; tighten the budgets

## 10. Remaining slow commands

The commands still 10 % or more slower than the original after groups 2 to 8 and not covered there (measured 2026-11-15). Per task: find the hot part by measurement, then an assembler block or a C rework; keep the results; tighten the budgets.

- [x] 10.1 Text output and input: Print (string and number), Locate, Inkey$ (printlib, fontlib, the input libraries)
- [x] 10.2 Graphics primitives of 2dlib: Circle, Circlef, Box, Boxf, Line, Poly, Polyf, Green, RGB
- [x] 10.3 Collisions: RectsHit, ShapesHit, SpritesHit
- [x] 10.4 Display and palettes: Show, Show x,y, DisplayPalette, ShowPalette, Use Palette, PalRGB, FadePalette, DisplayBitMap, DisplaySprite, ShowSprite, Scroll
- [x] 10.5 Frac (vallib) and the per-frame interrupts of medlib and trackerlib (PlayMed, PlayModule) - done (2026-11-16): Frac in 10.9 (#1064), the players in 10.10 and 10.12 (PlayModule about 2116 against 2410, PlayMed 9795 against 9950)
- [x] 10.6 Measure the whole benchmark again and list every command still 10 % or more slower than the original, with the reason - done (2026-11-16): three runs of `ctest --preset perf` and one of `perf-original`, table in `~/.local/share/bb2-c-work/perf-analysis/final-table.md`: of 131 commands 4 are 10 % or more slower (Val 1.63 vallib, Volume 1.37 audiolib, Str$ 1.20 printlib, Instr 1.17 stringfunclib; real gaps, none noise or frame loops), 18 are 2 to 10 % slower, 62 within 2 %, 47 faster

- [x] 10.7 Sort with the original's tail recursion: the right part as a loop around the body, only the left part a recursive call (the original: `BRA 'recurs`, two longs per level); the same results, since the two parts are sorted independently (user decision 2026-11-15)
- [x] 10.8 BBlit, UnBuffer and QBlit: keep the bitmap pointer, the plane step and the other values that are only added and written to the custom registers as `ULONG` in data registers (pinned with `register ... __asm("dN")` where needed), as the original does with d1/d3/d6/d7, so that GCC does not spill them to the stack in the plane loop; C, assembler only if C stays more than 1-2 % behind the original (user decision 2026-11-15)

- [x] 10.9 Frac with an argument of unknown type (`BL_UNKNOWN`, as Int, Sgn and Abs), still a Float result: a Quick argument builds the FFP result directly from its fraction word (normalise, mantissa, exponent, sign) without `qutofl` and the mathffp calls; the Float argument keeps its path (a bit-mask variant instead of SPFix/SPFlt/SPSub only if it gives the same bits). The Quick path does not reproduce the original's loss of low fraction bits for large integer parts (qutofl's 24-bit mantissa): the result is the exact fraction (user decision 2026-11-15); this is an intended deviation, documented in docs/libs/deviations.md and on the website, with the library cases where it differs as a pair; the compiled code of every program that uses Frac changes, the corpus outputs must not

- [x] 10.10 The players (medlib, trackerlib) by tuning the C source: an analysis first compares the original's replay code with GCC's output and counts the cycles per pattern (sign/zero extension from integer promotion, reloads of the channel state from memory, `volatile` reloads, `switch` against the original's jump table, stack spills for lack of address registers); then the C is reworked pattern by pattern (types that avoid extension, the channel state in locals and `restrict`, values in data registers, `goto *tab[cmd]` for the effect dispatch), measured after each step; assembler only where C stays more than 1-2 % behind (user decision 2026-11-15) - done (2026-11-16): trackerlib in C below the original; medlib finished by the assembler line routine of 10.12 (design decisions 24 and 25)

- [x] 10.11 Polyf (2dlib): `prodgt` as `BL_ALWAYS_INLINE` without the zero extensions the value range makes unnecessary; then measure and tune the C clipping of Polyf (the fixes #1000 and #1033 stay) as the players in 10.10: extensions, reloads, spilled registers; C as long as it comes within 1-2 % of the original (user decision 2026-11-16)

- [x] 10.12 medlib: newline and playnote as an assembler line routine in the original's form (PlayMed stays 1.05 times the original after the C tuning of 10.10, design decision 24), keeping the player state in a3-a5 under the saving thunk

## 11. The last slow commands (after the final measurement of 10.6)

The four commands still 10 % or more slower than the original in `perf-analysis/final-table.md`, and SColl again with what groups 2-10 showed (user decision 2026-11-16). Same method: find the cost in the disassembly against the original, C first, assembler only where C stays more than 1-2 % behind; results bit for bit.

- [x] 11.1 Val (vallib `getdec`: its state in memory, the loop in C; 1.63 times) - done (2026-11-16): C, the state in registers, the mathffp calls declare only d0/d1/a0/a1 changed, `BLITZ_ENTRYPOINT`; 3138.2 -> 1670.2 (original 1927.0), design decision 27
- [x] 11.2 Str$ (printlib; 1.20 times) - done (2026-11-16): C, the output call inline in the entries (one register save instead of three), the conversions return their text, the handler `strprint` the original's assembler; 4964.7 -> 4164.6 (original 4131.5), design decision 27
- [x] 11.3 Instr (stringfunclib, the search loop; 1.17 times) - done (2026-11-16): C, inline in `BLITZ_ENTRYPOINT` entries, CaseSense tested once, the first character tested before the rest; 4119.2 -> 2281.7 (original 3513.0), design decision 27
- [x] 11.4 Volume (audiolib, the mask loop of four passes; 1.37 times) - done (2026-11-16): the four channels unrolled in C, 382.1 -> 204.1 cycles (the original 278.7; design decision 26)
- [x] 11.5 SColl (collslib): look again for extensions, absolute addressing, spills and the #110 fix's cost; the fix of #110 stays - done (2026-11-16): one path, the #110 fix as NEG/NEG/ADDX instead of ADDQ/ADD, 136.0 -> 124.0 cycles (the original 126.3; design decision 26)
- [x] 11.6 Measure, tighten the budgets, update final-table.md - done (2026-11-16): three runs of `ctest --preset perf` on main 4375d842 (the originals of 10.6 stay valid, no bench program changed), table in `~/.local/share/bb2-c-work/perf-analysis/final-table.md` (10.6's as `final-table-10.6.md`): of 131 commands none is 10 % or more slower (the closest Inkey$ 1.10, 9.8 %), 17 are 2 to 10 % slower, 63 within 2 %, 51 faster; no budget reached or exceeded and none of group 11 stale, `budgets.txt` unchanged

## 12. A light call form for system libraries and memlib (analysis: ~/.local/share/bb2-c-work/analyse-sysjsr.md)

- [x] 12.1 `BLITZ_SYSJSR`, `BLITZ_SYSJSR1`/`2` (and a d0/a1 form if needed) in blitzlib.h: only d0/d1/a0/a1 clobbered, a6 input only; vallib's FFPJSR1/2 move onto it; `BLITZ_BASEJSR1/2` and their texts go - done (2026-11-16): `BLITZ_SYSJSR` (d0/d1/a0/a1 in and out), `BLITZ_SYSJSR1`/`2`; memlib locfree takes the general form (no d0/a1 form: it costs only `moveq #0,d1`/`move.l d1,a0` on the rare free path); vallib.obj stays byte-identical with `memory`, so bstr.val is unchanged
- [x] 12.2 strings2lib and maxlenlib call memlib's table with it (tested contract, systest KEEPS); measure bstr.pstr/bstr.fstr and tighten their budgets - done (2026-11-16): `BLITZ_SYSJSR2` for localloc, `BLITZ_SYSJSR` for locfree; `stringass_new` saves d2/d3/a6 instead of d2-d7/a2/a6 and keeps no frame; bstr.pstr 11989.6 -> 11560.6 (original 11614.0), bstr.fstr 12688.1 -> 12252.6 (12353.5), two runs; budgets 11680 and 12379
- [x] 12.3 The system-library calls (exec, graphics, diskfont, intuition, rexxsyslib, translator; BL_FREESIGNAL) use it; BLITZ_BASEJSR stays for the open-contract calls (fontlib previous handler), sortlib FFP_CC_, runerrlib spfix and gtlib addglist stay private, libs/userlibs/rwe unchanged - done (2026-11-16): fontlib (CloseFont twice, OpenDiskFont), intuifontlib (CloseFont, OpenDiskFont), environlib (GetScreenData), arexxlib (ErrorMsg, the general form for its a0), speaklib (Translate) take `BLITZ_SYSJSR`; `BL_FREESIGNAL` is `BLITZ_SYSJSR1` and seriallib uses it (utillib and brexxlib change with it); per function the save of d2-d7/a2/a6 becomes one of a6 (-124 cycles; environlib and fontlib `doloadfont` keep d2/a2/a6 for their own use, brexxlib a2/a6); fontlib's previous input handler keeps `BLITZ_BASEJSR`
- [x] 12.4 Update final-table.md - done (2026-11-16): `perf-analysis/final-table.md` has the new `bstr.pstr`/`bstr.fstr` rows and counts (2 to 10 % slower 17 -> 15, within 2 % 63 -> 65); two runs of the whole perf preset within every budget; design decision 28

## 13. The remaining slower commands

Commands that are below the 10 % limit of group 11 but still slower than the original (the goal: not slower than the original). Same method: the cost in the disassembly against the original, C first; results bit for bit, the earlier fixes kept; library cases recorded from the original for the changed paths.

- [x] 13.1 Left$, Mid$ and Right$ (stringfunclib, analysis `~/.local/share/bb2-c-work/analyse-g13-strings.md`): the helpers `left`, `right` and `mid` `BL_ALWAYS_INLINE` (nested functions with stack arguments); `BLITZ_ENTRYPOINT` where it gains (a2 may change, as with Instr), with the KEEPS statements and the epcheck build check
- [x] 13.2 Hex$: a rotate instead of the variable shift by 28 and a 16-byte PC-relative digit table
- [x] 13.3 Bin$: the same treatment, at the original's 36 cycles per digit (`addx` if needed); a new benchmark line `bstr.bin` with the original's value from `perf-original`
- [x] 13.4 Long results of Mid$ and Right$: not slower than the original's 8-byte copy (movestring); one shared out-of-line copy if the short case stays at or below the original; a benchmark line with a long Mid$
- [x] 13.5 Cvi, Cvl and Cvq: `cvbytes` inline in the entries (the nested-function pattern)
- [ ] 13.10 Blit and BlitColl (blitlib) in C (analysis `~/.local/share/bb2-c-work/analyse-g13-blit.md`): the original's modulo pair (`MOVEQ #-2:SWAP:MOVE bpr:SUB ebwidth:SUBQ #2`), the shift as a rotate, the next plane's address computed while the blitter runs, `bcon0` PC-relative, BlitColl's result as a long; Blit with Excessonoff gets a Blitz mode entry that saves nothing and its bitmap depth read once; BlitColl becomes an assembler block only if it does not measure below the original (881.4); results bit for bit, the Amiga mode BlitColl's upper word (kind T) included; not slower than the original for 1, 3 and 6 planes (perf lines `bgfx.blit16p1`, `blit16p6`, `blitex`, with their original values from `perf-original`)
- [ ] 13.11 ClipBlit (2dlib) as an assembler block from the analysis's draft (`clipblit-draft.s`), with the `blitzlib.h` macros as 2dlib's other blocks: the original's `b_docblit`/`b_docblitx` with short branches, `TST.b` as the first blitter read, the next plane prepared during the blit; it keeps the leftover `MOVEM.l d6-d7` to BLTxMOD; `systest_blit` covers a 1-plane shape and the clipping at all four edges, recorded from the original with `libs-capture` and confirmed with `libs_emulator_originals`; perf lines for ClipBlit clipped at the four edges and with Excessonoff (`bgfx.clipblitc`, `clipblitex`)
- [ ] 13.12 QBlit with FlushQueue (`bgfx.qblit8f`, 21 cycles over the original): compare qblitlib, bblitlib and stencillib with the originals by the same method and fix what shows
- [x] 13.30 lmullib `longmult` as an assembler entry (analysis `~/.local/share/bb2-c-work/analyse-g13-mul.md`, listing L2): only the sign of b tested (a negative b negates both factors), MULU by the words of |b| as in the original, paths for zero high words; results bit for bit (#199 fixed), d2-d7/a0-a6 kept; library cases for every path; measure `bmath.lmul` and a new `bmath.lmuls` (100*20) and tighten the budgets - done (2026-11-16): `bmath.lmul` 408.6 -> 284.6 (original 383.0), `bmath.lmuls` 184.6 (original 370.0)
- [x] 13.31 lmullib `quickmult` as an assembler entry (listing Q1): the sign in the code path (four entries, two copies of the core), no flag register, MULU by the words of |b| as in the original; results bit for bit (#199 fixed), d2-d7/a0-a6 kept; library cases for every path; measure `bmath.qmul` and tighten its budget - done (2026-11-16): `bmath.qmul` 458.1 -> 386.8 (original 434.0)

## 9. Closing

- [x] 9.1 Check `libs/register-contract`, `libs/performance`, `testing/performance-budgets` and the `testing/library-capture` delta against the implementation; adjust the text where decisions changed it - done (2026-11-16): the speed profile is six libraries (strings1lib/strings2lib are not in it); "C first, assembler where C stays behind" and "Slow commands are listed" in `libs/performance`; internal tokens keep contracts without thunks, `__entrypoint` with `no_icf` and its build check in `libs/register-contract`; the budgets in the `B~T` form, warp mode and parallel sessions in `testing/performance-budgets`; the `library-capture` delta covers the acidlibs only (the RWE userlib programs keep their sets); proposal and design decisions 2 and 6 name the later state; `openspec validate libs-performance --strict` passes
- [x] 9.2 Run `ctest --preset all`, `ctest --preset bug-orig`, `ctest --preset originals` (needs the CD) and `ctest --preset perf`; run `openspec validate libs-performance`; record the final measurement table next to the old one in `~/.local/share/bb2-c-work/perf-analysis/` - done (2026-11-16): report `~/.local/share/bb2-c-work/libs-performance-acceptance-2.md` (prev 848bb9d1; all 708, bug-orig 4 known DIFFERS, originals, perf three runs; the stale brexxlib-playwait-noarg.orig re-recorded and the bgfx.qblit8f budget widened afterwards), final table `perf-analysis/final-table.md`
