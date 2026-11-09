## Why

While compiling from TED, the compiler draws its progress bar (the
"racing stripe") by XORing bytes straight into the first bitplane of
TED's screen (`combitmap`, `bmapmul` from TED's `comdata`). It does not
know whether that screen is open or visible, and the offset of the
stripe's bottom line is a signed word: from 3641 bytes per row it
writes about 28 KB before the bitmap (#980). The screen is TED's; the
compiler should only say how far it is, and TED should draw.

## What Changes

- blitz2-c reports its progress to TED through a new call in TED's
  `comdata` table instead of writing into TED's bitplane: the state of
  the main stripe and of the include stripes, at the points where it
  XORs today.
- TED-C announces the new call in its `comdata` (a marker and the
  entry after the original table) and draws the same stripe as today
  through its RastPort with graphics.library, only while its screen is
  open and shown.
- blitz2-c draws no stripe when the editor does not announce the call
  (the original TED, the headless editor stub of the tests): the
  compiler never writes into an editor's bitmap any more.
- TED-C keeps filling `combitmap` and `bmapmul`, so the original
  compiler still runs with TED-C and draws its own stripe as before.
- #980 is fixed for the combination blitz2-c with TED-C. The original
  compiler keeps its bug (it cannot be changed); the original TED with
  blitz2-c shows no progress bar.

## Capabilities

### New Capabilities

- `compiler/compile-progress`: how the compiler reports its progress
  while compiling from the editor, how the editor shows it, and how the
  two find out whether the other side supports it (including the
  combinations with the original TED and the original compiler).

### Modified Capabilities

(none)

## Impact

- `programs/blitz2/src/compile.c` (`compile` set-up of the stripe,
  `nextline`), `main.h`/`main.c` (the `comdata` offsets and the new
  editor call wrapper), `glue` (register call).
- `programs/ted/src/input.c` (`struct mydata`: marker and entry after
  `req_window`), `ted.h`, a new progress routine in TED-C (drawing in the
  screen title area through the RastPort), `glue.c` (register stub).
- `shared/edstub/edstub.c`: the stub announces the call when a test asks
  for it and records the reports, so the compiler side can be tested
  without TED; with it, guard memory and a wide `bmapmul` show that the
  compiler writes nothing into the bitmap.
- Tests: a bug-orig test for #980 (`; @fixed #980`); the TED A/B
  scripts that compile (`06-compile.drv` and others): pair C5-Cc must
  stay equal (TED-C draws the same stripe as the original compiler
  did); R5-Rc may differ in the dumps taken after compiling (Rc: no
  stripe), named as an intended difference.
- Docs: `docs/libs/deviations.md` row of #980 (kind F), the interface
  description of `comdata` (`docs/ted/`, `docs/blitz2/`), the website
  page of fixed original bugs (coordinator).
- Results: the executables are unchanged (the stripe is no code
  generation); TED and blitz2 change as programs.
