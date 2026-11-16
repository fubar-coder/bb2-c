## Why

CLAUDE.md states the goal as functional BB2 compatibility; identical
executables are not a goal. Optimizations (`compiler-intrinsics`, later
`compiler-function-optimizer`) change the port's executables without fixing a
bug. The comparisons with the original demand an issue for every changed
executable today:

- the CD result table `tests/data/blitz2-cd.tsv` needs a `#N: ... (2.10: HASH)`
  note per row whose hash differs from the original's;
- `corpus_originals` and `corpus_runerr_originals` need an executable pair
  `NAME.orig.exe` / `NAME.fixed-N.exe`, and only one `fixed-N` per program;
- `ted_ab_originals` compares the executables TED's compile makes with the
  original compiler's.

An optimization therefore needs an issue and an expect pair per changed
program, or hundreds of them. The user decided on 2026-11-16: against the
original, only outputs and errors are compared.

## What Changes

- **Against the original, an executable's hash is information.** It is shown
  in the reports and never fails a test. What is compared: the output of a
  program, the serial data, whether a source compiles, the error line and
  message of one that does not, the files TED writes, the library results.
- **Corpus:** the observation `exe` and the files `NAME.orig.exe` /
  `NAME.fixed-N.exe` go; the configuration `port` of `*_originals` reports a
  differing executable and judges the output. The configuration `prev`
  stays byte exact (`PREV_INTENDED`). The two existing pairs (`t130_blit`)
  are removed.
- **CD table:** `corpus_cd` compares result, error line and message; size and
  hash of an `ok` row are reported, not compared. The table keeps the
  original's size and hash as reference data; `blitz2-cd-update` does not
  overwrite them with the port's. A `#N:` note stays for a fix that changes the
  compile result (an error the port now reports, an error line).
- **ted_ab_originals:** the executables of the combinations with the port's
  compiler (`@expect corpus`, executable files in a pair comparison against
  a combination with the original compiler) are reported when they differ;
  the other expectations and pairs stay strict.
- The comparison with the `prev` build stays byte-exact; an intended change
  is named by `PREV_INTENDED`.

Out of scope: program code (blitz2, libs); the comparison of library results
and TED files with the original; the `prev` comparisons.

## Impact

Specs `testing/original-comparisons` (new requirement), `testing/expected-results`
(the corpus pair and the CD table rows), `testing/bug-fix-workflow` (what a fix
commit names). `compiler-intrinsics` changes 28 CD rows and `t21_val`'s
executable without a note or a pair after this change.
