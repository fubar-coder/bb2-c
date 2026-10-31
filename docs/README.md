# Documentation: analysis and translation

Findings from the analysis of the original sources (`blitz2.s`, `ted.s`,
libraries) and notes on the translation to C: surveys, reviews,
deviations from the original, conventions. This is where the reasons for
code comments like `BUG(orig)`, `DIFF`, `ted.s:NNN` or `blitz2.s:NNN`
are given.

**Taken over from the working repository bb2, state `ee6174b`
(28.10.2026), unchanged in content and translated to English line by
line.** Paths, commands and line references have since been adapted to
this repository: paths point to the files here, line references into
the C code (`file.c:NNN`) were determined anew by content, commands and
tools of bb2 are replaced by their counterparts here (e.g.
`tools/asmcmp`, `tools/blitzlibcompare`, `tests/ted-ab/`, ctest names).
Line references into the originals (`blitz2.s:NNN`, `ted.s:NNN`, library
sources) are unchanged; the originals are the same files. The line
structure of the files is mostly that of bb2.

| here | bb2 |
|---|---|
| `architecture.md`, `documentation.md`, `formats.md`, `token-table.md` | `architektur.md`, `dokumentation.md`, `formate.md`, `token-tabelle.md` in the top-level directory |
| `blitz2/` | `blitz2-c/notes/`; `CONVENTIONS.md` from `blitz2-c/`, `bb2-readme.md` = `blitz2-c/README.md` |
| `ted/` | `ted-c/notes/`; `deviations.md` = `abweichungen.md`, `bb2-readme.md` = `ted-c/README.md` |
| `libs/` | `libs-c/notes/`; `deviations.md` = `abweichungen.md`, `bb2-readme.md` = `libs-c/README.md` |
| `ab3/` | – (new here, 2026-10-31: what AmiBlitz3 changed relative to BB2 2.10) |

Not taken over: the work orders (`*BRIEF*.md`), the detokenised library
sources (`libs-c/bb2src/*.bb`, from material without a clear licence;
the same files are generated locally by the build target `bblibsrc`, see
`README.md`), the results of the corpus runs (`compilerlauf/`), token
dumps (`tokens/`), `toolchain.md` and `ci-exploration.md`.

Reading the texts:

* In `blitz2/` and `ted/`, paths like `src/…` or `tests/host/…` are
  relative to the component: `programs/blitz2/`, `programs/ted/`.
* Phase plans and logs (`ted/PLAN.md`, `ted/PHASE3-PLAN.md`,
  `ted/PHASE3-LOG.md`, `blitz2/PHASE-B.md`), reviews, surveys and the
  former READMEs (`bb2-readme.md`) are the history of the work in bb2:
  their content is unchanged, only paths to things that exist here are
  adapted. Tools, build directories and results mentioned there that
  have no counterpart here (Python scripts `*.py`, `results/`,
  `build/host/…`, `tools/amiberry.cmake` …) are those of bb2 and were
  not taken over.
* Remaining historical mentions of bb2 paths read as follows:

| bb2 | here |
|---|---|
| `quelltext/amibl2src/files/amiblitz2source/bb2src.zip.dir/blitz2.s` | `archive/sources/blitz2-1.95/blitz2.s` |
| `…/bb2src.zip.dir/tedt/ted.s` | `archive/sources/ted-1.1/ted.s` |
| `…/bb2src.zip.dir/`, `…/bblibsrc.lha.dir/bblibsrc/` | `archive/aminet/amibl2src/bb2src/`, `…/bblibsrc/` |
| `blitz2-c/src/`, `ted-c/src/` | `programs/blitz2/src/`, `programs/ted/src/` |
| `libs-c/libs/`, `libs-c/deflibs/`, `libs-c/include/` | `libs/acidlibs/`, `libs/deflibs/`, `libs/include/` |
| `cd/` | `archive/cd/ultimate-blitz-basic-2.1/` (upper case, see `archive/README.md`) |
| `compilerlauf/corpus/`, `compilerlauf/tedstub/` | `tests/corpus/`, `tests/corpus/amiga/` |
| `ted-c/tests/amiga/`, `ted-c/tests/emu/` | `tests/ted-ab/amiga/`, `tests/ted-ab/scripts/` |
| `libs-c/bb2src/NAME.bb` | `build/<preset>/bblibsrc/NAME` (build target `bblibsrc`) |
