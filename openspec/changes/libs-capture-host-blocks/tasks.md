# Tasks

## 1. The judge reads the raw block file

- [x] 1.1 `tools/lib/libscmp`: the observation iterator over `NAME.bin` (design H1, H3), the merge of the block places of `NAME.txt` on loading a run, the raw data kept with the run; verify with host tests from memory in `tools/tests` (every record type formatted as `capture.c` writes it, the block hash and `first=`, partial blocks, and broken input: a truncated record, an unknown type, a bad length, a missing `.bin`, places and blocks that do not fit, a key too long), clean also with `-DHOST_SANITIZE=ON`
- [x] 1.2 Diagnostics (design H4): `libs-compare` lists differing blocks into `NAME.block-N.obs` and with `--other DIR` names the first differing observation; `libs-accept` names the first differing observation of a non-deterministic block and refuses `lost` inside a block; `results libs-dump`; verify with host tests (memory and `results` on files made by `tools/tests/CMakeLists.txt`) and the `results` README/help tests

## 2. The writer

- [ ] 2.1 `libs/tests/capture.[ch]`: block mode writes `NAME.bin` and the block places (design H1, H2), no block dump; `test_capture_format` checks the raw records and, with the judge's merge, that block mode gives the same lines and hashes as the text path for every value format
- [ ] 2.2 `emulator.cmake`, `expected.cmake`, `libs/tests/CMakeLists.txt`: the host dump of `CAP_DUMP`, `.bin` deleted after a pass and kept on failure or with `KEEP_BIN`/`LIBS_KEEP_BIN` (design H4, H5); verify with `libs_capture_target` (the fake run writes a `.bin`), `libs_emulator_dump_*` and a block dump run (`LIBS_CAP_DUMP=BLOCK:N`)

## 3. Documentation

- [ ] 3.1 `docs/testing.md`, `tests/README.md`, `tools/results/README.md`, `site/content/docs/project-structure/library-test-cases.md`, the comments of `capture.h`, `libscmp.h`: blocks, the raw file, its deletion, the dump and the diagnostics (current state only); verify `readme_results` and a hugo build of the page

## 4. Acceptance

- [ ] 4.1 Clean builds without warnings (m68k, host, host with `-DHOST_SANITIZE=ON`), `ctest --preset host`, `ctest --preset all` with a prev build (results byte-identical, expected data unchanged: `libs_emulator` passes)
- [ ] 4.2 With the CD: `ctest --preset originals -R libs_emulator` passes and `libs-capture` without filter writes no file (0 written)
- [ ] 4.3 Times of `systest_med`, `rwe_ridisasmlib` and the whole `libs_emulator` on an idle machine, recorded in design.md
