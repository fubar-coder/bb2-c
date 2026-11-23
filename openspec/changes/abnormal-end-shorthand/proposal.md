# Proposal

## Why

An original whose faulty part corrupts memory has no defined end, and its
test declares the general abnormal end (tests/README.md, "An original that
corrupts memory"). Because a query that no `[ends]` alternative names keeps
its default, that end needs four alternatives (`abnormal`,
`abnormal_after_case`, `abnormal_after_run`, `abnormal_run_only`), each
repeating `session != done`, `rc not exists` and `requester count >= 0`.
58 tests carry the block.

The user chose a built-in alternative that the runner expands (2026-11-23,
`~/.local/share/bb2-c-work/decisions/archive/2026-11-23-tr/decisions.md`, "O1-O3 of the
handoff answered", quote "(a)" to option (a) of O1 in
`open-questions.md` there).

## What Changes

- An `[ends."SELECTOR"]` table may declare `abnormal = ["any"]`: the
  general abnormal end, which the runner expands to the four alternatives
  of today with their names and assertions. Further elements of the list
  (`"check.NS.NAME != ok"`, `output`, `run.*` assertions) are added to each
  of the four.
- Refused, naming file and line: the word `any` anywhere but as the first
  element of `abnormal`, another bare word in its place, a string value
  `abnormal = "any"`, an extra element that names `session`, `rc`,
  `requester`, `end.case` or `end.run`, and the keys `abnormal_after_case`,
  `abnormal_after_run`, `abnormal_run_only` next to the built-in in the
  same table.
- No existing `[ends]` table changes its meaning: a list without the word
  `any` stays an ordinary named alternative (also under the key
  `abnormal`), and an element without a blank was refused before.
- The 58 tests are rewritten to the short form; their verdicts do not
  change. tests/README.md ("Alternative ends", "An original that corrupts
  memory") and the guide show the short form.

## Capabilities

### Modified Capabilities

- `testing/basic-test-runner`: a requirement on the built-in general
  abnormal end is added.

## Impact

- `tools/lib/testconf` (the validation and the expansion of the
  alternatives), its host test `tools/tests/test_testassert.cpp`.
- 58 `ID.test.toml` files under `tests/`.
- `tests/README.md`.
- Results (rule 3): none; no executable, TED file or library file changes.
