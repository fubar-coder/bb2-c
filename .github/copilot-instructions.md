# Copilot instructions

This repository is a C port of Blitz Basic 2 for AmigaOS/68000, built with
`m68k-amigaos-gcc`. Start with `README.md`, `docs/structure.md`,
`docs/development.md`, and `tests/README.md`; use `docs/architecture.md` for
the compiler/editor/runtime flow. Translation notes and original-source
references are in `docs/`.

## Build and test

Requirements include CMake 4.3+, Ninja, `m68k-amigaos-gcc` on `PATH`, and
native C/C++ compilers. Configure and build the Amiga programs and host tools:

```sh
cmake --preset amiga
cmake --build --preset amiga
```

Run host tests without an emulator, or select a single CTest by its exact
registered name:

```sh
ctest --preset host
ctest --preset host -N                         # list registered tests
ctest --preset host -R '^blitzlib_asm_sections_broken$'
```

For emulator acceptance tests, configure Amiberry, ROMs, and hardfiles as
described in `AMIGA-EMULATION-SETUP.md`, then use `ctest --preset all`.
`ctest --preset emulator` runs only emulator-labelled tests. Other specialized
presets include `oracle` (AmiBlitz3 comparisons), `originals` (requires the
original material; see `cmake/fetch-originals.cmake`), `bug-orig` (known
original bugs, expected to reproduce), and `perf`. Preset contents and
requirements are in `tests/README.md`.

Basic-test-model cases can be selected by source with
`ctest --preset all -R '^ID__$'`; see “Running and reading a test” in
`tests/README.md`. Build-time checks are part of the target build; a failed
check means its result is not built. A clean rebuild should have no warnings.
For host-side sanitizer checks, configure with `-DHOST_SANITIZE=ON`.

## Architecture

- `blitz2` loads TED (`ted`) as a code segment; both run in the compiler's
  process. TED is the UI and calls the compiler through the editor interface.
  The compiler emits machine code into a memory buffer, incorporates required
  library code, and writes or runs a single-hunk executable—there is no
  separate linker or object-file stage.
- `acidlibs` contains Blitz command libraries and runtime/system libraries;
  `deflibs` contains OS-call libraries and declarations. The compiler loads
  these BigLib libraries and appends their tokens to the language token list.
  `blitz2-ab3` adds AmiBlitz3 source support; `ab3cli` is its headless editor
  companion.
- TED separates its Amiga UI (`programs/ted/src/`) from its OS-independent,
  host-testable core (`programs/ted/src/core/`). Code under `shared/` is
  reused by programs and/or host tools; each build side compiles it with its
  own environment and options.
- Host tools and host tests are built natively through `host/`; tools use a
  C++ CLI layer and keep reusable logic in C libraries under `tools/lib/`.
  Tests cover host behavior, emulator behavior, and comparisons against
  original/reference builds.

## Repository-specific conventions

- Treat `archive/` as immutable original material. Translation evidence and
  explanations for original behavior belong in `docs/`; generated data belongs
  in the build tree or `tests/data/`.
- Preserve assembly traceability in the compiler and TED ports: retain
  original label names and `BUG(orig)` notes. Use `DIFF:` only where C cannot
  reproduce the assembly exactly, with the reason; document intended behavior
  changes in the relevant `docs/*/deviations.md`. Follow “Fixing an original bug” in
  `docs/development.md` for fixes, including their expected-behavior tests.
- Preserve output compatibility: unless a change is intentional, generated
  executables, TED-written files, and library files must remain byte-identical
  to the previous commit. Use the `prev` build described in
  `docs/development.md` / `tests/README.md` when checking a rework or an
  intended change.
- There are two compilation environments. Libraries use Blitz conventions
  from `cmake/BlitzLibrary.cmake`; `blitz2` and TED are programs built with
  `-fbaserel`. Shared sources may be compiled in both environments and must
  use the appropriate environment header. Host tests are C++20 Snitch
  programs, generally kept with their component.
- Keep project infrastructure in C/C++ and CMake. Binary-format and tool
  logic belongs in C under `tools/` (shared logic in `tools/lib/`); do not
  introduce a scripting-language dependency into the build or tests.
- Repository and GitHub text is in English. Respect existing user decisions
  recorded in the relevant entries under
  `~/.local/share/bb2-c-work/decisions/` when they apply.

For component-specific rules, consult `docs/blitz2/CONVENTIONS.md` and
`docs/ted/CONVENTIONS.md`.
