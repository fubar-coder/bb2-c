# Design

## Context

Observed (main at `cbef539c`):

- The pass file key `file=DEST|SRC[|TAG]` copies SRC to `CORPUS:DEST` for
  every configuration (`tests/corpus/runner/corpus.h`, `stage()` in
  `corpus.c:960-980`; a missing SRC is "cannot copy" and fails the pass).
  `corpus_pass()` (`tests/corpus/CMakeLists.txt:96-190`) writes the pass
  files and has no argument for it yet; every pass gets
  `mount=BB2|archive/cd/ultimate-blitz-basic-2.1` (`:157`).
  `skip_missing_expect` drops the sources whose text names `BB2:` when the
  directory is missing (`corpus.c:787-818`, only for passes with
  `expect_dir=`).
- `tests/bug-orig/CMakeLists.txt:397-414` builds `AREA/NAME.spec` to
  `testdata/NAME` with `${MKTESTDATA}` (defined in the top-level
  `CMakeLists.txt:76`, a host tool: `DEPENDS ... ${MKTESTDATA} host_tools`);
  the tests stage it with `; @pass file=DEST|@TESTDATA@/NAME`. Existing
  descriptions to copy from: `trackerlib-init-patternzahl.mod.spec` (M.K.
  with 31 empty samples), `audiolib-loadsound-fileopen.8svx.spec` (VHDR
  and BODY), `medlib-getmedvolume-clears.med.spec` (MMD1: header with
  labels, one block, one sample).
- What the loaders read: `LoadSound` through iffiolib: VHDR (one-shot and
  repeat counts, samples per second -> period 3579440/sps), BODY
  (`audiolib.c:308-380`); `LoadModule`: M.K., song length and positions,
  the highest pattern (`trackerlib`, `modlib.c`); the MED player: the MMD0
  header with `song`, `blockarr`, `smplarr`, `expdata` (0 is allowed:
  `medlib.c:654`), `song->numsamples` at +787, 3-byte notes in MMD0
  (`medlib.c:1160-1180`: byte 0 = note plus instrument bits 5-4 in bits
  7-6, byte 1 = instrument bits 3-0 in the high nibble and the command in
  the low one, byte 2 = command data), `JumpMed n` = song position n-1
  (`medlib.c:1887`). In MMD0 a block starts with `numtracks`, `lines - 1`
  as bytes (`medlib.c:1147`).
- The recorded outputs of the CD versions: `t110_audio.out` lines 15-18
  (`bounce`: length 7 words, repeat 14, replen 0 = one-shot only;
  `hitwall`: length 91, repeat 152, replen 14), `t111_tracker.out` (7646
  bytes, `M.K.`, song length 8, restart 127), `t112_med.out` (47812 bytes;
  eight notes per 48 pulses on four channels, a `0C` command row).

## Decisions

### D1 The data: descriptions in `tests/corpus/data/`, staged by `file=`

`tests/corpus/data/NAME.spec` -> build directory `tests/corpus/data/NAME`
(the way `bug-orig` does it, one custom command per file, `DEPENDS` on the
spec and the tool, target `corpus_data` in `ALL` and a dependency of
`corpus_sources`). `corpus_pass()` gets an argument `DATA` (the files to
stage); `corpus` and `corpus_runerr` (the passes whose sources include
`t110` to `t112`) write `file=data/NAME|BUILD/NAME` lines; the other passes
stage nothing. The programs read `CORPUS:data/NAME`. Names begin with the ID
of the program that uses them, as in bug-orig: `t110_bounce.8svx`,
`t110_hitwall.8svx`, `t111_song.mod`, `t112_song.med`. The `mount=BB2|...`
line is removed from `corpus_pass()`; the runner's left-out logic
(`corpus_skip_missing`, its tests in `test_corpus.c`) stays: it is generic
and not part of this change.

The staged path is not part of any output: the programs print no file
names, so the `.out` files do not depend on it. The volume `CORPUS:` is
writable and the files are a few KB per configuration.

### D2 What each module needs

The values of the CD files are not reproduced; the description gives the
same shape with different numbers. Needs, from the programs:

`t110_audio` (`bounce`, `hitwall`): 8SVX with VHDR (oneshot, repeat,
samples per cycle, samples per second, octaves 1, no compression, volume
`$10000`) and BODY. `bounce`: one-shot only (repeat 0), a few bytes, a
rate of 8000 to 12000 samples/s (the period computation overflows below
55 Hz, `audiolib.c:348-350`; that is not what this test is for). `hitwall`: a one-shot part and a repeat part (the second
`LoadSound 5` then replaces the first and checks that the Sound fields and
the checksum follow). Data bytes not zero, not constant (a ramp or a short
triangle written out with `b`/`w` lines).

`t111_tracker` (`song.mod`): `M.K.`; 20-byte name; 31 sample headers of
which at least three have a length > 0 (volume 64, finetune 0, repeat
offset 0, repeat length 1 word = none) and the rest zero; song length 8,
restart 127; position table 128 bytes with highest pattern number 1 at a
position below 126 (not at 126: that is `#289`, tested by
`trackerlib-init-patternzahl`); the table's highest pattern decides where
`silence{}` starts clearing, so with two patterns it starts at
1084 + 2 x 1024; two patterns (2048 bytes, notes in the first rows, the
rest `fill`); sample data behind them (a few hundred non-zero bytes, so
the checksum before `silence` differs from the one after it). Avoid
sample numbers above 31, repeat words and volume commands with the known
bugs (`docs/libs/cmdlibs-port.md:264`): the file is for what `t111`
checks, the bugs have their own tests. The program prints length and
checksums only; `mt_music` runs in the vertical-blank interrupt between
`PlayModule` and `StopModule` without effect on the output (the samples
are silent after `silence{}`).

`t112_med` (`song.med`): MMD0 (`'MMD0'`), `modlen` = `@end`, `song`,
`blockarr`, `smplarr` as labels, `expdata` 0, `pstate` 0. Song: 4 samples
(`svol` 64, 46, 33 and one unused slot with a null pointer in `smplarr` so
that `silence{}`'s `If p` has both branches; `numsamples` = 4 at song +
787), instrument struct 8 bytes each, 63 slots, `numblocks` 4, `songlen`
6, `playseq` 0 1 2 3 1 0, `deftempo` 125, `tempo2` 6, track volumes 64,
master volume 64. Blocks: 4 tracks, 32 lines each (`numtracks` 4,
`lines - 1` = 31): 4 x (2 + 32 x 12) bytes. Notes: in every block, notes
on all four tracks in the first twelve lines (so the phases after
`JumpMed 3` -> position 2 -> block 2, with `SetMedVolume 32` and with
`SetMedMask 5`, all print notes; mask 5 = tracks 0 and 2 only, so the
other two tracks must have notes too, to show that they disappear), two
different instruments per block, one `0C` (set volume) command row like
the CD's `11/0C/2E`, and no speed or break command, so the block change
time is known: 400 pulses at 6 per line is 66 lines, crossing block 0 and
block 1 of 32 lines before the program prints `pos block`. Samples: type 0, length 64 to 128 bytes (non-zero data,
so `silence{}` changes the checksum). The file is a few KB; `Dim
mm.l(l/4+2)` and `ReadMem` in the program follow the real size.

`modlen` and the label offsets are computed by `mktestdata`; the module
is relocated by `StartMedModule` ("MMD0" -> "RMD0"); `DecodeMedModule`
gets a RAM copy, so it relocates the copy and leaves the first.

### D3 No extension of mktestdata

The format has `b w l s fill align`, labels with forward references and
IFF structure; every field above is expressible. The sizes are small (the
blocks' empty lines are `fill N 0`; a block has about ten `b` lines for
its note rows). A `rep N` ... `endrep` would shorten the descriptions but
is not needed; if a description turns out unreadable (more than about 150
lines), add it then, with tests of broken input (unclosed `rep`, a label
inside) in `tools/tests/test_mktestdata.c`. This change adds no tool
code, and nothing in the executables the compilers write or the library
files.

### D4 The CD variants do not stay in `originals`

After the change the real files (`bounce`, `hitwall`, `hhmusic.mus`,
`lambada.med`) are not loaded by any corpus pass. The comparison with the
original is kept for the library code: `ref` runs the original libraries
on the generated files and every configuration is judged against the same
recorded output. What the real files add is wider data (a 47 KB MMD0
module with commands and several sample types, a module with 31 sample
slots in use): the MED player and tracker library are compared with the
original per pulse on generated MMD0/MMD1 modules with synth, hybrid and
octave instruments in `libs/tests/systest_med.c`
(`docs/libs/cmdlibs-port.md:268`) and `libs_emulator_originals` runs those
against the CD's libraries. Keeping a second set of three programs on CD
data would double the corpus programs for little (and a second recording
per program). Recommendation: do not keep them; asked in
`questions-for-user.md` because it gives up a check on real files.

### D5 Recording

The three programs change, so their `.out` (in `expect/corpus` and
`expect/corpus_runerr`) change: `corpus-expect-record` with
`CORPUS_RECORD_FILTER=corpus:t110_audio,...` for both passes (the target
runs the original twice with different layouts; a program that differs is
"not deterministic" and must be fixed in the program, not accepted).
These are new unqualified files for programs that exist, not a fix:
`corpus-expect-update` is not used, there is no `Fixes` line and no
`[orig]`/`[fixed #N]` pair; the commit names the data change as the reason
for the new content. The CD is needed once:
in a worktree `archive/cd` is not present (it is ignored); use the
checkout's directory (`cmake/fetch-originals.cmake` or a symlink) for
the recording only.

### D6 What stays unchecked

`t111_tracker` clears the samples before `PlayModule`, so the checksum
after `mt_init` equals the one before it whatever `mt_init` clears: the
program checks that nothing else changes the module. The CD version had
the same property; this change keeps the program logic.

### D7 The check

`tests/corpus/check-data.cmake` (`cmake -DROOT=... -P`): collect the
strings `CORPUS:data/NAME` in `tests/corpus/tests*/*.bas` (case as
written) and the files `tests/corpus/data/*.spec` (name without
`.spec`); fail on a name in only one of the sets, naming the file or the
source. Run at configure time (like `bug_orig_fixed_orig_check`) and by
two ctests with `PASS_REGULAR_EXPRESSION` on a broken tree under
`tests/corpus/selftest/` (a source naming a missing file; a description
no source names) and one on the correct tree.

## Risks / Trade-offs

- A generated module the library rejects or plays differently than the
  CD's changes nothing for the original and the port equally, but the
  test then checks little. Mitigation: the review of the recorded `.out`
  against the requirement "the generated modules give the player programs
  something to check" (task 4.2).
- The three programs' expected output changes completely; reviewed as new
  recordings, not as differences.
- Without the CD the recording cannot be redone; whoever changes a
  description records again with the CD (as for every corpus program).
