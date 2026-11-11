## Context

TED's line is a string of bytes ended by 0. A byte with bit 7 and the
byte after it are a token (`docs/formats.md` §3.2, `docs/token-table.md`):
the word is `$8000 + library * 128 + entry`, so the first byte is
`$80 + library / 2` and the second byte is `entry`, plus `$80` for an odd
library. Everything that walks a line assumes it: the
display (`sa_tokprint`, `programs/ted/src/core/fileio.c:940`), the
detokeniser (`detok`, `detoka5`, `prtfile_line`), the column arithmetic
(`dispcol`, `help_core`), the loader (`readline`), and in blitz2
`getcharb` (`parse.c`), `findit`, the macro and include scans
(`compile.c:413-421`). All token names come from one function,
`findtoke` (`fileio.c:533`), which walks the token list the compiler hands
to the editor (the compiler's own tokens, then `acidlibs`, `deflibs` and
the `userlibs`); an unknown word gives `tokeerr` ("?????"), and with no
list at all (`tokens == 0`, standalone TED, host tests) every token is
unknown.

Latin-1 today:

- Typing: `germs_map` (`keymap.c:25`) maps ä ö ü Ä Ö Ü to 26..31, any
  other character is passed on.
- Tokeniser: `tokskip` (`tokit.c`) does `c = *src++ & 0x7F` for every
  character, strings and comments included, so ß ($DF) becomes `_`, é
  ($E9) becomes `i`.
- Compiler: `gchar2` (`parse.c:366`) maps 26..31 inside quotes with the
  table `gs`; strings are collected by `litstring` (`eval.c:2290-2305`,
  `pokedata1b((UBYTE)c)`; the original `blitz2.s` `litstring` moves the
  word `d0` to `d1` and pokes a byte), so a token word read inside quotes
  already gives its **low byte**. `getchar_` handles the words `$801C` to
  `$801F` (conditional compilation) before the string code, also inside
  quotes (`parse.c:397-421`); no other word is special there.
- AmiBlitz3 keeps strings and comments of its sources as raw text
  (`docs/formats.md` §3.4); its `gs` swaps ä and ö
  (`docs/formats.md` §4.1). `blitz2-ab3` loads an AB3 text source through
  `ab3cli` and TED's core with `shared/tokenizer` (rules AB3: strings and
  comments copied untouched), and the compiler then reads a raw byte >= $80
  in a string as the start of a pair (`ab3chr.c:52`: "a byte above $7F:
  getcharb has read the next one with it"). Task 1.4 measures what that
  does to the compiled string today.
- #374 is fixed (TED fix round R3c, `15762513`, site `826d8987`):
  `readline` (`programs/ted/src/core/fileio.c:262-345`) takes a byte
  >= $80 of a text file for one character, no longer for the start of a
  pair (`programs/ted/tests/host/test_readline_latin1.cpp`). With tokenise
  on the six umlauts of `germs` are stored as codes 26..31
  (`fileio.c:318`) and the tokeniser clears bit 7 of every other byte
  (ß becomes `_`); with tokenise off the bytes stay as they are. A save
  with tokenise off (`realsave`, `loadsave.c:324-345`) writes the stored
  bytes with the 0 turned into LF, and Block Save with tokenise off
  (`lines.c:827-840`) the same; with tokenise on `realsave` writes the
  tokenised lines and Block Save writes token names (`detoka5`). Nothing
  writes a literal as `cc` yet: this change does (D6).

## Goals / Non-Goals

**Goals:**
- Any Latin-1 character survives in strings and comments through TED-C
  (typing, loading, display, editing, search, blocks, print, save,
  tokenise on and off) and reaches the compiled string.
- A tokenised source with literal characters is an ordinary tokenised BB2
  source: no header, no marker. The original compiler reads it, also as an
  `Include`; the original TED loads and saves it without loss and shows the
  characters when the userlib of D3 is loaded.
- Sources without a literal character stay byte-identical, and TED-C with
  the original compiler (C5) keeps working.
- The same for blitz2-ab3, and the host tools.

**Non-Goals:**
- Characters outside Latin-1 (UTF-8 text, other code pages); the pair
  carries one byte.
- Changing the original TED or the original compiler. (The libraries get
  one new library, D3; no existing library changes.)
- Typing a literal character in the original TED: its tokeniser clears
  bit 7 (`tokit`) and its key map knows only the six umlauts (`germs`).
- Latin-1 in identifiers, labels or library names.
- A new font or a font that has glyphs the screen font lacks.

## Decisions

### D1 A token pair `$90 cc`, not a string token

Alternatives:

| | What | For | Against |
|---|---|---|---|
| A | `$90 cc`: one pair per character | the shape of every token: all pair walkers (TED, `readinc`, macro scans, `bblines`, `bbdetok`) stay in step without a change; a library (D3) can name the 128 words, so the original TED shows them with no change; the compiler already pokes the low byte; in a comment it survives next to real tokens | two bytes per character: a line of 128 bytes holds 64 literal characters; a first byte is reserved (D2) |
| B | a string token `$90 len chars...` (a Pascal string of raw bytes) | one token per run of characters, compact | the bytes inside are >= $80 and break every walker that does not know the token (`getcharb`, `findit`, the macro scans, `readinc`, `bblines`); `findtoke` cannot answer it; editing in the middle of the run changes the length byte; much more code |
| C | `$FF xx` with xx = the character, in the free words of library 254/255 | none | `$FF00`-`$FFFF` hold the OS-call tokens of deflibs (only `$FF00`, `$FF80`, `$FFFD`-`$FFFF` free); no room for 128 characters |
| D | the word `$90xx` interpreted only inside strings (no marker byte) | no reserved token | comments hold real tokens (a line commented out by typing `;` in front keeps them, `docs/formats.md` §3.4), so a pair in a comment would be ambiguous |
| E | three bytes `M 0 cc` | none | `cc` >= $80 would start a pair for an old walker |

A is chosen. The compiler needs almost nothing (D5); TED-C needs one
rule in `sa_tokprint` and a few others (D3).

### D2 First byte `$90`: libraries 32 and 33 are reserved

**Decision (user, 2026-11-11):** the whole first byte `$90` - the
libraries 32 and 33, the words `$9000` to `$90FF` - is reserved for
literal characters, wherever possible (below). The pair `$90 cc` with `cc` >= $80 is the word
`$9000 + cc`, which is library 33, entry `cc - $80`; the words
`$9000`-`$907F` (library 32) stay unused and reserved with it. The
reservation is documented where library numbers are documented (tasks
8.1 to 8.3) and checked in the build (task 3.4).

**Measurement behind it** (2026-11-10, repeated for AmiBlitz3 on
2026-11-11): `bbsrc tokens` over the CD's compiler, `acidlibs`,
`extras/bigdeflibs/deflibs` (the largest standard set) and AmiBlitz3
v3.14.0's compiler with `baseLibs`, `csLibs`, `OsLibs` and `utilLibs`
(4653 tokens): of the first bytes `$80` to `$FF`, exactly one is used by
no token of both: `$90`. The Blitz 2 libraries use 83 first bytes,
AmiBlitz3's 126, together 127.

**AmiBlitz3 check (2026-11-11), result: AmiBlitz3 does not use `$90`.**
Sources in `archive/amiblitz3/`:

- `github-58a11658` (v3.14.0): `bbsrc tokens` over `Amiblitz3` (the
  compiler's own list) and `BlitzLibs/baseLibs`, `utilLibs`, `OsLibs`,
  `csLibs`: 4653 tokens, first bytes `$80`, `$82`-`$8F`, `$91`-`$FF`, none
  `$90` (the file also has no `$81`, which the Blitz 2 libraries use).
  `blitzlib extract BIGLIB 32` and `33` on each of the four BigLibs:
  "no library 32" / "no library 33" in all of them.
- `svn-r279`: only `RIAmosFuncLib.obj`, library 75 (`blitzlib cmdref`).
- `github-b1a9dee`: five sources (no library files, no `libnums`; the
  one library source, `LotanMemLib.ab3`, names `libnums.res` as a
  resident but the archive does not hold it).
- The archive holds no AmiBlitz3 `libnums` or `sorted_libnums.ab3`; the
  library numbers AmiBlitz3 keeps there are known only from
  `docs/ab3/userlibs.md` and `docs/rwelibs/README.md`. They show one
  case: RWE's `RIDOSLIB` was library 33 in AmiBlitz3's optional userlib
  files (`BlitzLibs/UserLibs/userlibs/`) until
  `AmiBlitz/AmiBlitz3@66d3c4be` (2026-08-19) renumbered it to 58
  (`docs/ab3/userlibs.md:115-120`); v3.14.0 (2026-10-19) is after that and
  has no library 33.

**What clashes with the reservation, and the decision (user,
2026-11-11):** the CD's `BSS2/REDBLITZLIBS/LIBRARIES/USERLIBS/RIDOSLIB.OBJ`
has the tokens `$9081` to `$908B` (library 33; `bbsrc tokens` over every
`*.OBJ` below the CD's `userlibs` directories finds only this file with a
first byte `$90`), and this repository builds the same library as
`libs/userlibs/rwe/ridoslib.c` with `LIB_ridoslib 33`
(`libs/variants/rwe-1999/libnums.h:12`). What the loaders do with two
libraries of one number decides how far the reservation reaches:

- In BB2 mode blitz2 loads `acidlibs`, then `deflibs`, then the userlibs
  and never replaces a library number; `findlib` and TED's `findtoke`
  take the first match (`programs/blitz2/src/asmlibs.c:1271`,
  `:1181-1200`, `programs/ted/src/core/fileio.c:533`,
  `docs/architecture.md` 4.7). A library 33 that is loaded second is
  never found.
- In AB3 mode a userlib replaces an earlier library with the same number
  (`ab3libs.c:98-117`).

So: **`$90` is reserved wherever possible.** The default `acidlibs` and
every variant without RWE get `literallib` as library 33; library 32 stays
free and reserved. The variant `rwe-1999` and its RWE sub-variants
(`rwe-1999-eval`, `rwe-1999-ahi`) keep RIDosLib as library 33 with its
original number and have no literal library; literal characters are not
supported there (a user who wants them uses another variant). The check
of the reservation (task 3.4) allows `ridoslib` as library 33 in the
`rwe-1999` variants only, and library 33 other than `literallib` and
`ridoslib` nowhere. The userlib `literallib.obj` for the original Blitz 2
works only if no other library with number 33 is loaded before it (the
loader takes the first match); the documentation says so (task 8.1).

The marker is chosen by the measurement and by decision, not by a
rule. The marker is a constant in one header per program (`tokit.h`,
`main.h`, `shared/xtra`).

Alternative: take the pair from a library number the user never loads
(`$FF80`..): rejected, `deflibs` has all of them but three.

### D3 A library names the 128 words; TED shows them like any token

A new library, a command library like those of `acidlibs`, holds the
words `$9080` to `$90FF`: library number 33, 128 entries, entry `n` is
named by the single character with code `$80 + n` (a one-byte name,
Latin-1; written as `"\xDF"` in the C source). Header: `BLITZ_LIBHEADER` with library
number 33 (the tokens `$9080` to `$90FF` follow from the number), no init
or finit code, no help text. The name decides the
display: `findtoke` answers `$90cc` with the one-character name, so
`detok`, `detoka5` (search, replace, Block Save text), `dispcol` (#377),
`readline`'s tab columns, `help_core` and the display copy the
character with the right width, in TED-C and in the original TED alike,
with no change of either.

The name check in the tokeniser cannot take the new names for keywords:
`tokskip` compares `namec | 32` with `c | 32` where `c` is a source
character with bit 7 cleared (`tokit.c:106-114`), so a name byte >= $80
never matches, and the keyword search starts only at a letter.

**The library is not callable.** Its 128 entries have the kind word 0 (the
shape of `BLITZ_NULLSUB`, `libs/include/blitzlib.h:245`: neither
statement flag nor function flag; code and error addresses 0). What the
compiler does with such a token, read in `programs/blitz2/src`:

- as a statement, `handlelib` (`compile.c:1426-1430`) tests
  `sub[1] & 1` after `findtoke` and gives `syntaxerr()`; nothing is
  generated;
- in an expression, `function` (`eval.c:2220-2223`) tests `fl & 2` and
  gives `syntaxerr()`;
- other callers of `findtoke` (`getmaxlib` `tokens1.c:214`, `eval.c:1881`,
  `tokecode` `asmlibs.c:1035`, `libcall.c:832`) are reached only with a
  token the earlier code accepted; task 4.1 reads each and records what a
  literal word does there (an error of any kind and no code is the
  requirement, spec `compiler/literal-characters`).

So a literal outside a string and a comment is a clean "Syntax Error"
whenever the library is loaded (blitz2-c with its `acidlibs`, the
original compiler with the userlib). Where it is not (`blitz2-ab3` loads
AmiBlitz3's own libraries, D2), `findtoke` does not find the word and the
compile stops with "Token Not Found : $90xx." (`parse.c:630-636`): also
clean. The compiler's code does not change for either (D5).

**Where the library lives.** In the library set the compiler reads at
startup: `libs/acidlibs` (`ACIDLIBS_ORDER`, one C file
`libs/acidlibs/literallib.c`), so it is in `acidlibs` of the default
variant and of every variant without RWE (decision of 2026-11-11:
`acidlibs` only; `deflibs` stays as it is). The RWE variants
(`rwe-1999`, `rwe-1999-eval`, `rwe-1999-ahi`) include the default
`libnums.h` (`libs/variants/rwe-1999/libnums.h:10`) and build the default
`acidlibs` list (`acidlibs_build` loops over `ACIDLIBS_ORDER`,
`libs/acidlibs/CMakeLists.txt:43`), so they would get the library; they
keep RIDosLib as 33 instead (D2). `literallib` is therefore left out of
them: `LIB_literallib 33` is defined in `libs/variants/bb2-2.10/libnums.h`
and `#undef`ed in `rwe-1999/libnums.h` (which the two sub-variants
include), and `acidlibs_build` skips `literallib` for the variants named
in a list `ACIDLIBS_WITHOUT_LITERAL` (the RWE ones; task 3.1). Test
variants in `libs/tests/variants/` take the default; the check of
task 3.4 catches one that uses 33 otherwise. The same C source
is built a second time as a userlib file for the original Blitz 2: the
single library file `literallib.obj` for `blitz2:userlibs/`, named in the
documentation like the RWE userlibs. Its number is 33 as well; it is for a
Blitz 2 that does not have the library in its `acidlibs` (the CD's), and
must not be loaded together with `acidlibs` that have it (a number twice,
which `blitzlib numbers` refuses). Userlibs are loaded last and the
first library of a number wins (D2), so it works only if no other
library with number 33 (RIDosLib of the CD) is loaded before it.

**Does TED-C still need a constant table? Yes, as a fallback only.** The
word `$90cc` is answered by the token list when it holds the entry, and
by a 128-entry constant table of one-character strings when it does not.
Three cases have no entry, and one rule covers them:

1. `tokens == 0` (`fileio.c:538`): the standalone TED and every host test
   of the TED core, and the tools that use the core (`tedsrc`);
2. `blitz2-ab3` (AB3 mode): the list is AmiBlitz3's, with AmiBlitz3's
   libraries, which cannot hold our library (and `ab3_tokens` requires the
   list to equal the AmiBlitz3 compiler's, `tests/ab3/CMakeLists.txt`);
3. TED-C with the original compiler (C5), a Blitz 2 without the userlib
   and the RWE variants:
   a literal would show as `?????` and could not be searched or printed,
   a loss TED-C can avoid.

The fallback is 128 two-byte strings (about 256 bytes); the entry of the
list wins when both exist (both give the same character). In the RWE
variants (no literal library, D2) the fallback is what shows a literal.

The consumers that the name does not settle need their own rule, for a
word `$90cc` with `cc >= $80` (the range test, not the name):

- `sa_tokprint` (`fileio.c:940`): a literal is drawn with `normcol` and
  without the token flag bit (`row[col] |= 0x80` marks token cells; a name
  character >= $80 and the flag would be ambiguous in the chmap).
- `prtfile_line` (`loadsave.c:150`): the bold ESC sequence is not sent
  for a literal.
- `help_core` (`services.c:175`): `result` stays NULL for a literal
  (Help on a character finds nothing) but `x` counts one column.
- `lines.c:901,945` (`& 0x7F`): read in task 1.2 (what the two loops
  copy and why).
- The tokeniser `tokskip`: `c = *src++ & 0x7F` stays for the *keyword
  matching* (a letter comparison), but a character >= $80 is emitted as
  the pair, not as `c & 0x7F`; `putchar_` counts one column for two
  bytes (it counts bytes against `last`, `tokit.c`: the line limit is in
  bytes, see Risks).
- Tokenise off: lines are stored the same way (D6).
- `findtoke` for a word `$90cc` with `cc < $80` (library 32) is not a
  literal: it stays `tokeerr` unless the list names it.

The original TED prints a name through its own `tokprint` in the token
colour, so it shows a literal as a character in the colour of a keyword.
That, and that the glyph of a name byte >= $80 reaches the screen
unchanged, is not read from `ted.s` but measured: task 9.2 runs the
original TED in Amiberry with the userlib.

### D4 What happens to 26..31: keep, with one stored form

**Decision (user, 2026-11-11): option (a).** Alternatives: (a) keep the
six umlauts as the codes 26..31 and make every other character >= $80 a
literal; (b) store every character >= $80 as a literal and read 26..31
only from old files; (c) both forms accepted everywhere, stored as typed.

(a) is chosen. Reasons: a file that holds only German umlauts (the
common case, and the one the six codes were made for) stays as it was,
and the files the CD and the corpus already hold stay valid; the compile
result is unchanged (`gs` maps 26..31 back in strings); only files with
ß, é, à, ... hold pairs. (b) would put a pair into every German source
and change files that need no change. (c) gives two stored forms of one
character, so a search for `ä` would miss half the text. Hence one stored
form per character: typing, the text loader and the tokeniser write the
six umlauts as 26..31 in BB2 mode, every other character >= $80 as a
pair. The umlauts' codes 26..31 in *comments* stay as they are (the
compiler skips comments).

**Where a literal counts (decision of 2026-11-11):** everywhere, for
simplicity: typing `é` outside a string and a comment also gives
`$90 E9` (a syntax error at compile time instead of today's silent `i`),
and the tokeniser clears bit 7 of no character. Only inside strings and
comments is this behaviour guaranteed (the spec says so); outside them it
is not a guaranteed behaviour and may change.

A tokenised file is loaded as it is stored: a pair `$90 E4` that a foreign
tool wrote stays a pair (decision of 2026-11-11), so load and save keep
the file byte for byte; it is a second stored form of that umlaut, and a
search for `ä` does not find it. **AB3 mode loader (decision of
2026-11-11):** the raw bytes are turned into literals in the loader
(`ab3cli`, `shared/tokenizer`), not by a mode switch in `getcharb`, so
`getcharb` stays one code path for both modes. In **AB3 mode** the six umlauts must not become 26..31 (AB3's `gs`
maps the codes to other characters: code 26 gives ö there, ä in Blitz 2),
so `blitz2-ab3`'s loader uses a literal for every byte >= $80 (spec
`ab3-compat/strings`); the compiled bytes are then those AmiBlitz3 gives.

### D4a The line limit counts bytes

A literal is two bytes, so a line of 128 bytes holds 64 of them
(decision of 2026-11-11). The byte limit stays as it is (#375: 129 bytes
and the 0; the original 128): a pair is never cut at it, and a physical
line that is longer is split as for #375. Counting characters would allow
up to 256 bytes and not fit `charbuff2`.

### D5 The compiler

By D1 and the existing `litstring` path no new code is needed to get the
byte into a string: the pair arrives as a word `$90cc` and its low byte
is poked. The change is an audit and tests: (1) every reader of string
characters (`litstring`, `fetchqasm`, `Data$`, `Dc.b`, quoted constants
in `ab3chr.c`, `ext_strescape`) takes the low byte of a word, not
`(UBYTE)c < 32` mapping or a `c & 0x8000` branch that treats the pair as
a token (`ab3chr.c:52` adds 2 to the length: wrong for a literal in a
quoted constant: it must count one); (2) a literal outside a string
reaches `function(c, ...)` (`eval.c:1612`) or `handlelib` and ends in an
error without code (D3). The original compiler does the same inside
strings (its `litstring` is the source of the C code), which is why C5
works for files with literals, from TED-C's memory and as an `Include`:
an include is read line by line like the main source and a pair is an
ordinary token to `readinc`.

### D6 TED-C: one stored form, whatever the options

Typing, loading and the tokeniser produce the same stored form whether
tokenise is on or off, so `Tokenise` can be switched at any time and the
file can be saved either way. What the code does today is in the Context
(#374 is fixed): `readline` stores a byte >= $80 of a text file as one
byte, the six umlauts as 26..31 with tokenise on, and a save with
tokenise off writes the stored bytes. The change to it: `readline` and
the tokeniser store a byte >= $80 of a text (not one of the six umlauts)
as the pair `$90 cc`, with tokenise on and off, and a text save
(tokenise off) writes a literal as the byte `cc`, so the text round trip
of a file with `ß` gives the byte `$DF` again. A text save writes only
text (decision of 2026-11-11): the codes 26..31 as ä ö ü Ä Ö Ü, the
counterpart of the loader, and real tokens (from a tokenised file loaded
or from typing with tokenise on) as their names (`detoka5`, as Block Save
with tokenise on does); tokenise off never turns a character into a
token. So loading and saving a text file gives the same bytes. The text loader turns
`cc` >= $80 into a literal (a text file has no pairs) or, for the six
umlauts, into the code 26..31 (D4); this replaces the rule at
`fileio.c:318` ("the byte stays as it is" with tokenise off, bit 7 lost
in the tokeniser with it on), and the save of the stored bytes with
tokenise off.

### D7 No file format of its own

A source with literal characters is a tokenised source in TED's format:
no header, no flag, no new extension. TED-C writes a tokenised file
exactly as before; a line may now hold the pair `$90 cc`. A reader that
does not know the pair meets an unknown token word (`$9080`-`$90FF`),
which every reader of TED's format already handles. The reservation of
the first byte (D2) is the whole contract.

### D8 Compatibility

| Combination | File without a literal | File with a literal |
|---|---|---|
| TED-C + blitz2-c (Cc) | as today | works |
| TED-C + original compiler (C5) | as today | compile from TED's memory works (D5), `Include` works; a literal outside a string is a Syntax Error with the userlib, "Token Not Found" without |
| original TED + any compiler (R5, Rc) | as today | loads and saves without loss; shows the characters (in token colour) when the userlib of D3 is loaded, else `?????`; typing a literal is not supported (D1, Non-Goals) |
| original TED loads a text file TED-C wrote with Latin-1 | as today | as today (a text file has no pairs; #374) |
| the `rwe-1999` variants (no literal library) | as today | not supported (D2); the editor's fallback table shows the characters, a stray literal is "Token Not Found" |
| original Blitz 2 with the userlib `literallib.obj` | as today | works only if no other library numbered 33 is loaded before it (D2) |

The rule "a file without a literal is byte-identical" holds by saving
and by the tokeniser: a source without a byte >= $80 outside the six
umlauts tokenises exactly as before. The TED A/B pair C5-Cc stays equal.
A script that types a literal character compares TED-C only for the
typing (R5 and Rc cannot type it; named in the script as intended); a
script that loads a file with literals compares the screens of the
original TED (with the userlib) and TED-C.

### D9 Host tools

`bbdetok` (`tools/lib/bbdetok/bbdetok.c:98-114`) writes a literal as its
byte and no longer as `{lib:entry}`; `tedsrc tok` uses the TED core and
so gets the tokeniser rule; `bblines` needs no change (it walks pairs);
`bbsrc tokens` lists the library's 128 tokens like any other; `blitzlib
numbers` (or `check`) refuses a library numbered 32 and any other library
numbered 33 (task 3.4); `shared/xtra` and the `.xtra` handling are
untouched.

## Risks / Trade-offs

- [The reservation of 32 and 33 takes a number that a third-party userlib
  may use] → decided (D2); the build checks the libraries of the
  repository (task 3.4); RIDosLib, the one known case, keeps 33 in the
  RWE variants, which have no literal library; the scan of task 1.1
  covers the corpus and the CD's userlibs.
- [`tokskip` counts the line limit in bytes and a literal takes two while
  showing one column] → the byte limit stays (D4a); a host test with a
  line of 64 literals and 65 literals; no change of `chcols` code.
- [Typing a Latin-1 character outside a string used to give a wrong but
  compilable program (`é` -> `i`) and now gives a syntax error] → D4 and
  the site entry; the old behaviour was silent corruption.
- [A pair for one of the six umlauts from a foreign tool is a second
  stored form of that character] → D4.
- [The original TED shows the pairs as `?????` without the userlib] → D8:
  no loss on load and save; the userlib is shipped (task 3.3).
- [The new library makes `acidlibs`, the token list and the results of
  the library tests differ from the CD's] → intended; named with
  `-DPREV_INTENDED`; the CD comparisons get the library as a named
  addition (tasks 3.2, 3.3).
- [this change replaces #374's rule for a byte >= $80 of a text file]
  → #374 is fixed (Context); task 1.5 re-reads it before the loader is changed.
- [The screen font has no glyph for $80..$9F] → task 1.3 measures; such a
  character is shown with the font's default glyph (spec) and kept.
- [The original TED draws a name byte >= $80 differently from the
  expected glyph, or in a way that confuses its chmap] → task 9.2 runs
  it; the fallback is a documented limit of the original TED (D8), not a
  change of the format.

## Migration Plan

TED-C, blitz2-c and the library are shipped together. A file with
literals is a TED file: rolling back TED-C or blitz2-c leaves such files
readable (the original TED shows unknown tokens, the compilers stop at a
pair outside a string and compile it in a string to its low byte). No
file is converted. Rollback of the library is the revert of its commits;
a source that still holds pairs then shows `?????` in the original TED
until the userlib is loaded.
