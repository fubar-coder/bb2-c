## Context

TED's line is a string of bytes ended by 0. A byte with bit 7 and the
byte after it are a token (`docs/formats.md` §3.2, `docs/token-table.md`):
the word is `$8000 + library * 128 + entry`, so the first byte is
`$80 + library / 2`. Everything that walks a line assumes it: the
display (`sa_tokprint`, `programs/ted/src/core/fileio.c:940`), the
detokeniser (`detok`, `detoka5`, `prtfile_line`), the column arithmetic
(`dispcol`, `help_core`), the loader (`readline`), and in blitz2
`getcharb` (`parse.c`), `findit`, the macro and include scans
(`compile.c:413-421`). All token names come from one function,
`findtoke` (`fileio.c:479`); an unknown word gives `tokeerr`
("?????").

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
- #374 (open, TED fix rounds R3) makes a text file's bytes >= $80
  characters on load (interim rule); this change is the full solution
  (`~/.local/share/bb2-c-work/ted-bugs-decisions.md`).

`BB22` (`docs/rwelibs/README.md` §4.1, `tools/lib/bb22/bb22.h`): `BB22`,
version word (8 or 10), column, row, 0, top (and 2 for version 10), then
per line a prefix of 1 or 2 bytes (always 0), the tokenised text and a 0
byte. No editor of this repository writes it.

## Goals / Non-Goals

**Goals:**
- Any Latin-1 character survives in strings and comments through TED-C
  (typing, loading, display, editing, search, blocks, print, save,
  tokenise on and off) and reaches the compiled string.
- Sources that use the new token are recognised by their format; sources
  that do not stay byte-identical, and TED-C with the original compiler
  (C5) keeps working for them.
- The same for blitz2-ab3, and the host tools.

**Non-Goals:**
- Characters outside Latin-1 (UTF-8 text, other code pages); the pair
  carries one byte.
- Changing the original TED, the original compiler or the libraries.
- Latin-1 in identifiers, labels or library names.
- A new font or a font that has glyphs the screen font lacks.

## Decisions

### D1 A token pair `$90 cc`, not a string token

Alternatives:

| | What | For | Against |
|---|---|---|---|
| A | `$90 cc`: one pair per character | the shape of every token: all pair walkers (TED, `readinc`, macro scans, `bblines`, `bbdetok`, `bb22`) stay in step without a change; `findtoke` can answer it with a one-character name, so display, columns, tabs, search (via `detoka5`), print and the detokeniser nearly work at once; the compiler already pokes the low byte; in a comment it survives next to real tokens | two bytes per character: a line of 128 bytes holds 64 literal characters; a marker byte is needed |
| B | a string token `$90 len chars...` (a Pascal string of raw bytes) | one token per run of characters, compact | the bytes inside are >= $80 and break every walker that does not know the token (`getcharb`, `findit`, the macro scans, `readinc`, `bblines`); `findtoke` cannot answer it; editing in the middle of the run changes the length byte; much more code |
| C | `$FF xx` with xx = the character, in the free words of library 254/255 | none | `$FF00`-`$FFFF` hold the OS-call tokens of deflibs (only `$FF00`, `$FF80`, `$FFFD`-`$FFFF` free); no room for 128 characters |
| D | the word `$90xx` interpreted only inside strings (no marker byte) | no reserved token | comments hold real tokens (a line commented out by typing `;` in front keeps them, `docs/formats.md` §3.4), so a pair in a comment would be ambiguous |
| E | three bytes `M 0 cc` | none | `cc` >= $80 would start a pair for an old walker |

A is chosen. The compiler needs almost nothing (D5); TED-C needs one
rule in `findtoke` and one in the colour of `sa_tokprint`.

### D2 First byte `$90`

Measured on 2026-11-10 with `bbsrc tokens` over the CD's compiler,
`acidlibs`, `extras/bigdeflibs/deflibs` (the largest standard set) and
AmiBlitz3 v3.14.0's compiler with `baseLibs`, `csLibs`, `OsLibs` and
`utilLibs` (4653 tokens): of the first bytes `$80` to `$FF`, exactly one
is used by no token: `$90` (libraries 32 and 33). The Blitz 2 libraries
use 83 first bytes, AmiBlitz3's 126, together 127. The marker is chosen
by this measurement, not by a rule; third-party userlibs that use
library 32 or 33 would clash (task 1.1 scans the corpus and the
`userlibs` of the CD; a hit changes D2). The marker is a constant in one
header per program (`tokit.h`, `main.h`, `shared/xtra`).

Alternative: take the pair from a library number the user never loads
(`$FF80`..): rejected, `deflibs` has all of them but three.

### D3 `findtoke` answers the pair; the consumers decide

`findtoke(tok)` returns, for a word `$90cc` with `cc >= $80`, a pointer
into a constant table of 128 two-byte strings (`"\xDF"`, 0). Then
`detok`, `detoka5` (search, replace, Block Save text), `dispcol` (#377),
`readline`'s tab columns, `help_core` and the display copy the character
with the right width with no change. The remaining consumers need their
own rule:

- `sa_tokprint` (`fileio.c:940`): a literal is drawn with `normcol` and
  without the token flag bit (`row[col] |= 0x80` marks token cells).
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
- `findtoke` for a word `$90cc` with `cc < $80` stays `tokeerr` (the
  readers refuse it, `formats/bb23-source`).

### D4 What happens to 26..31: keep, with one stored form

Alternatives: (a) keep the six umlauts as the codes 26..31 and make every
other character >= $80 a literal; (b) store every character >= $80 as a
literal and read 26..31 only from old files; (c) both forms accepted
everywhere, stored as typed.

(a) is chosen. Reasons: a file that holds only German umlauts (the
common case, and the one the six codes were made for) stays a classic TED
file, readable by the original TED and compiler, and the files the CD
and the corpus already hold stay valid; the compile result is unchanged
(`gs` maps 26..31 back in strings); only files with ß, é, à, ... become
`BB23`. (b) would turn every German source into `BB23`. (c) gives two
stored forms of one character, so a search for `ä` would miss half the
text. Hence one stored form per character: the loaders and the tokeniser
rewrite `$90 E4` (and the other five) to 26..31 in BB2 mode. The
umlauts' codes 26..31 in *comments* stay as they are (the compiler
skips comments).

Consequences to decide in the review (Q3, Q4): a literal for one of the
six umlauts read from a `BB23` file is changed to the code; saving then
changes a file a foreign tool wrote. In **AB3 mode** the six umlauts must
not become 26..31 (AB3's `gs` maps the codes to other characters: code 26
gives ö there, ä in Blitz 2), so `blitz2-ab3`'s loader uses a literal for
every byte >= $80 (spec `ab3-compat/strings`); the compiled bytes are
then those AmiBlitz3 gives.

### D5 The compiler

By D1 and the existing `litstring` path no new code is needed to get the
byte into a string: the pair arrives as a word `$90cc` and its low byte
is poked. The change is an audit and tests: (1) every reader of string
characters (`litstring`, `fetchqasm`, `Data$`, `Dc.b`, quoted constants
in `ab3chr.c`, `ext_strescape`) takes the low byte of a word, not
`(UBYTE)c < 32` mapping or a `c & 0x8000` branch that treats the pair as
a token (`ab3chr.c:52` adds 2 to the length: wrong for a literal in a
quoted constant: it must count one); (2) a literal outside a string
reaches `function(c, ...)` (`eval.c:1612`) with a word that no table
has: the audit checks that the error is a clean "Syntax Error" and not a
read of a missing library (D2: `$90xx` is library 32/33, which the
compiler may find loaded as a userlib). The original compiler does the
same inside strings (its `litstring` is the source of the C code), which
is why C5 works for files without the header and, from TED-C's memory,
also for files with literals; the original compiler cannot read an
`Include` in `BB23` (D8).

### D6 TED-C: one stored form, whatever the options

Typing, loading and the tokeniser all produce the same stored form
whether tokenise is on or off, so `Tokenise` can be switched at any time
and the file can be saved either way. In a text save (tokenise off) a
literal is written as `cc`; the text loader turns `cc` >= $80 into a
literal (a text file has no pairs; `readline` of the interim rule of
#374 is the base: it must be changed from "the byte stays as is" to "the
byte becomes a literal or, for the six umlauts, the code 26..31").
How tokens (not literals) that are in a line are written by a text save
is #374's open point; this change does not change it.

### D7 BB23

Header: `BB23`, version 1, column, row, 0, top (14 bytes, words
big-endian), then TED's lines, no prefix. It has the layout of `BB22`
version 8's header, so `tools/lib/bb22` shares the header code; it has
neither the 1- or 2-byte line prefix nor version 10's word 2, because no
prefix has a meaning (`docs/rwelibs/README.md` §4.1: "not known"). The
marker is the file: TED-C writes `BB23` iff a line holds a pair with the
first byte `$90` (spec `formats/bb23-source`), so the header is both
the format and the flag. The cursor fields give TED-C a place to
restore the cursor on load (Q6). Detection by the 4 bytes plus the word
1: bytes `00 01` after `BB23` could only come from a TED file whose first
line is `BB23` followed by a line that begins with the control code `$01`
(a case the CD's sources do not have).

Alternatives: (a) `BB22` with version 11 and the same lines: rejected,
`BB22` is a historic format with its own readers that refuse another
version; mixing makes "is this file a source of RWE's editor?" ambiguous.
(b) A marker in the `.xtra` file instead of a header: rejected, the
`.xtra` can be missing or out of step with the source, and the original
TED would load the source without noticing.

### D8 Compatibility

| Combination | File without a literal | File with a literal |
|---|---|---|
| TED-C + blitz2-c (Cc) | as today | works |
| TED-C + original compiler (C5) | as today | compile from TED's memory works (D5); an `Include` of a `BB23` file does not (the original reads the header as text) |
| original TED + any compiler (R5, Rc) | as today | the file is not readable: it shows the header as a line of garbage and the pairs as unknown tokens (`?????`); documented, no support |
| original TED loads a text file TED-C wrote with Latin-1 | as today | as today (a text file has no header) |

The rule "a file without a literal is byte-identical" holds by saving
(D7) and by the tokeniser: a source without a byte >= $80 outside the
six umlauts tokenises exactly as before. The TED A/B pair C5-Cc stays
equal, and a script that types a literal character compares TED-C only
(R5 and Rc cannot read the result; named in the script as intended).

### D9 Host tools

`shared/xtra` gets `XTRA_T_BB23` (detection, `hdr` 14, the cursor into
`editor`); `tools/lib/bb22` reads `BB23` as a second format of the same
library (shared header and line splitting; the `BB22` code paths and
tests untouched) and `bbsrc` gets `bb23`/`bb23-info`; `bbdetok`
(`tools/lib/bbdetok/bbdetok.c:98-114`) writes a literal as its byte and
no longer as `{lib:entry}`; `tedsrc tok` uses the TED core and so gets the
tokeniser rule; `xtra dump` names the type. `bblines` needs no change
(it walks pairs).

## Open questions (for the user)

- **Q1 The marker `$90`.** It is the only unused first byte of all known
  libraries (D2), but a user's own userlib in library 32 or 33 would clash.
  Accept `$90`, or reserve a whole first byte by decision (document it as
  "libraries 32 and 33 are reserved")? Alternative: a word in
  `$FFxx` has no room (D1 C).
- **Q2 Where does a literal character count?** Everywhere (typing `é`
  outside a string gives `$90 E9` and a syntax error at compile time,
  instead of today's silent `i`; recommended), or only inside strings and
  comments (the tokeniser knows the context; outside them it would keep
  `& 0x7F` or refuse the key)?
- **Q3 Typed umlauts: keep 26..31 (D4 a, recommended) or literal pairs
  for all?** Keeping them keeps German sources in the classic format
  and in C5/R5; literals for all is uniform but turns most German
  sources into `BB23`.
- **Q4 A `BB23` file with a literal for ä, ö, ü, Ä, Ö, Ü** (written by
  a foreign tool): TED-C changes it to 26..31 on load (spec), or keeps it
  as read? It matters only for byte-exact round trips of foreign files.
- **Q5 Header layout.** The user's brief described `BB22` as "header,
  then TED's lines"; the real `BB22` has a prefix of 1 or 2 bytes per
  line. D7 drops the prefix and the second header word. Keep it
  (recommended), or take version 10's layout (16-byte header, 2-byte
  prefix) so one reader serves both?
- **Q6 TED-C loading `BB23`:** restore the cursor and the top line from
  the header (TED-C would then also write them on every save, a new
  behaviour for `BB23` files only), or ignore them and write the cursor
  of the moment? And should TED-C also read `BB22` (it cannot today)?
- **Q7 Line limit.** A literal is two bytes: a line of 128 bytes holds 64
  of them. Count characters (columns, as `chcols` shows them) or bytes?
  Today's limits (#375: 129 bytes; the original 128) are in bytes; with
  characters a line could hold up to 256 bytes, which does not fit
  `charbuff2`. Recommended: keep the byte limit; the loader splits as for
  #375.
- **Q8 Save with tokenise off.** A literal is written as the byte `cc`
  (D6). What the save does with real tokens is #374's decision (names or
  raw pairs): this change follows it. Is the order "#374 first, then this
  change" right, so #374's interim rule is replaced by D6?
- **Q9 AB3 mode: loader or compiler?** D4/AB3 turn raw bytes into
  literals in the loader (`ab3cli`, `shared/tokenizer`). The alternative
  is a mode switch in `getcharb`: inside quotes in AB3 mode a byte >= $80
  is a character, no pair. The loader keeps `getcharb` one code path for
  both modes; the compiler variant would handle AB3 token sources written
  by AmiBlitz3's own PED (`.ab3` is text only since 2014, so none exist
  today). Recommended: the loader.
- **Q10 Name of the pair in the docs** (token-table, formats): "literal
  character", `LITCHAR`; and whether `bbsrc tokens` lists it as a pseudo
  token (so a token check over the libraries can name `$90` as taken).
- **Q11 What an old TED-C build or the original TED shows** is not
  something this change can fix (D8); is a short warning in the TED-C
  save requester ("this file needs TED-C or newer, BB23") wanted when
  `BB23` is first written for a path?

## Risks / Trade-offs

- [`$90` clashes with a userlib's token] → task 1.1 scans the corpus and
  the CD's `userlibs`; Q1.
- [`tokskip` counts the line limit in bytes and a literal takes two while
  showing one column] → the byte limit stays (Q7); a host test with a
  line of 64 literals and 65 literals; no change of `chcols` code.
- [Typing a Latin-1 character outside a string used to give a wrong but
  compilable program (`é` -> `i`) and now gives a syntax error] → Q2 and
  the site entry; the old behaviour was silent corruption.
- [Two stored forms of a six-umlaut file from a foreign tool] → D4 (one
  form) and Q4.
- [The original TED shows a `BB23` file as garbage] → D8: a file
  without the token is unchanged; the format exists to make the break
  visible and not silent.
- [#374's interim rule lands first and this change replaces part of it]
  → Q8; task 2.1 re-reads what #374 landed before the loader is changed.
- [The screen font has no glyph for $80..$9F] → task 1.3 measures; such a
  character is shown with the font's default glyph (spec) and kept.

## Migration Plan

TED-C and blitz2-c are shipped together; a `BB23` file appears only when
the user types a character the format is for. Rollback is the revert of
the commits; a `BB23` file written before a rollback is read by `bbsrc
bb23` and converted with `tedsrc detok`/`tok` to the TED format (the
literals become the six-umlaut codes or are lost, as in the original).
