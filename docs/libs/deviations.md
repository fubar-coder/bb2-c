# Deviations of the C ports from the originals

Complete overview of all known places where the
C versions (`libs/acidlibs/*.c`, `libs/deflibs/*`, `programs/blitz2`, `programs/ted`) behave differently from the
originals (CD binaries or `blitz2.s`). Basis: all `DIFF` markers
in the sources, the DIFF sections in [cmdlibs-port.md](cmdlibs-port.md) and
[deflibs.md](deflibs.md), the project conventions and a review of our own ports
for unmarked deviations. `BUG(orig)` (original bugs deliberately
carried over) is **not** listed here, but in the header comments
and in `cmdlibs-port.md`.

Rule (user decision): bugs of the reference are reproduced and
documented, not fixed. Deviations only where they are technically
unavoidable, with a reason. The deviations already present
remain (not reverted), but must be fully
documented here.

## Categories

| Code | Kind | Meaning |
|---|---|---|
| **B** | user decision | project convention, fixed for all ports |
| **T** | technically unavoidable | the original is undefined here (register leftovers, stack contents, timing, memory layout) or cannot be expressed in C |
| **K** | silent correction | the original could be reproduced, the C version behaves "more safely" (trap instead of jump into nowhere, return instead of crash, register preserved). No longer permitted under the new rule; existing cases remain, but are listed here |

"observable?": whether a Basic program can see the difference
(result, output, crash) or only a register/memory comparison.

## 1. Conventions (apply to all ports)

| Area | Original | C version | Kind | Reason / observable? |
|---|---|---|---|---|
| a2 (argument stack of the commands) | routines fetch lengths/types with `MOVE.x -(a2),dN` and leave a2 decremented by different amounts depending on the path | all routines get a2 as a `BL_REG(a2)` parameter, read via a local pointer (`BL_POPL`/`BL_POPW`), a2 stays unchanged | **B** | user decision; `preplibst` sets a2 anew before every call, the compiler never reads it afterwards (cmdlibs-port.md "General"). Not observable. Affects among others stringfunclib, vallib, printlib, editlib, intuifontlib, speaklib, audiolib, screenslib, gtlib, asllib, blitzcoplib, animlib, arexxlib, seriallib, fileiolib, brexxlib (Type) |
| Register preservation | routines change arbitrary registers (spec: everything except a4/a5/a7) | thunks preserve at least the registers the original preserves, often more (d0-d1/a0-a1 for statements) | **B** | agreement "thunks preserve at least what the original preserves"; additionally preserved registers (e.g. d1 in maxslib/modlib, d2/a2 in maxlenlib) are not results. Not observable |
| Error checks "at least the same" | check routines in the error check area | likewise (`BLITZ_ERRCHK`), messages as assembler data there | – | no deviation, except section 6 |

## 2. System libs

| Library/routine (file:line) | Original | C version | Kind | Reason / observable? |
|---|---|---|---|---|
| memlib err1 (memlib.c:76) | jump out of localloc/newmem with saved registers on the stack, never returns | err1 returns, caller returns 0 | **K** | only if the runtime error trap returns (normally the program ends); practically not observable |
| memlib locfree err2 (memlib.c:171) | unknown block: one-time `TRAP #0` "Unable to Free Memory" (err2 in normal code); if the trap returns, the code continues in `newmem`, the registers saved by locfree are still on the stack | locfree returns after the runtime error | **K** | only with a returning trap; practically not observable |
| arrayslib newlocarr/newloclist (arrayslib.c:29) | swaps a4/a5 around the memlib call | passes the base directly | **T** | a4/a5 are fixed registers in C; memlib does not use a5. Not observable |
| arrayslib dimension calculation (arrayslib.c:90) | returns d1.w+1, changes d2 | d2 preserved, d1 undefined | **B** (register preservation) | compiler does not use d1/d2. Not observable |
| maxlenlib (maxlenlib.c:19) | changes d2 and a2 | preserves d2/a2 | **B** | not results. Not observable |
| maxslib doallocst (maxslib.c:33) | d1 = -1 after copying | d1 preserved | **B** | not a result |
| maxslib dofreest (maxslib.c:55) | d0 = memlib's d0 | d0 preserved | **B** | not a result |
| modlib domodquick (modlib.c:29) | d1 returned swapped | d1 preserved | **B** | not a result |
| runerrlib check chains (runerrlib.c:238) | continues through the error chain after a returning trap | C check returns | **K** | only with a returning trap (the debugger terminates). Practically not observable |
| fontlib calcoff (fontlib.c:45) | after a trap (no bitmap/font) it runs into the next trap | draws nothing; unreferenced check `bitmapchk` missing | **K** | only with a returning trap |
| inputoutputlib defin single character (inputoutputlib.c:45, :233) | `BRA err1` into the error check area (without debugger: jump behind the lib) | trap and message in normal code | **K** | without debugger: runtime error instead of jump into nowhere (observable only in this error case). The jump would be technically possible (cf. audiolib, screenslib, shapeslib) |
| wbstartuplib AddArg (wbstartuplib.c:74) | without memory, jump to `err` in the error check area | runtime error, NULL, caller aborts | **K** | only without memory |
| linklistlib SortList (linklistlib.c:35) | destroys a3 and d6 | a3 unchanged | **K** | statement, a3 is not read afterwards. Not observable |

## 3. Command libraries

| Library/routine (file:line) | Original | C version | Kind | Reason / observable? |
|---|---|---|---|---|
| 2dlib overflows (2dlib.c:44) | Polyf with more than 16 points / BitMapPoly edge lists overwrite the code behind polypts/l1 | overwrite the data behind them | **T** | memory layout; observable only with these overflows (BUG(orig)). FloodFill (yofill) is the original assembler, a3 as in the original |
| polylib overflows (polylib.c:34) | more than 16 points / 10 edges per side hit the lib's own code | hit the data behind them | **T** | memory layout; only with overflows |
| animlib err6/err7 (animlib.c:45) | `BEQ err7` into the error check area or err6 in normal code | reached via JSR (return address and saved registers of the C function on the stack) | **T** | GCC drops code after an inline BRA or merges the paths; target and message as in the original, only the stack at the trap differs; not observable |
| animlib a2 (animlib.c:45) | a2 decremented | unchanged | **B** | convention |
| arexxlib ReplyRexxMsg (arexxlib.c:62, :505) | with a non-RexxMsg: pops 15 never-saved registers (crash) | returns | **K** | observable only in this error case (crash vs. nothing) |
| arexxlib CreateRexxMsg (arexxlib.c:62) | copies -8(a2) to -4(a2) on the compiler stack | does not copy, a2 unchanged | **B** | convention; the compiler does not read the value |
| asllib title (asllib.c:44) | writes maxslib's d1 (-1) into the length cell of the title on the compiler stack | stack and a2 stay | **B** | convention; not observable |
| audiolib (audiolib.c:47) | a2 decremented (volumes/lengths) | unchanged | **B** | convention |
| bblitlib (bblitlib.c:25) | entries without nullsub save a4/a5 | likewise (note: no deviation in behaviour) | – | – |
| blitlib BlitColl Amiga (blitlib.c:323) | upper word of d0 = leftover from DisownBlitter | 0 | **T** | register leftover of the OS (undefined); #word function, not observable |
| blitzcoplib CustomCop (blitzcoplib.c:56) | a2 decremented by 4 | unchanged | **B** | convention |
| blitzkeyslib | – | no deviation (DIFF-free; binary has 6 more unused bytes) | – | – |
| displaylib a2 (displaylib.c:50) | a2 decremented as argument stack | unchanged | **B** | convention |
| displaylib error jumps (displaylib.c:50) | all errors: Bcc from normal code into the error check area | via JSR (one more return address on the stack) | **T** | like animlib; same target, only stack contents at the trap |
| displaylib table indices (displaylib.c:50) | sprite mode > 5, mode table beyond resolution 2 / fetch 3, doublescan > 6 read the lib's own code | read other data | **T** | memory layout; only with invalid values |
| blitzoslib coordinates (blitzoslib.c:66) | upper word of the coordinates passed to Boxf/Line/Locate = sign of the box field | word value sign-extended | **K** | reproducible; differs only for coordinates around 0/negative and only if the called lib uses the upper word |
| blitzoslib BCALL (blitzoslib.c:308) | Blitz calls from BoxShow (dobs) leave a3 = bitmap of the page | BCALL restores a3/a4 (and a6) after every call | **K** | **not yet marked as DIFF**; statements, a3 is not read afterwards |
| brexxlib SaveTape/LoadTape (brexxlib.c:60, :594) | `BEQ err1` into the error check area | trap and message in normal code | **K** | observable only for "Can't Open File" without debugger |
| cliargslib ParPath$ (cliargslib.c:24, :117) | branches on the CCR that dos.library leaves after NameFromLock | checks d0 = 0 | **T** | CCR of an OS function is undocumented; result the same |
| crunchlib CrM2 (crunchlib.c:57) | corrupted data index into code/data of the lib | likewise, but the layout of the C lib is different | **T** | only with corrupted CrM2 data |
| crunchlib work area (crunchlib.c:61) | clears 176 bytes on the stack before returning; uses a5 as work register | leaves locals in place; a5 unchanged | **T** | stack below sp, a5 is a fixed register. Not observable |
| editlib (editlib.c:24) | a2 decremented | unchanged | **B** | convention |
| fileiolib errors in normal code (fileiolib.c:64, :279) | err1/err2/err3/err6 (and message err7) in the error check area, without debugger jump into nowhere | trap and message in normal code | **K** | only in these error cases without debugger |
| fileiolib registers (fileiolib.c:64) | changes d4-d7, a2, a3 | unchanged; unused check `reclenchk` missing | **B**/**K** | not observable |
| freqlib error path (shared/filereq/filereq_core.c, header and `fq_request`; also applies to TED) | without chip memory/library/window: first long of the (uninitialised) FIB buffer | 0 | **T** | contents of uninitialised memory |
| gameiolib | – | no deviation (DIFF-free) | – | – |
| gtlib GTTags/GTSetAttrs (gtlib.c:58) | pop the tag pairs from a2 | a2 unchanged; unused check routine freqchk missing | **B** | convention |
| iffiolib iffopen (iffiolib.c:34) | a0 lost after dos Open | a0 preserved | **B** (register preservation) | **not marked as DIFF**, described in the header; not observable |
| ilbmifflib ilbminit (ilbmifflib.c:46) | a0 after failure = leftover from dos.library | a0 = file name | **T** | register leftover; described in the header |
| intuifontlib (intuifontlib.c:28) | a2 decremented | unchanged | **B** | convention |
| maxlenlib/maxslib/modlib | see system libs | | | |
| medlib (medlib.c:56) | LoadMedModule/SetMedVolume leave a3 on object+8 or track | a3 unchanged | **K** | statements; not observable |
| palettelib errors (palettelib.c:55, :288) | AGARed/Green/Blue `err3` and messages err0/err1/errd in the error check area | trap and messages in normal code | **K** | only in error cases without debugger |
| palettelib empty palette (palettelib.c:52, :726, :732) | ShowPalette/Use Palette Amiga mode with an empty palette runs into the Blitz code `b_showpal`, in both branches: from ExecVersion 39 on (`agamode`, a0 arbitrary) and below via `make4bitrgbs` (a0 = palette object); writes nothing | returns in both branches | **K** | result the same (nothing written); only with `agamode` and odd a0 on the 68000 an address error in the original |
| printlib (printlib.c:23) | a2 decremented | unchanged | **B** | convention |
| screenslib (screenslib.c:57) | a2 popped | unchanged | **B** | convention |
| screenslib err1 (screenslib.c:53, :412) | Can't open Screen: `BEQ err1` from normal code into the error check area | reached via JSR (one more return address on the stack) | **T** | like animlib/displaylib; same target (without debugger likewise jump behind the lib), only the stack at the trap differs; not observable |
| scrolllib | – | no deviation (DIFF-free; clsnotint after the binary) | – | – |
| seriallib ReadSerial (seriallib.c:53, :424) | a3 stays on the 1-byte buffer rb1 in the lib; a2 = unit slot | a3/a2 unchanged | **K**/**B** | a3: **observable** when `ReadSerial` is in a string expression – the rest of the expression is then built from rb1 on, two `ReadSerial` in one statement give "97b98" in the original, "97 98" in C (t64 therefore reads in separate statements, bb2 `compilerlauf/README.md` "Serielle Tests", not taken over); a2: convention |
| seriallib noser (seriallib.c:203) | "Serial Device Not Open": `TRAP #0` in normal code; if the trap returns, the code runs into the message text (saved a6 still on the stack) | runtime error, after which the routine returns without input/output | **K** | only with a returning trap (otherwise the program ends with the message) |
| speaklib (speaklib.c:30) | a2 popped | unchanged | **B** | convention |
| stringfunclib (stringfunclib.c:24, :634, :730, :815) | a2 decremented differently depending on the path (String$ ≤ 0, Replace$ empty, Cvi/Cvl/Cvq not at all) | always unchanged | **B** | convention |
| utillib CreateMsgPort (utillib.c:30, :153, :165, :193) | after the runtime error (`TRAP #0`, "Failed to allocate memory/signal for Port") the code runs into the message text; the NoSig branch first falls into `Error_NoMemory` (pops a0 and the registers from the stack a second time, second trap) | runtime error, then return with 0 | **K** | only with a returning trap; the original never reaches the NoMem branches anyway (memlib globalloc does not return on lack of memory), the NoSig branch never (`AllocSignal(-1)` does not return 0) |
| trackerlib (trackerlib.c:47) | – | interrupt routine in C, same registers | – | explicitly "no deviation" |
| vallib (vallib.c:18) | a2 decremented by 2/4 | unchanged | **B** | convention |
| windowslib err6/err7 (windowslib.c:73) | jumps into the error check area | trap in normal code | **K** | only in these error cases without debugger |
| windowslib registers (windowslib.c:73) | changes a3 (WBlit, inputstr), d7, a4 (Window); WindowTags with tag pairs writes the title length back to the argument stack | does not | **K**/**B** | not observable |
| windowslib WindowTags &TagList (windowslib.c:73, :2379) | parks the title length with `MOVE.l (a7)+,(a2)+` in the 4 bytes **behind the TAG_DONE pair of the user's tag list** and reads it back there | reads the length from the argument stack, writes nothing into the tag list | **K** | reproducible; **observable** when user data lies behind the tag list (e.g. an array element: original = title length, C = unchanged) |
| windowslib coordinates (windowslib.c:73) | unknown upper word (zcursor) | sign-extended | **T** | register leftover |
| shapeslib (shapeslib.c) | – | no deviation (err10 jumps into the error check area as in the original) | – | – |
| shapetrixlib XFlip (shapetrixlib.c, header) | no wait after the last blit before the CPU mirroring | reproduced likewise; result timing-dependent, C loop is faster | **T** | timing; in the test different for shapes 13 and 21 pixels wide (21 pixels only with runtime error checking); t141 checks XFlip only without shifting blit |
| spriteslib, collslib | – | no deviation | – | – |
| iffmakelib | – | no deviation (check functions in the check area since c4a736d, see section 6) | – | – |

## 4. deflibs (AmigaOS call declarations)

| Library/routine (file:line) | Original | C version | Kind | Reason / observable? |
|---|---|---|---|---|
| init of all libraries/resources (libs/deflibs/deflib.h:20) | `BEQ init+6` jumps into the middle of the `LEA`: `ORI.b #0,(a0)+` with arbitrary a0, then OpenLibrary with the a1 left by the failure | endless loop, each time with the correct name | **T** | a0/a1 after a failed OpenLibrary are undefined; observable only if the library is missing (both hang) |
| init/finit in general ([deflibs.md](deflibs.md)) | destroys a6, `JMP CloseLibrary` | GCC saves a6, `JSR` | **B** (register preservation) | not observable |
| Resources (deflib.h) | `LEA` points 6 bytes into the name | reproduced (`libname + DEFLIB_RESOURCE_SKEW`) | – | BUG(orig), no deviation |

## 5. Compiler blitz2-c (`programs/blitz2`)

Produces byte-identical programs (test corpus and CD reference run), hence only
internal deviations. Conventions: `-fbaserel` (globals a4-relative,
`GLUE_LOAD_A4` in all entry points from outside, [README](../blitz2/bb2-readme.md)), assembler only in
`programs/blitz2/src/glue*.c`, registers with persistent state as globals
(`srcptr`, `numd3`/`numd6`, `getvnamelen`).

| Place (file:line) | Original | C version | Kind | observable? |
|---|---|---|---|---|
| errstack/errcont (asmlibs.c:1427, compile.c:184, data.c:1460, data.c:1830, main.c:1354, parse.c:216, parse.c:305, stmts.c:1189, stmts.c:1634) | error return is not reset | `errjmp = olderr` | **T** | stack trick (a7/errcont) only with longjmp in C; no |
| Stack tricks `addq #4,a7` / `move.l passstack,a7` (asmlibs.c:403, :467, asmlibs.h:69, compile.c:1108, tokens1.c:783, tokens1.c:855) | return past the caller | return value / longjmp | **T** | no |
| Placeholder longs (compile.c:829, stmts.c:432, tokens1.c:256, :509, :635, :1050, tokens2.c:10) | poke register leftovers as placeholders | 0 or the recognisable value | **T** | are always overwritten (except nomemleft); no |
| Register results (eval.h:236, eval.c:126, libcall.h:19, compile.c:1385, tokens1.c:343, tokens2.c:81, parse.c:1157) | results in d2/d3/d6/CCR, values from the caller's stack | globals / parameters / return values | **T** | no |
| Constant folding (eval.h:171, eval.c:823, libcall.c:1182) | executes generated code with the compiler registers | `callregs` with the relevant registers | **T** | no |
| Range/overflow checks (eval.c:485, data.c:1693, parse.c:351, compile.c:198) | without check (reading/writing beyond tables, DIVU by 0) | C checks and does nothing | **K** | only in cases that according to the comment do not occur |
| pushprec (eval.c:929) | stores operator/precedence without bounds check; "Expression too complex" only in `popprec`, after writing past the end of the stack | checks before writing and reports `pserr` immediately | **K** | same message; the only difference is that the original first writes beyond `precstackf` (only with very deeply nested expressions) |
| savefile HUNK_SYMBOL (parse.c:269) | writes the name length rounded up to longwords directly from the label node: **reads** up to 3 bytes behind the allocation and writes them as padding bytes into the executable | copy padded with zeros (padding bytes 0) | **K** | only with "Create Debug Info": the padding bytes in HUNK_SYMBOL (original random, C always 0); otherwise no |
| libcall.c:256, libcall.c:1438 | makeint/comparison with leftovers in the upper byte | FALSE / defined comparison | **T** | no |
| parse.c:538, parse.h:75, compile.h:32 | err without a set return | message + `exit(RETURN_FAIL)` | **T** | only with internal errors |
| parse.c:797 | `findlast` (dead code) | walks the list | **T** | no |
| stmts.c:1664 | flushc after the register restore | before it | **T** | no |
| Start (main.c:31, :1072, :1096, :1134, :1141) | own Workbench/CLI start, opens/closes dos.library | libnix start (DOSBase, WBStartup, argv) | **T** | command line is split by libnix (quotes) |
| main.c:1529 | SuperState/movec/UserState | `Supervisor()` | **T** | no |

The individual reasons are each given at the comment `DIFF:` in
`programs/blitz2/src/`.

## 5a. Editor ted-c (TED 1.1, `programs/ted`)

The reference is the vasm build of `ted.s` ([`../ted/reference.md`](../ted/reference.md)).
Separate list of the TED deviations: [`../ted/deviations.md`](../ted/deviations.md)
(continuously extended there); here the rows that concern the shared
file requester.

| Place (file:line) | Original | C version | Kind | observable? |
|---|---|---|---|---|
| `flink` (ted.s:7953; programs/ted/src/filereq.c) | the embedded requester hunk `filereq` relocates itself at runtime on start (from `outside`) (`add.l basis` on 312 longwords in its own code hunk, without cache flush; a second run would relocate twice – BUG(orig)) | dropped: the requester is C code (`shared/filereq/filereq_core.c`), its pointers are relocated by LoadSeg | **T** | technically unavoidable: no embedded hunk any more, nothing to relocate. No (except the double relocation, which never happens in the original) |
| Requester data block (programs/ted/src/filereq.c with `shared/filereq/filereq_core.c`) | TED's copy of `filereq` differs from the one in freqlib in 7 bytes (padding bytes behind gadget texts, e.g. `"OK",0,$06` instead of `$00`, and remainder at the hunk end) | data block of the freqlib version | **T** | one source for both programs (user decision "Variante C"); the bytes are never read. No |

## 6. Error checks outside the error check area

`BLITZ_ERRCHK_THUNK`/`BLITZ_ERRCHK_MESSAGE` switch back in the inline assembler
with `.text`; GCC does not notice this. If a `BLITZ_ERRCHK`
C function follows such a thunk, GCC emits no new
`.section` directive and the function ends up in normal code
(the assembler of this toolchain does not know `.previous`/`.pushsection`).
Behaviour the same (the thunks are in the check area and call them), but without
debugger the code is not cut off. Remedy: all thunks behind all
check functions (as in shapeslib, shapetrixlib, spriteslib, collslib).
Checked by compiling all `libs/acidlibs/*.c` with `-S` and evaluating the
effective section of each `BLITZ_ERRCHK` function (as of c4a736d):
**no** check function is in normal code. (iffmakelib had four –
`bitchk_c`, `bitchk2_c`, `sschk1_c`, `sschk2_c` –, fixed in c4a736d.)

## 7. State

All command libraries are ported (c4a736d); the tables also contain
scrolllib, 2dlib, polylib (S12) and gameiolib, blitzkeyslib, animlib,
blitzcoplib, displaylib (S13).

## Summary

* **B – user decision:** the a2 convention (about 20 libraries,
  never observable) and register preservation "at least like the original"
  (additionally preserved registers are never results).
* **T – technically unavoidable:** undefined register leftovers and
  OS returns (blitlib BlitColl, freqlib, cliargslib CCR, windowslib
  zcursor, ilbmifflib a0, deflibs retry), memory layout with overflows and
  invalid indices (2dlib, polylib, displaylib, crunchlib), stack contents
  with error jumps via JSR (animlib, displaylib, screenslib) and other stack questions
  (crunchlib, arrayslib a4/a5), timing (shapetrixlib XFlip) and in the
  compiler all stack tricks and placeholders. Observable at most in
  error cases, with overflows or corrupted data.
* **K – silent corrections (existing):** traps in normal code instead of a jump
  into the cut-off error check area (inputoutputlib, fileiolib,
  palettelib, windowslib, brexxlib, wbstartuplib), return instead of crash
  or continuing error chain or running into the message text (arexxlib,
  memlib err1/locfree, runerrlib, fontlib, palettelib, seriallib noser,
  utillib CreateMsgPort), preserved a3 (linklistlib, medlib, seriallib,
  windowslib, blitzoslib), the title length not written into the tag list
  (windowslib WindowTags &TagList) and word coordinates
  (blitzoslib), in the compiler some range checks (pushprec, savefile).
  Observable mostly only in error cases without debugger or with a
  returning trap; visible in a normal run are seriallib
  ReadSerial in string expressions and WindowTags &TagList (memory behind
  the tag list).
