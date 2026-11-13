## Why

A Blitz 2 source cannot hold a Latin-1 character other than the six
umlauts. In TED's line format every byte >= $80 is the first byte of a
token pair, in strings and comments too (`readline`, `findtoke`,
`detok`, `sa_tokprint` in `programs/ted/src/core/fileio.c`; `getcharb`
in `programs/blitz2/src/parse.c`, the include and macro scans in
`programs/blitz2/src/compile.c`). Typing maps only ä ö ü Ä Ö Ü to the
codes 26..31 (`germs`, `ted.s:6022`, `programs/ted/src/core/keymap.c:18`),
which blitz2 maps back inside strings (`gs`, `blitz2.s:7181`,
`programs/blitz2/src/parse.c:38`). Every other byte >= $80 (ß, é, à, ...)
loses bit 7 in TED's tokeniser (`tokskip`, `c = *src++ & 0x7F`,
`programs/ted/src/core/tokit.c`): `é` ($E9) becomes `i`, silently. A text
file with such characters was loaded as token pairs (#374, fixed in TED
round R3c: bytes >= $80 of a text file are characters, the six umlauts
mapped as typed with tokenise on, the others losing bit 7 in the
tokeniser). The user (2026-11-10) wants any Latin-1 character to survive in
strings and comments.

## What Changes

- A new token pair, the **literal character** `$90 cc` (first byte `$90`,
  second byte the character `cc`, $81..$FF), carries one Latin-1
  character through TED and the compilers. It has the shape of every
  token pair, so every reader that walks a line pair by pair stays in
  step. A source with literal characters is an ordinary tokenised Blitz 2
  source: no header, no marker, no new file format.
- The whole first byte `$90` (libraries 32 and 33) is **reserved** for
  literal characters (decision of 2026-11-11). Neither the libraries of
  the CD's Blitz 2 nor AmiBlitz3 v3.14.0 use it (design D2); RWE's
  `RIDOSLIB` (library 33) does and keeps it in the RWE variants, which
  therefore have no literal library.
- A new library, **`literallib`** (library 33, 127 dummy tokens named by the
  characters $81 to $FF; $80 is no literal character), goes into `acidlibs` and its variants without RWE and is
  also shipped as a userlib file for the original Blitz 2. TED, the
  original and TED-C, then shows a literal through the normal token name
  lookup. Its entries are not callable: a literal outside a string is a
  compile error, not a library call.
- TED-C creates the pair when typing, when loading text files and when
  tokenising (every byte >= $80 that is not one of the six umlauts), and
  shows, edits, searches, copies, prints and saves it as the one
  character it stands for. A small constant table answers the pair when
  the token list has no entry (standalone TED, AB3 mode, no userlib).
- blitz2-c and `blitz2-ab3` compile a literal character inside a string
  to the byte `cc`. The compiler's string readers already store the low
  byte of a token word there (`eval.c:2304`, `blitz2.s` `litstring`);
  what changes is the audit of every string reader and the tests.
- `blitz2-ab3`: AmiBlitz3's text sources keep Latin-1 in strings and
  comments as raw bytes; the loader turns them into literal characters so
  the compiled strings hold the bytes AmiBlitz3's compiler gives them.
- Host tools: `bbdetok` writes a literal as its byte; `blitzlib` refuses
  library numbers 32 and a second 33.
- The 26..31 mapping stays: the six umlauts keep their codes (design D4).
- Compatibility: the original compiler reads a source with literals,
  also as an `Include`; the original TED loads and saves it without loss
  and shows the characters with the userlib (else `?????`); typing a
  literal in the original TED stays unsupported. Files without a literal
  character stay byte-identical to today's, and TED-C with the original
  compiler (combination C5) keeps working for them.

## Capabilities

### New Capabilities

- `editor/literal-characters`: how TED-C stores, shows, edits, finds,
  prints and saves a literal character, and which characters become one.
- `libs/literal-library`: the library of the 127 words, the reservation of
  the first byte `$90` and its check in the build.
- `compiler/literal-characters`: what blitz2-c compiles a literal
  character to, and what it does with one outside a string.
- `tooling/literal-characters`: how the host tools write and convert
  literal characters.

### Modified Capabilities

- `ab3-compat/strings`: Latin-1 characters in the strings and comments of
  an AmiBlitz3 text source in AB3 mode.

## Impact

- `programs/ted/src/core` (`tokit.c`, `fileio.c` `readline`/`findtoke`/
  `detok`/`detoka5`/`dispcol`/`sa_tokprint`, `loadsave.c` print,
  `lines.c:901,945`, `services.c` `help_core`, `keymap.c`),
  `programs/ted/src` (the colour of a literal on the screen), the TED A/B
  scripts.
- `programs/blitz2/src` (`eval.c` string readers, `parse.c`, `ab3chr.c`),
  `shared/tokenizer` (AB3 rules), `shared/ab3job`, `programs/ab3cli`.
- `libs/acidlibs` (`literallib.c`, `ACIDLIBS_ORDER`), `libs/userlibs`,
  `libs/variants/*/libnums.h`, `libs/CMakeLists.txt` (token and number
  checks), `libs/variants/rwe-1999/libnums.h` (`literallib` left out), `tools/lib/bbdetok`,
  `tools/blitzlib`, `programs/ted/tools/tedsrc`.
- Tests: host tests of the TED core (one program per test file), TED A/B
  scripts (R5/C5/Cc/Rc, P5/Pc), the corpus, `tests/ab3` (oracle preset),
  library tests (emulator), tool tests with deliberately broken input.
- Docs: `docs/formats.md` (§3.2, §4.1, §4.2), `docs/token-table.md`,
  `docs/libs/variants.md`, `docs/libs/deviations.md`,
  `docs/rwelibs/README.md`, `docs/ab3/variant.md`, `docs/ted/`,
  `docs/blitz2/`, the deviations lists (kind F, with #374), the website
  (coordinator).
- Results: executables and files of programs without a literal character
  are byte-identical to the `prev` build, except `acidlibs` of the
  variants without RWE and the new userlib file (intended, named with
  `-DPREV_INTENDED`); a source with a literal is a new result.
