## Context

The compiler is not a process of its own: TED calls it through the menu
routine of `edstruct` and it runs in TED's task until the compile ends.
TED does not look at its ports meanwhile; the compiler sees the STOP
gadget through an exec exception on the signal of the requester window
(`compile.c`, `stopcode_c`). The editor's services are the `comdata`
table TED hands back from its init (`ted.s:6030`; TED-C `struct mydata`
in `programs/ted/src/input.c`; the compiler's offsets in
`programs/blitz2/src/main.h`). The table ends with `req_window` at
offset 94; in the original TED its own variables follow (`topskip`,
`botskip`, ... at 98).

Today `compile` (`compile.c:190-214`) takes the stripe's right end from
the editor window's width (`riteend = width/8 - 7`), the 16.16 step of
the main bar (`titleadd[0]`) and `botline = 9 * bmapmul`; `nextline`
(`compile.c:337-369`) XORs one byte in row 0 and one in row 9 of
`combitmap` for the main bar, and for include level 1 to 3 one byte in
row `level` and in row `9 - level`. The headless editor of the tests
(`shared/edstub`, programs `tedstub` and `ab3cli`) gives a 640-pixel
fake window and an 80-byte-per-row bitmap of its own.

## Goals / Non-Goals

**Goals:**
- The compiler never writes into an editor's bitmap.
- With TED-C the bar looks as before, drawn by TED.
- The original compiler keeps working with TED-C.

**Non-Goals:**
- A different look of the progress display, or progress outside TED
  (`ab3cli`, `tedstub` show none).
- Changing the original programs (the original compiler's #980 stays).
- TED on RTG screens (TED needs a planar screen the blitter can reach).

## Decisions

### D1 A call in `comdata`, not an exec message

The progress goes through a new entry of the `comdata` table, called
with registers like the other editor services (`callregs`). An exec
message to a port of TED would only be read when TED's code runs, and
during a compile it does not run unless the compiler calls into it, so
the compiler would have to call a "fetch messages" entry after every
`PutMsg`: the same call with more parts. The call keeps what a message
would give: TED alone decides whether and how to draw, and draws only
when its screen is open (D4).

### D2 The marker after the original table

TED-C appends to `struct mydata`, after `req_window` (offset 98), the
longword `'TEDp'` ($54454470) and the entry of the progress routine
(offset 102). blitz2-c uses the entry only if offset 98 holds the
marker. In the original TED, offset 98 is the word `topskip` (pixels of
the screen title, below 256) followed by `botskip`, so its high byte is
0 and the long can never be $54454470. Alternative: a flag in
`edstruct` (compiler to editor) - rejected, since the compiler, not the
editor, must decide whether to call; the editor would still need to
answer in `comdata`. The edstub announces the call only when a test asks
for it (a pass-file option of `tedstub`), so the compiler side can be
tested both ways.

TED-C keeps every original field (`combitmap`, `bmapmul` included), so
the original compiler, which reads only offsets below 98, runs as
before (checked in task 1.1).

### D3 What the call carries

`d0` = bar (0 main, 1 to 3 include level), `d1` = the byte column the
original would XOR (main: `hi - 1`, include: `col`). The call means
"toggle this 8-pixel cell of this bar", exactly the XOR of today, so
TED-C reproduces the original's picture without repeating the
compiler's 16.16 arithmetic, and the compiler keeps computing the
columns from the window width as now (`riteend`, `titleadd`). The
compiler keeps all of `nextline`'s bookkeeping; only the two XORs per
bar become one call. Alternative: report lines done and total and let
TED compute the columns - rejected: equal pictures would need TED to
repeat the 16.16 truncation and the include counting, two copies of the
same arithmetic.

### D4 How TED-C draws

The routine draws into the screen's RastPort (`myscreen->RastPort`):
draw mode `COMPLEMENT`, write mask 1 (only plane 0, as the original
XORs only `combitmap`), a `RectFill` of 8 x 1 pixels at x = 8 x column
in row `bar` and in row `9 - bar` (main: rows 0 and 9). Draw mode and
mask are restored afterwards. It returns at once if TED's screen is not
open (between `screenclose` and `screenopen2`, before the first open).
Alternative: XOR into the bitplane in TED as the compiler did - rejected,
it keeps the planar assumption and the word offset of #980.

### D5 blitz2-c without the call

Without the marker `nextline` does the bookkeeping and calls nothing;
`combitmap`, `bmapmul` and `botline` are no longer read for drawing
(`botline` goes away). The original TED with blitz2-c therefore shows
no bar.

### D6 Tests

- bug-orig test for #980 (`tests/bug-orig/compiler/`, `; @fixed #980`):
  `tedstub` with a `bmapmul` of 4000 and guard bytes before and after
  its bitmap; the program has enough lines (and an include) that every
  bar moves. FIXED: the guards are untouched (and, with the call
  announced, the reported cells are those the original would XOR).
  On the unfixed compiler the guard before the bitmap is overwritten
  (REPRODUCED), as the original's `.orig` records.
- TED A/B (`06-compile.drv` and the other scripts that compile): C5-Cc
  stay equal (TED-C draws the original's picture); R5-Rc may differ in
  the dumps after compiling (Rc has no bar): an intended difference with
  `@fixed #980 R5-Rc` if the bar is in a dump.

## Risks / Trade-offs

- [The original compiler reads a `comdata` offset at or above 98] →
  task 1.1 checks every `comdata` access of `blitz2.s` before anything
  is appended.
- [The stripe is still visible in a TED A/B dump and C5-Cc differ by a
  pixel (draw mode, plane, position)] → compare C5 and Cc dumps of
  `06-compile.drv` early (task 3.3); the drawing follows the original's
  rows and columns exactly.
- [A third-party editor that hands blitz2 a `comdata` table] → without
  the marker it gets no bar and no write into its bitmap, which is safer
  than before.
- [The original TED with blitz2-c loses its progress bar] → accepted
  (user decision): the fix is for blitz2-c with TED-C.

## Migration Plan

None: TED-C and blitz2-c are built and shipped together; the old
combinations keep working as described in the spec. Rollback is the
revert of the commits.

## Findings of the implementation

- Task 1.1: every `comdata` access of `blitz2.s` (1.95; the CD's 2.10 is
  the same source with one changed line) and of blitz2-c reads an offset
  below 98 (the highest: the long `req_window` at 94); the list with the
  lines is in the message of the TED-C commit. D2 stands.
- Task 1.2: Intuition redraws the title area (pixel rows 0 to 9) when a
  requester of TED closes, so the dumps taken after a compile
  (`06-compile` `compiled`, `07-error` `line`; the `29-cmp-*` scripts take
  none) show no bar. Only the dump taken while the error requester is
  open, `07-error` `error`, shows it: the first cell of the main bar in
  rows 0 and 9 (the error is in line 2). In the run before the change,
  rows 0 to 9 of plane 0 were the same in all six combinations of every
  dump.
- Pairs: the original TED with blitz2-c (Rc) has no bar, so in a dump
  that shows the bar both R5-Rc and Rc-Cc differ (task 3.4 named only
  R5-Rc); C5-Cc stay the same. `; @fixed #980 R5-Rc Rc-Cc`.
- A dump with the whole bar: `tests/ted-ab/scripts/30-stripe.drv`
  compiles `tests/ted-ab/src/stripe.bas` (80 lines, an include of 12
  lines, an error in its last line) and dumps the error requester: main
  bar (73 cells) and the bar of include level 1 (12 cells) in rows 0, 1,
  8, 9. C5 (the original compiler XORs) and Cc (TED-C draws with
  RectFill/COMPLEMENT) are the same byte for byte; R5-Rc and Rc-Cc differ
  by 168 bytes (84 cells in two rows) and the "Largest Mem" field.
- Two passes: the tedstub jobs compile in two passes (two "COMPILING"
  requesters), and the XORs of the second pass undo those of the first:
  at the end of a job the original's stripe and even its writes before
  the bitmap are gone. The stub therefore checks at checkpoints (openreq,
  closereq, the end of the job) and logs the fullest picture and the most
  changed guard bytes. The original then shows rows 0 to 3 and 95 changed
  guard bytes before the bitmap (73 + 11 + 7 + 4 cells of rows 9 to 6);
  blitz2-c reports each cell twice (once per pass).
- "Shown" in the spec is taken as "open": TED-C draws while
  `screensopen` is set, also when its screen is behind another one (the
  original XORs then as well, and skipping toggles would leave a wrong
  picture when the screen comes to the front).
- The bug-orig test came out REPRODUCED with the stub of this change and
  the compiler before the fix (port and clib); its recorded original was
  deterministic (recorded twice). A fixed test has no `.orig`, so it is
  not committed.
