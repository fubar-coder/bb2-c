# Proposal

## Why

`blitz2-ab3` (change `ab3-mode-g1`, archived 2026-11-01) is built from
blitz2's chunk sources compiled a second time with `BLITZ2_AB3`, with 36
`#if BLITZ2_AB3` blocks spread over ten of them. Phase G2 would add
about ten more deep in the parser. The user wants `blitz2-ab3` to be an
extension of `blitz2` that reuses as much of it as possible, without
many `#if` switches; both programs stay (the internal structure may
become clearer with later reworks). Since the rule of 2026-11-01
(CLAUDE.md "Acceptance") the programs themselves need no longer be
byte-identical – only their results – so the shared code can be built
once and extended through real functions.

## What Changes

- The chunk sources (`programs/blitz2/src/*.c`, the translation of
  blitz2.s) are compiled **once**, without any define, into an object
  library that both programs link.
- The places where AB3 mode differs become **extension points**: plain
  functions declared in one header, called by the chunk code. `blitz2`
  links a small module with their BB2 behaviour (mode BB2, nothing
  handled); `blitz2-ab3` links the AB3 modules (`ab3*.c`) instead.
- No `#if BLITZ2_AB3` is left anywhere in the sources; a check in the
  build keeps it so.
- A rule for G2 and later: new AB3 behaviour is added behind an
  extension point in `ab3*.c`, not in the chunk code; documented in
  README and `docs/ab3/variant.md`.
- `blitz2` changes as a program (calls of the extension points); its
  results (generated executables) stay byte-identical. `blitz2-ab3`'s
  behaviour is unchanged.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

- `ab3-compat/bb2-equivalence`: "The default blitz2 is unchanged" now
  means its results, not its program file (rule of 2026-11-01).

## Impact

- `programs/blitz2/src/` (chunk sources, `main.h`, `compile.h`, a new
  extension header, the BB2 extension module, `ab3*.c`) and
  `programs/blitz2/CMakeLists.txt` (object library, the two programs).
- README ("blitz2-ab3"), `docs/ab3/variant.md` (structure, rule).
- Not affected: `ab3cli`, TED, the libraries, the tests' expectations.
