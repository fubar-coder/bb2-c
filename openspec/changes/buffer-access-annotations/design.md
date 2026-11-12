# Design

## Context

Facts from the probe (`access-attribute-probe.md`, base `10bdb582`):
565 declarations in 182 files were annotated mechanically; 549 pairs the
heuristic found, about 15% wrong; the final patch built with two
warnings, both from tests that pass bad arguments on purpose; generated
code was byte-identical (Amiga objects and hunks, 788 library files,
host objects); no real bug found. About 15 functions already use fixed
array parameters (`char hex[65]`, `uint8_t digest[32]`, ...); GCC treats
them as implicit bounds and `-Warray-parameter` checks declaration against
definition. `HOST_SANITIZE` is set in `host/CMakeLists.txt:12-16`
(`-fsanitize=address,undefined`); no `_FORTIFY_SOURCE` is used anywhere.
With `-O2 -D_FORTIFY_SOURCE=3` GCC uses the `access` size in
`__builtin_dynamic_object_size` inside the annotated function, so the
code of host builds differs there (more `_chk` calls) - the one place the
annotations act at run time.

## Goals / Non-Goals

**Goals:** signatures that say which number is a length; compile-time
checks where sizes are constants; run-time checks in the sanitizer
configuration; a rule for new code.

**Non-Goals:** finding bugs by mass annotation (the probe found none);
annotating Blitz library entry points (register conventions, buffers from
registers, no call site with known sizes) or blitz2/TED internals (buffers
in globals, few candidates); a promise that run-time sizes are checked at
compile time (they are not); replacing ASan (it already catches actual
overflows on tested paths); `-fanalyzer` (did not use `access` in the
probe).

## Decisions

**D1. A macro in one small header.** `BL_ACCESS(mode, ...)` expands to
`__attribute__((access(mode, __VA_ARGS__)))` under
`__has_attribute(access)`, else to nothing. Freestanding (no libc), so
the Amiga-side `shared/` code and host code include the same file. The
location (a new `shared/include/` or beside an existing shared header) is
fixed at implementation; it must be on the include path of every user
without new include-path plumbing per library. Alternative: raw
`__attribute__` at every site - noisy and unguarded. Rejected.

**D2. The attribute goes on the declaration callers see** (the header).
On the definition alone it checks only callers in the same file. The
definition repeats it only where useful; otherwise the header suffices.

**D3. Fixed sizes stay array bounds.** `char buf[65]`; where the size is
a parameter and the buffer is always at least that large, a VLA parameter
or `[static N]` is used if it reads better than the macro (it also gets
`-Wvla-parameter` / `-Warray-parameter` consistency checks). Both forms on
one parameter are avoided.

**D4. Review by hand; the diff is input.** Each of the ~100 functions
of stage 1 is checked against its source: which pointer, which number,
which mode. The probe's diff (565 declarations) is a candidate list only.

**D5. Cases where an annotation is impossible or misleading** (from the
probe; they stay unannotated, with a one-line comment where a reader
could be misled):
- the number is not the length: chunk ID plus chunk length, a code size,
  a value to format, an index, an element size, the font size, a slot
  count, issue numbers;
- the limit belongs to the other pointer (`edstub_strcpy(d, s, max)`,
  `rc_copytitle`): annotate the destination only;
- the length is in bytes for a typed pointer (`inscop2(ULONG *list,
  ULONG len)`): `access` counts elements; only through a `void *` or byte
  pointer;
- element count and element size apart (`grow`, `push`, `first_of`): the
  byte size `n * size` cannot be expressed;
- the length is an in/out pointer (`size_t *len`, `uint8_t **out`): only
  the input capacity can be annotated, never the result;
- NULL is allowed with a nonzero count (`mklist(n, NULL)`): GCC warns
  with `-Wnonnull` though `access` implies no `nonnull`;
- 0-terminated input with an optional limit (`next_line(p, end, line,
  cap)`): only the output side;
- free-like functions (`globfree`, `freefile`): `none` is possible but
  checks nothing useful - annotate only if the reader gains;
- Blitz library entry points: see Non-Goals.

Mode choice matters: `read_write` on a fill-only buffer makes
`-Wmaybe-uninitialized` fire for every fresh `malloc` (`hdf_format`).

**D6. `_FORTIFY_SOURCE=3` in `HOST_SANITIZE` only**, together with an
optimisation level that enables it (it needs -O1 or higher; the host
build type must provide that, to be confirmed on the first build, and
`-U_FORTIFY_SOURCE` first if the toolchain predefines it). It is the one
place the annotations are active at run time. Alternative: a separate
preset - rejected by the user's decision (one configuration).

**D7. Deliberate bad-argument tests.** In order of preference: pass the
size through a `volatile` or a helper the compiler cannot see through;
pass a correctly sized array and a size the function refuses by another
route; a narrowly scoped `#pragma GCC diagnostic` push/pop around the
single call only if neither works (not a file-wide pragma).
`tokenizer_init(..., bk, 65537)` and `xtra_is_ab3_xtra(nullptr, 1200)` are
the two cases. If the annotation cannot coexist with the test's intent,
the test, not the contract, changes.

**D8. Test of the checks.** A ctest compiles a small C snippet that calls
an annotated function with an oversized length against a smaller array,
with the project's warning flags plus `-Werror`, and expects the compile
to fail with the diagnostic (a ctest running the compiler on the snippet
with `PASS_REGULAR_EXPRESSION` on `-Wstringop-overflow` is preferred over
`try_compile`, which runs at configure time only). A second snippet with a
correct call must compile. A fortify test: a host test built under
`HOST_SANITIZE` passes a run-time size larger than the buffer to an
annotated function and expects the `_chk` abort; it is skipped when the
sanitizer configuration is off.

**D9. The hardenings.** `static_assert`s on the `offsetof` layout of both
structures at `tokens1.c:785` (source and destination both hold 5 words
from `offset`; the exact expression is chosen from the structure
definitions); `put_value` returns before the loop when `size == 0`.

**D10. Stage 2** (about 320 functions: programs, libs helpers, tests): a
function is annotated when its file is touched for another reason; an
optional sweep may follow later as its own change. The same by-hand
review applies. Acceptance of this change covers stage 1 only.

## Risks / Trade-offs

- A wrong annotation warns falsely, or documents a wrong contract that no
  call site trips -> every pair reviewed by hand against the source; the
  probe's flags (`-Wstringop-overflow=4 -Wstringop-overread
  -Warray-bounds=2 -Wvla-parameter -Warray-parameter=2`) used once.
- New warnings under newer compilers -> clean builds are an acceptance
  criterion; fix at the call site, do not blanket-disable.
- `_FORTIFY_SOURCE=3` may abort a test on a latent over-read the
  annotation exposes -> that is a finding: fix the code, name it in the
  commit, draft an issue text for the coordinator.
- Ongoing cost: each new buffer+length function needs a correct
  annotation -> the rule is in CLAUDE.md; reviewers check it.
- `-Warray-bounds=2` known false positive under ASan
  (`teststats/junit.c:283`) -> not in the default flags; leave unless the
  sanitizer build warns.
- Little gain at compile time: most calls have run-time sizes; the value
  is documentation and the run-time check in the sanitizer build.

## Migration Plan

Header first, then stage 1 library by library (one commit per library
group, byte comparison with the `prev` build after each), then the
`HOST_SANITIZE` flag, the tests, the docs. No rollback needed: the macro
expands to nothing if removed.

## Open Questions

- Exact location of the macro header (resolved at implementation by the
  include paths of `shared/` and `tools/lib/`).
