# Blitz Basic 2 in C

A C translation of Blitz Basic 2, built for AmigaOS (68000) with
m68k-amigaos-gcc. It produces the same executables as the original
compiler, apart from documented fixes of original bugs
(`docs/*/deviations.md`), and is tested against the originals.

| Part | What it is |
|---|---|
| **blitz2** | the compiler, Blitz Basic 2 v2.10 (`docs/v2.10/`) |
| **ted** | the editor, TED 1.1 |
| **acidlibs**, **deflibs** | the libraries blitz2 reads at startup: the command libraries and the declarations of the AmigaOS calls |
| **blitz2-ab3** | blitz2 with an additional mode for AmiBlitz3 sources |

The repository also holds the build, the reference builds of the original
sources, the tests and the notes on the translation in `docs/`.

## State

Open items, decisions and the state of the port are in `TODO.md`. The
port reproduces the bugs of the original and documents them (`BUG(orig)`
in the code, `docs/*/deviations.md`); deliberate fixes are listed there as
intended deviations and can change the executables. The tests of the
reproduced bugs fail on purpose (`tests/README.md`).

## Requirements

* CMake ≥ 4.2.3 and Ninja
* m68k-amigaos-gcc (bebbo/AmigaDev) in `PATH`, otherwise
  `-DAMIGA_TOOLCHAIN_PREFIX=…` or the environment variable of the same
  name; configure checks that it is recent enough
  (`cmake/check-saveds.cmake`)
* a C and C++ compiler for the host (tools and host tests)
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

The build checks its results itself; if a check fails, the result counts
as not built. Other presets, the checks and using an IDE:
`docs/development.md`; variants of the libraries: `docs/libs/variants.md`.

## Testing

```
ctest --preset host       # on the host, without emulator
ctest --preset emulator   # in Amiberry
ctest --preset all        # both
```

Some tests compare with the original CD and sources, which are not in the
repository; `cmake -P cmake/fetch-originals.cmake` downloads them
(`archive/README.md`), and the affected tests are skipped without them.
The emulator tests also need Amiberry, a Kickstart ROM and hardfiles set
up locally: `AMIGA-EMULATION-SETUP.md`. The suites, the presets and
coverage: `tests/README.md`.

## Extensions of blitz2

`blitz2` compiles a Blitz Basic 2 source to the same executable as the
CD's compiler, apart from the documented bug fixes. Its extensions only
give a meaning to sources the original refuses: `docs/blitz2/extensions.md`.

## blitz2-ab3: AmiBlitz3 sources

`blitz2-ab3` compiles Blitz Basic 2 sources exactly as `blitz2` and, from
the shell without the editor, AmiBlitz3 sources with AmiBlitz3's token
list and binary libraries (AB3 mode). It needs AmiBlitz3's files under
`Blitz3:`. Example: a text file `hello.ab3` with

```
a$ = Trim$("  Hello  ")
NPrint "[", a$, "]"
End
```

(`Trim$` exists only in AmiBlitz3's libraries) is compiled and run with

```
blitz2:blitz2-ab3 -s hello.ab3 -e RAM:hello
RAM:hello
```

Usage, the supported AmiBlitz3 version and the tests:
`docs/ab3/usage.md`; what AmiBlitz3 changed relative to Blitz Basic 2:
`docs/ab3/`.

## Directories

```
programs/           the compiler, the editor and ab3cli
libs/               the libraries
shared/             code compiled by several programs
host/               the host side: tools and host tests
tools/              host tools and their libraries (tools/README.md)
tests/              the test suites and their data
docs/               analysis of the originals, notes on the translation
archive/            unchanged originals
cmake/              toolchain, helpers of the build
openspec/           OpenSpec specifications and changes
site/               the project website
amiga-emulation/    your local ROMs and hardfiles (not in git)
```

The full listing: `docs/structure.md`.

## Further reading

* <https://fubar-coder.github.io/bb2-c/>: the project website (sources in
  `site/`, see `site/README.md`)
* `docs/README.md`: index of the documentation
* `tests/README.md`: the test suites and how to run them
* `AMIGA-EMULATION-SETUP.md`: setting up the emulator tests locally
* `docs/ab3/`: AmiBlitz3's changes and the AB3 mode
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

The host tools are built with CLI11 (BSD-3-Clause, a build-time
dependency, `cmake/CLI11.cmake` downloads a fixed release when the build
is configured); nothing of it is in the m68k programs and libraries.
