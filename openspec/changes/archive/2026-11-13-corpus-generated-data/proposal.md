# Proposal

## Why

`ctest --preset all` is meant to run every test that does not read the
CD's source files (`openspec/specs/testing/original-comparisons/spec.md`,
scenario "all without the CD"). Three corpus programs do not meet that:
they load their data from the CD volume `BB2:`
(`tests/corpus/CMakeLists.txt:157`, `mount=BB2|archive/cd/...`), and
`corpus_skip_missing` (`tests/corpus/runner/corpus.c:773-818`) leaves a
source out of the passes `corpus` and `corpus_runerr` when the directory is
missing (`tests/README.md:455-456`):

| program | loads | what it checks |
|---|---|---|
| `t110_audio.bas:96-100` | `LoadSound` of two 8SVX files (`bounce`, `hitwall`) | the Sound fields (period, length, repeat), a checksum of the data |
| `t111_tracker.bas:26` | `LoadModule` of a NoiseTracker module (`hhmusic.mus`, 7646 bytes) | id, song length, restart, checksums before and after `PlayModule`, freeing by reloading |
| `t112_med.bas:36,81` | `LoadMedModule` and `ReadFile` of an MMD0 module (`lambada.med`, 47812 bytes) | relocation `MMD0` to `RMD0`, 400 + 3 x 60 + 120 player pulses with the notes, instruments and volumes per row, `JumpMed`, `SetMedVolume`, `SetMedMask`, `DecodeMedModule` |

So `LoadSound`, `LoadModule` and the MED player are not exercised by
`all` on a machine without the CD, although the expected outputs are
committed (`tests/corpus/expect/corpus/t11[012]*.out` and the same in
`corpus_runerr`). The tool `mktestdata` (`tools/lib/mktestdata`) already
builds 8SVX, tracker and MMD0/MMD1 files from text descriptions in the
build; the bug-orig tests use it (`tests/bug-orig/libs/*.8svx.spec`,
`*.mod.spec`, `*.med.spec`, staged with `; @pass file=DEST|@TESTDATA@/NAME`).
The corpus can use the same mechanism: its pass files already have the key
`file=DEST|SRC` (`tests/corpus/runner/corpus.h`, staged in `stage()`,
`corpus.c:960-980`, to the volume `CORPUS:`).

## What Changes

- Text descriptions `tests/corpus/data/*.spec` of five files (two 8SVX,
  one NoiseTracker module, one MMD0 module; the MED module serves both
  `LoadMedModule` and `ReadFile`), built in the build with `mktestdata`
  and staged by the passes `corpus` and `corpus_runerr` as
  `CORPUS:data/NAME` (key `file=`).
- `t110_audio`, `t111_tracker` and `t112_med` load from `CORPUS:data/...`
  instead of `BB2:...`; nothing else in their logic changes. The
  `mount=BB2|...` line of every corpus pass goes; the programs are no
  longer left out without the CD.
- The expected outputs of the three programs are recorded again once from
  the original (`corpus-expect-record`, needs the CD once) in both passes;
  from then on `all` checks them without the CD, and `_originals` checks
  `ref`, `port` and `orig-clib` against the same files.
- A configure-time check: every `CORPUS:data/NAME` literal in the corpus
  sources names a described file, and every description is used by a
  source (with a ctest on broken input, like `check-fixed-orig.cmake`).
- Documents (`tests/README.md`, `docs/testing.md`,
  `docs/libs/cmdlibs-port.md`, `TODO.md`) say that the corpus runs
  without the CD.

The CD-sourced variants do not stay in `originals` (design D4).

## Out of scope

- The TED A/B scripts with CD sources (`@source K ...`); `corpus_cd` and
  `corpus_cd_ab3` stay CD-only.
- Changes to the libraries or the compiler: the executables the compilers
  generate and all library files stay byte-identical (acceptance rule of
  `CLAUDE.md`); the committed expected outputs of other programs do not
  change.
- Strengthening what `t111_tracker` checks (design D6).

## Capabilities

### Modified Capabilities
- `testing/expected-results`: corpus programs that load sounds and
  modules get their data from the build, so the passes of `all` run all of
  them without the CD.

## Impact

`tests/corpus/CMakeLists.txt`, `tests/corpus/data/` (new),
`tests/corpus/tests/t110_audio.bas`, `t111_tracker.bas`, `t112_med.bas`,
`tests/corpus/expect/corpus*/t11[012]*.out`, a check script with its
ctest, and the documents above. No change to `tools/` or `programs/`
(design D3). The run time of the passes does not grow beyond the three
programs' sessions, which ran with the CD before.
