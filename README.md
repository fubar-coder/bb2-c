# Blitz Basic 2 in C

A C translation of Blitz Basic 2, built for AmigaOS (68000) with
m68k-amigaos-gcc. It produces the same executables as the original
compiler, apart from documented fixes of original bugs
(`docs/libs/deviations.md`), and is tested against the originals.

| Part | What it is |
|---|---|
| **blitz2** | the compiler, Blitz Basic 2 v2.10 (the source of 1.95 with the changes of the CD's compiler, `docs/v2.10/`) |
| **ted** | the editor, TED 1.1 |
| **acidlibs**, **deflibs** | the libraries blitz2 reads at startup: 83 command libraries and 39 declaration libraries of the AmigaOS calls |
| **blitz2-ab3** | blitz2 with an additional mode for AmiBlitz3 sources |

The repository also holds the build, the reference builds of the original
sources, the tests (host tests, comparisons with the original compiler,
editor and libraries in an emulator) and the notes on the translation in
`docs/` (analysis of the originals, deviations).

## State

Open items, decisions and the state of the port are in `TODO.md`. The
port reproduces the bugs of the original and documents them (`BUG(orig)`
in the code, `docs/*/deviations.md`); deliberate fixes are listed there as
intended deviations and can change the executables, and a few sources that
compiled now give an error. The tests of the reproduced bugs fail on
purpose (`tests/README.md`).

## Requirements

* CMake ≥ 3.28 and Ninja
* m68k-amigaos-gcc (bebbo/AmigaDev, tested with gcc 16.2-rc23) in `PATH`,
  otherwise `-DAMIGA_TOOLCHAIN_PREFIX=…` or the environment variable of the
  same name; gcc ≥ 15 because of `#embed`, 16.2-rc21 or later because of
  `__saveds` (AmigaPorts/gcc#141; configure checks it,
  `cmake/check-saveds.cmake`)
* a C compiler for the host (for the tools in `tools/`)
* for the emulator tests only: Amiberry and your own Kickstart ROM and
  hardfiles (`AMIGA-EMULATION-SETUP.md`)

## Building

```
cmake --preset amiga
cmake --build --preset amiga
cmake --install build/amiga        # optional: build/amiga/install/blitz2/
```

The result in `build/amiga/blitz2/` is the contents of the `blitz2:`
directory; `cmake --install` copies all of it:

| File | Contents |
|---|---|
| `blitz2` | the compiler |
| `ted` | the editor (blitz2 loads it as `blitz2:ted`) |
| `acidlibs` | the command libraries |
| `deflibs` | the declaration libraries of the AmigaOS calls |
| `blitz2-ab3` | the compiler with AmiBlitz3's mode as well |
| `ab3cli` | the headless editor of a command line run of `blitz2-ab3` (loaded as `PROGDIR:ab3cli`, so it stays next to `blitz2-ab3`) |

The preset `amiga-debug` builds the same code (`-Os`) with symbols and
without strip. The build checks its results itself; if a check fails, the
result counts as not built (`docs/development.md`). Variants of the
libraries and userlibs: `docs/libs/variants.md`. Using an IDE (CLion and
others): `docs/development.md`.

## Testing

```
ctest --preset host       # on the host, without emulator (seconds)
ctest --preset emulator   # in Amiberry; needs the setup below
ctest --preset all        # both
```

Some tests compare with the original CD and sources, which are not in the
repository; `cmake -P cmake/fetch-originals.cmake` downloads them (about
40 MB, `archive/README.md`), and the affected tests are skipped without
them. The emulator tests also need Amiberry, a Kickstart ROM and hardfiles
set up locally: `AMIGA-EMULATION-SETUP.md`. The suites, the presets, the
tests of reproduced original bugs (`ctest --preset bug-orig`), statistics
and coverage: `tests/README.md`.

## Extensions of blitz2

`blitz2` compiles a Blitz Basic 2 source to the same executable as the
CD's compiler, apart from the documented bug fixes
(`docs/libs/deviations.md`). Its extensions only give a meaning to
sources the original refuses. The one a program author meets is
`#__debug`, a constant the compiler defines: −1 if the program is
compiled with runtime error checking, 0 without. Code for debugging
only goes between `CNIF` and `CEND`:

```
CNIF #__debug
  NPrint "only with runtime error checking"
CEND
```

Details and tests: `docs/blitz2/extensions.md`.

## blitz2-ab3: AmiBlitz3 sources

`blitz2-ab3` compiles Blitz Basic 2 sources exactly as `blitz2` and, from
the shell without the editor, AmiBlitz3 sources with AmiBlitz3 v3.14.0's
token list and binary libraries (AB3 mode). It needs AmiBlitz3's files
under `Blitz3:`. Example:

```
cd Blitz3:
blitz2:blitz2-ab3 -s Sourcecodes/Amiblitz3/Tests/atoi.ab3 -e RAM:atoi
RAM:atoi
```

Options, modes, `ab3cli` and the tests: `docs/ab3/usage.md`; what
AmiBlitz3 changed relative to Blitz Basic 2.10: `docs/ab3/`.

## Directories

```
programs/blitz2/    the compiler (src/: the sections of blitz2.s + glue)
programs/ted/       the editor (src/core/: the part free of OS calls)
programs/ab3cli/    headless editor of blitz2-ab3's command line
libs/               acidlibs/, deflibs/ (one C file per library), userlibs/,
                    variants/, include/, tests/
shared/             code compiled by several programs (file requester,
                    .xtra settings, tokenizer, ...)
host/               the host side: tools and host tests, built with the
                    native compiler
tools/              host tools (C) and their libraries in tools/lib/
tests/              corpus, TED A/B, bug-orig, AB3, test data
docs/               analysis of the originals, notes on the translation
archive/            unchanged originals (sources, reference data)
cmake/              toolchain, helpers of the build
openspec/           OpenSpec specifications and changes
amiga-emulation/    your local ROMs and hardfiles (not in git)
```

The full listing: `docs/structure.md`.

## Further reading

* `docs/README.md`: index of the documentation (development, structure,
  libraries, the translation notes)
* `tests/README.md`: the test suites and how to run them
* `AMIGA-EMULATION-SETUP.md`: setting up the emulator tests locally
* `tools/lib/amiga_emulation/README.md`: the emulation tool, for developers
* `docs/ab3/`: AmiBlitz3's changes and the AB3 mode
* <https://fubar-coder.github.io/bb2-c/>: the project website (sources in
  `site/`, see `site/README.md`)
* `TODO.md`: open items; `CLAUDE.md`: working rules of the repository
* `archive/README.md`: the originals and their licenses

## License

GPL-3.0-or-later (`LICENSE`) with a runtime exception
(`RUNTIME-LIBRARY-EXCEPTION`): programs compiled by blitz2 may distribute
the code that blitz2 copies into them from the libraries (`acidlibs`,
`deflibs`) and its code templates under any license; the compiler, editor
and libraries themselves remain under the GPL.

The basis is the source code of Blitz Basic 2 and TED 1.1, which Acid
Software released as "under GNU" (without a version number – according to
GPL 2 § 9 one may then choose any version). The exception is granted by
the contributors of this project for the rights they hold; for the
translated original parts nobody can grant it bindingly, since the rights
holders cannot be reached. What is known about the license of the
originals is in `archive/README.md`.
