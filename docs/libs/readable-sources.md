# Readable library sources

The Blitz sources of the libraries (`bblibsrc.lha` in the Aminet archive,
unpacked to `archive/aminet/amibl2src/bblibsrc/`) are tokenized. The build
stores them as text, the way TED shows them – **line N of the file =
line N in TED**; line references like "bblibsrc/windowslib line 1277" in
the issues refer to this numbering:

```
cmake -P cmake/fetch-originals.cmake          # once (tests/README.md)
cmake --build --preset amiga --target bblibsrc
build/amiga/bblibsrc/NAME                     # e.g. windowslib, palettelib.aga
```

Same name as in the archive (without extension where the archive has
none); omitted are `NAME.xtra` (TED's editor state) and the data/object
files `*.bin`, `*.inc`, `*.obj`. The target belongs to `ALL` as soon as
the archive is present; without the archive it only reports that it is
skipped. The files are byte-identical to bb2 `libs-c/bb2src/NAME.bb`
(references `windowslib.bb:NNN` thus mean the same line); bb2 had only 82
of the 102. Several versions of the same library:
`palettelib`/`palettelib.aga`, `displaylib`/`displaylibfuk`/`olddisplaylib`,
`gtlib`/`oldgtlib`, `switchlib`/`switchlib.ok`,
`wbstartuplib`/`wbstartuplibnew`, `2dlib`/`sis2dlib.bb`.

The tool behind it is `teddetok` (`programs/ted/tools/teddetok`, on the C
core of TED like `tedtok`): loads a source like TED (`fileopen2`) and
writes every line detokenized. A token TED does not know (TED shows
`?????`) is an error, the file is then not written; a line longer than
TED's line buffer is split by TED when loading – `teddetok` reports that
and counts like TED.

```
build/amiga/host/ted-tools/teddetok/teddetok tests/data/ted-tokens.txt IN OUT
build/amiga/host/ted-tools/teddetok/teddetok tests/data/ted-tokens.txt --check IN...
build/amiga/host/ted-tools/teddetok/teddetok --blitz2 BLITZ2 [--lib LIB]... IN OUT
```

The token list is either a list file (`tests/data/ted-tokens.txt`, made
by the original TED) or it is made from the compiler and its libraries
(`--blitz2` and `--lib` in the compiler's order: acidlibs, deflibs, then
userlibs; library or BigLib files) as blitz2 builds it for TED – for
sources written with other libraries. `tedtok` takes the same forms.
`tools/blitztokens` writes such a list as a list file
(`blitztokens BLITZ2 LIB... > tokens.txt`), for both tools.

ctest `teddetok_*` checks it with small inputs (also broken ones: unknown
token, token byte at the end of the file, a compiler that is no load file)
and `teddetok_bblibsrc` all sources of `bblibsrc/` (skipped without the
archive); the `*_libs` tests do the same with the list made from the CD's
compiler and libraries (`tedtok_libs`: the same file as with the list
file).

Some of RWE's library sources (`rwelibsrc/`, 1997) are saved in another
format, `BB22` (`docs/rwelibs/README.md` §4.1: a header with the cursor,
a prefix before each line). `tools/bb22` converts them into TED's format,
for teddetok and the compiler; it refuses whatever it does not know:

```
build/amiga/host/tools/bb22/bb22 IN OUT
build/amiga/host/tools/bb22/bb22 --info IN
```

ctest `bb22` checks it with files built in memory (also broken ones),
`bb22_rwelibsrc` converts the 9 `BB22` files of `rwelibsrc/` and reads
them with teddetok (skipped without the archive or the CD). The
userlibs build converts a `BB22` source of `RWE_LIBS` the same way
before teddetok (target `rwelibsrc`) and before the staging of the
reference build (`build/amiga/libs/userlibs/rwe/ted/SOURCE`).

The companion settings of a source – the Blitz 2 `.xtra`, the cursor in
a `BB22` header, AmiBlitz3's "Embedded .xtra Header" of `.ab3` sources –
have one structure in `shared/xtra/xtra.h` (`docs/formats.md` §1.4, §7),
shared code that blitz2 and TED are meant to use later. `tools/xtra`
shows, checks and converts them:

```
build/amiga/host/tools/xtra/xtra dump FILE.xtra [--bb22 SRC]
build/amiga/host/tools/xtra/xtra dump --ab3 SRC.ab3
build/amiga/host/tools/xtra/xtra dump --source SRC
build/amiga/host/tools/xtra/xtra check [--bb22|--ab3] [--exact] FILE...
build/amiga/host/tools/xtra/xtra convert --to-ab3 IN.xtra OUT [--bb22 SRC | --ab3 SRC]
build/amiga/host/tools/xtra/xtra convert --to-xtra IN.ab3 OUT.xtra
```

ctest `xtra_blitz2`, `xtra_bb22`, `xtra_ab3` check the code with inputs
in memory (also broken ones), `xtra_tool` the tool on files,
`xtra_originals` all 575 `.xtra` of the CD and those of `rwelibsrc/`
(read and written again byte for byte) and the 9 `BB22` sources with
their `.xtra`, `xtra_ab3_examples` five AmiBlitz3 sources from GitHub
(`fetch-originals`; both skipped without the material).

## Sources of other editors and libraries: `bbdetok`

`teddetok` shows a source exactly as TED does and fails on a token TED
does not know. For sources written with other editors and libraries –
PED, AmiBlitz 2 and AmiBlitz3, like the snapshots in `archive/sources/` –
`tools/bbdetok` writes the text line for line (line N of the text = line
N of the file, no line split, TAB and other bytes as they are, each line
ended by an LF):

```
build/amiga/host/tools/bbdetok/bbdetok [--ted] [--strict] [--unknown] TOKENS... IN [OUT]
```

The token list is made from `TOKENS`, in the compiler's order (the first
entry of a number wins): `--tokens FILE` (a list file as
`tools/blitztokens` writes it, e.g. `tests/data/ted-tokens.txt`),
`--blitz2 FILE` (a compiler's own tokens; `--last XXXX` before it picks
one of several lists as `blitztokens -last`, blitz2-ab3: `80DB`) and
`--lib FILE` (a library or BigLib file). Without `OUT` the text goes to
stdout.

* **Rules.** By default AmiBlitz's (`docs/formats.md` §3.4): strings and
  comments are text, their bytes from `$80` ISO 8859-1 characters; in a
  comment a known token number is still a token (a line commented out).
  `--ted`: TED's, every byte from `$80` starts a token.
* **Unknown tokens** are written as `{L:N}`: the word is
  `$8000 + L*128 + N`, L the library number, N the entry of a user
  library counted from 1 (`{217:2}` is `$EC82`). The count goes to
  stderr; `--unknown` lists every number (`{217:2} $EC82 14`),
  `--strict` makes them an error.
* **Which list.** The token numbers depend on the libraries the source
  was written with, not on ours. AmiBlitz 2 and AmiBlitz3 3.00 sources
  resolve with AmiBlitz 2.42's compiler and libraries (Aminet
  distribution, `cmake -P cmake/fetch-originals.cmake`); with our
  `blitz2`, `acidlibs` and `deflibs` (the CD's numbers) dozens of
  tokens per source stay unknown (the AmiBlitz 2 libraries) and the
  compiler's own `$8014`, `$803B`, `$807F`, `$80AD` read `Jimi`,
  `Jimi`, `MOVEP`, `TAS` instead of `Syntax`, `EXTB`, `Chip`, `BSS`;
  with AmiBlitz3 v3.14.0's (`blitz2-ab3` and the BigLibs of
  `archive/amiblitz3/github-58a11658`) fewer stay unknown, but some
  numbers have other names (AmiBlitz 2's `optimize` is `ftan` there).

```
d=archive/aminet/ab2.42-distri/Amiblitz2
build/amiga/host/tools/bbdetok/bbdetok --blitz2 $d/Amiblitz2 --lib $d/AcidLibs \
    --lib $d/Deflibs archive/sources/amiblitz2-2.41/ped.bb2 ped-2.41.txt
build/amiga/host/tools/bbdetok/bbdetok --unknown --blitz2 build/amiga/blitz2/blitz2 \
    --lib build/amiga/blitz2/acidlibs --lib build/amiga/blitz2/deflibs IN > /dev/null
```

The build stores the snapshots of `archive/sources/` as text this way,
with AmiBlitz 2.42's compiler, `AcidLibs` and `Deflibs` and `--strict`
(target `archive_text`, part of `ALL` when the Aminet distribution is
there, else it only reports that it is skipped):

```
cmake --build --preset amiga --target archive_text
build/amiga/archive-text/PATH.txt     # for archive/sources/PATH, e.g.
diff build/amiga/archive-text/amiblitz2-2.41/ped.bb2.txt \
     build/amiga/archive-text/amiblitz2-2.43b12/ped.bb2.txt
```

Every `*.bb2` there except `compiler_gui.bb2` of 2.50beta, which is
plain text already. All tokens of the snapshots are known to that list
(also those of AmiBlitz3 3.00.19a; whether every name is still the one
AmiBlitz3 3.00 had is not checked, its libraries are not in the
archive).

Exit status 1 if a file cannot be read, a token list is refused, the
source ends with a token byte (no second byte), `--strict` finds an
unknown token or `OUT` cannot be written (`OUT` is then removed); 2 for
wrong arguments. A last line without its `0` (a text file) is written
with a note. ctest `bbdetok` checks the logic with sources built in
memory (also broken ones), `bbdetok_tool` the tool on files (a text
tokenised by `ab3tok` and back, an unknown token, broken arguments,
lists and sources).
