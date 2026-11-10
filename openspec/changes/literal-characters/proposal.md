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
file with such characters is loaded as token pairs (#374; its interim
rule: bytes >= $80 of a text file are characters, the six umlauts mapped
as typed). The user (2026-11-10) wants any Latin-1 character to survive in
strings and comments, and a file format that tells tools and editors which
sources need that.

## What Changes

- A new token pair, the **literal character** `$90 cc` (first byte `$90`,
  second byte the character `cc`, $80..$FF), carries one Latin-1
  character through TED and the compilers. It has the shape of every
  token pair, so every reader that walks a line pair by pair stays in
  step. No library of the CD's Blitz 2 or of AmiBlitz3 v3.14.0 has a
  token whose first byte is $90 (checked with `bbsrc tokens`, design D2).
- TED-C creates the pair when typing, when loading text files and when
  tokenising (every byte >= $80 that is not one of the six umlauts), and
  shows, edits, searches, copies, prints and saves it as the one
  character it stands for.
- TED-C writes a tokenised file that holds at least one literal character
  in the new format **`BB23`** (derived from `BB22`: a header with a
  version and the cursor fields, then TED's lines); a file without one is
  written in TED's own format as today.
- blitz2-c and `blitz2-ab3` compile a literal character inside a string
  to the byte `cc`. The compiler's string readers already store the low
  byte of a token word there (`eval.c:2304`, `blitz2.s` `litstring`);
  what changes is the audit of every string reader, the tests, the
  handling outside strings, and `Include` of a `BB23` source (the header
  is skipped).
- `blitz2-ab3`: AmiBlitz3's text sources keep Latin-1 in strings and
  comments as raw bytes; the loader turns them into literal characters so
  the compiled strings hold the bytes AmiBlitz3's compiler gives them.
- Host tools (`bbsrc`, `tedsrc`, `xtra`, `tools/lib/bb22`,
  `tools/lib/bbdetok`, `shared/xtra`) read, convert and recognise `BB23`.
- The 26..31 mapping stays: the six umlauts keep their codes (design D4),
  so a file that holds only umlauts is a classic TED file.
- **BREAKING (only for files that use the token):** the original TED and
  the original compiler cannot read a `BB23` file. Files without a
  literal character stay byte-identical to today's, and TED-C with the
  original compiler (combination C5) keeps working for them.

## Capabilities

### New Capabilities

- `editor/literal-characters`: how TED-C stores, shows, edits, finds,
  prints and saves a literal character, and which characters become one.
- `formats/bb23-source`: the `BB23` file format and when TED-C writes it.
- `compiler/literal-characters`: what blitz2-c compiles a literal
  character to, and how it reads `Include` files in `BB23`.
- `tooling/source-formats`: how the host tools read, convert and
  recognise `BB23` sources and literal characters.

### Modified Capabilities

- `ab3-compat/strings`: Latin-1 characters in the strings and comments of
  an AmiBlitz3 text source in AB3 mode.

## Impact

- `programs/ted/src/core` (`tokit.c`, `fileio.c` `readline`/`findtoke`/
  `detok`/`detoka5`/`dispcol`/`sa_tokprint`, `loadsave.c` save, print and
  header, `lines.c:901,945`, `services.c` `help_core`, `keymap.c`),
  `programs/ted/src` (the colour of a literal on the screen), the TED A/B
  scripts.
- `programs/blitz2/src` (`eval.c` string readers, `parse.c`, `libcall.c`
  `readinc`, `ext_bb2.c`, `ab3inc.c`, `ab3chr.c`), `shared/tokenizer`
  (AB3 rules), `shared/ab3job`, `programs/ab3cli`.
- `shared/xtra` (`XTRA_T_BB23`), `tools/lib/bb22`, `tools/lib/bbdetok`,
  `tools/bbsrc`, `programs/ted/tools/tedsrc`, `tools/xtra`.
- Tests: host tests of the TED core (one program per test file), TED A/B
  scripts (R5/C5/Cc/Rc, P5/Pc), the corpus, `tests/ab3` (oracle preset),
  tool tests with deliberately broken input.
- Docs: `docs/formats.md` (§3.2, §4.1, §4.2, a new section),
  `docs/rwelibs/README.md` §4.1 (a pointer), `docs/ab3/variant.md`,
  `docs/ted/`, `docs/blitz2/`, the deviations lists (kind F, with #374),
  the website (coordinator).
- Results: executables and files of programs without a literal character
  are byte-identical to the `prev` build; a source with one is a new
  result (intended, named with `-DPREV_INTENDED`).
