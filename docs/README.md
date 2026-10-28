# Documentation: analysis and translation

Findings from the analysis of the original sources (`blitz2.s`, `ted.s`,
libraries) and notes on the translation to C: surveys, reviews,
deviations from the original, conventions. This is where the reasons for
code comments like `BUG(orig)`, `DIFF`, `ted.s:NNN` or `blitz2.s:NNN`
are given.

**Taken over from the working repository bb2, state `ee6174b`
(28.10.2026), unchanged in content and translated to English line by
line.** Paths, commands and line references in the files therefore refer
to the layout of bb2 (e.g. `libs-c/libs/gtlib.c`,
`quelltext/amibl2src/…/blitz2.s`); the mapping is given below. The line
numbers of the files are the same as in bb2.

| here | bb2 |
|---|---|
| `architektur.md`, `dokumentation.md`, `formate.md`, `token-tabelle.md` | same name in the top-level directory |
| `blitz2/` | `blitz2-c/notes/`; `CONVENTIONS.md` from `blitz2-c/`, `bb2-readme.md` = `blitz2-c/README.md` |
| `ted/` | `ted-c/notes/`; `bb2-readme.md` = `ted-c/README.md` |
| `libs/` | `libs-c/notes/`; `bb2-readme.md` = `libs-c/README.md` |

Not taken over: the work orders (`*BRIEF*.md`), the detokenised library
sources (`libs-c/bb2src/*.bb`, from material without a clear licence;
the same files are generated locally by the build target `bblibsrc`, see
`README.md`), the results of the corpus runs (`compilerlauf/`), token
dumps (`tokens/`), `toolchain.md` and `ci-exploration.md`.

Mapping of the paths in the texts:

| bb2 | here |
|---|---|
| `quelltext/amibl2src/files/amiblitz2source/bb2src.zip.dir/blitz2.s` | `archive/sources/blitz2-1.95/blitz2.s` |
| `…/bb2src.zip.dir/tedt/ted.s` | `archive/sources/ted-1.1/ted.s` |
| `blitz2-c/src/` | `programs/blitz2/src/` |
| `ted-c/src/` | `programs/ted/src/` |
| `libs-c/libs/` | `libs/acidlibs/` |
| `libs-c/deflibs/`, `libs-c/include/` | `libs/deflibs/`, `libs/include/` |
| `cd/` | `archive/cd/ultimate-blitz-basic-2.1/` (upper case, see `archive/README.md`) |
| `compilerlauf/corpus/` | `tests/corpus/` |
| `libs-c/bb2src/NAME.bb` | `build/<preset>/bblibsrc/NAME` (build target `bblibsrc`) |
