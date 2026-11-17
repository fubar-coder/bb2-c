# blitzbasic2-c – notes for Claude

C port of Blitz Basic 2 (compiler `blitz2` v2.10, editor `ted` 1.1,
libraries `acidlibs`/`deflibs`) for AmigaOS (68000), built with
m68k-amigaos-gcc. This repository is the reference for the code; the
original sources are in `archive/`, the notes on the translation (reasons
for `BUG(orig)`, `DIFF`, `ted.s:NNN`, `blitz2.s:NNN`) in `docs/`.
Start here: `README.md` (building), `docs/structure.md`,
`docs/development.md` (conventions, the checks in the build, fixing an
original bug), `tests/README.md` (tests and presets). Open items:
`TODO.md` (`- [ ] **Title** …` open, `- [x] **Title** done (date): …`;
no strikethrough). The user's decisions:
`~/.local/share/bb2-c-work/decisions.md`.

## Building and testing

```
cmake --preset amiga && cmake --build --preset amiga   # result: build/amiga/blitz2/
ctest --preset host       # without emulator (seconds)
ctest --preset all        # host and emulator: the acceptance run
ctest --preset bug-orig   # open original bugs: fail on purpose
ctest --preset originals  # comparisons with the originals; needs the CD
ctest --preset oracle     # AmiBlitz3's compiler checks tests/ab3 (before merging tests/ab3)
ctest --preset perf       # library benchmark against tests/perf/budgets.txt
```

What each preset runs and needs: `tests/README.md`, "Original material
and presets".

* The Bash tool runs zsh (the login shell is fish): put loops, word
  splitting and heredocs in `bash -c '…'`.
* Emulator only via `tools/lib/amiga_emulation` or `amiga_run`; the
  hardfiles are read-only, tests write only to `TEST:` or `RAM:` (a write
  to `SYS:` hangs the run).

## Rules, in order of weight

1. **The user's statements.** An answer to a concrete question with the
   options shown is a decision; record every such answer with the quote,
   date and scope. It goes to `decisions.md` only if it is to serve as
   precedent for other, future tasks; otherwise to the round's decisions
   file, or outside a round to `task-decisions.md` (both in
   `~/.local/share/bb2-c-work/`). A rule you derive from a side remark,
   a counter-question or a guess ("vermutlich") is *proposed*: apply it
   only to the case it was said for and ask before using it elsewhere;
   collect proposals and show them together. Do not write such a rule
   into CLAUDE.md, an agent definition or memory without the user's OK
   to the wording. Quote decisions by ID instead of rewording them. Why: rewordings dropped the
   user's qualifications and turned single-case remarks into general rules
   (decisions.md, D-001).
2. **Conflicts are reported.** If two rules contradict each other, or a
   rule does not fit the case, say so (agents: in the report) instead of
   resolving it silently or adding a workaround next to it. Why: a
   workaround leaves the contradiction in place for the next reader.
3. **Results stay byte-identical** to the previous commit unless a change
   is intended and named: the executables the compilers write, the files
   TED writes, `acidlibs`, `deflibs` and all library files. `ctest
   --preset all` compares them with the `prev` build (`cmake --preset
   amiga -DBLITZ2_PREV=ON`; `tests/README.md`, "The prev build"); an
   intended change is named with `-DPREV_INTENDED` and in the commit
   message. The programs themselves (`blitz2`, `ted`, host tools) and
   toolchain updates may change; check them with the tests and name the
   rework. Why: the byte comparison is the only check that catches an
   unintended change in hundreds of results.
4. **Original bugs are fixed deliberately**; the goal is functional BB2
   compatibility with the bugs fixed (D-003), not identical executables.
   Every fix follows `docs/development.md`, "Fixing an original bug" (spec
   `testing/bug-fix-workflow`): one issue per bug; one fix commit with the
   code change, the test of the expected behaviour (`; @fixed #N`, FIXED),
   `Fixed (#N)`, the kind-F entry in `docs/*/deviations.md`, the changed
   results named and a line `Fixes #N`; fix-scope run by the agent and
   again by the coordinator before the merge. Agents propose the severity
   (by the level texts in `site/data/fixoverview.yaml`) and the topic; the
   website entry, the overview row, the label `fixed-in-port` and the
   check after the push are the coordinator's. A bug not fixed yet:
   `BUG(orig) (#N)` and a bug-orig test that comes out REPRODUCED; checked
   only in the error check area on purpose: `By design (#N)`, label
   `as-designed`. Which bugs get fixed and how: decisions.md (D-005 to
   D-007).
5. **Testing an agent's work:** agents run the tests their brief names,
   the tests of what they touched and `ctest --preset host`, in the
   foreground; the coordinator runs `all`, `bug-orig` and `originals` once
   after merging (D-010). Why: a full run takes up to 20 minutes; once
   per round is enough to catch what the targeted tests miss.
6. **Only C/C++ and CMake** in the project infrastructure (build, tests,
   tools): what compiler and linker can do themselves without a tool
   (`#embed` + `static_assert`, `strip`); text and control in CMake;
   binary formats and logic as a C tool in `tools/` (logic in
   `tools/lib/`, tests in `tools/tests/`). Why: a script written once
   tends to become infrastructure without anyone deciding it (D-002).
   Claude Code hooks are exempt (any language), but nothing in the
   project may depend on them.
7. **`archive/`** holds unchanged originals: never edit; a new state is a
   new directory with a version number, its origin in `archive/README.md`.
   Derived data goes to the build directory or `tests/data/`. Material
   without a clear license (the CD) is loaded by
   `cmake/fetch-originals.cmake`, not checked in.

## Code conventions

* A clean rebuild has no warnings, also none of `blitzlib check`.
* Checks in the build run in the build step of their result (another
  `COMMAND` or `POST_BUILD`); a failed check means the result is not
  built. No temporary or stamp files. Every check has a test with
  deliberately broken input.
* Two compilation worlds: libraries with the Blitz conventions
  (`cmake/BlitzLibrary.cmake`), blitz2 and TED as programs with
  `-fbaserel`; code in `shared/` is compiled by each side with its own
  environment header.
* Host tests live with their component, are snitch programs (C++20),
  one program per test file; also check with `-DHOST_SANITIZE=ON`.

## License

GPL-3.0-or-later with `RUNTIME-LIBRARY-EXCEPTION` (README, "License");
the originals' license situation in `archive/README.md`. New code under
the same license; no code with an unclear or GPL-incompatible license.

## Git

* One topic per commit; never combine separable work. Check `git status`
  first and stage only the topic's files.
* All texts in the repository and on GitHub are English.
* `origin` (GitHub `fubar-coder/bb2-c`): the user pushes; no force push,
  no rewriting of pushed history.
