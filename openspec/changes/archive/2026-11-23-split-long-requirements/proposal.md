## Why

`openspec validate bug-orig-basic-tests --strict` warns of two requirements
over 500 characters, and the requirement "Entries of the result file" of
`testing/basic-test-interface` lists three kinds of entries in one text.
Decisions of 2026-11-23
(`~/.local/share/bb2-c-work/decisions/rounds/tr/decisions.md`):

- "basic-test-local-data: dhandlerlib, timeout, order, spec split", Q7:
  "Split, own change (Recommended)" (a small change splits "Entries of the
  result file"; until then the ADDED requirement of `basic-test-local-data`
  stays).
- "The two long requirements of bug-orig-basic-tests into the split
  change": "Ja, die beiden zu langen Anforderungen in eine kleine spec
  change zum Aufteilen" (the small spec-only change splits both, without
  changing their meaning).

## What Changes

- `testing/basic-test-interface`: "Entries of the result file" keeps the
  block rule and its two scenarios (MODIFIED); the entries move to the
  ADDED requirements "Entries of a block", "Entries of a check" and
  "Entries of an order error". The entry lists are unchanged.
- The two long requirements of `bug-orig-basic-tests`, "The fixed issue is
  one key of three kinds" and "Recorded observation of the original", are
  split in that change's own delta spec (`testing/bug-orig`), not here;
  this change records it so the work has one place.

No code, test or result changes; no behaviour changes.

## Capabilities

### Modified Capabilities

- `testing/basic-test-interface`: the requirement of the entries split into four.

## Impact

Specs only. Relation to the open change `basic-test-local-data` (not
edited): it ADDs "Entries of a started program" (`run`, `runend`) to the same
capability under a new header, and this change MODIFIES and ADDs other
headers, so the archive order does not matter. The sentence "Besides the
entries of "Entries of the result file"" in that requirement stays
understandable (the entries of that requirement are now named by it and the
three new ones); `basic-test-local-data` may word it as "Besides the other
entries" when it is archived.
