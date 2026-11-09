# tedsrc

Puts a Blitz 2 source through the C core of TED on the host, exactly as
the original TED treats it (the same for every source of the CD:
`test_loadsave` and `test_detok` against the TED oracle): `tok` loads it
(`fileopen2`, ted.s:680, `tokenise = 1`) and saves it again (`realsave`,
ted.s:769), so a text source comes out tokenised as TED would save it;
`detok` loads it and writes every line detokenised (`detok`, ted.s:3455),
line N of the output is line N in TED. The program links the TED core
(`tedhost`, `tedcore_host`), which is why it is a program of its own
([`tools/bbsrc`](../../../../tools/bbsrc/README.md): the tokens and
sources without TED; `bbsrc detok --ted` is the nearest to `detok`).

## Usage

```
Usage: tedsrc [OPTIONS] SUBCOMMAND

Blitz 2 sources through the C core of TED on the host.

OPTIONS:
  -h,     --help              Print this help message and exit

SUBCOMMANDS:
  tok                         a source through TED (load and save, tokenised) on the host
  detok                       a Blitz 2 source (tokenised or text) as TED shows it, one line
                              per line of TED
```

The arguments `FILE...` of the subcommands:

* `tedsrc tok TOKENS IN OUT`, `tedsrc tok --blitz2 BLITZ2 [--lib LIB]... IN OUT`;
* `tedsrc detok TOKENS IN OUT`, `tedsrc detok --check TOKENS IN...`, and
  the same with `--blitz2 BLITZ2 [--lib LIB]...` instead of `TOKENS`.

The token list is the list file `TOKENS` (`tests/data/ted-tokens.txt`, or
one written by `bbsrc tokens`), or it is made from the compiler `BLITZ2`
and the library or BigLib files `LIB` in the compiler's order (acidlibs,
deflibs, userlibs) as blitz2 builds it for TED (`tools/lib/blitztokens`).

### tok

```
Usage: tedsrc tok [OPTIONS] FILE...

a source through TED (load and save, tokenised) on the host

POSITIONALS:
  FILE TEXT ... REQUIRED      TOKENS IN OUT, or IN OUT with --blitz2

OPTIONS:
  -h,     --help              Print this help message and exit
          --blitz2 BLITZ2     the token list made from this compiler and the LIBs instead of
                              TOKENS
          --lib LIB           with --blitz2: a library or BigLib file, in the compiler's order
                              (acidlibs, deflibs, userlibs)

TOKENS: the token list file (tests/data/ted-tokens.txt, or one bbsrc tokens
writes). A text source comes out tokenised, as TED saves it. Exit status 0, 1 IN
cannot be loaded, a line of a text source is longer than TED allows, a token
list cannot be read or OUT is not written, 2 usage error.
```

Loads `IN` (text or tokenised) and writes it tokenised to `OUT`. A text
line longer than TED's line buffer (129 characters) is refused: TED would
cut it.

### detok

```
Usage: tedsrc detok [OPTIONS] FILE...

a Blitz 2 source (tokenised or text) as TED shows it, one line per line of TED

POSITIONALS:
  FILE TEXT ... REQUIRED      TOKENS IN OUT (TOKENS IN... with --check), without TOKENS with
                              --blitz2

OPTIONS:
  -h,     --help              Print this help message and exit
          --check             only the checks of the files IN..., no output
          --blitz2 BLITZ2     the token list made from this compiler and the LIBs instead of
                              TOKENS
          --lib LIB           with --blitz2: a library or BigLib file, in the compiler's order
                              (acidlibs, deflibs, userlibs)

TOKENS: the token list file (tests/data/ted-tokens.txt, or one bbsrc tokens
writes). TED's text: TAB expanded to tabstop 2, a line longer than TED's line
buffer split (noted on stderr). Exit status 0, 1 IN cannot be loaded, a line has
a token TED does not know or a token byte without its second byte, a token list
cannot be read or OUT cannot be written (OUT is removed), 2 usage error.
```

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
  sources (`libs/userlibs/rwe`), the test `bb22_rwelibsrc`
  (`tools/bbsrc/bb22-rwelibsrc.cmake`); the tests `teddetok_*`
  (`CMakeLists.txt` here, `tests/run.cmake`, `tests/bblibsrc.cmake`).
* Tests: `help_tedsrc`, `readme_tedsrc`.

## Example

```
$ tedsrc detok tests/data/ted-tokens.txt programs/ted/tools/tedsrc/tests/good.bb2 good.txt
$ cmp good.txt programs/ted/tools/tedsrc/tests/good.txt
```
