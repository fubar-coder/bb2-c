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
   functions only. Internal tokens (operators, strcomp, copstring,
   lastring/astring, the prologue routines, freelast, clrloc, newmem,
   localstat), nullsubs and all error checks (`BLITZ_ERRCHK_THUNK`) keep
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
6. **Speed profile** adds strings1lib, strings2lib (measured +8 and +4
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
    GCC keeps the bitmap pointer and the plane step on the stack), so
    no assembler block (decision 15). #1045 is not fixed; its three
    additions are marked, and a fix (the step zero-extended once
    outside the plane loops) costs no cycle in the loops. The library
    test `systest_blit` (new) covers the commands in both modes: every
    shift, junk in the upper words of x and y, the handle, the excess
    planes, the modes, a collision and none, ClipBlit clipped at each
    edge and outside; recorded from the original, the C build of main
    and of the change give the same cases.

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
