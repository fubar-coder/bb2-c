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
| `blocktab` (ted.s:327; core/loadsave.c) | without tokenising the copy leaves the pointer behind the 0 byte: every tabbed line keeps it as its last character (`numchars` + 1), saved as `text\0<LF>`, an empty line after each on reload | the pointer goes back onto the 0 (as `blockuntab` does) | **F** | #347: observable in standalone TED (tokenise off): Block Tab no longer adds a 0 byte to each line; host test `blocktab and blockuntab` (`test_search.cpp`) |
| `blockuntab` (ted.s:361; core/loadsave.c) | removes the first tab-size characters of every block line whatever they are; a line shorter than the tab size, or tab size 0, becomes empty | removes up to tab-size leading spaces and keeps the rest of the line | **F** | #348: observable with Block Untab over lines not indented by the tab size (no text is lost any more); host test `blocktab and blockuntab` (`test_search.cpp`) |
| `deleteline` (ted.s:1173; core/lines.c) | deleting the line of a one-line block sets `bboty` to −1 without `blokdis`: the block menu items stay enabled, the status still says "Block", the F1 start mark stays | `blokdis` when the deleted line was the whole block | **F** | #353: observable in the block menu, the status line and a later F2; host tests `block coordinates: …` and `deleteline of a one line block ends it with blokdis` (`test_lines_blocks.cpp`) |
| `join` (ted.s:1305; core/lines.c) | frees the appended line's item with `numchars` bytes instead of `numchars` + 9 (`lenitem`): 8-16 bytes lost per Join until reboot | frees `numchars` + `lenitem`, as `freethis` | **F** | #364: observable as free memory (no leak per Join); host test `join and split: line contents` (`test_lines_blocks.cpp`; TED A/B `03-join` logs the memory but does not compare it) |
| `readline` (ted.s:3375; core/fileio.c) | with tokenise on, only lines ended by LF are tokenised: the last line of a text file without LF and the first part of a line cut at the buffer end stay text | such a line is tokenised too when an earlier line of the file ended by LF or the line has no byte ≥ $80 (token pairs of a tokenised file and a half token at the end, #379, stay as they are) | **F** | #376: observable when a text source is loaded with tokenise on (under blitz2): its keywords there become tokens (token colour, Help, saved tokenised); host test `readline tokenises a text file's line without end` (`test_readline_noeol.cpp`), TED A/B `28-long` (`@fixed #376 R5-C5 Rc-Cc`: its line over 129 bytes) |
| `join`, `split`, `blokcopy` (ted.s:1242, 1313, 1940; core/lines.c) | after `blokfix` has ended in `blokdis` (the block start pushed past column 127), they go on adjusting the line bounds: Join leaves `bboty` = $FFFE, Block Copy adds `blokdis`'s clobbered d1 (256 or 192): a "ghost block" that Block Kill/Save act on | no further adjustment once `blokfix` has removed the block | **F** | #346: observable with Join when the block start of the next line moves past column 127 (no inverted ghost block, Block Kill deletes nothing); in `split` and `blokcopy` not reached as far as checked; host tests `block coordinates: …` and `join: no ghost block after blokdis` (`test_lines_blocks.cpp`) |
| `a0toname` (ted.s:2990; core/services.c) | splits the name blitz2 hands to TED by the caller's d0 (blitz2's LoadSeg result) instead of the copied character: normally `filepath` = "" and `filename` = the whole name, but with a low byte `/` or `:` TED opened a wrong file or none | never splits: `filepath` = "", `filename` = the whole name (the original's normal case) | **F** | #340: observable only where the original failed (TED started from blitz2 without the file, or "Couldn't open file!"); screen title and file requester as before; host test `a0toname` (`test_services_core.cpp`) |
| `getbchar2` (ted.s:3875; core/fileio.c) | without tokenising, `move.b` returns a byte ≥ $80 negative: Block Save and Block Copy take a Latin-1 character for the end of the block and stop there | the byte is zero-extended, as in the token path | **F** | #362: observable in standalone TED (tokenise off) with Latin-1 characters in a block (the whole block is saved or copied); TED bug-orig test `ted-getbchar-latin1-block-end` |
| `bloksave`, `blokkill` (ted.s:1706, 1628; core/lines.c) | detokenise the middle lines of Block Save and the last line of Block Kill with `detoka5` also without tokenising: a Latin-1 character and the byte behind it are written as a token name (`?????` in standalone TED, where `findtoke` walks the empty list, #359) | without tokenising, Block Save writes the middle lines from the line as they are, Block Kill reads the last line without `detoka5` | **F** | #354: observable in standalone TED (tokenise off) with Latin-1 characters in the middle lines of a saved block; TED bug-orig test `ted-detoka5-without-tokenise`, host tests `bloksave and blokload through the file hooks` (`test_lines_edit.cpp`) and `blokkill and blokforget` (`test_lines_blocks.cpp`) |

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
