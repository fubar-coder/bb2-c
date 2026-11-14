## Why

The libraries are compiled with `-ffixed-a3/a4/a5` (`cmake/BlitzLibrary.cmake:90-91`):
a3 is the Blitz string pointer, a4 the base of the locals, a5 the globals.
gcc does not save or restore a fixed register around an `__asm` block, so
an NDK inline call whose parameter sits in a3 or a4 returns with the
register changed. 35 NDK functions in 12 headers take a parameter in a3
(5 of them also in a4); the NDK protects only its a5 parameters (with
`exg d7,a5`) and, with `-fbaserel`, a4, which the libraries do not use
(audit `analyse-ndk-a345.md` of 2026-11-14, section 1;
`docs/libs/cmdlibs-port.md` "OS calls with a parameter in a3, a4 or a5",
commit `df718d77`). Nothing in the build stops a library from calling one
raw. Seven such call sites exist now (audit sections 2-4): four in gtlib
(#328, fixed in round RP by `KEEP_A3`), `EZRequest` in rimisclib (new
bug, #1027), `AddAppIcon` in riapplib (#516), `AutoRequest` in rundebuglib
(not observable). Two further calls restore the register by hand
(rilvcallhooklib through its thunk, riwbspritelib through its own `__asm`,
#1028).

A second, smaller finding: `-fno-toplevel-reorder` keeps every unused
`static __stdargs ... __X_va` varargs wrapper of the NDK's inline headers,
so each library that includes a `proto/` header carries all wrappers of
that header as dead code, about 11 KB over 49 libraries (audit, "Side
finding").

## What Changes

- **Shadow headers.** `libs/include/ndk/inline/*.h`, one for every NDK
  inline header the libraries include, found before the NDK in the include
  path of `blitz_library` (libraries only; blitz2 and TED are not
  affected). They are generated as text by CMake.
- **Varargs wrappers inline.** Each shadow header includes the NDK header
  with `__stdargs` defined as `inline __attribute__((__stkparm__))`, so an
  unused `__X_va` wrapper is not emitted. Measured (gcc 16.2-rc24, `-Os
  -m68000 -ffreestanding -ffixed-a3/a4/a5 -fno-toplevel-reorder -Wall
  -Wextra`): a test object with `proto/intuition.h` shrinks from 10
  wrappers / 264 bytes to the used one / 96 bytes, the used wrapper is
  byte-identical, no warning. About 11 KB saved over 49 libraries: an
  intended change of those library files, no change of behaviour.
- **Safe calls.** For the NDK functions with a parameter in a3/a4 that the
  libraries use, a macro of the same name saves the register around the
  call with `exg` (the NDK's own technique for a5: 4 bytes per call and
  register, no stack). For every other affected function the shadow header
  `#undef`s the name and poisons it, so a raw call does not compile.
- **Check against an NDK update.** A configure-time check compares the
  functions with a3/a4/a5 parameters that the NDK headers declare with the
  project's list; a difference is a configure error naming the function.
  A test with a broken list and a compile test with a poisoned call prove
  both checks.
- **Fixes on top** (each its own commit, bug-fix workflow): #516
  (riapplib `AddAppIcon`, a3 and a4), #1027 (rimisclib `EZRequest`, a3;
  first measure whether the function form must also give back the space of
  its string arguments, the rule of the #846 comment), and the reworks
  without a change of behaviour: rundebuglib, rilvcallhooklib,
  riwbspritelib (with the kind-F deviation entry for #1028, already closed
  as fixed-in-port). gtlib (#328, fixed in round RP) is checked for a
  switch to the safe macro.

## Capabilities

### New Capabilities
- `libs/os-calls`: how a library calls an NDK function with a parameter in
  a3, a4 or a5, and what the build guarantees about it.
- `tooling/ndk-list-check`: the configure-time comparison of the NDK's
  register-parameter functions with the project's list.

### Modified Capabilities
None.

## Impact

- New: `libs/include/ndk/inline/` (generated into the build directory from
  a text source under `libs/include/ndk/`), a list file, a CMake script
  for the check, tests under `cmake/` and `libs/tests/`.
- Changed: `cmake/BlitzLibrary.cmake` (include path), the libraries that
  include a `proto/` header (smaller: the dead wrappers go), riapplib,
  rimisclib (their fixes), rundebuglib, rilvcallhooklib, riwbspritelib
  (bytes only), `docs/libs/cmdlibs-port.md`, `docs/libs/deviations.md`,
  `tests/README.md`, `TODO.md`.
- Acceptance (`CLAUDE.md`): `ctest --preset all` against a prev build; the
  changed library files are named with `PREV_INTENDED` per commit. The
  compilers' output, TED and the files TED writes do not change.

## Out of scope

- Hand-written `__asm` calls of OS functions: covered by the rule in
  `docs/libs/cmdlibs-port.md` and by the library tests, not by the build.
  `neilsreqtoolslib` (reqtools, not NDK; it saves a3/a4 with `movem.l`)
  stays.
- A scan of objects or disassembly for writes to a3-a5 (not decidable
  without an m68k decoder with data flow, audit section 5).
- Calls of OS functions from Blitz source (`AutoRequest_`, `RawDoFmt_`,
  #958).
- blitz2, TED and `-fbaserel` code.
