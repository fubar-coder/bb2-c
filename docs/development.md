# Development: IDE and checks in the build

## IDE (CLion and others)

The build uses two compilers: the m68k-amigaos-gcc for everything that
runs on the Amiga, the native compiler for the host side (`host/`: the
tools the build uses and the host tests). One CMake build has one
compiler per language, so the m68k build runs the host side as
`ExternalProject` (`build/<preset>/host`) – an IDE does not see its
targets. Hence two profiles from `CMakePresets.json`:

* **`amiga`** (or `amiga-debug`): the real build. Editing, navigation and
  building of blitz2, ted and the libraries; the m68k programs do not run
  on the host – their tests run through CTest (the emulator tests in
  Amiberry). Without a preset the m68k toolchain is the default too.
* **`native`** (`BLITZ2_HOST=ON`, `build/native`, Debug): only the host
  side as a project of its own, native compiler – every tool and host
  test is a target of its own, to run and debug in the IDE; CTest lists
  its 58 tests one by one (`ctest --preset native`).

CLion reads the presets as CMake profiles (Settings → Build, Execution,
Deployment → CMake: enable `amiga` and `native`; the profile CLion adds
itself, `Debug` in `cmake-build-debug`, is not needed). The toolchain is
found through `PATH`, else in `/opt/amiga` or `/opt/m68k-amigaos`, else
set `AMIGA_TOOLCHAIN_PREFIX` (environment or cache). Tests are not
projects of their own: they are targets of the same CMake project,
registered with `add_test`, and run by CTest – in CLion through a CTest
run configuration (labels as arguments, e.g. `-LE emulator`). The host
tests use snitch (a Catch2-like framework), which CLion does not know
as a test framework; they run as CTest tests and as plain programs.
`.clangd` (generated) is for clangd-based editors; it holds the m68k
options clangd does not know.

The first configure writes `.clangd` (from `cmake/clangd.in`, with the
include paths of the toolchain found; not in the repository). After
changing the toolchain, delete `.clangd` and configure again; switch off
with `-DBLITZ2_CLANGD=OFF`.

## Function comments and the references

The functions are documented with Doxygen comments
(`/** @brief ... @param ... @return ... */`), which clangd shows on
hover and from which the API reference of the website is made. In the
libraries:

* The `@brief` of a routine that implements a Blitz command starts with
  the command's name (`@brief WMouseY - ...`, `@brief GTMenuItem
  GTMenuList#,...: ...`); a routine for several commands names them all.
  The Blitz syntax itself stays in `BLITZ_NAME`.
* `@param NAME REG: ...` and `@return REG: ...` give the register
  (`@param a d0: the dividend`, `@return d0: the quotient`); a void
  routine has no `@return`; what it keeps is a sentence (`Returns
  nothing (all registers kept).`).
* Routines in assembler (`BLITZ_INLINE`, `BLITZ_ASM_ROUTINE`,
  `BLITZ_AB_ROUTINE`) have no C parameters and no C result: their
  register arguments are `@arg` items and their result a paragraph
  `@par Result` followed by `REG: ...`.
* A global function name occurs only once in the input of the API
  reference: Doxygen joins global functions of one name across files,
  so one library's page would show another's description. A routine
  that only its own file refers to is `static` (init/finit routines in
  C such as `codeinit`, `a_doblit_c`); an entry point in assembler (a
  thunk, `BLITZ_ASM_ROUTINE`, `BLITZ_INLINE`, ...) whose name another
  library also has gets the library's name as prefix
  (`audiolib_codeinit`, `debuglib_docall`). Comments keep the original's
  label (`codeinit`).
* Original bugs: `BUG(orig) (#NNN)`; Blitz notation (`#ld0`, `Window#`)
  is no link.

**API reference** (C developers): `Doxyfile` in the repository root,
over `libs/include`, `deflib.h`, the sources of acidlibs and of all
userlibs (`libs/userlibs`, a new userlib directory is included), and
`shared/` (main page `docs/api-mainpage.md`). Not part of
the CMake build: the Pages workflow runs it in
`ghcr.io/doxygen/doxygen:Release_1_16_1` with warnings as errors into
`site/static/api/` (not committed), served as `/bb2-c/api/`. A comment
error (a `@param` of no parameter, a `@return` on a void routine, an
unclosed backquote) fails the publication of the site; run it locally
as `site/README.md` describes.

**Command reference** (Blitz programmers): the command tables of the
built library files, `tools/cmdref`, committed in `site/data/libraries/`
and rendered by Hugo (`site/README.md`); checked in the build, see
below.

## Checks in the build

The build checks its results itself; if a check fails, the result counts
as not built, and nothing that depends on it is built (the next run checks
again):

* every library file (`tools/blitzlibcheck`, in the same step as link and
  strip): one code hunk, relocs only into it, library header, token table
  readable up to the end, no reference from the normal code into the error
  check area (without the debugger it would point behind the end of the
  code; the originals have such references, the port has them fixed).
  `acidlibs`/`deflibs`
  are made only from checked libraries.
* the command reference of the website (`tools/cmdref`, in the same
  step as link and strip of every library file of acidlibs and deflibs of
  the default variant and of every RWE userlib): the command table the
  tool writes from the library file (`build/<preset>/cmdref/GROUP/NAME.json`)
  equals the committed `site/data/libraries/GROUP/NAME.json`. After an
  intended change of a library's declarations, build the target
  `cmdref_update` (copies the generated files into the site) and commit
  them with the change. ctest `cmdref_complete`: no committed file
  without a library; `cmdref_check_broken`, `cmdref_complete_broken`:
  the checks with broken input.
* `acidlibs`, `deflibs` (also of every variant), in the same step as the
  join, and every userlib in its build step (against the acidlibs and
  deflibs of its variant): no library number twice (`biglib check`; deflibs together with
  the acidlibs loaded with it) – two libraries with one number would have
  the same tokens.
* `blitz2` after every link (POST_BUILD): `programs/blitz2/checklink.cmake`
  on the link map - no libnix20 member and no base of a library that
  Kickstart 1.3 lacks (`UtilityBase`): blitz2 starts on Kickstart 1.x like
  the original (#936; ctest `blitz2_checklink_broken_*` with the map of
  the earlier link).
* reference build of blitz2: the original
  `archive/sources/blitz2-1.95/blitz2.s` with vasm to
  `build/amiga/programs/blitz2/ref/` (`blitz2`, listing, `blitz2.map`,
  `blitz2.labels`), in the same step `vasmlisting verify`.
* reference build of TED: the original `archive/sources/ted-1.1/ted.s`
  with vasm (like Devpac) to `build/amiga/programs/ted/ref/` (`ted`,
  listing, `ted.map`, `ted.labels`); the same step checks that listing and
  binary match (`vasmlisting verify`). The reference for the port is this
  build, not the TED binary of the CD (a different build).
* `ted` after every link (POST_BUILD): hunk layout (`tools/hunkcheck`:
  offset 0 `bsr.w`, offset 4 `outside`; `--stored`: every hunk stored
  whole, as `ted` and `ab3cli` have their `.bss` in the data hunk and
  Kickstart 1.x's `LoadSeg` would not clear an unstored tail, #963 – the
  linker script `cmake/baserel-nostart.ld` stores it as zeros; the same
  for `ab3cli`) and `programs/ted/checklink.cmake`
  (no libgcc/libnix helper functions that would need `UtilityBase`) and
  `tools/checklabels`: every code label of `ted.s` exactly once as
  `ted.s:NNN label` in `programs/ted/src/` (completeness of the
  translation; a changed comment line rebuilds the object, so the check
  runs then as well).
* the residents of the RWE reference build (`tools/blitzres`, in the
  same step as the write): read back, the file holds exactly the
  constants written. Its staging: `tools/bblines` fails on an `INCDIR`
  line with a further statement, the `LIBNUMS.BB` step on a number of
  `libnums.h` the CD's file has with another value (ctest
  `rwe_refbuild_libnums_broken`).
* `ted.sym` (the same link with symbols) after every link:
  `tools/checkdata` compares the data of the C build with the reference
  build of `ted.s` (`programs/ted/checkdata.tsv`: 534 entries – fonts,
  mouse pointers, option block, texts, screen, windows, gadgets, menus,
  requesters; relocated longwords of the reference skipped). `ted` is
  linked only after that.
