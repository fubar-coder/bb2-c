# tedsrc

Puts a Blitz 2 source through the C core of TED on the host, exactly as
the original TED treats it (the same for every source of the CD:
`test_loadsave` and `test_detok` against the TED oracle): `tok` loads it
(`fileopen2`, ted.s:680, `tokenise = 1`) and saves it again (`realsave`,
ted.s:769), so a text source comes out tokenised as TED would save it;
`detok` loads it and writes every line detokenised (`detok`, ted.s:3455),
line N of the output is line N in TED. The program links the TED core
(`tedhost`, `tedcore_host`), which is why it is a program of its own
(`tools/bbdetok`, `tools/blitztokens`: the tokens and
sources without TED; `bbdetok --ted` is the nearest to `detok`).

## Usage

```
usage: tedsrc tok TOKENS IN OUT
       tedsrc tok --blitz2 BLITZ2 [--lib LIB]... IN OUT
       tedsrc detok TOKENS IN OUT
       tedsrc detok TOKENS --check IN...
       tedsrc detok --blitz2 BLITZ2 [--lib LIB]... IN OUT
       tedsrc detok --blitz2 BLITZ2 [--lib LIB]... --check IN...
```

`tedsrc SUB -h` prints the description and the options of `SUB`.

The token list is the list file `TOKENS` (`tests/data/ted-tokens.txt`, or
one written by `blitztokens`), or it is made from the compiler `BLITZ2`
and the library or BigLib files `LIB` in the compiler's order (acidlibs,
deflibs, userlibs) as blitz2 builds it for TED (`tools/lib/blitztokens`).

### tok

Loads `IN` (text or tokenised) and writes it tokenised to `OUT`. A text
line longer than TED's line buffer (129 characters) is refused: TED would
cut it.

### detok

Writes `IN` as TED shows it to `OUT`, one line per line of TED, each with
an LF. The text is TED's, with its quirks: TAB expanded to tabstop 2, a
line longer than TED's line buffer (129 bytes, tokenised) split into
several (noted on stderr), a text line tokenised and detokenised again.
A token TED does not know (TED shows `?????`) or a token byte without its
second byte is an error. `--check IN...` only checks the files, without
output.

## Inputs and outputs

Blitz 2 sources, tokenised or text (`docs/formats.md`); the token list
file `tests/data/ted-tokens.txt`. `tok` writes a tokenised source, `detok`
a text file (Latin-1, LF).

## Exit codes

* 0 success;
* 1 `IN` cannot be loaded, the token list cannot be read or made, `OUT`
  is not written; `tok`: a text line longer than TED allows; `detok`: an
  unknown token or a token byte without its second byte (`OUT` is then
  removed);
* 2 usage error.

Messages go to stderr as `tedsrc SUB: [FILE: ]message`.

## Callers

* `tok`: the tokenised sources of the corpus (`tests/corpus`), of
  `tests/ab3`, `tests/blitz2`, `tests/bug-orig`, `tests/ted-ab` and of the
  RWE reference build (`libs/userlibs/rwe`); the tests `tedtok_line129`,
  `tedtok_line130`, `tedtok_libs` (`CMakeLists.txt` here).
* `detok`: the build target `bblibsrc` (`bblibsrc.cmake` here), the RWE
  sources (`libs/userlibs/rwe`), the tests of `bb22`
  (`tools/bb22/rwelibsrc.cmake`); the tests `teddetok_*`
  (`CMakeLists.txt` here, `tests/run.cmake`, `tests/bblibsrc.cmake`).
* Tests: `help_tedsrc`, `readme_tedsrc`.

## Example

```
$ tedsrc detok tests/data/ted-tokens.txt programs/ted/tools/tedsrc/tests/good.bb2 good.txt
$ cmp good.txt programs/ted/tools/tedsrc/tests/good.txt
```
