# Proposal

## Why

The port is complete (TODO.md section 3) and the project now fixes
original bugs (862 open `bug-orig` issues, fixed in rounds by area). Many
tests were written to prove that the port equals the originals: the
bug-orig runner needs a reference session of the original for every test,
the corpus compares with the CD's compiler and libraries, `libs_emulator`
compares every library with the CD's binary, the TED oracle hashes and the
CD result table hold the originals' output. Every fix makes one of these
comparisons fail by design, and the only way out so far was editing the
test around the fix (corpus `t30_date.bas`, commit `fd76b11`, #120). A
fixed bug-orig test is also run by no normal preset: the label `bug-orig`
keeps it out of `all`, so ~20 fixes are guarded by nothing anybody runs.

User decisions of 2026-11-07: the goal is BB2 compatibility with fixed
bugs; whoever wants the original behaviour uses the original compiler and
libraries (no bug-preserving variant); tests check expected behaviour,
not equality with the reference; "results byte-identical to the previous
commit unless intended" stays.

## What Changes

- **Fixed bug-orig tests** carry a header `; @fixed #NNN`, run in `all`
  (label `emulator`) without a reference session and must come out FIXED.
- **Recorded original observation**: the original's observation of each
  open bug-orig test is committed once as `ID.orig`; the normal run judges
  `port`/`clib` against `ID.expect` and `ID.orig`. The reference session
  only runs to record, under the new label `originals`.
- **`libs_emulator`**: per-check `expect_fixed(NNN, value)` replaces
  "equal to the original" by the correct value for that check.
- **Corpus**: expected outputs committed (`tests/corpus/expect/NAME.out`,
  about 200 files); the runner compares every configuration with them.
- **`prev` build**: a build of the previous commit (`-DBLITZ2_PREV=...`)
  and a ctest that compares every result byte by byte; the corpus
  executable comparison and TED A/B combos `P5`/`Pc` use it.
- **Label `originals`** for every test that needs the CD or the reference
  build; `oracle` stays AmiBlitz3's. `all` runs without the CD.
- **Override rows** with the issue number in the TED oracle hashes and
  the CD result table, kept by their generators.
- **Intended lists** for `checkdata`, `rwe_declarations_NAME`, `asmcmp`
  routines and a `; @fixed #NNN PAIR` directive for `ted_ab`, each at the
  first fix of its area.
- **Rules rewritten**: CLAUDE.md "Do not fix original bugs" becomes the
  fix workflow of the bug-fix phase; the same in
  `.claude/agents/implementer.md` and `tester.md`, `tests/README.md`,
  `docs/development.md`, `README.md`.

## Capabilities

### New Capabilities

- `testing/bug-orig`: verdicts, `; @fixed`, the recorded `ID.orig`, the
  labels `bug-orig`/`bug-port`/`originals`.
- `testing/expected-results`: expected data committed with override rows
  (corpus outputs, CD table, TED oracle hashes, `expect_fixed`).
- `testing/previous-commit`: the `prev` build and what is compared byte
  by byte.
- `testing/original-comparisons`: what still compares with the originals
  and why, and how fixes are allowed to differ there.
- `testing/bug-fix-workflow`: what every fix of an original bug delivers.

### Modified Capabilities

- `ab3-compat/bb2-equivalence`: the "previous commit" of the default
  build is the `prev` build.

## Impact

- `tests/corpus/runner` (`bugorig.c`, `bugorig.h`, `corpus.h`, a new
  `expect_dir=` mode), `tests/bug-orig/CMakeLists.txt`, `tests/blitz2`,
  `libs/tests` (`sysload.h`), `tests/ted-ab` / `tedab`, `tools/` (verdict,
  checkdata, blitzlibcompare callers, cdrun/`blitz2-cd-update`,
  `ted-oracle`), `CMakePresets.json`, `CMakeLists.txt`.
- About 350 new `.orig` files and about 200 new `tests/corpus/expect/`
  files, all generated once with the CD and reviewed.
- Documents: `CLAUDE.md`, `.claude/agents/implementer.md`, `tester.md`,
  `tests/README.md`, `docs/development.md`, `README.md`,
  `docs/libs/deviations.md`, `TODO.md`.
- Not covered here: the pending changes `test-coverage` (its
  `testing/test-selection` gets a delta once archived, see tasks 9.3),
  `blitz-coverage`, `library-reference`.
- No change of results: the programs, libraries and executables the
  compilers write stay byte-identical (checked by task 8's `prev` build,
  and by hand before it exists).
