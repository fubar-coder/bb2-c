# Translation conventions ted.s → C (phase 2b)

Derived from [`../blitz2/CONVENTIONS.md`](../blitz2/CONVENTIONS.md)
with the points of [PLAN.md](PLAN.md) §1–§5. Where nothing is stated here,
blitz2-c applies. Code comments and documentation in English.

## 1 Files and ownership

| Chunk | Lines | Core (`src/core/`, host-testable) | Amiga (`src/`) |
|---|---|---|---|
| 01 | 1–1015 (+wbheader, aboutreq, specialreq) | `loadsave.c/.h` | `main.c/.h` |
| 02 | 1016–1980 | `lines.c/.h` | `edit.c/.h` |
| 03 | 1981–3062 | `services.c/.h` | `mouse.c/.h` |
| 04 | 3063–4197 | `fileio.c/.h` | `reqs.c/.h` |
| 05 | 4198–5252 | `tokit.c/.h` | `screen.c/.h` |
| 06 | 5253–6285 | `lineedit.c/.h` (+ data in `state.c`) | `input.c/.h` |
| 07 | 6286–8072 | `keymap.c/.h` (+ `germs`, `fontcalc`) | `data.c/.h`, `rawkey.c/.h`, `filereq.c/.h` |

Shared, **owned by the coordinator** (do not change without agreement, §9):
`core/tedtypes.h`, `core/tedcore.h`, `core/hooks.h`, `core/state.h`,
`core/state.c`, `src/structs.h`, `src/glue.h`, `src/glue.c`,
`src/glue_start.c`, `src/ted.h`, `src/filereq.c`, `CMakeLists.txt`,
`tests/host/hostenv.h`, `tests/host/stubs.c`, `tools/*`, and
`shared/filereq/` (shared with freqlib in `libs/acidlibs`).

The chunk headers (`core/<x>.h`, `src/<x>.h`) already contain **every
routine** of the chunk with prototype and comment (inputs/outputs, asm
register assignment, BUG(orig) notes), the `.c` files every function with
an empty body (`/* TODO(2b) */`). Translators fill in the bodies; the
signatures are agreed – changes only per §9.

## 2 Core and Amiga part

* **Core** (`src/core/*.c`) includes only `tedtypes.h`, `tedcore.h`,
  `hooks.h`, `state.h` and core headers – never `<exec/…>`, `<proto/…>`,
  `structs.h`, `ted.h`. It builds on the host (`TED_HOST`). Everything
  operating-system related goes via `hooks.h`.
* **Amiga part** (`src/*.c`) includes `ted.h` (umbrella) and the NDK
  headers it needs (`<proto/exec.h>`, `<proto/intuition.h>` …).
  Exception: `filereq.c` (own unit, see there).
* **Mixed routines** (survey table H "CORE+UI"): the core part lives in the core
  under the original label name; it then calls the output as a hook (hook =
  UI routine with the original label name, implemented in the Amiga part of its
  chunk). Where the UI routine predominates, it stays in the Amiga part under the
  label name and the core part is a new function (`help_core`,
  `prtfile_line`, `gl_*`, `chmap_*`, `germs_map`, `rawspecial`) – marked in the
  comment as `ted.s:NNN (label, core part)`.
* The order "first change data, then draw" stays; a hook changes
  no core state except where noted in hooks.h as "state:".

## 3 Names, comments, markers

* Every label stays an identifier (`tokit`, `gd03`, `blokcopy`). Conflicts with
  C/libc: append `_` (`return_`, `putchar_`, `sleep_` (data)).
* **Every function starts with `/* ted.s:NNN label */`**; labels inside
  a function (goto targets, fail chain, `done2`, `getline2`, `ginp1` …)
  get a comment `/* ted.s:NNN label */` at their place. The
  skeletons collect these markers at the end of the file – move them to the
  right place when translating. `tools/checklabels.py` (ctest `checklabels`; here `tools/checklabels`, POST_BUILD of `ted`)
  requires every code label **exactly once** in `src/**/*.c`. References that
  are not a definition site are written differently (`ted.s:570 (prtfile, …)`,
  "ted.s line 128").
* Take over original comments (English), `/* @NNN */` at main blocks of
  long functions.
* `BUG(orig): …` – deliberately carried-over bug (all from the surveys,
  section I). `DIFF: …` – unavoidable deviation with justification; every
  new location as a line in [deviations.md](deviations.md).
* `ORIG_HAZARD("…")` (tedcore.h) at places where the original reads/writes outside
  its buffers or uses NULL and C reproduces that
  (Amiga: empty; host: counted).

## 4 Types and state

* Exec types (core: `tedtypes.h`), sizes as in the asm (`dc` = 16 bits,
  `dc.l` = 32 bits); signedness according to use, with mixed comparisons
  cast at the comparison (`(WORD)`/`(UWORD)`) like `bcs/bhi` vs. `blt/bgt`.
* **Globals stay globals**, name and initial value as in the original
  (`state.h` + chunk header). Word pairs that the asm copies/compares as a longword
  are written out (both fields). Byte/word aliasing
  (gadget `+13` bit 7 = `Flags & GFLG_SELECTED`, `numchars`+`chars[0]` as
  word) explicit. Options block: `struct defblock defstart` with macros
  `fonthite`, `tabstop` … – **no local variables with these names**.
* Register state across several calls: a context struct on the stack of
  the operation, passed down by pointer: `struct rdstate` (loading:
  d7/a5/d6), `struct bcur` (block cursor: a5/d6/d5), `struct tedevent`
  (event at dispatch: d7/d6).  Otherwise parameters/return values,
  assignment in the header comment (`asm: Z=1 -> TRUE`).
* Handlers from `jumptable`/`menutable`: `void name(void)`, asm a4 =
  `thisitem` on entry; a handler that reads d7/d6 is
  `void name(const struct tedevent *ev)` (`evroutine` of the table entry).
* Token words always `(hi << 8) | lo` from bytes; structures only via
  fields. Divisions `divu`: explicit 16-bit quotient/remainder; division by 0
  in the original (e.g. `tabstop = 0`) stays (BUG(orig), ORIG_HAZARD before it).

## 5 Control flow

* Tail call `bra x` → `x(); return;`; fall-through → call + comment
  `/* falls through into x */`.
* `addq #4,a7` aborts (`erase`, `getfname3`, `getbname`, `doopen`) →
  return value; **every caller checks and returns immediately**.
* `escape` → `edquit = TRUE` (mainedit returns). `outmem` → `longjmp`
  (NORETURN; `memstack` = `jmp_buf *` of mainedit).
* Tokenizer exit `pchover` → `goto`/`return` in the tokenizer.

## 6 Amiga-specific

* **Inline assembler only in `src/glue*.c`** (coordinator). Blitter/hardware
  with `<hardware/custom.h>` in C, order and width of the accesses as in the
  original.
* **No libnix startup code** (`-nostartfiles`): library bases are set by
  `outside_c`/`edstart_c` itself (`SysBase`, `DOSBase`, `IntuitionBase`,
  `GfxBase` in main.c). Do not use libnix functions that need constructors
  (stdio, malloc, `printf` …); allowed: `memcpy`/`memset`/`strlen`
  etc. and Exec/DOS calls.
* `-fbaserel`: globals a4-relative; every entry from outside loads a4 (glue).
  Code that is called by blitz2 only via the stubs (`glue.h`).
* Callbacks to blitz2 only via `call_menurout`, `call_cleanup`,
  `call_loadxtra`, `call_savextra` (glue.c, `callregs`).

## 7 Data

* Requesters (`findreq`, `repall`, `errorreq`, `erasereq`, `defaultreq`,
  `prtreq`, `gotoreq`, `aboutreq`, `specialreq`): initialisers with
  `tools/req2c.py` (fork of blitz2-c, extensions survey-07 G) – to be
  created in 2b; menus, NewScreen/NewWindow by hand. Alias labels as
  `#define` (`int1`, `errorpnt`, `gotoint`, `viewmode` …), strides via
  pointer tables (data.h).
* Binary includes: `src/core/res.c` embeds the original files at build time
  with `#embed` (sizes via `static_assert`), never copies into the
  repository.
* Data check: `tools/checkdata.py` – new data must be entered there
  in `CHECKS` (here `tools/checkdata` with the table `programs/ted/checkdata.tsv`).

## 8 Checking

* Amiga: `cmake --build --preset amiga` (without warnings as far as sensible);
  bb2: `--target headers`, `ctest --preset amiga` (`ref_verify`, `checkhunk`,
  `checkdata`, `checklabels`) – here checks in the build step (README).
* Host: bb2 `cmake --build --preset host`, `ctest --preset host`; with
  sanitizers `-DHOST_SANITIZE=ON` (bb2: `TED_SANITIZE`). Single test with
  output: bb2 `build/host/tests/host/test_tokit build/host/fixtures -v`.
* Listing of the reference: `tools/tedlisting.py build/amiga/ref/ted.lst …`
  (`addr`, `line`, `bytes`, `carray`; bb2 only; here `tools/vasmlisting` – `map`, `labels`, `verify` – on `build/amiga/programs/ted/ref/ted.lst`).

## 9 Changing shared headers

If something is missing in a shared header (hook, global, field, type) or
a signature of a **foreign** chunk header does not fit: do not change it
yourself, but report it to the coordinator (file, desired change, reason,
asm evidence). The coordinator changes it, enters a line in
[changes.md](changes.md) and informs the others. One may extend one's **own**
chunk header (new non-static helper function), but
change existing signatures only after agreement – other chunks call them.
