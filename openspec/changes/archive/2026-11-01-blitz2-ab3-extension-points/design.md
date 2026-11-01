# Design

## Context

See proposal.md (Why). Current state (main, 2026-11-01):

- `programs/blitz2/CMakeLists.txt` `blitz2_program(TARGET [DEFINES])`
  compiles the chunk sources (`main compile stmts asmlibs parse eval
  libcall tokens1 tokens2 data` + `glue*.c`) once per program;
  `blitz2-ab3` adds `BLITZ2_AB3=1`, `ab3main.c`, `shared/xtra`,
  `shared/ab3job`.
- 36 `#if BLITZ2_AB3` lines in `main.c` (10), `asmlibs.c` (5),
  `libcall.c` (5), `stmts.c`, `compile.c`, `tokens1.c`, `main.h` (3
  each), `parse.c` (2), `eval.c`, `compile.h` (1). Kinds:
  - notifications: `ab3newtoken(tok)` (compile.c, eval.c),
    `ab3_incline`/`ab3_incdone` (libcall.c), `ab3_errpos()` (parse.c);
  - alternatives by mode: `makeab3libs` (main.c), the BigLib loop and
    `loadab3userlibs` (asmlibs.c), `loadab3res` (stmts.c), the library
    error in `compile` (compile.c), `ab3_incread` in `readinc`
    (libcall.c), the include checks in `doinc2` (tokens1.c), editor
    name and job in `begin` (main.c), command line and return code in
    `main` (main.c);
  - data: `progmode`, `tokimg_ab3`, `ab3libname`, `ab3libsat`,
    `ab3reserr`; the macros `mathq` and `tokens` in `main.h`;
  - includes of `ab3main.h`.
- Rule of 2026-11-01 (CLAUDE.md): results byte-identical, programs may
  change with a rework, checked by `ctest --preset all`/`bug-orig`.
- blitz2 must keep starting on Kickstart 1.x (#936, `checklink.cmake`:
  no libnix20 member, no `UtilityBase`).

## Goals / Non-Goals

**Goals:** the chunk sources compiled once and shared; no
`#if BLITZ2_AB3`; extension points as plain functions; AB3 code only in
`ab3*.c`; both programs kept; `blitz2`'s results unchanged; a rule for
G2 on.

**Non-Goals:** one program with both modes (decided against, both stay);
run-time plug-ins (function pointer tables, loading modules); changes to
`ab3cli`, TED, the libraries; a general redesign of the compiler's
internals (the user expects the structure to become clearer with later
reworks).

## Decisions

### D1: Link-time extension, one shared core
`blitz2_core` (CMake object library) holds every chunk file and the glue,
compiled once with no define. The extension points are declared in
`programs/blitz2/src/ext.h`; each program links exactly one
implementation:

- `blitz2`: `ext_bb2.c` – `progmode` fixed to BB2 (a `const` object),
  every point returns "not handled" or does nothing, the BB2 data
  (`tokens` = blitz2's token image, `mathq` = 192).
- `blitz2-ab3`: `ab3*.c` (today's `ab3main.c` split by area) plus
  `shared/xtra` and `shared/ab3job`.

*Why:* maximal reuse – both programs run the same object code for
everything but the extension points; no preprocessor switch; the
interface between core and extension is a visible list in `ext.h`.
*Alternatives:* compile-time constants with one switch header
(previous draft; needed only for byte-identical programs, which the new
rule no longer asks for, and compiles the core twice); a function
pointer table (indirection without need while there is one extension
per program); one program with both modes (decided against).

### D2: Extension points are coarse and named after the behaviour
Each `#if` block of today becomes one point named after what it does
(`ext_makelibs`, `ext_loadlibs`, `ext_loadres`, `ext_compilestart`,
`ext_incopen`, `ext_incline`, `ext_readinc`, `ext_incdone`,
`ext_errpos`, `ext_newtoken`, `ext_editor`, `ext_cmdline`,
`ext_result` …). Where a block replaces part of a function, the point
returns whether it handled the case and the BB2 path stays in the chunk
file (`if (ext_loadres(res)) continue;`). Mode-dependent data that the
core reads (`tokens`, `mathq`, `progmode`) is reached through the
extension (objects defined by `ext_bb2.c` or `ab3*.c`), never as
`#define`s depending on a build switch.
*Why:* the chunk files stay translations of blitz2.s with visible,
few extension points; G2's parser branches use the same pattern
(`if (ext_newsyntax()) …`).

### D3: What stays where
The chunk files keep all BB2 behaviour; `ext_bb2.c` contains no BB2
logic, only the "not handled" answers and the BB2 values of the
mode-dependent data. AB3 logic and AB3 data live only in `ab3*.c`.

### D4: Build check
A check in the build step of the core fails if any file under
`programs/blitz2/src` contains `BLITZ2_AB3` (CMake over the sources),
with a test on broken input (a copy with the word must fail). The
`blitz2_program` define parameter goes away.

### D5: Acceptance
`blitz2`: its results are byte-identical (corpus, bug-orig and CD
executables – the existing `ab3_bb2_equivalence*` compares `blitz2-ab3`
with `blitz2`; additionally the executables of the corpus/bug-orig run
of `blitz2` before and after each commit are compared); `ctest --preset
all`/`bug-orig`; `checklink` for both programs. `blitz2-ab3`: all AB3
tests (`ab3_cli`, `ab3_cli_ref`, `ab3_tokens`, `ab3_bb2_equivalence*`,
`corpus_cd_ab3`).

## Risks / Trade-offs

- [`blitz2` gets calls into almost empty functions: a few bytes and
  cycles] → accepted under the new rule; `-Os` keeps them small.
- [`ext_bb2.c` drifts from the AB3 side's interface] → both implement
  the same `ext.h`; a missing function is a link error in one program.
- [Kickstart 1.x: shared code must not pull in KS2-only calls into
  `blitz2`] → the AB3 code with `PROGDIR:` etc. stays in `ab3*.c`;
  `checklink` runs for `blitz2`.
- [Comparing `blitz2`'s executables across commits needs the previous
  build] → the agent builds the previous commit in a scratch tree, as
  for the byte comparison today.

## Migration Plan

Small commits, one area each (core library and `ext.h`, libraries,
residents, includes, errors and tokens, command line and editor, build
check, docs); each with the acceptance of D5.

## Findings during the implementation

- *Order of the tasks:* the core can only be compiled once without a
  define (task 1.2) when no `#if BLITZ2_AB3` block is left in it – before
  that, `blitz2-ab3` would lose its AB3 mode. So 1.2 follows 2.5: until
  then `blitz2-ab3` still compiles the chunk files itself with
  `BLITZ2_AB3`, and every block that becomes an extension point is gone
  from both builds.
- *`progmode` is not an extension datum:* once the blocks are points, no
  chunk file reads it – only the AB3 modules do. It is AB3 state
  (defined in `ab3main.c`), `ext_bb2.c` has none; a chunk file that would
  ask for the mode calls an extension point instead (the rule for G2).
- *The data of the mode are objects:* `ext_toklist` (asm `tokens`) and
  `ext_mathq` are variables with BB2's values in both implementations;
  `blitz2-ab3` sets AB3's when its command line decides the mode, before
  anything is loaded (the mode does not change after that). The asm names
  stay as macros in `main.h` (`tokens`, `mathq`), with no build switch.
- *`ext_errpos` lives in `ab3inc.c`, not `ab3tok.c`:* the place of an
  error is the include line count of a command line run, the state of
  `ab3inc.c`; `ab3tok.c` has the token list, `ext_newtoken`, and the mode
  with its data (`progmode`, `ext_toklist`, `ext_mathq`, set together by
  `ab3_setmode`). The macros a token image is built of (asmmac equates,
  node members) are shared by `main.c` and `ab3tok.c` in `tokimg.h`.
- *The check of D4 runs at the link of the programs:* CMake refuses
  `POST_BUILD` commands on an object library ("may not have PRE_BUILD,
  PRE_LINK, or POST_BUILD commands"). `checkcore.cmake` therefore runs
  in the `POST_BUILD` step of `blitz2` and `blitz2-ab3`, next to
  `checklink.cmake`, over every file of `src/`; both programs have all of
  them as `LINK_DEPENDS`, so a change of any file relinks and checks
  again. A failing check leaves the program not built, as for
  `checklink`.
