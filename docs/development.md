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
* Original bugs: `BUG(orig) (#NNN)` while not fixed, `Fixed (#NNN)`
  once fixed; Blitz notation (`#ld0`, `Window#`) is no link.

## Fixing an original bug

Original bugs are fixed deliberately (CLAUDE.md "Original bugs are fixed
deliberately"; spec `testing/bug-fix-workflow`). A fix delivers:

1. one issue per bug;
2. in the fix commit: the code change; the test of the expected
   behaviour - the bug-orig test gets the header `; @fixed #NNN` (or the
   equivalent of its suite) and must come out FIXED in
   `ctest --preset all` (`tests/README.md`, "Fixed tests"); the comment
   `BUG(orig) (#NNN)` becomes `Fixed (#NNN)`; the entry in
   `docs/*/deviations.md` as an intended deviation of kind F; the
   changed results (executables, TED's files, library files) named in
   the commit message, every other result byte-identical to the
   previous commit; and a line `Fixes #NNN` in the commit message
   (GitHub closes the issue on the push);
3. the entry on the website page of fixed original bugs
   (the area page in `site/content/docs/compared-with-original/fixed-bugs/`:
   a one-sentence entry, or a section with an example of the original's
   and the port's output for a bug ordinary programs meet; a source the
   port refuses or compiles differently also goes into `new-errors.md`);
4. the label `fixed-in-port` on the issue (every fix);
5. after the push: the issue is closed (by `Fixes #NNN`; otherwise with
   a comment naming the commit).

A bug not fixed yet keeps its `BUG(orig)` comment and its bug-orig test,
which comes out REPRODUCED in `ctest --preset bug-orig`. The test is
judged against `ID.orig`, the recorded observation of the original
(`ID.expect` is the correct behaviour); no session of the original runs
in the normal tests. A new test gets its `ID.orig` once from
`ctest --preset originals -R bug_orig_record_ID` (needs Amiberry and the
CD), and the file is committed with the test; a recording run
(`ctest --preset originals`, or `BUGORIG_RECORD=1` for `corpusrun pass` and
`tedab run`) writes nothing but the `.orig` files and is the only run that
needs the original compiler and TED (`tests/README.md`, "Recorded
originals"). A fixed test (`; @fixed #NNN`) has no `ID.orig`: delete the
file with the fix.

The instruction comparison of the acidlibs routines that stay assembler
(`asmcmp_runerrlib`, `asmcmp_debuglib`, `asmcmp_dhandlerlib`,
`asmcmp_memacclib`; `refcheck asm`, the routine lists of `asm_check` in
`libs/acidlibs/CMakeLists.txt`) lists only routines still equal to the
original: a fix that changes a listed routine removes the routine from
its `asm_check` list in the same commit and names it in the commit
message. There is no list of intended differences for instructions.

The expected data that came from the originals changes in the fix commit
as well, never the test program around it (`tests/README.md`):

* a corpus program's output (`tests/corpus/expect/PASS/NAME.out`,
  `NAME.serial`): written from the port's run by
  `cmake --build --preset amiga --target corpus-expect-update`, checked
  with `git diff tests/corpus/expect` ("Corpus");
* a result of a CD source (`tests/data/blitz2-cd.tsv`): the target
  `blitz2-cd-update` rewrites the row, a note `#NNN: ... (2.10: ...)`
  names the CD's result ("Corpus");
* an output of TED's detok, tokit2 or load and save
  (`tests/data/ted-oracle.sha256`): the row gets the port's hash and a
  note `#NNN: ... (TED 1.1: HASH)` with the original's ("Oracle data of
  the TED tests").

The target that regenerates the file from the original (`ted-oracle`,
`blitz2-cd-reference`) keeps those rows.

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
built library files, `blitzlib cmdref`, committed in `site/data/libraries/`
and rendered by Hugo (`site/README.md`); checked in the build, see
below.

## Checking a rework against the previous commit

A rework changes no result; a fix changes only the results its commit
names (CLAUDE.md "Acceptance"). The build of the previous commit, the
`prev` build, makes that a test:

```
cmake --preset amiga -DBLITZ2_PREV=ON   # once; HEAD~1, or a revision, or a build directory
cmake --build --preset amiga            # builds build/amiga/prev/ from git archive (the checkout stays untouched)
ctest --preset all                      # prev_results, corpus (config prev), ted_ab (P5, Pc)
```

`prev_results` compares `acidlibs`, `deflibs` and every library file
byte by byte. The corpus compares the executables of the previous
compiler with those of the current one, and `ted_ab` runs the previous
TED. `-DPREV_INTENDED=ON` or a list names the differences a fix changes,
each of which must really differ: a library path
(`libs/acidlibs/2dlib.obj`), `corpus:NAME` (also `corpus_runerr:NAME` and
the other corpus passes) for a corpus source, `ted_ab:CASE[:C5-P5|:Cc-Pc]`
for a TED A/B case, e.g.
`-DPREV_INTENDED="libs/acidlibs/2dlib.obj;corpus:t01_int;corpus_runerr:t01_int"`.
A run that finds differences prints the list to quote in the commit
message (`prev differs in: PREV_INTENDED="..."`). Options, what is compared and how the
tests are skipped without a prev build: `tests/README.md`, "The prev
build". Without the option nothing of this is configured or built;
`prev_results` is then skipped.

## Checks in the build

The build checks its results itself; if a check fails, the result counts
as not built, and nothing that depends on it is built (the next run checks
again):

* every library file (`blitzlib check`, in the same step as link and
  strip): one code hunk, relocs only into it, library header, token table
  readable up to the end, no reference from the normal code into the error
  check area (without the debugger it would point behind the end of the
  code; the originals have such references, the port has them fixed).
  `acidlibs`/`deflibs`
  are made only from checked libraries.
* the command reference of the website (`blitzlib cmdref`, in the same
  step as link and strip of every library file of acidlibs and deflibs of
  the default variant and of every RWE userlib): the command table the
  tool writes from the library file (`build/<preset>/cmdref/GROUP/NAME.json`)
  equals the committed `site/data/libraries/GROUP/NAME.json`. After an
  intended change of a library's declarations, build the target
  `cmdref_update` (copies the generated files into the site) and commit
  them with the change. ctest `cmdref_complete`: no committed file
  without a library; `cmdref_check_broken`, `cmdref_complete_broken`:
  the checks with broken input.
* a fixed bug-orig test keeps no recorded original (`ID.orig` next to a
  `.bas` with `; @fixed #N` or a TED `.drv` with the `@fixed` directive):
  configuring `tests/bug-orig` fails naming the file
  (`tests/bug-orig/check-fixed-orig.cmake`; ctest
  `bug_orig_fixed_orig_broken` with a fake tree).
* `acidlibs`, `deflibs` (also of every variant), in the same step as the
  join, and every userlib in its build step (against the acidlibs and
  deflibs of its variant): no library number twice (`blitzlib numbers`; deflibs together with
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
  `blitz2.labels`), in the same step `refcheck listing-verify`
  ([`tools/refcheck`](../tools/refcheck/README.md)).
* reference build of TED: the original `archive/sources/ted-1.1/ted.s`
  with vasm (like Devpac) to `build/amiga/programs/ted/ref/` (`ted`,
  listing, `ted.map`, `ted.labels`); the same step checks that listing and
  binary match (`refcheck listing-verify`). The reference for the port is this
  build, not the TED binary of the CD (a different build).
* `ted` after every link (POST_BUILD): hunk layout (`hunkcheck start`,
  [`tools/hunkcheck`](../tools/hunkcheck/README.md): offset 0 `bsr.w`,
  offset 4 `outside`; `hunkcheck stored`: every hunk stored
  whole, as `ted` and `ab3cli` have their `.bss` in the data hunk and
  Kickstart 1.x's `LoadSeg` would not clear an unstored tail, #963 – the
  linker script `cmake/baserel-nostart.ld` stores it as zeros; the same
  for `ab3cli`) and `programs/ted/checklink.cmake`
  (no libgcc/libnix helper functions that would need `UtilityBase`) and
  `refcheck labels`: every code label of `ted.s` exactly once as
  `ted.s:NNN label` in `programs/ted/src/` (completeness of the
  translation; a changed comment line rebuilds the object, so the check
  runs then as well).
* the residents of the RWE reference build (`blitzres create`,
  [`tools/blitzres`](../tools/blitzres/README.md), in the
  same step as the write): read back, the file holds exactly the
  constants written. Its staging: `bbsrc comment` fails on an `INCDIR`
  line with a further statement, the `LIBNUMS.BB` step on a number of
  `libnums.h` the CD's file has with another value (ctest
  `rwe_refbuild_libnums_broken`).
* `ted.sym` (the same link with symbols) after every link:
  `refcheck data` compares the data of the C build with the reference
  build of `ted.s` (`programs/ted/checkdata.tsv`: 534 entries – fonts,
  mouse pointers, option block, texts, screen, windows, gadgets, menus,
  requesters; relocated longwords of the reference skipped); the checks
  of `programs/ted/checkdata-intended.txt`, if there, differ on purpose
  (below). `ted` is linked only after that.

### Intended differences

The comparisons with the originals accept the change of a fixed original
bug only as a listed difference with its issue. In every list `#`
begins a comment line and empty lines are skipped; every entry must
occur: a stale entry (the difference is gone) fails like a difference
the list does not name. The build finds a list at configure time:
configure again after adding one. The fix commit adds its entry (or,
for `refcheck asm`, removes its routine).

* **Declarations** (`blitzlib compare`; ctests
  `declarations_acidlibs`, `declarations_deflibs`,
  `rwe_declarations_NAME`): `libs/GROUP/declarations-intended.txt` for
  acidlibs and deflibs, `libs/userlibs/rwe/NAME.declarations-intended.txt`
  for an RWE userlib. One message line of `blitzlib compare` per line,
  exactly as it prints it; the comment above names the issue. From
  `libs/deflibs/declarations-intended.txt`:

  ```
  # #959: CachePreDMA_/CachePostDMA_ take flags in d0 (exec_lib.fd), the
  # original declares d1 (register byte $01 -> $00)
  lib 253: entry 113 CachePreDMA_ +$C: $01 != $00
  lib 253: entry 114 CachePostDMA_ +$C: $01 != $00
  ```

* **TED's data** (`refcheck data` after the link of `ted.sym`):
  `programs/ted/checkdata-intended.txt`, one check of
  `programs/ted/checkdata.tsv` per line - its C symbol, the issue, a
  note (optional), separated by tabs; a symbol at most once. The whole
  check may differ; a missing symbol or label still fails. For example
  (with tabs):

  ```
  # the fixed original bugs among the data of ted.s
  _mpalit	#123	the palette value the fix corrects
  ```

* **Instructions** (`refcheck asm`; ctests `asmcmp_runerrlib`,
  `asmcmp_debuglib`, `asmcmp_dhandlerlib`, `asmcmp_memacclib`): no list.
  The routines of `asm_check` in `libs/acidlibs/CMakeLists.txt` are those
  still equal to the original; a fix that changes one removes it from
  the list in the same commit ("Fixing an original bug").

The checks of the lists themselves, with deliberately broken input:
`blitzlib_compare` and `checkdata` (`tools/tests`, lists from memory:
correct, stale, an unlisted difference), `blitzlibcompare_single_stale`,
`ted_checkdata_intended`, `ted_checkdata_unlisted`,
`ted_checkdata_stale` (the tools on built files).

## Host programs

The host programs (`tools/`, the TED tools in `programs/ted/tools/`, the
runners in `tests/corpus/runner/` and `tests/ted-ab/runner/`) are built
by the host side (`host/`) and called by the build and the tests; nothing
installs them. Each one has a `README.md` in its directory; what it does
and how it is called is written there and in its `-h`, not in other
documents (they name the program in one line and link the README).

**Syntax.** A program with several modes has subcommands:
`PROGRAM SUB [OPTIONS] ARGS`, the subcommand first, its name lower case
with `-` between words; options are `-x` or `--name` flags of one
subcommand and follow it. A mode is a subcommand, never a flag or the
position of an argument. A program with one mode is called
`PROGRAM [OPTIONS] ARGS`. Programs that work on the same kind of input
for the same callers are one program; a program that links the TED core
(`tedhost`, `tedcore_host`) is not merged with one that does not.

**Dispatch.** The programs parse their command line with CLI11
(`cmake/CLI11.cmake`, a fixed release, BSD-3-Clause) through the shell
`tools/lib/cli/cli.hpp` (C++20, header only); no program compares `argv`
entries itself. The program and command layer is C++: `main.cpp` only
declares the subcommands on a `CLI::App` and returns `cli::run()`; each
subcommand is `void cmd_NAME(CLI::App &app, int &status)` in
`cmd_NAME.cpp` next to it, which adds the subcommand with a one-line
description, declares its options and arguments bound to its own state,
and sets the status in its callback. The logic stays C in `tools/lib/`
(or the component's library); C++ includes its headers inside
`extern "C" { ... }`. A program with one mode declares its options on the
`CLI::App` itself. Programs not moved to CLI11 yet have a table of
`subcmd_main()` (`tools/lib/subcmd/subcmd.h`) and parse their options by
hand. The smallest example is the fake program of the checks' tests,
`tools/tests/fake_cli11.cpp`:

```cpp
static void cmd_alpha(CLI::App &app, int &status)
{
    struct options {
        bool x = false;
        std::string file;
    };
    auto o = std::make_shared<options>();
    CLI::App *sub = app.add_subcommand("alpha", "the first subcommand");
    sub->add_flag("-x", o->x, "a flag");
    sub->add_option("FILE", o->file, "the input")->required();
    sub->callback([sub, o, &status] {
        std::printf("%s: %s%s\n", cli::name(*sub).c_str(), o->file.c_str(), o->x ? " -x" : "");
        status = 0;
    });
}
```

```cpp
int main(int argc, char **argv)
{
    CLI::App app{"The fake program of the help and README checks.", "fake_cli11"};
    int status = 0;
    app.require_subcommand(1);
    cmd_alpha(app, status);
    cmd_beta(app, status);
    return cli::run(app, argc, argv, status);
}
```

```
$ fake_cli11 -h
Usage: fake_cli11 [OPTIONS] SUBCOMMAND

The fake program of the help and README checks.

OPTIONS:
  -h,     --help              Print this help message and exit

SUBCOMMANDS:
  alpha                       the first subcommand
  beta                        the second subcommand
$ fake_cli11 alpha -x f
fake_cli11 alpha: f -x
$ fake_cli11 alpha
FILE is required
Run with --help for more information.
```

CLI11 calls the callback only after the whole command line was parsed, so
a usage error runs nothing. `cli::run()` prints the help for `-h` or
`--help` (of the program, or after a subcommand of that subcommand) on
stdout with status 0; every command line CLI11 rejects - no or an unknown
subcommand, an unknown option, a missing value, a surplus or missing
argument, a value a check refuses - gives CLI11's message and "Run with
--help for more information." on stderr and status 2; otherwise the
status the callback set. The help has one shape (`cli::Formatter`, CLI11's
formatter at a fixed width, without colour): the `Usage:` line first, the
description, `POSITIONALS:`, `OPTIONS:`, `SUBCOMMANDS:` and the footer
(the longer text of a subcommand, its exit codes). `cli::name(*sub)` is
`PROGRAM SUB`, the prefix of the subcommand's messages. Options: a flag
`add_flag("-x", var, ...)`, a value `add_option("--name", var, ...)`
(`--name VALUE` or `--name=VALUE`; a value that starts with `-` is given
as `--name=VALUE`), a repeatable one bound to a `std::vector`, arguments
as `add_option("NAME", var)` without dashes (a name holds no `:` or `=`),
flags that exclude each other with `->excludes()`. An option bound to a
`std::vector` takes every following argument that is no option; a
repeatable option with N values each time is
`cli::add_repeated(*sub, "--name", var, N, ...)`, and
`cli::occurrences(*sub)` gives the options in the order they were given,
each time with its values (operations applied in that order).

**Exit codes and messages.** 0 success; 1 the checked thing failed or an
input could not be processed; 2 usage error (also an unknown subcommand
or option); 77 skipped (an optional input such as the CD or the previous
build, or the emulator, is missing). Where a program returns 2 for an
unreadable input, it stays so; its README says which. Messages go to
stderr as `PROGRAM SUB: [FILE: ]message` (`argv[0]` of the subcommand),
results to stdout or the named output file.

**Tests.** `host_help_test(PROGRAM CLI [SUB...])` (`host/CMakeLists.txt`)
adds `help_PROGRAM` (`cmake/check-help-cli.cmake`): `-h` and `--help` of
the program and of every subcommand exit 0, stdout starts with
`Usage: PROGRAM` (`Usage: PROGRAM SUB`), stderr is empty; an unknown
option gives status 2 and a message on stderr; and every subcommand of the
`SUBCOMMANDS:` section of the program's help is in the list.
`host_readme_test(PROGRAM README.md CLI [SUB...])` adds `readme_PROGRAM`
(`cmake/check-readme-cli.cmake`): the output of `PROGRAM -h` and of every
`PROGRAM SUB -h` is in the README verbatim (trailing blanks aside), and
every fenced block of the README that starts with `Usage: PROGRAM` is one
of them. When a help changes, the test fails and prints the new help in
a fenced block: that is the text for the README. The checks with broken
input: `help_check_cli_*` and `readme_check_cli_*` (`tools/tests`, the
fake program `fake_cli11` and the READMEs in `tools/tests/fake-readme-cli`).
Without `CLI` the two functions run the checks of the programs on
`subcmd` (`cmake/check-help.cmake`: synopsis lines `usage: PROGRAM SUB`;
`cmake/check-readme.cmake`: every synopsis line of `-h` is a line of the
README and the other way round; their tests `help_check_*`,
`readme_check_*` with `fake_subcmd`). Adding a subcommand: the
`cmd_NAME.cpp`, its call in `main.cpp`, its name in the program's
`host_help_test` and `host_readme_test` calls, its help block and a
paragraph in the README.

**README.** The sections, in this order (`tools/hdftool/README.md` is an
example):

````
# PROGRAM

What it is for, in a paragraph; where its logic is (tools/lib/NAME).

## Usage

```
(the output of PROGRAM -h, verbatim)
```

```
(the output of PROGRAM SUB -h, verbatim, for every subcommand)
```

One paragraph per subcommand: what it does, its options in words.

## Inputs and outputs

File formats (a link to docs/formats.md where it has one), what goes to
stdout, which files are written.

## Exit codes

## Callers

The CMake files, targets and tests that run each subcommand.

## Example

A call that was run, with its output.
````
