# Command libraries in C: findings per library

State of the port of the "simple" command libraries from `acidlibs`
(overview and test state: [bb2-readme.md](bb2-readme.md)). Source in each case
`build/<preset>/bblibsrc/NAME` (detokenised from `bblibsrc`), reference the CD binary
(`BLITZ2/ACIDLIBS`, `blitzlib extract`). `BUG(orig)` = peculiarity of the original, deliberately
carried over (for the comparison with AmiBlitz3).

## General

* **Calling convention of the commands** (from sources and `blitz2.s`
  `preplibst`): arguments in d0-d5; arguments that the compiler puts on the
  stack (string lengths, array end and type) are read by the routine via
  a2 = stack pointer before pushing (`MOVE.l -(a2),dN`). The originals
  leave a2 decremented afterwards. All C versions get a2 as a
  register parameter of the respective routine (`BL_REG(a2)`), read with
  negative offsets and leave a2 unchanged (DIFF); there is no global
  register variable for a2 anywhere. Reason: the compiler sets
  a2 anew before every call (`preplibst`: `LEA N(a7),a2`), never reads it
  afterwards (it cleans up the stack via a7), and lib code relies on
  a2 being preserved across ALibJsr calls (windowslib `dowtitle`,
  screenslib); the libsdev documentation describes a2 only as input.
  See [architecture.md](../architecture.md) 5.5 and `EXTRAS/LIBSDEV/DOCUMENTATION` of the CD.
* **String results** from a3 on, d0 = length; a3 then normally points
  to the zero byte (`blitz_string` in C). A **missing terminating 0**
  is practically invisible: assignment, Print and concatenation work with
  the length, and where the compiler passes a string on as an address/argument,
  it appends `clr.b (a3)+` itself (`sbtolong`/`ceos`,
  blitz2.s:8767–8771, C `eval.c:567`) – a3 is behind the data then.
  The entries "without 0" below (Peeks$, RexxError, ReadSerialString,
  BoxString) are therefore peculiarities without observable consequence; what does become visible
  is an a3 that is *behind* the 0: the next part of a string expression
  is then written one byte too far (GetRexxCommand/GetResultString,
  fixed #52; FileRequest$, fixed #161).
* **Register preservation:** according to the AmiBlitz3 spec a lib routine may change everything
  except a4/a5/a7. As with the system libs, the C versions
  nevertheless preserve at least the registers that the original preserves (thunks,
  checked in `[cmd]`).
* **OS calls with a parameter in a3, a4 or a5** (e.g. workbench
  `AddAppIconA`: DiskObject in a3, tag list in a4; intuition
  `EasyRequestArgs`, `AutoRequest`, graphics `TextFit`,
  `ChangeExtSpriteA`: a3): the libraries are compiled with
  `-ffixed-a3/a4/a5`, and gcc does **not** restore such a register after
  the call. Measured with gcc 16.2 (`-Os`/`-O2`, `-Wall -Wextra`,
  2026-11-14):
  * the NDK inline macro (`proto/*.h`, register variables on a3/a4)
    compiles without error or warning, loads a3/a4 and returns with them
    changed;
  * an `__asm` block that loads a3/a4 itself with `"a3","a4"` in the
    clobber list: the same, no warning, no save (a clobber of a fixed
    register makes gcc save nothing; the prologue saves only the
    callee-saved registers it allocates itself);
  * register variables on a3/a4 plus `"a3","a4"` in the clobber list:
    a compile error ("'asm' specifier for variable 'a3' conflicts with
    'asm' clobber list").

  So such a call has to save and restore the register itself, inside the
  `__asm` block. For the NDK's inline macros the build does that:
  * every library sees the NDK's `inline/X.h` through a shadow header
    (`build/<preset>/libs/include/ndk/inline/X.h`, written at configure by
    `cmake/ndk-calls.cmake`, first in the include path of
    `blitz_library`; blitz2 and TED do not see them). Per
    `libs/include/ndk/functions.list` a function with a parameter in
    a3/a4 is either **safe** - `libs/include/ndk/safe/X.h` redefines its
    `__NAME_base` with the a3 (a4) value in a register gcc chooses and
    `exg` with a3 (a4) around the `jsr` (4 bytes per call and register,
    no stack, the condition codes untouched: the NDK's own technique for
    a5) - or **poisoned**: a call does not compile
    (`attempt to use poisoned 'NAME'`, a note at the shadow header). The
    varargs forms of both (`EasyRequest`, `__EasyRequest_va`) are
    poisoned (they were expanded with the NDK's macro). The a5 functions
    are **native** (the NDK saves a5 with `exg %/d7,%/a5`). A library that
    needs a poisoned function writes its safe form (the NDK's macro as the
    model, as `safe/intuition.h` does) and marks it safe in the list.
  * configuring compares the list with the functions the NDK's inline
    headers declare with a register variable on a3/a4/a5 or an `exg` with
    one of them, in both directions: a difference (a function of a new
    NDK, a name the NDK no longer has, other registers, another state) is
    a configure error naming the function and its header.
  * the shadow headers also make the NDK's varargs wrappers `static
    inline` (`__stdargs` redefined around the include): with
    `-fno-toplevel-reorder` every unused one was kept as dead code.

  Hand-written `__asm` calls of OS functions (or of non-NDK libraries,
  e.g. neilsreqtoolslib's reqtools calls with `movem.l a3-a4,-(sp)`) are
  not covered by the build: such a block saves the register inside
  itself (`exg` with a free register as in `safe/*.h`, or
  `movem.l a3/a4,-(sp)` before the parameters are loaded and
  `movem.l (sp)+,a3/a4` after the `jsr`, careful with sp-relative
  operands); the library tests record a3/a4 per routine. Tests:
  `ndk_list_ok`, `ndk_list_missing`, `ndk_list_extra`, `ndk_list_pending`
  (the configure check with a list changed in memory),
  `ndk_poisoned_call` (+ `_broken`), `ndk_varargs_inline` (+ `_broken`).
* **Shared helpers:** what several ports need is in
  `libs/include/blitzlib.h`/`blitzhelp.h` instead of locally: `BLITZ_ALIBJSR_A6`
  (ALibJsr with a6, vallib), `BLITZ_ERRCHK_THUNK`/`BLITZ_ERRCHK_MESSAGE`
  (error checks in C with messages in the error check area, linklistlib,
  vallib), `BLITZ_BASEJSR1/2` + `LVO_SP…` (mathffp), `BL_SETW`/`BL_SETB`/
  `BL_EXTL`/`BL_SWAP`, `bl_divu_w`/`bl_divs_w` (datelib, modlib),
  `BL_POPL`/`BL_POPW` (a2 cursor, stringfunclib, vallib), `TOK_…`/`MEMBASE_…`
  (memlib, floatquicklib etc.). New ports use these instead of their own
  variants.
* **Inline routines** (`Dc $a000,len`): stay assembler text
  (`BLITZ_INLINE`); behind the inline code there is an RTS in C, the
  original has none (the compiler copies only `len` bytes).

## 180 memacclib (Poke, Peek, Peeks$, Call)

Binary = source. Poke/Peek are the only `!ustatement`/`!ufunction`
of BB2: type in d0 (1/2/3/4/5/7), address in d1, data in d2.

* `Peek.b`/`Peek.w` replace only the lower byte/word of d0 (rest =
  upper bits of the type, 0 with the compiler); type comparison as word, type 0
  like byte, all others except 2 and 7 like long.
* `Peek$` copies up to and including 0, a3 afterwards on the 0.
* `Peeks$(a,n)`: writes no terminating 0 (as in the original, not a bug, #213: result
  only via d0 = n; without visible consequence, see General); a negative n gives the empty string, as 0
  does (fixed, intended deviation, #214: the original counted n as an unsigned
  32-bit number and copied ~4 GB; bug-orig test `memacclib-peeks-negative`). A negative n is a
  programming error: with runtime error checking it is the error "Can't Peeks$ a negative number
  of Characters!" (`peekschk`, fixed, intended deviation, #214: the original had no error check,
  memacclib has an error check area only for it; bug-orig test `memacclib-peeks-negative-check`).
  A zero byte within the n characters stays in.
* `Poke.s`: copies the length of the string (from the stack) + 0, embedded
  zero bytes included.
* `Call`: `MOVE.l d0,a0:JMP (a0)` - stays assembler, because the called
  routine gets all registers unchanged and may change them freely.
* `Poke a,x` without type = Quick (`Poke a,-77` writes $FFB30000).

## 110 sortlib (Sort, SortUp, SortDown)

Binary = source. Recursive Hoare quicksort ("Blitz 1 Sort algorithm"),
direction via a word in the code (init = SortUp). Type from the compiler (word on
the stack): 1 byte, 2 word, 5 float (mathffp `SPCmp`, a6 = ffplib),
7 string (strcomplib `$CE02`), everything else (3 long, 4 quick) as long.

* Fixed (intended deviation, #275): for word arrays the pivot element was
  read with `MOVE.l` and compared with `CMP.w` - the pivot was the element
  **after** the middle. For arrays with at least 2 elements it was still
  within range, the result correct; for a single element (`Dim a.w(0)`)
  it lay behind the array: the forward search read on behind it, the
  backward search possibly read words before the array – reading only,
  result correct. The pivot is now read as a word, the middle element
  (snitch test `sortlib-word-pivot`, `[.bug-orig]`: the array at the end
  of the highest memory region, the reads behind it logged by Amiberry).
* Fixed (intended deviation, #276): string elements that were never
  assigned (pointer 0) read their length from -4(0) and their characters
  from 0 (an arbitrary order); a pointer 0 is now the empty string (bug-orig
  test `sortlib-string-null`).
* Recursion depth in the worst case O(n) (a property of quicksort, not a
  bug); the C version needs per level more stack than the 12 bytes of the
  original.
* Assembler only for evaluating the CCR of `SPCmp` (the original
  branches directly on the flags); strcomplib returns -1/0/1 in d0 and
  the same value in the CCR, there d0 suffices.

## 192 mathtranslib (ACos … HTan, `^` on Float)

Binary = source; the valid lib number is the second definition
`#mathtranslib=192` in libnums.bb. All 14 routines are inline `JSR lvo(a6)`
with a6 = mathtrans base (init value).

* **Fixed (#202, intended deviation):** the original's init tries
  `OldOpenLibrary("mathtrans.library")` endlessly if the library is missing,
  so a program using a transcendental function hangs at startup. Now the
  init returns the base or 0 (as the deflibs since #44) and the finit closes
  it only if it is not 0; a call of a function of the missing library then
  crashes (test `mathtranslib-init-endless`).
* Edge values (Log(0), Sqr(-1), ACos(2), 10^19 …) are delivered by mathtrans itself,
  e.g. Log(0) = -9.223371E+18; not the lib's business.

## 194 utillib (CreateMsgPort, DeleteMsgPort, Wait, TaskName$)

Binary = source (bum8). Ports are in a block of $2A bytes
(memlib globalloc): +0 next, +4 prev (own port list, head
`PortListStart`), +8 MsgPort; the name in a separate block
{size.l, characters, 0}. Finit deletes all ports still present. The
init value is the **address** of the variable `signals` (for other libs:
`#utillib,#la0`, then `btst sigbit,(a0)`).

* `TaskName$` leaves a3 **behind** the zero byte
  (no `SUBQ #1,a3`); d0 = length is correct, in the corpus without visible consequences
  (`TaskName$(0)+"|"` is correct; not a bug). Fixed (intended deviation,
  #297): a task without a name copied from address 0; its name is now the
  empty string (bug-orig test `utillib-taskname-no-name`).
* `CreateMsgPort` returns with a3 = name argument ("Protect BB2 String
  area"); the name length comes from -4(a2) of the caller; copied are
  (length & $FFFF)+1 bytes (`DBRA`). Empty name: port not public,
  `ln_Name` = 0.
* Fixed (intended deviation, #298): a failure of `AllocSignal(-1)` (-1)
  was not detected (`TST.l d0:BEQ` checks for signal 0), `mp_SigBit` was
  $FF and `DeleteMsgPort` called `FreeSignal(255)`; the never-reached
  error branch would have freed the port address (block+8) and $22
  instead of the block and $2A. Now -1 frees the block with $2A bytes and
  gives the runtime error "Failed to allocate signal for Port" (bug-orig
  test `utillib-createmsgport-no-signal`). `FreeSignal` in DeleteMsgPort
  is zero-extended in C (the NDK inline takes BYTE).
* Memory error → runtime error "Failed to allocate memory for Port"
  (`TRAP #0`); the original then runs into its message text, the
  C version returns with 0 (#300, fixed in the port). Not testable.
* `DeleteMsgPort` checks nothing: foreign ports destroy the list (an
  invalid argument). Fixed (intended deviation, #299): pending messages
  remained unanswered; they are now replied before the port is freed
  (bug-orig test `utillib-deletemsgport-unchecked`).
* `Wait` waits for **all** allocated signals of the task (`tc_SigAlloc`),
  result also in `signals` (the intended semantics, not a bug). In the test beforehand with inline assembler
  `Signal(FindTask(0),SIGBREAKF_CTRL_F)`.

## 193 environlib (WBWidth … WBExtViewMode, Processor, cr_vbr, ExecVersion)

Binary = source. Everything is determined once in the init (a6 = intuition):
`GetScreenData(buf,346,WBENCHSCREEN,0)` (result unchecked), width
+12, height +14, depth from RastPort.BitMap (+88) → Depth (+5).

* The long at +76 is `ViewPort.Modes` (upper word, WBViewMode) and
  SpritePriority/ExtendedModes (lower word, WBExtViewMode).
* `Processor` = number of shifts until `AttnFlags & 15` is zero (68020 → 2).
* `cr_vbr` is read from Processor ≥ 1 on (`MOVEC VBR` in supervisor mode
  via SuperState/UserState), 0 on the 68000. Fixed (intended deviation,
  #42): the original reads it only from Processor ≥ 2 on, on the 68010 it
  stays 0, although it has a VBR (likewise errtraplib, runerrlib and the
  compiler's calcvbr; bug-orig test `multi-68010-vbr`). `MOVEC` as
  `.word` (68000 assembler).
* The word functions load with `MOVE.w`, the upper word of d0 stays -
  reproduced in C via `BLITZ_THUNK_POST(... "move.w d0,2(sp)")`.
* Values depend on the environment, but are the same in every run of the same Amiberry
  configuration (A1200: 640x512x8, Modes $C004, 68020,
  VBR 0, Exec 47).

## 78 datelib

C translation of bblibsrc/datelib, binary = source. The word/byte arithmetic of the original is reproduced register-exactly (`BL_SETW`/`BL_SETB`, DIVU/DIVS as an instruction with `bl_divu_w`/`bl_divs_w` as in modlib), so that invalid inputs also come out the same. Fixed (intended deviation, #120): day 366 of a 4-year cycle appeared as "32/12" of the leap year (`BLE` instead of `BLT`), now the 1 January of the next year (bug-orig test `datelib-day366`). Fixed (intended deviation, #1001): the 4-year cycles made every year divisible by 4 a leap year (1900, 2100 too), so dates before 1/3/1900 and from 1/3/2100 on were off by a day per century; Date$ and NumDays now correct the day count by the Gregorian century rule, nothing changes from 1/3/1900 to 28/2/2100 (bug-orig test `datelib-century-leap-years`). Fixed (intended deviation, #121): WeekDay subtracted 2 with `SUBQ.w` (no borrow: the weekday 2 too large for a day count whose low word is 0 or 1, e.g. 15/9/1973), now `SUBQ.l` (bug-orig test `datelib-weekday-borrow`). Fixed (intended deviation, #122): NumDays corrected the count with `ADDQ.w`/`SUBQ.w` without carry (NumDays("31/12/1977") = 65535), now `ADDQ.l`/`SUBQ.l` (bug-orig test `datelib-numdays-word-carry`). Fixed (intended deviation, #124): NumDays indexed the month table without a check (month 0 read the tick word of the last DateStamp, 13 the end mark, 14/15 the two words behind the table, larger months foreign memory); a month outside 1-12 now gives 0 (bug-orig test `datelib-numdays-month-range`). Fixed (intended deviation, #123): a "/" as the first character of a field (an empty field) gave the field the value 47 ("/5/1990" = 47 May 1990); an empty field now gives 0 (bug-orig test `datelib-numdays-slash-field`). Fixed (intended deviation, #125): the year field ended only at "/" or after 4 characters, so a year of fewer digits without "/" read beyond the end of the string; it now ends at the end of the string too, an empty year gives 0 (bug-orig test `datelib-numdays-short-year`). Fixed (intended deviation, #1029): the day and month fields read their second character and the "/" behind it past the end of a short string; such a field now gives 0 (bug-orig test `datelib-numdays-short-field`). Test: t30_date (without the days of #120; its dates before 1/3/1900 and from 1/3/2100 on are the Gregorian ones, #1001).

## 128 linklistlib

CD binary older than the source (SetItem/CountList/CountItem missing) – ports only the 14 tokens + init of the binary, code otherwise the same as the source. Interrupt lock ($dff09a) as in the original, list heads as nodes, item stack via memlib globalloc/globfree. Error checks in C in the error check area (`BLITZ_ERRCHK_THUNK`, messages as assembler data with `BLITZ_ERRCHK_MESSAGE`). Fixed (intended deviation, #201): SortList compared with CMP.l/BMI (the sign of the difference: keys more than 2^31 apart, ±2·10^9, stayed unsorted); the compare is signed now (bug-orig test `linklistlib-sortlist-sign`; t31_list.fixed-201.out). Not bugs (not observable): ClearList hangs the items into the free list in reverse order, deleted items keep their data. DIFF: SortList does not destroy a3. Tests: t31_list as well as the runtime error tests t32–t36 in corpus/tests-runerr (runnable only with error checking); all outputs the same, except the SortList of t31_list with ±2·10^9 (#201: `t31_list.orig.out` / `t31_list.fixed-201.out`).

## 185 stringfunclib

C, declarations identical to the CD binary (source = binary). a2 (length stack of the string arguments) is a register parameter of every routine that reads lengths; reading is done via a local pointer, a2 stays unchanged (DIFF, see General). Chr$ and Len stay inline (assembler text). The init value is, as in the original, the address of movestring (a0/a3 are advanced; no other lib of the CD uses it). The original leaves a2 at different places depending on the case (String$ with count ≤ 0 and Replace$ with an empty search string take fewer lengths from the stack, Cvi/Cvl/Cvq none at all); the C version always leaves a2 unchanged (DIFF, results the same). The inline Len stays `MOVE.l -(a2),d0` as in the original. Fixed (intended deviation, #424): Replace$ with an empty search string left a3 behind all three arguments; it now ends on the 0 of the result (bug-orig test `stringfunclib-replace-empty-a3`). Fixed (intended deviation, #285): Right$ and UnRight$ took the offset Len-n as a word (for Len-n >= 32768 the characters before the string); it is a long now (bug-orig test `stringfunclib-right-word-offset`). Fixed (intended deviation, #286): Mid$ compared start-1+length signed after the addition, so a huge length copied that many bytes; the length is now compared with the rest of the string (bug-orig test `stringfunclib-mid-overflow`). Fixed (intended deviation, #1030): the end test of Instr added the length of f$ to the position and compared after the addition, a start position near $7FFFFFFF overflowed it and the search ran through memory; it now compares the position with the difference of the lengths (bug-orig test `stringfunclib-instr-end-overflow`). Fixed (intended deviation, #287): Instr with a start position ≤ 0 compared before the string; such a start is now the first character (bug-orig test `stringfunclib-instr-startpos`; t20_strfunc.fixed-287.out); such a start is a programming error: with runtime error checking it is the error "Instr Startpos MUST be greater than zero" (`instrchk`, error check area only, the original had none; bug-orig test `stringfunclib-instr-startpos-check`; in corpus_runerr t20_strfunc stops there). Fixed (intended deviation, #288): Cvi read 2 bytes, Cvl/Cvq 4, whatever the length of the string; a shorter string now gets 0 for the missing bytes (bug-orig test `stringfunclib-cvi-length`); such a string is a programming error: with runtime error checking it is the error "Cvi needs a String of at least 2 Characters" ("Cvl ...", "Cvq ...": 4; `cvichk`, `cvlchk`, `cvqchk`, error check area only, the original had none and no error check area; bug-orig tests `stringfunclib-cvi-length-check`, `stringfunclib-cvl-length-check`, `stringfunclib-cvq-length-check`). Test: `t20_strfunc` (all 28 tokens; the original libs give `t20_strfunc.orig.out`, the Instr start positions of #287 differ).

## 120 vallib

C, declarations identical. Int/Sgn/Abs/Val and intchk get a2 as a register parameter and read the type word or length with a negative offset; a2 stays unchanged, the original leaves it decremented by 2 or 4 (DIFF, see General). All float calculations go through mathffp with the same calls and operands. QFrac, QWrap, QAbs, VPos, True and False stay inline; the arc table of QAngle comes from the binary, DIVU including overflow is reproduced in C. Rnd reads the hardware in the same order; the sequence depends on timing, hence only a range check. Fixed (intended deviation, #301): Sgn of a float was always 1 (SPTst checks D1, which held the type word); SPTst gets the float in d1 now (bug-orig test `vallib-sgn-float`); Val counted the length as a word (lengths 32769..65536 gave 0), now as a long (#303, test `vallib-val-word-length`); Val takes only the first 9 decimal places (the original took all of them modulo 2^32: wrong from 10 places on, the fractional part dropped from 32 on) (#304, test `vallib-val-decimal-places`). BUG(orig) kept: Val reads one byte behind the string after "-", "$" and "%" (normally the terminating 0, which the compiler writes behind every string argument) (#302). Fixed (intended deviation, #1003): QAngle subtracted the coordinates as words, so a difference of 32768 or more in magnitude gave the angle of another quadrant; the differences are longs now (bug-orig test `vallib-qangle-word-range`). Test: `t21_val` (all 16 tokens; the QAngle lines with differences of 32768 or more are the fixed ones, #1003).

## 124 fileiolib

C, declarations identical (binary = source, March 1995). Object lib "File" (32 bytes, maximum 5, Free = CloseFile); object pointer in a1/a2/a3 depending on the token ($1180/$1280/$1380). The buffered DOS layer (myread/mywrite/myseek/myflush) is C with the word arithmetic of the original; without work (length 0, position in the buffer) d0 comes back unchanged (FileSeek/Put/Get then compare the file number or field address with -1). The handlers for inputoutputlib (FileInput/FileOutput) keep the BRA table before the entry address (-8/-4/0) as a short assembler block, call C via thunks. BUG(orig): Exists = 0 also for empty files (#143, won't fix: Exists returns the length of the file, as the manual says, and programs use it as the length; test `fileiolib-exists-empty-file`), -1 for drawers (fixed, intended deviation, #1026: for every directory type, fib_DirEntryType > 0, not only 2, and 0 if Examine fails; the original returned the contents of the FileInfoBlock then; test `fileiolib-exists-root-examine`); Fields adds to the fields already set (not a defect, #141: the author's "record len so far"; a further Fields continues the record, two statements give the record of one with the joined list); Lof does not flush (not a bug: that is the length including buffered data); Put/Get transfer the fields in the reverse order of Fields (not a bug, decision 2026-10-31: consistent with itself, existing data files depend on it) (#142). Fixed (intended deviation, #138): OpenFile/ReadFile/WriteFile on an open file with Fields read the name length from -4(0) and cut the name there; the length is now fetched before the file is closed (test `fileiolib-reopen-fields-name`). Fixed (intended deviation, #140): CloseFile freed the Fields but left the pointer to them and the record length in place; it now clears both (test `fileiolib-closefile-fields-stale`). Fixed (intended deviation, #144): Eof at the end of the file left the buffer position 1 too high (myseek stored the new position before the DOS seek, which fails there), so Loc/Lof were +1 afterwards (t50: Lof 32 for 31 bytes); a failed seek now keeps the old position (test `fileiolib-eof-loc-lof`, corpus t50_fileio). DIFF: err1/err2/err3/err6 and the message of err7 are jumped to by the original from normal code into the error check area (cut off without debugger) – here in normal code; a2, d4-d7, a3 stay unchanged. Tests: t50_fileio, t53_fileran, t54–t57 (tests-runerr).

## 106 cliargslib

C, declarations identical. Data of wbstartuplib in a0. FromCLI loads only the lower word of d0 (thunk). By design (#106): the argument number is checked only with runtime error checking. BUG(orig): ParPath$ from the CLI calls NameFromLock(0) (#107) (lock 0 = root of the boot file system; which text dos.library returns for it, e.g. "SYS:", is not checked). DIFF: after NameFromLock, d0 = 0 is checked instead of the CCR of dos.library. Tests: t51_cliargs (the corpus runner passes no arguments; checked locally with arguments), t58/t59 (tests-runerr).

## 76 banklib

C, declarations identical. Object lib "Bank" (16 bytes: pos, size, memtype; maximum 5), no Free. LoadBank asks fileiolib Exists (ALibJsr `TOK_exists`, a6 = dos). InitBank stores size/type before allocating (without consequence in the original: memlib does not return on lack of memory, only with the C memlib, K; #71). Not bugs (no observable effect / description): FreeBank leaves memtype in place; LoadBank allocates anew for a different type or if the file is not smaller (sign of the difference) and reads bank-size bytes; missing or empty file: nothing. Fixed (intended deviation, #72): LoadBank of a drawer (Exists = -1) with a different memtype made the size -1 (an AllocMem of 7 bytes, BankLoc behind it); now only a size > 0 loads, a drawer leaves the bank as it is (test `banklib-loadbank-drawer`). Test: t52_bank.

## 164 inputoutputlib

C, declarations identical, binary = source. Channels are handler addresses with the entry points -8 (string with 0), -4 (string a0/d0 or read line) and 0 (character); BRA.w stand before the handler. dosetin/dosetout (+3/+4) put the old handler on a ring of 16 longs (index & 63) and count; init puts handler 0 on both stacks. The five dispatchers (+5…+9, `JSR off(a6)` with a6 = handler) stay assembler because they pass all registers unchanged to foreign handlers; the rest is C, dos via proto/dos.h. Default input = `Read(Input(),a0,d0+1)-1` (end of file -1, error -2). DIFF: the single-character entry of defin is in the original `BRA err1` into the error check area (without debugger a jump behind the lib); in C trap and message are in normal code. Fixed (intended deviation, #193): the message of the single-character entry read "Defualt", now "Default" (bug-orig test `inputoutputlib-default-typo`; t43_inkeyerr.fixed-193.out). By design (#194): without runtime error checking, more pops than pushes take old ring entries and a PopOutput directly after the start sets the handler to 0. Fixed (intended deviation, #1032): the error check of PopInput/PopOutput tested the count for 0, but init's push counts, so the first pop after the start passed; a count of 1 or less is now the error (t44_popinerr.fixed-1032.out; t45_popouterr shows the same output, its error now comes one pop earlier). Tests: t41_io, t43–t45 (runtime errors).

## 187 printlib

C, declarations identical. `libincs:ffptoascii` is missing on the CD; the routine is translated from the binary ($658–$9D3: 7 digits by subtracting powers of ten, table 1E19…1E-28, rounding via $80 below the mantissa). Variables, buffer and table lie one after the other as in the original (the fixed point buffer 2 bytes longer, #241), so that the Format overflow (#240) comes out the same. Output via inputoutputlib (`TOK_iooutsingle`/`TOK_iooutstring1`); Str$/UStr$ set their own handler (strprint, writes to a3). FloatMode 0 = fixed point for 1E-9 ≤ |x| < 1E7. Fixed (intended deviations, kind F): a Quick fractional part that rounds to 0 no longer keeps the point ("1" for $00010001, "0" for ±1/65536) (#234); a Quick fractional part that rounds to 10000 carries into the integer part ("3" for $0002FFFF, the original "2.") (#233); Format rounds the digits before formatting them, so the carry beyond the first digit makes a new digit (999.96 with "####.#" → "1000.0", the original " 000.0") (#235) and numbers without an integer digit (|x| < 1) are rounded too (#236); on overflow "?" is followed by the lowest digits (1234.5 with "##" → "?35", the original "?  ") (#238); without "-"/"+" in the format the "-" of a negative number takes a leading place (-7 with "###" → " -7", the original "  7") (#237); FloatMode -1 no longer writes behind the fixed point buffer for numbers < 1E-17, into the table of powers of ten (#241); Str$ and UStr$ of a string return the string (the original printed the lower byte of its address) (#242). By design (#240): Format over 32 characters overwrites formlen/formlead (checked only with runtime error checking). Not bugs: no leading 0 (`.5`, the output style of Blitz 2); every Str$ puts two entries on the output stack (a ring of 16, harmless). The compiler knows exponents only up to 18 (`1E-19` → "Numeric Over Flow"). DIFF: a2 stays unchanged. Test: t40_print (all types, edge values, FloatMode -1/0/1/2/-5, 23 format strings).

## 186 editlib

C, declarations identical, binary = source. Edit$ pads the default with spaces to n characters and reads over it (`TOK_iogetstring`). Edit uses its own buffer of 80 bytes, the default via UStr$ (`TOK_ustr`) and the result via Val (`TOK_val`), both with a2 on the block `pars` {length.l, type.w = 5} (`BLITZ_ALIBJSR_A2A6`). Fixed (intended deviation, #128): Inkey$(0) returned the caller's d6 as the length (MOVEQ #0,d6 skipped), now the empty string (bug-orig test `editlib-inkey-zero`). Fixed (intended deviation, #1031): Inkey$(n) counts n as an unsigned long, a negative n makes about 4 billion attempts; the original had no error check for it, an error check of its own (error check area only) now refuses it with "Can't Inkey$ a negative number of Characters!" (bug-orig test `editlib-inkey-negative`). Fixed (intended deviation, #129): with Edit(value,n) the default lost its last character through the padding (UStr$ leaves a3 without 0 behind the digit, the `SUBQ #1,a3` is meant for string arguments; Edit$ was not affected); the padding now starts behind the default, and Edit(n) has no default (the original's "0" was overwritten for n >= 2) (bug-orig test `editlib-edit-default-pad`). Not a bug: Edit calls UStr$/Val with the caller's a6, but the compiler links each ALibJsr call through a stub that loads the callee's `!libs` first, so Val always gets the ffplib base (#130). By design (#131): Edit(80) reads 81 bytes (the length is checked only with runtime error checking). Fixed (intended deviation, #132): at the end of the file DefaultInput returns the length -1 (-2 on an error), which Edit$ returned and added to a3 and Edit gave Val; it now counts as nothing read (bug-orig test `editlib-edit-eof`). DIFF: a2 stays unchanged. Tests: t41_io (DefaultInput via a child process with redirected input), t42_edit (FileInput), t46–t49 (runtime errors).

## 74 crunchlib (`#romuluscrunchlib`)

C, declarations identical, binary = source. Imploder (Implode/Deplode), Crunch-Mania decruncher (CrM!/CrM2) and PowerPacker decruncher (PP20) translated register-exactly (widths byte/word/long, shift counts mod 64, DBF on the low word); additionally checked on the host in unicorn (68020) against the original code: Implode output, Deplode and all decrunchers byte-identical on 330+ data sets and 400 corrupted inputs. Implode always runs with "Mode 8" (smallest table, window 128), writes COLOR00 in the process and polls the left mouse button (abort → -1). BUG(orig): the Implode search reads up to 254 bytes behind the data (`im30` up to 15, length measurement `im32`/`im33` up to 254, not added; result independent of it) (#112); result 0 leaves the buffer partly overwritten (#114); CrM overlap copy with CrLen<4 copies 4 GB (#115), CrMDecrunch with one argument overwrites MinSecDist bytes before the buffer (#116); ReadTab/ReadD1_SUB with 0 run 65536 times (#117, #118). Kept, not a bug: after $4012 literals the rest is stored uncompressed (limit of the Imploder format, #113). DIFF: corrupted CrM2 data can reach beyond the tables into the lib code (result depends on the lib layout). Test: t60_crunch (test data for PP20/CrM from our own encoder).

## 92 arexxlib

C, declarations identical, binary = source. BB2 ArgStrings (memlib, ra_Size with bit 31 = "from Blitz"), RexxMsgs (128 bytes, ln_Name "REXX"). Errors (memory, no REXX port, no rexxsyslib) show a recoverable alert and return the error code (0/4/8). BUG(orig): ReplyRexxMsg checks with `BTST #17,28(a0)` (memory BTST = bit 25 = RXFUNC) instead of RXFF_RESULT (#48); ClearRexxMsg/FillRexxMsg without limit 16 (#49); index (n-1)*4 as word (n=0 = rm_Action) (#51); GetResultString leaves rm_Result2 pointing to the freed string (#53); SendRexxCommand without REXX port: alert with interrupts disabled (#55). Fixed (intended deviation, #52): GetRexxCommand/GetResultString left a3 behind the 0; a3 now ends on it (test `arexxlib-getstring-a3`). Kept, not a bug: RexxError without terminating 0 (invisible, see General; #54). DIFF: ReplyRexxMsg with a non-RexxMsg returns (original: crash; #47); -8(a2) is not copied to -4(a2). Test: t61_arexx with its own port "REXX" (priority 100), RexxMast not needed. Not tested: real ARexx host, error paths (alert waits for the mouse).

## 90 seriallib

C, declarations identical, binary = source. allio: 8 units × 256 bytes (read half +0, write half +128, each with request pointer, MsgPort, IOExtSer). BUG(orig): CloseSerial/SetSerialBuffer/SetSerialLens/SetSerialParams/SerialEvent calculate unit*128, the others unit*256 (only unit 0 consistent) (#252); ReadSerialMem reads via the write request (#253); WriteSerialString flag 2 = TrueString and DoIO at the same time (flag 1 unchecked) (#254), ReadSerial "wait" = flag 2 (#255); CloseSerial AbortIO without WaitIO, write request remains (#258); if AllocSignal of the write half fails, the device stays open (#259); "Serial Device Not Open" via TRAP #0 even without debugger (#262, the C version returns). Kept, not a bug: ReadSerialString without 0 (invisible, see General; #257). DIFF: ReadSerial leaves a3 unchanged. Test: t62_serial (unit 0 without remote station), t63_serialerr in tests-runerr, t64_serialrx in `tests/corpus/tests-serial` with remote station (`tests/corpus/runner/serial.c`: Amiberry `serial_port=TCP://…`, answering program on the host): ReadSerial without/with flag 1/with wait bit 2 (blocking), ReadSerialString with data, ReadSerialMem (via the write request), SerialEvent 1/2/both, WriteSerialString asynchronous (length -1 sends the terminating 0 along) and address form, SetSerialLens 7 bit, SetSerialParams (baud, EOF mode: has no effect on ReadSerialMem, since the write request has the old flags), SetSerialBuffer 64 with overflow (reading returns SerErr_BufOverflow 12). Result: line bytes the same in all four configurations, output the same except for the DIFF a3 (two ReadSerial in one statement: in the original the second read overwrites the string part built in rb1 in between, `97b98`; the test now reads separately, after that 0 deviations). Not tested: units > 0 with remote station, 7WIRE/handshake, parity, break.

## 158 screenslib

C, declarations identical, binary = source. Object {Screen.l, DBufInfo.l}; own screens get UserData "bLtZ", only those are closed by CloseScreen/Finit. Screen n,mode goes into the long form via a fake a2 stack (`dummydata`); width rounded up to 16, ExtNewScreen with SA_DisplayID/SA_Pens, font = current IntuiFont. ShowBitMap from V39 on with ChangeVPBitMap, before that copper list/MakeVPort (only ported, not executed). ViewPort, Show/Hide/Beep/MoveScreen, SMouseX/Y stay inline. Fixed (intended deviation, #250): in the original SA_DisplayID gets only the lower word of the viewmode (display IDs with a monitor part open on the default monitor); here the whole long (bug-orig test `screenslib-displayid-lowword`). Fixed (intended deviation, #251): the original's err1 (Can't open Screen) is a jump from normal code into the error check area, err8/err12/err13 have their messages there; here traps and messages are in normal code (bug-orig test `screenslib-err1-errorarea`). Fixed (intended deviation, #247): in the original the routines that free an object's old screen (Screen, ScreenTags, WbToScreen, FindScreen) pass a2 = their argument stack as graphics base, so with a DBufInfo (ShowBitMap on V39+) FreeDBufInfo calls into nowhere; here the DBufInfo is freed with the graphics base that allocated it (bug-orig test `screenslib-freescreen-gfxbase`). Fixed (intended deviation, #248): in the original freescreen closes every screen with "bLtZ", also one the object only found (FindScreen) that another object opened, so FindScreen/WbToScreen/CloseScreen on the finding object close it; here FindScreen/WbToScreen mark the object as found (bit 0 of its DBufInfo long) and freescreen only forgets such a screen (bug-orig test `screenslib-findscreen-closes-owner`). Fixed (intended deviation, #249): in the original ScreenTags …,&TagList copies the list without limit into the 32 longs of usertags (over the error check area, without the debugger the code of the next library); here at most 15 tags, a longer list is linked with TAG_MORE from the 16th entry on, as WindowTags (#317) (bug-orig test `screenslib-screentags-taglist-overflow`). BUG(orig): ScreenTags with tag pairs copies them without limit into the 16 pairs of usertags (#1039; bug-orig test `screenslib-screentags-pairs-limit`). Tests: t81_screen, t86–t89.

## 142 fontlib

C, declarations identical, binary = source. BlitzFont = TextFont pointer, only 8x8 and not proportional; init loads topaz 8 into object 0. BitMapOutput draws into the bitmap via handlers (-8/-4/0); BitMapInput is a line editor on the bitmap (`doinput1` stays assembler, jumps to the old handler). Not a bug (a pixel cursor by design): Locate y is Quick, CursX/CursY = cursor/8 as Quick. BUG(orig): depth 0 lets the plane loops run 65536 times (#149); a wrong font is not closed (#150); BitMapInput waits endlessly for 13 (#151). Fixed (intended deviation, #148): characters ≥ LoChar+128 were dropped (signed byte compare), now an unsigned compare (bug-orig test `fontlib-lochar-127`). Fixed (intended deviation, #955): err1-err3 trap in normal code, their messages are in the error check area in the original; here in normal code (bug-orig test `fontlib-messages-errarea`). DIFF: if the trap in calcoff returns, C draws nothing; the unused `bitmapchk` is missing. BitMapInput is only allowed in Blitz mode and not tested. Tests: t82_font, t84.

## 134 intuifontlib

C, CD binary older than the source (token `Text` missing) – the state of the binary is ported. Object = TextAttr + Font + Pad (16 bytes); init value = address of `textattr`. LoadFont copies the name with maxslib and calls OpenDiskFont; `loadchk` checks the CCR (assembler line, thunks set Z). Fixed (intended deviation, #198): in the original the name copy of a previous LoadFont is never freed; here LoadFont frees it before copying the new name (Free IntuiFont keeps it, the object stays a valid TextAttr) (bug-orig test `intuifontlib-loadfont-name-leak`). Fixed (intended deviation, #197): in the original the style goes as a word to +6 (LoadFont …,9,1 → style 0, flags 1: OpenDiskFont looked for a font of these flags); here ta_Style = the style's low byte, ta_Flags = 0 (corpus t82_font, the pair `t82_font.orig.out`/`t82_font.fixed-197.out`). Tests: t82_font, t85.

## 130 rawkeylib

C, declarations identical. `libincs:rawcode` is missing on the CD; everything except unconvert is translated from the binary. Init opens console.device and fills two tables (105 entries); F1–F10 → 129–138, cursor keys → 28–31, Help → 139, otherwise RawKeyConvert. BUG(orig) (#245): qconvertkey indexes with code & 255 beyond the tables into the lib's own code (the 151 code bytes of the original are appended as data). No dedicated runtime test (a direct ALibJsr call from BASIC already hung with the original libs); checked indirectly via windowslib (keys, Edit$ in t72).

## 138 windowslib

C, declarations identical (75 tokens). Object lib "Window" (32 bytes), one shared MsgPort for all windows, own event list (24-byte nodes). Input/output handlers for inputoutputlib with BRA table (-4 line editor, -8/-4 strings) as a short assembler block, otherwise C. Fixed (intended deviation, #318): in the original the title copies of Window, WindowTags and WTitle are never freed; here the object keeps them (+20 window title, +24 WTitle's screen title, free bytes of the 32-byte object): WTitle frees the old ones, CloseWindow and a failed open free them (bug-orig test `windowslib-title-leak`, now also with Window and WindowTags). Fixed (intended deviation, #317): in the original WindowTags …,&TagList copies the list without limit into the 32 longs of usertags; here at most 15 pairs, a longer list is linked with TAG_MORE from the 16th entry on (bug-orig test `windowslib-taglist-overflow`). Fixed (intended deviation, #320): in the original WMove/WSize busy-wait until the window has exactly the new position/size, endlessly if Intuition limits it (size limits, screen edge); here the wait also ends when Intuition changed the window, at the latest after 50 frames (WaitTOF) (bug-orig test `windowslib-wmove-wsize-endless`); a size beyond the size limits of a window with a sizing gadget (the only windows Intuition limits) is a programming error: with runtime error checking wsizechk now also refuses it, "Can't Size Window" (the original's check tested only the screen edge; bug-orig test `windowslib-wsize-limits-check`). Fixed (intended deviation, #322): in the original CloseWindow replies all messages of the shared port and discards the events of all windows; here only those of the closed window (as AmiBlitz3; bug-orig test `windowslib-closewindow-events`). Fixed (intended deviation, #310): in the original Menus/MenusOn/MenusOff go over the 32-byte objects in 16-byte steps (half of the windows not reached); here in 32-byte steps (bug-orig test `windowslib-menus-16byte`). Fixed (intended deviation, #314): in the original WCls fills from y=1 (the top pixel line stays); here from y=0 (bug-orig test `windowslib-wcls-y1`). Fixed (intended deviation, #315): in the original WScroll/WPrintScroll set the window's A pen to 0 (ScrollRaster called through the pen setting of the drawing commands with colour 0); here only the B pen and the draw mode are set (bug-orig test `windowslib-wscroll-apen`). Fixed (intended deviation, #316): in the original WindowTags without tags writes no TAG_DONE and takes over the user tags of the last WindowTags; here the user tags are ended (bug-orig test `windowslib-windowtags-usertags`). Fixed (intended deviation, #319): in the original a Window with flag $80 (super bitmap) takes the bitmap of the last Window with BitMap#, also in the forms without BitMap#; here only the form with BitMap# gives one (bug-orig test `windowslib-superbitmap`). Fixed (intended deviation, #321): in the original calcwidth (the line editor of Edit$ in a window) tests the index of a proportional font signed (SUB.b/BMI), characters from LoChar+128 on (the umlauts) get tf_XSize; here unsigned (bug-orig test `windowslib-calcwidth-proportional`). Fixed (intended deviation, #313): in the original Qualifier (#long) sets only the lower word of d0; here the long (bug-orig test `windowslib-qualifier-long`). Fixed (intended deviation, #311): a string gadget down gives an artificial "gadget up" on the next Event unless that is the real one (kept); in the original the real gadget up came as a second report when another message came in between; here it is taken without a report (bug-orig test `windowslib-stringgadget-double-up`). Fixed (intended deviation, #312): MENUPICK appends a further event for NextSelect; in the original its qualifier and IAddress are uninitialised, here those of the picked event (bug-orig test `windowslib-nextselect-qualifier`). Fixed (intended deviation, #961): WindowTags with tag pairs copies them without limit into the 16 user tags (an invalid argument, the command unchanged); the new error check `wtagschk` (debugger only) stops more than 15 pairs with "Too Many Tags", the original has none (bug-orig test `windowslib-windowtags-pairs-limit`; declarations: `declarations-intended.txt`). Not a bug: Event compares classes as word (IDCMP classes are single bits, same result). DIFF: err6/err7 from normal code; a3, a4, d7 and the argument stack stay unchanged. Tests t70–t72 (events as self-built IntuiMessages before waiting, Edit$ with keys queued in advance and timer watchdog), t75/t76/t78/t79. Not tested: real Intuition input, SetEventFilter, commands with shapes/bitmaps, windows with gadget lists.

## 136 menuslib

C, declarations identical. Object lib "MenuList" (4 bytes). Not a bug, by design (#416): without runtime error checking wrong menu/item numbers walk over the end of the lists. Fixed (intended deviation, #216): MenuState on a single menu set bit 4 instead of MENUENABLED, now MENUENABLED (bug-orig test `menuslib-menustate-menu-bit`). Fixed (intended deviation, #217): MenuChecked with a sub-item jumped back into the menu loop (only 0/0 correct), now it walks the lists as MenuState with a sub-item (bug-orig test `menuslib-menuchecked-sub-loops`). Fixed (intended deviation, #218): ShapeItem/ShapeSub took their flags from the upper word of the longword, now from the whole long as MenuItem (bug-orig test `menuslib-shapeitem-flags-hiword`). Fixed (intended deviation, #219): the flag test was a word comparison after a long test ($10000 gave the flags of 4), now a long comparison (corpus `t73_menus`, pair `[fixed #219]`). Fixed (intended deviation, #220): replacing a SubItem shifted the following sub-items by (number − old height), now by the height difference (bug-orig test `menuslib-subitem-replace-shift`, corpus `t73_menus`). Fixed (intended deviation, #981): an empty text of a menu, item or subitem was stored as 0 (allocst of length 0), read at address 0; now copied with length 1, the empty string, as AmiBlitz3 does (bug-orig test `menuslib-empty-item-text`). Not bugs: a shortcut sets the flags to 4 before the OR (the intended result); MenuTitle width = 8·length + 8 (a limitation noted in the original source). Tests t73, t71, t77.

## 108 mouselib

C, declarations identical. Almost everything only in Blitz mode (err1 = 1). BUG(orig): Mouse On swaps the JOY0DAT bytes (#223). Not bugs: the interrupt leaves a5 in place (a scratch register for interrupt servers); the sprite number of Pointer is object offset >> 4 (offset = number * 16, correct). Test t74 (Blitz mode without Mouse On; interrupt and Pointer with sprite not testable).

## 162 iffiolib

C, declarations identical, binary = source. No tokens of its own; routines for other libs (ilbmifflib, animlib, audiolib, palettelib, lib 154): +1 iffopen (a0 = name), +2 iffclose, +3 iffbyte, +4 iffword, +5 ifflong, +6 ifffind (d0 = ID → chunk length), +7 iffskip (d0 = number of **bytes**, the source says "long words"), +8 iffread (d0 bytes to a0). init (a0 = dos) gets the 2048-byte buffer via globalloc, init value = buffer address. Result in the CCR (Z = ok, otherwise d0 = -1/N), iffbyte/iffword replace only d0.b/d0.w; the thunks set the CCR via `tst.w` from a status variable. All registers except d0 (iffread: a0) are preserved. BUG(orig): charsleft = lower word of the Read result (read error → 65535, buffer contents count as data) (#177); ifffind aborts at "BODY" and reads byte by byte (#178); iffskip/iffread with 0 count from 2^32 (#179). Not a bug (all callers ignore d0/a0 afterwards): iffread reads from the end of the buffer via Read directly to a0 (a0 not advanced). Tests indirectly via t100 and t103.

## 161 ilbmifflib

C, declarations identical, binary = source. For other libs: +1 ilbminit (a0 = name → d0 = -1/0, a0 = iffstuff, file stays open), +2 ilbmgetbyte (ByteRun1-decoded), +3 ilbmbitmap (a0 = name, a1 = BitMap). ViewMode: pageWidth ≥ 480 → $8000, pageHeight ≥ 300 → |4. BUG(orig): ILBMInfo closes the file even if it was not opened (Close(0)) (#186); ilbmbitmap ignores a failed ilbminit (#187); DecodeILBM does not round up chunk lengths (#189), writes all planes of the ILBM via the plane pointers of the BitMap (also behind Planes[]) (#190), does not clip ByteRun1 runs at the end of the line (#191). Fixed (intended deviation, #188): ilbmbitmap stores a plane while its number is below the byte Depth (BitMap+5); the original compared with the word BitMap+4, Flags<<8 | Depth (bug-orig test `ilbmifflib-plane-word`). Fixed (intended deviation): the original has the error routines in normal code but their messages in the error check area (without the debugger d0 points behind the library); here the messages are in normal code too (#192, test `bug_orig_ilbmifflib_messages_errarea`). Not a bug (word functions): ILBMWidth/Height/Depth load only d0.w. Tests: t100, t103.

## 114 iffmakelib

C, declarations identical, binary = source. The four commands are only allowed in Amiga mode: odd code address in !subs (blitz2.s doajsrrout), in C `AMIGAONLY()`. ILBM: BMHD, CMAP (upper bytes of the 32-bit values), BODY per line and plane ByteRun1, lengths filled in afterwards via Seek. SaveScreen: Exec ≥ 39 → GetRGB32 per colour, otherwise ColorTable 4 bit → 32 bit. BUG(orig): line/plane loops run at least once (#181); savearow reads one byte behind the line (#183); CMAP with odd length without padding byte (#184); palette with 0 colours → 65536 colours (#182); SaveShape does not check the depth (#326) (depth 0: 65536 plane pointers from `bmcopy` on, depth ≥ 15: beyond its 14 pointers into `usewid`/`scrcols`). Fixed (intended deviation, #180): the plane loop compares with the byte Depth (BitMap+5), as nPlanes; the original compared with the word BitMap+4, Flags<<8 | Depth (bug-orig test `iffmakelib-plane-word`). Fixed (intended deviation, #185): "Can't open File" traps in normal code, its message is in the error check area in the original; here in normal code (bug-orig test `iffmakelib-openerr-errarea`). Tests: t100, t104, t107.

## 132 brexxlib

C, declarations identical, binary = source. init opens input.device and installs an input handler with priority 51 (C function, calls memlib globalloc from the input.device task like the original). Tapes = lists of InputEvents (22 bytes). BUG(orig): TapeTrap without a number falls through into TapeTrap Tape# (#95); appending to a tape clears mode (#96); ClickButton passes the d0 of the first call to the "release" (under TapeTrap with QuietTrap On "press left, release RIGHT", proven in the original) (#97); AbsMouse and RelMouse both generate POINTERPOS (#98); PlayWait restarts the tape (#99); Type: unknown character → code $FFFF (#102); XStatus contains bit 15 (#103). DIFF: "Can't Open File" (SaveTape/LoadTape) in normal code instead of a jump into the error check area. Tests: t101 (events only on tapes), t106.

## 117 blitzoslib

C, declarations identical, binary = source (code compared by hand with source and disassembly). Error check field `1` in !subs = Blitz mode only; allowed in Amiga mode are only InitPage, FreePage, AddBox, SetBox Box#,Value, SetBoxString, SetPagePens, ChangeBox. `!libs …,$0604` = d6 = offset of the object variable + 4. Drawing via BLibJsr to 2dlib, fontlib, scrolllib, shapes, mouselib, blitzkeyslib (macro BCALL). BUG(orig): SelectBox Box# unchecked (#82); SpotBox without page check (#83); DeSelectBox does not clear `selected` (#94). Fixed (intended deviation, #84): a string box 8–15 pixels wide filled 65536 spaces behind its 1-byte text; the text is now just its 0 (bug-orig test `blitzoslib-addbox-string-narrow`). Fixed (intended deviation, #81): box numbers were shifted and added as words (ASL.w #5/ADDA.w, BoxSelected LSR.w), wrong from box 1024 on; now longs (bug-orig test `blitzoslib-box-number-word`). Fixed (intended deviation, #85): BoxString stripped trailing spaces backwards without a limit (an all-space answer copied up to 65535 bytes); it now stops at the start of the answer (bug-orig test `blitzoslib-boxstring-all-spaces`). Fixed (intended deviation, #89): AddBox kept the pointer of a freed text, and a box with the text length 0 was printed from address 0 with that pointer as the length; AddBox now clears the pointer and such a box prints nothing (bug-orig test `blitzoslib-addbox-old-text`). Fixed (intended deviation, #90): BlitzEvent read the type of the selected box on cursor up/down also with none selected (address 8); it now skips the list test then (bug-orig test `blitzoslib-blitzevent-noselect`). Fixed (intended deviation, #91): in input mode, Backspace at position 0 and the control characters other than Backspace and DEL left the cursor off; it is drawn again (bug-orig test `blitzoslib-backspace-cursor`). Kept, not a bug: BoxString without 0 (invisible, see General; #86); BoxPrint overwrites the string length -4(a2) with the scroll size (the argument area belongs only to this call; #88); BlitzEvent's BMI after CMP.b (same result as an unsigned compare; #92); SelectBox redraws the old box (only extra time; #93). DIFF: coordinates to Boxf/Line/Locate as sign-extended words. No emulator test of its own in the library test suite yet. The crashes seen with the original lib (a Blitz mode test and an error path test) were compiler bug #335: blitzoslib reaches shapeslib only through its drawing code, and the compiler gave that object library no array (fixed in blitz2-c, commit 124f83ae, test `compiler-maxs-libjsr`); with it fixed an emulator test of blitzoslib is possible (bug-orig tests that compile and run blitzoslib programs exist: `tests/bug-orig/libs/blitzoslib-*.bas`).

## 116 audiolib

C, declarations identical, binary = source (the VBlank code between the two CELSE is not in the binary). Sound object {data, period, length in words, repeat position, repeat length, "cL"}. Everything goes directly to Paula; Sound/LoopSound switch off the DMA and wait 12 line changes of VHPOSR. IFF 8SVX is read by iffiolib (errors in the CCR, local macro `IFFCALL`). Period = 3579440 / samplesPerSec with DIVU (overflow stays). BUG(orig): sndchk checks, for SetPeriod, the period as channel mask (period > 15 → "Illegal Chanel Mask" with debugger, t115) (#66); LoadSound with a too long sample leaves the file open (#67); DiskPlay never ends if the file length is a multiple of the buffer length (#69); DiskPlay allocates its chip buffer before the open and does not free it on err1–err3 (lost while a SetErr program goes on, memlib frees it at the end) (#950). Fixed (intended deviation): in the original SetPeriod without a sound branches to err5 in the error check area, and the texts of err1–err4 (normal code) lie there too; here err5 and the texts of err1–err5 are in normal code (#70, test `bug_orig_audiolib_setperiod_err5`). Fixed (#68): freeing a sound clears the "cL" mark of DecodeSound's in-place data, so a later sample in that object (DecodeSound of fast memory, LoadSound, InitSound) is freed again (test `bug_orig_audiolib_decodesound_cl`).

**Compiler/lib interplay (fixed in the compiler, #46, important for AmiBlitz3):** object functions like PeekSound load the object into a3 via `!libs …,$1380` – but a3 is also the string pointer. In the original, such a call in the middle of a string expression (`s$=s$+Str$(PeekSound(0,i))`, `NPrint "x",PeekSound(..)`) made the rest of the expression write its strings into the object ("Unable to Free Memory", without debugger Guru 8000 0020). blitz2's libfunction now saves a3 around such a call inside a string expression (`docs/libs/deviations.md` section 5, test `compiler-libfunc-a3-string`). The corpus tests use such functions only in simple assignments. DiskPlay not tested (busy-waits for audio interrupts). Tests: t110, t114, t115, t119 (only zero data or volume 0).

## 96 trackerlib

C, binary = source. NoiseTracker player (mt_init/mt_music/mt_end) in C; mt_music in the VBlank interrupt via intlib $C100/$C101 (Blitz/Amiga variant). The two DBF wait loops and the prefixes stay assembler. mt_init clears the first long of each sample. BUG(orig): mt_init checks positions 0–126 (127 never); a new maximum at position 126 lets the counter run from 0 to $FFFF (search ~64 KB further, `CLR.l` behind the module) (#289); the volume word contains the finetune byte (#290); repeat × 2 as word (#291); after D/B the song end is compared with the first pattern byte (#292); Cxx writes the volume as byte (#293); sample numbers > 31 read behind the table (#295); LoadModule of a missing file does not free the FileInfoBlock, for a drawer additionally not the lock (#296). Fixed (intended deviation, #957): "Error Accessing Module File" traps in normal code, its message is in the error check area in the original; here in normal code (bug-orig test `trackerlib-err1-errarea`). Tests: t111 (a generated M.K. module, `tests/corpus/data/t111_song.mod.spec`: checksums before/after mt_init, Play/Stop immediately), t116, t117.

## 82 medlib

C, binary = source (MED V3 player by T. Kinnunen; SYNTH, RELVOL, IFFMOCT, HOLD, PLAYMMD0; no interrupt, PlayMed = one tick), plain C with structs for the module and the track structures (until 2026-10-31 translated at register level; the register values the results depend on are kept explicitly: the qualifier word of a line, the d1 _PlayNote leaves). Data base, 16 period tables and two jump tables from the binary in the original layout. BUG(orig): Bxx stores the whole word of d4 (#203); 7xy with x = 0 jumps into the vibrato code (#204); with 1Fxx the retrigger nibble is lost (#205); the synth period limit sets d1 instead of d5 (#206); relocation after a MIDI dump reads the next module pointer from (expansion address AND $FFFF0000) (#207); pushnewvals with a gappy channel mask: higher channels get no follow-up values (track/bit stay paired) (#208); tracks 4–15 write beyond their 20-byte structure (#209); JumpMed without module writes to address 44–47 (exception vector 11) (#210); GetMed* read and clear (#212). DIFF: LoadMedModule and SetMedVolume do not change a3. Tests: t112 (a generated MMD0 module, `tests/corpus/data/t112_song.med.spec`, 400 + 3 x 60 + 120 ticks), t118, libs/tests/systest_med.c (generated MMD0/MMD1 modules with synth, hybrid and IFF octave instruments, the whole player state compared with the original after every pulse).

## 88 speaklib

C, binary = source. narrator_rb byte-exact (88 bytes, V37 part at an odd address = VoiceLoc). If translator.library or narrator.device is missing, "Failed To Open Speech Library" occurs even without debugger (TRAP in normal code). BUG(orig): Translate$ writes the phonemes over the argument (length with terminating 0, a3 behind it) (#277); Finit closes device and signal even after a failed init (#278); SetVoice limits with word comparisons (#279). Not a bug (a statement): PhoneticSpeak leaves a3 on the argument. In the emulator (OS 3.2) translator.library and narrator.device are missing → t113 (tests-runerr) checks only the init error; Translate$/SetVoice/VoiceLoc not comparable.

## 140 gadgetslib

C, declarations identical, binary = source. Object lib GadgetList (4 bytes), Gadgets/IntuiTexts/Borders/StringInfo/PropInfo from memlib, texts via maxslib. Host comparison (unicorn) against the original: OS calls and heap byte-identical; t90_gadgets the same in all four configurations. Fixed (intended deviation, #163): freegadget never freed the SpecialInfo (type overwritten by `MOVEQ #22`), now it frees StringInfo, buffer and PropInfo (bug-orig test `gadgetslib-specialinfo-leak`). Fixed (intended deviation, #164): StringGadget with Borders Off wrote its border offsets to 0/2/36/38, now only into a border (bug-orig test `gadgetslib-stringgadget-noborder`). Fixed (intended deviation, #165): StringGadget with Maxlen 0 clears 64 KB behind its buffer and the original has no check routine for it; the new check routine stringchk gives "Maxlen MUST be greater than zero" (bug-orig test `gadgetslib-stringgadget-maxlen0`); By design (#166): without runtime error checking findgadget does not stop at the end of the list. Fixed (intended deviation, #952): SelectMode only stored selmode, which nothing read: no effect, gadgets always got defflags = 0 (GADGHCOMP); now it sets defflags (GADGHBOX for 1), a shape gadget with an alternative shape keeps GADGHIMAGE (bug-orig test `gadgetslib-selectmode-noop`). Not a bug (word functions): GadgetStatus/ButtonId with a foreign upper word.

## 141 gtlib (`#mygadtoolslib`)

C after the CD binary (the source is a different state; DetachGTList, GTGimmeCludge, GTSetHighlight, GTFreeGadget only in the binary). Tag lists static in the original layout, hence the same peculiarities; not a bug (#170): GTBevelBox gives GTBB_Recessed the value 0, but DrawBevelBoxA takes the presence of the tag, so the box is recessed. Fixed (intended deviation, #173): GTShape copied its static image over the GadTools button's GadgetRender and shared one static SelectRender image; now each GTShape gadget has two images of its own (in the library's block list), GadTools' render pointers are put back before FreeGadgets (bug-orig test `gtlib-gtshape-image`). Fixed (intended deviation, #168): the GTPalette colour ended up in the low word of the TAG_MORE pointer (+38 of an older layout); now in GTPA_Color, the form without it sets GTPA_Color 0 (bug-orig test `gtlib-gtpalette-tagmore`). Fixed (intended deviation, #169): the optional values (cycle/mx active, listview top/selected, scroller top, slider level, string default) stayed in the static tag lists for a later call without them; now the forms without them set the initial values (bug-orig test `gtlib-optional-values-stay`). Fixed (intended deviation, #327): GTShape went on with the result of a failed CreateGadget (writes to addresses 12-25); now it leaves a failed gadget alone (bug-orig test `gtlib-gtshape-creategadget-null`). Fixed (intended deviation, #174): the Cycle/MX labels had room for 63 without a check and were never freed; now the block is sized by counting the `|`, kept in a list of the library (GTList, gadget) and freed with the gadget (GTFreeGadget) or the GTList (bug-orig test `gtlib-getoptions`). Fixed (intended deviation, #328): GTGetAttrs/GTSet…/GTChangeList/GTSetHighlight left a3 on their tag list (GadTools takes it in a3), so the rest of a string expression was written over it; a3 now comes back as it was at the call (bug-orig test `gtlib-a3-tag-list`). Fixed (intended deviation, #171): GTButton with the toggle flag set TOGGLESELECT in the result of a failed CreateGadget (address 14); now only in a gadget (bug-orig test `gtlib-button-toggle-null`). Fixed (intended deviation, #990): GTShape replaced the gadget's Flags and Activation after CreateGadget, so its flags 64 (disabled) and 128 (immediate) had no effect; now GFLG_DISABLED and GACT_IMMEDIATE stay (bug-orig test `gtlib-gtshape-flags`). Fixed (intended deviation, #172): GTShape left the label of the previous gadget in the NewGadget, GadTools got it for the shape gadget; now it has none (bug-orig test `gtlib-gtshape-label`). Fixed (intended deviation, #962): GTFreeGadget of the last gadget left the GTList's last gadget pointer on the freed gadget, the next gadget was linked into freed memory; now the gadget in front of the freed run is the last one (bug-orig test `gtlib-freegadget-current`). Host comparison identical; no emulator test: the original run crashed at program end (Guru 8100 0005), cause not found.

## 80 asllib (`#myasllib`)

C, declarations identical; the binary differs from the source (additional font tag ASL_TB+17=256, pattern "" instead of 0, freeing with FreeFileRequest -36) – the binary is the reference. Fixed (#60): ASLScreenRequest returns a static copy of the requester's fields made before it is freed (the original returned the freed requester; test `asllib-screenreq-freed`). Fixed (#61): ASLPathRequest$ has no initial file and copies fr_File into the result only (the original copied it to the address in d2, with dimensions x; test `asllib-pathreq-d2`). Fixed (#63): the title and pattern copies (maxslib allocst) are freed after every request (the original never freed them; test `asllib-title-leak`). Fixed (#62): without asl.library ASLFileRequest$/ASLPathRequest$ return the empty string (the original: the title address as the length; test `asllib-noasl-title-len`). Fixed (#64): ASLPathRequest$ without dimensions clears the pattern flag (the original kept that of the previous ASLFileRequest$; test `asllib-pathreq-pattern-flag`). Fixed (#403): FreeFileRequest only for an allocated requester (the original also with 0; test `asllib-free-null-request`). Host comparison identical; no stable emulator test (closing task in the BASIC test not reliable).

## 152 freqlib

C: wrapper from the source, the requester (libincs:filereq.s, not on the CD) from the disassembly; its data as a data block with the original offsets (bytes + relocs from the binary), meanwhile as `struct fq_data fq` (NDK structures, variables, buffers; every offset checked via `static_assert` against the address in the binary; data block byte- and reloc-identical, code shorter because GCC saw the assembler block as a byte array with alignment 1; host comparison unicorn 27 scenarios the same). **Split (for TED, "Variante C"):** the requester core (code, data block `fq`, parameter block `freqst[4]`, entry `freqcode()`) is in `shared/filereq/filereq_core.c` (a separate translation unit), without blitzlib.h; `freqlib.c` is now only the BlitzLib shell (`dofreq`/`freqchk`) and defines `fq_divu_w` (= `bl_divu_w`) before the `#include`. The same file is included by `programs/ted/src/filereq.c` (TED's embedded hunk `filereq` is the same code). What the includer has to provide beforehand is stated at the head of the .inc. `freqlib.obj` is byte-identical after the split (`cmp` against the state before; `biglib.py compare` with ref: declarations identical). Host comparison identical in 7 scenarios. Fixed (#153): on OK the wrapper sets the lengths of path$ and file$ to those of their new contents (the original left the old lengths; test `freqlib-lengths`). The FileInfoBlock must be longword-aligned (BPTR) – explicitly aligned in C (the first emulator run hung because of this). Fixed (#160): the original's window shows (height − 85) / 8 lines without limit, but there are only 64 line gadgets (up to the end of the data block $39DA); from window height 605 on (first window = screen height − 40, so every screen from 645 lines on) its layout writes the further lines behind the data block into the following program. Measured in the host harness (screen 1050: window 1010, 115 lines, ~7.2 KB behind $39DA; screen 256: nothing); stack, a2–a6, pr_WindowPtr, locks and memory are in order on return. On the RTG Workbench (1680×1050) of the test emulator the program therefore crashed directly after the requester (Cancel as well as OK, t93/t94 with reqclose: reboot, earlier "ILLEGAL Instruction"). The port shows at most 64 lines, its window is at most 597 high (the first one too, and MaxHeight); test `freqlib-64-lines` (RTG variant of the emulator, a 1024×2048 screen). Fixed (#159): the list nodes have the size of their name (the original's 40-byte nodes were overrun by names over 34 characters), an entry without memory is left out (the original did not check the AllocMem) (tests `freqlib-node-overrun`, `freqlib-node-allocfail`). Fixed (#345): the CD gadget cleared the upper word of the UndoBuffer pointer of the path gadget ($111A instead of DispPos), so Intuition later copied the path into the first 64 KB; it now clears BufferPos and DispPos (test `freqlib-cd-undobuffer`). Fixed (#154): dos, intuition and graphics, opened at every call, are closed again at its end (test `freqlib-libs-not-closed`). Fixed (#156): the volume list walks the DOS list locked for reading (LockDosList; Forbid before dos V36), the original without a lock (the labels <ASN>/<VOL> are correct: `dl_Type` 1 = DLT_DIRECTORY → "<ASN>", 2 = DLT_VOLUME → "<VOL>", #155; test `freqlib-doslist-unlocked`). Fixed (#158): building the path from the current directory unlocks the ParentDir locks it takes (test `freqlib-parentdir-leak`). Fixed (#157): the CD gadget still makes the path the current directory, but keeps the one before the first CD instead of unlocking it (the shell's lock); a new finit of freqlib (`fq_finit`; TED calls it at its end) makes it current again and unlocks the requester's lock (tests `freqlib-cd-currentdir`, TED `ted-freq-cd-start-lock`). Fixed (intended deviation, #161): the wrapper `dofreq` copied the result with its 0 and left a3 behind it; a3 now ends on the 0 (test `freqlib-a3-behind-0`; only the wrapper, not the requester TED shares). Fixed (#1042): names and paths were copied into the path buffer (160 bytes) and the file buffer (64 bytes) without a limit (start strings, the path of the current directory, PARENT's "/", directory, volume and file clicks); a file name of 64 or more characters ran into the result buffer, and OK then copied without end. Now whatever does not fit 159 / 63 characters or the MaxLen of Pathname$ / Filename$ (the wrapper passes them in `fq_maxlen`) is refused with "Path too long" / "Name too long" in the screen title bar, and the string gadgets' MaxChars follow the same limits (tests `freqlib-file-name-unbounded`, `regression/freqlib-names-refused`, `regression/freqlib-names-limits`). Tests t93 (Cancel) and t94 (OK) in `tests/corpus/tests-req` via reqclose, therefore on a separate 320×256 screen (window 216, 16 lines; in the harness without overflow, Cancel as well as OK).

## 160 bitmaplib

C, declarations identical, binary = source. Object lib "BitMap" (64 bytes: struct BitMap + extra fields +52 interleaved, +54 plane distance, +56 width, +58 height, +60 colours, +62 0/1 borrowed/-1 own). BUG(orig): sizes as MULU word products (#76). Not a bug: CopyBitMap without setting Used, chip memory not cleared (every word is overwritten by the copy). Kept, not a bug: CludgeBitMap leaves the plane distance +54 in place (no library of the CD reads +54 of a non-interleaved bitmap; #77). By design (#393, #74): BitMap with depth 0 stores 65536 plane pointers behind the object; its error check refuses depth 0 or more than 8. Fixed (intended deviation, #393, #74): CludgeBitMap had no error check; an error check of its own (error check area only) now refuses depth 0 or more than 8 with "Illegal BitMap Depth" (bug-orig test `bitmaplib-cludgebitmap-depth-zero`). Tests t120, t124, t125.

## 156 palettelib

C after the CD binary (newer than the source: Red/Green/Blue #long, make4bitrgbs takes the lower byte – fixed here, see below –, AGARed/Green/Blue check the screen and mask the colour number). Object lib "Palette" (128 bytes, data in LoadRGB32 format, 14 cycles). Blitz/Amiga variants ($a001). RGB in Amiga mode jumps directly into SetRGB4 like the original (a1 unchanged): with a1 = ViewPort the graphics.library of the emulator stored the value $8C instead of $88 for green 8 (t122, diagnostic line "raw") – an example of how an "unused" register changes the result of an OS call. Fixed (intended deviation, #224): DecodePalette sets only the upper byte, the CD binary's make4bitrgbs (agamode 0, Kickstart < 3.0) takes the lower byte: black; here make4bitrgbs takes the upper byte (test `palettelib-make4bitrgbs-lowbyte`). Fixed (intended deviation, #1002): DecodePalette and LoadPalette compared the CRNG flags word with 1 only, so a disabled range (flags 0 or 2) was entered and cycled backwards; here bit 0 (RNG_ACTIVE) decides whether a range is entered and bit 1 (RNG_REVERSE) its direction (test `palettelib-crng-flags`). BUG(orig): PaletteRange with start=end divides by 0 (#227); StopCycle does not reset cycon (#228); InitPalette without cycle end marker (#229). Not a bug (the results are right): FadePalette/MixPalette read 2 bytes too early. DIFF: runtime errors from normal code instead of a jump into the error check area. Tests t121–t123, t126–t129.

## 157 fadelib

C, declarations identical (`libheader{157`). Blitz mode only; 16 steps on the copper colour moves of the used slice. BUG(orig): `calcfade` shifts the whole word d0 with `LSR #4` (at the call palette# × 128, the compiler's object address): bits 8–11 end up in the red, bits 12–15 in the green fractional part of the **first** colour, after that the upper byte is empty (palette 0/1 not affected; reproduced register-exactly since the correction, before that every fractional part got the bits in C) (#134); the checks test the upper word of the pointers (#135); wrong messages: an uninitialized palette gives "Not enough colours in Slice" (`err2`, `mess3` "Uninitialized Palette" is never used) (#330), low/high beyond the slice's colours "Uninitialized Slice" (`err1`) (#331). Test t123 (ASyncFade On + DoFade, without timing; only palette 0, does not cover the bug).

## 150 blitlib / 146 bblitlib / 148 qblitlib / 122 stencillib

C, declarations identical, binaries = sources (+ include bliterrs). The routines program the blitter directly (volatile accesses in the order and width of the original); Blitz/Amiga variants ($a001): Amiga mode with OwnBlitter/wait/DisownBlitter. bblitlib saves the background into records in the buffer (UnBuffer, ShowStencil via stencil as B), qblitlib remembers the blits (UnQueue clears or copies from bitmap), stencillib = OR of all planes or cookie added (SBlit). BUG(orig): no clipping (only the checks), depth 0 = 65536 DBF iterations: such a bitmap or shape is an invalid argument, refused at its creation in the error check area (BitMap, and since the fix of #74/#243 CludgeBitMap and InitShape), by design (#73, #423): overflow of Buffer/Queue is tested only with runtime error checking, FreeBuffer/FreeQueue leave the write position in place (#75, #244), stencil bltsize only up to 1024 rows (#284). Not bugs: Block at byte positions (word boundary blitting, as documented), BlitColl over all bitmap planes, FreeStencil leaves the memory pointer (the width is cleared). DIFF: b_doblitex does not return its register state to other libs; BlitColl in Amiga mode with upper word 0. Tests t130–t134 (t134 in Blitz mode), t135–t139 (tests-runerr). Note on test construction: `BitMap n,…` makes the new bitmap the used one – a BBlit with more planes than the bitmap writes, without error checking, via plane pointer 0 into low memory.

## 154 shapeslib

C, declarations identical, binary = source (incl. ANIM loader). Shape block 32 bytes (+0 width … +14 data CHIP, +18 cookie, +22/+24/+26/+28 sizes, +30 "cL" = data not owned). GetaShape (Amiga: OwnBlitter around the Blitz routine) lets bitmaplib create a bitmap and copies into it via scrolllib. LoadShape reads ILBM via ilbmifflib/iffiolib; every "ANHD" (op 5, interleave 1) yields a delta frame in the NEXT shape object. BUG(orig): MULU low words and ADDA.w steps (#263); BODY unchecked (#264); ANIM frames without maximum check (#265), XOR bit only from the first ANHD (#266); Fixed (intended deviation): LoadShapes/DecodeShapes computed the last shape as a word offset (`(max-1) << 5`, before the table from a maximum of 1025 on), now a long offset (#999, test `bug_orig_shapeslib_range_word_offset`). DecodeShapes with address 0 branches in the original into the error check area (err10); here err10 and its message are in normal code (#270, test `bug_orig_shapeslib_decodeshapes_null`). Likewise the messages of err2/err3/err7/err8 (normal code: SaveShapes, LoadShapes, LoadShape), in the original in the error check area (#953, test `bug_orig_shapeslib_messages_errarea`). Fixed (#269): freeashape clears the "cL" mark too, so a later shape in that object is freed again (test `bug_orig_shapeslib_freeashape_cl`). Fixed (#268): CopyShape clears the "cL" mark of the copy, which owns its data (test `bug_orig_shapeslib_copyshape_cl`). Fixed (#267): LoadShapes, and DecodeShapes of memory above 2 MB (which copies the data), clear +30 of the block they take from the file, which holds "cL" when it was saved from a DecodeShapes shape (test `bug_orig_shapeslib_loadshapes_cl`). Not bugs: only `depth` DLTA pointers (one per plane); DecodeShapes stops silently at the first non-matching block, in place below 2 MB (by design); ShapeWidth/Height load only d0.w (word results). Fixed (intended deviation, #243, #74): InitShape had no error check, depth 0 made a shape whose blits run 65536 planes; an error check of its own (error check area only) now refuses it with "Illegal Shape Depth" (bug-orig test `shapeslib-initshape-depth-zero`). Tests: t140, t144 (ANIM generated in the test), t145/t146/t149.

## 118 shapetrixlib

C, declarations identical, binary = source; `libincs:qrot` is missing on the CD, the sine table comes from the binary. XFlip shifts each plane with the blitter and then mirrors bytes and bits via CPU; YFlip swaps lines; Rotate (Quick 0..1 = 256 steps, back-rotation of each target pixel); Scale (size via lmullib, step via ldivlib, per target pixel the colour with the largest brightness sum). BUG(orig): XFlip does not wait after the last blit before the CPU mirrors – the result depends on timing (blitter against CPU); with the faster C code a 1-word shape differed, this case is removed from t141, the C version does not wait, like the original (#271); Scale reads beyond the shape at the edge (#273); word arithmetic for row/plane steps and sizes (#274). Fixed (intended deviation): Rotate rounded negative coordinates with -$8000 (one further from zero, the rotated shape one pixel too large); now +$8000 for both signs (#1004, test `bug_orig_shapetrixlib_rotate_rounding`). Kept, not a bug: the brightness table keeps old values (the colour choice compares only 0..numcols; #272). Test: t141.

## 112 spriteslib

C, declarations identical, binary = source. Sprite block 16 bytes; SpriteMode 0/1/2 = 16/32/64 pixels. ShowSprite/InFront/InFrontF/InFrontB Blitz mode only (odd error check address), not tested. BUG(orig): sizes when freeing/saving with the CURRENT SpriteMode (harmless, memlib takes the stored length) (#280); SpriteMode > 2 reads behind the width table (#281); GetaSprite reads whole sprite widths (#282); ShowSprite clips at the top with 4 bytes per line and adds the whole long register (#283). Fixed (intended deviation): LoadSprites computed the last sprite as a word offset (`(max-1) << 4`, before the table from a maximum of 2049 on), now a long offset (#999, test `bug_orig_spriteslib_loadsprites_word_offset`). The messages of err10/err11 (normal code: SaveSprites, LoadSprites) lie in the original in the error check area; here in normal code (#954, test `bug_orig_spriteslib_messages_errarea`). Tests: t142, t147.

## 104 collslib

C, declarations identical, binary = source. ShapesHit/ShapeSpriteHit/SpritesHit/RectsHit: rectangle test with signed word comparisons. The test (`chkifhit`, collslib.c:198-205) is asymmetric: it uses `<=` for x1+w1 against x2 and for y1+h1 against y2, but `<` for x2+w2 against x1 and y2+h2 against y1. Rectangles that only touch (e.g. A at x 30..49 and B at x 10..29) are therefore a hit when B lies left of or above A and no hit when B lies right of or below A, so `ShapesHit(A,B)` and `ShapesHit(B,A)` (likewise `RectsHit`) can differ for touching shapes. Kept as the original behaviour (possibly intended: it guarantees a gap on one side), not marked BUG(orig), not changed; locked in by t143 (`tests/corpus/expect/corpus/t143_colls.out`). SetColl/SetCollOdd/SetCollHi write CLXCON and read CLXDAT. SColl/PColl/DoColl Blitz mode only, not tested. BUG(orig): SColl calculates the CLXDAT bit number (BTST on the upper byte, bits 9–14 = sprite pairs, intended) correctly only if channel 0/1 is not the second argument: SColl(2,0)/(4,0)/(6,0) test bit 10/11/12 instead of 9/10/11, two channels of the same pair a foreign bit (#110); By design (#111): SetColl with 0 planes runs 65536 times; its error check refuses it ("Illegal number of BitPlanes"). Tests: t143, t148.

## 100 scrolllib / 119 2dlib / 121 polylib

C, declarations identical to the CD binaries. scrolllib = source except for clsnotint (binary: long operations when splitting the plane size, taken over). 2dlib = source `2dlib` (not sis2dlib.bb). polylib = source polylib (#chunkylib=121; renderlib is the same file with 87). 2dlib and polylib are plain C with the integer semantics of the original (wrapping word arithmetic, MULS/DIVS as instructions, CMP/BMI where the original tests the sign of a difference, the edge positions as its swapped fixed point numbers). Blitter routines are Blitz/Amiga variants ($A001); polylib never owns the blitter (like the original). The fill loop of FloodFill (yofill) is the original assembler, because it keeps the open lines unbounded on the 68000 stack. All routines leave a3 like the original.

BUG(orig): Box adds the bitmap origin twice (#391); Circle/Circlef with points above the top edge: MULU of the negative y, Plot far outside (#392); Poly with 1 point: 65535 iterations (#396); Polyf/BitMapPoly: overflow with more than 16 points or more than 10 edges per side (#395); the statement form of AllocFill takes a3 = string buffer as bitmap (#390); FreeFill leaves the buffer pointer in place (#394); depth 0: 65536 or 256 plane iterations, such a bitmap is an invalid argument refused at its creation in the error check area (BitMap, CludgeBitMap since the fix of #393); polylib: no orientation check (#419), Fixed (intended deviation, #1035): the dither form stepped the dither bitmap per plane by 2 x the target's plane distance - its own (planes 1 and up read behind a dither bitmap of smaller planes), now by its own plane distance (bug-orig test `polylib-dither-plane-step`), RenderToBitmap b,d + RenderPoly swaps the bitmaps (#421), RenderPoly without RenderTo… draws on a3 = string buffer (#420), RenderToChunky colours > 14 read behind the colour table (#422), ChunkyVlin without CLIP with y2 < y1 writes about 64K words (#417); BitMapPoly/ChunkyPoly/RenderPoly with an edge of 4096 lines or more that is at least as wide as high, or with points 32768 or more apart, draw a wrong shape or nothing (word arithmetic; won't fix, #1019, test `polylib-word-range`). Fixed (intended deviation): Polyf computed the slopes, the clipping and the orientation of the polygon with word arithmetic and drew nothing (or a wrong shape) for vertices far outside the bitmap (about +-8000 and beyond); a polygon of 3 to 16 points outside the range in which that arithmetic is exact (edges up to 8191, points up to +-16383) is now clipped to the bitmap with long arithmetic first, and the orientation compares exact products (#1000, test `bug_orig_2dlib_polyf_word_range`). A side of Polyf, BitMapPoly, ChunkyPoly or RenderPoly that starts beyond the clip edge on a vertical first edge is not cut there, and the polygon is not drawn (the original divides by the dx of 0 of that edge: "GURU! - Division by Zero"; #1033, tests `bug_orig_2dlib_polyf_vertical_cut`, `bug_orig_polylib_vertical_cut`); the clipping of #1000 can make such an edge for a polygon outside the word range (port regression #1034, test `regression_2dlib_polyf_clip_vertical_cut`). Not bugs (no observable effect): Scroll writes the width of the 960 split back to the argument stack (it belongs to this call); left/top ClipBlit or right Scroll edge: the blitter reads/writes back one word before/behind the line unchanged. DIFF (memory layout, unavoidable): what an overflow of polypts/l1 hits (original: own code, C: the data behind it). Tests: t150–t157, t158/t159 (tests-runerr). Test construction: circles never above the top edge, Polyf ≤ 16 points, steep edges in polylib (DIVS overflow, #1019). Incidentally: t110–t112 are now silent (samples zeroed after the checksums).

## 190 gameiolib

C, declarations identical, binary = source. Joyx/Joyy/Joyr/Joyb/Gameb read JOY0DAT/JOY1DAT, CIAA PRA and POTGOR directly (Amiga and Blitz mode); Joyb tests with BTST on the byte $DFF016 (POTGOR bit 10/14). BUG(orig) (#415): Gameb port 0 jumps via DBF into the loop of port 1 and leaves it via its exit (POTGO $3000 instead of $0300). Not a bug (a word function): Joyr keeps the upper word of Joyy's MOVEQ. Test: t160 (without input; port 0 against our own decoding of JOY0DAT, Gameb with Disable), t165. Not tested: real inputs.

## 98 blitzkeyslib

C, declarations identical (binary has 6 more unused bytes). Init value = keyboard.device base; matrix from $136 (V36+) or $106, key buffer +$70/+$72/+$74. BUG(orig) (#408): BlitzKeys Off only resets the flag (handler remains). Not bugs (no observable effect): ison via NOT; entry -8 of the handler is not a BRA; word results keep the upper word of d0. Test: t161 (BlitzQualifier, RawStatus 0–127, BlitzRepeat, HandleRawKey). Not tested: BlitzKeys (Blitz mode), BlitzKey (writes the read index of keyboard.device).

## 94 animlib

C, declarations identical, binary = source. Object "Anim" (32 bytes), BODY and DLTA nodes via memlib, file via ilbmifflib/iffiolib. BUG(orig): BODY always as ByteRun1 (#397); freeanim reads the node after freeing (#399); a missing BODY gives length -1 (#400); height/depth/width 0 run 65536 times (#402). Not bugs: wrap-around to delta 2 (the ANIM op-5 double-buffer loop convention); a3 stays on the last node or behind the data (statements; reproduced). Fixed (#401): a single delta: the wrap takes node 1 when there is no node 2, NextFrame does nothing without a delta (bug-orig tests `animlib-single-delta`, `animlib-no-delta`). Fixed (intended deviation, #398): the original's "Can't Open Anim File" is a BEQ into the error check area, err6 has its message there and continues into err1; here traps and messages are in normal code (bug-orig test `animlib-errors-check-area`). DIFF: a2 unchanged. Test: t162 (self-generated ANIM in RAM:, result = host model), t168.

## 144 blitzcoplib

C, declarations identical to the binary (source additionally has `Help`, CNIF #ddftest=1). Slices build a shared copper list; Slice/ColSplit/CustomCop as $A001 routines. BUG(orig): unused slice pointers are shifted along (#407); an empty CustomCop string copies 2^32 longs (#404); maxslice 0 runs the pointer loop 65536 times (#406); the marker also changes the source buffer (#405). Not a bug: hwait as word, the upper byte is used (written and read the same way). Test: t163 (Amiga mode, copper list with masked plane pointers), t166. Not tested: ShowBlitz, DisplayOff/On, Blitz variants.

## 143 displaylib

C, declarations identical, binary = source. Object "CopList" (128 bytes), mode values from the table (resolution × fetch, 64 bytes per entry). Fixed (intended deviation): the error routines and their messages are in the normal code, not in the error check area as in the original (#412), so without the debugger an error raises a TRAP #0 instead of branching behind the library; the !subs check `_checkcoplist` stays in the area. BUG(orig): init writes the mysprite address into the SPR0POS/CTL data (#413); DisplayBitMap without x,y stores the caller's d2/d3 in x0/x1 (#411); CustomColors with thru256flag sets bot = 257 without reading the flag (0 acts as 1) (#951); DBF/word arithmetic, CMPA.l signed (not a bug: only with out-of-range arguments); attached sprites with an odd channel number overwrite memory (#414); CustomChunky with a split position copies 16 bytes and clears spri, the sprite resolution shift (#949); CustomSprites (#324) loads via `!libs{#spriteslib,$1181}` the sprite object with the number from d1 (= CCOffset) – the compiler shifts d1 by the object shift (×16) in the process, CCOffset acts 16-fold and must be a valid sprite number (maximum check); without debugger the overflow led via the jump into the error check area to somewhere (found in the test: the original happened to continue, C hung; since #412 a TRAP #0); a3 stays on bp1 or the last CopList (not a bug: statements). DIFF: a2 unchanged, table indices beyond the data (the original reads its own code). Test: t164 (all commands except CreateDisplay), t167, t169; bug-orig displaylib-errors-errarea (#412, FIXED). Not tested: CreateDisplay (Blitz mode).

## 1 dhandlerlib

Binary = source; the C build (`libs/acidlibs/dhandlerlib.c`) is
**byte-identical** to the CD's library, relocation table included. No
tokens, only init/finit (`!libs #doslib,#la6,#graphicslib,#ld6`). The
number is not in libnums.bb; the binary has 1 (`LIB_dhandlerlib` in the
section "not in libnums.bb" of `libnums.h`, like blitz2.s
`dhandlerlib=1`). blitz2 uses it together with debuglib when a program
is RUN from the editor (not for executables: rundebuglib).

* **init** gets d0 = the BPTR of the debugger program (blitz2 makeinits
  `move.l #dseg,d0`; `bblibsrc/dhandler.bb2` is such a program) and
  starts it as the process "Blitz ][ Debug Proc" (CreateProc, stack 4096):
  the segment is a copy (memlib globalloc) of the template
  `mystart`..`mystart_f`, patched at `off_1` (address of `ltable`, put
  into the process's `tc_UserData`) and `off_2` (address of `progadd` =
  BPTR*4+4, the debugger's code). `ltable` = 9 stubs l0..l8 that call
  debuglib $D500..$D508; l2 (dosethandler) also keeps a0 = the
  debugger's handler table. init waits (WaitTOF) until l2 was called,
  then installs `check_c` as VBlank server (intlib SetInt, $8035):
  Ctrl ($63), LAlt ($64) and C ($33) pressed together call debuglib
  dostop ($D501), once per key press (`laststop`).
* **finit** removes the server, calls the cleanup handler (handler table
  +20), waits (WaitTOF) for `taskdone` (set by the process after the
  debugger's code returned) and frees the segment (globfree).
* **Assembler:** everything but the header and the token table (the
  libmacs macros in C). Reasons: the template is copied and patched at
  fixed offsets; init/finit consist only of calls with their own register
  contracts (EXG d6,a6, a5/a6 kept around the cleanup handler); and byte
  identity of the relocation table: Blitz 2 wrote the references to its
  own labels in ascending order (the last ALibJsr, globfree in finit,
  among them) and then the other ALibJsr placeholders in descending
  order. gas groups relocations by target (section, then external
  symbol) in the order they are made and ld keeps that order, so code and
  data are in `.blitzlib.head` with the token table (otherwise the
  header's references to the code would come before those to the table
  entries), the globfree placeholder is relative to a label at offset 0
  of that section, and the other placeholders are absolute operands with
  explicit `.reloc` directives against `_blitz_tok`, in descending order.
  The trailing `MOVEQ #0,d0:RTS` is the end code Blitz 2 appends to every
  program. `asmcmp_dhandlerlib` compares all code with the CD.
* **BUG(orig):** the copied process code is started without a cache
  flush (CacheClearU; 68020+ instruction cache, 68040/060 copyback)
  (#409); finit frees the segment as soon as `taskdone` is set, while the
  process still has to execute the RTS behind it (small window, finit
  looks only after a WaitTOF) (#410). Not bugs: the unchecked globalloc result (memlib ends
  the program with "Out of Memory!"), and check_c's return without
  `MOVEQ #0,d0` when the keys stay pressed (Z is set by `TST laststop`,
  the server chain goes on).
* **Tests:** none in `libs/tests`: original against C build compares
  identical bytes, and a functional test needs a debugger program as
  seglist, debuglib with its trap vectors, intlib's VBlank server and a
  running process - out of proportion for a byte-identical binary.

## Incidentally

* `libs/tests/systest_system.c`: the log entry "trap N handler" contained the
  upper word of an absolute address in the lib copy and thus depended on the
  memory layout (flipped after adding `systest_cmdlibs.c`);
  now only the opcode.
