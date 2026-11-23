## Context

A split changes no meaning: every sentence of the old text is found in
exactly one new requirement (the diff is in the commit's report). A
MODIFIED requirement must carry its whole new text; the new requirements
are ADDED.

## Decisions

- The block rule ("An entry SHALL belong to the block of the last `block`
  entry before it.") and the two scenarios stay in the MODIFIED header,
  which gains a sentence that points to the requirements holding the
  entries (and says that further requirements may add more, as
  `basic-test-local-data` does).
- `begin`, `block`, `blockend`, `incomplete`, `end` are the entries that
  frame the file and its blocks; the check entries and `require` and
  `expect-error` belong to checks; `broken` is the order error.
- The two long requirements of `bug-orig-basic-tests` are split in that
  change's delta (`specs/testing/bug-orig/spec.md`), since the delta is
  where they are defined; both changes validate `--strict`.
