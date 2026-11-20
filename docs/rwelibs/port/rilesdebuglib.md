# rilesdebuglib (49): LES debug

The helper library of RWE's debugger program "REDDebugger" (Leading Edge
Software; the program itself is not part of the release). 33 commands:

* Memory and registers as text: `D_GetSR buffer,SR` ("XNZVC" with `-`
  for the clear bits), `D_HexWord source,address,num` (num words as
  "XXXX "), `D_CheckNMove`/`D_CheckNMoveA`/`D_CheckNMoveWord
  source,dest[,count]` (longs as 4 characters with a byte left between
  them; A: no 0 characters; Word: words as 2 characters, no gap),
  `GetHex$(value,mode)` / `GetHexAdr$(address,mode)` (2, 4 or 8 hex
  digits), `PeekMax$(address,max)`, `GetDataItem(address,size)`. The
  newer routines show addresses below $420 as `*` (GetDataItem: 0).
* The font printer for bitmaps of 80 bytes per row: `FLocate x,y`
  (column, row of 8 pixels), `FPrint a$`, `FNPrint a$`, `FInverse n`,
  `FYOffset pixels`, on plane 0 of the used bitmap (`#ua0`), with the
  library's 8×8 font.
* `FPrintDetoke line,cio,typ,lineoffset` prints a tokenised source line:
  tokens (a byte with bit 7 and the next) as their names from the token
  list `SetupLESDebug token_addr,tokenise` set (nodes {next.l, token.w,
  name}; with tokenise also into plane 1, i.e. in another colour),
  `?????` for an unknown token; before the character at offset cio a
  marker – `>` (typ 0) or an inverse space; lineoffset characters
  skipped from the left.
* `GetCopIns$(copdata)` – a copper instruction as "MOVE vvvv to NAME" or
  "WAIT/SKIP (xx,yy) MASK (xx,yy) [BFD]"; `GetCustomReg(offset)` /
  `GetCustomRegAdr(name$)` – the custom chip register names $000-$1FE.
* `OutText x,y,rastport,string` (Move to y + the font's baseline, Text),
  `OutTextPen rastport,pen` (SetAPen).
* `D_InitParse s`, `D_NextArg$` – comma separated arguments;
  `ProcessString` replaces `%p` ("REDDebugger"), `%c` (the token list's
  address in hex, from SetupLESDebug), `%%` and up to 20 entries of
  `AddProcessPtr char$,string` (`ClearProcessPtrs`) in the parser's text.
* `SearchMem(type,value,start,end)` (1 byte, 2 word, 3/5/6 long, 4
  string; the space bar aborts) with `SearchMemFound`,
  `D_FindStringPtr(areg5,lookfor)`, `FindLineToken(line)`,
  `ExtractLabel(string)`, `D_FindVolume(name)` (FindDosEntry; below
  dos.library V36 a walk of the device list under `Forbid`).

init opens graphics.library (any version) and sets the `%p`/`%c`
entries; finit closes it.

It is a library of ordinary commands: it has no error checks and does
not talk to blitz2's runtime debugger (unlike `ridebuglib`, 51). The
program it was written for hands it the debugger's token list and the
tokenised lines; that program is not in the release.

* **Source:** `rwelibsrc/LESDebuglib/LESDEBUGlib.bb` (1996-12-01; as TED
  shows it: `build/amiga/rwelibsrc/LESDebuglib/LESDEBUGlib.bb`). The older
  `Debuglib/LESDEBUGlib.bb` (1996-09-17) lacks FindLineToken, GetDataItem,
  the `*` of D_CheckNMoveA, FPrintDetoke's fix for rows beyond 407 pixels
  and its case of a bitmap without plane 1; its declarations equal
  AmiBlitz3's `RILESDEBUGLib.obj` (11632 bytes), whose code is yet another
  version (more code, e.g. in `draw_char`). The CD's BUM9B/BUM10A `RILESDEBUGLIB.OBJ` (8476 bytes)
  is older than both sources (28 entries). No binary has the state of
  either source: the reference build of the older source (9524 bytes)
  equals neither binary.
* **Font:** both sources `IncBin "Blitz2:Debugger/Font.bin"`, a file
  that exists nowhere. The 2048 bytes (256 characters, 8 rows each) are
  in the binaries at the place of the IncBin, between the code of
  `draw_char` and `_GetHexAdr` – the same bytes in the CD's and
  AmiBlitz3's builds, and in AmiBlitz3's disassembly
  `rilesdebuglib.ab3` (its commented `Dc.l` lines). AmiBlitz3 publishes
  both in its GitHub repository (Apache License 2.0); only the font is
  taken over (decision of 2026-10-30, not the library's binary):
  `libs/userlibs/rwe/rilesdebuglib-font.bin`, cut from
  `BlitzLibs/Decompiled/thirdpartylibs/RILESDEBUGLib.obj` of commit
  `b1a9dee` at offset 4566; the provenance is in `archive/README.md`.
  ctest `rwe_lesfont_cd` compares it with the CD's BUM
  `RILESDEBUGLIB.OBJ` at offset 3460 (skipped without the CD;
  `rwe_lesfont_broken` is the check with broken input). The C file
  embeds it (`#embed`, `static_assert` on its size); the reference build
  stages it as `BLITZ2/Debugger/Font.bin`, added to `blitz2:`.
* **Number:** `#riLESDEBUGlib` = 49 (the binaries; the constant in the
  source's case, as blitz2's constants are case-sensitive:
  `LIB_riLESDEBUGlib` in `libs/variants/rwe-1999/libnums.h`).
* **Reference:** the reference build of the newer source by blitz2 2.10
  (`refbuild`, ctest `rwe_refbuild`:
  `build/amiga/libs/userlibs/rwe/ref/rilesdebuglib.obj`); declarations
  equal (ctest `rwe_declarations_rilesdebuglib`).
* **C file:** `libs/userlibs/rwe/rilesdebuglib.c`; test
  `libs/tests/rwe/rilesdebuglib.c` (snitch case "rwe rilesdebuglib").

## C and assembler

All routines are C behind `BLITZ_THUNK`; graphics and dos through the
NDK inlines, RawStatus (blitzkeyslib, $3102) through `BLITZ_ALIBJSR`.
FPrint reads the string length below a2; FPrintDetoke gets d6 as a
register parameter, because the original keeps the caller's high word of
its column counter (it matters for one flag test, see BUG(orig)). The
word `inverse` is tested as a word by FPrint and by its high byte in
FPrintDetoke (`INVERSE_HI`). The `%` entries are one packed array
(2 fixed, 20 for AddProcessPtr, the end) like the original's
`stringtokens`/`userstringtokens`. The thunks keep d1/a0/a1 (the
statements also d0), the C code d2-d7/a2-a6; the original changes d0-d3,
d5-d7, a0-a3 and a6 in places.

**DIFF:**

* a2 and a3 stay unchanged where the original uses them as work
  registers (FPrint: a2 = the font, a3 = the glyph; SearchMem: a2 = the
  end, a3 in the string search; GetCustomRegAdr: a2);
  `docs/libs/cmdlibs-port.md` "General".
* FPrintDetoke's inverse drawing computes the inverted bytes; the
  original inverts the glyph in the font itself (`NOT.l`) and back. Same
  output.
* AddProcessPtr's 21st entry overwrites the code of D_InitParse in
  the original, other data in the C build.

## Fixed (kind F, docs/libs/deviations.md 3a)

* `D_FindVolume` works on Kickstart 1.x; from dos.library V36 on it stays
  as it was (`LockDosList`, `FindDosEntry`, `UnLockDosList`). The
  original called those V36 functions on the program's dos.library
  without a version check: on Kickstart 1.x the offsets lie behind the
  jump table and the program crashed at `D_FindVolume`. Below V36 the
  command walks the DOS device list of `DosInfo` under `Forbid`, as
  programs did before `LockDosList` existed (devices, volumes and
  assigns; names compared without case, as `FindDosEntry`; the helper
  `bl_finddosentry_v34` of `libs/include/blitzdos.h`, shared with
  riamosfunclib), and returns the entry's address after `Permit` - the
  same kind of value as from V36 on, an entry of a list the program no
  longer holds (#1148; test `rilesdebuglib-v36-calls` on the a500,
  a500-204 and a1200).
* `GetCustomReg` compares the register word unsigned with $1FC and looks
  at the word only. The original compared the word signed and tested the
  whole long for a negative value: a word of $8000 or more with d0.l not
  negative passed, and the name was copied from up to 32 KB in front of
  the name table (the code of GetCopIns$ in the original). (#714; test
  `rilesdebuglib-getcustomreg-negative-word`; the `libs_emulator` cases
  `[fixed #714]` of $FFFF0010 and $80000000, whose words name $010 and
  $000.)
* `GetCustomReg` and `GetCustomRegAdr` take the last register of the
  name table, $1FE `NOOP`, too (`GetCustomReg($1FE)` is "NOOP",
  `GetCustomRegAdr("NOOP")` 510). The original ended both at $1FC
  (`CMPI.w #$1fc`): $1FE had no name and "NOOP" was not found (#702;
  test `rilesdebuglib-customreg-noop`; the `libs_emulator` cases
  `[fixed #702]`, also of $1FD, an odd offset inside the range).
* `GetCustomReg` copies at most 8 characters of the name. The original
  stopped only after a 9th: a name of 8 characters (BLTCON0L, DENISEID,
  the SPRxDATA/B) got the first character of the next name behind it and
  the length 8, a3 one behind the result, so a string expression behind
  it lost its next part (`GetCustomReg($5A)+"|"` gave "BLTCON0LB") (#712;
  test `rilesdebuglib-getcustomreg-eight-chars`; the `libs_emulator`
  cases `[fixed #712]`).
* `GetCustomRegAdr` finds a register only by its whole name (up to the
  table's space, at most 8 characters). The original matched a prefix -
  the string could go on behind the name (`DMACONXYZ` gave $096, `STRX`
  and `STRHOR` gave $038, STR) - and never found a name of 8 characters
  (DENISEID, SPR7DATB, BLTCON0L gave BLTCON0's $040): its compare ran
  into the next name (#713; test `rilesdebuglib-getcustomregadr-prefix`;
  the `libs_emulator` cases `[fixed #713]`).
* The "below $420" tests (D_HexWord, D_CheckNMoveA, D_CheckNMoveWord,
  GetDataItem) compare unsigned. The original's `CMP.l`/`BLT` took every
  address from $80000000 for low memory too: `*` in place of the bytes,
  0 from GetDataItem - on a machine with 24 address lines the same
  memory as the low 24 bits, the ROM at $FFFFFFF0 among it (#717; test
  `rilesdebuglib-lowmem-signed` on the a500; the `libs_emulator` cases
  `[fixed #717]` of $FFFFFFF0).
* A count of 0 makes D_HexWord, D_CheckNMove, D_CheckNMoveA and
  D_CheckNMoveWord write nothing, D_CheckNMoveWord counts its bytes
  (count*2) as a long, and GetDataItem reads a long for every size but 1
  and 2, as its help text says ("otherwise long"). The original's DBRA
  ran 65536 times for a count of 0 (327680 bytes written by D_HexWord and
  the D_CheckNMove forms; the debugger computes the count from the
  window's width), D_CheckNMoveWord's word byte count did the same for
  $8000 and cut counts above it, and GetDataItem shifted 65536 bytes
  through the result for size 0, a negative size fewer (#701; test
  `rilesdebuglib-count-zero`; the `libs_emulator` cases `[fixed #701]` of
  GetDataItem's sizes 0 and -1).
* `PeekMax$(address,max)` copies at most max characters, none for a max
  below 1. The original tested the count after the copy: it wrote max+1
  characters and returned the length max, a3 one behind the result, so
  a string expression behind it lost its next part
  (`PeekMax$(&t$,4)+"|"` gave "abcde") (#718; test
  `rilesdebuglib-peekmax-extra-char`; the `libs_emulator` cases
  `[fixed #718]`).
* `ExtractLabel` gives the empty string for a line without a `.` behind
  its first 9 characters, or one shorter than 9 characters. The original
  searched the `.` without a limit, from the 10th character on - also
  behind a 0 among the 9 it skips - and took the label from the memory
  behind the string (#703; test `rilesdebuglib-extractlabel-dot-unbounded`;
  the scenario of `libs_emulator` has no such line).
* A space ends `ExtractLabel`'s label too, as the source says
  (`CMPI.b #" ",d2`; `ExtractLabel` of "012345678.a b" is "a"). In every
  binary a space did not end it: the CD's blitz2 2.10 assembles that
  line as `CMPI.b #0,d2` (the reference build, the CD's and AmiBlitz3's
  binaries). This is compiler bug #343, fixed in blitz2-c; the reference
  build is still made by the CD's compiler
  (`libs/userlibs/rwe/refbuild.cmake`), so the recorded original keeps
  it. The later line `MOVE.w #">"*8,d0` (line 1016 of the source)
  triggers it; the operands are evaluated last line first, so what
  matters is the line's position, not the destination register (`#" "`
  in GetCustomReg, which comes after that line, is $20) (#704; test
  `rilesdebuglib-extractlabel-space`; the `libs_emulator` cases
  `[fixed #704]` of "012345678.a b" and "012345678 more.name end").
* `D_InitParse` copies at most 255 characters into its 256 byte buffer,
  with a 0 behind them. The original copied without a limit: a longer
  string - the debugger passes lines of its menu file - overwrote the
  parser's pointer and the code behind the buffer (in the C build other
  data) (#716; test `rilesdebuglib-initparse-overflow`; the scenario of
  `libs_emulator` has no string that long).
* `ProcessString` gives the empty string when the parser has no position
  (before D_InitParse, after the last D_NextArg$), as D_NextArg$ does.
  The original read the text at address 0. It works on the parser's
  current position by design (a D_NextArg$ before it leaves less; the
  debugger calls `D_InitParse s$ : a$=ProcessString`): its help text
  "string" named an argument that the command never had and is now
  "- the text of D_InitParse with %x replaced" (a declaration change,
  `rilesdebuglib.declarations-intended.txt`) (#719; test
  `rilesdebuglib-processstring-no-position`; the `libs_emulator` cases
  `[fixed #719]` "no parser position").
* A `%` at the end of `ProcessString`'s text ends it. The original took
  the 0 behind the `%` as the name of an entry, found none and read on
  behind the text (the rest of a longer text the parser's buffer held
  before; the debugger's menu file lines can end so) (#720; test
  `rilesdebuglib-processstring-trailing-percent`; the scenario of
  `libs_emulator` has no such text).
* `SearchMem`'s word and long searches (types 2, 3, 5, 6) find a value
  only if it lies in front of the end address (the end moved by 1 or 3
  once in front of the loop; a range shorter than the value finds
  nothing). The original tested only the value's start against the end
  and found a word or long that went on behind it, comparing bytes up to
  1 or 3 behind the end (#722; test `rilesdebuglib-searchmem-across-end`;
  the `libs_emulator` cases `[fixed #722]` "word across the end" and
  "long across").

## BUG(orig)

* D_HexWord at an odd source writes "XX YY" per word, without the space
  the even case writes behind each word (#715).
* GetCopIns$ masks bit 0 of both digits of a horizontal position (WAIT,
  SKIP and their masks): odd digits never show (#711).
* FPrint/FPrintDetoke: the column test is `CMPI.b #80` with `BPL`, so
  columns 80-207 stop, 208-255 print (and go on to 0-79); nothing checks
  the bitmap's size; in FPrintDetoke the marker takes a column without
  this test (#707). FPrint ignores strings of 32769 characters or more
  (`SUBQ.w #1`/`BMI`) (#710).
* FInverse sets a word; FPrint tests the word, FPrintDetoke its high
  byte (FInverse 1 inverts only FPrint), and FPrintDetoke's marker sets
  and clears the high byte (#706). With the high byte set and tokens
  drawn into both planes, plane 1 gets the glyph of another character
  (the inverse drawing leaves the last glyph byte in d0's low byte)
  (#709).
* FPrintDetoke: after the `>` marker an inverse space follows too when
  the column counter (with the caller's high word of d6) became 0 (#708).
* D_FindStringPtr never compares the long at areg5 itself (#705).
* SearchMem: the string search does not go back after a partial match
  (`ab` in `aab` is not found there) (#723); the empty string compares
  the bytes behind its 0 (#721). The space bar test reads RawStatus
  every 127 steps; the abort returns 1 and keeps SearchMemFound.
* AddProcessPtr has no limit of 20 entries (#700).

Like the original, and no defects: ClearProcessPtrs clears only the
first entry's character (the others stay behind it, unreachable: it
ends the lookup, and the next AddProcessPtr reuses it); the `%p`/`%c`
entries are set by init and point to 0 before it (init always runs
first); an unknown `%x` drops the x (intended by the source).

## Test

Each run loads one library set (the C build or the original); the
scenario writes its observations as cases, compared with the expected
data `libs/tests/expected/rwe_rilesdebuglib.txt` recorded from the
original. RawStatus is a stub (sysload_stubs) that counts its calls,
records the key and answers a chosen value; it keeps all registers but
d0 (the original's SearchMem relies on a1). Memory the routines read is
one block, so results that are addresses are observed as offsets into
it. Observed: init; D_GetSR (11 values); D_HexWord (1-8
words, even and odd source, from $41C/$41B/$41F and $FFFFFFF0);
D_CheckNMove/A/Word (both forms, 1-5, zero bytes, below $420 and from
$FFFFFFF0); D_FindStringPtr (found, the long at areg5, not found, 0);
PeekMax$ (max -1..100); GetHex$/GetHexAdr$ (modes 0-4, $101, $FF02);
GetDataItem (sizes -1..5, $7FFF, 0 and -1 - in the original 65536 and
65535 bytes -, below $420, $80000000); GetProgMode; GetCopIns$ (14 instructions: MOVE, WAIT, SKIP,
odd digits, BFD); GetCustomReg for $000-$1FD and 6 more (above $1FC, a
negative long); GetCustomRegAdr for every name and 16 more (prefixes,
8 characters, NOOP, lower case, empty); the font printer – positions,
the row change, the empty string, FInverse 1 and $100, FYOffset, the
right edge, all characters 1-255, columns from 210 on – observed on both
bitplanes after every call; FPrintDetoke with 4 lines × 6 cio × 5
lineoffsets × 2 marker types × tokenise off/on (480 calls), the right
edge inside and in front of a token, FInverse 1/$100, no token list, no
plane 1; OutTextPen/OutText on a RastPort of the test (pen, cp_x/cp_y,
both planes); SearchMem (24 cases: every type, invalid types, found at
the start/inside/across the end, start = end, the partial match, the
empty string, the space bar down) with SearchMemFound and the RawStatus
calls; FindLineToken; ExtractLabel (7 strings); D_InitParse/D_NextArg$
(before D_InitParse, empty arguments, past the end); SetupLESDebug,
ProcessString with `%p`, `%c`, `%%`, unknown entries, AddProcessPtr,
ClearProcessPtrs, after a D_NextArg$ and without a parser position (the
text at address 0, set for the call); D_FindVolume (device, volume,
assign, missing, empty); finit. Kept registers on every call.

Not covered: counts of 0 for D_HexWord/D_CheckNMove* (the original
writes 65536 entries into the destination, #701); the DIFF cases above;
ProcessString with a `%` at the end; the debugger program that used the
library.
