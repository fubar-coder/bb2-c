# Design

## Context

The format of the cases and of the block cases is design D4/D6 of the
archived change `2026-11-12-libs-tests-capture`; the judge is D3
(`tools/lib/libscmp`, `tools/results`), the record and update targets D5.
Today `capture.c` builds the key, the value text and the FNV-1a of every
block observation on the 68000. The measurement (report
`libs-capture-host-hash.md` of 2026-11-12, throwaway diff and converter)
showed that the formatting and hashing, not the file I/O, is the cost:
writing raw records at 5 MB/s through `TEST:` halves the two block
programs, and a host converter reproduces the committed `BLOCK:N` lines
byte for byte.

## Goals / Non-Goals

**Goals:**
- the same `NAME.txt` content for the judge as today (the merged text is
  byte-identical to what the old writer wrote), so compare, accept
  (identity by bytes, the run's order) and update stay as they are;
- the host checks the raw file strictly (broken input fails, never a
  silent wrong hash);
- diagnostics from the raw file: block listing, first differing
  observation between two runs, `lost` inside a block, the block dump.

**Non-Goals:**
- no change of the plain-case path (all programs outside block mode);
- no change of the expected data or of the set of block programs;
- no compression of the raw file (it is deleted after a pass instead).

## Decisions

### H1. The raw file `NAME.bin`

Written by `capture.c` only in block mode, through its own static 64 KB
buffer (the measurement: a larger buffer gains nothing). Text header line
`# capture-bin NAME 1\n`, then records; numbers big-endian; check and
vector raw (unescaped, NULL as empty), each ended by a 0 byte:

| type | record |
|---|---|
| `$01` | routine: the escaped routine part, 0-ended (escaped once per `cap_routine`, as today) |
| `$02` | end of block mode: a partial block is closed (`cap_blocks(0)`, `cap_end`) |
| `$10` | number: CHECK, VEC, u32 |
| `$11` | bytes: CHECK, VEC, u32 N, N bytes (all bytes; the host decides hex or fnv) |
| `$13` | text: CHECK, VEC, u8 0 (NULL) or u8 1 and the text, 0-ended |
| `$14` | address: CHECK, VEC, u8 kind: 0 `prog`; 1 u32 (plain); 2 u32 OFF, NAME (`NAME+$OFF`); 3 the same, `NAME-$OFF` |
| `$15` | registers: CHECK, VEC, u16 lost set (bits 0-14) |

The `prog` test of `cap_ptr` and the pointer normalisation of
`cap_ptrbytes` stay on the Amiga (they need the test program's hunks).
Writing check and vector with every record needs no table on the Amiga
(a dictionary was measured and dropped: systest_med has more than 40 000
distinct triples).

Alternative: keep text but speed up the Amiga side (fewer `emit` calls,
cached escaped keys). Rejected by the user decision: the measurement
puts its gain below half of the host variant's.

### H2. The block places in `NAME.txt`

The Amiga still counts the block observations (an increment) and writes,
where today's writer wrote `BLOCK:N<TAB>COUNT HASH first=KEY`, the line
`BLOCK:N<TAB>COUNT in NAME.bin`. So the position of each block case among
the plain cases (`not captured` in block mode, the cases before and
after) is kept exactly; accept writes the run's order and must reproduce
the committed files byte for byte.

Alternative: append the block cases at the end of the output. Rejected:
the committed order would change on the next record.

### H3. The merge in the judge

`libscmp` loads a run's `NAME.txt`; if it has block places or a
`NAME.bin` exists, it converts `NAME.bin` (an iterator over the
observations that formats key and value as `capture.c` did and hashes
`KEY<TAB>VALUE<LF>`) and replaces each place by the block case, checking
that N and COUNT agree, before parsing. A missing `.bin`, a truncated
record, an unknown type, a length beyond the end, a bad kind or flag, a
key longer than the writer's 1023 bytes, or blocks without places (or
places without blocks) make the output malformed (exit status 2, the file
and the byte offset named). The raw data stay with the parsed run for
the diagnostics. Conversion cost: under a second per program.

### H4. Diagnostics

- `libs-compare`: for the first three differing (or added) block cases of
  a program, the block's observation lines go to
  `RUN_DIR/NAME.block-N.obs` and the report names the file and shows its
  first lines. `--other DIR`: a second run (another set or another
  record run) whose `NAME.bin` is read too; for each listed block the
  report names the first observation in which the two differ, with both
  values (or the one that only one run has).
- `libs-accept`: when the two runs' merged texts differ in a block case,
  the first differing observation of that block is named under "test not
  deterministic"; every observation `lost` in a block is refused like a
  plain `lost` case (key and value named).
- `results libs-dump RUN_DIR KEY|BLOCK:N [--only NAME]`: from each
  `NAME.bin`, the block's observation lines, or for the key of a bytes
  observation its bytes as `KEY<TAB>at $OFFS HEX` (32 a line), appended
  to `RUN_DIR/NAME.dump` - the format `capture.c` wrote. `capture.c`
  keeps the dump of a plain bytes case (`LIBS_CAP_DUMP=KEY`) and drops
  its block dump. `emulator.cmake` passes a `BLOCK:N` no longer to the
  Amiga, runs `libs-dump` after the run for `CAP_DUMP` (also a key, for
  block observations) and shows the dump as before; the names `CAP_DUMP`
  and `LIBS_CAP_DUMP` stay.

### H5. Deleting the raw files

`emulator.cmake` with `EXPECTED` deletes `captures/*.bin` when the run
passed (compare, dump and result all well); `expected.cmake` deletes the
runs' `.bin` after a written accept or update. A failure keeps them.
`-DKEEP_BIN=ON` or the environment variable `LIBS_KEEP_BIN` keeps them
(for `libs-compare --other` with a passing run). The next run of the same
test removes its work directory anyway.

## Findings of the implementation

- `NAME.bin` is opened by `cap_blocks(1)` and must be closed by
  `cap_blocks(0)`, not by `cap_end()`: the first `libs_emulator` run with
  the file open until `cap_end()` failed `ridisasmlib: all memory back`
  (56 bytes: the DOS file handle, allocated inside the program's memory
  balance window). So a program has one block mode; a second
  `cap_blocks(1)` is a fault ("block mode a second time"). All four block
  programs have one.
- A key of a block observation longer than 1023 bytes was a fault of the
  writer; it is now found by the judge (malformed `NAME.bin`), as the
  writer no longer builds the key.
- CMake cannot write a 0 byte, so the CMake-level tests
  (`libs_capture_target`) use a `NAME.bin` without records; the records
  are tested from memory (`test_libscmp_bin.c`, `test_capture_format`).

## Measurements (task 4.3)

Wall clock on an idle machine (load average 1.2-1.4 before and after
each run, no foreign compiler, ninja, ctest or Amiberry process), C build
(`LIBSET=obj`), the `libs_emulator` command with a filter, compare with
the expected data included; before: the throwaway measurement of
2026-11-12 (today's capture.c, same machine):

| | before | now |
|---|---|---|
| `systest_med` (`[med]`) | 53.5 s | 26.2 s, 26.5 s |
| `rwe_ridisasmlib` | 67.2 s | 29.4 s, 29.4 s |
| `libs_emulator` (all 60 programs) | about 150 s | 77.5 s, 75.8 s |

The raw files of one run: `systest_med.bin` 132.5 MB, `rwe_ridisasmlib.bin`
168.2 MB, each `libtest` 1.2 MB; deleted after the pass.

## Risks / Trade-offs

- [Two writers of one format: the host formatting must stay in lock step
  with `capture.c`'s text path] -> a host test runs every value format
  through `capture.c` in text and in block mode and checks that the
  judge's merge gives the same lines and hashes; the acceptance runs
  (`libs_emulator`, `libs_emulator_originals`, a record without filter
  writing nothing) prove it on the real data.
- [A program that crashes leaves a truncated `.bin`] -> the merge reports
  the offset; the run fails anyway (no `rc=`).
- [300 MB per run in the build directory while a test runs; kept on
  failure] -> deleted after a pass; documented.
- [The hash is no longer computed where the observation is made] -> none
  in practice: the raw values are what was hashed before.
