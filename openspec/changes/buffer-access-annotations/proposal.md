# Proposal

## Why

Many C functions in `shared/` and `tools/lib/` take a pointer and a number
next to it, and the signature does not say whether the number is the
buffer's length. The probe of 2026-11-12 (`access` attribute applied
mechanically to 565 declarations) showed the ambiguity: about 15% of the
mechanically found pointer+number pairs were not lengths at all (chunk
lengths such as `begin(form, vhdr, vlen, rate)`, values to print such as
`out(a, b, -999, c)`, code sizes such as `make_cov(p, size, bits)`, indices,
element sizes). With the rule "every real buffer+length pair is annotated",
a missing annotation tells the reader that the number is not a length.

A second purpose is protecting future code: GCC checks calls with sizes
known at compile time against the annotation (`-Wstringop-overflow`,
`-Wstringop-overread`, `-Wnonnull`), and in the host sanitizer
configuration `_FORTIFY_SOURCE=3` turns the annotations into run-time
checks inside the called function.

Limits, stated honestly: the probe found no real bug and no latent risk
(most annotated functions are called with sizes known only at run time,
which GCC cannot compare at compile time); ASan already catches actual
overflows on tested paths. The value is in the documentation and in
guarding code written later.

## What Changes

- New freestanding header with the macro `BL_ACCESS(mode, ptr_index,
  size_index)` (modes `read_only`, `write_only`, `read_write`, `none`),
  usable by the Amiga world (m68k gcc 16) and the host world, guarded with
  `__has_attribute(access)`.
- Stage 1: every public header of `shared/` and `tools/lib/` (about 100
  functions) is reviewed pair by pair by hand and annotated; fixed-size
  buffers keep array bounds (`char buf[65]`), VLA or `[static N]`
  parameters are used where they fit, and a short comment marks a number
  next to a pointer that is deliberately not a length. The probe's diff is
  input only, never applied mechanically.
- Stage 2 (the rest of the repository, about 320 further functions in
  `programs/`, `libs/` helpers, `tests/`, `tools/tests/`, `libs/tests/`):
  annotated when a file is touched anyway, plus optionally a later sweep.
  Not part of the acceptance of this change.
- `_FORTIFY_SOURCE=3` is added to the `HOST_SANITIZE` configuration (host
  side only; the m68k libnix has no `_chk` functions).
- Rule in `CLAUDE.md` and `docs/development.md`: new code with a
  buffer+length pair is annotated.
- Two tests that pass bad arguments on purpose
  (`tools/tests/test_tokenizer.cpp:230`,
  `tools/tests/test_xtra_ab3_binary.cpp:118`) are reworked so that the
  build stays free of warnings, without a blanket pragma if avoidable.
- Tests of the new checks: a ctest that compiles a snippet with a wrongly
  sized call and expects the diagnostic.
- Two small hardenings: a `static_assert` on `offsetof` for the 10-byte
  `memcpy` across fields at `programs/blitz2/src/tokens1.c:785`;
  `put_value` in `tools/lib/teststats/statline.c:30` no longer writes
  `dst[0]` when `size` is 0.

## Capabilities

### New Capabilities
- `tooling/buffer-access-annotations`: how buffer+length pairs are
  declared and checked: the macro, the annotation rule, the compile-time
  and host run-time checks, and their tests.

### Modified Capabilities

None.

## Impact

- New header (location decided in design.md); edits to the public headers
  of `shared/` and `tools/lib/`, with matching definitions where the
  array-parameter form is used.
- `host/CMakeLists.txt` (`HOST_SANITIZE` branch), a new ctest for the
  compile-fail snippet, the two tests named above, `tokens1.c`,
  `statline.c`.
- `CLAUDE.md`, `docs/development.md`.
- Results: generated code stays byte-identical (`prev_results`); the
  probe showed that annotations alone change no code. Only host code built
  with `_FORTIFY_SOURCE=3` (sanitizer configuration, not a result of the
  build) differs.
- Planned only; implementation is postponed by the user's decision of
  2026-11-12 (`TODO.md`).
