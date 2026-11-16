## Context

Sources: `~/.local/share/bb2-c-work/perf-analysis/decisions.md` (all
decisions), `report.md` (the measurements), `thunks.md` (register
analysis), `~/.local/share/bb2-c-work/analyse-thirdparty-regs.md`
(third-party and AB3 callers). Facts used here:

- 537 `BLITZ_THUNK` entries; 445 of 661 entries pay 86 cycles or more
  before the C function starts, the originals nothing (`report.md`, cause T).
- gcc saves d2-d7/a2/a6 on every path of a function that contains a
  `BLITZ_BASEJSR`/`BLITZ_ALIBJSR` (cause R), e.g. vallib Sgn/Int/Abs.
- The compiler's generated code relies on a3 to a7 only (`thunks.md` 1.2).
  Internal tokens have their own contracts (1.4): operators keep d2-d7,
  strcomp/copstring d0-d7, `lastring`/`astring` d1-d7 and a2, the procedure
  prologue routines d0-d5, freelast d0 and the flags, error checks all.
- The experiment with `-fcall-saved-d1/a0/a1` failed because gcc 16.2
  miscompiles tail calls with it (the epilogue restores d1/a0/a1 before the
  `jmp`); it is not a path (`thunks.md` 0 and 5).
- The callers that need registers: `analyse-thirdparty-regs.md` sections 0
  and 5 (eight entries).

## Goals / Non-Goals

**Goals:** the C libraries come as close to the originals' speed as the
decisions allow, without changing what any command does; the register rule
is the documented one; the speed is measured by a test that fails on a
regression.

**Non-Goals:** no change of observable behaviour; no new error checks; no
fix of the issues listed in the proposal; no `-fcall-saved` ABI flag; no
other optimisation level (the libraries stay at `-Os` except the speed
profile); no byte comparison of the changed library files with the prev
build.

## Decisions

1. **The register rule is Sibly's.** Every user-level entry keeps a4, a5, a6,
   a7 and (functions) a3; nothing is guaranteed for d0, d1, a0, a1, and the
   upper word of d0 of a word function is undefined. Alternative: a general
   rule "functions keep a0/a1", suggested by the three CD programs under
   inline assembler; the user decided for a list of exceptions instead.
2. **The default is an alias of the C function.** gcc keeps d2-d7 and a2-a6,
   which covers the rule. The thunk is removed for user-level statements and
   functions only. Internal tokens keep their contracts but, after group 8
   (Decision 17), most have no thunk any more (operators, conversions,
   strcomp, the prologue routines and freelast are aliases or assembler
   entries; copstring, lastring/astring, newmem and localstat keep minimal
   thunks); nullsubs and all error checks (`BLITZ_ERRCHK_THUNK`) keep
   their thunks.
3. **Exceptions only for known callers.** Per entry the analysis names the
   caller. Where gcc's output keeps the register by nature the entry is an
   alias and the KEEPS test states the register: if a compiler change makes
   gcc use it, the test fails instead of a third-party library. Otherwise
   (InitSound; possibly EventWindow and RawStatus) a reduced thunk that saves
   exactly the needed registers, or an assembler block when the entry is hot
   (Rnd, RawStatus). Each exception is verified by disassembling the
   generated code, not by reading the C.
   A getter of two instructions whose behaviour a caller relies on
   (EMouseX, EMouseY, EventCode) is an assembler entry as in the original,
   `move.w x,d0 : rts`, not an alias (user decision 2026-11-15). The code
   is as short as the C function's, and the guarantee (only d0.w is
   written) is in the code instead of depending on gcc's choice; the KEEPS
   test stays as a check. The other event getters of windowslib
   (GadgetHit, MenuHit, ItemHit, SubHit, EventQualifier) get the same form
   with the small entries.
4. **Word functions** keep the upper word of d0 only where a caller needs it
   (EMouseY keeps it; EventCode needs zero or kept; both are assembler
   entries, point 3): the entries with
   `BLITZ_THUNK_POST(.., "move.w d0,2(sp)")` that have no such caller become
   aliases. `ExecVersion` returns a zero-extended long (the analysis: a
   caller compares the word as a long).
5. **Assembler blocks are the regular fix for hot routines** (precedents:
   SColl, Scale). A block is the original's algorithm (from the CD's
   library, extracted with `blitzlib extract`), with the fixes of decided
   bugs kept at the entry (the ldivlib fix of #200). A block keeps the
   entry's rule of Decision 1 or its exception. The order is the decided one:
   the ldivlib loop, the string copy loops, the small entries, the blit core
   and sortlib. Each step is measured by the benchmark before and after.
6. **Speed profile** (final state: medlib and trackerlib join; strings1lib
   and strings2lib are not in it after their copy loops were rewritten,
   Decision 15; the text below is the measurement of task 3.1/3.2 as made)
   adds strings1lib, strings2lib (measured +8 and +4
   bytes, -510 cycles per 21 character assignment), medlib (-630 per PlayMed
   frame, +776 bytes) and trackerlib (-220 per PlayModule frame, +212 bytes)
   to `ACIDLIBS_SPEED` (`libs/acidlibs/CMakeLists.txt:29`). The rest stays
   `-Os`. Measured in `tests/perf` (task 3.1): strings1lib gains about 360
   cycles per string fetch (`r$=a$` 2521.5 -> 2160.0, Len 1228.0 -> 864.0,
   concat and compare about 720) for 8 bytes and slows no command, so it is
   in the list. strings2lib is left out: its `-O2` copy loop has a longer
   set-up (a `tst.l` of the length, the `dbf` split and an extra branch on
   the reuse path), so a 1 character assignment gets slower (`Chr$` 807.0
   -> 820.0) while longer ones gain (`r$=a$` another 150, String$ 140);
   task 5.2 replaces that loop by the original's assembler anyway.
   medlib and trackerlib are in the list (task 3.2, five runs each): PlayMed
   12854.8 -> 12222.2 cycles per frame (original 9950.0) for 776 bytes
   (12388 -> 13164), the PlayModule interrupt 3214.0 -> 3106.0 per frame
   (original 2409.5; the five runs within 7 cycles) for 212 bytes
   (3356 -> 3568); `acidlibs` grows by 988 bytes (285436 -> 286424).
7. **Rare paths out of line.** Where the cost is gcc's register save on a rare
   path (Sgn/Int/Abs float case, the allocation in `assign`), the rare part
   becomes a `noinline` function; an assembler entry is the alternative if
   that is not enough.
8. **The KEEPS judge.** The kept set of a routine is a4, a5, a6, a7 (a3 for
   functions) plus the registers of its stated exception. Today the judge
   states the intersection of what the original keeps (`tests/README.md`,
   "Kept registers"); the new one states the rule, so a library may keep
   fewer of the accidental registers without the test failing, and the
   exception entries still fail if they lose a needed register. The result
   and change cases stay. The summary of the registers the original keeps
   stays as information. The existing `KEEPS` statements (about 1016) are
   reduced mechanically.
9. **Benchmark `perf`.** A test runs one `For i=1 To N` loop per command in
   the cycle-exact A500 configuration of the measurement (`cpu_cycle_exact`,
   `cpu_memory_cycle_exact`, `blitter_cycle_exact`, `cpu_speed=real`,
   Kickstart 1.3, runtime error checking off), reads a CIA-B timer cascade,
   subtracts the empty loop. A budget file states per command the allowed
   cycles per call. The ctest label is `perf`; the preset `perf` is separate
   and not part of `all`. The emulator runs in warp mode (`warp=true`: no wait for the host clock, CPU, chipset and blitter stay cycle-exact) with one session per bench program, seven side by side; the preset takes about 13 s (it took about 3 minutes at real speed in one session, with the same values). The method repeats to 2-3 cycles per call for the commands the CPU does alone;
   blitter- and frame-dependent loops have a stated tolerance (Decision 11).
10. **Acceptance.** Library bytes change on purpose, so `prev_results` and
    the corpus `prev` configuration are run with `-DPREV_INTENDED`; each
    commit names the library files and executables whose bytes change.
    What must stay identical: every compiler output that links no changed
    library, TED's files, and the behaviour measured by the library tests
    (`libs_emulator`, KEEPS under the new judge), the corpus, the bug-orig
    tests, and the perf budgets.
11. **Where the benchmark lives (task 1.1, decided with the first
    measurement of 2026-11-15).** In `tests/perf/`, beside the corpus,
    whose machinery it uses:
    - `tests/perf/bench/*.bas`: the Blitz sources of the measurement
      (`perf-analysis/bench/`, generated there by a throw-away script, now
      plain sources kept by hand), one program per area, tokenised by
      `tedsrc tok` like the corpus. `bmod` is reworked: an `empty` loop
      like the others, the PlayModule interrupt as a loop of the kind
      `frame` (the cycles per PAL frame the handler takes from the empty
      loop: 142102 x (1 - E / loop), E the empty loop), the timed one-call
      loops of the start and stop commands dropped (a resolution of 1000
      cycles).
    - The run: not a new runner on `tools/lib/amiga_emulation` but
      `corpusrun pass --observe` (`tests/perf/perf.cmake`), which already
      compiles with blitz2 in Amiberry, runs on Kickstart 1.3 (`rcput`),
      stages the module files, takes the `machine=`/`uae=` lines of the
      cycle-exact A500 and skips without Amiberry; the measurement of
      `report.md` used it too. One session per bench program (the tests
      `perf_bmath` ... `perf_bmod`) in warp mode; `perf.pass` with all
      programs in one session stays for the target `perf-original`.
    - The arithmetic and the budgets in C: `results perf` with the logic
      in `tools/lib/perfbudget` (the Python `res4.py` of the analysis
      ported), tested by `tools/tests/test_perfbudget.c`.
    - The budget file `tests/perf/budgets.txt`; the original's values
      come from the target `perf-original` (the same pass with the CD's
      libraries, not a test).
    The first measurement in the repository (clib, then the CD's
    libraries, one session each, 216 s and 168 s) reproduces `report.md`
    to the cycle for the unchanged commands (e.g. Long division 5673.1
    against 2661.0, Peek.b 217.1 against 129.0, Sort 653474 against
    77694); it differs where main changed since `b59cbe2e` (SColl 136.4,
    from the SColl rework; DisplaySprite 1578.5). Of 119 commands 101 are
    slower than the original by more than 2 %, 15 equal, 3 faster (Float
    assignment, QAbs, String$).
12. **Division (task 4.1, findings).** Quick division is not an exact
    16.16 division: its 16 fraction steps shift the divisor right past
    bit 0, so a divisor with fraction bits gives other bits than
    `(a << 16) / b` (1/3.25: `$4EC5`, exact `$4EC4`). A DIVU path is
    therefore exact only where the original's steps are known: Long with
    a divisor below `$10000`; Quick with a divisor without a fraction
    (two DIVU by its high word) or below 1.0 (two DIVU for the integer
    part, then the 16 fraction steps on words). Every other divisor runs
    the loop, among them the benchmark's 3.25. With the original's loop
    Quick went from 8277 to 3648 cycles there, only 4 % under the
    original's 3820: the loop's first steps only move the divisor bit by
    bit into d3 (17 of 48 for 3.25). The loop therefore normalises the
    divisor first (shifts by 8, 4 and single bits until bit 31 is set)
    and runs only the compare steps, without the original's tests of d1
    and of d3 = 0 (never 0 for a divisor from `$10000` on); the same
    bits (user decision 2026-11-15), 1965 cycles for 3.25. Long Mod uses
    the same DIVU remainder for a divisor below
    `$10000` (a - (a/b)*b is the remainder of the magnitudes with the
    dividend's sign, as lmultiply is exact modulo 2^32) and the
    original's two calls otherwise. Measured: Long 5673 -> 519 (the
    original 2661), Long Mod 6479 -> 523 (3113).

13. **The exceptions in the generated code (task 2.1, disassembly of the
    objects of main `848bb9d1`, `-Os`, gcc 16.2-rc24).** Per entry what
    the C function writes, and the decision:
    - `EventWindow` (`doeventwindow_c`): `tst.l`/`move.l` of the window
      pointer, `move.l` it to a0, `or.w 120(a0),d0`; writes d0 and a0,
      keeps a1 by nature (the original also uses a0): **alias**.
    - `RawStatus` (`dorstat_c`): reads the matrix pointer into a0, writes
      d0 and d1, pushes and pops d2; keeps a1 by nature: **alias**, the
      assembler entry of the original follows with the small entries
      (task 6.3). The upper word of d0 is undefined (no caller needs it).
    - `Rnd` and `Rnd(Range)` (`dornd1_c`, `dornd2_c`, `genrnd`): d0-d4
      only (d2-d4 saved by `genrnd`, d2 by `dornd2_c`); keep a0 and a1 by
      nature: **alias** (assembler entry in task 6.3).
    - `WBWidth`, `WBHeight` (`move.l x,d0`), `WBDepth` (`moveq #0,d0 :
      move.w x,d0`): d0 only, keep a0 and a1 by nature: **alias**.
    - `ExecVersion` (`moveq #0,d0 : move.w x,d0`): a zero-extended long,
      as decision 4 requires: **alias**.
    - `EMouseX`, `EMouseY`, `EventCode`: `move.w x,d0 : rts` already, but
      by gcc's choice; **assembler entries** (decision 3, task 2.4).
    - `InitSound`: the C functions are the entries already and `alloc`
      clears a0 before the globalloc call (#1058): **reduced thunk**
      saving a0 (task 2.4).

14. **`__entrypoint` (task 2.6, experiment of 2026-11-15, gcc 16.2-rc24).**
    Of the 417 aliases of 2.3/2.4, 138 C functions save callee-saved
    registers at their start (a `movem.l`/`move.l` to `-(sp)`); many of
    them save a6 because they call the OS. Three properties of GCC's
    `__entrypoint` shape its use:
    - GCC uses a6 as a free register in such a function (`lea tab,a6`
      in a loop), so the rule's a6 is lost unless the check refuses it;
    - a caller in the same unit does not know that the function keeps
      nothing: it keeps its values in d2/d3 across the call (reproduced);
    - identical code folding merges a plain function with the same body
      into a jump to the `__entrypoint` one (the plain function loses its
      saves, reported by the coordinator); `__attribute__((no_icf))` keeps
      them apart (the spelling `noicf` is ignored with a warning).
    So `BLITZ_ENTRYPOINT` is `__entrypoint __attribute__((no_icf))`, the
    function is never called from C, `BLITZ_ALIAS_EP` marks it, and the
    build step of every library runs `blitzlib entrypoints`
    (`tools/lib/epcheck`): a6 written without a save at the start, a call
    or jump into the routine from other code, and a routine that begins
    with a jump into another one are refused (tests with broken input:
    `blitzlib_entrypoints_broken_*`, `cmake/entrypoint/`). Measured on 11
    candidates without a6 and without a C caller: a gain for ShowSprite
    (-136 cycles per call), Plot (-87), PColl (-70.5), Volume (-70.5),
    UCase$ (-52), Joyr (-50), Hex$ (-46); none for Mid$, Left$, Right$,
    Instr (their `move.l a2,-(sp)` is an argument, not a save). Adopted:
    ShowSprite, PColl, Volume, UCase$, Joyr, Hex$. Not Plot: it becomes
    an assembler entry in 6.1 (decision 16).

15. **String copy loops (group 5, findings).** C first, assembler only
    where C stays more than about 10 % slower than the original
    (coordinator, 2026-11-15, after a first assembler version; that one
    is kept on the branch `g5-asm` for the comparison). The copy is one
    macro, `BL_STRCOPY` (`blitzhelp.h`): from 8 bytes on by longs where
    both addresses have the same parity (below 64K), otherwise 8 bytes
    per pass as in the original, the rest by a byte DBF loop. What makes
    GCC compile it well: word counters compared with -1 (`(WORD)--n !=
    -1` after a check for 0) give DBF; an empty `asm` with the pointers
    as `"+a"` operands in each copy step keeps GCC from turning them into
    an index register plus copies (it then saved d2/a2/a6); a macro on
    lvalues instead of an inline function, so that the loop works on a3
    itself instead of a copy (8 cycles per call); `-Os` (at `-O2` GCC
    adds register copies to the loops: `r$=a$` 78, Chr$ 63 cycles slower
    than at `-Os`), so strings1lib leaves `ACIDLIBS_SPEED`. `copstring`
    keeps the thunk for d0-d1 (contract d0-d7; the KEEPS statement of the
    test follows the contract, without the a1 the original keeps by
    chance); its C function uses d0, d1, a0 and a3 only. Measured
    (`tests/perf`, cycles per call, main -> C, the assembler version of
    `g5-asm` in brackets): `r$=a$` 2160.0 -> 1972.5 (1997.7), Len 864.0
    -> 677.5 (701.6), Asc 931.0 -> 745.0 (767.1), concat 3607.0 -> 3386.9
    (3283.7), compare 3564.0 -> 3335.9 (3237.7), Val 3139.0 -> 3137.9
    (3074.2), Instr 4307.0 -> 4118.9 (4074.7). The C version is faster
    where the strings have the same parity and slower by about its thunk
    where they do not (the second operand of compare and concat).
    `lastring`/`astring` are C too: `stringass_c` is the common path (the
    string fits: the length is set, then `BL_STRCOPY` from a3 itself),
    the allocation path is `stringass_new` (`noinline`); the length is
    read back from the header after that call, so no register is kept
    across it and the function uses d0, d1, a0 and a3 only. The thunk is
    a small assembler block that saves only d1 (contract d1-d7 and a2;
    a0 is not needed, thunks.md 1.4: the KEEPS statements follow the
    contract), loads the length, removes it from the stack with `move.l
    (sp)+,(sp)` (the `BLITZ_THUNK_POP` sequence costs about 50 cycles
    more) and lets `astring` add 8 to a6 and fall into `lastring`.
    strings2lib stays at `-Os` (`-O2` is slower, above). Measured, after
    5.1 -> after 5.2 (`g5-asm` in brackets, original): `r$=a$` 1972.5 ->
    1219.5 (1261.0, 1367.5), Chr$ 807.0 -> 400.0 (332.0, 411.0), Mid$
    2083.4 -> 1642.5 (1599.5, 1553.5), concat 3386.9 -> 2154.0 (2181.5,
    2318.5), UCase$ 3500.4 -> 2743.5 (2788.0, 3124.0), Hex$ 2159.4 ->
    1684.5 (1592.5, 1570.0), Inkey$ 1486.0 -> 1076.0 (1014.1, 601.7).
    No command whose cost is the copy stays more than 10 % above the
    original (Mid$ 1.06, Left$ 1.05, Right$ 1.04, Hex$ 1.07, Chr$ 0.97);
    Str$, Val, Instr, compare and Inkey$ are 13 to 80 % above it with the
    assembler version too, their cost is in printlib, vallib,
    stringfunclib, strcomplib and the input libraries. So neither
    routine needs the assembler loop.

16. **The small entries (group 6, 2026-11-15).** Peek, Poke, Plot, Point,
    Joyx, Joyy, Joyr, Joyb, the four mouse functions, RawStatus, QLimit,
    Rnd, DoColl, PColl, ShapeWidth, ShapeHeight, Handle, Colour, Locate,
    the five event getters of windowslib, Int, Sgn and Abs are assembler
    entries (`BLITZ_ASM_ROUTINE`) in the original's form, data read
    PC-relative where the original does. Where a few instructions make
    the original's code faster with the same results, they differ:
    Poke compares the type with SUBQ on d0; Joyx/Joyy/Joyr read JOY1DAT
    first (port 1 is 14 cycles faster); Joyr takes its result from one
    table of 16 longs indexed by the counter bits 9, 8, 1 and 0 (the
    original's d0 for each of the 65536 counter words, checked); PColl
    tests its second bit only when the first is set; Rnd(Range) keeps
    the range in the upper word of d1 (genrnd works on words) instead of
    d7, and genrnd is inline in both forms; Abs clears the sign with
    BCLR. Task 6.6 allowed a noinline float case; the assembler entry
    was taken directly, as the brief of the group asked for assembler
    entries. Plot changes d2 and d3 like the original and keeps d4-d7
    and a2 (user decision 2026-11-15): the original's Plot changes them,
    so no caller can rely on them, and MikesSpaceFXLib, which calls it
    from its starfield loop, saves d0-d2 itself around the call, with
    nothing else live (the analysis of the third-party libraries). A
    first form saved d2/d3 for that loop (522.8 cycles); without the
    save Plot takes 468.8. Joyr and PColl replace their `__entrypoint`
    C functions of task 2.6. Every entry is at or below the original's
    cycles (Plot 468.8 against 486.9). The library test `systest_cmdlibs`
    covers their entry conditions (upper words of word arguments, every
    type of Int/Sgn/Abs, clipping, limits, colval values through the
    operand of DoColl's MOVE); the expected data were recorded from the
    original, and the C build of main (the prev build) and the
    assembler give the same cases; two cases of Sgn of a float are the
    pair `[orig]` / `[fixed #301]`.

17. **Internal tokens (group 8, 2026-11-15).** Each token keeps its
    contract, stated in a `KEEPS` case with its reason (the compiler, or
    the third-party callers of `analyse-internal-token-callers.md`), not
    what the original happens to keep. The contracts: the operators and
    conversions (Mod, `qutofl`/`fltoqu`, the multiplication and division)
    d2-d7/a2-a6, because the compiler saves only d0/d1 around them;
    `strcomp` d0-d7; the procedure prologue routines (`clrloc`, `newmem`,
    `localstat`) d0-d5; `freelast` d0 and its Z flag; `arrmult`, `newarr`
    and `maxlen` nothing (Dim and MaxLen use only d0 after them). The
    third-party contracts: `globalloc` a0/a1 and Z clear (#1059),
    `globfree` d1/a0, `allocst` ($CF01) a0/a1, `freest` d0/a0/a1 (d0 for
    LotanArray `ArSet`; the original does not keep it, so only the case
    with d0 = 0 states it), `fltoqu` a0/a1, `longmult` a0/a1 (by its
    code: the C function uses d0-d4 only, checked in the disassembly; a
    new `KEEPS` case for it and `quickmult`, without an overflow, #199).
    Task 8.2 (cycles per call, before -> after, original):
    - Mod Word/Byte/Quick are the C functions (`BLITZ_ALIAS`); with
      `mod_w` `always_inline` GCC writes the original's six
      instructions (without it, a call with stack arguments). `w3=w1 MOD
      w2` 440.6 -> 246.7 (247.0), Quick 472.6 -> 278.6 (278.0).
    - Mod Float is the original's assembler (two SPFix, then into
      `domodlong`): in C the two mathffp calls made GCC save d2-d7/a2/a6
      (`BLITZ_BASEJSR` treats the call as changing them). 1310.5 ->
      966.1 (3701.0).
    - `qutofl` an alias (2394.7 -> 2287.7, original 1905.5), `fltoqu` a
      thunk of a0-a1 (2253.7 -> 2237.7, 1764.0); both become the
      original's assembler in task 8.3.
    - `newmem` and `localstat` save d0-d1 only, `allocst` a0-a1, `freest`
      d0/a0-a1; `arrmult`, `newarr`, `newlocarr` and `domaxlenl` are
      aliases, `domaxlen` an assembler entry (`addq.w #8,a6`, then into
      `domaxlenl`). A Statement with a string parameter and a local
      string (newmem, lastring, freelast) 12393.2 -> 12359.8 (11614.0).
    - Unchanged: `clrloc` (already d0-d1), `globalloc`, `globfree`,
      `longmult`, `quickmult`, `quickdiv`, `longdiv`; the power routine
      is an inline `jsr -90(a6)` (no thunk).
    The new loops of the benchmark: `bmath.wmod`, `qmod`, `fmod`, `q2f`,
    `f2q`, `plocal` (a Statement with Long locals) and `bstr.pstr`,
    `fstr` (a Statement and a Function with a string parameter and a
    local string).
    Task 8.3, assembler entries without a thunk (C cannot keep the
    registers of these contracts or return a flag):
    - `strcomp` ($CE00): the original's frame (lengths from the stack,
      d0-d1 saved, a3 to the left string, the result as MOVEQ flags), the
      loop CMPM.b/DBNE over the shorter length followed by the lengths'
      difference, 22 instead of 54 cycles per character. That is the
      original's result for every length below 2^31; a length from 2^31
      on takes the original's SUBQ.l/BMI loop (new library cases: such
      lengths, which end that loop before a character is read, and
      strings above 64K for the second DBNE pass). `If a$=b$` 3337.8 ->
      2158.9 (original 2943.5).
    - The procedure entry and exit (`clrloc`, `newmem`, `localstat`,
      `freelast`): `clrloc` clears with `MOVE.l a1,(a0)+` up to a4 and
      saves nothing (28 cycles per long; the original's DBF loop needs
      d0-d1 saved and a count, faster only from 11 longs of locals on);
      `newmem` and `freelast` are the original's code with d0-d1/a6 and
      d0/a2/a6 saved, `freelast`'s Z from the original's MOVEQ #-1 / BEQ;
      `localstat` is the loop of `alstat` on a0/a1 with d0-d1/d4 saved
      (it keeps d2, which the original changes: see the finding below).
      The library finit (`codefinit`) calls the `freelast` entry while
      there is a list, the init the `newmem` entry. A new library case
      for `clrloc` (a frame of 1 and of 8 longs between LINK and UNLK).
      `plocal{a}` 444.5 -> 325.1 (original 389.5), `pstr{a$}` 12359.8 ->
      11991.8 (11614.0), `c=fstr{a$}` 13049.0 -> 12686.2 (12353.5).
    - `qutofl`/`fltoqu` ($D300/$D301): the original's code (user
      decision 2026-11-15), with the TST.l of the #146/#147 fix at the
      start of `qutofl`; they write only d0, d1 and the stack (d2 saved)
      and call mathffp through `jsr -xx(a6)` with the original's
      operands, so a0/a1 are the caller's (the ROM's mathffp leaves them
      alone, as the original relies on); both `KEEPS` state a0-a1. `f3=q1`
      2287.7 -> 1898.2 (original 1905.5), `q3=f1` 2237.7 -> 1764.7
      (1764.0).
    - `copstring` and `lastring`/`astring` keep their minimal thunks of
      group 5 (d0-d1, d1; Decision 15).
    - Finding (static, not run): the original's `localstat` changes d2
      (`MOVE (a2)+,d2` in `alstat`) and d4 (`DBF`), and the compiler
      calls it in the prologue before it stores the parameters, pushing
      only d4 (five or more parameters, `procfixer`). With three or more
      parameters and a local NEWTYPE variable the third parameter would
      arrive as the last table offset in the original. The port's
      `localstat` keeps d2 (the C function did as well); reported to the
      coordinator for an issue.
    Task 8.4, the whole benchmark twice on the final state (cycles per
    call: main ecef902a -> group 8, the original):

    | Command | main | group 8 | original |
    |---|---:|---:|---:|
    | `If a$=b$` | 3337.8 | 2159.1 | 2943.5 |
    | `w3=w1 MOD w2` | 440.6 | 247.2 | 247.0 |
    | `q3=q1 MOD q2` | 472.6 | 278.7 | 278.0 |
    | `c=f1 MOD f2` | 1310.5 | 966.2 | 3701.0 |
    | `c=a MOD b` (Long, group 4) | 522.5 | 523.2 | 3113.0 |
    | `f3=q1` | 2394.7 | 1899.7 | 1905.5 |
    | `q3=f1` | 2253.7 | 1762.7 | 1764.0 |
    | `q3=Frac(q1)` (both conversions) | 4971.6 | 3986.2 | 3718.0 |
    | `plocal{a}` | 444.6 | 323.7 | 389.5 |
    | `pstr{a$}` | 12393.2 | 11991.6 | 11614.0 |
    | `c=fstr{a$}` | 13082.2 | 12686.5 | 12353.5 |

    Every command of the group is at or below the original except the
    two procedures with a string (3 % above; most of their 12000 cycles
    are exec's AllocMem and FreeMem and the string assignment of group
    5, not looked into further) and Frac (7 %, task 10.5). The other
    commands of the benchmark stay within their budgets; `bmath.frac`
    is tightened (5025 -> 4030).

19. **Sort (task 7.3, 2026-11-15).** C, no assembler: one recursive
    function per element type (`SORT_FN` in `sortlib.c`: byte, word,
    long for long, quick and the unknown types; float and string), each
    with its comparison in the scan loops, as the original has
    sortbyte ... sortfloat. Three things make GCC (`-O2`, sortlib is in
    `ACIDLIBS_SPEED`) compile it like the original: the scan loops as
    `v = *i++` and `--j; v = *j` with an empty `asm` that keeps the
    pointer in its address register (`MOVE.l (a0)+,d1 : CMP.l d0,d1 :
    BLT` and `SUBQ.l #4,a1 : CMP.l (a1),d0 : BLT`, without it a copy of
    the pointer in a data register per step); the recursive function
    takes lo, hi and the mathffp base in a0, a1 and a6 (`BL_REG`: no
    stack arguments per level); the float comparison is `JSR -42(a6)`
    with an Scc right after it, inline in the loop. The string
    comparison stays a `noinline` call of strcomplib $CE02 (the token
    may change every register but a4/a5/a7). Measured (201 sorted
    longs, `bstr.sort201`): 653454 -> 95454 (generic loops, stack
    arguments) -> 88774 (the scan loops) -> 83274 cycles (register
    arguments), the original 77694 (1.07): below the 10 % of decision
    15, so no assembler block. The algorithm and its order are the
    original's (Hoare's quicksort is not stable: the order of equal
    strings is fixed by library cases of 201 elements of every type,
    `systest_cmdlibs`, recorded from the original); the fixes #275 and
    #276 stay. sortlib.obj grows from 980 to 1244 bytes.

20. **The blit core (tasks 7.1 and 7.2, 2026-11-15).** Measured first
    (`tests/perf`, main `ecef902a`): Blit 1.42, QBlit 1.28 to 1.38,
    BBlit/UnBuffer 1.25, ClipBlit 1.31, Block 1.23, BlitColl 1.59 times
    the original; the disassembly shows the cause of `report.md` (H):
    the helpers are functions with stack arguments, the blit state a
    struct in memory, and the Blitz mode code saves four to six
    registers. Decided for every file (blitlib, qblitlib, bblitlib,
    2dlib ClipBlit, stencillib): C, no assembler block. The helpers
    are `BL_ALWAYS_INLINE` (`blitzhelp.h`), so the state struct is
    in registers; the Blitz mode code of each command (the `Dc.l` of
    its Blitz/Amiga routine) is `BLITZ_ENTRYPOINT`, marked by
    `BLITZ_EP_MARK` (or `BLITZ_ALIAS_EP`) for the build check: it saves
    nothing, as the original, which changes d0-d7 and a0 too (the
    register rule keeps a3-a7). Two things keep GCC off a6 there: the
    plane offset is `bl_blitoff` (the original's `MULS bpr,y : ASR.w
    #3,x : EXT.l : ADD.l`; GCC adds with LEA in an address register and
    took a6 for it), and where the code needs more address registers
    than a0-a2 (QBlit, BBlit, UnBuffer, ShowStencil, SBlit, ClipBlit)
    `BLITZ_EP_KEEP_A6` makes a6 a live hard register through the
    routine. The Amiga mode code inlines the core too (faster, but each
    library file grows: blitlib 2448 -> 2864, qblitlib 2568 -> 3192,
    bblitlib 2780 -> 3876, 2dlib 13576 -> 14836, stencillib 2132 ->
    2584 bytes). Blit with Excessonoff of blitlib stays a normal
    function (other libraries reach it through the nullsub
    `b_doblitex`). Measured, before -> after (the original): Blit
    2986.3 -> 2116.9 (2108.3), Blit 32 5422.3 -> 4729.4 (4723.4),
    Block 1246.3 -> 1006.9 (1009.9), BlitColl 1398.8 -> 901.4 (881.9),
    4 QBlit + UnQueue 16397.8 -> 12907.8 (12785.9), 4 QBlit +
    FlushQueue 12507.8 -> 9197.8 (9087.9), 4 BBlit + UnBuffer 22781.8
    -> 19169.9 (18173.8), ClipBlit 3286.4 -> 2577.3 (2516.3), and
    outside the benchmark (a program of the same kind) SBlit 3515.8 ->
    2682.9 (2702.2), 4 BBlit + ShowStencil + FlushBuffer 26309.8 ->
    22687.9 (21731.8). Every one within 5 % of the original (BBlit:
    GCC keeps the bitmap pointer on the stack), so
    no assembler block (decision 15). The fix of #1045 zero-extends
    the plane step once outside the plane loops; GCC then keeps the
    step of BBlit in a register (ADDA.l, 8 cycles a plane instead of
    24 from the stack: 4 BBlit + UnBuffer 19168 -> 19072, 8 BBlit +
    UnBuffer 38002 -> 37666), and the loops of UnBuffer and
    ShowStencil are the same instructions as before. The library
    test `systest_blit` (new) covers the commands in both modes: every
    shift, junk in the upper words of x and y, the handle, the excess
    planes, the modes, a collision and none, ClipBlit clipped at each
    edge and outside; recorded from the original, the C build of main
    and of the change give the same cases.

21. **Remaining slow commands, part A (group 10, 2026-11-15).** Measured
    first in `tests/perf` and in the disassembly; cycles per call,
    before (main `e5eb09d0`) -> after (the original). The library test
    `systest_cmdlibs` gets cases for every routine of the group that
    had none (the rectangle tests, Frac, BlitzKey, Inkey$, Print on a
    bitmap); recorded from the original, the C build of main and of the
    change give the same cases.
    - Collisions (task 10.3): `chkifhit` was a function with eight stack
      arguments, and each entry saved the registers of its arguments.
      It is `BL_ALWAYS_INLINE` and the four entries (ShapesHit,
      ShapeSpriteHit, SpritesHit, RectsHit) are `BLITZ_ENTRYPOINT`: they
      save nothing and change d0-d5 (the original changes d0-d7). C, no
      assembler: ShapesHit 731.4 -> 361.4 (383.4), SpritesHit 771.4 ->
      391.9 (418.9), RectsHit 553.4 -> 262.9 (286.9).
    - Print on a bitmap (task 10.1, fontlib): the drawing helpers
      (`calcoff`, `docr`, `bitchar`) were functions with the line
      offset in memory, and GCC built the two colour masks of each
      plane with MULS. With the helpers `BL_ALWAYS_INLINE`, the masks
      from `-(bits & 1)` and the eight lines unrolled, C reached 1.15
      (`Print "SCORE"`) and 1.11 (`Print a`) times the original: GCC ran
      out of registers and kept the line pointers on the stack. So the
      plane loop is an assembler block inside `bitchar` (decision 15),
      the original's loop with each line written once: (~font & bg) |
      (font & fg) = bg ^ (font & (bg ^ fg)), the same bytes as the
      original's two writes. `bitout2` takes its arguments in registers.
      `Print "SCORE"` 28319.8 -> 12079.9 (13097.9), `Print a` 35567.8 ->
      16111.9 (17797.8); fontlib grows from 2560 to 2836 bytes. Locate
      stays at 128.3 (128.3, group 6).
    - Inkey$ (task 10.1): Inkey$ (editlib) and BlitzKey (blitzkeyslib,
      the input handler of BlitzKeys On) are the original's assembler.
      In C Inkey$ saved d2-d7/a2/a6 around its ALibJsr, and BlitzKey was
      a C function behind a thunk that saved d2/d3; a C BlitzKey with the
      key conversion out of line stayed at 1.26. The channel
      dispatchers of inputoutputlib read the handler PC-relative, as the
      original. `r$=Inkey$` 1075.5 -> 662.5 (601.7): 1.10; the rest is
      the assignment of the empty string (strings2lib, decision 15: its
      C path calls the copy routine and tests the length before it sees
      0, about 50 cycles more than the original's `BEQ 'endass`), not
      changed here.
    - Frac (task 10.5, vallib): the C function saved d2-d7/a2/a6 around
      its mathffp calls; it is the original's assembler entry, as Abs,
      Int and Sgn (decision 16), with the original's tail jump into
      SPSub. `q3=Frac(q1)` (with both Quick/Float conversions) 3986.1 ->
      3710.2 (3718.0).
    - The players (task 10.5, medlib and trackerlib, `-O2` since
      decision 6): their per-frame code is a chain of static functions
      with structure pointers on the stack (ticks, synth, playnote;
      mt_checkcom and the effects). `-mregparm=3` for the two libraries
      (`ACIDLIBS_REGPARM`, `libs/acidlibs/CMakeLists.txt`; they call no
      function compiled without it) passes them in registers: PlayMed
      12017.8 -> 11583.6 (9950.0), the PlayModule interrupt 3091.6 ->
      2602.8 to 2720.8 per frame (2409.5), medlib 348 and trackerlib 88
      bytes smaller. `-mregparm=4` gives the same; inlining
      (`-finline-functions`) PlayMed 220 cycles less for 712 bytes more
      and nothing for the interrupt. Both stay above the 10 % of
      decision 15 (1.16 and 1.13); their cost is spread over the
      player's C (the disassembly shows no single hot routine: the
      busy waits of the DMA timing are the same as the original's, the
      register saves fewer), and an assembler player (the original's
      whole replay routine) is not done here. Task 10.5 stays open for
      that decision.

22. **The drawing, display and palette commands (tasks 10.2 and 10.4,
    2026-11-15).** Measured first; the causes were those of the blit
    core (decision 20): helpers with stack arguments, Blitz mode code
    behind a thunk or saving the registers of its arguments, state in
    memory. C first, per routine:
    - C (helpers `BL_ALWAYS_INLINE`, the Blitz mode code
      `BLITZ_ENTRYPOINT`, a6 kept live where needed): palettelib PalRGB,
      AGAPalRGB (the growing out of line), ShowPalette, Use Palette (the
      top byte of a component read as a byte), RGB, Red, Green, Blue;
      blitzcoplib Show, ShowF, ShowB (the offset added with ADD.l, the
      high word with SWAP); 2dlib FloodFill, ReMap and Polyf without
      their thunks.
    - The original's code in assembler, as the C stayed more than 10 %
      slower after those steps (decision 15): palettelib FadePalette
      (1.12: the MOVEM.l of the three components), spriteslib ShowSprite
      (1.12, with the fix of #283), displaylib DisplayBitMap (1.19 for
      0,0, with #411), DisplaySprite (with #414 and the errors of #412)
      and DisplayPalette (the colour loop, 1.8), scrolllib Scroll in Blitz
      mode (GCC kept the offsets on the stack in the plane loop; the
      Amiga mode code and the nullsub b_doscroll stay C), 2dlib Line,
      Box, Poly (the line clipper, with #391), Boxf, Circle, Ellipse,
      Circlef, Ellipsef (with #392) and the line loop of Polyf (the
      clipping of #1000/#1033 stays C).
    Measured (cycles per call, main -> after, the original): Circle
    66239.8 -> 33074.8 (33609.8), Circlef 36454.8 -> 16939.8 (17189.8),
    Polyf 87154.8 -> 50269.8 (46969.8), Box 13093.8 -> 9345.8 (9571.9),
    Boxf 3141.8 -> 1975.8 (1991.8), Line 4993.8 -> 3891.8 (3967.8),
    Poly 10104.8 -> 7339.8 (7524.9), Green 360.8 -> 166.4 (167.3), RGB
    206.8 -> 179.4 (178.8), Show 1206.3 -> 583.4 (589.4), DisplayPalette
    2370.0 -> 1287.8 (1292.0), ShowPalette 2427.9 -> 1310.0 (1616.0),
    PalRGB 835.9 -> 456.9 (653.5), FadePalette 3844.9 -> 2999.9
    (3044.9), DisplayBitMap 581.0 -> 496.4 (494.0), DisplaySprite 1505.0
    -> 945.4 (965.5), ShowSprite 714.3 -> 623.9 (629.4), Scroll 9469.8 ->
    8003.9 (8075.9). Every command is below the original or within 2 %
    of it (Show 0,8,8 1.02, DisplayBitMap 1.01, RGB 1.00) except Polyf
    (1.07, within the 10 %). The library
    test `systest_gfx` (new, tag `[gfx]`) covers the commands: clipping
    at each edge, the origin, every colour and a negative one, the
    shifts of Show and DisplayBitMap, the sprite clipping, attached
    sprites and both sprite banks, palettes that grow, AGA and ECS
    colours; recorded from the original (75 cases are pairs of the
    fixes #229, #391 and #411), the C build of main and of the change
    give the same cases. Not in the scenario: points above the top edge
    (#392) and a cut vertical Polyf edge (#1033), where the original
    writes outside the bitmap or divides by zero, and the top clipping
    of ShowSprite in SpriteMode 1 and 2 (#283).
    - Polyf's set-up (task 10.11, 2026-11-16), C except the line loop:
      the cycles went to call shape and to the line loop's addressing,
      not to the clipping of #1000/#1033 (polyinrange and clippoly run
      only for points outside 0..8191). Per step, Polyf 3 points (the
      original 46969.8): `prodgt` inline, its MULU without zero
      extension (the magnitudes fit in the low word) 50269.7 ->
      50094.8; the state of ploop read PC-relative as the original
      (`cmp.w _pq+8(pc)`, `btst d6,_pq+11(pc)`: 4 cycles per access, 28
      per line with three planes) -> 48969.9; the points copied as
      longs, ORed together for a quick range test (no bit of
      $E000E000 and no origin: polyinrange is true without its loop),
      drawpoly inline -> 47734.9; outline and both sides inline (they
      were functions with seven and eight stack arguments, each saving
      eight registers) -> 46659.9 (0.99). 2dlib 13380 -> 13308 bytes.
      Not taken: polyinrange inline (180 cycles more on the quick path,
      register pressure; it stays a function and compares words);
      the original's `MOVE lup1(pc,d6),$44(a4)` (the tables would have
      to be within 128 bytes of the loop, 130 with the loop's code: two
      LEA of 8 cycles per line stay, 640 cycles of the benchmark).

23. **Sort, the buffered blits and Frac (tasks 10.7 to 10.9,
    2026-11-16).** Cycles per call, before -> after (the original).
    - Sort (task 10.7): the C of decision 19 already had the original's
      tail recursion (GCC compiled the right part as a loop: the
      `for (;;)` around the body, only the left part a `JSR`); the 7 %
      came from elsewhere, found in the disassembly. GCC kept `sdir`,
      `first`, `last` and `i - 1` in registers of their own and saved
      four registers per level (`MOVEM.l d2-d4/a2`), and it kept the
      backward scan's element in a register for the swap, which needed a
      fifth. Now: `lo` is the first element itself (`i` is its own
      register), `sdir` is read in every round of `'loop0` as in the
      original (a register less per level for a `TST.w` of 16 cycles a
      round; keeping it in a register measured 78574, worse), an empty
      `asm` after `i--` makes GCC step `i` itself, a memory barrier
      before the swap makes it read `*j` again (`MOVE.l (a1),(a0)+`),
      and the original's `j++` before the backward scan and `j--` after a
      swap, which cancel, are left out (`j` is decremented only when the
      loop ends after a swap). The integer sorts save two registers per
      level (`MOVE.l a2,-(sp) : MOVE.l d2,-(sp)`, the original pushes two
      longs), the swap is three moves and a compare. The float and string
      sorts keep their pointers with `"r"` instead of `"a"` (their
      comparison is a call that may change a0/a1; the `"a"` made GCC copy
      the pointer between a data and an address register each step).
      Left as it is: GCC never compiles a compare with `(An)+` as its
      source (tried in five forms and with `-fno-ivopts`), so the forward
      scan is `MOVE.l (a2)+,d1 : CMP.l d0,d1` (4 cycles a step more than
      `CMP.l (a0)+,d0`). C, no assembler: `bstr.sort201` 83274 -> 77794
      (77694), 1.001; sortlib.obj 1244 -> 1132 bytes. The order of the
      comparisons and swaps is the original's, so the results are the
      same bit for bit (the library cases of `systest_cmdlibs`, every
      type, both directions, 201 elements).
    - BBlit, UnBuffer and QBlit (task 10.8): measured with two extra
      loops (not committed: 4 BBlit + FlushBuffer, 1 BBlit + UnBuffer;
      the original 13849.9 and 4767.9): BBlit was 93 cycles a call
      behind, UnBuffer 28 a record. The plane loops wait for the blitter,
      so what counts is the CPU code between a wait and the write of
      BLTSIZE and the setup of the command, not the loop alone: the
      original's loops as asm blocks in the C of decision 20 gave nothing
      (18640 -> 18662). What helped, in this order: BBlit reads
      everything from the shape and the bitmap first, so the bitmap
      pointer is dead before the record is written; the bitmap's Planes
      is a `ULONG` in a data register (the original keeps the bitmap in
      a2, here the record pointer) and the buffer pointer goes to the
      stack once (a `volatile` local, stored once and read once), so no
      other value is spilled; the shift is `AND #15 : ROR #4` (an asm of
      one instruction; GCC's `MOVEQ #12 : LSL` costs 12 cycles more); the
      values of the shape blit (the swapped modulos, BLTCON0) are
      computed before the wait; UnBuffer reads a record's fields before
      the wait. `register ... __asm("dN")` was tried and not kept: GCC
      moved the values to other registers or the stack anyway. With that
      C around them, the plane loops in C (empty `asm` barriers on the
      pointers) stay 1.6 % behind (4 BBlit + UnBuffer 18460), GCC copying
      the plane pointer between a data and an address register each
      plane; so the save and blit loops of BBlit and the plane loop of
      UnBuffer are the original's loops as asm blocks (`MOVE.l
      (a0)+,d0 : ADD.l d3,d0 ... DBF`; the step `ADD.l`, #1045 stays
      fixed). QBlit keeps its C loop: with the `ROR` it is 0.7 % behind
      (an asm loop: 0.3 %), within the 1-2 % of the user's rule; moving
      its offset and item code before the wait made it slower (9510:
      more spills, and the blitter of the short blits is idle by then).
      Five runs: 4 BBlit + UnBuffer 19072 -> 18160 (18173.8), 8 BBlit +
      UnBuffer 37666 -> 35890 (36041.8), 4 QBlit + FlushQueue 9200 ->
      9154 (9087.9), 8 QBlit + FlushQueue 18306 -> 18250 (18063.8), 4
      QBlit + UnQueue 12910 -> 12908 (12785.9), 8 QBlit + UnQueue 25714
      -> 25706 (25417.8); of the extra loops 4 BBlit + FlushBuffer 14222
      -> 13880 (13849.9) and 1 BBlit + UnBuffer 4810 -> 4802 (4767.9).
      bblitlib 3844 -> 3764 bytes, qblitlib 3192 -> 3200. ShowStencil,
      which the benchmark does not run, is unchanged; the library cases
      of `systest_blit` (BBlit, UnBuffer and QBlit in both modes, every
      plane count, the excess planes) give the same values.
    - Frac (task 10.9, #1064): `!args` #unknown with intchk, as Int and
      Sgn (user decision 2026-11-16: `Frac(a$)` is no longer a compile
      error but the runtime error "Illegal Type" with the debugger). The
      type word at -2(a2) selects: Float the original's code (SPFloor or
      SPCeil, the tail jump into SPSub); Quick the FFP built from the
      fraction word of |q| (swap, `CLR.w`, shifted left until the top bit
      is set, exponent 64 minus the shifts, the sign of q), no mathffp
      call, at most 15 shifts; Byte, Word and Long 0 (the original's
      SPFlt gave an integral float). The library cases record the
      original as the compiler called it (qutofl or SPFlt, then Frac) and
      call the C entry with the value and its type: for integer parts
      below 256 the values are the same; from 256 on qutofl's 24-bit
      mantissa rounds the low fraction bits (15 of the 54 Quick cases,
      e.g. 30000 + 10/65536 gives 0, 387.796 one bit more), the pairs
      [orig] / [fixed #1064]; the Quick 0 gave the FFP "-0" $00000080
      (qutofl's #146, fixed in the port's qutofl since), now 0, the pair
      [orig] / [fixed #146]. The Float path keeps SPFloor/SPCeil/SPSub: a
      bit-mask variant was not tried, it would need a proof over every
      exponent and mantissa that it gives mathffp's bits.
      `q3=Frac(q1)` (with the compiler's fltoqu after it) 3710 -> 1314
      (3718.0).
    - QBlit and UnQueue, a second look (after the rebase onto part B,
      2026-11-16; the C of task 10.8 was 0.7 % behind with FlushQueue,
      1 % with UnQueue). Compared with the original instruction by
      instruction, three clear causes: GCC reads `bcon0` with an absolute
      address (`OR.w bcon0,d0`, 8 cycles more than the original's
      `OR bcon0(pc),d0`; the library is one hunk, so the asm `or.w
      _bcon0(pc)` links), writes the rest of the queue item with offsets
      instead of `(a4)+` (4 cycles a field, and a LEA for the write
      position), and UnQueue reads an item with offsets (`MOVE.w
      4(a0),d0` ...: 12 instead of 8, 16 instead of 12, 24 cycles an
      item) where the original has `(a2)+`. Now QBlit's or, its item and
      UnQueue's item read are asm statements with `(An)+`, and the plane
      loops of both are the original's loops (asm blocks with `DBF`, as
      BBlit's). Five runs: 4 QBlit + FlushQueue 9154 -> 9078 (9087.9), 8
      QBlit + FlushQueue 18246 -> 18082 (18063.8), 4 QBlit + UnQueue 12906 ->
      12744 (12785.9), 8 QBlit + UnQueue 25704 -> 25356 (25417.8); qblitlib
      3200 -> 3052 bytes. UnQueue with a BitMap (unqueue2), which the
      benchmark does not run, is unchanged.
24. **The players (task 10.10, 2026-11-16).** Tuned in C after the
    analysis `analyse-players.md` (per-pattern cycle counts of the
    original's replay code against GCC's), one step per commit, each
    measured. trackerlib: `mt_checkcom`, `mt_checkcom2` and
    `mt_playvoice` are `BL_ALWAYS_INLINE` in `mt_music` (the per-voice
    call, the argument moves and the save of a2 for the audio address
    went), `mt_playvoice` takes the note's address instead of a pointer
    to the position, the sample number is a `UWORD` and the loop
    counter a `WORD` (DBF without GCC's `clr.w/subq.l/bcc` tail). The
    effect routines keep the audio address as a `ULONG`: as a pointer
    GCC writes the period of the no-effect path as `d16(a6)`, 4 cycles
    more per voice and frame. medlib:
    - M1: `_IntHandler` and the routines of a pulse (`newline`,
      `nextline`, `hold`, `ticks`) with their helpers are inline in
      `_PlayMed`: one function per pulse as in the original.
    - M3: the player state in the fixed registers of the libraries
      (user decision 2026-11-15): during a pulse a3 holds the module, a4
      its song, a5 the track at hand, as the global register variables
      `blitz_string`, `blitz_locals`, `blitz_globals` (macros `PM`,
      `PS`, `PT` in `medlib.c`); GCC has only a0-a2/a6 for addresses
      and spilled them as locals. `_PlayMed` is a thunk that saves and
      restores a3-a5, as the original's `_PlayMed` saves a4-a6 (which
      keeps the module in a2, the song in a4, the track in a5). Nothing
      a pulse runs relies on their Blitz meaning: the player makes no
      ALibJsr, no OS call and no string work, its functions take
      nothing from a3-a5 but this state, and an interrupt during
      PlayMed finds the program's registers as with the original (an
      interrupt server gets a5 from exec, a Blitz handler takes the
      program's a5 from intlib's `newint` and loads a3 at the start of
      each string expression). `playnote` is inline in `newline`; an
      out-of-line copy serves the replays of the ticks.
    - Pattern 4: `periods_of` adds `(BYTE)(finetune << 2)` as a byte
      offset to `&periodtable[8]` (the same pointer as the division by
      4 and the long index), the note of a sample is a `UWORD` after its
      clamp, the track loops compare with a local copy of `numtracks`.
      Making the instrument number a `UWORD` changes nothing: GCC proves
      `(WORD)(i << 3)` non-negative and indexes with a long anyway.
    - Pattern 5: `playnote` makes the channel mask once; the loops
      already test ChanMask with BTST in a register.
    - Pattern 6: `switch (cmd & 0x1f)` in the ticks: the command is 0-31
      (masked where it is stored), the AND tells GCC so and the jump
      table loses its range check and the AND.l #255. GCC ignores
      `__builtin_unreachable` for the range (and splits the table when
      every value has a case), and a `goto *` would stop the inlining of
      `ticks` (GCC does not inline a function with a computed goto).
    - Pattern 7: `pushnewvals` is a DBF loop that shifts the mask and the
      DMA bits as it uses them and walks the track pointers.
    - `while ((BYTE)n < 0) n += 12;` in `playnote`: GCC computed the
      number of passes with a DIVU; an empty `asm` on n in the loop body
      keeps the plain loop (22 cycles per pass, as the original). Not
      in the benchmark's path; the library test `systest_med` got a
      third module with notes below 0 for it.
    Measured (cycles per call; PlayModule: per frame), before -> after
    each step, the original in brackets: PlayModule 2843.8 -> 2115.5
    (2409.5); PlayMed 11583.1 -> M1 11129.5 -> M3 10837.2 -> 4: 10647.1
    -> 5: 10649.2 -> 6: 10514.2 -> 7: 10415.2 (9950.0). The PlayMed
    value is quantised by the raster waits of a line tick (it ends on a
    raster line), so each step was also measured in a scratch build
    without the waits (`wait1lin` empty): M3 6714.6 -> 6580.8 -> 6533.5
    -> 6433.1 -> 6319.2. The PlayModule frame value moves with the
    layout of the libraries in the program (the empty loop it subtracts
    runs 150000 times: 0.3 cycles per iteration are 340 per frame): with
    trackerlib unchanged the medlib steps give 2224.9 to 2614.1, so its
    budget's B is the highest of the runs of all layouts. trackerlib is
    below the original; PlayMed stays 1.05 times the original (about
    6 % in the code without the waits). The remaining gap is spread:
    GCC keeps the pointer into `trackdataptrs`, `numtracks` and the
    period table pointer of a note on the stack in `newline` (register
    pressure: a0-a2/a6 for addresses besides a3-a5), reads the data base
    with `abs.l` instead of the original's `d16(a6)` and zero-extends
    bytes before indexed accesses. The library test `systest_tracker`
    (new, tag `[tracker]`) covers trackerlib's player (every effect,
    the audio registers and voices after every frame), recorded from
    the original; the C build of main and of each step give the same
    cases.

25. **medlib's line routine in assembler (task 10.12, 2026-11-16).**
    `plr_loop0` with `_PlayNote` inline is the assembler block
    `medlib_newline` (`BLITZ_ASM_BLOCK`) in the original's form: its
    labels, its jump table of the commands, the data base through a6
    (`d16(a6)` instead of GCC's `abs.l`), the track number in d7 and the
    qualifier word in d4 (#203). Differences from the original: the
    module is in a3 (the player state of decision 24), so the instrument
    of `_PlayNote` (the original's a3) is a1 until its repeat and length
    are loaded into d2/d3; the note pointer is in d6 and the pointer into
    `trackdataptrs` in a2 instead of two stack slots; the IFF octave path
    saves d6 and d7. The field offsets are `.set` symbols from one list
    that `_Static_assert` checks against `offsetof`. `newline()` calls it
    with an asm statement that clobbers every register but a3-a5 (the
    prologue of `_PlayMed_c` saves d2-d7/a2/a6 anyway) and has a3/a4 as
    `"+a"` operands: without them GCC kept copies of the module and the
    song in stack slots across the call. FFF and FFE call the C
    functions `ChannelOff` and `SoundOff` (`noinline, used`). The C
    `playnote` stays for the replays of the ticks. The results are those
    of the C version: `systest_med` gives the same cases, and a fourth
    module (an FFE stop, recorded from the original) covers the path
    that no other module meets.
    Then the accesses to `db` in the C parts (coordinator's hint): per
    pulse `IntHandler` reads bpmcounter, module, blkdelay, dmaonmsk,
    numtracks once each, `nextline` nextblockline, numlines, nextblock
    and nxtnoclrln once each, `hold` none in its loop, and the end of the
    pulse dmaonmsk and ChanMask once; no field of `db` is read per track.
    Per track only `ChanMask` (a static next to `db`) was read again in
    the loop of the ticks (`moveq #0` + `move.w abs.l`); read once per
    pulse into a local it is a register (`btst d0,d5`), 47 cycles per
    call less without the waits; GCC then keeps `vibrspd` of the vibrato
    paths in a stack slot (about 16 cycles more per vibrato tick and
    track, not in the benchmark).
    Measured (cycles per call; without the waits: the scratch build with
    `wait1lin` empty), the original 9950.0: C of task 10.10 10415.2
    (6319.2) -> line routine 9906.8 (5791.7-5791.8) -> ChanMask once
    9794.8-9795.8 (5744.8-5745.2), 0.98 times the original (on main
    8fcb651f, with the layout-independent `bmod.bas`; the line routine on
    the old main gave 9906.1 and 5793.8).

26. **Volume and SColl (tasks 11.4 and 11.5, 2026-11-16).** Cycles of the
    68000 counted statically without the call, then measured
    (`tests/perf`, cycles per call, two runs).
    - *Volume:* the disassembly confirmed the earlier guess. GCC made the
      loop of four passes a counter in d1 and d2, a computed address
      (`move.l d2,d3 : lsl.l #4,d3 : movea.l #$dff0e8,a0 : suba.l d3,a0`)
      and two SUBQ per pass; the original's loop is LSR/BCC/LEA/DBF (196
      cycles for one channel). Unrolled in C, each channel is one bit
      test and one write to its absolute AUDxVOL (GCC: `btst #0`/`btst
      #1`, then `move.w d0,ccr` and BNE/BPL for bits 2 and 3): 130
      cycles, no assembler. GCC reads the volume with `d16(a2)` plus
      SUBQ instead of `-(a2)`; neither `*--p` nor an empty `asm` with a
      `"+a"` operand changed that (10 cycles per set bit), and C is
      already a quarter below the original. The `__entrypoint` function
      changes a2 by all popped volumes but the last (its last SUBQ is
      dead), so the library test does not observe a2 (the compiler does
      not read it). Measured 382.1 -> 204.1 (the original 278.7, 0.73).
    - *SColl:* the original (bytes of the CD's ACIDLIBS) is `LSR #1,d1 :
      LSR #1,d0 : BEQ.w : ADDQ #1,d0 : ADD d0,d1 : BTST d1,colval.l :
      SNE : EXT : RTS`. Its bit is p1 for p0 = 0 and p0 + 1 + p1
      otherwise (p = channel / 2); the right bit of two different pairs
      is p0 + p1 + 1 when neither is pair 0, else p0 + p1, so the
      original is wrong only for p0 != 0 and p1 = 0. The two-path form
      of R1 (CMP and Bcc in front, 12 cycles, then the original's code
      with PC-relative BTST, -4) cost 8 cycles. The new form keeps one
      path and replaces ADDQ/ADD with `NEG d1 : NEG d1 : ADDX d0,d1`: the
      second NEG sets X for p1 != 0 and leaves p1, ADDX adds p0 and X.
      p0 = 0: 62/64 cycles (no collision/collision) against the
      original's 66/68; p0 != 0: 72/74 against 76/78 (the 4 extra cycles
      of NEG/NEG/ADDX are paid by BEQ.s against the original's BEQ.w,
      the PC-relative BTST saves 4 on both paths). Every input gives the
      original's bit except p0 != 0 with p1 = 0 (the #110 fix), and the
      same as the R1 form for every input. Alternatives counted: a table
      of 64 bytes indexed by c0 * 8 + c1 (42 cycles to the BTST like the
      original's short path, but an unchecked channel indexes memory
      outside the table), per-pair masks ANDed with colval (2 cycles
      more), BTST with a byte offset (the bit of a pair is not a sum of
      per-channel values: no table of one channel works). Measured 136.0
      -> 124.0 (the original 126.3, 0.98). `systest_cmdlibs` gets SColl
      for all 64 channel pairs (the bits it reports and d0 with junk in
      the upper words, the 12 pairs of #110 as `[orig]`/`[fixed #110]`)
      and Volume for every mask (Paula's registers patched to a RAM copy
      as in `systest_tracker`), recorded from the original.

27. **Val, Str$ and Instr (tasks 11.1 to 11.3, 2026-11-16).** C, no
    assembler; measured in `tests/perf` (cycles per call, main 65066af5
    -> after, the original in brackets), each step against the
    disassembly of GCC's code and of the original.
    - Val (vallib, task 11.1): `doval_c` passed a `struct valstate`
      (pointer and count) by address to `doval2`, `notneg`, `getdec`,
      `gethex` and `getbin`, so every character wrote the pointer and
      the count back to memory, and each of the functions saved
      d2-d7/a2/a6, because `BLITZ_BASEJSR1/2` declare d2-d7/a2 changed
      (a call through a6 may be anything). The mathffp calls are
      `FFPJSR1/2` in vallib: mathffp is a system library and changes
      d0/d1/a0/a1 only (the original keeps its digits in d5-d7 across
      SPFlt), so only those are declared. The helpers are
      `BL_ALWAYS_INLINE` with the pointer and the count as locals, the
      "-" path takes the one copy of `notneg` and negates after it, the
      integer part skips the scale (the original computes 10^n there and
      drops it), and `doval_c` is `BLITZ_ENTRYPOINT` (it saves nothing;
      the original changes d0-d7 too; editlib, the one library caller,
      takes a2 and the others as changed). 3138.2 -> 1670.2 (1927.0),
      0.87 times the original. New library cases (`systest_cmdlibs`,
      Val): every path of `doval2` with three bytes behind the string
      (#302 reads it for hex, binary and "-"), the count ending inside a
      number, and the pairs `[orig]` / `[fixed #304]` (10 and 13
      fraction digits) and `[orig]` / `[fixed #303]` (40000 characters);
      recorded from the original, the C build of main gives the same
      cases.
    - Str$ (printlib, task 11.2): every output went through `prtype2`,
      `prtype1` and `output`, three functions, and each saved
      d2-d7/a2/a6 because the ALibJsr in `output` may change them; Str$
      paid that three times on top of `dostrstr`'s own save. Now the
      conversions (`plong2`, `pquick2`, `pfloat2`, `makeval`) return
      their text, `prtype1` only formats into `chars` and returns the
      length (called only with a Format string), and `prtype2`, `output`
      and the type dispatch (`numtext`, `wstr`, `ptype`) are inline in
      the two entries, Str$ and Print: one save per call (Print: per
      statement instead of twice per argument). The handler `strprint`
      is the original's assembler (it changes d1/a0 like the original;
      its callers, printlib's output calls between the two dosetout,
      take them as changed) instead of a thunk that saved d1/a0/a1
      around two C functions. The second dosetout gets a fresh register
      set, so d1/a1 of the first are not kept on the stack. Inlining
      `makelong` as well was slower (4785.1: GCC then spilled around the
      ALibJsr), so it stays a function. `r$=Str$(a)` 4964.7 -> 4164.6
      (4131.5), 1.01 times the original; printlib 4484 bytes. New library
      cases: Str$ and UStr$ of every type without and with four Format
      strings (the pairs `[orig]` / `[fixed #233]`, `[fixed #234]` and
      `[fixed #238]` where the values meet those fixed bugs), Print of
      the byte, word, quick, float and type 6 values.
    - Instr (stringfunclib, task 11.3): `instr` was a function with stack
      arguments that saved d2-d7/a2/a6, tested CaseSense per character
      and kept the position in a stack slot. It is `BL_ALWAYS_INLINE` in
      the two entries (`BLITZ_ENTRYPOINT` with `BLITZ_EP_KEEP_A6`, saving
      nothing), CaseSense is tested once, and the positions are counted
      with a test of the first character of Findstring$ before the
      compare of the rest (`addq/cmp.b -1(a0)/beq/cmpa/bne`, 44 cycles
      per position; the original compares from the first character at
      every position, about 106): the same result and no other characters
      read. `c=Instr(a$,"Blitz")` 4119.2 -> 2281.7 (3513.0), 0.65 times
      the original. New library cases: Instr with CaseSense On and Off
      (the characters around "a".."z"), matches at the start, middle and
      end, cut off by the end, empty strings, Startpos 1 to behind the
      string.
28. **A light call form for system libraries and memlib (group 12,
    2026-11-16).** `BLITZ_BASEJSR` declares every register but a3-a5/a7
    changed, so each C function with such a call saved d2-d7/a2/a6. The
    family `BLITZ_SYSJSR` (d0/d1/a0/a1 in and out through `struct
    bl_regs`), `BLITZ_SYSJSR1` (d0 in) and `BLITZ_SYSJSR2` (d0/d1 in),
    result d0, declares only d0/d1/a0/a1 changed, with a6 an input and
    `cc` and `memory` clobbered. Its contract (`blitzlib.h`): a library
    by the AmigaOS convention, or memlib's membase table by the `KEEPS`
    cases of `libs/tests/systest.c` (localloc/globalloc change d0/d1,
    locfree/globfree d0/a1; recorded from the original, so a memlib
    rework that breaks them fails `libs_emulator`). `BLITZ_BASEJSR`
    stays for open contracts (fontlib's previous input handler; the RWE
    userlibs are unchanged); `BLITZ_BASEJSR1/2` had no compiled use left
    and are gone. No d0/a1 form: locfree takes the general form, which
    costs `moveq #0,d1`/`move.l d1,a0` (8 cycles) on the rare free path.
    vallib's `FFPJSR1/2` are `BLITZ_SYSJSR1/2`; the `memory` clobber
    costs nothing there (vallib.obj byte-identical, `bstr.val` 1670.2).
    strings2lib's `stringass_new` (reached twice per call of a procedure
    with a string parameter and a local string) saves d2/d3/a6 instead of
    eight registers and keeps no stack frame (326 -> about 120 cycles of
    frame code by the count of `analyse-sysjsr.md`): `bstr.pstr`
    11989.6 -> 11560.6 (11614.0), `bstr.fstr` 12688.1 -> 12252.6
    (12353.5), two runs, both now faster than the original. The system
    calls of fontlib, intuifontlib, environlib, arexxlib (ErrorMsg, with
    the general form for its a0), speaklib and `BL_FREESIGNAL` (seriallib,
    utillib, brexxlib) save a6 instead of eight registers, -124 cycles
    per function, none in a benchmark. sortlib `FFP_CC_`, runerrlib
    `spfix` (a flag from the CCR in the same asm) and gtlib `addglist`
    (a2 argument) keep their private forms.
29. **The list commands and Sound (tasks 13.20 to 13.22, 2026-11-16).**
    C, no assembler, by the analysis `analyse-g13-sound-lists.md`
    (cycles counted from the disassemblies, then measured in
    `tests/perf`, two runs; the original in brackets).
    - NextItem/PrevItem (task 13.20): `firstitem2`/`lastitem2` were
      functions with a stack argument (`move.l d0,-(sp) : jsr : addq.l
      #4,sp`) and computed `link ? item : 0` branchless (SNE/EXT/EXT/AND);
      the current item was tested in an address register (`cmpa.w #0`,
      10 cycles) and every exit went to one shared RTS by BRA.s. Now
      both helpers are `BL_ALWAYS_INLINE` with an exit per result, the
      current item is read into a data register (`__asm__("" :
      "+d"(cur))`; GCC still adds a TST.l, 4 cycles, where MOVE set the
      flags; `register ... __asm("d0")` did not help), and the middle item
      is the likely path. GCC put the RTS of that path behind the rare
      exits as long as the function itself stood in the `!subs` table
      before its body (its address taken): the entries are `BLITZ_ALIAS`
      of a static C function, which gives the middle item its own RTS.
      Middle item 160 -> 144 cycles (148), first call 244 -> 140 (160),
      the end 142 -> 140 (126). `bstr.nextitem51` 231.8 -> 213.9 (217.8),
      0.98 times the original.
    - AddItem, KillItem, AddFirst, AddLast, ClearList (task 13.21):
      `unlinkitem` was a function with a stack argument (70 cycles of
      call overhead), and the entries saved a2/a6 (KillItem d2/a2/a6).
      `unlinkitem`/`linkitem` are `BL_ALWAYS_INLINE`, and the five
      entries are `BLITZ_ENTRYPOINT` with `BLITZ_EP_KEEP_A6()`: GCC works
      in d0/d1/a0-a2 and saves nothing (it keeps the array base in d1 and
      reloads an address register from it, `movea.l d1,a0`, 4 cycles each;
      a2 changes as in the original, whose `unlinkitem`/`linkitem` change
      a2 and save a3 only; no other library calls them, epcheck passes).
      The path of the benchmark: AddItem 412 -> 308 cycles (380), KillItem
      416 -> 286 (384). `bstr.addkill` 1025.7 -> 785.0 (961.0), 0.82 times
      the original. The library cases of the list commands
      (`systest_cmdlibs`, recorded from the original: every list state of
      0, 1 and 3 used and 0 or 2 free items with each current item, two
      traversals, KillItem until the list is empty) are unchanged.

29. **The remaining slower string commands (group 13, tasks 13.1 to
    13.5, 2026-11-16).** Analysis `analyse-g13-strings.md`; C, measured in
    `tests/perf` (cycles per call, two runs, the original in brackets).
    The library cases of `systest_cmdlibs` for Left$, UnLeft$, Right$,
    UnRight$, Mid$ (both forms), Hex$, Bin$, Cvi, Cvl and Cvq (lengths
    0, 1, 5, 8, 21 and 70 at even and odd addresses, the boundary counts,
    starts and lengths) were recorded from the original before the
    changes; the C build of main gives the same cases.
    - Left$, Mid$, Right$ (task 13.1): `left`, `right` and `mid` were
      functions with stack arguments, because the speed profile has
      `-fno-inline-functions -fno-inline-small-functions` (the pattern of
      Instr). They are `BL_ALWAYS_INLINE`. `BLITZ_ENTRYPOINT` only where
      GCC saved a register: Right$ and Mid$(String$,Startchar) took a2 as
      a temporary, UnRight$ d2; Left$, UnLeft$ and the three-argument
      Mid$ save nothing either way. The three entries may change a2 and
      d2, as the original does (it pops the lengths) and as Instr does; no
      library calls them. `r$=Mid$(a$,3,5)` 1642.7 -> 1454.6 (1553.5),
      `r$=Left$(a$,5)` 1365.5 -> 1243.7 (1299.0), `r$=Right$(a$,5)` 1553.2
      -> 1424.2 (1495.5). Left$ alone is 80 cycles against the
      original's 72 (GCC loads the length into a register before the
      compare, the original compares with `-(a2)`); the statement is
      faster through the port's string fetch and assignment.
    - Hex$ (task 13.2): GCC compiled `v >> 28` as `moveq #28 : lsr.l`,
      64 cycles per digit. A rotate (`v << 4 | v >> 28`, `rol.l #4`) and
      a table of the 16 digits, read PC relative (no relocation), make a
      digit `rol.l/moveq/and.l/move.b 0(a0,d2.l),(a3)+/dbf`, 56 cycles
      (the original's LSL/ROXL pairs and the "9" test: 120). 16 bytes of
      data. `r$=Hex$(a)` 1684.7 -> 1065.7 (1570.0).
    - Bin$ (task 13.3): `'0' + (v >> 31)` became `add/subx/neg/addi.b`,
      60 cycles per digit and a save of d2 (the original: `moveq #48 :
      lsl.l #1 : addx.w`, 36). The same treatment as Hex$ needs no addx:
      a rotate by 4 and a table of the 16 groups of four digits (64
      bytes, PC relative), copied by four `move.b (a0)+,(a3)+` (a3 may be
      odd; an empty asm on the pointer keeps GCC from `d16(a0)`): 110
      cycles per four digits, 27.5 per digit; `BLITZ_ENTRYPOINT` drops
      the save. A new loop `bstr.bin` (`r$=Bin$(a)`): 2784.2 -> 1695.2
      (2119.0, perf-original).
    - The long copies of Mid$ and Right$ (task 13.4): `movestr` is a byte
      loop, 22 cycles per byte; the original's movestring copies 8 bytes
      per pass, about 14.25. Mid$, UnRight$, Right$ and both Mid$ forms
      copy below 8 bytes with an inline byte DBF loop (without the outer
      loop for 64K and more, which only the long path needs) and jump
      from 8 bytes on to one out-of-line `movelong` (`BL_STRCOPY`: by
      longs where the source and a3 have the same parity, 7.5 cycles per
      byte, else 8 bytes per pass as the original). The copy goes
      forward with a3 at or below the source, so the long and 8-byte
      steps give the bytes of the byte loop. `movelong` keeps the start
      of a3 in an address register (an empty asm), so it saves nothing;
      the three-argument Mid$ now saved d2 and is `BLITZ_ENTRYPOINT` too.
      The four inlined `BL_STRCOPY` of the analysis would have added 524
      bytes; this adds 180 (text 2500 -> 2680). The short case stays
      at or below the original (Right$ with 5 bytes 248 cycles against
      262, Mid$ 270 against 306). New loops `bstr.midlong`
      (`r$=Mid$(n$,3,40)`, n$ 60 characters) 2837.2 -> 2389.7 (3136.0)
      and `bstr.rightlong` (`r$=Right$(n$,40)`) 2804.2 -> 2349.2
      (3077.0); `bstr.mid` 1454.7 -> 1452.2, `bstr.right` 1424.2 ->
      1414.2 (on main 60e0fb53). The two loops moved `bstr.pstr` and
      `bstr.fstr` of the port (11560.6 -> 11331.6) and of the original
      (11614.0 -> 11382.5) without a change of the libraries (a new
      string variable instead of n$ moved them the other way): those
      loops depend on the layout of the program; their targets follow
      perf-original.
    - Cvi, Cvl, Cvq (task 13.5): `cvbytes` was a function with three
      stack arguments that saved d2-d4 and tested the length per byte
      (the fix #288). It is `BL_ALWAYS_INLINE` with the length tested
      once: a string of 2 or 4 bytes or more is read by the original's
      instructions in a small asm block (`move.b (a0)+,d0 : lsl.w #8,d0 :
      move.b (a0),d0`, into the low word for Cvi, whose upper word stays
      the string address's; for Cvl two such words with `swap` between,
      80 cycles against the original's `lsl.l #8` DBF loop of 168; C
      gave or-based code about twice the original's for Cvi), a shorter
      one by the old byte loop. The entries are `BLITZ_ENTRYPOINT` (Cvl
      takes a2 as a temporary). Nominal cycles from the entry to its
      `rts` for a string long enough: Cvi about 460 -> 98 (the original
      62; the length test of #288 is 36 of them), Cvl and Cvq about 600
      -> 144 (the original about 200). No benchmark loop; the library
      cases cover 0 to 6 bytes at even and odd addresses.
30. **lmullib (tasks 13.30 and 13.31, 2026-11-16; analysis
    `~/.local/share/bb2-c-work/analyse-g13-mul.md`).** The extra cost of
    the C routines was not the fix of #199 (its arithmetic is 4 and 8
    cycles cheaper than the original's) but GCC's code around it: a
    detour on the positive path, one more saved register and MULU source
    words taken from a instead of b (MULU takes 2 cycles per 1 bit of
    its source word), so the port was between 38 cycles faster and 46
    slower than the original for Long and between 62 faster and 82
    slower for Quick, depending on the operands; the comments that said
    "the same MULU on the same words, 8 cycles fewer" were wrong about
    the built code. Both routines are `BLITZ_ASM_ROUTINE` and use the
    original's MULU source words (the words of |b|), so the MULU time is
    the original's for every input; they keep d2-d7 and a0-a6 and change
    d1, as the original; the CCR is no result.
    - `longmult` (listing L2, 84 bytes): the product modulo 2^32 needs no
      signs, so only the sign of b is tested and a negative b negates
      both factors; hi(b) = 0 skips lo(a) * hi(b), hi(a) = 0 as well
      hi(a) * lo(b) and the register save (one MULU and an RTS). 50 to
      222 cycles below the original for every input (counted with a
      68000 model over 30 million pairs and all pairs of scaled words).
      `c=a*b` (123456 * 789) 408.6 -> 284.6 (383.0); the new loop
      `bmath.lmuls`, `c=a4*b4` with 100 * 20, 184.6 (370.0). New library
      cases (`systest_mathstr`, longmult): every path, zero factors and
      words, $80000000 as a and as b, the four sign combinations and
      products that overflow, recorded from the original; the seven
      cases where #199 shows are pairs `[orig]` / `[fixed #199]`.
    - `quickmult` (listing Q1, 100 bytes): Quick truncates the product of
      the magnitudes, so the magnitudes stay; the sign lives in the code
      path (four entries by the sign bits, two copies of the core, one
      followed by NEG.l), so the flag register, its MOVEQ and the final
      TST/BEQ go and two registers are saved instead of three. The sign
      bits are tested as in the original: negating both factors for a
      negative b, as longmult does, is wrong here, because NEG leaves
      $80000000 negative (`$80000000 * $FFFFFFFF` would give -0.5). The
      core (52 cycles besides the MULU, the original's 60) keeps the #199
      fix. 42 to 52 cycles below the original for every input. Paths for
      zero words of b (listing Q2) were left out: they cost 24 cycles
      when both words of b are non-zero. `q3=q1*q2` (12.5 * 3.25) 458.1
      -> 386.8 (434.0). New library cases (quickmult): the four sign
      combinations, zero factors and words of a and b, $80000000 as a
      and as b, `$80000000 * $FFFFFFFF`, the truncation of the low
      product and products whose integer parts overflow; the six cases
      where #199 shows are pairs `[orig]` / `[fixed #199]`.
31. **The blitter commands, group 13 (tasks 13.10 to 13.12,
    2026-11-16).** After the analysis `analyse-g13-blit.md`; cycles per
    call (`tests/perf`, five runs), before -> after (the original). The
    loops that wait for the blitter count the CPU code before the first
    BLTSIZE write and behind each wait; what lies between a BLTSIZE write
    and the next wait is hidden by the blit.
    - Blit and BlitColl (blitlib, task 13.10): C. The modulo pair is the
      original's `MOVEQ #-2:SWAP:MOVE bpr:SUB ebwidth:SUBQ #2` (an asm
      statement; GCC built it with a long constant), the shift a rotate
      (`ROR.w #4:ANDI.w`, 22 cycles against `MOVEQ #12:LSL` 34), `bcon0`
      PC-relative as in qblitlib, the next plane's address computed
      before the wait, and BlitColl's -1/0 a `LONG` (no `EXT.l` in the
      entry; the Amiga mode code keeps the word, its upper word 0, the
      kind T deviation). The waits in the plane loops are
      `BL_BLITWAIT_TST` (new in `blitzhelp.h`: the first read of DMACONR
      as `TST.b 2(An)`, no data register), used only where
      `BL_BLITWAIT`'s read into a register made GCC spill (decision of
      the coordinator: not across all libraries). Blit with Excessonoff
      was a normal function (it saved d2-d7/a6) whose excess loop read
      the shape's depth again after the chip writes (a volatile UWORD
      store may alias it), kept BLTSIZE and the plane pointer on the
      stack and read DMACONR into a stack slot; now its Blitz mode code
      is `BLITZ_ENTRYPOINT` (saves nothing, as Blit; a6 kept with
      `BLITZ_EP_KEEP_A6`), the depths are read once and the excess
      planes counted down, and the nullsub `b_doblitex` for other
      libraries keeps its normal C function (the build check refuses a
      call into an `__entrypoint` routine). Blit 2116.9 -> 2050.3
      (2108.9), Blit 32 4729.4 -> 4675.9 (4722.9), Blit of a 1-plane
      shape 928.4 -> 903.9 (933.9), of a 6-plane shape 3885.4 -> 3768.8
      (3873.9), Blit with Excessonoff (two excess planes) 2064.8 ->
      1890.9 (1944.4), BlitColl 901.4 -> 873.5 (881.4): below the
      original, so no assembler block. Block is unchanged. The plane
      loops end with `TST/BNE/SUBQ` and a `BRA` back instead of `DBF`
      (GCC rotates the loop); the instructions in front of the wait are
      hidden by the blit, so only the `BRA` counts. blitlib 2864 -> 3068
      bytes (the second copy of the Excessonoff code). The library cases
      of `systest_blit` (Blit, Blit excess, Block, BlitColl in both
      modes) give the same values.
    - ClipBlit (2dlib, task 13.11): an assembler block (`BLITZ_ASM_BLOCK`
      with `BLITZ_AB_ROUTINE`, as 2dlib's other blocks), from the
      analysis's draft: the original's `b_docblit` and `b_docblitx` with
      short branches, `TST.b 2(a1)` as the first read of DMACONR, `ADDQ`
      for `ADD #2`, the shift as `MOVEQ #15:AND:ROR #4`, 2 * (x ASR 4) kept
      in a0 from the right-edge test and added once (the original shifts x
      again and adds twice), and the next plane's B pointer and
      destination computed before the wait. Every value written to the
      blitter is the original's, and the original's leftover `MOVEM.l
      d6-d7,$060(a1)` stays (the caller's d6/d7 in BLTCMOD/BLTBMOD and
      BLTAMOD/BLTDMOD, overwritten before the blit; no new deviation). The
      Amiga mode code is the original's again (`MOVEM.l d1/a1` around
      OwnBlitter, the Blitz mode code, the wait, `JMP` DisownBlitter), and
      `bcon0` is read PC-relative from the variable ClipBlitMode writes.
      The C (decision 20) stayed 2.4 % behind; two C attempts of the
      analysis stayed behind too (GCC kept the dummy read in a stack slot,
      d6/d7 being the caller's junk, and spilled y). ClipBlit 0,100,100
      2577.3 -> 2378.3 (2516.9), ClipBlit clipped at the four edges (four
      calls) 8979.9 -> 8042.8 (8643.9), ClipBlit of a 1-plane shape with
      Excessonoff 2513.4 -> 2232.3 (2310.4); 2dlib 13308 -> 11608 bytes
      (the four inlined C copies of the clip code are one block).
      `systest_blit` gets a 1-plane shape (Blit, Blit excess, ClipBlit
      clipped at each edge and outside, ClipBlit excess), recorded from
      the original and confirmed with `libs_emulator_originals`; the
      block gives every case in both modes.
    - QBlit with FlushQueue (task 13.12, `bgfx.qblit8f` 21 cycles over the
      original): FlushQueue is the original's inline `MOVE.l 4(a3),(a3)`,
      the plane loop and the queue item already the original's (decision
      23); the difference was QBlit's modulo pair, which GCC built as
      `MOVEQ #-2:SUB:ADD:MOVE.l #$FFFE0000:OR` (40 cycles, and a second
      register) against the original's `MOVEQ #-2:SWAP:MOVE:SUB:SUBQ #2`
      (32), and so qblitlib 3052 -> 3036 bytes. With the original's sequence as an asm statement, as in
      blitlib: 4 QBlit + FlushQueue 9081.8 -> 9044.0 (9089.9), 8 QBlit +
      FlushQueue 18083.7 -> 17999.8 (18059.9), 4 QBlit + UnQueue 12743.8 ->
      12666.0 (12785.9), 8 QBlit + UnQueue 25359.8 -> 25209.8 (25417.9).
      bblitlib needs nothing (4 and 8 BBlit + UnBuffer are below the
      original, decision 23). stencillib's SBlit has the same C modulo
      pair and the shift by `LSL`, but no benchmark line; it measured
      below the original in decision 20 (2682.9 against 2702.2), so it
      stays as it is. The library cases of `systest_blit` (QBlit, QBlit
      excess, UnQueue in both modes) give the same values.

## Risks / Trade-offs

- A caller outside the analysed set (`analyse-thirdparty-regs.md` section 7:
  static analysis of 566 library images) relies on a register the new rule
  drops. Mitigation: the deviation lists the rule and the exceptions; the
  site page says what a library author may rely on; KEEPS tests for the
  exceptions.
- A toolchain update may make gcc use a register an alias entry keeps by
  nature. Mitigation: its KEEPS test fails first (Decision 3).
- Without the thunk a tail call of the entry becomes a direct jump into the
  C function; the `-fcall-saved` failure shows tail calls interact with
  register contracts. Mitigation: exceptions are verified in the
  disassembly with the shipped flags.
- Assembler blocks are less maintainable and gcc cannot check them.
  Mitigation: small blocks from the original's code, covered by the library
  tests (they run original and C build against the same expected data) and
  by new cases where the original leaves an entry condition undefined.
- Timing in an emulator: Amiberry's cycle model was not checked against
  hardware (`report.md`, open questions). Budgets compare library sets on
  the same emulator; they are not absolute hardware numbers.
- #1048: an error check in ldivlib/lmullib would add a jsr/rts per division
  in release programs. Nothing here adds one.

## Migration Plan

One group of tasks per implementer round, one commit per topic, in the order
of tasks.md. Each commit builds, passes `ctest --preset all` and
`ctest --preset bug-orig`, and names its changed results. The benchmark and
the rule (groups 1 to 3) come first, because the small-entry blocks and the
budgets need them. Rollback is one commit per entry group.

## Open Questions

- Where the benchmark sources live (decided in task 1.1, Decision 11):
  `tests/perf/`.
- Budgets (decided): each command's budget is the current measurement plus
  a tolerance (a guard against slowing down) and is tightened after each
  step; the original's measurement is recorded next to it as the target.
  The goal is the original's speed or better (the user's rule: no change
  may make correct programs slower). The tolerance (task 1.4): the larger
  value of two runs plus 1 %, rounded up, plus 3 cycles
  (`tests/perf/budgets.txt`): above the resolution (0.5 cycles per call at
  N = 2000, 20 for Sort and Cls at N = 50) and the run-to-run spread, small
  enough that an extra register save (8 cycles) or a lost inline helper
  fails a small entry. The spread of two runs of the same build (the
  first after a new snapshot key, the second from the snapshot): at most
  1 cycle per call for the CPU-bound commands (spec: repeated to 1 cycle),
  more where the loop waits for the blitter or follows the frame: Cls 300
  of 84000 per call (0.4 %), Circle and Circlef 15, BBlit 14 per 8 blits,
  and the PlayModule frame value 3214 against 3000 (7 %; the analysis's
  two runs agreed there). Five further runs of one build (all
  `snapshot=fallback`) differ by more than 1 cycle per call for 30
  commands (Cls 540, BBlit 8 loop 26, Circle 20, Circlef 15, PlayModule
  frame value 7, the rest at most 8): those 30 carry a tolerance of their
  own, `B~T` in `budgets.txt` (B the highest of the five runs, T twice their
  spread, rounded up; B + T the limit); the other commands keep the rule
  above. Every measured command has a budget (125 with the
  seven `emptylong` checks of the subtraction), not only the 101 that are
  slower; a measured command without a budget fails the test.
- Whether EventWindow and RawStatus keep their registers by nature
  (task 2.1; if not: reduced thunk or, for RawStatus, an assembler entry).
- #1058 (decided): the InitSound fix is done in this change, task 2.4, in
  one commit with its test and `Fixes #1058`.
