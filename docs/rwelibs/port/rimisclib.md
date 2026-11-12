# rimisclib (47): miscellaneous commands

Reflective Images; the font prefs commands and the fixes of 1996/97 by
Steve Matty (header of the source). 24 commands:

* `EZRequest [title$,]body$[,gadget$]` and `EZRequest
  title$,body$,gadget$,var..` – EasyRequestArgs on the window of
  `EZOutput [win#]` (none: the default screen); `|` in the body becomes a
  line feed; the repeated arguments (`#unknown|#push`) are the format
  arguments. Gadget default "Ok".
* `EZRequestArgs args..` – the body is built from the arguments: `%s`,
  `%ld` (long, quick, float) or `%d` (word, byte) each, separated by the
  `EZSeparator char` character (default `|`, i.e. a line feed).
* `InitArgParse s$` copies s$ into a 256 byte buffer; `NextArg$` gives
  the next word (spaces separate, quotes group, `""` inside quotes is a
  quote), `NextArgChar$ c` everything up to the next c.
* `ProgPath$`, `ProgDir$`, `ProgName$` – GetProgramDir/NameFromLock (no
  program dir: GetCurrentDirName) and GetProgramName (none: the task
  name), joined with `/` unless the directory ends in `:`.
* `OpenFontPrefs` reads `ENV:SYS/Font.Prefs` into a 16 KB buffer (-1 if
  it is FORM PREF with PRHD and a FONT chunk), `DefaultFontHeight(type)`
  / `DefaultFontName(type)` search its FONT chunks for fp_Type (0 icon, 1
  system, 2 screen), `CloseFontPrefs` frees the buffer.
* `Flip$(s$)` (reversed), `LtoS(l)` (the four bytes as a string),
  `ASCRaw(rawkey)` / `RawASC(char$)` (a table of raw key codes),
  `BInstr(s$,f$)` (the last position of f$ in s$, 1 based), `PortSigBit`,
  `FromBlitz` (the task name starts with "Blitz ][ Program Proc"),
  `WorkbenchHere` (a screen title contains "Workbench"), `TimerReset`
  (CIA-A TOD to 0), `ClickWait` (left mouse button pressed and released).

init opens dos.library and intuition.library (any version); finit closes
both and frees the font prefs buffer.

* **Source:** `rwelibsrc/misclib/misclib2.bb` (1997-03-13; as TED shows
  it: `build/amiga/rwelibsrc/misclib/misclib2.bb`). It is newer than the
  CD's BSS2 `RIMISCLIB.OBJ` (no `NextArgChar$`/`BInstr`, the older
  `NextArg$`), and AmiBlitz3's `Rimisclib.obj` is an extension of it
  (`docs/rwelibs/README.md` 3.2).
* **Reference:** the reference build of the source by blitz2 2.10
  (`refbuild`, ctest `rwe_refbuild`: `build/amiga/libs/userlibs/rwe/ref/rimisclib.obj`);
  declarations equal (ctest `rwe_declarations_rimisclib`).
* **C file:** `libs/userlibs/rwe/rimisclib.c`; test
  `libs/tests/rwe/rimisclib.c` (snitch case "rwe rimisclib").

## C and assembler

All routines are C behind `BLITZ_THUNK`; dos and intuition through the
NDK inlines, floatquicklib's `fltoqu` ($D301, a Float argument of
EZRequest/EZRequestArgs) through `BLITZ_ALIBJSR_A6` with the a6 the
routine got. The repeated arguments are read below a2 (type word, value;
a string: length, address), the count from d7.w (the compiler's
`movenumrep`). The data keeps the original's order where it matters: the
0 in front of `tempprogname` ends ProgPath$'s backward search, the raw key
table has the original's 114 bytes, the EasyStruct is followed by the
argument pointer and the 16 longs of arguments. The thunks keep d1/a0/a1
(the statements that keep d0 also d0), the C code d2-d7/a2-a6; the
original changes d0-d3, d6/d7, a0-a3 and a6 in places.

**DIFF:**

* a2 stays unchanged where the original uses it as a work register
  (EZRequest/EZRequestArgs set it to 0, BInstr leaves it on the string),
  and BInstr keeps a3 (the original leaves it on the search string);
  `docs/libs/cmdlibs-port.md` "General". EZRequest/EZRequestArgs leave a3
  = the argument array (0 without) like the original: the NDK inline of
  EasyRequestArgs loads the fixed register a3 and does not restore it.
* EZRequestArgs writes its format text byte by byte; the original's
  `MOVE.w #"%s",(a1)+` after an odd number of bytes is an address error
  on a 68000. FromBlitz compares bytes where the original compares longs
  (`CMPM.l`, an address error on a 68000 for a task name at an odd
  address).
* ASCRaw/RawASC outside the table: the original reads the code behind its
  table (`_ltos`, `_fromblitz` …, partly relocated addresses), the C
  build whatever is behind its table – a raw key above 113 (e.g. a key up
  code) or a negative one, and RawASC of a character that is not in the
  table, give other results.
* InitArgParse with more than 251 characters overwrites the code behind
  the buffer in the original, other data in the C build.

## BUG(orig)

* WorkbenchHere takes IntuitionBase from d0 (`MOVE.l intuibase,a0` is
  overwritten by `MOVE.l d0,a0`) and has no arguments: d0 is whatever
  the program left there (#759). The search never ends the screen list
  (no 0 check: it reads the "screen" at 0 and goes on), so it returns -1
  or hangs (#761); a title that ends with the start of "Workbench"
  ("…Work") restarts the same screen for ever (#762); after a partial
  match it goes on with the next screen (a title "WWorkbench" does not
  count) (#760).
* Flip$ of a string of 0 or 1 characters: the count `len/2 - 1` is -1,
  the DBRA swaps 65536 byte pairs around the string (#751).
* InitArgParse copies without a limit (256 byte buffer, 4 zeros behind
  the copy) (#752). NextArgChar$ does not check the parse pointer: before
  InitArgParse and after NextArg$ came to the end it reads from address
  0 (#754).
* NextArg$: after `""` inside quotes the next quote copies the character
  behind it and the word goes on (`"a""b" c` gives `a"b c`) (#755).
* EZRequestArgs writes no 0 behind its format text (160 byte buffer): a
  shorter text than the one before keeps the end of the old one (and so
  more formats than arguments) (#747). Neither form limits the number of
  arguments (16 longs, the original overwrites code behind them) (#748),
  and EZRequestArgs without an argument reads one type word from a2 (the
  count 0 runs the DBRA 65536 times) (#746).
* A Quick argument is passed as its integer part zero extended (`SWAP:
  ANDI.l #$ffff`): -3.5 gives 65532 (#750). A Float is converted with
  floatquicklib's fltoqu with the caller's a6 (fltoqu needs the ffp base
  there), and the converted value is written back onto the argument
  stack (#749).
* ProgPath$ leaves a3 behind the 0 of the result (d0 is right) (#743) and
  reads the byte before the result when the directory name is empty
  (#758); ProgDir$/ProgName$ leave a3 behind the 0 too (#743).
* LtoS replaces the bytes 0-31 and 128-255 by 128 (`CMPI.b #32` with a
  signed branch) (#753).
* ASCRaw/RawASC do not check the range (see DIFF); RawASC searches until
  it finds the character somewhere (#744).
* OpenFontPrefs does not check AllocMem (#756); a second OpenFontPrefs
  loses the first buffer (#757). DefaultFontHeight/Name step through the
  whole buffer by 164 bytes (the size of a FONT chunk), also through the
  uncleared memory behind the file, and read the last step up to 26
  bytes behind the buffer (#745).

Original behaviour that is no defect:

* NextArg$ ends a quoted word at the closing quote even without a space
  behind it (`"ab"cd` gives `ab`, then `cd`), as ReadItem does.
* NextArg$/NextArgChar$ write no 0 behind the result: a string result
  needs none (d0 has the length; where the result is passed on, the
  compiler adds the 0; see #257).
* EZRequest/EZRequestArgs change `|` to a line feed in the body string
  they get, which is the compiler's copy of the argument.
* ProgDir$/ProgName$ call ProgPath$ only while their buffer is empty (the
  result is never updated; the program's directory and name do not
  change).
* DefaultFontHeight keeps d0's high word (the result is a word).
* finit closes intuition.library and dos.library without clearing the
  bases (CloseLibrary(0) for a missing dos.library); nothing uses them
  after finit.

## Test

Each run loads one library set (the C build or the original); the
scenario writes its observations as cases, compared with the expected
data `libs/tests/expected/rwe_rimisclib.txt` recorded from the original.

EasyRequestArgs is replaced (SetFunction) by a stub that records window,
EasyStruct, its three strings and 16 longs of arguments and returns a
chosen value; GetProgramName/GetProgramDir are replaced by `moveq #0,d0`
for ProgPath$'s fallbacks; the font prefs are files the test writes to
`ENV:SYS/Font.Prefs`, with `ENV:` assigned to a directory of its own
(`RAM:rimisclib-env`) for that part: the system's IPrefs watches the real
file and reads every version of it (Workbench 3.1's keeps the one with
the bad PRHD in use, every later write fails). Observed:
init (d0 = IntuitionBase); TimerReset (the TOD
right after it); PortSigBit; WorkbenchHere on a fake IntuitionBase with
1-4 fake screens (found first, later, after partial matches, empty
title); EZOutput both forms; EZRequest all four forms – the body buffer
after the `|` change, the argument stack (Floats written back), all
argument types (string, long, word, byte, Quick also negative, Float
through fltoqu with a6 = mathffp), 16 longs; EZRequestArgs with the same,
EZSeparator `,`, `""` and `|`, the stale end of the format text;
Flip$ with 2..250 characters and with 0 and 1 (a 140 KB buffer each);
InitArgParse/NextArg$/NextArgChar$ on 21 lines (spaces, quotes, `""`,
unterminated quotes, separators, mixed), also before InitArgParse and
after the end (address 0); ProgPath$/ProgDir$/ProgName$ (normal, no
program name: the task name, no program dir: the current directory);
FromBlitz with 9 task names; LtoS; ASCRaw over the whole table; RawASC
for every character of the table; the font prefs (not open, no file, bad
FORM/PREF/PRHD/FONT, a PRHD of 10 bytes, types 0-3 and 7 – one found in
the padding behind the chunks –, d0's high word, CloseFontPrefs twice);
BInstr (12 pairs); finit with the font prefs open (both buffers freed).
Besides these: the argument array and body text of one call,
the font of type 1/2, FromBlitz -1. Kept registers on every call.

Not covered: ClickWait (needs the mouse button); WorkbenchHere's endless
cases; ASCRaw/RawASC outside the table (DIFF); EZRequestArgs without
arguments; strings longer than the buffers; OpenFontPrefs when AllocMem
fails.
