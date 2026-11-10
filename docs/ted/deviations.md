# Deviations of the C version from TED 1.1

Reference: vasm build of `ted.s` ([reference.md](reference.md)). Rule
(CLAUDE.md "Original bugs are fixed deliberately"): bugs of the original
not fixed yet are marked `BUG(orig)`; a fix is an intended deviation of
kind **F**, listed here with its issue and its test; other deviations
only where technically unavoidable – marked in the code with `DIFF:` and
a justification, and listed here in full.
Categories as in [`../libs/deviations.md`](../libs/deviations.md)
(**B** decided by the user, **T** technically unavoidable, **K** silent
correction – not permitted, **F** fixed original bug). The lines concerning the file requester shared with freqlib
are additionally listed there in section 5a.

As of: phase 2c (integration review). All "safer" spots of phase
2b are back to original on the Amiga (section "Reverted in phase
2c" below); guard checks exist only in the host build
(`#ifdef TED_HOST`, marked with `ORIG_HAZARD`) – they do not change the behaviour
of the Amiga version and are not deviations.

| Location | Original | C version | Kind | observable? |
|---|---|---|---|---|
| `flink` (ted.s:7953; src/filereq.c) | relocates the embedded requester hunk `filereq` itself at startup (312 RELOC32, without cache flush; a second run would relocate twice – original bug #360) | dropped: the requester is C code from `shared/filereq/filereq_core.c`, LoadSeg relocates | **T** | no |
| Requester data block (src/filereq.c) | TED's `filereq` differs from freqlib in 7 padding bytes (`"OK",0,$06` instead of `$00` …, rest at the hunk end) | data block of the freqlib version (one source, "variant C") | **B**/**T** | no (bytes are never read) |
| `ezfreq` → `filerequest+36` (src/filereq.c) | a0 = `reqstruct`, the requester reads the block directly | `filerequest()` copies the 4 longwords to `fq_args` and calls `fq_request()` | **T** | no (the block is not changed while the requester runs) |
| `jumptable`/`menutable` (src/input.c) | packed 6-byte entries `{code.b, flag.b, fn.l}` | array `struct jtentry` (function pointers) in the same order, same end markers | **T** | no (nobody else reads the tables) |
| `escape` (ted.s:1016) | `lea 12(a7),a7` + `bra freeall`: returns behind `bsr mainedit` | `edquit = TRUE`, `mainedit` returns directly after the handler | **T** | no |
| `erase`/`getfname3`/`doopen`/`getbname` | `addq #4,a7`: return past the caller | return value, every caller checks and returns | **T** | no |
| `outmem`/`memcont` | `move.l memstack,a7 / bra memcont` | `longjmp(*memstack)` to the `setjmp` in `mainedit` (like the original, also discards blitz2 frames); `memstack == 0` (O1, #370): `memcont_nostack` (src/glue.c) does it literally (a7 = 0, memcont, `bra mainedit`) – crash as in the original, only the called C routines use the stack differently | **T** | no (crash location at O1 possibly a different one) |
| Sentinel `dc ' :'` before `filepath` (ted.s:6139) | `.tomain` reads `filepath[-2]` (= ':') on an empty path | empty path is checked explicitly (same result) | **T** | no |
| edstruct copy (ted.s:24) | 14 words in one go to `tokens…remark` | field by field into individual globals | **T** | no |
| chmap blits in `scrollup`/`scrolldown`/`eraseline`/`cls` | blitter also on the character map | `chmap_*` in the core with the CPU (same bytes), bitplanes still via blitter | **T** | no (timing: blitter and CPU work in parallel in the original) |
| fixed strides over structures (survey-07 G.1) | `ga4 + n*84`, `hp1 + n*86`, `ga10 + n*44`, `bo14 + n*28/56` | pointer tables `intgads`, `hps`, `colgads`, `colbords` (src/data.c) | **T** | no |
| comdata entry points (src/glue.c) | partly save nothing (openreq, getinput, error …) | REGSTUB saves all registers except the results | **B** (as the libraries: "at least what the original preserves") | no |
| `edstart`/wbheader, `dosopen` fails (src/main.c `edstart_c`) | `wb_exit` `rts` returns to `outside`, a1 undefined (survey-01 I.2) | `edstart_stub` always sets a1 = 0: `outside` continues as standalone (without DOSBase) | **T** | no (dos.library is never missing) |
| Menu items: padding byte after `Command` (src/data.c) | `dc.b 'X '`: padding byte = space | `struct MenuItem` has padding there, C initialises it with 0 | **T** | no (never read; not in the COMPILER copies of `getstuff` either) |
| `sleepon` path "already active" (src/main.c) | after the `movem` restore, `makemenus` destroys a0/d0 of the blitz2 caller (survey-01 I.7) | glue saves all registers (line "comdata entry points") | **B** | no |
| `swapd0bord` (ted.s:3238; src/reqs.c) | button index ≥ 16 (#384) (colour value ≥ 4 from a damaged options file): swaps bytes 4/5 at `bo14 + (d0*56).w` and +28 | same computation relative to `defaultreqbo14` (original strides); the C data is laid out differently, other variables are there | **T** (data layout) | only with a damaged `l:blitzeditor.opts` |
| `blokfixb` (ted.s:5393; core/lineedit.c) | for a multi-line block tests `btopy` instead of `bbotx` against 128: an end column ≥ 128 stays, a block starting at line ≥ 128 gets its end column set to the end of the line | tests `bbotx` for every block | **F** | #349: observable in sources over 128 lines, typing in a multi-line block no longer extends it to the end of its last line; host test `blokfix and blokfixb` (`test_lineedit_keys.cpp`) |

## Reverted in phase 2c (Amiga again like the original)

| Location | up to 2b | now (Amiga) | Host |
|---|---|---|---|
| `divu` (all 11 `divu` of ted.s; `divu_w`, `tabrem`, `showvprop`/`showhprop`, file requester `fq_divu_w`) | C division; by 0 like overflow, otherwise `__udivsi3`/`__umodsi3` from libnix20 (need `UtilityBase` → crash at every spot) | `divu_w` in src/glue.c = the instruction `divu.w` itself (overflow: V, registers unchanged; by 0: trap 5 as in the original); ctest `checklink` ensures that no arithmetic helper is linked | C version, abort at 0 (O3) |
| `findtoke`, `tokit` `.tsttoke` with `tokens == 0` (#359) | unknown token / no match | walks the "node" at address 0 (word at 4, name from 6, `next` = longword at 0) – reachable: TED standalone loads a tokenised file | guard check |
| `freeall` more rounds than lines | abort | reads the "item" at 0, `FreeMem(0, …)`, continues | guard check |
| `freeupa5` `.first` with an inconsistent list | no write access | `clr.l 4` (overwrites the ExecBase pointer, like the original) | guard check |
| `chmap_scrolldown` | reading before chmap was dropped | reads the bytes before chmap like the blit | guard check |
| `swapd0bord` index ≥ 16 (#384) | ignored | swaps like the original (line above, T because of data layout) | – (Amiga only) |
| `outmem` with `memstack == 0` (O1, #370) | `longjmp` through NULL | a7 = 0 + memcont (line above) | – |
| `openio` → `mycli` (Z flag, #368) | handle != 0 | `WBenchToFront` via `callregs`: d0 and CCR as in the original; comdata+60 passes the flags on (`REGSTUB_CCR`) | – |
