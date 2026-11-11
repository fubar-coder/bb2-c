# Design

## Context

- Every library file starts with a header and a token table
  (`libs/include/blitzlib.h`, the table macros). `tools/lib/blitzlib`
  reads it for `blitzlibcheck`, `blitzlibcompare` and `blitztokens`, but
  skips the argument types of the forms and the registers of the
  AmigaOS calls.
- Each library file is made by one custom command of
  `blitz_library()` (`cmake/BlitzLibrary.cmake`): link, strip,
  `blitzlibcheck`, and for userlibs `biglib check`. acidlibs and deflibs
  of every variant, and the test variants, use the same function.
- Per entry the table has: the kind word (statement 1, function 2,
  command 3, +8 typed by the command's suffix such as `Peek.w`, the
  result type in the high byte; 0 nullsub, 8 object name/keyword
  (`!dumtoke`), 6 AmigaOS call), the forms (count word with the
  repetition group in its high byte, one type byte per argument), the
  name and the help text. The help texts are the original's, typos
  included, and 59 of acidlibs are empty.
- The site is Hugo 0.154.5 with Hextra, built by the Pages workflow in
  `hugomods/hugo:exts-0.154.5`; it has no layouts of its own yet.
- The Doxygen trial of the analysis (1.18.0, all of `libs/` and
  `shared/`): 1184 warnings, most of them configuration (Blitz `#name`
  read as links, parameter-list macros not expanded, `@return` on void
  routines whose thunk returns d0), a few genuine comment mismatches.

## Goals / Non-Goals

**Goals:** a command reference that cannot drift from the libraries; an
API reference with no warnings; no change to any build result.

**Non-Goals:** linking a command to its routine's Doxygen comment (the
library file has no C names); the per-library introductions (written
later by hand); building the Amiga libraries in CI.

## Decisions

### D1 The table data in `tools/lib/blitzlib`

`struct blitzlib_entry` gets the forms (`struct blitzlib_form`: the count
word, a pointer to the type bytes inside the code and their number),
the result type, and for an AmigaOS call the base library, offset and
the register bytes; `struct blitzlib` gets the object of `!libfin`
(the name its `toke` long points to, the default maximum). The forms of
all entries are one array of the library, the entries index into it,
so `blitzlib_free` stays one place. The walk and its bounds checks are
unchanged; the existing users only gain fields.

### D2 `cmdref`: one JSON file per library file

`cmdref [-o OUT] --group GROUP --name NAME LIBFILE` writes a JSON object:
`name`, `group`, `number`, optionally `opens` and `object`, and
`entries` in table order (nullsubs left out). An entry has `name`,
`kind` (`statement`, `function`, `command`, `object`, `oscall`),
`typed` (the command takes a type suffix), `help`, `result` (type word
of a function or command), `forms` (each `args`: type and flags, and
`repeat` `{from, count}` of a repetition group); an AmigaOS call has
`base`, `offset`, `registers`. Type words: `byte`, `word`, `long`,
`quick`, `float`, `string`, `any` (type 8, the type is passed along),
`suffix` (type 0: the command's suffix); flags `address` (address of a
variable), `array`, `arraytype`, `arrayend`, `stack` (pushed). Strings
are written as JSON with every byte outside printable ASCII escaped as
`\u00XX` (the help texts are Latin-1 at most), so the output is ASCII
and stable. Keys and entries keep a fixed order, two-space indentation,
a final newline - the file is meant to be diffed.

`opens`: an AmigaOS call library with an init opens its library by a
name that lies in its code behind the table (`libname[]` of every
`libs/deflibs/*.c`). The tool takes the one 0-terminated printable
string in the code that ends in `.library`, `.resource`, `.device` or
`.gadget`; none for a library without init (`doslib2`, `graphicslib2`
add calls of another library's base), more than one is an error.

### D3 The check in the library's build step

`blitz_library(... CMDREF GROUP)` adds two commands to the custom
command that makes `NAME.obj`: `cmdref` writes `cmdref/NAME.json` in the
build directory (a byproduct, not a temporary file: it is the file to
copy when the change is intended), then `cmake -E compare_files` against
`site/data/libraries/GROUP/NAME.json`. A difference fails the step, so
the library file counts as not built. The JSON name is the library name
without the target prefix (`deflib_`, the variant prefix of userlibs).
Only acidlibs and deflibs of the default variant (`bb2-2.10`) and every
RWE userlib (each is built once, in its variant) pass `CMDREF`; other
variants would give other library numbers. A target `cmdref_update`
copies all generated files into `site/data/libraries/` for an intended
change.

Tests: `cmdref_check_broken` runs the two commands with the committed
file of another library (WILL_FAIL); `cmdref_complete` (a CMake script)
checks that every committed JSON file belongs to a library of its
group, and `cmdref_complete_broken` runs it with a name list missing one
library (WILL_FAIL). The host tests of `tools/lib/cmdref` build library
images in memory, including broken ones.

### D4 Pages: one content adapter, templates as partials

`site/content/docs/libraries/_content.gotmpl` adds one page per
library: `commands/NAME` (acidlibs), `amigaos/NAME` (deflibs with an
`opens`, titled with the AmigaOS name; the calls of the libraries
without init whose base is this library are merged in), `userlibs/NAME`.
The sections have hand-written `_index.md` files that explain the
notation and list the libraries (shortcode `library-list`). The page
content is Markdown made by partials in `site/layouts/_partials/cmdref/`,
so the headings (one per command) go into the table of contents and the
search index. Argument types are shown in Blitz notation: `.b .w .l .q
.f $`, `any`, `.Type` (the command's suffix), `&` in front for the
address of a variable, `()` behind for an array; a repetition group as
`{ ... }...`. AmigaOS calls are one table per page (name with help text,
registers, offset).

Hand-written slot: `site/data/libintros/GROUP/NAME.yaml` with
`description` and `intro` (Markdown). The adapter uses them if the file
exists; otherwise a generated description.

### D5 Doxygen

A committed `Doxyfile` at the repository root (inputs relative to it).
`INPUT`: `libs/include`, `libs/deflibs/deflib.h`, `libs/acidlibs`,
`libs/userlibs` (all userlibs, a new directory included), `shared`;
`EXTRACT_STATIC = YES` (most entry
routines are static behind a thunk); `AUTOLINK_SUPPORT = NO`;
`ENABLE_PREPROCESSING = YES`, `MACRO_EXPANSION = YES`,
`EXPAND_ONLY_PREDEF = YES` with `PREDEFINED` empty definitions for
`BL_REG(r)`, `BLITZ_ROUTINE`, `BLITZ_ERRCHK`, `__saveds`, ... and the
table macros (`BLITZ_NAME(n,h)=`, ...), and `EXPAND_AS_DEFINED` for the
parameter-list macros. `WARN_AS_ERROR = FAIL_ON_WARNINGS`, `HAVE_DOT =
NO`, `OUTPUT_DIRECTORY = site/static/api`, `HTML_OUTPUT = .`,
`GENERATE_LATEX = NO`. The Pages workflow gets a job `api` on the
runner itself (not in the Hugo container: a container job cannot start
another container) that runs the official image on the checkout
(`docker run --rm -v $PWD:/src -w /src ghcr.io/doxygen/doxygen:Release_1_16_1`)
and passes `site/static/api/` as an artifact to the build job, which
puts it in place before Hugo runs. `.gitignore` excludes the tree.

## Risks / Trade-offs

- The JSON files are generated from the Amiga build, so the site cannot
  regenerate them; a stale commit is caught by the build, not by the
  site. That is the decision (committed data, checked).
- The help texts keep the original's typos and gaps; the pages show
  them as the editor shows them.

## Findings

- **Data** (2026-11-06): 175 command tables - 83 acidlibs, 39 deflibs,
  53 RWE userlibs; 605 statements, 506 functions, 147 commands, 18
  object names, 992 AmigaOS calls. 28 libraries declare no commands
  (the compiler's own libraries, numbers 64000 and up, and three helper
  libraries); their pages say so. 28 libraries manage objects. No help
  text has a byte outside ASCII; 45 have a `|` (none of an AmigaOS call).
- **`opens`**: the scan of the code for the library name finds the right
  name in all 37 deflibs libraries with an init (`gadgets/colorwheel.gadget`
  included); `doslib2` and `graphicslib2` have none and are merged into
  the pages of dos.library and graphics.library. The scan takes a
  printable run that ends in the name; if code bytes in front of a name
  were printable, the name would get a prefix - the committed files
  would show it, and the build check catches any later change.
- **Hugo**: the content adapter needs `description` as a top-level key
  of `AddPage` (in `params` it is not the page's description); partials
  that build Markdown return strings (`return`), so html/template does
  not escape them. Sorting entries by name is case-insensitive.
- **Doxygen** (1.18.0; the pinned image is 1.16.1, which gives the same result without the broken letter-index links of 1.17 and 1.18): with the configuration of D5, 731 warnings on
  the first run, then 311 with the parameter-list macros expanded and
  the thunk/assembler macros as declarations, 0 after the comment
  fixes. Global functions of one name in several files (`codeinit`,
  `codefinit`, `doinit`, `_libfinit`) are joined by Doxygen into one
  entity, across files, so a function's page would show the
  description of a namesake from another library. The sources keep
  such names unique (`docs/development.md`): of 56 names shared by
  acidlibs and the RWE userlibs (102 files), the 75 routines in C are
  `static` and the 193 entry points in assembler have the library's
  name as prefix; the library files are byte-identical either way (no
  symbols in them). The only functions Doxygen joins are a header's
  declaration with its definition. The init/finit routines whose docs
  clashed use `@arg`/`@par Result`. `SOURCE_BROWSER` is off (26 MB of HTML instead of 52 MB; the
  sources are on GitHub). `OUTPUT_DIRECTORY` is created only one level
  deep, so the workflow creates `site/static/api` first.
