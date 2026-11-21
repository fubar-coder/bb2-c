# Design

## Context

See proposal.md, "Why". The contract comes from the spikes of 2026-11-21
(`~/.local/share/bb2-c-work/tr-spike1-macros-report.md` to
`tr-spike4-endref-report.md`, `tr-task14-ab3-report.md`) and the user's
decisions recorded in `task-decisions.md` (case form: subroutine; the
preprocessor numbers the checks; control data compiled into the driver;
library number 81). The prototype of spike 2 (branch `spike/testlib`)
used the same line form with string IDs.

`shared/` already holds code that a library and a program, or a program
and the host, compile each with its own environment header:
`shared/filereq` (freqlib and TED), `shared/xtra` and `shared/tokenizer`
(programs and the host tools, `tools/lib/CMakeLists.txt:23-27`).

## Goals / Non-Goals

**Goals:**
- One text both sides are written against, archived before they start.
- One codec of the result file for the library and the host.

**Non-Goals:**
- The library itself, the preprocessor, the driver generator, the judge:
  changes `basic-testlib` and `basic-test-runner`.
- Check kinds beyond long, string and condition (Quick, Float, memory
  ranges): later additions of the same form.

## Decisions

**D1 The codec in `shared/basictest/`.** A writer that formats an entry
into a caller's buffer (no allocation, no division: decimal of `K` and
hex of values by shifts and subtraction, as the prototype did, since the
library has no libgcc) and a reader that parses a line into a struct and
reports errors with the line. The library compiles the writer in its
environment (`cmake/BlitzLibrary.cmake`); `tools/lib` compiles both for
the host. A build check compiles the writer in the library environment
before the library exists, so the codec cannot use what a library may not.

**D2 The macro text as a file** in `shared/basictest/` (`macros.bas`), the
text of the spec's requirement "The macro text"; the runner copies it into
every driver. It is checked in the build of `basic-testlib` against the
token lists (`basic-testlib` task 3.2) and by the emulator tests there.

**D3 Global numbers in the program, namespaces on the host.** `K` counts
over the whole source; the namespace (case, procedure, `init`, `end`) and
the number in it are the preprocessor's table. The program holds one long
per check, no string: no change of the string buffer by an identification,
nothing of the source text in memory.

**D4 A failed precondition only is written** (`require K broken`), since
a passing one says nothing the next entries do not say.

**D5 A format version** in `begin`, so that a later change of the format is
refused by an old reader instead of being misread.

## Risks / Trade-offs

- [The interface turns out wrong while one of the two changes is built] →
  a change of this spec in its own change, both sides adapted; the codec's
  tests catch a format drift at once.
- [`shared/` code in the library environment] → the build check of D1.

## Findings of the implementation

- **The block field.** The spec's entry list gives `block` as `init`,
  `case NAME` or `final`, and its scenario shows `block` `case a` as two
  fields. The codec writes and reads `case NAME` as one field (the word,
  a blank, the name as a string value): `block` has two fields for every
  block. A case name never holds a blank or a TAB.
- **One form per value.** The reader takes every value only in the form
  the writer gives it: `K` and the version without sign and leading zero
  and within 32 bits, a long value with upper-case hex digits, `\xHH` with
  upper-case digits and only for a byte that does not stand as it is
  (`\x41` is refused), `ok`/`fail` and `broken` as written, a case name
  not empty. A line ends at its LF; a CR in it is part of the last field.
- **What the writer refuses.** It returns 0 (no line) for a buffer too
  small, an unknown kind or block and a case without a name; the library
  decides what it writes then.
- **The macro text.** The spec names the macros, their arguments, the
  labels and the routines but gives no text; `shared/basictest/macros.bas`
  is the case form of `basic-testlib` D3 with `TLBlockEnd` (the spec's
  routine; D3 still writes `TLCaseEnd`), the batch init and end in the
  same form with their fixed labels (`Dc.l tc__init_end` ... `.tc__init_end`,
  `If TLInit=0 Then Return`; the same for `tc__final`), and the checks
  `TLCheck K,EXPECTED,ACTUAL` (`!CheckEq`), `TLCheckStr K,EXPECTED$,ACTUAL$`
  (`!CheckStrEq`), `TLRequire K,COND`, `TLExpectError K`. The spec lists
  no routine for `!Check{K,COND}` ("ok if not 0"); the text maps it onto
  `TLCheck K,-1,(COND)<>0`, so a failed condition is recorded as expected
  `$FFFFFFFF`, actual `$00000000`. This rests on a comparison giving -1
  for true in 1.95, 2.10 and the port, which the emulator tests of
  `basic-testlib` have to confirm.
