# rimisclib (47): miscellaneous commands

Reflective Images; the font prefs commands and the fixes of 1996/97 by
Steve Matty (header of the source). 24 commands:

* `EZRequest [title$,]body$[,gadget$]` and `EZRequest
  title$,body$,gadget$,var..` – EasyRequestArgs on the window of
  `EZOutput [win#]` (none: the default screen); `|` in the body becomes a
  line feed; the repeated arguments (`#unknown|#push`) are the format
  arguments. Gadget default "Ok". Below Kickstart 2.0 an AutoRequest of
  the library's own (Fixed, below).
* `EZRequestArgs args..` – the body is built from the arguments: `%s`,
  `%ld` (long, quick, float) or `%d` (word, byte) each, separated by the
  `EZSeparator char` character (default `|`, i.e. a line feed).
* `InitArgParse s$` copies s$ into a 256 byte buffer (at most 255
  characters, the rest is cut off); `NextArg$` gives
  the next word (spaces separate, quotes group, `""` inside quotes is a
  quote), `NextArgChar$ c` everything up to the next c.
* `ProgPath$`, `ProgDir$`, `ProgName$` – GetProgramDir/NameFromLock (no
  program dir: GetCurrentDirName) and GetProgramName (none: the task
  name), joined with `/` unless the directory ends in `:`. Below
  Kickstart 2.0 the current directory and the shell's command name
  (Fixed, below).
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
NDK inlines; a Quick or Float argument of EZRequest/EZRequestArgs is
converted to its integer part in C (the original called floatquicklib's
`fltoqu`, $D301). The repeated arguments are read below a2 (type word, value;
a string: length, address), the count from d7.w (the compiler's
`movenumrep`). The data keeps the original's order where it matters: the
0 in front of `tempprogname` ends ProgPath$'s backward search, the raw key
table has the original's 114 bytes, the EasyStruct is followed by the
argument pointer and the 16 longs of arguments. The thunks keep d1/a0/a1
(the statements that keep d0 also d0), the C code d2-d7/a2-a6; the
original changes d0-d3, d6/d7, a0-a3 and a6 in places. The original
has no error check area; the port has one (the header's error field)
with the two checks of #748 (`ezargschk`, `ezreqargschk`) and the two of
#1150 (`ezgad3chk`, `ezgad2chk`; `ezargschk` also checks the gadgets);
they are C behind `BLITZ_ERRCHK_THUNK` and keep d0-d1/a0-a1. The
declarations differ
from the reference there (`rimisclib.declarations-intended.txt`).

**DIFF:**

* a2 stays unchanged where the original uses it as a work register
  (EZRequest/EZRequestArgs set it to 0, BInstr leaves it on the string),
  and BInstr keeps a3 (the original leaves it on the search string);
  `docs/libs/cmdlibs-port.md` "General".
* EZRequestArgs writes its format text byte by byte; the original's
  `MOVE.w #"%s",(a1)+` after an odd number of bytes is an address error
  on a 68000. FromBlitz compares bytes where the original compares longs
  (`CMPM.l`, an address error on a 68000 for a task name at an odd
  address).

## Fixed

Intended deviations from the original (`docs/libs/deviations.md`,
kind F):

* NextArg$ takes `""` inside quotes as one quote and ends the word at
  the closing quote (`"a""b" c` gives `a"b`, then `c`); the original
  counted the pair as one quote, so the closing quote behind it copied
  the next character and the word went on (`a"b c`; at the end of the
  string it copied the 0) (#755). NextArg$ also stays on the 0 at the
  end of the string; the original stepped behind it.
* InitArgParse copies at most 255 characters and the 0 into its 256 byte
  buffer, the rest of a longer string is cut off; the original copies
  without a limit and clears four more bytes behind the copy, so a
  string of more than 251 characters overwrites the code behind the
  buffer (NextArgChar$ and NextArg$) (#752). The four zeros were an end
  guard for NextArg$, which no longer steps behind the 0 (#755).
* ProgPath$, ProgDir$, ProgName$ and DefaultFontName leave a3 on the 0
  of their result; the original copied the result with its 0 and left
  a3 behind it, so in a string expression the next part was written one
  byte too far (`ProgDir$+"/data"` held the 0 and lost the `a`) (#743).
* EZRequest and EZRequestArgs leave a3 on their first string argument
  (title$, body$ in the forms without a title, for EZRequestArgs the
  first string among its arguments; unchanged without one); the original
  left it on the argument array it passes to `EasyRequestArgs` (0 for
  the forms without one), so as a function in a string expression the
  rest of the expression was written from address 0 on or over the
  library's argument array (#1027). a3 on the first string argument
  gives back the space of the string arguments, which the compiler does
  not take back after a function with a numeric result (#846); the call
  itself uses the libraries' safe form of the NDK macro, which keeps a3
  (`cmake/ndk-calls.cmake`).
* WorkbenchHere ends at the end of the screen list and returns 0 when no
  title contains "Workbench"; a screen without a title is skipped. The
  original never tested for the end of the list (`'nextscreen: MOVE.l
  (a0),a0 : BRA 'loop`, its `'oops` unused): after the last screen it
  read the "screen" at address 0 and went on, so it hung or returned -1
  (#761).
* WorkbenchHere goes on with the next screen when a title ends during a
  partial match of "Workbench"; the original restarted the same title
  (`TST.b (a1) : BEQ 'loop`, a0 unchanged), so a title that ends with
  the start of "Workbench" ("My Work") hung it (#762).
* WorkbenchHere searches each title from every character on; the
  original went on with the next screen at the first mismatch after a
  "W" (`CMP.b (a1)+,d0 : BEQ 'innerloop`, then `'nextscreen`), so
  "A Window on Workbench" or "WWorkbench" was not found (#760).
* WorkbenchHere searches the screens of the library's IntuitionBase; the
  original took it from d0 (`MOVE.l intuibase,a0` overwritten by
  `MOVE.l d0,a0`), and the function has no arguments: it searched the
  "screen list" of whatever the program left in d0 (#759).
* Flip$ of a string of 0 or 1 characters returns it unchanged; in the
  original the count `len/2 - 1` is -1, the DBRA swapped 65536 byte
  pairs around the string (#751).
* EZRequestArgs ends the format text it builds (160 byte buffer) with a
  0; the original wrote none, so a shorter text than the one before kept
  the end of the old one, and so more formats than arguments (#747).
* A Quick or Float argument of EZRequest/EZRequestArgs is passed as its
  integer part truncated towards 0, a long for `%ld` (-3.5 gives -3, as
  `Int` does); the Float is converted in C. The original passed the
  integer part zero extended (`SWAP: ANDI.l #$ffff`: -3.5 gave 65532)
  (#750), and converted
  a Float with floatquicklib's fltoqu with the caller's a6: the commands
  load no library base, fltoqu needs mathffp's there; the converted
  value was written back onto the argument stack (#749).
* ProgPath$ without a directory name (NameFromLock or GetCurrentDirName
  failed, e.g. for a path longer than 255 bytes) is the program name
  alone; the original decided on the "/" by the byte before its result
  (`CMPI.b #":",-1(a3)`), so it gave "/name", or "name" behind a ":"
  (#758).
* OpenFontPrefs returns 0 when the AllocMem of its 16 KB buffer fails;
  the original did not check it and read `ENV:SYS/Font.Prefs` to address
  0, over the exception vectors and the ExecBase pointer (#756).
* OpenFontPrefs frees the buffer of an OpenFontPrefs before; the original
  lost it, 16 KB of AllocMem until the next reboot (#757).
* DefaultFontHeight/Name go through the chunks from the first FONT chunk
  by their sizes (padded to even) as far as the bytes OpenFontPrefs read,
  and take only FONT chunks; OpenFontPrefs checks the PRHD size against
  the bytes read and fails on a read error. The original stepped through
  the whole 16 KB buffer by 164 bytes (the size of a FONT chunk), also
  through the uncleared memory behind the file, where a type not in the
  file was "found", and read the last step up to 26 bytes behind the
  buffer (#745).
* ASCRaw of a raw key outside 0..113 (a key up code 128-255, a negative
  value) gives the character 0, as the table's keys without a character
  do; RawASC of a character that is not in the table (every upper case
  letter, `!`, ...) and of "" gives -1 (as `Asc("")` does). The original
  checked neither: ASCRaw read the code behind (or before) its table
  (`_ltos`, `_fromblitz` ..., partly relocated addresses), RawASC
  searched on behind it until it found the byte somewhere, or for ever,
  and found "" (its 0) at 14 (#744).
* NextArgChar$ before InitArgParse and after NextArg$ came to the end
  gives "", as NextArg$ does; the original did not check the parse
  pointer and read from address 0 on (an empty result only while the
  long at 0 is 0) (#754).
* EZRequest title$,body$,gadget$,var.. and EZRequestArgs store their
  arguments in 16 longs (a string, long, Quick or Float a long, a word
  or byte a word) without a limit; the original overwrites the code
  behind them (WorkbenchHere's). An invalid argument (the number of
  arguments is in the source): the commands are unchanged, and with
  runtime error checking more than 64 bytes of arguments is the error
  "Too Many EZRequest Arguments" (#748). Within 64 bytes the format text
  of EZRequestArgs (160 bytes) cannot overflow.
* EZRequest and EZRequestArgs work below Kickstart 2.0 (intuition.library
  below V36, which has no `EasyRequestArgs`): the library formats body
  and gadgets with the arguments itself (exec's `RawDoFmt`, the gadgets
  with the arguments behind the body's, as `EasyRequestArgs` does),
  makes one IntuiText per line of the body (`|` and line feeds break
  it; topaz 8) and calls `AutoRequest` on the EZOutput window (none: the
  Workbench screen) with one gadget (the negative one) or two (left
  positive, right negative). The title is not shown (AutoRequest has
  none). The result is that of `EasyRequestArgs`: 1 for the left of two
  gadgets, 0 for the right or only one. With more than two gadgets no
  requester appears and the result is 0; with runtime error checking it
  is the error "More than two EZRequest gadgets need Kickstart 2.0"
  (`ezgad3chk`, `ezgad2chk`, `ezargschk`). Without memory for the
  texts: no requester, 0. The original called `EasyRequestArgs` there,
  behind intuition's jump table: a crash at the first requester (#1150;
  tests `tests/bug-orig/rwe/rimisclib-v36-ezrequest`,
  `rimisclib-v36-ezrequest-gadgets`, a500; the user's decision of
  2026-11-20).
* ProgPath$, ProgDir$ and ProgName$ work below Kickstart 2.0 (dos.library
  below V36, which has none of `GetProgramDir`, `GetCurrentDirName`,
  `NameFromLock`, `GetProgramName`): the directory is the current
  directory, its path built by the library from the names of its
  directories up to the root (`Examine`, `ParentDir`: `RAM
  DISK:pp/sub` on Kickstart 1.3; 254 characters at most, longer or a failure: empty), the
  name the last part of the shell's command name (`cli_CommandName`;
  without a shell the task name). Difference to Kickstart 2.0: a
  program started with a path from another directory (`dh0:tools/prog`
  from `dh0:`) gets the current directory as ProgDir$, not its own
  directory; from Workbench, which makes the program's directory the
  current one, both agree. The original called the V36 functions there,
  behind dos's jump table: a crash (#1150; test
  `tests/bug-orig/rwe/rimisclib-v36-progpath`, a500; the user's decision
  of 2026-11-20).

## BUG(orig)

* EZRequestArgs without an argument reads one type word from a2 (the
  count 0 runs the DBRA 65536 times) (#746).

Original behaviour that is no defect:

* LtoS replaces the bytes 0-31 and 128-255 by 128 (`CMPI.b #32` with a
  signed branch), as RWE's manual says (RIMiscLib.guide, node LtoS:
  "Non-ASCII characters are replaced with character 128") (#753).

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
right after it); PortSigBit; WorkbenchHere on 1-4 fake screens, passed
in d0 as a fake IntuitionBase (the original's) and set as the system's
FirstScreen for the call under Forbid (the C build's) (found first,
later, after partial matches, empty title; d0 another IntuitionBase,
#759; in the C build only, where the original does not return: no
match, a title that ends in a partial match, a mismatch before the
match, a screen without a title); EZOutput both forms; EZRequest all four forms – the body buffer
after the `|` change, the argument stack (the original writes the
Floats back), all argument types (string, long, word, byte, Quick also
negative, Float with a6 = mathffp and with a6 another library), 16
longs; EZRequestArgs with the same,
EZSeparator `,`, `""` and `|`, the stale end of the format text;
Flip$ with 2..250 characters and with 0 and 1 (a 140 KB buffer each);
InitArgParse/NextArg$/NextArgChar$ on 21 lines (spaces, quotes, `""`,
unterminated quotes, separators, mixed), also before InitArgParse and
after the end (address 0; once with "xy" at address 0 for the call,
under Disable); ProgPath$/ProgDir$/ProgName$ (normal, no
program name: the task name, no program dir: the current directory);
FromBlitz with 9 task names; LtoS; ASCRaw over the whole table; RawASC
for every character of the table (its 0 is "": -1 in the C build); in
the C build only ASCRaw of 8 keys outside the table and RawASC of 4
characters not in it; the font prefs (not open, no file, bad
FORM/PREF/PRHD/FONT, a PRHD of 10 bytes, types 0-3 and 7 – one the original
finds in the padding behind the chunks, #745 –, d0's high word, CloseFontPrefs twice);
BInstr (12 pairs); finit with the font prefs open (both buffers freed).
Besides these: the argument array and body text of one call,
the font of type 1/2, FromBlitz -1. Kept registers on every call.

Not covered: ClickWait (needs the mouse button); WorkbenchHere's endless
cases in the original; ASCRaw/RawASC outside the table in the original
(they read the code behind it, which differs from the C build's); EZRequestArgs without
arguments; strings longer than the buffers; OpenFontPrefs when AllocMem
fails.
