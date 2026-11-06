# Tasks

## 1. Table data

- [x] 1.1 `tools/lib/blitzlib`: keep forms (count word, type bytes), result type, AmigaOS-call base/offset/registers and the `!libfin` object (D1). Verify: `test_blitzlib` checks them on tables built in memory (a function with two forms and a repetition group, an AmigaOS call, an object library); `blitzlibcheck`, `blitzlibcompare` and `blitztokens` tests unchanged; `ctest --preset host`

## 2. The tool

- [x] 2.1 `tools/lib/cmdref` + `tools/cmdref/main.c`: JSON of one library file (D2), including `opens`. Verify: host tests with inputs from memory - a command library, an AmigaOS call library with and without a library name, escaping of a non-ASCII help byte, broken inputs (table past the end, two library names) give an error and no output file

## 3. The check in the build

- [x] 3.1 `blitz_library(... CMDREF GROUP)`: `cmdref` and `cmake -E compare_files` in the library's build step; acidlibs and deflibs of the default variant and every RWE userlib; target `cmdref_update`; the JSON files committed under `site/data/libraries/` (D3). Verify: a clean build passes the check; `cmdref_check_broken` fails as it should; a changed committed file fails the build step
- [x] 3.2 ctest `cmdref_complete` and `cmdref_complete_broken` (D3). Verify: both pass (the broken one by failing)

## 4. The pages

- [ ] 4.1 Content adapter, partials and the `library-list` shortcode; sections `commands`, `amigaos`, `userlibs` with hand-written `_index.md`; the slot `site/data/libintros/`; the Libraries overview links the sections and the API reference; `site/README.md` describes data, slot and update (D4). Verify: the site builds with no errors or warnings; links checked; screenshots of an overview, a command library (windowslib), an AmigaOS library (dos.library with doslib2's calls), a userlib

## 5. API reference

- [ ] 5.1 `Doxyfile` (D5) and the comment fixes it needs, comments only. Verify: Doxygen 1.18 in its official image runs with no warning; the table macros are not listed; programs, acidlibs, deflibs and every library file byte-identical to the build before; clean build with no warnings
- [ ] 5.2 Pages workflow: job `api`, artifact into `site/static/api/` before Hugo; `.gitignore`. Verify: the workflow file is valid YAML and the steps run locally in the same images (Doxygen, then Hugo with `static/api/` present); screenshot of the API start page

## 6. Convention and docs

- [ ] 6.1 The `@brief` convention (command name first) in `docs/development.md`, the API reference and the command reference in `docs/development.md`/`docs/structure.md`; `TODO.md` entry of the function documentation updated (the optional Doxygen target is the Pages job)
