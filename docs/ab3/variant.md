# AmiBlitz3 source compatibility: plan

> As of 2026-10-31 (revised the same day: the goal is source
> compatibility, not a library variant). A plan, nothing is built. Basis:
> the change lists of this directory ([compiler.md](compiler.md),
> [deflibs.md](deflibs.md), [acidlibs-a-l.md](acidlibs-a-l.md),
> [acidlibs-m-z.md](acidlibs-m-z.md), [userlibs.md](userlibs.md)),
> `docs/formats.md` §1.3 and §7 (`shared/xtra`), the loader of our
> blitz2 (`programs/blitz2/src/asmlibs.c`), AmiBlitz3's compiler source
> (`Sourcecodes/Amiblitz3/Compiler/`), `Docs/Amiblitz3.guide`, and
> measurements with `blitztokens` and a throw-away scanner over AB3's
> sources. Guesses are marked "(guess)". `@xxxxxxxx` =
> `AmiBlitz/AmiBlitz3@xxxxxxxx`.

## Goal and result

**Goal** (decided by the user): when blitz2 gets a source it recognises
as an AmiBlitz3 source, it compiles it with AB3's semantics: AB3's
compiler language and AB3's libraries (`baseLibs`, `csLibs`, `utilLibs`,
`OsLibs`, its userlibs and residents), **loaded as AB3's binary files**,
not ported to C in this step. TED and AB3's IDE stay out of scope. BB2
sources compile exactly as today.

**Result:**

* Feasible in stages. The library side needs little in the compiler
  (a loader for BigLibs without end mark, a flat userlibs directory,
  AB3's residents, AB3's own token table); the bulk is AB3's language.
* **Pin AmiBlitz3 v3.14.0** (`@58a11658`, 2026-10-19): the latest
  release; its libraries and compiler are identical to HEAD `@b1a9dee1`
  (only documentation changed since), and its example sources match its
  libraries.
* **Recommended form:** one source tree, a second program `blitz2-ab3`
  that has BB2 *and* AB3 mode and switches **per source** at run time.
  The default `blitz2` has no AB3 code; `blitz2-ab3` must give the same
  executables as `blitz2` for every BB2 source. (Built first with
  `-DBLITZ2_AB3`; since 2026-11-01 both programs link one core and
  differ by its extension points, section "Structure".)
* A static scan of AB3's 226 example programs (`Sourcecodes/Examples`,
  `Templates`) estimates what compiles after each phase: about **60 %**
  after the AB3 mode itself (text sources, AB3 libraries and tokens),
  **76 %** after directives, string escapes and optional parameters,
  **82 %** after FPU types, **96 %** after the 68020 assembler (upper
  bounds, guess; details in section 3).

## Measured facts

* **Tokens** (`blitztokens`; joined BigLibs, see 2 and the spike result):
  BB2 2.10 has 1611 library tokens; v3.14.0 has 4435 (239 libraries:
  baseLibs 33, csLibs 32, utilLibs 101, OsLibs 73). 225 of BB2's library
  tokens have another number in v3.14.0 or are gone (167 in v3.11.4, 125
  in AB2 = SVN r1 `@ea8c25c7`; 688 of r1's tokens moved by v3.14.0).
* **AB3's compiler token table** (`Amiblitz3` binary): 218 tokens against
  our 210. `SYNTAX` ($8014) and `EXTB` ($803B) take the two `Jimi`
  slots, `Chip` replaces `MOVEP` ($807F), `BSS` replaces `TAS` ($80AD),
  `RunErrsOn`/`RunErrsOff` are spelt so, new: `USELASTPATH`, `RegLoad`,
  `RegSave`, `Object`, `USEPATH2`, `LastRunErrsMode`, `KEEP`, `OPTIMIZE`
  ($80D4–$80DB).
* **Sources**: since `@9f5e7b8c` (SVN r444, 2014) AB3 keeps its sources
  as ASCII `.ab3` with the embedded header (formats.md §7); the
  repository has 221 `.ab3` examples, 570 `.ab3` includes, 8 templates,
  68 test programs (`Sourcecodes/Amiblitz3/Tests`), and still 156
  tokenised `.bb2`/`.bb` (AB2-era numbering).
* **Directives used** (examples, templates, includes): `OPTIMIZE` 191
  times – 7 (68020 + FPU + new syntax) 110, 5 (68020 + new syntax) 53,
  4 22, others 6; `SYNTAX` 159 times – 2 101, 6 48, 4 9, 1 once.
  Residents in the headers: `all.res` 130, `amigalibs.res` 27, a few
  others.
* AB3's own CLI mode: `Amiblitz3 -s SOURCE -e EXE [-r|-d] [-dump …]`
  (`AmiBlitz3.ab3` 548 ff.) – usable as the test reference.

## 1. Detecting an AB3 source and switching mode

### Detection

`shared/xtra` already classifies a source (`xtra_detect`): `BB22` at the
start → RWE's BB22; a `$00` byte → tokenised (TED); first line
`; XTRA` → AB3; other text → AB3 if the name ends in `.ab3`, else plain
text. Proposed rules for blitz2:

| Source | Mode |
|---|---|
| text with embedded header, or name `*.ab3` (`XTRA_T_AB3`) | AB3 |
| text without header, other name (`XTRA_T_TEXT`, e.g. `.bb`, `.ab2`) | an option; default open (decision) |
| tokenised with an AB3 `.xtra` (15-entry switch block, since `@bdae3f9e` 2026-01; formats.md §1.3) | AB3 |
| tokenised otherwise (`XTRA_T_TED`, `XTRA_T_BB22`) | BB2, unless the option says AB3 |

Tokenised AB3 sources are only reliable if written with the pinned
state's token numbers: an AB2-era `.bb2` (r1 numbering) read with
v3.14.0's list gives other commands. A token heuristic (tokens BB2 does
not have: compiler $80D4–$80DB, library numbers BB2 lacks) is good for a
*warning*, not for switching (guess: too many false decisions).

### Where the mode applies

blitz2 gets its source from TED (comdata, lines already tokenised with
BB2's tokens; architecture.md §3). With TED out of scope, AB3 mode is
reached through a **command-line compile**: `blitz2-ab3 -s SOURCE -e
EXE` modelled on AB3's CLI (same option names, so AB3 build scripts
work), and through the headless batch stub of the tests
(`tests/corpus/amiga/tedstub.c`). blitz2 then reads the file itself,
detects it, reads the embedded header (`xtra_load`, `XTRA_T_AB3`) for
options, residents and object maxima, and tokenises the text with AB3's
token list (AB3's rules, `LoadSource`/preprocessor in
`CLIModeSupport.ab3`; whether TED's core tokeniser can be shared via
`shared/` is to be checked, guess: mostly). Includes (`INCLUDE`,
`XINCLUDE`, extension fallback `.ab3`/`.ab2`/`.bb2`, search order:
include directory, source directory, executable directory) are read and
tokenised the same way. The mode is the main source's and holds for the
whole compile (one program); includes follow it (guess: AB3 does the
same). The library set is chosen per compiled program: it is reloaded
only when a compile has another mode than the one before (a CLI run
loads once anyway).

**Mixed includes** (decided 2026-10-31): an include in *text* form is
tokenised in the program's mode. A *tokenised* include of the other
mode (an AB3 program including a tokenised BB2 file, or the other way
round) is **rejected** with a clear error: its token numbers belong to
the other library set. Converting it by name (detokenise with its own
token list, tokenise with the program's) is added only when real
sources need it.

### One blitz2 with a run-time mode, or a separate build?

| | Run-time mode in the one `blitz2` | Separate `blitz2-ab3` build (AB3 only) | **`blitz2-ab3` with BB2 + AB3 mode (recommended)** |
|---|---|---|---|
| Default `blitz2` byte-identical | no (new code in it; acceptance only functional, deviation to be named in every commit) | yes | yes (`#if BLITZ2_AB3` off; since 2026-11-01 its results, section "Structure") |
| Per-source switch | yes | no | yes |
| BB2 behaviour of the AB3-capable binary | must be proven | – | must be proven: same executables as `blitz2` over corpus, bug-orig, CD sources |
| Library sets | BB2 and AB3 libraries have the same numbers: only one set loaded; a switch reloads (as menu item 8 "reload libs") | AB3 only | as the first column |
| TED session | must stay BB2 (TED has BB2's token list) | – | same |

Recommended: the third column. A later decision can make `blitz2-ab3`
the default once the tests show identical BB2 results.

## 2. Loading AB3's libraries, and what the compiler must provide

**Files.** AB3 loads `Blitz3:BlitzLibs/baseLibs`, `utilLibs`, `OsLibs`,
`CsLibs` (paths from tool types) and every file of the flat directory
`BlitzLibs/UserLibs` (no subdirectories, unlike BB2's
`userlibs/DIR/`); residents from `BlitzLibs/Residents`. In AB3 mode
blitz2-ab3:

* loads the four BigLibs with `loadlibgroup` in AB3's order (base,
  util, OS, cs). Each v3.14.0 file ends with the end mark (a long 0),
  and AB3's own loader (`load_biglibsfile`) stops at it as ours does
  (spike result below), so the loop needs no change; it is only called
  four times instead of twice;
* scans the flat userlibs directory, and replaces a loaded library of
  the same number by a userlib (`@7ab0350b`);
* loads residents from the header's `Residents` list (up to 8, AB3
  warns after 7) relative to the residents directory. The `.res` format
  is the same (formats.md §2.1); its macros carry AB3 token numbers,
  and its NEWTYPEs can use AB3's type `.d` (double, `newtype_double`) –
  the resident loader must know that type;
* uses AB3's compiler token table instead of BB2's (a second static
  token chain in `main.c`, chosen per mode; the libraries append to the
  chain of the mode).

**Number ranges.** AB3's tokens stay in BB2's scheme ($8000 + library ×
128 + index, libraries 1–255; system libraries by their 16-bit numbers),
so `fixlibs`/`findtoke` need no change (guess, to verify with the token
check below). BB2 and AB3 libraries share numbers, so never both sets
at once.

**What AB3's libraries expect from the compiler** (from compiler.md and
the acidlibs notes; the last column is the result of the spike of
phase 0, section "Spike result" below: *needed* = BB2-language programs
fail without it, *not needed* = they work without it, *unknown* = not
reached by the spike):

| Entry / convention | Called by AB3's compiler for | Without it | Spike |
|---|---|---|---|
| strings2lib `$C703` setfpumode | every program (`OPTIMIZE` 2) | printlib/vallib use the non-FPU paths | not needed (FFP `Print`/`Str$`/`Val`/`Sin`/`Sqr` correct); with G6 |
| strings1lib `$C602`/`$C603` | `StringBuffer` option of the header | default buffer | not needed (the init gets the buffer size from `makessize` as in BB2) |
| runerrlib `$C320` checkpointer, `$C31F` updatestack | runtime checks | fewer checks | not needed (runtime error checking on works with BB2's entries) |
| arrayslib `$C405`/`$C406` | `Dim KEEP` | `Dim KEEP` impossible | not needed for G1 (G5) |
| arrayslib `$C403`/`$C404` + list size in d4 | `Dim List` (BB2 has the same entries and `setles`) | – | not needed (BB2's `Dim List` works, also the dynamic `Dim List x(0)`) |
| staticslib `$C502`–`$C509` | temporaries of AB3's code generation | not called by BB2 code | not needed |
| lmullib/ldivlib `$CA02`/`$CB02` | 68020 mul/div (`OPTIMIZE` 1) | `$CA00`/`$CB00` as BB2 | not needed (G7) |
| exitslib `$C801` | exit handlers | – | not needed (not called; programs end normally) |
| memlib pools, 40-byte list header (fields from −32 unchanged) | internal to the libraries | nothing for the compiler | not needed (lists of newtypes and strings, 8000 items, `ClearList`) |
| debuglib `$D50A`, ComData `$EEE9`, dhandlerlib | AB3's debugger/PED | out of scope: AB3 mode compiles without the debugger | unknown (out of scope; AB3's debuglib ends at `$D50A`, BB2's at `$D50F`) |
| math library of `^` (`mathq`: BB2 library 192, AB3 library 44, token `$1601`, `AmiBlitz3.ab3` 14565) | every float power | `^` fails: "Library not Found" | **needed** |
| init conventions of `makeLibInits` (varslib, debuglib, strings1lib, exitslib, datalib, dhandlerlib; memlib then `alstat`/`setmaxs`) | every program | – | not needed (AB3's are BB2's) |
| compile-time plugins (`$12345678` in a sub, `AmiBlitz3.ab3` 9303) | – | – | not needed (no library of v3.14.0 has one) |
| residents: names relative to `BlitzLibs/Residents` | header `Residents` | BB2 needs a full path | **needed** (the `.res` itself loads unchanged) |

## 3. The language changes in our blitz2, by what sources need

**Test set.** A throw-away scanner (comments and strings separated,
includes followed transitively through `Sourcecodes/Includes` and the
source directory) looked for each feature in the 226 programs of
`Sourcecodes/Examples` and `Templates`, and in the 68 programs of
`Sourcecodes/Amiblitz3/Tests`. 9 respectively 3 have an include that
does not exist in the repository. A program counts as compiling after a
phase if all its features are in the phases so far. This is an **upper
bound** (guess): the scanner sees only the listed features, not
semantic differences (new syntax mode, type rules, code generation), and
counts 68020 code even inside `CNIF` blocks.

| Phase | Features | Examples (226) | Tests (68) | C effort (guess) |
|---|---|---|---|---|
| G1 AB3 mode | text sources and includes, embedded header, AB3 token table, AB3 binary libraries and residents (section 2) | 136 (60 %) | 44 (65 %) | 1500–2500 lines |
| G2 directives | `SYNTAX` (accepted; checks later), `OPTIMIZE` (bit 4 "new syntax": about 10 branch points in AB3's compiler, e.g. local labels, line joining `@@`, global `SHARED`, assignment shortcut), `RunErrsOn`/`Off`, `LastRunErrsMode` | 153 (68 %) | 54 (79 %) | 500–1000 |
| G3 strings | escapes `\n`, `\XX`, `\Rnn`, `\__THIS_FUNCTION`, `\__CODELINE`, `\__DATE__` …, `@"ABCD"` | 163 (72 %) | 54 | 300–500 |
| G4 procedures | optional parameters `@p` (missing = −1/""), up to 10 parameters, `FAST` | 172 (76 %) | 57 (84 %) | 800–1500 |
| G5 small | `USELASTPATH`, `Null`, casts `(.type)`, `Dim KEEP`, `Dim List a(0)`, constants in `Data` | 176 (78 %) | 60 (88 %) | 500–800 |
| G6 FPU | types `.d`/`.f` on the FPU, `RegLoad`/`RegSave`, `OPTIMIZE` 2 code generation, `setfpumode` | 186 (82 %) | 62 (91 %) | 2000–3000 |
| G7 68020 assembler | `MULU.L`/`DIVU.L`, `EXTB`, scaled index `(a0,d0*4)`, other 020 modes; `OPTIMIZE` 1 code | 217 (96 %) | 65 (96 %) | 800–1500 |
| G8 checks and code equality | `SYNTAX` 1/2/4/6 type checks (only reject bad code), function optimizer, `HUNK_SYMBOL` names, A6 handling, jump/`moveq` optimizations | – (needed for AB3's error behaviour and for byte-equal executables) | – | 2000–4000 |

Most frequent AB3-only names in the examples (needing AB3's libraries
in G1): `BlitzMsgPort`, `DoMethodA`, the jpeg and icon V44 calls,
`BestCModeIDTagList_`, `FLen`, MUI, wizard (`WZ…`) and tag-list
commands. In all, about 6000–12000 lines of C in `programs/blitz2/src`
(guess), taken from ~240 non-cosmetic AB3 compiler commits and the
pre-2009 AB2 changes, which have no history. Every addition is an
extension point of the core (section "Structure") and active only in
AB3 mode.

## 4. Phases, tests, license

### Phases

0. **Spike** (tests only, no compiler change): AB3 v3.14.0's BigLibs
   with an end mark appended, joined in the build directory into an
   `acidlibs`/`deflibs` pair; a few BB2-language AB3 programs tokenised
   on the host (`xtra convert`, `tedtok --blitz2 … --lib …` with AB3's
   lists), compiled by today's blitz2 through the batch stub. It shows
   which library conventions break without AB3's compiler (table of
   section 2). A few days.
1. **Build and mode:** `blitz2-ab3` target, CLI front end, detection,
   header, tokeniser, loader, residents, token table (G1). **Done
   2026-11-01** (OpenSpec change `ab3-mode-g1`): `blitz2-ab3` + `ab3cli`,
   tests `ab3_bb2_equivalence(_runerr)`, `corpus_cd_ab3`, `ab3_tokens`,
   `ab3_cli` (26 cases), `ab3_cli_ref` (17 cases against AmiBlitz3's own
   compiler). Of AmiBlitz3's 68 test programs the suite holds the 10
   BB2-language ones of the spike, all compile and run; the share over
   all 68 (estimate for G1: 65 %) is not measured yet. On the way: #34
   fixed (AmiBlitz3's `all.res` relies on it).
2. **G2–G5** in that order (most programs per line of code). **G2
   done 2026-11-01** (OpenSpec change `ab3-mode-g2`): AB3 mode's
   tokeniser (`shared/tokenizer`), AmiBlitz3's include rules,
   `OPTIMIZE`, `SYNTAX`, `RunErrsOff`/`LastRunErrsMode`, `BSS`/`Chip`,
   the new syntax, `@@`, global `SHARED`, `_` names, `.name` labels, the
   `+` prefix, `:@`, `#__include`/`#__debug`/`CNIF @`; `ab3_cli` and
   `ab3_cli_ref` with ten more of AmiBlitz3's test programs. Measured
   share of the 68 test programs: section "Measured share". After G2
   (OpenSpec change `ab3-function-statement`, 2026-11-01): a `Function`
   called as a statement compiles as in AmiBlitz3 (`jsr`).
3. **G6, G7** (FPU and 68020; needs an A1200/FPU machine in the
   emulator for the tests).
4. **G8** only if AB3's error behaviour or byte-equal executables are
   wanted. Taken over before it (OpenSpec change `ab3-pointer-assign`,
   2026-11-01, spec `ab3-compat/type-rules`): the rule AmiBlitz3 applies
   without `SYNTAX`, a pointer takes no basic value other than long
   (`AmiBlitz3.ab3` 23531-23572; AmiBlitz3 types the right side by the
   last variable or function outside parameters). Still open for G8:
   NewType left sides, the `SYNTAX 4` part of the check (NewType against
   NewType, casts) and every other `SYNTAX` check.

Rough total: G1–G5 about 20–40 commits, G6–G7 about 15–25, G8 open
(guess).

### Tests

* **Default unchanged:** acceptance as always (`blitz2` byte-identical);
  in addition every BB2 test (corpus, bug-orig, CD sources) with
  `blitz2-ab3` must give the same executables as `blitz2`.
* **Host:** detection and header (existing `xtra` tests extended), the
  tokeniser (inputs from memory, also broken ones), `loadlibgroup`
  without end mark (with and without, truncated input).
* **Token check:** the AB3-mode token list of `blitz2-ab3` with AB3's
  BigLibs equals that of AB3's own `Amiblitz3` with them (both through
  `blitztokens`; a broken input must fail); AB3's `-dump token` in
  Amiberry as a second source.
* **Reference compiler:** AB3 v3.14.0's `Amiblitz3` in Amiberry in CLI
  mode (`-s … -e …`, OS 3.x, A1200 configuration) compiles the test set;
  per program the result is compared with `blitz2-ab3`'s: compile
  error or not, the program's output (functional equality); byte-equal
  executables only with G8. The configuration pattern is that of the
  corpus (ref/port), with AB3's files in `Blitz3:` in the emulator.
* **Test set:** `Sourcecodes/Amiblitz3/Tests` first (small, mostly
  non-interactive), then the examples that end on their own; interactive
  ones like `corpus_req`. The share that compiles is reported per phase
  (as in section 3, but measured).

### License

* We **load** AB3's binaries; we do not port or distribute them. A user
  points blitz2-ab3 at an existing AB3 installation (`Blitz3:`). For the
  tests, `cmake/fetch-originals.cmake` fetches the BigLibs, residents,
  userlibs, the `Amiblitz3` binary and the test sources at the pinned
  commit with SHA-256 (`archive/amiblitz3/…`, not in git), as for the
  CD material. That includes third-party libraries inside AB3's BigLibs
  under their own, often unstated licenses: they are only downloaded as
  AB3 publishes them, never in git or in our results.
* The compiler changes are **our C reimplementation** of AB3's language
  from reading AB3's compiler (Apache-2.0 per `LICENSE.txt`, added by
  `@2c5e1fdb` 2025-08-11 after the SourceForge page; GPL-2-or-later per
  `Docs/Amiblitz3.guide`): both compatible with GPL-3.0-or-later; keep
  attribution with `@hash` references in code and notes, and a note in
  `archive/README.md`. Header reading and tokeniser are own code after a
  description, as `shared/xtra` already is.
* Porting AB3's libraries to C is a later, separate step; then the
  license questions of the third-party parts apply (McMinn's gtlib,
  Elmore/Lotan/wzlib commands, ZX0 under zlib, …).

## Decisions (2026-10-31)

1. Pinned state: AmiBlitz3 v3.14.0 (`58a11658`).
2. Form: `blitz2-ab3` with BB2 and AB3 mode beside an unchanged
   `blitz2`.
3. Detection: AB3 by the `; XTRA` header, the name `*.ab3`, an AB3
   `.xtra` of a tokenised source, or an option; anything else is BB2.
4. Entry point: AB3-compatible CLI options (`-s`, `-e`, `-r`, `-d`) for
   `blitz2-ab3`; TED stays BB2-only.
5. Target: functional equality (same compile result and output,
   G1–G7); byte-equal executables (G8) only if needed.
6. No runtime debugger in AB3 mode for now.
7. Mixed includes: a tokenised include of the other mode is rejected;
   conversion by name only when needed (§1).
8. Order: spike, G1–G5, then G7 (68020), then G6 (FPU).

## The command line (G1, 2026-11-01)

```
blitz2-ab3 -s SOURCE -e EXE [-r] [-mode ab3|bb2]
```

The option names are those of AmiBlitz3's CLI mode (its help text in
v3.14.0's compiler); `-s`/`-source`, `-e`/`-exe`, `-r`/`-release` and
`-h`/`-help` are supported, `-mode` is this repository's. AmiBlitz3's
other options (`-d`/`-debug`, `-run`, `-dump`, `-setcom`, `-setdbg`,
`-strbufsize`, `-stacksize`, `-maxerrors`, `-1`/`-1pass`, `-dform`,
`-PED`, `-save`, `-v`/`-verbose`) are refused. Without any argument
starting with `-`, `blitz2-ab3` starts the editor exactly as `blitz2`
(BB2 mode). Return codes: 0 compiled (or `-h`), 10 compile error, 20
argument or set-up error (assumption, AmiBlitz3's own are not
documented).

How it works (`shared/ab3job`, `programs/blitz2/src/ab3main.c` and
`ab3inc.c`, `programs/ab3cli`): `blitz2-ab3` parses the arguments,
reads the source and its companion `.xtra` and decides the mode (`xtra_mode`, §1) before
it loads anything; then it runs as for the editor, but loads
`PROGDIR:ab3cli` instead of `blitz2:ted` and hands it the job instead
of a file name. `ab3cli` is a headless editor on the edstub frame with
TED's core: it loads the source as TED does (a text source tokenised
with the token list of the program's mode; in AB3 mode, since G2, with
AmiBlitz3's tokeniser, below), hands the compiler the
options (AmiBlitz3's embedded header as a Blitz 2 `.xtra` image, else
the companion `.xtra`, else the compiler's own; `-r` switches runtime
error checking, the debugger and debug information off), compiles with
CREATE EXECUTABLE and prints errors as `FILE:LINE: MESSAGE`.

Includes: blitz2 only reads tokenised includes (`readinc`, design D4 of
the change). In a command line run an include of text is therefore
tokenised by `ab3cli` first – the mode's way of loading a source, with
the program's token list – and `readinc` reads those lines; an editor
session reads includes exactly as `blitz2`. Whether an include is text:
in BB2 mode by its contents, in AB3 mode since G2 by its name (below,
"Includes of AB3 mode"). A tokenised include of the
other mode is refused ("Include of another mode: NAME"). A compile error
inside an include is printed with the include's name and line
(`INC:LINE: MESSAGE`): `blitz2-ab3` counts the lines `readinc` takes of
every open include and places the first error of the compile at the
innermost include that has a line read (an error of opening an include,
or of its mode, at the `INCLUDE` line outside); `ab3cli` prints that
place (job fields `errline`/`errfile`).

Limits of G1, found on the way:
* Relative names (source, executable, includes, residents with a path)
  are relative to the shell's current directory.
* The `Max NAME` lines of the embedded header are not used (a Blitz 2
  `.xtra` names the library by number; mapping needs the libraries):
  the compiler's maximums apply.
* A text line longer than TED's 128 characters is split as TED splits
  it; names that contain a command name are tokenised inside (`show2{`
  becomes `Show` + `2{`), as in TED and AmiBlitz3's tokenised sources.
  (BB2 mode only since G2: in AB3 mode the text is tokenised by
  AmiBlitz3's rules, below.)
* `PROGDIR:` needs Kickstart 2.0; the editor session runs on 1.3 as
  `blitz2`. The `.xtra` image goes to the compiler through a temporary
  file in `T:` (else `RAM:`).
* The line of an error in an include of text counts the lines as TED
  loads them (BB2 mode): after a line TED splits (longer than 128 characters) the
  number is one too high. Of more than 16 nested includes, an error in a
  deeper one is placed at the 16th.

Tests (`tests/ab3/CMakeLists.txt`, label `emulator`, skipped without
Amiberry or the fetched AmiBlitz3 files): `ab3_cli` compiles the cases
of `tests/ab3/cli/` with `blitz2-ab3 -s … -e …` in one session
(`tests/ab3/cli.cmake`: `blitz2:`, three `Blitz3:` trees - the full
one, one without `csLibs`, one with a userlib of number 152 replacing
`baseLibs`' freqlib), runs each program through `reqtrap` and compares
the compiler's console output, its return code, the program's output and
return code with `NAME.expect` (`tools/outmatch`: literal lines, `~`/`+`
patterns for memory sizes, timing and the system's language). The cases:
AmiBlitz3's ten test programs of the spike (`memoryleak_test` with its
`OPTIMIZE 13` line since G2) and the ten G2 makes compile (below), `^`
(library 44), header residents with and without a
path, includes of text in both modes, the userlib, and the errors - a
missing BigLib, a missing resident, tokenised includes of the other mode
(both ways), `-d`, a syntax error, errors in an include of text and in a
tokenised one. `ab3_cli_ref` compiles the AB3 cases with AmiBlitz3
v3.14.0's own compiler (`Blitz3:Amiblitz3 -s … -e …`, CLI mode, the
fetched binary; no further setup) and compares the programs' outputs with
the same expect files, or with `NAME.ref.expect` where AmiBlitz3 differs
(below). `ab3_cli_broken`/`ab3_cli_ref_broken`: a wrong expect file must
fail.

## Structure: the core and its extension points (2026-11-01)

`blitz2` and `blitz2-ab3` are two programs built from one core (change
`blitz2-ab3-extension-points`): the chunk files of `programs/blitz2/src`
(the translation of blitz2.s: `main.c`, `compile.c`, `stmts.c`,
`asmlibs.c`, `parse.c`, `eval.c`, `libcall.c`, `tokens1.c`,
`tokens2.c`, `data.c`, each with its header, `blitz2.h`, `structs.h`,
the token list `toklist.h` with `tokimg.h`) and the glue (`glue.c`,
`glue_direct.c`, `glue.h`) are compiled **once**, without any define,
as the CMake object library `blitz2_core`, and both programs link it. Where AB3 mode
differs, the chunk code calls an **extension point**: a plain function
(or reads an object) declared in `ext.h`. Each program links one
implementation:

| Program | Implementation of `ext.h` | |
|---|---|---|
| `blitz2` | `ext_bb2.c` | BB2 only: every point handles nothing, the data are BB2's (no logic) |
| `blitz2-ab3` | `ab3main.c` | the command line, the editor (`ab3cli`) and the return code of a command line run |
| | `ab3libs.c` | AmiBlitz3's BigLibs and flat userlibs directory |
| | `ab3res.c` | AmiBlitz3's residents directory, the set-up errors of a compile |
| | `ab3inc.c` | the mode of a tokenised include, includes of text, the place of an error |
| | `ab3tok.c` | the mode (`progmode`), AmiBlitz3's token list, the library of `^`, its tokens not compiled yet |
| | `ab3syn.c` | AmiBlitz3's syntax rules (new syntax, `@@`, global `SHARED`, ...) |
| | `ab3sect.c` | the sections `BSS` and `Chip`: their hunks in `savefile` |
| | `ab3dir.c` | AmiBlitz3's directives (`OPTIMIZE`, `SYNTAX`, `RunErrsOff`/`LastRunErrsMode`; change `ab3-mode-g2`) |
| | `ab3.h` | the state the AB3 modules share (not for the chunk files) |
| | `shared/xtra`, `shared/ab3job` | the mode of a source, the arguments and the job of a command line run |

The points (`ext.h`): `ext_makelibs`, `ext_loadlibs`,
`ext_loaduserlibs`, `ext_freelibs` (libraries), `ext_resbegin`,
`ext_loadres` (residents), `ext_compilestart` (the start of a compile),
`ext_incname`, `ext_xinclude`, `ext_incopen`, `ext_readinc`,
`ext_incline`, `ext_incdone` (includes),
`ext_errpos` (a compile error), `ext_newtoken` (a built-in token),
`ext_passstart`, `ext_passend` (a pass), `ext_token` (a statement
token of AB3 mode), `ext_newsyntax` (AmiBlitz3's new syntax),
`ext_macargs`, `ext_namelabel`, `ext_dotlabel`, `ext_stmtprefix`,
`ext_comment`, `ext_letter`, `ext_shared`, `ext_sharedvar`,
`ext_nextline`, `ext_cnifstart`, `ext_cnifend`, `ext_constant`
(AmiBlitz3's syntax rules), `ext_funcstmt` (a `Function` called as a
statement, change `ab3-function-statement`), `ext_savefile` (the
executable),
`ext_cmdline`, `ext_editor`, `ext_editorarg`, `ext_result` (command
line and editor); the data of the mode `ext_toklist` (asm `tokens`) and
`ext_mathq` (asm `mathq`). A point that stands for a part of a chunk
function returns whether it handled the case; the BB2 code stays in the
chunk file (`if (ext_loadres(res)) … else` blitz2's way), so an
implementation that handles nothing gives blitz2's results.

**Rule for G2 and later:** new AB3 behaviour is added behind an
extension point – declared in `ext.h`, its BB2 answer in `ext_bb2.c`
(nothing handled), its AB3 code in an `ab3*.c` module – not in the
chunk files; a chunk file does not ask for the mode (`progmode` is AB3
state), it calls a point named after the behaviour (e.g.
`if (ext_newsyntax()) …` for a parser branch). The chunk files keep
every BB2 behaviour and stay translations of blitz2.s. No source of
`programs/blitz2/src` may contain `BLITZ2_AB3`, the define of the
earlier second build: the link of both programs checks it
(`programs/blitz2/checkcore.cmake`; ctest `blitz2_checkcore_broken`).
`blitz2` itself may change with such a rework, its results may not
(CLAUDE.md, rule of 2026-11-01): the executables it writes for the
corpus, the bug-orig tests and the CD's sources stay byte-identical.

## AB3 mode's tokeniser (G2, 2026-11-01)

In AB3 mode `ab3cli` tokenises a text main source and the includes of
text with `shared/tokenizer` (change `ab3-mode-g2`, design D2/D3), which
follows AmiBlitz3 v3.14.0's `preproc_Tokenize`
(`Shared/preprocessor.include.ab3:358` at `@58a11658`), with the token
list blitz2-ab3 hands its editor (its own tokens, then the libraries'; a
name given twice keeps its first token):

* names match byte for byte: `execbase`, `nprint`, `if` stay names;
* only a whole word that starts with a letter is looked up, and only if
  it starts at the line start or after a space, an operator, a bracket,
  `:`, `,`, `;`, `"` or a control character - never directly after `.`,
  `\`, `#`, `$`, `'`, `@`, `_`, a digit or a letter (`show2{`, `sub1`,
  `\Width`, `#MAX`, `.ExecBase` stay as they are);
* at a `$` `name$` is looked up first, then `name`; a word is looked up
  when a character that is not a letter, digit, `_` (or a `$`/`#` that
  belongs to no token) ends it - a word at the very end of the file
  (no line end behind it) is not looked up, as in AmiBlitz3;
* strings and comments are copied unchanged, except a TAB (a space);
  outside them a TAB becomes a space, CR, DEL and the other control
  characters are dropped, a byte from 128 becomes `?`; LF and a 0 byte
  end a line;
* the main source: its embedded header (to the line that reads `/XTRA`,
  AmiBlitz3's `ParseAB3Header`; not for `.ab2`) is skipped - here as
  empty lines, so that the line numbers of errors stay the file's - and
  a line longer than 250 characters is cut there (`LoadSource`; in
  AmiBlitz3 such a line also ends the source, below);
* an include of text: its header (AmiBlitz3's `SkipXtraHeader`: leading
  lines with a `;` up to a `/XTRA`, `/xtra` or `/Xtra`) skipped, as empty
  lines; no line cut.

A tokenised source and BB2 mode keep TED's way of loading (TED's own
tokeniser stays for TED, a later change).

`ab3_tokenise` (label `emulator`, `tests/ab3/tokenise.cmake`) checks the
tokens: AmiBlitz3 v3.14.0 loads each source in CLI mode and writes its
lines (`-save NAME.bb2`), `tools/ab3tok` loads it with `shared/tokenizer`
and AB3 mode's token list, and the lines must be equal - for AmiBlitz3's
test programs, the `.ab3` sources of `tests/ab3/cli/` and the edge cases
of `tests/ab3/tokens/`. `ab3_tokenise_broken`: with another token for
`NPrint` it must fail.

## Includes of AB3 mode (G2, 2026-11-01)

In AB3 mode an `INCLUDE`/`XINCLUDE` is found and judged as AmiBlitz3
v3.14.0 does it (change `ab3-mode-g2`, design D4; `ab3inc.c` behind
`ext_incname` and `ext_xinclude`, the name rules in `shared/tokenizer`;
`AmiBlitz3.ab3` at `@58a11658`):

* the name is the `INCDIR` string followed by the name (`makeiname`, as
  BB2), searched (`check_incdir`, 25836) in `Blitz3:Sourcecodes/Includes`
  (AmiBlitz3's default includes directory, `config\path_includes`), the
  main source's directory, the directory of the innermost open include,
  and `Blitz3:Sourcecodes/Includes` with the name's file part - joined
  with `AddPart`, so a name with a device is taken as it is;
* at each place the name as given, then its extension replaced by
  `.ab3`, `.ab2`, `.bb2`, then these behind the old extension
  (`ab3_GetIncludeLock`, 25796: `mylib` → `mylib.ab3`, `dos.include` →
  `dos.include.ab3`); the found file's full name (`NameFromLock`, e.g.
  `TEST:src/lib.ab3`) is the include's name from then on, also in
  messages;
* by the last three characters of that name, case-sensitively
  (`processInclude`, 20754): `ab3`, `AB3`, `ab2` are tokenised as text
  (by `ab3cli`, AmiBlitz3's tokeniser), `bb2` and `.bb` are read as
  tokenised - a text file of such a name is one comment "line" -, any
  other name stops the compile with "You can only include tokenized
  files (.bb2 |.bb)" (also when no such file is there; a name like
  `defs.h` is refused unless `defs.ab3` or another fallback exists);
  a tokenised include still gets the check of its mode (its companion
  `.xtra`, G1);
* `XINCLUDE` skips a file whose file part was included with `XINCLUDE`
  before (`do_xinclude`, 20615): `XINCLUDE "lib"` after `XINCLUDE
  "lib.ab3"` is skipped; `INCLUDE` of a file that is open (an include of
  itself) is "Already Included", one after the other is compiled each
  time - as in AmiBlitz3 (`findinclude` looks at the open includes) and
  BB2.

The include depth (the includes open now: 0 in the main source) is
`ab3_incdepth()` of `ab3.h`. AmiBlitz3's constants of the include
mechanism (`ab3syn.c`, points `ext_constant` in fetchconst and
`ext_cnifstart`/`ext_cnifend` in docnif): `#__include` is 1 while an
include is open, 0 in the main source (also after the last include);
`#__debug` is the core's, as in BB2 mode (README, "Extensions of
blitz2", change `debug-constant`): −1 with runtime error checking, 0
without, AmiBlitz3's `debugmode` (`AmiBlitz3.ab3` 3422-3443: added to
the constant list before the source, so a defined-test finds it and a
source's own `#__debug=…` replaces it; a resident's, as `all.res` has
one, gets the value at the start of every compile). Cases
`debugconst_on`, `debugconst_off`, `debugconst_r` (`-r`), `debugconst_res`,
`debugconst_res_r` (`all.res`); in `ab3_cli_ref` `debugconst_off` as ours,
`debugconst_on` and `debugconst_res` with AmiBlitz3's 0 ("Differences"). An `@`
right behind `CNIF` makes every `#name` of that
condition a "defined?" test giving 1 or 0, without "Constant Not Found"
(`CNIF @#mylib_include=0`, the include guard of AmiBlitz3's includes);
an `@` elsewhere in a condition is "Illegal Constant Expression", as in
AmiBlitz3 (`AmiBlitz3.ab3` 10677-10691, 15400-15412). Cases `incguard`,
`cnifdef`, `cnifat_err`, also in `ab3_cli_ref`.

`ab3_cli` cases: `incfallback` (the fallback), `increfuse` (a refused
name), `inctok_ab3` (a `.bb2` with AmiBlitz3's `.xtra` read as
tokenised), `incxtwice`, `incitwice` (repeated includes),
`inctext_bbname` (a text `.bb` read as tokenised), all also in
`ab3_cli_ref` with AmiBlitz3's results.

## Differences to AmiBlitz3's compiler (G2+, 2026-11-01)

Found by `ab3_cli_ref` (AmiBlitz3 v3.14.0 in CLI mode on the same
machine); recorded, not changed in G1:

| Case | AmiBlitz3 v3.14.0 | blitz2-ab3 | For |
|---|---|---|---|
| `exec_data` | return code 0 after a compile error ("A unknow error happend!") | 10 (design D6) | none: ours is the documented convention |
| text include without a final line end | its last line is lost: a last `NPrint` is not compiled, a last `INCLUDE` never done (measured 2026-11-01; guess: `read_includeline`'s scan stops before the 0 at the data end) | the last line is compiled | none: kept (silently losing code is not reproduced; user decision 2026-11-01) |
| `opt_fpu` | `OPTIMIZE 7` compiles on the reference machine (Amiberry's A1200 has a 68882, `fpu_model` of `default.uae`: `_checkfpu` finds it) and the program runs | refused: "Not supported: OPTIMIZE 2 (FPU)" (spec `ab3-compat/directives`) | G6: FPU code |
| `runerrs` | in CLI mode without its debugger (`Defaultdbug`, not fetched) the executable has no runtime error checking: the array index out of range at the end is not seen | checked: blitz2's runtime error requester | none: the runtime part of AB3 mode |
| `sect_both`, `reloc_chip`, `reloc_chip_bss`, `reloc_bss_chip` | the relocations inside the chip section (`Dc.l` to a BSS, Chip or code label) are written to the code hunk's RELOC32 lists with wrong offsets (`sect_both`: 554, `AmiBlitz3.ab3` 10060-10130 at `@58a11658`: one RELOC32 table, always for hunk 0; AmiBlitz3 issue #128, fix in PR #130, not merged on 2026-11-03): the longs stay wrong (0), and `LoadSeg` adds the hunks' addresses to instructions of the runtime's start code (`sect_both`: `movea.l -32744(a5),a1`). The program then runs code that depends on where the hunks land: its output can still be right, but it may damage memory (measured 2026-11-03: one byte of `ramlib`'s `DOSBase` in ExecBase's `ex_RamLibPrivate`, so `ramlib` crashes with 80000004 at the next library it loads from disk, the start of the next AmiBlitz3). `ab3_cli_ref` keeps these executables but does not run them (`REF_NORUN`); the cases' expect files, the output of a correct compiler, are derived from the sources (AmiBlitz3's output format from its run of `sect_both`), not from a run of AmiBlitz3 | relocated in the chip hunk's own RELOC32 block | none: ours is the correct executable |
| `tokens/longline` | a main-source line of 250 characters or more ends the source: `LoadSource` re-reads it with `PeekT(…, 10)`, which goes past the line's 0 to the end of the buffer (no LF is left after tokenising), so the following lines are dropped (`CLIModeSupport.ab3` 480-497 at `@58a11658`, measured with `-save`) | the line is cut at 250 characters, the following lines stay | none: not reproduced |
| `int_tests` | `Stop` without its debugger (CLI mode, no `Defaultdbug`) is compiled to nothing (`#stopignore`, `do_Stop`, `AmiBlitz3.ab3` 22810-22831 at `@58a11658`): the program runs on | blitz2's `Stop`: with `-r` "Debugger must be enabled for STOP" (line 92), with runtime error checking a call of debuglib's `Stop` (`$D501`), which waits for the debugger - the program hangs in a command line run. The rest of the program (`int_tests_nostop`, the `Stop` line empty) gives AmiBlitz3's output | none: BB2's `Stop` is kept in both modes (user decision 2026-11-01, change `debug-constant`); debug-only code goes into `CNIF #__debug … CEND` |
| `noend` | a program without `End` with runtime error checking ends normally | an empty requester at the end: blitz2's end code (`blitz2.s:2683 eopcode`, `moveq #3,d0 : trap #0`) without `End`, BUG(orig) #6 | with #6 (the runtime part of AB3 mode) |
| `debugconst_on`, `debugconst_res`, `cnifdef` without `-r` | in CLI mode without its debugger (`Defaultdbug`, not fetched: "Debugging is not possible") `#__debug` is 0, also with `RuntimeDebug = 1` in the header (measured; with the debugger it would be −1, `AmiBlitz3.ab3` 3422-3443 and `do_runerrson`) | runtime error checking as the options say (the header, else the compiler's own, on): −1 (`cnifdef` therefore runs with `-r`, `debugconst_on` and `debugconst_res` have AmiBlitz3's 0 as `.ref.expect`) | none: the runtime part of AB3 mode, as `runerrs` |

AmiBlitz3's test programs of G2 (task 5.1 of `ab3-mode-g2`, fetched
at `@58a11658`), in `ab3_cli` and `ab3_cli_ref`: `hex_error`
(`OPTIMIZE 1`), `hexdigits` (`_` names), `int_problem`, `int_tests` /
`int_tests_nostop` (`OPTIMIZE 4`, `SYNTAX 2`), `macro_test` (`@@`),
`kick13` (`OPTIMIZE 4`, `SYNTAX 2`; it ends after its first `NPrint`, so it
runs headless), `windowtags` (`@@` in a tag list; opens its window on the
Workbench screen and ends; compiled with `-r`: it has no `End`, case
`noend`), `memoryleak_test` (`OPTIMIZE 13`) and, since the change
`ab3-function-statement`, `jsr` (a `Function` called as a statement)
give AmiBlitz3's output; `unknown_function` (an include named without
extension, `.ab3` added) is "Procedure not found" in both; `exec_data`
is "Can't convert types" in both (since `ab3-pointer-assign`);
`int_tests` differs (table above).

AmiBlitz3's console output in CLI mode also names files it does not
need for compiling (`Blitz3:System/token_mapping_libs.txt`,
`token_mapping_userlibs.txt`, the debugger `Defaultdbug`); they are not
fetched.

AmiBlitz3 adds `Blitz3:Libs` to the multiple assign `LIBS:` when it
starts (`AmiBlitz3.ab3` 344-346 at `@58a11658`) and removes it when it
ends. `dos_AddAssign` (`Sourcecodes/Includes/dos.include.ab3` 1066-1075)
does not check the lock of the directory: without `Blitz3:Libs` it calls
`AssignAdd_` with lock 0, which succeeds, so `LIBS:` gets an entry
without a directory. The removal at the end then fails ("Could not
remove assign: Blitz3:Libs"), and every compile leaves one more such
entry. `ab3_cli_ref` therefore has an empty `Libs/` in each Blitz3 tree
(`tests/ab3/cli.cmake`). `blitz2-ab3` makes no assigns.

## Measured share of AmiBlitz3's test programs (G2, 2026-11-01)

A throw-away run (not in the repository): every one of the 68 test
programs of `Sourcecodes/Amiblitz3/Tests` at `@58a11658` (67 `.ab3`, the
includes among them, and the tokenised `68k_crash_newtype`) compiled
once by `blitz2-ab3` (state of task 5.1 of `ab3-mode-g2`, the options of
each program's header) and once by AmiBlitz3 v3.14.0 in CLI mode, in
Amiberry's A1200 (`default.uae`, 68020 + 68882), with AmiBlitz3's
`Sourcecodes/Includes` and `BlitzLibs/Residents` of the same commit;
compile only, the programs are not run.

* **blitz2-ab3 compiles 47 of 68 (69 %)**; AmiBlitz3 itself compiles 57
  (84 %). Both give the same result (compiled or not) for 58: 47
  compile in both, 11 fail in both. (The run measured 47 at the end of
  G2, with `exec_data` compiled by blitz2-ab3 only and `jsr` by
  AmiBlitz3 only; since `ab3-pointer-assign` `exec_data` fails in both,
  since `ab3-function-statement` `jsr` compiles in both - each checked by
  `ab3_cli`/`ab3_cli_ref`, the whole run not repeated.)
* Compiled by blitz2-ab3 only: `exec_data` (AmiBlitz3's type rule of the
  assignment). Since `ab3-pointer-assign` (2026-11-01) it fails in both
  (`ab3_cli`, `ab3_cli_ref`).
* Not compiled by blitz2-ab3, by AmiBlitz3 (10): `benchmark2`,
  `putdiskobject`, `quickdivtest`, `serial_read`, `statementcalls`
  ("Can't Convert Types" in `error.include.ab3` 226: its optional
  parameters `@titletext.s`, G4), `function_problem` (`@p`, G4),
  `benchmark` (`FAST` in `convert.include.ab3`, G4), `data_constants`
  (`Data.w #c`, G5), `double_print`, `fpu_test2` (`OPTIMIZE 2`, G6).
* Not compiled by either (10): `ab3_Errors`, `dimcrash`,
  `errorInIncludes` (errors on purpose), `include_dc`, `include_dc_main`,
  `includes_test` (`.h` includes), `_new_includes` (includes not in the
  repository), `printlib_test`, `unknown_function` (on purpose),
  `68k_crash_newtype` - AmiBlitz3 stops with "Garbage at the end of
  line" in line 1, **blitz2-ab3 hangs while compiling it** (open).
* `int_tests` compiles with runtime error checking (its `Stop` hangs at
  run time, "Differences"), `windowtags`, `setwindowpointer`, `hunktest`
  compile; the research table (G2 research §9) expected 52 after G2 -
  `jsr` was counted for G2 wrongly (it compiles since the change
  `ab3-function-statement`), the rest of the difference are its
  upper bounds.

## Spike result (2026-10-31)

Phase 0, no compiler change: today's blitz2 (build of `main`) with
AmiBlitz3 v3.14.0's BigLibs, A1200 in Amiberry, through the corpus
runner (`corpusrun`, tedstub, reqtrap; a throw-away pass file, nothing
of AB3 in the repository).

**Libraries.** `baseLibs`, `utilLibs`, `OsLibs`, `csLibs` (58 + 98 + 71
+ 30 = 257 libraries) **do end with the end mark** (a long 0), and AB3's
`load_biglibsfile` stops at it too; the plan's "no end mark" was wrong.
Joined in AB3's load order as `acidlibs` = base + util and `deflibs` =
OS + cs, the end mark of the first file of each pair dropped. blitz2
loads them unchanged. `biglib check`: no library number twice.
`blitzlibcheck` (each library taken out): no errors, 51 warnings in 15
libraries, all "code refers into the error check area, dangling without
debugger" (the class the CD's originals had). Token list
(`blitztokens`): 4645 = 210 compiler + 4435 library tokens; 10 names
twice (`STOP` as 68000 mnemonic and library `$ED12`; `fadd`, `fsub`,
`fmul`, `fdiv`, `fneg`, `MakeDir`, `DBactive`, `PeekMax$` in two
libraries each) – AB3's own state, not a break. Every internal entry
blitz2 calls (the 81 tokens `$C000`–`$D404` of `main.h`, and
`$BF00`) is in the AB3 library of the same number at the same token;
AB3 only appended entries
(memlib to `$C00C`, runerrlib to `$C320`, arrayslib to `$C406`,
staticslib to `$C509`, strings1lib to `$C603`, strings2lib to `$C704`,
exitslib `$C801`, lmullib/ldivlib `$CA02`/`$CB02`, wbstartuplib
`$BF01`). AB3's `makeLibInits` passes the same values to the same
libraries as BB2's.

**Programs.** From `Sourcecodes/Amiblitz3/Tests` (embedded header cut
off, tokenised with `tedtok --blitz2 … --lib` on the joined BigLibs):
`atoi`, `compilersignals` (`WBStartup`, `FromCLI`, `NumPars`,
`ProgDir$`, `Par$`, `Delay_`), `data_restore_in_function`, `exec_data`
and `localization` (both with AB3's `all.res` as resident, through a
`.xtra` with a full path), `fabrizio_speedquestion`, `memoryleak_test`
(its `OPTIMIZE 13` line removed; dynamic `Dim List xyz.s(0)`, 8000
items, `ClearList`, `AvailMem_`), `setint` (`SetInt 5`),
`test_newtypes`, `trim` (`Trim$`): **all compile and run with plausible
output** (the same twice, executables identical). Own probes with the
same libraries: files (`OpenFile`, `FileOutput`, `ReadFile`, `Edit$`,
`Eof`, `Exists`), banks, `AllocMem_`; lists of newtypes (`AddLast`,
`KillItem`, `FirstItem`, `LastItem`, `PrevItem`), arrays of newtypes;
procedures with strings, `Select` on strings, string compares, `Data$`,
string growth to 2000 characters; FFP `Sin`/`Cos`/`Sqr`, long
multiply/divide: output equal to the same source compiled with BB2's
libraries where both have the commands. Runtime error checking on: the
same programs work. Not compared with AB3's own compiler (that is the
reference test of G1).

**Failures and causes:**

| Program | Result | Cause | For |
|---|---|---|---|
| probe `a.f^10` | "Library not Found" | blitz2 calls the float power as token `mathq` × 128 + 1, `mathq` = library 192 (`main.h`, `eval.c` `dopow`); in AB3 the math library is number 44 (token `$1601`, `AmiBlitz3.ab3` 14565) and 192 is an OsLibs library | G1: the only hard-coded user library number in blitz2; per mode |
| `data_constants` | "Garbage at End of Line" at `Data.w #test1,#test2,3,-1` | constants in `Data` | G5 (known) |
| `hexdigits` | "Syntax Error" at `NPrint Bin$(_start)` | a variable name beginning with `_` is not accepted in an expression (the assignment `_start.l = …` compiles) | language, new: G2 or G5 |
| `elmoreinclib` | hangs | the unregistered ElmoreInclude library shows its alert "CLICK MOUSE TO CONTINUE" at start (the library's own behaviour, also under AB3) | none; not usable headless |
| `exec_data`, `localization` without resident | "Type Not Found" | `ExecBase`, `Locale` come from `all.res`; with it they work | G1: header residents relative to `BlitzLibs/Residents` |

The other programs of the test directory were not tried: they need
`OPTIMIZE`/`SYNTAX`, optional parameters, `@@`, FPU or 68020 code, wait
for the mouse or for input, or open windows. Tokenising pitfalls of own
sources (not AB3's): AB3's and BB2's command and mnemonic names become
tokens inside identifiers (`show{`, `add{`, `sub1` failed as names).

**What G1 must therefore contain** (beyond section 1): loading the four
BigLibs in AB3's order (the existing loop, no end-mark change); a
per-mode value for `mathq` (44 in AB3 mode); AB3's compiler token table
and AB3's library tokens for the tokeniser; residents from the header,
resolved relative to the residents directory (the `.res` loader as is).
Not needed for BB2-language AB3 programs: any new internal entry
(`$C405`/`$C406`, `$C502`–`$C509`, `$C602`/`$C603`, `$C703`,
`$C31F`/`$C320`, `$CA02`/`$CB02`, `$C801`), a change of the init
conventions, the plugin mechanism. Add to G2/G5: identifiers beginning
with `_`. Still unknown: the debugger with AB3's debuglib (out of
scope), `.d` fields in residents (no AB3 program of the spike used one),
and whether outputs equal AB3's own compiler (G1's reference test).
