# Compiler: what AmiBlitz3 changed relative to Blitz Basic 2.10

> As of 31.10.2026. Documentation only: the basis for deciding which
> AmiBlitz3 changes the C port takes over. Nothing was built or run;
> statements come from commit messages, changelogs and reading the
> AmiBlitz3 compiler source. Uncertain mappings are marked "(?)".

## Sources and method

* **AmiBlitz3** (GitHub `AmiBlitz/AmiBlitz3`), full history
  2009-02-10 … 2026-10-25 (`AmiBlitz/AmiBlitz3@ea8c25c7` … `AmiBlitz/AmiBlitz3@b1a9dee1`).
  The first 602 commits (to `AmiBlitz/AmiBlitz3@c95cce17`, 2016-06-01) are
  the SourceForge SVN history (r1–r602); SVN revision numbers were taken
  from the `git-svn-id` lines of the git-svn import of the same history.
* **Compiler source**: `Sourcecodes/Amiblitz3/Compiler/amiblitz3.ab2` at
  `ea8c25c7` (then `AmiBlitz3.ab2`, from `AmiBlitz/AmiBlitz3@a04ed520`,
  2013-12-01, `AmiBlitz3.ab3`) plus `internal_commands.*`,
  `internal_errormsg.*`, `CLIModeSupport.*`; 336 commits touch it
  (`git log --follow`).
* **Pre-git delta**: the 2009 source is already AmiBlitz 2.x (Bernd
  Roesch, 1999–2008) plus the first AmiBlitz3 betas. These changes are
  known only from `Docs/Changelog.guide` (at `ea8c25c7`; HEAD:
  `Docs/Miscellaneous/Changelog.guide`, which also has the release notes
  3.7.0–3.14.0) and from the code itself. Changes Bernd made are marked
  `; new >` … `; < new` in the source.
* **Baseline**: `archive/sources/blitz2-1.95/blitz2.s`, the 2.10 changes
  in [`docs/v2.10/blitz2.md`](../v2.10/blitz2.md), our issues
  (`fubar-coder/bb2-c`, label `bug-orig` + `compiler`).
* **Method**: compiler entries of the changelogs (PED, TED, editor,
  debugger GUI and IDE skipped); subjects of all 336 commits, diffs read
  where the subject names a fix or is unclear; labels of the
  disassembly were renamed to the names of the original source in
  2009/2015, so HEAD can be compared with `blitz2.s` label by label.
  Breadth over depth: a fix is listed when the code was checked or the
  message is unambiguous; changelog-only entries are marked as such.

## Fixes of BB2 bugs

| AB3 commit | Date | What | Our issue | blitz2.s |
|---|---|---|---|---|
| `AmiBlitz/AmiBlitz3@d7d380c0` | 2026-10-02 | Relocation table split into several `HUNK_RELOC32` blocks (the V36+ loader cannot load more than 65535 entries in one) | #943 | 6852–6874 (`savefile`) |
| `AmiBlitz/AmiBlitz3@808bc4c2` (SVN r291) | 2012-03-07 | Amiga library call with a parameter in A4/A5: `movem.l a4/a5,-(a7)` / `movem.l (a7)+,a4/a5` around the call (`checka4a5`/`restorea4a5`); BB2 loads A4/A5 from the stack and loses the local/global base | #958 | 3879 (`stamigalib`), function-call path |
| `AmiBlitz/AmiBlitz3@aee91740` | 2025-03-07 | `IncBin`: the lock is released right after `Examine`, before the directory check (the file handle is still not closed on a read error); message also names "not closing include files after compile error" (?) | #38 (lock part only) | 12772 (`doincbin`) |
| `AmiBlitz/AmiBlitz3@185c218e` | 2026-06-29 | Userlibs loader (rewritten in BASIC): the reloc-header `Read` is compared with `= 12`, so −1 fails; the header and code reads are still unchecked; deflibs/acidlibs are read through another routine | #3 (partial, userlibs only) (?) | 6470 (`loadalib`) |
| `AmiBlitz/AmiBlitz3@c7fc6f60` | 2024-09-16 | "fixed bug in freeing memory for macrobuffer": now `FreeMem(begin-4, macrobuffer_size+4)`; still the **current** size, not the one stored at allocation – does not fix the mismatch | #336 (not effective) (?) | 1384–1390, 1463–1466 |
| pre-git (AB2 2.21) | ≤2008 | Source line counter is a long (`linenumat`/`compilelinecounter` `.l`): more than 32768/65535 lines | – | `linenumat` (word), e.g. 2093 |
| pre-git (AB3 3.00x) | ≤2009 | Userlibs scanned with a new routine; path built with `dos_AddPart` | #2 (already fixed in 2.10) | 6400 (`loadblitzlibs`) |
| pre-git (AB2 2.27), changelog only | ≤2008 | "Fixed old BB2 bug when call OS command with more than 2 stringvars" | – (?) | – |
| pre-git (AB2 2.22), changelog only | ≤2008 | "memtrash bug if object buffer increases between compiles (object+lib buffer)" | – (?) | `alloxinc`/`makeiallox` (?) |
| pre-git (AB2 2.27), changelog only | ≤2008 | `type\a,1,2,3,4,5` wrong when the start is not word-aligned | – (?) | – |
| pre-git (AB2 2.26/2.3), changelog only | ≤2008 | Characters ≥ 128 in `Data$`; `a && 4` gives "can't get address of constant" | – (?) | – |
| pre-git (AB2 2.25/2.27), changelog only | ≤2008 | Macros: `End` and `If … Then` inside macros, parameter line joining | – (?) | – |
| pre-git (AB2 2.27), changelog only | ≤2008 | "object address calculations work with 32 bit instead of 16 bit, up to 32000 objects" – **not found**: HEAD `getmaxel`/`free`/`use` still emit `lsl.w` + `adda.w` | #939 (not in code) (?) | 12196–12266 |

Spot-checked in HEAD and **unchanged** (no AB3 fix): #9 (`=<`,
`.noteqgt`), #15 (`muld1`, ×256), #37 (`Dcb` count 0), #16/#17/#325/#341
(`doincdir`/`makeiname`), #29 (`makename4`, despite the 2.23 entry "long
varnames, 128 chars"), #939.

Areas AB3 replaced wholesale, so the BB2 bug is gone or changed with the
code (not verified in detail): the options requester (moved to PED,
#8, #32), loading of the debugger module (#23, removed), running a
program (`CreateProc` → `CreateNewProc`, `AmiBlitz/AmiBlitz3@fcc76476`,
`AmiBlitz/AmiBlitz3@4b5e552a`; #24, #25), signal allocation (BASIC,
`AllocSignal_(-1)`, still unchecked; #18), Workbench start
(re-activated in `AmiBlitz/AmiBlitz3@ca137a15`; #19–#22, #337), library
loading (BASIC, `AmiBlitz/AmiBlitz3@3bf667d9`, `AmiBlitz/AmiBlitz3@d3a79260`;
#3, #28), VBR code removed from the run path
(`AmiBlitz/AmiBlitz3@d196cf4a`; #42).

## Extensions

| AB3 commit / version | What |
|---|---|
| pre-git AB2 2.2–2.51 | Optional parameters (`@par`, missing = −1/""), 6 → 10 procedure parameters, 68020 addressing modes and `extb.l` in inline asm, `Dim KEEP` (redim), `Syntax` bits (declare, type checks), "new syntax" (`Optimize 4/5`), string escapes `\\xx`/`\\Rnn`, `@"ILBM"` string-to-long, FPU register variables (`regload`/`regsave`), `FAST` functions, local `List`s, `Dim` in functions, `End d0`, `Debug`, removal of unused functions (function optimizer), Include search in IncDir and source dir, `.res` search in `blitzlibs/residents/` |
| pre-git AB3 3.00–3.01 | Compiler constants `__THIS_FUNCTION`, `__CODELINE`, `__THIS_INCLUDE`, `VER_MAJOR/MINOR/PATCH/BUILD` |
| `@0379077e` r44, `@5727df1e` r43, `@e3d8e6b2` r112, `@c9d7be8b` r201 | ASCII sources and includes (`.ab2`, `.ab3`), include extension fallback `.ab3`/`.ab2`/`.bb2` |
| `@70d37cb1` r132 and follow-ups | `.dbg` file (hunk offset → source line) |
| 2009–2014 (`@457f1060` r118, `@06c411dc` r121, `@cc3084c8` r452, `@93f9def2` r454 …) | CLI mode: compile from the shell, dumps of internals, settings as arguments |
| `@43383ece` r260, `@f8598aa6` r313 | Continue after errors, maximum error count |
| `@a499d7d7` r270 … `@98cbaf65` r304, `@0ef25617` r472 | Bernd's type checks (`Syntax 4/6`, function return types, pointer types), casts `(.type)`, `*p.type = New`, `Null` for all types, `\\THIS_FUNC` (`@51097ba6` r261) |
| `@e1bb054f` r542 | `Dim` checks that the name was not used as a non-array variable |
| `@2e216c5b` r557, `@929e1e69` r559 | One-pass compiling with big buffers; "minimized executable" loop in the compiler |
| `@dc18bb84` 2022-08-31 | `moveq` instead of `move.l` for byte-range constants |
| `@5432585d` 2022-09-29, `@c6e35d97` | Operators `<<`, `>>`, `ROL`/`ROR`; upper-case size suffixes `.B/.W/.L` |
| `@4fb07073` 2025-11-14 | Constants in `Data` (`Data.l #X`) |
| `@2b7def51`, `@bb485acf` 2026-01 | Jump optimizations (short branches for `Repeat … Forever`, local procedures; `OPTIMIZE` flag 8) |
| `@448ae996`, `@f26d9eee`, `@6e902bbe` 2026 | `OPTIMIZE` moved from fpulib into the compiler; constants as parameters; quick mul/div for 68020+; macro expansion after `@@` line joining |
| `@7ab0350b` 2025-02-04 | A userlib with a lib number already loaded replaces that lib |
| `@06d84c9f` 2024-03-19 | Error for an unknown token used inside a blitzlib |

(`@xxxxxxxx` = `AmiBlitz/AmiBlitz3@xxxxxxxx`.)

## Incompatible changes

| AB3 commit / version | What |
|---|---|
| pre-git (AB2) | New tokens and syntax (`Optimize`/`Syntax` semantics, "new syntax" changes operator parsing: `a and b` → `&`, `:` after labels before `lsl`); `Dcb.w/.l` no longer aligns the PC (`bsr alignpc` commented out); `Ds` writes zero bytes |
| `@b5839e6b` r487, `@03f751c6` | Token of `Null` moved (old `Null` → `StrToAdr`); the compiler patches the token id |
| `@b0627f37` 2025-04-01 | `KEEP` is a token (sources with an identifier `keep` change) |
| `@8418c48c` 2022-09-13 | Token names `syntax`/`optimize` → `SYNTAX`/`OPTIMIZE` |
| `@9dc4796f` 2021-11-18, `@b043e8fa`, 3.13.0 | Library collections `baseLibs`/`UtilLibs`/`osLibs`/`csLibs` instead of acidlibs/deflibs; lib numbers changed (mathtrans, reordering) – tokenised sources and `ALibJsr` depend on them |
| `@8b33ff50` r268 | `HUNK_SYMBOL` names: `'` → `_`, `$` → `S`, names with `?` dropped |
| `@e69cec92` r134 | A6 no longer kept across procedures (different code) |
| AB2/AB3 runtime environment | Compiler needs OS 2.0+ (`CreateNewProc`, `AddPart`, icon.library) and exec memory pools (`AllocPooled`, V39); AB2 array code used `MULU.L` (68020) until `@f9c423e5`/`@6b18f7e6` (2020); FPU code only with `OPTIMIZE` (checks since 3.7.0) |
| `@e1bb054f` r542, Bernd's type checks | Code BB2 accepted can now fail (depends on `Syntax`) |

## AB3 regressions noticed

* 68000: AB2 array code with `MULU.L` – fixed `AmiBlitz/AmiBlitz3@f9c423e5`,
  `AmiBlitz/AmiBlitz3@6b18f7e6` (2020); `Dim` limited to 32768 elements by
  `MULS.W` – fixed `AmiBlitz/AmiBlitz3@8f2e48f8` (2025).
* Inline asm `.l` misparsed (`BNE` for `BEQ`), introduced
  `AmiBlitz/AmiBlitz3@c6e35d97`, fixed three days later
  `AmiBlitz/AmiBlitz3@35b09858`.
* Procedure call with a function call as parameter ≥ 7 (AB2's 10
  parameters): `AmiBlitz/AmiBlitz3@d6f6b02e` (r468),
  `AmiBlitz/AmiBlitz3@a5bbb961` (r469); optional string parameters:
  `AmiBlitz/AmiBlitz3@2433ad55` (r188), `AmiBlitz/AmiBlitz3@1d654960` (r189),
  `AmiBlitz/AmiBlitz3@93dabadd` (also removed an FPU dependency).
* `*ta_Name.b` problem from AB2's offset optimization (`leaa0a0`):
  `AmiBlitz/AmiBlitz3@a77db083` (r141); expression look-back past the
  buffer end: `AmiBlitz/AmiBlitz3@750195a1` (r218).
* Userlibs list corrupted (`AmiBlitz/AmiBlitz3@a62f6d4f`,
  `AmiBlitz/AmiBlitz3@cd9238df`), `ALibJsr` "unknown token" after lib
  renumbering (3.13.0), `Null` token change broke `StrToAdr`
  (`AmiBlitz/AmiBlitz3@b5839e6b`).
* The macro buffer free (#336) still uses the current size.

## Cosmetic and other commits

Of the 336 commits, 97 are cosmetic: 29 label renamings to the
original names (2009, 2015), 46 clean-ups, comments and reformatting,
7 asm → BASIC conversions without an intended change, 15 version
numbers / executables only. Most of the rest are IDE, PED, debugger,
`.xtra`/`.ab3` format, CLI-mode, logging, localisation, icon and
PED–compiler interface (`comdata`) changes, or fixes of AB's own
additions; they do not affect the compiled code.
