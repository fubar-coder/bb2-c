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
