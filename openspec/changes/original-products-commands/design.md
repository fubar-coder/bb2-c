# Design

## Context

- lmullib (`libs/acidlibs/lmullib.c`, system library 64835, $CA00 quickmult, $CA01 longmult) is linked whole into every program with a Long or Quick `*`; a routine added there would change every such executable. A new library is linked only into programs that call its functions.
- The original routines: `archive/aminet/amibl2src/bblibsrc/lmullib` (the SWAP addition of the partial products); the library capture `libtest_lmullib` holds their results as `[orig]` blocks (`libs/tests/expected/libtest_lmullib.txt`, recorded from the CD's library, 11 blocks of up to 4096 pairs plus edge values).
- A library function with two arguments gets them in d0 and d1 and returns in d0 (Blitz library convention), the registers the operator's routines take; the user-level register rule (`libs/register-contract`) asks a function to keep a3-a7 only.
- The Blitz functions `OldMul{}`/`OldQMul{}` (`~/.local/share/bb2-c-work/analyse-199/oldmul.bas`) were checked in the emulator against the original library with 3000 pairs (`docs/libs/bb2-readme.md`).
- Library numbers free in every `libs/variants/*/libnums.h`, the CD's `LIBNUMS.BB` and `BUM9B`/`BUM10A` `LIBNUMS.BB2` and AmiBlitz3's `libnums.ab3` (two copies): 5, 38, 63, 77, 95, 97, 115, 165, 166 (grep of the lists, 2026-11-22; the library files under `archive/` and AmiBlitz3's binary libraries: task 1.1).

## Goals / Non-Goals

**Goals:** the original's products, bit for bit, at the cost of the original operator; no change of any existing result.

**Non-Goals:** see the proposal, "Not in scope".

## Decisions

The user's decisions 1-2 are in the proposal.

1. **A command library "bb2mullib" (working name) with two functions**, `LMulBB2` (Long, `.l` arguments and result) and `QMulBB2` (Quick), each an assembler entry (`BLITZ_ASM_ROUTINE`, as longmult/quickmult) with the original's instructions; no init, no finit, no error checks (an overflow is the point, not an error).
2. **The original code, not the port's longmult with a correction.** The routines are the translation of the archive's lmullib, so the results are the original's by construction; the 3000 pairs and the capture prove it.

### Planning choices (mine)

- `Point: "as fast as the operator" read as the original's operator (the CD's lmullib, the routine the functions copy), not the port's faster longmult — the user may mean the port's operator — L2 Possible / I2 Medium / E2 Reasoned → Plan`. The budget's target is the original's `a*b` (`perf-original`); faster paths with the same results are allowed but not required. Speed alone is not I3 (catalog).
- `Point: inside a larger expression the compiler saves its live registers around a library function (savem), around the operator not: a few cycles more than the operator there — L3 Likely / I1 Low / E2 Reasoned → Monitor` (named in the docs; the simple assignment `seed=LMulBB2(seed,69069)+1` has none).
- `Point: the library test compares with lmullib's [orig] blocks (same vectors, the new tokens) rather than recording the new library against the CD — the CD has no such library — L1 Unlikely / I1 Low / E2 Reasoned → Monitor`. The new capture's expected data is the port's output (`libs-update`), and a check of the test asserts that each block's checksum equals the `[orig]` checksum of the same block of `libtest_lmullib`.
- `Point: Quick arguments of Long values — QMulBB2(300,300) converts the Long constants to Quick as any Quick parameter does — L1 Unlikely / I1 Low / E2 Reasoned → Monitor`.

### Order

Not in the eval.c line (no compiler file). No shared file with `console-break-option` (it adds no `acidlibs` library any more).

## Risks / Trade-offs

- [A source that uses `LMulBB2` as a variable or label] → tokenised as the function after the library is added. No hit in AmiBlitz3's sources and the port's libraries for the names.
- [Not available with the CD's libraries or in AB3 mode] → "Token Not Found"; the OldMul{} text stays for them (the proposal's planning choice).
