# An AmiBlitz3 variant: plan

> As of 2026-10-31. A plan, nothing is built. It is based on the change
> lists of this directory ([compiler.md](compiler.md),
> [deflibs.md](deflibs.md), [acidlibs-a-l.md](acidlibs-a-l.md),
> [acidlibs-m-z.md](acidlibs-m-z.md), [userlibs.md](userlibs.md)), the
> variant build (README, "Variants"; `libs/CMakeLists.txt`,
> `cmake/BlitzLibrary.cmake`) and token counts measured with
> `blitztokens` on AmiBlitz3's BigLibs. Guesses are marked "(guess)".
> `@xxxxxxxx` = `AmiBlitz/AmiBlitz3@xxxxxxxx`.

## Result

* AmiBlitz3's **incompatible library changes** (token tables, numbers,
  removed libraries, functions instead of commands, new memory and list
  layout) can be built as a variant with a **small extension** of the
  variant mechanism: a per-variant define plus optional per-variant
  source files and library lists (CMake only). The default variant stays
  byte-identical because it never sees the define.
* Many AB3 changes **need AB3's compiler**: its own token table differs
  (e.g. `MOVEP` → `Chip`, `TAS` → `BSS`, 8 new tokens), it calls new
  internal library entries, and AB3 sources use its language extensions
  (optional parameters, `Syntax` type checks, `<<`/`>>`, `KEEP`,
  `OPTIMIZE`, ASCII sources …). A variant of AB3's *libraries* with
  blitz2 2.10's *compiler* compiles only programs written in the BB2
  language against AB3's commands.
* **Recommendation:** a variant `ab3-3.14` pinned to the release
  **v3.14.0** (`@58a11658`, 2026-10-19; libraries and compiler identical
  to HEAD `@b1a9dee1`), built in stages: (1) the mechanism, (2) the
  libraries Blitz Basic 2 has (acidlibs + deflibs), checked token for
  token and declaration for declaration against AB3's binaries, with the
  unchanged blitz2; (3) a compiler variant `blitz2-ab3` only after a
  separate decision, driven by a corpus of AB3 sources; (4) AB3's ~120
  new libraries only on demand and only with a clear license. Start after
  the port is complete (CLAUDE.md: original bugs are not fixed before,
  and an AB3 variant contains AB3's fixes).

## Measured facts

Token lists with `blitztokens` (our blitz2's own 210 tokens + the
BigLibs; AB3's BigLibs have no end mark, a copy with 4 zero bytes
appended was used where the tool refused):

| State | Library tokens | BB2 library tokens with another number or gone |
|---|---|---|
| BB2 2.10 (acidlibs + deflibs) | 1611 (96 libraries with tokens) | – |
| AB2 2.x = AB3 SVN r1 `@ea8c25c7` (2009) | 3669 | 125 (in 12 libraries: deflibs 235, 248, 250, 254; brexxlib, gadgetslib, gtlib; polylib 121, crm 200, colorwheel 219, xprzmodem 2 removed) |
| v3.11.4 (2026-03-09, before the renumbering `@dbc34db6`) | 4373 | 167 |
| v3.14.0 = HEAD (2026-10-19) | 4435 (239 libraries: baseLibs 33, csLibs 32, utilLibs 101, OsLibs 73) | 225 (in 24 libraries); 40 names gone, e.g. `GetIcon_`, `ChildFree_`, `cr_vbr`, `SetTextGadget`, `GTGimmeCludge`, the polylib/CrM/XPR tokens |

Of r1's tokens, 688 have another number at HEAD. AB3's compiler binary
(`Amiblitz3`, HEAD) has 218 own tokens against our 210: `SYNTAX`
replaces `Jimi` ($8014), `EXTB` the second `Jimi` ($803B), `Chip`
replaces `MOVEP` ($807F), `BSS` replaces `TAS` ($80AD),
`Runerrson`/`Runerrsoff` are spelt `RunErrsOn`/`RunErrsOff`, and
`USELASTPATH`, `RegLoad`, `RegSave`, `Object`, `USEPATH2`,
`LastRunErrsMode`, `KEEP`, `OPTIMIZE` are new ($80D4–$80DB).

Since `@9f5e7b8c` (SVN r444, 2014) AB3 keeps its sources as ASCII
`.ab3`; PED tokenises them when loading, with the token list of the
libraries present. For ASCII sources the token *numbers* therefore matter
less than the *names, forms and behaviour*; numbers matter for tokenised
`.bb2` files (AB2 era: r1 numbering), for `ALibJsr`/`BLibJsr` between
libraries, and for binary userlibs built against AB3's numbers.

## 1. Mechanism

Today a variant is `libs/variants/NAME/libnums.h`; `blitz_library` puts
the variant directory first in the include path, `acidlibs_build` and
`deflibs_build` build the fixed lists `ACIDLIBS_ORDER`/`DEFLIBS_ORDER`
from the fixed source directories. TODO.md 5.4 decided: "Further switches
besides the numbers: none needed, added only when a library needs one".
An AB3 variant is that case. Proposed, in this order of preference:

1. **Per-variant define in the variant directory.** `libs/variants/ab3-3.14/libnums.h`
   defines the numbers and `#define BLITZ_VARIANT_AB3 1` (or a separate
   `variant.h` that `blitzlib.h` includes with `__has_include`). Every
   library already includes `<libnums.h>`, so no CMake change. Library
   sources use `#if BLITZ_VARIANT_AB3` for changed token-table entries
   (the table is in definition order, `-fno-toplevel-reorder`, so an
   entry inserted or left out in the middle moves the later tokens just
   as in AB3) and for small code changes. The default variant never
   defines it, so its preprocessed sources and therefore its files stay
   byte-identical; the normal acceptance (build of the previous commit)
   proves it. One define per *pinned state*, no version arithmetic
   (`#if AB3 >= 31100`): one state is hard enough to keep exact.
2. **Per-variant source files** for libraries AB3 rewrote (memlib,
   arrayslib + linklistlib with the 40-byte list header, gtlib from
   McMinn's NCS version, windowslib from the RedBlitzLibs one,
   strings1lib/strings2lib/staticslib, debuglib/dhandlerlib if ever) and
   for libraries BB2 does not have: `libs/variants/ab3-3.14/acidlibs/NAME.c`
   replaces `libs/acidlibs/NAME.c` in that variant. Rule of thumb: `#if`
   while the BB2 code stays readable; an own file once more than about a
   third of the library differs (guess). `#if` keeps the BB2/AB3
   difference visible in one place; an own file avoids unreadable C.
3. **Per-variant lists in CMake.** An optional `libs/variants/NAME/variant.cmake`
   that `acidlibs_build`/`deflibs_build` include: the library list and
   order (AB3 drops polylib, crmlib, colorwheellib, xprzmodemlib and adds
   libraries), the override directory of point 2, and the compiler the
   variant belongs to (point 4). blitz2 reads only `acidlibs` and
   `deflibs` (+ `userlibs/`), so AB3's split into `baseLibs`, `csLibs`,
   `utilLibs`, `OsLibs` is not reproduced: the variant joins its command
   libraries into `acidlibs` and its OS libraries into `deflibs` (number
   and token order are what count; `biglib check` keeps numbers unique).
   The file order inside a BigLib only decides which of two equal names
   wins (guess: AB3 has none that matter; the token check below shows it).
4. **A compiler variant** (only with stage 3): `programs/blitz2` built a
   second time as `blitz2-ab3` with `-DBLITZ2_VARIANT_AB3`, output
   `build/amiga/variants/ab3-3.14/blitz2` (then the variant directory is
   a complete `blitz2:`). Same rule: `#if` in place, own files for large
   AB3-only parts (e.g. the type checker); the default `blitz2` stays
   byte-identical (acceptance as above).

Fits the rules: only C and CMake; the checks in the build stay the same
(`blitzlibcheck`, `biglib check` in the join); new checks against AB3
binaries need fetched material and are ctests (skipped without it), each
with a deliberately broken input. `BLITZ_DEFAULT_VARIANT=ab3-3.14` would
build the AB3 libraries to `blitz2/`; then the CD tests
(`declarations_*`, `tokens_port`) must be skipped for a non-BB2 default
(they assume bb2-2.10, like `variant_selftest_*` already do).

## 2. Self-contained in the libraries vs. needs AB3's compiler

**Self-contained** (library code and declarations; blitz2 unchanged):

| Change | Libraries |
|---|---|
| Renumbering (192→44, 185→6, 194→107, 186→79, 193→188, 190→39, OS libraries) | `libnums.h` only (today's mechanism) |
| Moved/removed/added tokens | brexxlib, gadgetslib, editlib, environlib, linklistlib (`FillArray`), sortlib (`ArraySort` at 2), vallib (`True`/`False`/`QAngle` $BC0E→$BC0B), fontlib (?), RI libraries |
| `Window`, `WindowTags`, `Screen`, `ScreenTags`, `FindScreen`, `FileSeek` as functions | `BLITZ_ACOMMAND` exists (BB2's `WaitEvent` uses it) |
| `True`/`False` as `#long`, new seeded `Rnd`, `RndSeed` | vallib |
| `WaitEvent` with signal masks, event records 24 → 32 bytes, `CloseWindow` per window | windowslib (and libraries that read events, guess: none outside windowslib/gtlib) |
| polylib, CrM, colorwheel, xprzmodem removed; numbers 200/219 reused | library list (point 3) |
| deflibs: V40–V47 calls inserted mid-library, private calls removed, init without loop (and the resource displacement `$001D`, an AB3 bug) | 14 deflibs, generated declarations |
| memlib: 16-byte block header, mungwall, exec pools / own pools, pool API `$c009`–`$c00c`; arrayslib/linklistlib: list creation in linklistlib, 40-byte list header, dynamic lists | memlib, arrayslib, linklistlib. The header grew *in front* (fields from −32 keep their offsets) and the compiler still only calls `$C400`–`$C404`, so blitz2 needs no change (guess, to verify: blitz2's inline list code and runerrlib `currentchk` $C315) |
| FPU paths in printlib/vallib, string buffer size, `$c702` string assignment | code is there, but inert until the compiler calls `setfpumode` ($c703) / `setstringbase` ($c603): with blitz2 the non-FPU paths and the default buffer run (guess) |
| statics in one allocation | staticslib (guess: blitz2's `alstat`/`localstat` calls still fit; to verify) |

**Needs AB3's compiler:**

| Change | Compiler part |
|---|---|
| Own token table (`SYNTAX`, `EXTB`, `Chip`, `BSS`, 8 new, `MOVEP`/`TAS` gone) | `main.c` token nodes; small, but every AB3 source using them needs it |
| `Null` token moved (`StrToAdr`), the compiler patches the id; `KEEP` token | parse |
| New internal entries: arrayslib `$C405`/`$C406` (`Dim KEEP`), exitslib `$C801`, runerrlib `$C31F`/`$C320` (stack limit, pointer check), staticslib `$C502`–`$C509`, strings1lib `$C602`/`$C603`, strings2lib `$C702`–`$C704`, lmullib/ldivlib `$CA02`/`$CB02` (68020) | code generation; the library side can be built without them |
| Language: optional parameters, 10 procedure parameters, `Syntax`/type checks and casts, "new syntax" (`Optimize 4/5`), `<<`/`>>`/`ROL`/`ROR`, `.B/.W/.L`, string escapes, `@"ILBM"`, FPU register variables, `FAST`, local lists, `Dim` in functions, `End d0`, function optimizer, compiler constants (`__THIS_FUNCTION` …), constants in `Data`, jump optimizations, 68020 modes/`EXTB` in inline asm, long line counter, continue after errors | parse, eval, stmts, compile |
| ASCII sources and includes (`.ab3`, `.ab2`, extension fallback), `.xtra` header | source loading (our `tools/xtra` + `tedtok` can tokenise on the host instead) |
| `HUNK_SYMBOL` names, A6 not kept across procedures, `Dcb` without alignment, `Ds` zero bytes | code generation (different executables) |
| Several BigLibs, userlib replaces a loaded number | loader (avoidable, point 3) |
| Debugger (debuglib `$D50A`, ComData `$EEE9`, dhandlerlib) | AB3's debugger/PED, not TED: out of scope |

**Size in our port** (guess): blitz2.s has 16015 lines, our C 19433
(factor ~1.2). AB3's compiler source has 28234 lines (13k assembler
lines, the rest BASIC and comments), 336 commits, about 97 cosmetic; a
large part is CLI mode, PED/ComData, debugger, logging. The token table
and the internal-entry hooks are about **100–300 lines** of C. The
language extensions are estimated at **6000–10000 lines** of C changes
(about half of blitz2 again), to be traced through ~240 non-cosmetic
commits plus the pre-2009 AB2 changes, which have no history.

## 3. Target and options

What "ab3" should be:

* **A pinned release, not a moving HEAD.** AB3 renumbers and moves
  commands within months (`@dbc34db6` 2026-04, lists 2026-06). An exact
  variant needs a state its binaries can be checked against. **v3.14.0**
  is the latest release and equal to HEAD in libraries and compiler. An
  older pinned state is only worth it for a purpose: r1 (`@ea8c25c7`,
  AB2 2.x) is closest to BB2 (125 tokens) and fits tokenised AB2-era
  `.bb2` sources; v3.11.4 is the last release before the big renumbering
  (167 tokens).
* AB3 sources are ASCII and use AB3's language: compiling "AB3 sources"
  in general requires the compiler variant. With libraries only, the
  variant compiles BB2-language programs that use AB3's commands.

| Option | Contents | Effort (rough) | Value |
|---|---|---|---|
| 0. No variant | only AB3 fixes as deliberate deviations in the default variant once allowed (the candidate lists of this directory) | per fix | keeps BB2; no AB3 compatibility |
| 1. Numbers only | `libnums.h` with AB3 v3.14.0's numbers of BB2's libraries; drop the 4 removed libraries | mechanism point 3 + 1 file, ~1 commit | low: the in-library token shifts (225 − ~60 tokens) stay wrong |
| 2. **AB3 v3.14.0, BB2's libraries** (recommended) | declarations and code of the 83 acidlibs + 39 deflibs as in v3.14.0: 42 acidlibs with changes (29k lines of our C; ~3000–6000 lines changed or added, guess), 14 deflibs changed + 4 removed (~150 lines), compiler unchanged | mechanism points 1–3 (~2–3 commits) + ~40 library commits + tests | exact, checkable subset; AB3 fixes and behaviour in C; BB2-language programs against AB3 commands |
| 3. Option 2 + compiler variant | `blitz2-ab3`: tokens and internal entries first (~300 lines), then the language as a corpus needs it (6000–10000 lines) | ~50–100 commits (guess) | compiles real AB3 sources |
| 4. Full AB3 v3.14.0 | 3 + the ~120 libraries BB2 does not have (35 new OS libraries ≈ 800 generated declaration lines; ~85 command/third-party libraries, guess 40000–70000 lines of C from disassemblies) | comparable to the whole port | complete, but licenses of many third-party libraries unclear |
| 5. Selection | only AB3's compatible extensions, appended at the end of each library (BB2 numbers stay), in the default variant or an `ab3-compat` variant | per library | AB3 commands without breaking BB2 sources; not AB3-compatible in numbers |

**Recommendation:** option 2 now (after the port), as variant
`ab3-3.14`; option 3 only after a separate decision with a named corpus
(e.g. AB3's `Sourcecodes/Amiblitz3/Tests/*.ab3` and examples); option 4
not as a goal, single libraries on demand. Option 5 is independent of
this plan and can go into the default variant later.

## 4. License

* **AB3's own code** (AB2 by Bernd Roesch, the AB3 team): the repository
  carries Apache-2.0 (`LICENSE.txt`, added by `@2c5e1fdb` 2025-08-11
  following the SourceForge project page); `Docs/Amiblitz3.guide` says
  GPL-2-or-later for "AmiBlitz3" and "PED" and "their own original or
  updated licenses" for libraries and tools. Both allow a C
  reimplementation in this GPL-3.0-or-later project (Apache-2.0 is
  compatible with GPLv3; GPL-2+ can be taken as GPL-3+). With Apache-2.0:
  keep attribution, name the source commit, mark changes (§4) – the
  `@hash` references of this directory and a note in `archive/README.md`
  do that. Covers: the compiler changes, the BB2 libraries as changed by
  AB2/AB3 (memlib, arrayslib, linklistlib, vallib, screenslib, strings
  libraries, environlib's own commands, fileiolib, datelib, fontlib …),
  AB3's deflibs and `AB3FDConvert`.
* **Third-party code inside AB3 – port only with a clear license:**
  * ZX0 decruncher in crunchlib (Emmanuel Marty, zlib license in the
    source): portable, keep the notice.
  * gtlib (David McMinn / NCS, 2000–2004): no license found – unclear.
  * windowslib (descends from the RedBlitzLibs windowslib of the Blitz
    Support Suite, RWE): covered like RWE's 1999 sources if it is that
    code (guess); AB2's additions are AB2's.
  * Commands moved into acid libraries from Elmore*, Lotan*, NeilsSupport,
    riAmos, wzlib (environlib, banklib, rimisclib, vallib `Min`/`Max`,
    scrolllib `QPlot`, windowslib `WaitEvent [IDCMP]`): per author –
    RWE's are covered, Elmore/Lotan/wzlib/Neil O'Rourke unclear.
  * New libraries (MUI, Warp3D, render, ptplayer, chunky/c2p, SDL, …):
    each its own license, mostly not stated; OS library declarations are
    facts of the interfaces (LVOs, registers) and unproblematic.
* Binaries of AB3 as **test references**: fetched by
  `cmake/fetch-originals.cmake` at the pinned commit with SHA-256 (as
  `amiblitz3/github-b1a9dee/` today), not in git.

## 5. Tests

* **Tokens:** `blitztokens` of the variant's files against the token
  list of AB3 v3.14.0's BigLibs (compiler part: our blitz2's 210 tokens
  for option 2, AB3's 218 for option 3). Like `variant_selftest_renumbered`,
  plus a broken input (a token removed) that must fail.
* **Declarations:** `blitzlibcompare` per library against the library
  extracted from AB3's BigLib (`biglib extract` by number). Both tools
  want the BigLib end mark AB3 does not write: an option or tolerance in
  `tools/lib/biglib` with a unit test (input from memory, with and
  without end mark).
* **Byte identity of the default variant:** unchanged acceptance;
  additionally a `variant_selftest_same`-like test that a variant
  defining nothing but including `bb2-2.10/libnums.h` stays identical.
* **Behaviour:** `libs_emulator` cases and `tests/bug-orig` style
  programs with *ref* = blitz2 (CD 2.10 or blitz2-c) + AB3's binary
  BigLibs (joined in the build directory into an `acidlibs`/`deflibs`
  pair) and *port* = blitz2-c + the variant's C libraries. First a spike:
  whether AB3's binaries run without AB3's compiler (FPU mode and string
  buffer never set, `$c320` checks never emitted) (guess: yes for most).
  AB3 bugs the variant reproduces get a marker (`BUG(ab3)`, proposal)
  and an issue, like `BUG(orig)`.
* **Sources:** AB3's `.ab3` programs converted with `xtra convert`,
  tokenised with `tedtok --blitz2 … --lib …` against the variant; for
  option 2 only those in the BB2 language (to be selected), for option 3
  all of `Sourcecodes/Amiblitz3/Tests/`.
* **Compiler variant (option 3):** AB3's compiler binary (`Amiblitz3`,
  CLI mode, `CLIModeSupport.ab3`) in Amiberry as the reference, the
  executables compared as with `blitz2_ref`; needs an OS 3.x machine
  (A1200 configuration).

## Decisions for the user

1. Option (0–5); recommended 2, later 3 only with a named corpus.
2. Pinned state: v3.14.0 (recommended), v3.11.4 or r1/AB2.
3. Start: after the port is complete, or now with the AB3 fixes allowed
   in a non-default variant (CLAUDE.md speaks of the port and the
   default variant only).
4. Reproduce AB3's own bugs (e.g. the resource `$001D` displacement:
   base always 0) as `BUG(ab3)` with issues, or fix them in the variant.
5. Third-party parts with unclear license (McMinn's gtlib, Elmore/Lotan
   commands): leave out (the variant then is not exact for those
   libraries) or ask the authors.
6. Mechanism: `#if` in place vs. own files – the rule of thumb of
   point 1.2, and the variant name (`ab3-3.14`).
