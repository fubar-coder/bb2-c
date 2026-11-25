# Proposal

## Why

Since #199 a Long or Quick `a*b` whose product does not fit gives the product modulo 2^32 (Quick: bits 16..47); the original lmullib added the cross products SWAPped (Long: too large by hi16(ah*bl)+hi16(al*bh), at most 65534; Quick: by hi16(ah*bh), at most 0.25). A program that depends on the original's overflowing products, e.g. its own generator `seed=seed*69069+1`, gets another sequence (seed 4711: second number 535264088 with the original, 535263821 now; `~/.local/share/bb2-c-work/analyse-199-lmullib.md`, section 2). The documented way back, the Blitz functions `OldMul{}`/`OldQMul{}` (`docs/libs/bb2-readme.md`, "lmullib: the products of the original (#199)"; site "Getting the old products"), is exact but far slower than the operator.

User decision (2026-11-14, `~/.local/share/bb2-c-work/decisions/areas/libs.md`, "#199 lmullib overflow"): "Document how a programmer can get the old overflow results back, if possible". Issue #1036 proposes two library commands with the original routines, as fast as the operator; the task decision of 2026-11-22 (`tasks/decisions.md`, "Matrix limit in the eval.c line; more issues in parallel") puts it in parallel to the eval.c line.

## What Changes

Names and place are decided by the user (below); the number and the docs are planning choices (mine, below).

- **Two functions with the original's products** (user decision 1): `LMulBB2(a.l,b.l)` returns the Long product as the original lmullib's `longmult` gave it; `QMulBB2(a.q,b.q)` the Quick product as its `quickmult`. Both arguments in d0/d1, as the operator's routines take them; the routines are the original lmullib code (`archive/aminet/amibl2src/bblibsrc/lmullib`).
- **Usage:** `seed=LMulBB2(seed,69069)+1` gives the original sequence. The operator `*` keeps the port's products.
- **Cost:** a call costs what the original's operator cost: one JSR, the original routine (the tests measure it against the CD's lmullib).
- **A new command library** (user decision 2; number: planning choice below) in `acidlibs`. lmullib and every other existing library stay unchanged.
- **Results** (rule 3): without the functions every executable and every existing library file stay byte-identical. Changed (intended): the `acidlibs` files of the variants with the library, the token list TED and blitz2 load.
- **Docs and site** (planning choice below): the OldMul{}/OldQMul{} texts in `docs/libs/bb2-readme.md` and on the site point to the functions; the functions get the generated command page with an intro.

## Not in scope

- A generator command (a fixed LCG): programs use different generators; what they share is the multiplication.
- `Rnd` (vallib's generator, no lmullib call); Word and Byte multiplication (MULS, never affected).
- The operator `*`, lmullib, AB3 mode with AmiBlitz3's libraries.
- Loading the original lmullib as a userlib that replaces library 64835 (the alternative of the issue: replaces every multiplication).

## Decisions of the user (2026-11-22, `~/.local/share/bb2-c-work/decisions/tasks/decisions.md`, entry "#1172: behaviour and form; #1036: names and place")

1. **Names:** "LMulBB2 / QMulBB2" — `LMulBB2` (Long) and `QMulBB2` (Quick). (Not `OldMul`/`OldQMul`, which as tokens would break every source that defines the documented `Function OldMul{}`.)
2. **Place:** "Neue Library in acidlibs (Recommended)" — a new command library in `acidlibs` (number by the plan, 166 if free); lmullib and existing executables unchanged.

Planning choices (mine, not the user's):

- **Library number** 166, if task 1.1 confirms it free. `Point: library number 166 — a later find in AmiBlitz3's binary libraries or the archive takes it — L1 Unlikely / I3 Observable / E2 Reasoned → Plan`.
- **Docs and site:** the OldMul{}/OldQMul{} texts stay and point to the functions, as the issue proposes (the Blitz functions remain the way for the CD's libraries and AB3 mode); not replaced. `Point: OldMul{} texts kept with a pointer — the user may prefer them replaced — L2 Possible / I1 Low / E2 Reasoned → Monitor`.

## Capabilities

### New Capabilities

- `libs/original-products`: the library functions that give the original lmullib's Long and Quick products, their cost and that nothing else changes.

### Modified Capabilities

(none)

## Impact

- `libs/acidlibs/` a new library file and its CMake entry; `libs/variants/bb2-2.10/libnums.h`; `libs/acidlibs/declarations-intended.txt` if the build asks for it.
- Not the compiler: no file under `programs/` (not `eval.c`; not in the eval.c line).
- Tests: a library capture `libtest_NAME` in `libs/tests` whose blocks equal the `[orig]` blocks of `libtest_lmullib`; a test of the BASIC test model in `tests/blitz2/` (the 3000 pairs of #199 against `OldMul{}`/`OldQMul{}`, edge values, the LCG); two `tests/perf` lines; the prev comparison.
- Docs: `docs/libs/bb2-readme.md`, the library's head comment; site: `fixed-bugs/libraries/index.md` ("Getting the old products"), `site/data/libintros/acidlibs/NAME.yaml`, `site/data/cmddesc/acidlibs/NAME.yaml`.
- Overlaps: none with `console-break-option` any more (it adds no `acidlibs` library); `quick-long-arithmetic` edits `docs/libs/bb2-readme.md` (its tasks are done; another section).
