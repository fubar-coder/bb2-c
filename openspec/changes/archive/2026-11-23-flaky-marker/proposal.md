## Why

`compiler-amigalib-a4a5` failed in a full run as the second cell of a
session after a snapshot restore (the original's cell: rc 20, no result
file) and passes alone; `isolate = ["compiler=orig195"]` is the
workaround, the cause is not analysed. The user (Q-A17,
`rounds/tr/decisions.md` in `~/.local/share/bb2-c-work/decisions/`, entry
"Answers to Q-A17 to Q-A22"): "Die spekulative gewählte Lösung ist OK.
Analyse machen wir später. Vielleicht ein flakey-Kennzeichen in der toml
um es wiederzufinden, ggf. mit Beschreibung?" Asked for the form (entry
"Flaky marker; named compile jobs (Q-A17, Q-A20, Q-A24)"): "flaky =
\"description\" (Recommended)" - a key `flaky = "DESCRIPTION"`
(description required) in a `.test.toml`; testreg gives the test the
ctest label `flaky`; no influence on the verdict; compiler-amigalib-a4a5
gets it.

## What Changes

- New key `flaky` of the run configuration (`tools/lib/testconf`): a
  non-empty string, valid only in a test's own `ID.test.toml`. The
  description is the observation of one test; in a `tests.toml` it would
  mark tests nobody saw fail and give them a description that is not
  theirs. An empty string and a value of another type are errors naming
  file and line; in a `tests.toml` the key is an error naming file, line
  and key, as the other keys of a single test.
- testreg adds the ctest label `flaky` to every registered test of such a
  source (both sides). The runner's verdict does not change.
- `tests/bug-orig/compiler/compiler-amigalib-a4a5.test.toml` gets the key.
- Docs: `tests/README.md` (the key table, "Writing a test source"),
  `tools/testreg/README.md`.

## Impact

testconf, testreg and their host tests (`test_testconf`, `testreg_tool`);
no result of the compilers or libraries changes.
