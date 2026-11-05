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
Since the port is complete, original bugs may be fixed deliberately
(CLAUDE.md "Rules", decision 2026-10-30): such fixes are listed here as
intended deviations (kind **F**), each with its issue.

## Categories

| Code | Kind | Meaning |
|---|---|---|
| **B** | user decision | project convention, fixed for all ports |
| **T** | technically unavoidable | the original is undefined here (register leftovers, stack contents, timing, memory layout) or cannot be expressed in C |
| **K** | silent correction | the original could be reproduced, the C version behaves "more safely" (trap instead of jump into nowhere, return instead of crash, register preserved). No longer permitted under the new rule; existing cases remain, but are listed here |
| **F** | fixed original bug | intended deviation after the port (CLAUDE.md "Rules": once the port is complete, original bugs may be fixed deliberately, one issue per bug, in the default variant); the issue is named, and its bug-orig test now comes out FIXED where one is possible |

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
| memlib err1 (memlib.c:89) | jump out of localloc/newmem with saved registers on the stack, never returns | err1 returns, caller returns 0 | **K** | only if the runtime error trap returns (normally the program ends); practically not observable |
| memlib locfree err2 (memlib.c:196) | unknown block: one-time `TRAP #0` "Unable to Free Memory" (err2 in normal code); if the trap returns, the code continues in `newmem`, the registers saved by locfree are still on the stack | locfree returns after the runtime error | **K** | only with a returning trap; practically not observable |
| memlib messages (memlib.c:24) | err1/err2 trap in normal code, their messages "Out of Memory!"/"Unable to Free Memory" are in the error check area (without debugger d0 behind the lib) | messages in normal code; the error check area is empty | **F** | #956: the same runtime errors with the debugger; without it TRAP #0 with the right message (test `memlib-messages-errarea`) |
| arrayslib newlocarr/newloclist (arrayslib.c:29) | swaps a4/a5 around the memlib call | passes the base directly | **T** | a4/a5 are fixed registers in C; memlib does not use a5. Not observable |
| arrayslib newlist/newloclist (arrayslib.c:176) | links the first item before any check (do-while): a list of size 0 (e.g. `Dim List a(-1)`) gets its 8 bytes (next, last) written behind the 32 byte block | links an item only if its node fits; for the compiler's sizes (multiples of element size + 8) the same items | **F** | #59; observable only with size 0: no write behind the block (test `arrayslib-newlist-overrun`). AmiBlitz3 instead made size 0 a dynamic list (other list layout, not BB2-compatible) |
| arrayslib dimension calculation (arrayslib.c:90) | returns d1.w+1, changes d2 | d2 preserved, d1 undefined | **B** (register preservation) | compiler does not use d1/d2. Not observable |
| maxlenlib (maxlenlib.c:19) | changes d2 and a2 | preserves d2/a2 | **B** | not results. Not observable |
| maxslib doallocst (maxslib.c:33) | d1 = -1 after copying | d1 preserved | **B** | not a result |
| maxslib dofreest (maxslib.c:55) | d0 = memlib's d0 | d0 preserved | **B** | not a result |
| modlib domodquick (modlib.c:29) | d1 returned swapped | d1 preserved | **B** | not a result |
| VBR on the 68010: runerrlib codeinit (runerrlib.c:537), errtraplib codeinit (errtraplib.c:130), environlib codeinit (environlib.c:105); the compiler's calcvbr in section 5 | the VBR is read (MOVEC) only if the highest AttnFlags CPU bit is 68020 or above: a 68010 gets VBR 0 (errtraplib puts its TRAP #0 handler, runerrlib's getvbr gives debuglib/rundebuglib, environlib's cr_vbr the table at 0, not the active one) | read from the 68010 on | **F** | #42: differs only on a 68010 with a moved vector table; test `multi-68010-vbr` (a500-68010: cr_vbr and a SetErr handler) |
| runerrlib check chains (runerrlib.c:238) | continues through the error chain after a returning trap | C check returns | **K** | only with a returning trap (the debugger terminates). Practically not observable |
| datelib Date$ (datelib.c:268) | day 366 of a 4-year cycle (1 January after a leap year, e.g. `Date$(1096)`) stays in the leap year as "32/12" (`CMP #366`: `BLE`) | `BLT`: the 1 January of the next year | **F** | #120; observable: Date$, Days, Months, Years of these days (test `datelib-day366`). AmiBlitz3 rewrote the conversion as a year loop; here only the one compare, the date range and everything else stay |
| fontlib messages (fontlib.c:47) | err1-err3 (Can't Open Font, Uninitialized BitMap/Font) trap in normal code, their messages are in the error check area (without debugger d0 behind the lib) | messages in normal code | **F** | #955: the same runtime errors with the debugger; without it TRAP #0 with the right message (test `fontlib-messages-errarea`) |
| fontlib bitchar (fontlib.c:377) | characters below tf_LoChar dropped with SUB.b/BMI (signed): also those ≥ LoChar+128 (topaz: every code from 160, the umlauts); with LoChar > 128 codes more than 128 below it drawn as c-LoChar+256 | unsigned compare: only characters below LoChar dropped | **F** | #148 (as AmiBlitz3 `99f38fed`, BMI → BCS); observable: such characters are drawn and move the cursor (test `fontlib-lochar-127`) |
| fontlib calcoff (fontlib.c:52) | after a trap (no bitmap/font) it runs into the next trap | draws nothing; unreferenced check `bitmapchk` missing | **K** | only with a returning trap |
| inputoutputlib defin single character (inputoutputlib.c:45, :233) | `BRA err1` into the error check area (without debugger: jump behind the lib) | trap and message in normal code | **K** | without debugger: runtime error instead of jump into nowhere (observable only in this error case). The jump would be technically possible (cf. audiolib, screenslib, shapeslib, whose error routines are in the normal code now, kind F) |
| wbstartuplib AddArg (wbstartuplib.c:78) | without memory, jump to `err` in the error check area, its message there too | runtime error with its message in normal code, NULL, caller aborts | **F** | #305 (the trap moved first as a silent correction, now also its message); only without memory at the start of the program, before any program code: no test (no handler of the program can see d0) |
| linklistlib SortList (linklistlib.c:35) | destroys a3 and d6 | a3 unchanged | **K** | statement, a3 is not read afterwards. Not observable |

## 3. Command libraries

| Library/routine (file:line) | Original | C version | Kind | Reason / observable? |
|---|---|---|---|---|
| 2dlib overflows (2dlib.c:47) | Polyf with more than 16 points / BitMapPoly edge lists overwrite the code behind polypts/l1 | overwrite the data behind them | **T** | memory layout; observable only with these overflows (BUG(orig), #395). FloodFill (yofill) is the original assembler, a3 as in the original |
| polylib overflows (polylib.c:40) | more than 16 points / 10 edges per side hit the lib's own code | hit the data behind them | **T** | memory layout; only with overflows |
| animlib err6/err7 (animlib.c:46) | Can't Open Anim File: `BEQ err7` into the error check area; err6 (Anim format not supported) traps in normal code with its message in the error check area and runs on into err1 if the trap returns | traps and messages in normal code; LoadAnim returns if the trap returns | **F** | #398: the same runtime errors with the debugger; without it TRAP #0 with the right message (test `animlib-errors-check-area`) |
| animlib a2 (animlib.c:53) | a2 decremented | unchanged | **B** | convention |
| arexxlib ReplyRexxMsg (arexxlib.c:62, :505) | with a non-RexxMsg: pops 15 never-saved registers (crash) | returns | **K** | observable only in this error case (crash vs. nothing) |
| arexxlib CreateRexxMsg (arexxlib.c:62) | copies -8(a2) to -4(a2) on the compiler stack | does not copy, a2 unchanged | **B** | convention; the compiler does not read the value |
| asllib title (asllib.c:44) | writes maxslib's d1 (-1) into the length cell of the title on the compiler stack | stack and a2 stay | **B** | convention; not observable |
| audiolib (audiolib.c:47) | a2 decremented (volumes/lengths) | unchanged | **B** | convention |
| audiolib err5 and messages (audiolib.c, header) | SetPeriod without a sound: `BEQ err5` into the error check area; the messages of err1-err4 (normal code) there too: without debugger a jump behind the library, d0 behind the library | err5 and the messages `mess1`-`mess5` in the normal code: a TRAP #0 with the right message | **F** | #70; observable without debugger (SetPeriod: trap instead of a jump into nowhere; messages: only by a handler that reads d0); test `bug_orig_audiolib_setperiod_err5` |
| bblitlib (bblitlib.c:25) | entries without nullsub save a4/a5 | likewise (note: no deviation in behaviour) | – | – |
| blitlib BlitColl Amiga (blitlib.c:323) | upper word of d0 = leftover from DisownBlitter | 0 | **T** | register leftover of the OS (undefined); #word function, not observable |
| blitzcoplib CustomCop (blitzcoplib.c:56) | a2 decremented by 4 | unchanged | **B** | convention |
| blitzkeyslib | – | no deviation (DIFF-free; binary has 6 more unused bytes) | – | – |
| dhandlerlib | – | no deviation (byte-identical to the CD's library, relocation table included) | – | – |
| displaylib a2 (displaylib.c:64) | a2 decremented as argument stack | unchanged | **B** | convention |
| displaylib error jumps (displaylib.c:64) | all errors: Bcc from normal code into the error check area | via JSR (one more return address on the stack) | **T** | GCC drops code after an inline BRA or merges the paths; same target, only stack contents at the trap |
| displaylib error routines (displaylib.c:34, #412) | the ten error routines (`MOVE.l #message,d0:TRAP #0`) and their messages lie in the error check area; without the debugger every error (bitplanes for the resolution, uninitialised CopList/palette/sprite, sprite channels, custom copper space, CustomString length, AGA palette mode, CustomChunky ypos, custom cops overrun) branches behind the library | routines and messages in the normal code: with the debugger the same runtime error, without it a TRAP #0 with the message (no handler: a clean crash); the !subs check `_checkcoplist` stays in the error check area | **F** | fixed (#412); observable only in these error cases without the debugger. Tests: bug-orig `displaylib-errors-errarea` (FIXED), with the debugger corpus t167, t169 |
| displaylib table indices (displaylib.c:64) | sprite mode > 5, mode table beyond resolution 2 / fetch 3, doublescan > 6 read the lib's own code | read other data | **T** | memory layout; only with invalid values |
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
| gtlib GTPalette colour (gtlib.c:1049) | the colour (a2-18) goes to +38 of the tag list (layout of the source with two more tags), the low word of the TAG_MORE pointer: colour ignored, a later GTTags list reached through a wrong pointer | colour into GTPA_Color; the form without a colour sets GTPA_Color 0 (its value in the original) | **F** | #168; observable: GTPA_Color, GTTags after a GTPalette with colour (test `gtlib-gtpalette-tagmore`) |
| gtlib optional values (gtlib.c:56) | the forms with an optional value (GTCycle/GTMX active, GTListView selected/top, GTScroller Top, GTSlider Level, GTString default$) write it into the static tag list, the forms without it leave it there: a later gadget gets the previous value (GTString a pointer into the old string work area) | the forms without the value set the initial value of the list (0, GTLV_Selected -1, GTST_String NULL) | **F** | #169; observable: the start values of such gadgets (test `gtlib-optional-values-stay`: cycle, mx, string) |
| gtlib GTShape images (gtlib.c:1276) | the static image1 is copied over the GadgetRender of the GadTools button (or set if there is none), every GTShape gadget gets the static image2 as SelectRender: GadTools' render object overwritten, the last GTShape's images apply to all | two images of its own per gadget (memlib block with GTList and gadget); GadTools' GadgetRender/SelectRender are kept and put back before FreeGadgets (GTFreeGadget, Free GTList) | **F** | #173 (AmiBlitz3 `ea8c25c7`: own images per gadget, GENERIC_KIND; here the button stays); observable: the select image of an earlier GTShape (test `gtlib-gtshape-image`) |
| gtlib GTShape CreateGadget (gtlib.c:1227) | the result of CreateGadget is not checked: if it fails, GadgetRender is read from address 18 (20 bytes copied to where it points, or the image pointer written there) and SelectRender/Flags/Activation/GadgetType go to 12-25 (exception vectors) | a failed gadget is left alone (the GTList's last gadget is 0, as with the other creators) | **F** | #327; observable when CreateGadget fails, e.g. without a used screen (test `gtlib-gtshape-creategadget-null`) |
| gtlib getoptions (gtlib.c:58) | GTCycle/GTMX labels: block of option length + 258 bytes, pointers at +0, texts at +256 (63 labels + NULL, not checked: more overwrite the texts, many short ones also memory behind the block); never freed | block sized by the counted labels (pointers, then texts), remembered in a list of the library with GTList and gadget, freed by GTFreeGadget and Free GTList | **F** | #174 (as AmiBlitz3 `ea8c25c7`, McMinn's rewrite: counted and freed; the GTList object stays 16 bytes); observable: more than 63 labels, memory (test `gtlib-getoptions`) |
| iffiolib iffopen (iffiolib.c:34) | a0 lost after dos Open | a0 preserved | **B** (register preservation) | **not marked as DIFF**, described in the header; not observable |
| ilbmifflib ilbminit (ilbmifflib.c:46) | a0 after failure = leftover from dos.library | a0 = file name | **T** | register leftover; described in the header |
| ilbmifflib messages (ilbmifflib.c, header) | error routines in the normal code, their messages `mess2`-`mess4` in the error check area: without debugger d0 points behind the library | messages in the normal code | **F** | #192; observable without debugger only by a handler that reads d0 (SetErr, own trap handler); test `bug_orig_ilbmifflib_messages_errarea` |
| intuifontlib (intuifontlib.c:28) | a2 decremented | unchanged | **B** | convention |
| maxlenlib/maxslib/modlib | see system libs | | | |
| medlib (medlib.c:59) | LoadMedModule/SetMedVolume leave a3 on object+8 or track | a3 unchanged | **K** | statements; not observable |
| palettelib make4bitrgbs (palettelib.c:38, :420) | the 4 bit colours for LoadRGB4 (agamode 0: Kickstart < 3.0, ShowPalette and cycling in Amiga mode) from the low byte of each component (CD binary): DecodePalette colours (top byte only) black, FadePalette/MixPalette results wrong | from the top byte (the same for colours with the byte repeated: PalRGB, AGAPalRGB, LoadPalette) | **F** | #224; observable under Kickstart < 3.0 (test `palettelib-make4bitrgbs-lowbyte`, exec version set to 37) |
| palettelib errors (palettelib.c:55, :288) | AGARed/Green/Blue `err3` and messages err0/err1/errd in the error check area | trap and messages in normal code | **K** | only in error cases without debugger |
| palettelib empty palette (palettelib.c:52, :726, :732) | ShowPalette/Use Palette Amiga mode with an empty palette runs into the Blitz code `b_showpal`, in both branches: from ExecVersion 39 on (`agamode`, a0 arbitrary) and below via `make4bitrgbs` (a0 = palette object); writes nothing | returns in both branches | **K** | result the same (nothing written); only with `agamode` and odd a0 on the 68000 an address error in the original |
| printlib (printlib.c:23) | a2 decremented | unchanged | **B** | convention |
| screenslib (screenslib.c:59) | a2 popped | unchanged | **B** | convention |
| screenslib SA_DisplayID (screenslib.c:49, :502) | long form of Screen: `MOVE d1,newmodes+2` sets only the low word of the SA_DisplayID value, the upper word stays 0: display IDs with a monitor part (PAL, NTSC, DblPAL, ...) open on the default monitor | the whole viewmode long | **F** | #250; observable with a monitor part in the viewmode (test `screenslib-displayid-lowword`: $29000 gives $9000 in the original) |
| screenslib err1/err8/err12/err13 (screenslib.c:51) | Can't open Screen: `BEQ err1` from normal code into the error check area; err8/err12/err13 trap in normal code with their messages in the error check area (without debugger a jump behind the lib, d0 behind the lib) | traps and messages in normal code | **F** | #251: the same runtime errors with the debugger; without it TRAP #0 with the right message (test `screenslib-err1-errorarea`) |
| scrolllib | – | no deviation (DIFF-free; clsnotint after the binary) | – | – |
| seriallib ReadSerial (seriallib.c:53, :424) | a3 stays on the 1-byte buffer rb1 in the lib; a2 = unit slot | a3/a2 unchanged | **K**/**B** | a3: **observable** when `ReadSerial` is in a string expression – the rest of the expression is then built from rb1 on, two `ReadSerial` in one statement give "97b98" in the original, "97 98" in C (t64 therefore reads in separate statements, bb2 `compilerlauf/README.md` "Serielle Tests", not taken over); a2: convention |
| seriallib noser (seriallib.c:203) | "Serial Device Not Open": `TRAP #0` in normal code; if the trap returns, the code runs into the message text (saved a6 still on the stack) | runtime error, after which the routine returns without input/output | **K** | only with a returning trap (otherwise the program ends with the message) |
| speaklib (speaklib.c:30) | a2 popped | unchanged | **B** | convention |
| stringfunclib (stringfunclib.c:24, :634, :730, :815) | a2 decremented differently depending on the path (String$ ≤ 0, Replace$ empty, Cvi/Cvl/Cvq not at all) | always unchanged | **B** | convention |
| utillib CreateMsgPort (utillib.c:30, :153, :165, :193) | after the runtime error (`TRAP #0`, "Failed to allocate memory/signal for Port") the code runs into the message text; the NoSig branch first falls into `Error_NoMemory` (pops a0 and the registers from the stack a second time, second trap) | runtime error, then return with 0 | **K** | only with a returning trap; the original never reaches the NoMem branches anyway (memlib globalloc does not return on lack of memory), the NoSig branch never (`AllocSignal(-1)` does not return 0) |
| trackerlib (trackerlib.c:51) | – | interrupt routine in C, same registers | – | explicitly "no deviation" |
| trackerlib LoadModule err1 (trackerlib.c:47) | err1 traps in normal code, its message "Error Accessing Module File" is in the error check area (without debugger d0 behind the lib) | message in normal code | **F** | #957: the same runtime error with the debugger; without it TRAP #0 with the right message (test `trackerlib-err1-errarea`) |
| vallib (vallib.c:18) | a2 decremented by 2/4 | unchanged | **B** | convention |
| windowslib WindowTags &TagList copy (windowslib.c:73, dowindowtags3) | copies the program's list up to TAG_DONE into usertags (32 longs = 16 pairs) without a limit: a longer list overwrites bmapbase and the code of WindowFont behind it | copies at most 15 pairs; if the list goes on, the 16th entry is TAG_MORE to the rest | **F** | #317 (AmiBlitz3 links the whole list with TAG_MORE; short lists are still copied, so #316 behaves as before); observable with more than 15 pairs (test `windowslib-taglist-ueberlauf`: the original runs into the tag data in WindowFont). The form with tag pairs on the argument stack keeps the overflow (BUG(orig), #317) |
| windowslib titles (windowslib.c:80, dowtitle, freewindow) | Window, WindowTags and WTitle copy the titles (maxslib allocst) and never free the copies: every open or new title costs memory until the program ends | the object keeps its copies (+20 window title, +24 screen title of WTitle; bytes the original's 32-byte object does not use); WTitle frees the old ones after SetWindowTitles, CloseWindow and a failed open free them | **F** | #318 (AmiBlitz3 only frees the old window title in WTitle, read back from the Window); observable as free memory (test `windowslib-titel-leck`); a program that peeks the object at +20/+24 sees the pointers |
| windowslib WMove/WSize (windowslib.c:87, waitwin) | after MoveWindow/SizeWindow a busy loop until the window has exactly the new position/size: endless if Intuition limits the values (the size limits of the window; without runtime error checking also the screen edge) | waits until the window has the new values or other ones than before, at most 50 frames (WaitTOF) | **F** | #320 (AmiBlitz3 drops the wait; kept here so WindowX etc. are up to date after the command); observable with limited values (test `windowslib-wmove-wsize-endlos`: the original hangs) |
| windowslib CloseWindow (windowslib.c:94, freewindow) | replies every message at the port all windows share and frees the whole event list: the events of the other windows are lost | replies and frees only the messages and events of the closed window | **F** | #322 (as AmiBlitz3); observable with events of other windows not fetched yet (test `windowslib-closewindow-ereignisse`) |
| windowslib err6/err7 (windowslib.c:98) | jumps into the error check area | trap in normal code | **K** | only in these error cases without debugger |
| windowslib registers (windowslib.c:98) | changes a3 (WBlit, inputstr), d7, a4 (Window); WindowTags with tag pairs writes the title length back to the argument stack | does not | **K**/**B** | not observable |
| windowslib WindowTags &TagList (windowslib.c:98, :3218) | parks the title length with `MOVE.l (a7)+,(a2)+` in the 4 bytes **behind the TAG_DONE pair of the user's tag list** and reads it back there | reads the length from the argument stack, writes nothing into the tag list | **K** | reproducible; **observable** when user data lies behind the tag list (e.g. an array element: original = title length, C = unchanged) |
| windowslib coordinates (windowslib.c:98) | unknown upper word (zcursor) | sign-extended | **T** | register leftover |
| shapeslib DecodeShapes err10 (shapeslib.c, header) | address 0: `BEQ err10` into the error check area: without debugger a jump behind the library | err10 and its message in the normal code: a TRAP #0 with the right message | **F** | #270; observable without debugger (trap instead of a jump into nowhere); test `bug_orig_shapeslib_decodeshapes_null` |
| shapeslib messages (shapeslib.c, header) | error routines err2/err3/err7/err8 in the normal code, their messages in the error check area: without debugger d0 points behind the library | messages in the normal code | **F** | #953; observable without debugger only by a handler that reads d0 (SetErr, own trap handler); test `bug_orig_shapeslib_messages_errarea` |
| shapetrixlib XFlip (shapetrixlib.c, header) | no wait after the last blit before the CPU mirroring | reproduced likewise; result timing-dependent, C loop is faster | **T** | timing; in the test different for shapes 13 and 21 pixels wide (21 pixels only with runtime error checking); t141 checks XFlip only without shifting blit |
| spriteslib messages (spriteslib.c, header) | error routines err10/err11 (SaveSprites, LoadSprites) in the normal code, their messages in the error check area: without debugger d0 points behind the library | messages in the normal code | **F** | #954; observable without debugger only by a handler that reads d0 (SetErr, own trap handler); test `bug_orig_spriteslib_messages_errarea` |
| collslib | – | no deviation | – | – |
| iffmakelib | – | no deviation (check functions in the check area since c4a736d, see section 6) | – | – |
| iffmakelib err2 (iffmakelib.c:49) | "Can't open File" (err2) traps in normal code, its message is in the error check area (without debugger d0 behind the lib) | message in normal code | **F** | #185: the same runtime error with the debugger; without it TRAP #0 with the right message (test `iffmakelib-openerr-errarea`) |

## 4. deflibs (AmigaOS call declarations)

| Library/routine (file:line) | Original | C version | Kind | Reason / observable? |
|---|---|---|---|---|
| init of all libraries/resources (libs/deflibs/deflib.h:16) | `BEQ init+6` jumps into the middle of the `LEA`: `ORI.b #0,(a0)+` with arbitrary a0, then OpenLibrary with the a1 left by the failure, forever | the base or 0 (no retry); the finit of a library closes it only if the base is not 0 | **F** | #44: a program using a call of a missing library no longer hangs at startup; it runs with base 0, a call of that library crashes (test `deflibs-init-endless-retry`, Kickstart 1.3 without gadtools.library) |
| init/finit in general ([deflibs.md](deflibs.md)) | destroys a6, `JMP CloseLibrary` | GCC saves a6, `JSR` | **B** (register preservation) | not observable |
| Resources (deflib.h, e.g. battclocklib.c:42) | `LEA` points 6 bytes into the name: `OpenResource` always fails, the init hangs (#44) | opens the name itself | **F** | #45: a program using a call of ciaa, ciab, disk, misc, potgo, battclock, battmem or card no longer hangs at startup (test `deflibs-resource-name-skew`) |
| exec `CachePreDMA_`, `CachePostDMA_` (execlib.c:141) | `flags` declared in d1, exec reads d0 | d0 (`exec_lib.fd`); a declaration difference, listed in `libs/deflibs/declarations-intended.txt` | **F** | #959: exec gets the given flags; before, only by chance at the statement level (the flags evaluated last into d0), inside an expression d0 held another value (test `deflibs-cachedma-flags-d1`) |

## 5. Compiler blitz2-c (`programs/blitz2`)

Produces byte-identical programs (test corpus and CD reference run), hence only
internal deviations; except the fixed original bugs (kind **F**), which
change the generated code of the programs they concern. Conventions: `-fbaserel` (globals a4-relative,
`GLUE_LOAD_A4` in all entry points from outside, [README](../blitz2/bb2-readme.md)), assembler only in
`programs/blitz2/src/glue*.c`, registers with persistent state as globals
(`srcptr`; `numval`/`numfloat` local to libcall.c).

| Place (file:line) | Original | C version | Kind | observable? |
|---|---|---|---|---|
| errstack/errcont (asmlibs.c:1427, compile.c:184, data.c:1460, data.c:1830, main.c:1354, parse.c:216, parse.c:305, stmts.c:1189, stmts.c:1634) | error return is not reset | `errjmp = olderr` | **T** | stack trick (a7/errcont) only with longjmp in C; no |
| Stack tricks `addq #4,a7` / `move.l passstack,a7` (asmlibs.c:403, :467, asmlibs.h:69, compile.c:1108, tokens1.c:840, tokens1.c:953) | return past the caller | return value / longjmp | **T** | no |
| Placeholder longs (compile.c:829, stmts.c:432, tokens1.c:256, :545, :673, :1158, tokens2.c:10) | poke register leftovers as placeholders | 0 or the recognisable value | **T** | are always overwritten (except nomemleft); no |
| Register results (eval.h:236, eval.c:126, libcall.h:19, compile.c:1385, tokens1.c:343, tokens2.c:81, parse.c:1163) | results in d2/d3/d6/CCR, values from the caller's stack | globals / parameters / return values | **T** | no |
| Constant folding (eval.h:171, eval.c:824, libcall.c:1182) | executes generated code with the compiler registers | `callregs` with the relevant registers | **T** | no |
| Range/overflow checks (eval.c:486, data.c:1693, parse.c:351, compile.c:198) | without check (reading/writing beyond tables, DIVU by 0) | C checks and does nothing | **K** | only in cases that according to the comment do not occur |
| pushprec (eval.c:930) | stores operator/precedence without bounds check; "Expression too complex" only in `popprec`, after writing past the end of the stack | checks before writing and reports `pserr` immediately | **K** | same message; the only difference is that the original first writes beyond `precstackf` (only with very deeply nested expressions) |
| savefile HUNK_SYMBOL (parse.c:269) | writes the name length rounded up to longwords directly from the label node: **reads** up to 3 bytes behind the allocation and writes them as padding bytes into the executable | copy padded with zeros (padding bytes 0) | **K** | only with "Create Debug Info": the padding bytes in HUNK_SYMBOL (original random, C always 0); otherwise no |
| libcall.c:256, libcall.c:1438 | makeint/comparison with leftovers in the upper byte | FALSE / defined comparison | **T** | no |
| parse.c:544, parse.h:75, compile.h:32 | err without a set return | message + `exit(RETURN_FAIL)` | **T** | only with internal errors |
| parse.c:825 | `findlast` (dead code) | walks the list | **T** | no |
| stmts.c:1664 | flushc after the register restore | before it | **T** | no |
| Start (main.c:31, :1072, :1096, :1134, :1141) | own Workbench/CLI start, opens/closes dos.library | libnix start (DOSBase, WBStartup, argv) | **T** | command line is split by libnix (quotes) |
| main.c:1529 | SuperState/movec/UserState | `Supervisor()` | **T** | no |
| calcvbr (main.c:1538) | the VBR is read only from the 68020 on: on a 68010 the trap vectors of a program run from the editor (directtrap, debugger) go into the table at 0 | read from the 68010 on (as the libraries, section 2) | **F** | #42: only on a 68010 with a moved vector table; not covered by a test (the test `multi-68010-vbr` runs an executable) |
| savefile HUNK_RELOC32 (parse.c:243) | all relocations of the code hunk in one group (count, hunk 0, offsets): with more than 65535 the LoadSeg of AmigaOS V36+ refuses the executable ("file is not executable"; Kickstart 1.3 loads it) | groups of at most 65535 offsets in the block | **F** | #943: the executable differs only with more than 65535 relocations (8 bytes more per further group); the resident writer (stmts.c) is unchanged (residents are loaded by blitz2 itself). Test `compiler-reloc32-one-block` |
| stamigalib, amigalib (stmts.c:612, eval.c:1893) | Amiga library call with a parameter in a4 or a5 (graphics LockLayerRom_, UnlockLayerRom_, AttemptLockLayerRom_, workbench AddAppIconA_; exec Supervisor_ cannot be compiled at all, its token is doslib's finit, #960): the parameters are popped into their registers before `move.l off(a5),a6`, a4/a5 not saved - with a5 a6 is garbage (crash at the jsr), after the call a4/a5 hold the parameters instead of the variable bases | `movem.l a4/a5,-(a7)` before the parameters, the library base before the pops, `movem.l (a7)+,a4/a5` after the jsr | **F** | #958: only calls with an a4/a5 parameter get other code (8 bytes more); test `compiler-amigalib-a4a5` |
| doincbin (tokens1.c:776) | IncBin of a directory: "Can't open Include" with the lock left behind; a read error: "Error Reading File" with the file left open; Examine unchecked (an old content of macrobuff counts as the FileInfoBlock) | the lock is released and the file closed before the error; a failed Examine is "Can't open Include" | **F** | #38: the same errors, nothing left in use (tests `compiler-incbin-leaks`, `compiler-incbin-leaks-readerr`) |
| fixconsts (stmts.c:1039) | the next of the last constant of a resident is left as loaded: in a resident whose last constant is followed by other labels (Create Resident, `resconsts`; AmiBlitz3's `all.res`) it points to the end of the hunk, and every compile's `freelabels` (from `lastrescon`) frees what lies behind the hunk (alerts 8100 0005 / 0100 000F, depending on that memory) | next of the last constant 0, as AmiBlitz3's fixconsts (`CLR.l (a2)`, `AmiBlitz/AmiBlitz3@ea8c25c7`) | **F** | #34: no FreeMem behind the resident; residents written by Create Resident are unchanged (test `compiler-resconsts-last-next`) |
| blitz2's exit, fail2b (main.c:527) | `macrobuff`, allocated at the start with the Macro Buffer size of then (8192), is freed with the current `macrolen`: an .xtra loaded later (Macro Buffer of the Compiler Options) changes it - a smaller size leaves the rest lost, a larger one (AmiBlitz3's default 40000) corrupts exec's free list (alert 8100 0005 at the exit) | freed with the size of the allocation (`macrobuffsize`) | **F** | #336: only blitz2/blitz2-ab3 change, no generated code; AmiBlitz3 (`AmiBlitz/AmiBlitz3@c7fc6f60`) still frees with the current size (tests `compiler-macrobuff-freemem`, `compiler-macrobuff-freemem-large`, `ab3_cli` case `macrobuf_xtra`) |
| makemacbuff (stmts.c:779), called by loadxtra and optsdone | the Macro Buffer option has no effect: `macrobuff` keeps its 8192 bytes of the start, a macro's text is limited to 8192 bytes whatever the option says | where the other option-sized buffers follow the options (`makeiallox` in loadxtra and optsdone, outside of a compile), `macrobuff` is reallocated when `macrolen` differs from its limit: a macro's text is limited to `macrolen` ("Macro too Big"), at least 8192 bytes are allocated (doincbin's FileInfoBlock and the unchecked macro parameters, #4, use the buffer too); if the allocation fails the old buffer and its limit stay | **F** | #336: a source with a larger Macro Buffer compiles macros of more than 8192 bytes, a smaller one rejects macros between it and 8192 bytes; with the default 8192 nothing changes (tests `compiler-macrobuff-size`, macrobuf=16384, `compiler-macrobuff-size-small`, macrobuf=1024). A Macro Buffer over 65535 reaches the kept `BUG(orig)` of the macro text length (a word, tokens1.c domacro) |
| domacro (tokens1.c:884) | the node of a new macro (addhere, AllocMem without MEMF_CLEAR) gets only its use count; text pointer and length are written at End Macro: an error before it ("Macro too Big", "Macro without End Macro") ends the compile with the node half-built, and freemacs (errcont freeemup) frees a text with the pointer and length the memory held - on Kickstart 3.x an alert (0100 000F, 8100 0005) that waits for the mouse, the error never shown; with a 0 in the length word nothing happens | text pointer and length cleared after addhere: the half-built macro has no text | **F** | #965: only the error path changes, no generated code (tests `compiler-macro-uninit-free`, `compiler-macro-uninit-free-noend`, with inject fill=a5) |
| getchar `.cmake` (parse.c:432) | every character outside a false CNIF block is compared with `~`, the start of the constant maker `~expr~`, also inside a string literal: a `~` in a `Dc.b`, `Print`, `#"…"` or `Data$` string gives "Illegal Constant Expression", a string with a valid `~expr~` gets its value put in (`"a~1+2~b"` is `a3b`) | `~` starts a constant maker only outside quotes (`quoteflag`); inside a string literal it is an ordinary character, nothing is computed inside strings | **F** | #966: BB2 and AB3 mode; a program without `~` in a string literal compiles to the same bytes, `~expr~` outside strings is unchanged (test `compiler-cmake-tilde-in-string`, `ab3_cli`/`ab3_cli_ref` case `tilde_string`) |
| zeropc: doeven4, doeven8, alignpc/doeven, dods (tokens1.c:513), pokewd (eval.c:210); litstring (eval.c:2191), dodata (tokens1.c:703) | `Even`, `Even4`, `Even8`, the word alignment of `Dc.w/.l`, `Dcb.w/.l` and of inline assembler instructions (alignpc), the alignment pokewd does before every word of code or data (an odd `Dc.b`/`IncBin` followed directly by code), and `Ds` move `pc` only: the skipped pad bytes and the reserved storage hold what the object buffer held there - with runtime error checking on the debug record undodebug withdrew for the label or directive (trap #1, line item address, column), with Compile/Run the bytes of the previous compile of the session. A string literal (litstring, data1) and a `Data$` string (dodata, data2) of odd length round their pointer up to even, the pad byte is not written either: with Compile/Run it holds the previous compile's byte | the skipped bytes are written as 0, the way pokebyte writes (bounded by `libat`, `nomemleft` past it, `bigpc`); the pad byte of a string literal or `Data$` string as 0 with pokedata1b/pokedata2b (bounded like the string's bytes) | **F** | #969: BB2 and AB3 mode; executables differ only in those bytes, with Create Executable only with runtime error checking on (otherwise the buffer is cleared); of the CD's sources only QUIKASM.BB2 (runtime error checking on, `Ds` behind labels; `tests/data/blitz2-cd.tsv` holds the port's hash, with a note). The pads of string literals and `Data$` strings change only the program in memory of Compile/Run (Create Executable compiles into a cleared buffer: executables unchanged). (tests `compiler-even-pad-byte`, `compiler-even-pad-byte-code`, `compiler-data-pad-byte`) |
| Create Resident: writeitem, writeoffs, restypes, resmacs, resconsts (stmts.c:875); addhere (parse.c:856), domacro (tokens1.c:977) | every record is written with its length rounded up to even: for an odd one (constant, NewType field, type, macro, macro text) the byte behind its allocation (addhere and domacro allocate exactly the length, MEMF_PUBLIC). A constant's data and procnum keep what the allocation held (handleconst sets only refs and the value; `#c` without `=` not even the value), a field's type keeps the low word of the type's address (writeoffs replaces the high word by the type number), a type's next is the pointer to the next type in memory. The loaders never read these bytes, but the same source gives a different file depending on the memory | a 0 behind an odd record, the type number alone in a field's type (low word 0), next 0 in a type record; the nodes (addhere) and the macro text allocated with MEMF_CLEAR (a constant without `=` is 0) | **F** | #970: BB2 and AB3 mode; residents written by Create Resident differ in those bytes (and are reproducible), loading is unchanged. Executables differ only for sources that read a node field nobody set: `#c` without `=` (value 0), a NewType whose field has the type being defined (size 0). (test `compiler-resident-stale-bytes`, with inject guard fill=a5) |

The individual reasons are each given at the comment `DIFF:` in
`programs/blitz2/src/`.

## 5a. Editor ted-c (TED 1.1, `programs/ted`)

The reference is the vasm build of `ted.s` ([`../ted/reference.md`](../ted/reference.md)).
Separate list of the TED deviations: [`../ted/deviations.md`](../ted/deviations.md)
(continuously extended there); here the rows that concern the shared
file requester.

| Place (file:line) | Original | C version | Kind | observable? |
|---|---|---|---|---|
| `flink` (ted.s:7953; programs/ted/src/filereq.c) | the embedded requester hunk `filereq` relocates itself at runtime on start (from `outside`) (`add.l basis` on 312 longwords in its own code hunk, without cache flush; a second run would relocate twice – BUG(orig), #360) | dropped: the requester is C code (`shared/filereq/filereq_core.c`), its pointers are relocated by LoadSeg | **T** | technically unavoidable: no embedded hunk any more, nothing to relocate. No (except the double relocation, which never happens in the original) |
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
The RWE userlib rigtmenulib had seven (`existschk`, `statechk`,
`statechk2_c`–`statechk4_c`, `chkchk3_c`–`chkchk5_c`, behind the thunks
of createchk/setchk): their references to the messages were the three
`blitzlibcheck` warnings of rigtmenulib; the thunks now follow all check
functions.

## 7. State

All command libraries are ported (c4a736d); the tables also contain
scrolllib, 2dlib, polylib (S12) and gameiolib, blitzkeyslib, animlib,
blitzcoplib, displaylib (S13); dhandlerlib (lib 1), the last original
binary in acidlibs, followed on 2026-10-29.

## Summary

* **B – user decision:** the a2 convention (about 20 libraries,
  never observable) and register preservation "at least like the original"
  (additionally preserved registers are never results).
* **T – technically unavoidable:** undefined register leftovers and
  OS returns (blitlib BlitColl, freqlib, cliargslib CCR, windowslib
  zcursor, ilbmifflib a0), memory layout with overflows and
  invalid indices (2dlib, polylib, displaylib, crunchlib), stack contents
  with error jumps via JSR (displaylib) and other stack questions
  (crunchlib, arrayslib a4/a5), timing (shapetrixlib XFlip) and in the
  compiler all stack tricks and placeholders. Observable at most in
  error cases, with overflows or corrupted data.
* **K – silent corrections (existing):** traps in normal code instead of a jump
  into the cut-off error check area (inputoutputlib, fileiolib,
  palettelib, windowslib, brexxlib), return instead of crash
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
* **F – fixed original bugs (intended):** displaylib's error routines in
  the normal code (#412).
