# toklist

The compiler's own token list,
[`programs/blitz2/src/toklist.h`](../../programs/blitz2/src/toklist.h),
read as text: its BASIC keywords (the `TK` entries) as the JSON file
[`site/data/compiler/blitz2.json`](../../site/data/compiler/blitz2.json),
from which the website's grammar of the Blitz Basic code blocks takes the
compiler's keywords (`site/src/lib/grammar.mjs`). `json` writes the file
or checks the committed one. The logic is `tools/lib/toklist`
(`toklist.h`: the entries of the list, the JSON and the comparison);
tested by `tools/tests/test_toklist.cpp`.

## Usage

```
Usage: toklist [OPTIONS] SUBCOMMAND

The compiler's own token list (programs/blitz2/src/toklist.h): its BASIC
keywords as JSON for the website.

OPTIONS:
  -h,     --help              Print this help message and exit

SUBCOMMANDS:
  json                        blitz2's keywords as JSON, written or checked
```

```
Usage: toklist json [OPTIONS] TOKLIST

blitz2's keywords as JSON, written or checked

POSITIONALS:
  TOKLIST TEXT REQUIRED       the token list (programs/blitz2/src/toklist.h)

OPTIONS:
  -h,     --help              Print this help message and exit
  -o,     --out FILE          write the JSON to FILE (default: stdout)
          --check FILE        compare the committed file FILE with the generated JSON

Without --out and --check the JSON goes to stdout. With --check it is written
only to --out, and only if FILE equals it (a file at --out is removed
otherwise). Exit status 0, 1 the files differ (one message per differing line,
FILE:LINE: ..., at most ten, and their number), a malformed token list (TOKLIST:
line N: ...), or a file that cannot be read or written, 2 usage error.
```

The list is an X-macro list (its head comment): `TK` a BASIC keyword,
`TA` an assembler mnemonic, `TKLAST` the last node of blitz2's list,
`TK3`/`TK3LAST` the nodes only AmiBlitz3's list has; a name or help is a
string literal or `M("blitz2's", "AmiBlitz3's")`. The export holds
blitz2's keywords: the `TK` entries and the `TKLAST` one, in the order of
the list, with blitz2's name and help. The `TA` mnemonics are left out
(the operators `AND`, `OR`, `EOR`, `NOT`, `LSL`, `LSR`, `ASL`, `ASR` of
an expression are such nodes; the grammar names them itself), and so are
AmiBlitz3's nodes. A list that does not have this form fails with
`TOKLIST: line N: ...` (an unknown entry, a wrong number of arguments, a
name or help that is no string, an empty name, an unsupported escape, an
unclosed comment, string or entry, the entries in another order).

## Inputs and outputs

The JSON:

```
{
  "compiler": "blitz2",
  "generated": "by tools/toklist (toklist json) from the TK entries of ...",
  "source": "programs/blitz2/src/toklist.h",
  "keywords": [
    {"name": "NEWTYPE", "help": ".Typename"},
    ...
  ]
}
```

one keyword per line; in a string printable ASCII as it is (`"` and `\`
escaped), every other byte as `\u00XX`, as `blitzlib cmdref` writes the
library files of the site. The messages of a failed check go to stderr:
`FILE:LINE: differs from the generated file:` with the committed line and
the generated one, `FILE:LINE: missing:` (the committed file ends
earlier), `FILE:LINE: not in the generated file:`, at most ten, then
`FILE: lines that differ from the generated file: N` and the line that
names the update target.

## Exit codes

0 success; 1 the files differ, a malformed token list, or a file that
cannot be read or written; 2 usage error.

## Callers

* `programs/blitz2/CMakeLists.txt`: after every link of `blitz2`
  (POST_BUILD) `json --check` compares the committed
  `site/data/compiler/blitz2.json` with the list, so a file that differs
  fails the build; `blitz2` relinks after a change of `src/` and of the
  file. The target `blitz2_keywords_update` writes the committed file
  after an intended change of the keywords. The ctest
  `blitz2_keywords_broken` (`programs/blitz2/tests/keywords-broken.cmake`):
  a copy of the list with a changed keyword must fail the check, naming
  the line.
* `tools/toklist/CMakeLists.txt`: the ctest `toklist_json`, the same check
  in the build of the host side.

## Example

```
$ toklist json --check site/data/compiler/blitz2.json toklist.h
site/data/compiler/blitz2.json:79: differs from the generated file:     {"name": "Wend", "help": ""},
  generated:     {"name": "WEnd", "help": ""},
site/data/compiler/blitz2.json: lines that differ from the generated file: 1
site/data/compiler/blitz2.json: if the change of toklist.h is intended, build the target blitz2_keywords_update and commit the file
```
