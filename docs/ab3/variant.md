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
* **Recommended form:** one source tree, a second build `blitz2-ab3`
  (`-DBLITZ2_AB3`) that has BB2 *and* AB3 mode and switches **per
  source** at run time. The default `blitz2` is built without the AB3
  code, so it stays byte-identical; `blitz2-ab3` must give the same
  executables as `blitz2` for every BB2 source.
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
| Default `blitz2` byte-identical | no (new code in it; acceptance only functional, deviation to be named in every commit) | yes | yes (`#if BLITZ2_AB3` off) |
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
pre-2009 AB2 changes, which have no history. Every addition sits under
`#if BLITZ2_AB3` and is active only in AB3 mode.

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
   header, tokeniser, loader, residents, token table (G1).
2. **G2–G5** in that order (most programs per line of code).
3. **G6, G7** (FPU and 68020; needs an A1200/FPU machine in the
   emulator for the tests).
4. **G8** only if AB3's error behaviour or byte-equal executables are
   wanted.

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

How it works (`shared/ab3job`, `programs/blitz2/src/ab3main.c`,
`programs/ab3cli`): `blitz2-ab3` parses the arguments, reads the source
and its companion `.xtra` and decides the mode (`xtra_mode`, §1) before
it loads anything; then it runs as for the editor, but loads
`PROGDIR:ab3cli` instead of `blitz2:ted` and hands it the job instead
of a file name. `ab3cli` is a headless editor on the edstub frame with
TED's core: it loads the source as TED does (a text source tokenised
with the token list of the program's mode), hands the compiler the
options (AmiBlitz3's embedded header as a Blitz 2 `.xtra` image, else
the companion `.xtra`, else the compiler's own; `-r` switches runtime
error checking, the debugger and debug information off), compiles with
CREATE EXECUTABLE and prints errors as `FILE:LINE: MESSAGE`.

Includes: blitz2 only reads tokenised includes (`readinc`, design D4 of
the change). In a command line run an include of text is therefore
tokenised by `ab3cli` first – TED's way of loading a source, with the
program's token list – and `readinc` reads those lines; an editor
session reads includes exactly as `blitz2`. A tokenised include of the
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
* `PROGDIR:` needs Kickstart 2.0; the editor session runs on 1.3 as
  `blitz2`. The `.xtra` image goes to the compiler through a temporary
  file in `T:` (else `RAM:`).
* The line of an error in an include of text counts the lines as TED
  loads them: after a line TED splits (longer than 128 characters) the
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
AmiBlitz3's ten test programs of the spike (`memoryleak_test` without its
`OPTIMIZE` line), `^` (library 44), header residents with and without a
path, includes of text in both modes, the userlib, and the errors - a
missing BigLib, a missing resident, tokenised includes of the other mode
(both ways), `-d`, a syntax error, errors in an include of text and in a
tokenised one. `ab3_cli_broken`: a wrong expect file must fail.

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
