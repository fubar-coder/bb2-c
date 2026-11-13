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
  is an a3 that is *behind* the 0 (GetRexxCommand/GetResultString,
  FileRequest$).
* **Register preservation:** according to the AmiBlitz3 spec a lib routine may change everything
  except a4/a5/a7. As with the system libs, the C versions
  nevertheless preserve at least the registers that the original preserves (thunks,
  checked in `[cmd]`).
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
  only via d0 = n; without visible consequence, see General); n is counted as an unsigned 32-bit number
  (negative = ~4 GB; BUG(orig), #214). A zero byte within the n characters stays in.
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

* **BUG(orig) (#275):** for word arrays the pivot element is read with `MOVE.l`
  and compared with `CMP.w` - the pivot is the element **after** the
  middle. For arrays with at least 2 elements it is still within range,
  the result is correct; for a single element (`Dim a.w(0)`)
  it lies behind the array: the forward search stops at this word, the
  backward search possibly reads words before the array; then i > j, no
  swap, no recursion – reading only, result correct (not in the corpus).
* String elements that were never assigned (pointer 0) read their length
  from -4(0) (BUG(orig), #276).
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

* **BUG(orig) (#202):** init tries `OldOpenLibrary("mathtrans.library")` endlessly
  if the library is missing.
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
  (`TaskName$(0)+"|"` is correct; not a bug). **BUG(orig) (#297):** a task
  without a name reads from address 0.
* `CreateMsgPort` returns with a3 = name argument ("Protect BB2 String
  area"); the name length comes from -4(a2) of the caller; copied are
  (length & $FFFF)+1 bytes (`DBRA`). Empty name: port not public,
  `ln_Name` = 0.
* **BUG(orig) (#298):** a failure of `AllocSignal(-1)` (-1) is not
  detected (`TST.l d0:BEQ` checks for signal 0), `mp_SigBit` = $FF;
  `DeleteMsgPort` then calls `FreeSignal(255)` (in C explicitly
  zero-extended, the NDK inline takes BYTE). The never-reached
  error branch would free the port address (block+8) and $22 instead of
  the block and $2A.
* Memory error → runtime error "Failed to allocate memory for Port"
  (`TRAP #0`); the original then runs into its message text, the
  C version returns with 0 (#300, fixed in the port). Not testable.
* `DeleteMsgPort` checks nothing: pending messages remain
  unanswered, foreign ports destroy the list (BUG(orig), #299).
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

C translation of bblibsrc/datelib, binary = source. The word/byte arithmetic of the original is reproduced register-exactly (`BL_SETW`/`BL_SETB`, DIVU/DIVS as an instruction with `bl_divu_w`/`bl_divs_w` as in modlib), so that invalid inputs also come out the same. Fixed (intended deviation, #120): day 366 of a 4-year cycle appeared as "32/12" of the leap year (`BLE` instead of `BLT`), now the 1 January of the next year (bug-orig test `datelib-day366`). Fixed (intended deviation, #1001): the 4-year cycles made every year divisible by 4 a leap year (1900, 2100 too), so dates before 1/3/1900 and from 1/3/2100 on were off by a day per century; Date$ and NumDays now correct the day count by the Gregorian century rule, nothing changes from 1/3/1900 to 28/2/2100 (bug-orig test `datelib-century-leap-years`). Kept peculiarities (BUG(orig)): WeekDay calculates `SUBQ.w #2` (#121), NumDays `ADDQ.w`/`SUBQ.w` without carry (NumDays("31/12/1977") = 65535) (#122); month 0 reads the tick word of the last DateStamp, months 14/15 the two words behind the table (reproduced), months > 15 read foreign memory (not reproducible) (#124); two-digit years without "/" read beyond the end of the string (#125). Test: t30_date (without the days of #120; its dates before 1/3/1900 and from 1/3/2100 on are the Gregorian ones, #1001).

## 128 linklistlib

CD binary older than the source (SetItem/CountList/CountItem missing) – ports only the 14 tokens + init of the binary, code otherwise the same as the source. Interrupt lock ($dff09a) as in the original, list heads as nodes, item stack via memlib globalloc/globfree. Error checks in C in the error check area (`BLITZ_ERRCHK_THUNK`, messages as assembler data with `BLITZ_ERRCHK_MESSAGE`). BUG(orig): SortList compares with CMP.l/BMI (sign of the difference, ±2·10^9 stays unsorted) (#201). Not bugs (not observable): ClearList hangs the items into the free list in reverse order, deleted items keep their data. DIFF: SortList does not destroy a3. Tests: t31_list as well as the runtime error tests t32–t36 in corpus/tests-runerr (runnable only with error checking); all outputs the same.

## 185 stringfunclib

C, declarations identical to the CD binary (source = binary). a2 (length stack of the string arguments) is a register parameter of every routine that reads lengths; reading is done via a local pointer, a2 stays unchanged (DIFF, see General). Chr$ and Len stay inline (assembler text). The init value is, as in the original, the address of movestring (a0/a3 are advanced; no other lib of the CD uses it). The original leaves a2 at different places depending on the case (String$ with count ≤ 0 and Replace$ with an empty search string take fewer lengths from the stack, Cvi/Cvl/Cvq none at all); the C version always leaves a2 unchanged (DIFF, results the same). The inline Len stays `MOVE.l -(a2),d0` as in the original. BUG(orig) kept: Right$ calculates with a word offset (#285); Instr with start pos ≤ 0 compares before the string (#287); Mid$ with a huge length copies too much (#286). Test: `t20_strfunc` (all 28 tokens, output the same as with original libs).

## 120 vallib

C, declarations identical. Int/Sgn/Abs/Val and intchk get a2 as a register parameter and read the type word or length with a negative offset; a2 stays unchanged, the original leaves it decremented by 2 or 4 (DIFF, see General). All float calculations go through mathffp with the same calls and operands. QFrac, QWrap, QAbs, VPos, True and False stay inline; the arc table of QAngle comes from the binary, DIVU including overflow is reproduced in C. Rnd reads the hardware in the same order; the sequence depends on timing, hence only a range check. BUG(orig) kept: Sgn of a float is always 1 (SPTst checks D1 = type word) (#301); Val reads one byte behind the string after "-", "$" and "%" (normally the terminating 0) (#302); Val counts the length as a word (#303); from 32 decimal places on the fractional part is dropped (#304). Fixed (intended deviation, #1003): QAngle subtracted the coordinates as words, so a difference of 32768 or more in magnitude gave the angle of another quadrant; the differences are longs now (bug-orig test `vallib-qangle-word-range`). Not a vallib bug: Abs(Quick 0) runs via `$80` (floatquicklib turns Quick 0 into `$80`, #146; Abs clears bit 7, the result 0 is right). Test: `t21_val` (all 16 tokens; the QAngle lines with differences of 32768 or more are the fixed ones, #1003).

## 124 fileiolib

C, declarations identical (binary = source, March 1995). Object lib "File" (32 bytes, maximum 5, Free = CloseFile); object pointer in a1/a2/a3 depending on the token ($1180/$1280/$1380). The buffered DOS layer (myread/mywrite/myseek/myflush) is C with the word arithmetic of the original; without work (length 0, position in the buffer) d0 comes back unchanged (FileSeek/Put/Get then compare the file number or field address with -1). The handlers for inputoutputlib (FileInput/FileOutput) keep the BRA table before the entry address (-8/-4/0) as a short assembler block, call C via thunks. BUG(orig): Eof at the end of the file leaves the buffer position 1 too high (the DOS seek fails) – Loc/Lof afterwards +1 (t50: Lof 32 for 31 bytes; NPrint evaluates from the right, Eof first) (#144); Exists = 0 also for empty files (#143), -1 for drawers; CloseFile frees the Fields, leaves pointer and record length in place (#140); Fields adds (#141); Lof does not flush (not a bug: that is the length including buffered data); Put/Get transfer the fields in the reverse order of Fields (not a bug, decision 2026-10-31: consistent with itself, existing data files depend on it) (#142); OpenFile on an open file with Fields reads the name length from -4(0) (reproduced, not tested) (#138). DIFF: err1/err2/err3/err6 and the message of err7 are jumped to by the original from normal code into the error check area (cut off without debugger) – here in normal code; a2, d4-d7, a3 stay unchanged. Tests: t50_fileio, t53_fileran, t54–t57 (tests-runerr).

## 106 cliargslib

C, declarations identical. Data of wbstartuplib in a0. FromCLI loads only the lower word of d0 (thunk). BUG(orig): no check of the argument number without debugger (#106); ParPath$ from the CLI calls NameFromLock(0) (#107) (lock 0 = root of the boot file system; which text dos.library returns for it, e.g. "SYS:", is not checked). DIFF: after NameFromLock, d0 = 0 is checked instead of the CCR of dos.library. Tests: t51_cliargs (the corpus runner passes no arguments; checked locally with arguments), t58/t59 (tests-runerr).

## 76 banklib

C, declarations identical. Object lib "Bank" (16 bytes: pos, size, memtype; maximum 5), no Free. LoadBank asks fileiolib Exists (ALibJsr `TOK_exists`, a6 = dos). InitBank stores size/type before allocating (without consequence in the original: memlib does not return on lack of memory, only with the C memlib, K; #71). Not bugs (no observable effect / description): FreeBank leaves memtype in place; LoadBank allocates anew for a different type or if the file is not smaller (sign of the difference) and reads bank-size bytes; missing or empty file: nothing. BUG(orig) (#72): LoadBank of a drawer (Exists = -1) with a different memtype makes size -1. Test: t52_bank.

## 164 inputoutputlib

C, declarations identical, binary = source. Channels are handler addresses with the entry points -8 (string with 0), -4 (string a0/d0 or read line) and 0 (character); BRA.w stand before the handler. dosetin/dosetout (+3/+4) put the old handler on a ring of 16 longs (index & 63) and count; init puts handler 0 on both stacks. The five dispatchers (+5…+9, `JSR off(a6)` with a6 = handler) stay assembler because they pass all registers unchanged to foreign handlers; the rest is C, dos via proto/dos.h. Default input = `Read(Input(),a0,d0+1)-1` (end of file -1, error -2). DIFF: the single-character entry of defin is in the original `BRA err1` into the error check area (without debugger a jump behind the lib); in C trap and message are in normal code. BUG(orig): message "Defualt" (#193); more pops than pushes take old ring entries without debugger, a PopOutput directly after the start sets the handler to 0 (#194). Tests: t41_io, t43–t45 (runtime errors).

## 187 printlib

C, declarations identical. `libincs:ffptoascii` is missing on the CD; the routine is translated from the binary ($658–$9D3: 7 digits by subtracting powers of ten, table 1E19…1E-28, rounding via $80 below the mantissa). Variables, buffer and table lie one after the other as in the original, so that overflows come out the same. Output via inputoutputlib (`TOK_iooutsingle`/`TOK_iooutstring1`); Str$/UStr$ set their own handler (strprint, writes to a3). FloatMode 0 = fixed point for 1E-9 ≤ |x| < 1E7. BUG(orig) kept: a Quick fractional part that rounds to 10000 loses the carry ("2." for $0002FFFF) (#233), a fractional part that rounds to 0 keeps the point (#234); Format loses the carry beyond the first digit (999.96 with "###.#" → "000.0") (#235), never rounds numbers without an integer digit (|x| < 1) regardless of the format (#236), without "-"/"+" the sign is dropped (#237), on overflow "?" (#238); Format over 32 characters overwrites formlen/formlead (#240); FloatMode -1 writes behind the buffer for numbers < 1E-17 (#241); Str$ of a string outputs the lower byte of the address (#242). Not bugs: no leading 0 (`.5`, the output style of Blitz 2); every Str$ puts two entries on the output stack (a ring of 16, harmless). The compiler knows exponents only up to 18 (`1E-19` → "Numeric Over Flow"). DIFF: a2 stays unchanged. Test: t40_print (all types, edge values, FloatMode -1/0/1/2/-5, 23 format strings).

## 186 editlib

C, declarations identical, binary = source. Edit$ pads the default with spaces to n characters and reads over it (`TOK_iogetstring`). Edit uses its own buffer of 80 bytes, the default via UStr$ (`TOK_ustr`) and the result via Val (`TOK_val`), both with a2 on the block `pars` {length.l, type.w = 5} (`BLITZ_ALIBJSR_A2A6`). BUG(orig): Inkey$(0) returns the caller's d6 as length (#128); with Edit(value,n) the default loses its last digit through the padding (UStr$ leaves a3 without 0 behind the digit, the `SUBQ #1,a3` is meant for string arguments; Edit$ is not affected) (#129); Edit calls UStr$/Val with the caller's a6 (works only if a6 is still the ffplib base) (#130); Edit(80) reads 81 bytes (#131); at the end of the file DefaultInput returns the length -1 (#132). DIFF: a2 stays unchanged. Tests: t41_io (DefaultInput via a child process with redirected input), t42_edit (FileInput), t46–t49 (runtime errors).

## 74 crunchlib (`#romuluscrunchlib`)

C, declarations identical, binary = source. Imploder (Implode/Deplode), Crunch-Mania decruncher (CrM!/CrM2) and PowerPacker decruncher (PP20) translated register-exactly (widths byte/word/long, shift counts mod 64, DBF on the low word); additionally checked on the host in unicorn (68020) against the original code: Implode output, Deplode and all decrunchers byte-identical on 330+ data sets and 400 corrupted inputs. Implode always runs with "Mode 8" (smallest table, window 128), writes COLOR00 in the process and polls the left mouse button (abort → -1). BUG(orig): the Implode search reads up to 254 bytes behind the data (`im30` up to 15, length measurement `im32`/`im33` up to 254, not added; result independent of it) (#112); result 0 leaves the buffer partly overwritten (#114); CrM overlap copy with CrLen<4 copies 4 GB (#115), CrMDecrunch with one argument overwrites MinSecDist bytes before the buffer (#116); ReadTab/ReadD1_SUB with 0 run 65536 times (#117, #118). Kept, not a bug: after $4012 literals the rest is stored uncompressed (limit of the Imploder format, #113). DIFF: corrupted CrM2 data can reach beyond the tables into the lib code (result depends on the lib layout). Test: t60_crunch (test data for PP20/CrM from our own encoder).

## 92 arexxlib

C, declarations identical, binary = source. BB2 ArgStrings (memlib, ra_Size with bit 31 = "from Blitz"), RexxMsgs (128 bytes, ln_Name "REXX"). Errors (memory, no REXX port, no rexxsyslib) show a recoverable alert and return the error code (0/4/8). BUG(orig): ReplyRexxMsg checks with `BTST #17,28(a0)` (memory BTST = bit 25 = RXFUNC) instead of RXFF_RESULT (#48); ClearRexxMsg/FillRexxMsg without limit 16 (#49); index (n-1)*4 as word (n=0 = rm_Action) (#51); GetRexxCommand/GetResultString leave a3 behind the 0 (#52), GetResultString leaves rm_Result2 pointing to the freed string (#53); SendRexxCommand without REXX port: alert with interrupts disabled (#55). Kept, not a bug: RexxError without terminating 0 (invisible, see General; #54). DIFF: ReplyRexxMsg with a non-RexxMsg returns (original: crash; #47); -8(a2) is not copied to -4(a2). Test: t61_arexx with its own port "REXX" (priority 100), RexxMast not needed. Not tested: real ARexx host, error paths (alert waits for the mouse).

## 90 seriallib

C, declarations identical, binary = source. allio: 8 units × 256 bytes (read half +0, write half +128, each with request pointer, MsgPort, IOExtSer). BUG(orig): CloseSerial/SetSerialBuffer/SetSerialLens/SetSerialParams/SerialEvent calculate unit*128, the others unit*256 (only unit 0 consistent) (#252); ReadSerialMem reads via the write request (#253); WriteSerialString flag 2 = TrueString and DoIO at the same time (flag 1 unchecked) (#254), ReadSerial "wait" = flag 2 (#255); CloseSerial AbortIO without WaitIO, write request remains (#258); if AllocSignal of the write half fails, the device stays open (#259); "Serial Device Not Open" via TRAP #0 even without debugger (#262, the C version returns). Kept, not a bug: ReadSerialString without 0 (invisible, see General; #257). DIFF: ReadSerial leaves a3 unchanged. Test: t62_serial (unit 0 without remote station), t63_serialerr in tests-runerr, t64_serialrx in `tests/corpus/tests-serial` with remote station (`tests/corpus/runner/serial.c`: Amiberry `serial_port=TCP://…`, answering program on the host): ReadSerial without/with flag 1/with wait bit 2 (blocking), ReadSerialString with data, ReadSerialMem (via the write request), SerialEvent 1/2/both, WriteSerialString asynchronous (length -1 sends the terminating 0 along) and address form, SetSerialLens 7 bit, SetSerialParams (baud, EOF mode: has no effect on ReadSerialMem, since the write request has the old flags), SetSerialBuffer 64 with overflow (reading returns SerErr_BufOverflow 12). Result: line bytes the same in all four configurations, output the same except for the DIFF a3 (two ReadSerial in one statement: in the original the second read overwrites the string part built in rb1 in between, `97b98`; the test now reads separately, after that 0 deviations). Not tested: units > 0 with remote station, 7WIRE/handshake, parity, break.

## 158 screenslib

C, declarations identical, binary = source. Object {Screen.l, DBufInfo.l}; own screens get UserData "bLtZ", only those are closed by CloseScreen/Finit. Screen n,mode goes into the long form via a fake a2 stack (`dummydata`); width rounded up to 16, ExtNewScreen with SA_DisplayID/SA_Pens, font = current IntuiFont. ShowBitMap from V39 on with ChangeVPBitMap, before that copper list/MakeVPort (only ported, not executed). ViewPort, Show/Hide/Beep/MoveScreen, SMouseX/Y stay inline. BUG(orig): freescreen is called with a2 = argument stack as graphics base (with DBufInfo, FreeDBufInfo calls into nowhere) (#247); FindScreen/WbToScreen free the object first (a found Blitz screen is closed) (#248); ScreenTags copies a tag list without limit into 32 longs (#249). Fixed (intended deviation, #250): in the original SA_DisplayID gets only the lower word of the viewmode (display IDs with a monitor part open on the default monitor); here the whole long (bug-orig test `screenslib-displayid-lowword`). Fixed (intended deviation, #251): the original's err1 (Can't open Screen) is a jump from normal code into the error check area, err8/err12/err13 have their messages there; here traps and messages are in normal code (bug-orig test `screenslib-err1-errorarea`). Tests: t81_screen, t86–t89.

## 142 fontlib

C, declarations identical, binary = source. BlitzFont = TextFont pointer, only 8x8 and not proportional; init loads topaz 8 into object 0. BitMapOutput draws into the bitmap via handlers (-8/-4/0); BitMapInput is a line editor on the bitmap (`doinput1` stays assembler, jumps to the old handler). Not a bug (a pixel cursor by design): Locate y is Quick, CursX/CursY = cursor/8 as Quick. BUG(orig): depth 0 lets the plane loops run 65536 times (#149); a wrong font is not closed (#150); BitMapInput waits endlessly for 13 (#151). Fixed (intended deviation, #148): characters ≥ LoChar+128 were dropped (signed byte compare), now an unsigned compare (bug-orig test `fontlib-lochar-127`). Fixed (intended deviation, #955): err1-err3 trap in normal code, their messages are in the error check area in the original; here in normal code (bug-orig test `fontlib-messages-errarea`). DIFF: if the trap in calcoff returns, C draws nothing; the unused `bitmapchk` is missing. BitMapInput is only allowed in Blitz mode and not tested. Tests: t82_font, t84.

## 134 intuifontlib

C, CD binary older than the source (token `Text` missing) – the state of the binary is ported. Object = TextAttr + Font + Pad (16 bytes); init value = address of `textattr`. LoadFont copies the name with maxslib and calls OpenDiskFont; `loadchk` checks the CCR (assembler line, thunks set Z). BUG(orig): style goes as a word to +6 (LoadFont …,9,1 → style 0, flags 1) (#197); the old name copy is never freed (#198). Tests: t82_font, t85.

## 130 rawkeylib

C, declarations identical. `libincs:rawcode` is missing on the CD; everything except unconvert is translated from the binary. Init opens console.device and fills two tables (105 entries); F1–F10 → 129–138, cursor keys → 28–31, Help → 139, otherwise RawKeyConvert. BUG(orig) (#245): qconvertkey indexes with code & 255 beyond the tables into the lib's own code (the 151 code bytes of the original are appended as data). No dedicated runtime test (a direct ALibJsr call from BASIC already hung with the original libs); checked indirectly via windowslib (keys, Edit$ in t72).

## 138 windowslib

C, declarations identical (75 tokens). Object lib "Window" (32 bytes), one shared MsgPort for all windows, own event list (24-byte nodes). Input/output handlers for inputoutputlib with BRA table (-4 line editor, -8/-4 strings) as a short assembler block, otherwise C. BUG(orig): Menus/MenusOn/MenusOff go over the 32-byte objects in 16-byte steps (#310); string gadget down generates an additional "gadget up" (#311); MENUPICK appends a further event for NextSelect (qualifier/IAddress uninitialised) (#312); Qualifier (#long) sets only the lower word (#313); WCls from y=1 (#314); WindowTags keeps old user tags (#316), the tag pairs without limit into 32 longs (#317). Fixed (intended deviation, #318): in the original the title copies of Window, WindowTags and WTitle are never freed; here the object keeps them (+20 window title, +24 WTitle's screen title, free bytes of the 32-byte object): WTitle frees the old ones, CloseWindow and a failed open free them (bug-orig test `windowslib-titel-leck`, now also with Window and WindowTags). Fixed (intended deviation, #317): in the original WindowTags …,&TagList copies the list without limit into the 32 longs of usertags; here at most 15 pairs, a longer list is linked with TAG_MORE from the 16th entry on (bug-orig test `windowslib-taglist-ueberlauf`). Fixed (intended deviation, #320): in the original WMove/WSize busy-wait until the window has exactly the new position/size, endlessly if Intuition limits it (size limits, screen edge); here the wait also ends when Intuition changed the window, at the latest after 50 frames (WaitTOF) (bug-orig test `windowslib-wmove-wsize-endlos`). Fixed (intended deviation, #322): in the original CloseWindow replies all messages of the shared port and discards the events of all windows; here only those of the closed window (as AmiBlitz3; bug-orig test `windowslib-closewindow-ereignisse`). Not a bug: Event compares classes as word (IDCMP classes are single bits, same result). DIFF: err6/err7 from normal code; a3, a4, d7 and the argument stack stay unchanged. Tests t70–t72 (events as self-built IntuiMessages before waiting, Edit$ with keys queued in advance and timer watchdog), t75/t76/t78/t79. Not tested: real Intuition input, SetEventFilter, commands with shapes/bitmaps, windows with gadget lists.

## 136 menuslib

C, declarations identical. Object lib "MenuList" (4 bytes). BUG(orig): replacing a SubItem shifts the following sub-items by (number − old height) (#220); MenuState on a menu sets bit 4 (#216); MenuChecked with a sub-item jumps back into the menu loop (only 0/0 correct) (#217); ShapeItem flags = upper word of the longword (#218); flag test as word comparison (#219); without the debugger wrong menu/item numbers walk over the end of the lists (#416). Not bugs: a shortcut sets the flags to 4 before the OR (the intended result); MenuTitle width = 8·length + 8 (a limitation noted in the original source). Tests t73, t71, t77.

## 108 mouselib

C, declarations identical. Almost everything only in Blitz mode (err1 = 1). BUG(orig): Mouse On swaps the JOY0DAT bytes (#223). Not bugs: the interrupt leaves a5 in place (a scratch register for interrupt servers); the sprite number of Pointer is object offset >> 4 (offset = number * 16, correct). Test t74 (Blitz mode without Mouse On; interrupt and Pointer with sprite not testable).

## 162 iffiolib

C, declarations identical, binary = source. No tokens of its own; routines for other libs (ilbmifflib, animlib, audiolib, palettelib, lib 154): +1 iffopen (a0 = name), +2 iffclose, +3 iffbyte, +4 iffword, +5 ifflong, +6 ifffind (d0 = ID → chunk length), +7 iffskip (d0 = number of **bytes**, the source says "long words"), +8 iffread (d0 bytes to a0). init (a0 = dos) gets the 2048-byte buffer via globalloc, init value = buffer address. Result in the CCR (Z = ok, otherwise d0 = -1/N), iffbyte/iffword replace only d0.b/d0.w; the thunks set the CCR via `tst.w` from a status variable. All registers except d0 (iffread: a0) are preserved. BUG(orig): charsleft = lower word of the Read result (read error → 65535, buffer contents count as data) (#177); ifffind aborts at "BODY" and reads byte by byte (#178); iffskip/iffread with 0 count from 2^32 (#179). Not a bug (all callers ignore d0/a0 afterwards): iffread reads from the end of the buffer via Read directly to a0 (a0 not advanced). Tests indirectly via t100 and t103.

## 161 ilbmifflib

C, declarations identical, binary = source. For other libs: +1 ilbminit (a0 = name → d0 = -1/0, a0 = iffstuff, file stays open), +2 ilbmgetbyte (ByteRun1-decoded), +3 ilbmbitmap (a0 = name, a1 = BitMap). ViewMode: pageWidth ≥ 480 → $8000, pageHeight ≥ 300 → |4. BUG(orig): ILBMInfo closes the file even if it was not opened (Close(0)) (#186); ilbmbitmap ignores a failed ilbminit (#187) and compares the plane with the word BitMap+4 (#188); DecodeILBM does not round up chunk lengths (#189), writes all planes of the ILBM via the plane pointers of the BitMap (also behind Planes[]) (#190), does not clip ByteRun1 runs at the end of the line (#191). Fixed (intended deviation): the original has the error routines in normal code but their messages in the error check area (without the debugger d0 points behind the library); here the messages are in normal code too (#192, test `bug_orig_ilbmifflib_messages_errarea`). Not a bug (word functions): ILBMWidth/Height/Depth load only d0.w. Tests: t100, t103.

## 114 iffmakelib

C, declarations identical, binary = source. The four commands are only allowed in Amiga mode: odd code address in !subs (blitz2.s doajsrrout), in C `AMIGAONLY()`. ILBM: BMHD, CMAP (upper bytes of the 32-bit values), BODY per line and plane ByteRun1, lengths filled in afterwards via Seek. SaveScreen: Exec ≥ 39 → GetRGB32 per colour, otherwise ColorTable 4 bit → 32 bit. BUG(orig): the plane loop compares with the word BitMap+4 (#180); line/plane loops run at least once (#181); savearow reads one byte behind the line (#183); CMAP with odd length without padding byte (#184); palette with 0 colours → 65536 colours (#182); SaveShape does not check the depth (#326) (depth 0: 65536 plane pointers from `bmcopy` on, depth ≥ 15: beyond its 14 pointers into `usewid`/`scrcols`). Fixed (intended deviation, #185): "Can't open File" traps in normal code, its message is in the error check area in the original; here in normal code (bug-orig test `iffmakelib-openerr-errarea`). Tests: t100, t104, t107.

## 132 brexxlib

C, declarations identical, binary = source. init opens input.device and installs an input handler with priority 51 (C function, calls memlib globalloc from the input.device task like the original). Tapes = lists of InputEvents (22 bytes). BUG(orig): TapeTrap without a number falls through into TapeTrap Tape# (#95); appending to a tape clears mode (#96); ClickButton passes the d0 of the first call to the "release" (under TapeTrap with QuietTrap On "press left, release RIGHT", proven in the original) (#97); AbsMouse and RelMouse both generate POINTERPOS (#98); PlayWait restarts the tape (#99); Type: unknown character → code $FFFF (#102); XStatus contains bit 15 (#103). DIFF: "Can't Open File" (SaveTape/LoadTape) in normal code instead of a jump into the error check area. Tests: t101 (events only on tapes), t106.

## 117 blitzoslib

C, declarations identical, binary = source (code compared by hand with source and disassembly). Error check field `1` in !subs = Blitz mode only; allowed in Amiga mode are only InitPage, FreePage, AddBox, SetBox Box#,Value, SetBoxString, SetPagePens, ChangeBox. `!libs …,$0604` = d6 = offset of the object variable + 4. Drawing via BLibJsr to 2dlib, fontlib, scrolllib, shapes, mouselib, blitzkeyslib (macro BCALL). BUG(orig): box numbers via ASL.w #5/ADDA.w (#81); SelectBox Box# unchecked (#82); SpotBox without page check (#83); string box with 8–15 pixels width fills 65536 spaces (#84); BoxString backwards without limit (#85); DeSelectBox does not clear `selected` (#94). Kept, not a bug: BoxString without 0 (invisible, see General; #86); BoxPrint overwrites the string length -4(a2) with the scroll size (the argument area belongs only to this call; #88); BlitzEvent's BMI after CMP.b (same result as an unsigned compare; #92); SelectBox redraws the old box (only extra time; #93). DIFF: coordinates to Boxf/Line/Locate as sign-extended words. No emulator test of its own in the library test suite yet. The crashes seen with the original lib (a Blitz mode test and an error path test) were compiler bug #335: blitzoslib reaches shapeslib only through its drawing code, and the compiler gave that object library no array (fixed in blitz2-c, commit 124f83ae, test `compiler-maxs-libjsr`); with it fixed an emulator test of blitzoslib is possible (bug-orig tests that compile and run blitzoslib programs exist: `tests/bug-orig/libs/blitzoslib-*.bas`).

## 116 audiolib

C, declarations identical, binary = source (the VBlank code between the two CELSE is not in the binary). Sound object {data, period, length in words, repeat position, repeat length, "cL"}. Everything goes directly to Paula; Sound/LoopSound switch off the DMA and wait 12 line changes of VHPOSR. IFF 8SVX is read by iffiolib (errors in the CCR, local macro `IFFCALL`). Period = 3579440 / samplesPerSec with DIVU (overflow stays). BUG(orig): sndchk checks, for SetPeriod, the period as channel mask (period > 15 → "Illegal Chanel Mask" with debugger, t115) (#66); LoadSound with a too long sample leaves the file open (#67); a "cL" mark from DecodeSound stays in place in fast RAM on a later DecodeSound (#68); DiskPlay never ends if the file length is a multiple of the buffer length (#69); DiskPlay allocates its chip buffer before the open and does not free it on err1–err3 (lost while a SetErr program goes on, memlib frees it at the end) (#950). Fixed (intended deviation): in the original SetPeriod without a sound branches to err5 in the error check area, and the texts of err1–err4 (normal code) lie there too; here err5 and the texts of err1–err5 are in normal code (#70, test `bug_orig_audiolib_setperiod_err5`).

**Compiler/lib interplay (fixed in the compiler, #46, important for AmiBlitz3):** object functions like PeekSound load the object into a3 via `!libs …,$1380` – but a3 is also the string pointer. In the original, such a call in the middle of a string expression (`s$=s$+Str$(PeekSound(0,i))`, `NPrint "x",PeekSound(..)`) made the rest of the expression write its strings into the object ("Unable to Free Memory", without debugger Guru 8000 0020). blitz2's libfunction now saves a3 around such a call inside a string expression (`docs/libs/deviations.md` section 5, test `compiler-libfunc-a3-string`). The corpus tests use such functions only in simple assignments. DiskPlay not tested (busy-waits for audio interrupts). Tests: t110, t114, t115, t119 (only zero data or volume 0).

## 96 trackerlib

C, binary = source. NoiseTracker player (mt_init/mt_music/mt_end) in C; mt_music in the VBlank interrupt via intlib $C100/$C101 (Blitz/Amiga variant). The two DBF wait loops and the prefixes stay assembler. mt_init clears the first long of each sample. BUG(orig): mt_init checks positions 0–126 (127 never); a new maximum at position 126 lets the counter run from 0 to $FFFF (search ~64 KB further, `CLR.l` behind the module) (#289); the volume word contains the finetune byte (#290); repeat × 2 as word (#291); after D/B the song end is compared with the first pattern byte (#292); Cxx writes the volume as byte (#293); sample numbers > 31 read behind the table (#295); LoadModule of a missing file does not free the FileInfoBlock, for a drawer additionally not the lock (#296). Fixed (intended deviation, #957): "Error Accessing Module File" traps in normal code, its message is in the error check area in the original; here in normal code (bug-orig test `trackerlib-err1-errarea`). Tests: t111 (a generated M.K. module, `tests/corpus/data/t111_song.mod.spec`: checksums before/after mt_init, Play/Stop immediately), t116, t117.

## 82 medlib

C, binary = source (MED V3 player by T. Kinnunen; SYNTH, RELVOL, IFFMOCT, HOLD, PLAYMMD0; no interrupt, PlayMed = one tick), plain C with structs for the module and the track structures (until 2026-10-31 translated at register level; the register values the results depend on are kept explicitly: the qualifier word of a line, the d1 _PlayNote leaves). Data base, 16 period tables and two jump tables from the binary in the original layout. BUG(orig): Bxx stores the whole word of d4 (#203); 7xy with x = 0 jumps into the vibrato code (#204); with 1Fxx the retrigger nibble is lost (#205); the synth period limit sets d1 instead of d5 (#206); relocation after a MIDI dump reads the next module pointer from (expansion address AND $FFFF0000) (#207); pushnewvals with a gappy channel mask: higher channels get no follow-up values (track/bit stay paired) (#208); tracks 4–15 write beyond their 20-byte structure (#209); JumpMed without module writes to address 44–47 (exception vector 11) (#210); GetMed* read and clear (#212). DIFF: LoadMedModule and SetMedVolume do not change a3. Tests: t112 (a generated MMD0 module, `tests/corpus/data/t112_song.med.spec`, 400 + 3 x 60 + 120 ticks), t118, libs/tests/systest_med.c (generated MMD0/MMD1 modules with synth, hybrid and IFF octave instruments, the whole player state compared with the original after every pulse).

## 88 speaklib

C, binary = source. narrator_rb byte-exact (88 bytes, V37 part at an odd address = VoiceLoc). If translator.library or narrator.device is missing, "Failed To Open Speech Library" occurs even without debugger (TRAP in normal code). BUG(orig): Translate$ writes the phonemes over the argument (length with terminating 0, a3 behind it) (#277); Finit closes device and signal even after a failed init (#278); SetVoice limits with word comparisons (#279). Not a bug (a statement): PhoneticSpeak leaves a3 on the argument. In the emulator (OS 3.2) translator.library and narrator.device are missing → t113 (tests-runerr) checks only the init error; Translate$/SetVoice/VoiceLoc not comparable.

## 140 gadgetslib

C, declarations identical, binary = source. Object lib GadgetList (4 bytes), Gadgets/IntuiTexts/Borders/StringInfo/PropInfo from memlib, texts via maxslib. Host comparison (unicorn) against the original: OS calls and heap byte-identical; t90_gadgets the same in all four configurations. BUG(orig): freegadget never frees SpecialInfo (type overwritten by `MOVEQ #22`) (#163); StringGadget with Borders Off writes to 0/2/36/38 (#164); Maxlen 0 clears 64 KB (#165); findgadget without end check (#166); SelectMode only stores selmode, which nothing reads: no effect, gadgets always get defflags = 0 (GADGHCOMP) (#952). Not a bug (word functions): GadgetStatus/ButtonId with a foreign upper word.

## 141 gtlib (`#mygadtoolslib`)

C after the CD binary (the source is a different state; DetachGTList, GTGimmeCludge, GTSetHighlight, GTFreeGadget only in the binary). Tag lists static in the original layout, hence the same peculiarities: GTBevelBox never recessed (#170). Fixed (intended deviation, #173): GTShape copied its static image over the GadTools button's GadgetRender and shared one static SelectRender image; now each GTShape gadget has two images of its own (in the library's block list), GadTools' render pointers are put back before FreeGadgets (bug-orig test `gtlib-gtshape-image`). Fixed (intended deviation, #168): the GTPalette colour ended up in the low word of the TAG_MORE pointer (+38 of an older layout); now in GTPA_Color, the form without it sets GTPA_Color 0 (bug-orig test `gtlib-gtpalette-tagmore`). Fixed (intended deviation, #169): the optional values (cycle/mx active, listview top/selected, scroller top, slider level, string default) stayed in the static tag lists for a later call without them; now the forms without them set the initial values (bug-orig test `gtlib-optional-values-stay`). Fixed (intended deviation, #327): GTShape went on with the result of a failed CreateGadget (writes to addresses 12-25); now it leaves a failed gadget alone (bug-orig test `gtlib-gtshape-creategadget-null`). Fixed (intended deviation, #174): the Cycle/MX labels had room for 63 without a check and were never freed; now the block is sized by counting the `|`, kept in a list of the library (GTList, gadget) and freed with the gadget (GTFreeGadget) or the GTList (bug-orig test `gtlib-getoptions`). **GTGetAttrs/GTSet…/GTChangeList/GTSetHighlight leave a3 on their tag list** – strings afterwards in the same statement overwrite it (BUG(orig), #328, C the same). Host comparison identical; no emulator test: the original run crashed at program end (Guru 8100 0005), cause not found.

## 80 asllib (`#myasllib`)

C, declarations identical; the binary differs from the source (additional font tag ASL_TB+17=256, pattern "" instead of 0, freeing with FreeFileRequest -36) – the binary is the reference. BUG(orig): ASLScreenRequest returns the freed requester (#60); ASLPathRequest$ copies fr_File to the address in d2 (#61); without asl.library the length is the title address (#62). Host comparison identical; no stable emulator test (closing task in the BASIC test not reliable).

## 152 freqlib

C: wrapper from the source, the requester (libincs:filereq.s, not on the CD) from the disassembly; its data as a data block with the original offsets (bytes + relocs from the binary), meanwhile as `struct fq_data fq` (NDK structures, variables, buffers; every offset checked via `static_assert` against the address in the binary; data block byte- and reloc-identical, code shorter because GCC saw the assembler block as a byte array with alignment 1; host comparison unicorn 27 scenarios the same). **Split (for TED, "Variante C"):** the requester core (code, data block `fq`, parameter block `freqst[4]`, entry `freqcode()`) is in `shared/filereq/filereq_core.c` (a separate translation unit), without blitzlib.h; `freqlib.c` is now only the BlitzLib shell (`dofreq`/`freqchk`) and defines `fq_divu_w` (= `bl_divu_w`) before the `#include`. The same file is included by `programs/ted/src/filereq.c` (TED's embedded hunk `filereq` is the same code). What the includer has to provide beforehand is stated at the head of the .inc. `freqlib.obj` is byte-identical after the split (`cmp` against the state before; `biglib.py compare` with ref: declarations identical). Host comparison identical in 7 scenarios. BUG(orig): path$/file$ without new length (#153), libraries never closed (#154), volume list without locking the DOS list (#156; the labels <ASN>/<VOL> are correct: `dl_Type` 1 = DLT_DIRECTORY → "<ASN>", 2 = DLT_VOLUME → "<VOL>", #155), the CD gadget changes the current directory (#157) and clears the upper word of the UndoBuffer pointer of the path gadget ($111A instead of DispPos) (#345), ParentDir locks stay open (#158). The FileInfoBlock must be longword-aligned (BPTR) – explicitly aligned in C (the first emulator run hung because of this). Fixed (#160): the original's window shows (height − 85) / 8 lines without limit, but there are only 64 line gadgets (up to the end of the data block $39DA); from window height 605 on (first window = screen height − 40, so every screen from 645 lines on) its layout writes the further lines behind the data block into the following program. Measured in the host harness (screen 1050: window 1010, 115 lines, ~7.2 KB behind $39DA; screen 256: nothing); stack, a2–a6, pr_WindowPtr, locks and memory are in order on return. On the RTG Workbench (1680×1050) of the test emulator the program therefore crashed directly after the requester (Cancel as well as OK, t93/t94 with reqclose: reboot, earlier "ILLEGAL Instruction"). The port shows at most 64 lines, its window is at most 597 high (the first one too, and MaxHeight); test `freqlib-64-lines` (RTG variant of the emulator, a 1024×2048 screen). Names over 34 characters also overwrite their 40-byte list nodes (#159). Tests t93 (Cancel) and t94 (OK) in `tests/corpus/tests-req` via reqclose, therefore on a separate 320×256 screen (window 216, 16 lines; in the harness without overflow, Cancel as well as OK).

## 160 bitmaplib

C, declarations identical, binary = source. Object lib "BitMap" (64 bytes: struct BitMap + extra fields +52 interleaved, +54 plane distance, +56 width, +58 height, +60 colours, +62 0/1 borrowed/-1 own). BUG(orig): sizes as MULU word products (#76). Not a bug: CopyBitMap without setting Used, chip memory not cleared (every word is overwritten by the copy). Kept, not a bug: CludgeBitMap leaves the plane distance +54 in place (no library of the CD reads +54 of a non-interleaved bitmap; #77). Tests t120, t124, t125.

## 156 palettelib

C after the CD binary (newer than the source: Red/Green/Blue #long, make4bitrgbs takes the lower byte – fixed here, see below –, AGARed/Green/Blue check the screen and mask the colour number). Object lib "Palette" (128 bytes, data in LoadRGB32 format, 14 cycles). Blitz/Amiga variants ($a001). RGB in Amiga mode jumps directly into SetRGB4 like the original (a1 unchanged): with a1 = ViewPort the graphics.library of the emulator stored the value $8C instead of $88 for green 8 (t122, diagnostic line "raw") – an example of how an "unused" register changes the result of an OS call. Fixed (intended deviation, #224): DecodePalette sets only the upper byte, the CD binary's make4bitrgbs (agamode 0, Kickstart < 3.0) takes the lower byte: black; here make4bitrgbs takes the upper byte (test `palettelib-make4bitrgbs-lowbyte`). Fixed (intended deviation, #1002): DecodePalette and LoadPalette compared the CRNG flags word with 1 only, so a disabled range (flags 0 or 2) was entered and cycled backwards; here bit 0 (RNG_ACTIVE) decides whether a range is entered and bit 1 (RNG_REVERSE) its direction (test `palettelib-crng-flags`). BUG(orig): PaletteRange with start=end divides by 0 (#227); StopCycle does not reset cycon (#228); InitPalette without cycle end marker (#229). Not a bug (the results are right): FadePalette/MixPalette read 2 bytes too early. DIFF: runtime errors from normal code instead of a jump into the error check area. Tests t121–t123, t126–t129.

## 157 fadelib

C, declarations identical (`libheader{157`). Blitz mode only; 16 steps on the copper colour moves of the used slice. BUG(orig): `calcfade` shifts the whole word d0 with `LSR #4` (at the call palette# × 128, the compiler's object address): bits 8–11 end up in the red, bits 12–15 in the green fractional part of the **first** colour, after that the upper byte is empty (palette 0/1 not affected; reproduced register-exactly since the correction, before that every fractional part got the bits in C) (#134); the checks test the upper word of the pointers (#135); wrong messages: an uninitialized palette gives "Not enough colours in Slice" (`err2`, `mess3` "Uninitialized Palette" is never used) (#330), low/high beyond the slice's colours "Uninitialized Slice" (`err1`) (#331). Test t123 (ASyncFade On + DoFade, without timing; only palette 0, does not cover the bug).

## 150 blitlib / 146 bblitlib / 148 qblitlib / 122 stencillib

C, declarations identical, binaries = sources (+ include bliterrs). The routines program the blitter directly (volatile accesses in the order and width of the original); Blitz/Amiga variants ($a001): Amiga mode with OwnBlitter/wait/DisownBlitter. bblitlib saves the background into records in the buffer (UnBuffer, ShowStencil via stencil as B), qblitlib remembers the blits (UnQueue clears or copies from bitmap), stencillib = OR of all planes or cookie added (SBlit). BUG(orig): no clipping (only the checks), depth 0 = 65536 DBF iterations (#74, #243), overflow of Buffer/Queue only in the checks (#73, #423), FreeBuffer/FreeQueue leave the write position in place (#75, #244), stencil bltsize only up to 1024 rows (#284). Not bugs: Block at byte positions (word boundary blitting, as documented), BlitColl over all bitmap planes, FreeStencil leaves the memory pointer (the width is cleared). DIFF: b_doblitex does not return its register state to other libs; BlitColl in Amiga mode with upper word 0. Tests t130–t134 (t134 in Blitz mode), t135–t139 (tests-runerr). Note on test construction: `BitMap n,…` makes the new bitmap the used one – a BBlit with more planes than the bitmap writes, without error checking, via plane pointer 0 into low memory.

## 154 shapeslib

C, declarations identical, binary = source (incl. ANIM loader). Shape block 32 bytes (+0 width … +14 data CHIP, +18 cookie, +22/+24/+26/+28 sizes, +30 "cL" = data not owned). GetaShape (Amiga: OwnBlitter around the Blitz routine) lets bitmaplib create a bitmap and copies into it via scrolllib. LoadShape reads ILBM via ilbmifflib/iffiolib; every "ANHD" (op 5, interleave 1) yields a delta frame in the NEXT shape object. BUG(orig): MULU low words and ADDA.w steps (#263); BODY unchecked (#264); ANIM frames without maximum check (#265), XOR bit only from the first ANHD (#266); "cL" is taken over by CopyShape/LoadShapes and not cleared by freeashape (never freed) (#267, #268, #269). Fixed (intended deviation): LoadShapes/DecodeShapes computed the last shape as a word offset (`(max-1) << 5`, before the table from a maximum of 1025 on), now a long offset (#999, test `bug_orig_shapeslib_range_word_offset`). DecodeShapes with address 0 branches in the original into the error check area (err10); here err10 and its message are in normal code (#270, test `bug_orig_shapeslib_decodeshapes_null`). Likewise the messages of err2/err3/err7/err8 (normal code: SaveShapes, LoadShapes, LoadShape), in the original in the error check area (#953, test `bug_orig_shapeslib_messages_errarea`). Not bugs: only `depth` DLTA pointers (one per plane); DecodeShapes stops silently at the first non-matching block, in place below 2 MB (by design); ShapeWidth/Height load only d0.w (word results). Tests: t140, t144 (ANIM generated in the test), t145/t146/t149.

## 118 shapetrixlib

C, declarations identical, binary = source; `libincs:qrot` is missing on the CD, the sine table comes from the binary. XFlip shifts each plane with the blitter and then mirrors bytes and bits via CPU; YFlip swaps lines; Rotate (Quick 0..1 = 256 steps, back-rotation of each target pixel); Scale (size via lmullib, step via ldivlib, per target pixel the colour with the largest brightness sum). BUG(orig): XFlip does not wait after the last blit before the CPU mirrors – the result depends on timing (blitter against CPU); with the faster C code a 1-word shape differed, this case is removed from t141, the C version does not wait, like the original (#271); Scale reads beyond the shape at the edge (#273); word arithmetic for row/plane steps and sizes (#274). Fixed (intended deviation): Rotate rounded negative coordinates with -$8000 (one further from zero, the rotated shape one pixel too large); now +$8000 for both signs (#1004, test `bug_orig_shapetrixlib_rotate_rounding`). Kept, not a bug: the brightness table keeps old values (the colour choice compares only 0..numcols; #272). Test: t141.

## 112 spriteslib

C, declarations identical, binary = source. Sprite block 16 bytes; SpriteMode 0/1/2 = 16/32/64 pixels. ShowSprite/InFront/InFrontF/InFrontB Blitz mode only (odd error check address), not tested. BUG(orig): sizes when freeing/saving with the CURRENT SpriteMode (harmless, memlib takes the stored length) (#280); SpriteMode > 2 reads behind the width table (#281); GetaSprite reads whole sprite widths (#282); ShowSprite clips at the top with 4 bytes per line and adds the whole long register (#283). Fixed (intended deviation): LoadSprites computed the last sprite as a word offset (`(max-1) << 4`, before the table from a maximum of 2049 on), now a long offset (#999, test `bug_orig_spriteslib_loadsprites_word_offset`). The messages of err10/err11 (normal code: SaveSprites, LoadSprites) lie in the original in the error check area; here in normal code (#954, test `bug_orig_spriteslib_messages_errarea`). Tests: t142, t147.

## 104 collslib

C, declarations identical, binary = source. ShapesHit/ShapeSpriteHit/SpritesHit/RectsHit: rectangle test with signed word comparisons. The test (`chkifhit`, collslib.c:198-205) is asymmetric: it uses `<=` for x1+w1 against x2 and for y1+h1 against y2, but `<` for x2+w2 against x1 and y2+h2 against y1. Rectangles that only touch (e.g. A at x 30..49 and B at x 10..29) are therefore a hit when B lies left of or above A and no hit when B lies right of or below A, so `ShapesHit(A,B)` and `ShapesHit(B,A)` (likewise `RectsHit`) can differ for touching shapes. Kept as the original behaviour (possibly intended: it guarantees a gap on one side), not marked BUG(orig), not changed; locked in by t143 (`tests/corpus/expect/corpus/t143_colls.out`). SetColl/SetCollOdd/SetCollHi write CLXCON and read CLXDAT. SColl/PColl/DoColl Blitz mode only, not tested. BUG(orig): SColl calculates the CLXDAT bit number (BTST on the upper byte, bits 9–14 = sprite pairs, intended) correctly only if channel 0/1 is not the second argument: SColl(2,0)/(4,0)/(6,0) test bit 10/11/12 instead of 9/10/11, two channels of the same pair a foreign bit (#110); SetColl with 0 planes runs 65536 times (#111). Tests: t143, t148.

## 100 scrolllib / 119 2dlib / 121 polylib

C, declarations identical to the CD binaries. scrolllib = source except for clsnotint (binary: long operations when splitting the plane size, taken over). 2dlib = source `2dlib` (not sis2dlib.bb). polylib = source polylib (#chunkylib=121; renderlib is the same file with 87). 2dlib and polylib are plain C with the integer semantics of the original (wrapping word arithmetic, MULS/DIVS as instructions, CMP/BMI where the original tests the sign of a difference, the edge positions as its swapped fixed point numbers). Blitter routines are Blitz/Amiga variants ($A001); polylib never owns the blitter (like the original). The fill loop of FloodFill (yofill) is the original assembler, because it keeps the open lines unbounded on the 68000 stack. All routines leave a3 like the original.

BUG(orig): Box adds the bitmap origin twice (#391); Circle/Circlef with points above the top edge: MULU of the negative y, Plot far outside (#392); Poly with 1 point: 65535 iterations (#396); Polyf/BitMapPoly: overflow with more than 16 points or more than 10 edges per side (#395); the statement form of AllocFill takes a3 = string buffer as bitmap (#390); FreeFill leaves the buffer pointer in place (#394); depth 0: 65536 or 256 plane iterations (#393); polylib: no orientation check (#419), RenderToBitmap b,d + RenderPoly swaps the bitmaps (#421), RenderPoly without RenderTo… draws on a3 = string buffer (#420), RenderToChunky colours > 14 read behind the colour table (#422), ChunkyVlin without CLIP with y2 < y1 writes about 64K words (#417); BitMapPoly/ChunkyPoly/RenderPoly with an edge of 4096 lines or more that is at least as wide as high, or with points 32768 or more apart, draw a wrong shape or nothing (word arithmetic; won't fix, #1019, test `polylib-word-range`). Fixed (intended deviation): Polyf computed the slopes, the clipping and the orientation of the polygon with word arithmetic and drew nothing (or a wrong shape) for vertices far outside the bitmap (about +-8000 and beyond); a polygon of 3 to 16 points outside the range in which that arithmetic is exact (edges up to 8191, points up to +-16383) is now clipped to the bitmap with long arithmetic first, and the orientation compares exact products (#1000, test `bug_orig_2dlib_polyf_word_range`). Not bugs (no observable effect): Scroll writes the width of the 960 split back to the argument stack (it belongs to this call); left/top ClipBlit or right Scroll edge: the blitter reads/writes back one word before/behind the line unchanged. DIFF (memory layout, unavoidable): what an overflow of polypts/l1 hits (original: own code, C: the data behind it). Tests: t150–t157, t158/t159 (tests-runerr). Test construction: circles never above the top edge, Polyf ≤ 16 points, steep edges in polylib (DIVS overflow, #1019). Incidentally: t110–t112 are now silent (samples zeroed after the checksums).

## 190 gameiolib

C, declarations identical, binary = source. Joyx/Joyy/Joyr/Joyb/Gameb read JOY0DAT/JOY1DAT, CIAA PRA and POTGOR directly (Amiga and Blitz mode); Joyb tests with BTST on the byte $DFF016 (POTGOR bit 10/14). BUG(orig) (#415): Gameb port 0 jumps via DBF into the loop of port 1 and leaves it via its exit (POTGO $3000 instead of $0300). Not a bug (a word function): Joyr keeps the upper word of Joyy's MOVEQ. Test: t160 (without input; port 0 against our own decoding of JOY0DAT, Gameb with Disable), t165. Not tested: real inputs.

## 98 blitzkeyslib

C, declarations identical (binary has 6 more unused bytes). Init value = keyboard.device base; matrix from $136 (V36+) or $106, key buffer +$70/+$72/+$74. BUG(orig) (#408): BlitzKeys Off only resets the flag (handler remains). Not bugs (no observable effect): ison via NOT; entry -8 of the handler is not a BRA; word results keep the upper word of d0. Test: t161 (BlitzQualifier, RawStatus 0–127, BlitzRepeat, HandleRawKey). Not tested: BlitzKeys (Blitz mode), BlitzKey (writes the read index of keyboard.device).

## 94 animlib

C, declarations identical, binary = source. Object "Anim" (32 bytes), BODY and DLTA nodes via memlib, file via ilbmifflib/iffiolib. BUG(orig): BODY always as ByteRun1 (#397); freeanim reads the node after freeing (#399); a missing BODY gives length -1 (#400); a single delta gives current = 0 (#401); height/depth/width 0 run 65536 times (#402). Not bugs: wrap-around to delta 2 (the ANIM op-5 double-buffer loop convention); a3 stays on the last node or behind the data (statements; reproduced). Fixed (intended deviation, #398): the original's "Can't Open Anim File" is a BEQ into the error check area, err6 has its message there and continues into err1; here traps and messages are in normal code (bug-orig test `animlib-errors-check-area`). DIFF: a2 unchanged. Test: t162 (self-generated ANIM in RAM:, result = host model), t168.

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
