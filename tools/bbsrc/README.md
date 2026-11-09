# bbsrc

Works on Blitz Basic 2 and AmiBlitz sources and their tokens, without
TED: the token list a compiler hands to the editor, made from the
compiler and its libraries (`tokens`); a tokenised source as plain text
(`detok`); an edit of the lines of a tokenised source on its bytes
(`comment`); a source in the "BB22" format of some RWE library sources in
TED's format (`bb22`, `bb22-info`); a source tokenised as AmiBlitz3
v3.14.0 loads it (`ab3`, `ab3-cmp`). The logic is in `tools/lib/blitztokens`,
`tools/lib/bbdetok`, `tools/lib/bblines`, `tools/lib/bb22` and
`shared/tokenizer`, tested by `tools/tests/test_blitztokens.c`,
`test_bbdetok.c`, `test_bblines.c`, `test_bb22.c` and `test_tokenizer.cpp`.
A source as TED loads, saves and shows it is
[`tedsrc`](../../programs/ted/tools/tedsrc/README.md) (on the C core of
TED); `detok --ted` is the nearest to `tedsrc detok`.

## Usage

```
Usage: bbsrc [OPTIONS] SUBCOMMAND

Blitz Basic 2 sources and tokens without TED.

OPTIONS:
  -h,     --help              Print this help message and exit

SUBCOMMANDS:
  tokens                      the token list of Blitz Basic 2 as the compiler hands it to the
                              editor
  detok                       a tokenised Blitz 2 / AmiBlitz source as plain text, line for
                              line
  comment                     the lines of a tokenised source whose statement is TOKEN as ";"
  bb22                        a Blitz 2 source in the BB22 format in TED's format
  bb22-info                   the header of a BB22 source, its line count and its longest line
  ab3                         a source tokenised as AmiBlitz3 v3.14.0 loads it, every line with
                              its 0
  ab3-cmp                     a source tokenised as bbsrc ab3 does it, compared with
                              AmiBlitz3's -save output
```

### tokens

```
Usage: bbsrc tokens [OPTIONS] BLITZ2 [LIBS...]

the token list of Blitz Basic 2 as the compiler hands it to the editor

POSITIONALS:
  BLITZ2 TEXT REQUIRED        the compiler (blitz2:blitz2)
  LIBS TEXT ...               library or BigLib files in the order the compiler links them

OPTIONS:
  -h,     --help              Print this help message and exit
  -o OUT                      the list written (default: stdout)
          --last XXXX         of several lists in BLITZ2 (blitz2-ab3) the one whose last token
                              is $XXXX (hex)

The compiler's own tokens, then the tokens of LIBS (in the compiler's order:
acidlibs, deflibs, userlibs), one "XXXX name" per line like
tests/data/ted-tokens.txt. Exit status 0, 1 a file cannot be read or holds no
token list, or OUT cannot be written, 2 usage error.
```

Writes the token list of Blitz Basic 2 as the compiler hands it to the
editor: the tokens of the compiler `BLITZ2`, then those of `LIBS`
(library or BigLib files in the compiler's order: acidlibs, deflibs, the
files of userlibs), one `XXXX name` per line like
`tests/data/ted-tokens.txt`, to `OUT` (`-o`) or stdout. `--last XXXX`: of
several lists in `BLITZ2` (blitz2-ab3) the one whose last token is
`$XXXX` (hex; AmiBlitz3's: `80DB`).

### detok

```
Usage: bbsrc detok [OPTIONS] IN [OUT]

a tokenised Blitz 2 / AmiBlitz source as plain text, line for line

POSITIONALS:
  IN TEXT REQUIRED            the tokenised source
  OUT TEXT                    the text written (default: stdout)

OPTIONS:
  -h,     --help              Print this help message and exit
          --ted               TED's rules: every byte from $80 a token, also in strings and
                              comments (default: AmiBlitz's, strings and comments as they are)
          --strict            fail on a token the list does not have (else written as {L:N})
          --unknown           list the unknown tokens on stderr: {L:N} $XXXX count
          --tokens FILE       a token list file ("XXXX name" per line, bbsrc tokens)
          --blitz2 FILE       a compiler's own tokens
          --last XXXX         for the next --blitz2: its list that ends with $XXXX
          --lib FILE          a library or BigLib file (acidlibs, deflibs, userlibs)

TOKENS: at least one of --tokens, --blitz2, --lib, in the compiler's order (the
first entry of a number wins). A token not in the list is written as {L:N}, the
count goes to stderr. Exit status 0, 1 a file cannot be read, a token list is
refused, the source ends with a token byte or (--strict) has unknown tokens, or
OUT cannot be written (OUT is removed), 2 usage error.
```

Writes the tokenised source `IN` as plain text, line N of the text = line
N of the file, to `OUT` or stdout. `TOKENS`, in the compiler's order (the
first entry of a number wins): `--tokens FILE` a token list file,
`--blitz2 FILE` a compiler's own tokens, `--last XXXX` for the next
`--blitz2` its list that ends with `$XXXX`, `--lib FILE` a library or
BigLib file. `--ted`: TED's rules (every byte from `$80` a token, also in
strings and comments), else AmiBlitz's (strings and comments as they
are). A token not in the list is written as `{L:N}` and counted on
stderr; `--unknown` lists every such number, `--strict` makes it an
error.

### comment

```
Usage: bbsrc comment [OPTIONS] TOKEN IN OUT

the lines of a tokenised source whose statement is TOKEN as ";"

POSITIONALS:
  TOKEN TEXT REQUIRED         the token word (hex, e.g. 80BF = INCDIR)
  IN TEXT REQUIRED            the tokenised Blitz 2 source
  OUT TEXT REQUIRED           the source written

OPTIONS:
  -h,     --help              Print this help message and exit

Every line of IN whose statement is the token TOKEN becomes ";" in OUT, the
other lines stay byte for byte. Exit status 0, 1 IN cannot be read or is broken,
or OUT cannot be written (OUT is not left), 2 usage error or a TOKEN that is no
hex word.
```

Every line of the tokenised source `IN` whose statement is the token
`TOKEN` (hex, e.g. `80BF` = INCDIR) becomes `;` in `OUT`; the other lines
stay byte for byte.

### bb22, bb22-info

```
Usage: bbsrc bb22 [OPTIONS] IN OUT

a Blitz 2 source in the BB22 format in TED's format

POSITIONALS:
  IN TEXT REQUIRED            the BB22 source (some of RWE's library sources, 1997)
  OUT TEXT REQUIRED           the source written: lines of tokenised text, each ended by 0

OPTIONS:
  -h,     --help              Print this help message and exit

Refuses whatever it does not know. Exit status 0, 1 IN cannot be read or is
refused (OUT is not written) or OUT cannot be written, 2 usage error.
```

```
Usage: bbsrc bb22-info [OPTIONS] IN

the header of a BB22 source, its line count and its longest line

POSITIONALS:
  IN TEXT REQUIRED            the BB22 source

OPTIONS:
  -h,     --help              Print this help message and exit

The version, the cursor of the header, the line count and the longest line, on
stdout. Refuses whatever it does not know. Exit status 0, 1 IN cannot be read or
is refused, 2 usage error.
```

`bb22` converts a source in the BB22 format (`tools/lib/bb22/bb22.h`,
`docs/rwelibs/README.md` 4.1) to TED's format (lines of tokenised text,
each ended by 0) in `OUT`; `bb22-info` prints its header (version,
cursor), its line count and its longest line. Both refuse whatever they
do not know, with the offset or line.

### ab3, ab3-cmp

```
Usage: bbsrc ab3 [OPTIONS] TOKENS SOURCE OUT

a source tokenised as AmiBlitz3 v3.14.0 loads it, every line with its 0

POSITIONALS:
  TOKENS TEXT REQUIRED        the token list ("XXXX name" per line, as bbsrc tokens writes it)
  SOURCE TEXT REQUIRED        the source: a .bb or .bb2 as it is, any other name tokenised as
                              text
  OUT TEXT REQUIRED           the lines written (the form of AmiBlitz3's -save NAME.bb2)

OPTIONS:
  -h,     --help              Print this help message and exit

Text is loaded without its embedded header (not for .ab2) and cut into lines of
at most 250 characters. Exit status 0, 2 usage error, a file that cannot be read
or written or a broken TOKENS.
```

```
Usage: bbsrc ab3-cmp [OPTIONS] TOKENS SOURCE THEIRS

a source tokenised as bbsrc ab3 does it, compared with AmiBlitz3's -save output

POSITIONALS:
  TOKENS TEXT REQUIRED        the token list ("XXXX name" per line, as bbsrc tokens writes it)
  SOURCE TEXT REQUIRED        the source: a .bb or .bb2 as it is, any other name tokenised as
                              text
  THEIRS TEXT REQUIRED        AmiBlitz3's -save output

OPTIONS:
  -h,     --help              Print this help message and exit

Exit status 0 equal, 1 not equal (the first line that differs on stderr), 2
usage error, a file that cannot be read or a broken TOKENS.
```

`ab3` tokenises `SOURCE` as AmiBlitz3 v3.14.0 loads it in CLI mode (its
`-save NAME.bb2` form: every line of the editor with its 0) with the
token list `TOKENS` (`XXXX name` per line, as `tokens` writes it) and
writes it to `OUT`: a `.bb` or `.bb2` as it is, any other name tokenised
as text without its embedded header, in lines of at most 250 characters.
`ab3-cmp` compares the result with `THEIRS` (AmiBlitz3's `-save` output)
and names the first line that differs.

## Inputs and outputs

Tokenised and text sources (`docs/formats.md`), token list files
(`XXXX name` per line), load files of a compiler, library and BigLib
files, BB22 sources. `tokens` writes a token list, `detok` text,
`comment`, `bb22` and `ab3` tokenised sources; `bb22-info` and `ab3-cmp`
write only a report.

## Exit codes

* 0 success (`ab3-cmp`: equal);
* 1 `tokens`: a file cannot be read, is refused or `OUT` cannot be
  written; `detok`: a file cannot be read, a token list is refused, the
  source ends with a token byte, `--strict` and unknown tokens, `OUT`
  cannot be written (`OUT` is then removed); `comment`: a file cannot be
  read or written, a line with the token and more statements; `bb22`,
  `bb22-info`: `IN` cannot be read or is refused, `OUT` cannot be
  written; `ab3-cmp`: the lines differ;
* 2 usage error; also `tokens --last` or `comment TOKEN` not a token
  number; `ab3`, `ab3-cmp`: a file that cannot be read or written, a
  broken token list.

Messages go to stderr as `bbsrc SUB: [FILE: ]message`; the refusals of
`bb22` and `bb22-info` are `FILE: offset N: message`.

## Callers

* `tokens`: the tests `tokens_cd`, `tokens_port`, `tokens_broken`
  (`libs/CMakeLists.txt`, `tokens-check.cmake` here), the variant tests
  (`libs/tests/CMakeLists.txt`, `libs/tests/variants/check.cmake`), the
  token lists of the RWE bug-orig tests (`tests/bug-orig/CMakeLists.txt`),
  `ab3_tokens*` and `ab3_tokenise*` (`tests/ab3/tokens.cmake`,
  `tokenise.cmake`).
* `detok`: the build target `archive_text` (`detok-archive-text.cmake`
  here); the test `bbdetok_tool` (`detok-tooltest.cmake`).
* `comment`: the RWE reference build (`libs/userlibs/rwe/refbuild.cmake`).
* `bb22`: the readable RWE sources (`libs/userlibs/rwe/CMakeLists.txt`),
  the test `bb22_rwelibsrc` (`bb22-rwelibsrc.cmake`); `bb22-info` by hand.
* `ab3`, `ab3-cmp`: the test `ab3tok_tool` (`ab3-tooltest.cmake`);
  `ab3-cmp` in `ab3_tokenise*` (`tests/ab3/tokenise.cmake`).
* Tests: `help_bbsrc`, `readme_bbsrc` (`CMakeLists.txt` here).

## Example

```
$ bbsrc tokens -o tokens.txt archive/cd/ultimate-blitz-basic-2.1/BLITZ2/BLITZ2 \
    archive/cd/ultimate-blitz-basic-2.1/BLITZ2/ACIDLIBS archive/cd/ultimate-blitz-basic-2.1/BLITZ2/DEFLIBS
$ cmp tokens.txt tests/data/ted-tokens.txt
```
