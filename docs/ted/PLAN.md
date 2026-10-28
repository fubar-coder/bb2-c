# Plan: TED 1.1 (`ted.s`) → C

Basis: [overview.md](overview.md), [survey-01 … 07](overview.md#1-chunks),
[reference.md](reference.md), [xrefs.md](xrefs.md). Method as in the
compiler port ([`../blitz2/CONVENTIONS.md`](../blitz2/CONVENTIONS.md)): readable C, **fidelity before elegance**,
original label names as C identifiers, `/* ted.s:NNN label */` at every
function, `BUG(orig)` / `DIFF:` markers, inline asm only in `src/glue*.c`,
m68k-amigaos-gcc (`/opt/amiga`), 68000, libnix, `-fbaserel`.

New compared to blitz2-c: the **core** (line storage, tokenizer/detokenizer,
loading/saving, block operations, search/replace, line buffer logic of the
line editor) is written so that it **also compiles natively on the host**
and can be tested there against `bb2tok.py` and the CD sources.

The reference for all checks is the **vasm build of `ted.s` and its listing**
(`build/ref`, here `build/amiga/programs/ted/ref`, reference.md §1); a comparison with the CD binary is not part
of this plan.

---

## 1 Principles

1. **One routine, one place.** Every routine is translated exactly once. If
   it is `KERN` according to survey table H, it goes into the core file of its chunk
   (`src/core/`), otherwise into the Amiga file of the chunk (`src/`).
   Mixed routines (`KERN+UI`) are split **at the place named in H**:
   core part in `core/` (same label name), the output afterwards as a call
   of a hook (§3.3). The order "first change data, then draw"
   is preserved.
2. **No byte aliasing in the core.** Word pairs (`cursx/cursy`, `btopx/btopy` …)
   that the original copies/compares as a longword are written out
   (both fields). Token words always `(hi << 8) | lo` from bytes. Structures
   are accessed only via fields, never via offsets – on the Amiga
   `_Static_assert(offsetof(...))` secures the layout, on the host it does not matter.
3. **Globals stay globals** (as in the original, same names), register
   states across several calls become globals or small structs (survey-04
   G.4: read state `d7/a5/d6`, block cursor `a5/d6/d5`). Condition code
   results become return values; the assignment is in the header comment
   (`/* asm: N=1 EOF, Z=1 no EOL */`).
4. **Non-local returns**: `addq #4,a7` aborts (`erase`, `getfname3`,
   `doopen`, …) → `BOOL` return, every caller checks and returns;
   `escape` → return "quit editor" to `mainedit`; tokenizer exit
   (`pchover`) → `goto`/return; `outmem` → `longjmp` (§4).

## 2 File layout

```
ted-c/
  CMakeLists.txt, CMakePresets.json (amiga, host), cmake/m68k-amigaos.cmake, .clangd
  tools/   tedlisting.py, xrefs.py               (present; reference build: CMake target ref)
           req2c.py      requester data → C (fork of blitz2-c/tools/req2c.py, survey-07 G)
           checkhunk.py  hunk+4 = outside (like compilerlauf/tedstub)
           checkdata.py  C data against the listing (§6 b)
  src/core/            core: no AmigaOS header, also builds on the host
    tedtypes.h         UBYTE/UWORD/… (Amiga: <exec/types.h>, host: <stdint.h>)
    tedcore.h          umbrella of the core: struct item, struct tokennode, constants
    hooks.h            everything the core needs from outside (§3.3)
    state.c/.h         editor globals (ted.s:6057–6120, 6267–6282), chmap/charbuff*
    loadsave.c/.h      chunk 01: load/write loop, .bak/.xtra/.info, search/replace, blocktab, print format
    lines.c/.h         chunk 02: insert/delete/join/split/return core, gotolined0 arithmetic, block rules, blokload/bloksave, menu lookup
    services.c/.h      chunk 03: tokall/detokall loops, tokone, help search, mice list, a0toname, alloc2
    fileio.c/.h        chunk 04: doread/fillbuff/readline/allocline/linka0, detok/findtoke/detoka5, isithere2, freeupa5/makeline/calcitem, getbchar/sortblox, charstobuff, numtoascii
    tokit.c/.h         chunk 05: tokit*/putchar/alpha, allocnull/freethis/freeall, goup/godown/calcpos/storepos, chmap scroll as memmove
    lineedit.c/.h      chunk 06: line buffer operations of getline, horizpos/cursleft/cursrite logic, inbloks/blokfix, jumptable/menutable, germs
    keymap.c/.h        chunk 07: special key mapping from rawed (+$10A), font data access
    res.c              #embed of the original files: sleep, pointer, srcinfo, font6, font8
  src/                 Amiga part (UI, OS, glue)
    ted.h              umbrella: tedcore.h + structs.h + glue.h + all chunk headers
    structs.h          comdata/mydata table, edstruct copy, options block (50 bytes), OFFS asserts
    glue.h             struct m68kregs, REGSTUB, GLUE_LOAD_A4, callregs (taken over from blitz2-c)
    glue_start.c       hunk start, outside/.tomain, standalone start, comdata stubs, VBlank stub, outmem
    main.c/.h          chunk 01 (UI part)       edit.c/.h   chunk 02
    mouse.c/.h         chunk 03                 reqs.c/.h   chunk 04
    screen.c/.h        chunk 05                 input.c/.h  chunk 06
    data.c/.h          chunk 07: screen/window/menus/requesters (partly generated)
    rawkey.c/.h        rawed.obj in C (console.device)
    filereq.c/.h       file requester: core from libs-c/libs/freqlib.c, TED interface ezfreq
  tests/host/          test_*.c + helpers (token list from the BB2.1 tables, stubs for hooks.h)
  notes/               reference, overview, survey-NN, xrefs, PLAN, (phase 2:) changes.md
```

The core files are cut by chunk (not by topic), so that
parallel translators never edit the same file; their names name the
main topic. Cross-references between chunks go only via headers
(`xrefs.md` = mandatory contents of every chunk header, as in blitz2-c).
`state.c`/`structs.h`/`glue*`/`hooks.h`/`tedcore.h` belong to the coordinator
(changes as in `../blitz2/CONVENTIONS.md` §9 with `changes.md`).

## 3 Shared headers

### 3.1 `core/tedcore.h`

```c
/* ted.s:15 rsreset - one source line; blitz2 reads this list directly (a5) */
struct item {
    struct item *nextitem;      /* +0 */
    struct item *lastitem;      /* +4 */
    UBYTE numchars;             /* +8 length incl. the 0 byte; FreeMem size = numchars + lenitem */
    UBYTE chars[];              /* +9 tokenised text, 0-terminated */
};
#define lenitem 9               /* asm constant, NOT sizeof(struct item) on the host */

/* blitz2 token list (edstruct+0): {next.l, num.w, name\0, help\0, even} */
struct tokennode { struct tokennode *next; UWORD num; char name[]; };
```

On the Amiga: `_Static_assert(offsetof(struct item, chars) == 9)` etc.
(gcc m68k aligns `UWORD` to 2 – fits). Memory sizes are computed with
`lenitem` (original semantics, incl. the `join` bug), on the host
`ted_alloc()` internally allocates `offsetof(struct item, chars) + n`.

### 3.2 `core/state.h` (defined by chunk 06, used by all)

All editor globals the core needs: `firstitem`, `enditem`,
`thisitem`, `numlines`, `cursx/cursy`, `cursoff/curstop`, `scloc/chloc`,
`cursx2…`, `btopx/btopy/bbotx/bboty`, `bloktopx…`, `chcols`, `edrows`,
`edcols`, `tokenise`, `tabstop`, `makeicons`, `fonthite…`, `chmap`,
`charbuff…charbuff4`, `dosbuff`, `filehand`, `allalt`, `anyalt`,
`mousealt`, `memalt`, `invflag`, `qualifier`, as well as the edstruct copy
(`tokens`, `menitems`, `loadxtra`, `savextra`, `menurout`, `cleanup`,
`colon`, `quote`, `label`, `remark`) and the options block `defstart`
(50-byte file image, big-endian; access via getters, so that the host does not
depend on the byte layout). The `comdata` table (−8…+94, packed, with data
`bmapmul`/`req_window` that are globals at the same time) lives in `structs.h` /
`input.c`: the globals there are `#define bmapmul (mydata.bmapmul)`; the
code pointers of the table are set by `outside` at runtime (DIFF: static in the
original – without effect, blitz2 reads them only after the init).

### 3.3 `core/hooks.h` – the seam between core and Amiga

Pure function declarations; the Amiga files implement them, on the
host `tests/host/stubs.c` (counts calls, checks order):

| Group | Hooks |
|---|---|
| Memory | `ted_alloc(size, flags)`, `ted_free(p, size)`; failure = `outmem()` (NORETURN, §4) |
| Files | `ted_open(name, mode)`, `ted_read`, `ted_write`, `ted_close`, `ted_rename`, `ted_delete` (Amiga: dos.library 1:1, host: stdio) |
| Messages | `error(msg)` (comdata+12), `beepem()`, `erase()` (confirmation, BOOL) |
| Display | `showall()`, `showthisitem()`, `showtopitem()`, `showbotitem()`, `scrollup(row)`, `scrolldown(row)`, `scrupall()`, `scrdownall()`, `cls()`, `refresh()`, `showhprop()`, `showvprop()`, `pstats(...)`, `bloken()`/`blokprnt2()`, `showfname()`, `sleepon()`/`pointeron()` |
| Callbacks blitz2 | `call_loadxtra(fh)`, `call_savextra(fh)`, `call_cleanup()`, `call_menurout(d6, first, n)` (Amiga: `callregs`, host: stub) |

Hook names = original labels, so that calls in the core look like in the asm.
Where a core part needs a UI routine only "halfway" (e.g. `blokdis`:
state + status line), the routine is split: state change in the core
(same name), display as hook `blokprnt2()`.

### 3.4 Data and binary includes

* `sleep.bin`, `pointer.bin`, `src.info`, `chars.b`, `chars8.b` →
  `src/core/res.c` embeds the original files at build time with `#embed`
  (`const UBYTE sleep_[704]` …; until 27.10.2026 `tools/bin2c.py` generated
  `build/*/gen/res.c` from them). No copies in the repository.
  `mouseinit` copies them into chip RAM as in the original.
* Requesters (`findreq`, `repall`, `errorreq`, `erasereq`, `defaultreq`,
  `prtreq`, `aboutreq`, `specialreq`, `gotoreq`): `tools/req2c.py` (fork with
  the extensions from survey-07 G) generates the initialisers; fixed
  strides (84/86/44/56/28/54 bytes) are replaced by pointer tables
  (DIFF, same behaviour). Menus, NewScreen/NewWindow by hand.
* `rawed.obj` → `rawkey.c` (decision in reference.md §3.1),
  `filereq` → `filereq.c` with the shared requester core
  `shared/filereq/filereq_core.c` ("variant C", reference.md §3.2; in phase 2a
  implemented as `libs-c/libs/freqlib_req.inc`, since 27.10.2026 a separate
  translation unit).

## 4 What stays asm (`glue*.c`)

No original code stays asm (the only real asm foreign body, the
embedded hunks, is ported). Stubs to be newly written:

| Stub | Reason |
|---|---|
| Hunk start `bra _edstart` / hunk+4 `_outside` | blitz2 calls `BADDR(seg)+8`; no startup code may precede `outside` → `glue_start.c` as first object, `-nostartfiles`, `checkhunk.py` after every build |
| `_outside` | a1 = edstruct → C; return a0 = `.tomain` stub, a1 = `&comdata`, Z flag (BUG(orig): success even on failure, survey-01 I) |
| `_tomain` | a0 = file name; remembers the stack frame for `outmem` |
| `_edstart` | standalone start (wbheader replacement: CLI argument / WBStartup, own `SysBase`/`DOSBase`, since no libnix startup code) |
| comdata entry points (−8…+88) | register convention per service (survey-06 C, survey-03/04/05 B): `REGSTUB` pattern from blitz2-c, returns d0/d6/d7/a0 and Z/N; every stub loads TED's a4 (`GLUE_LOAD_A4`) |
| Callbacks to blitz2 | `menurout` (d6/a5/d7 → d0), `loadxtra`/`savextra` (d7 = fh, a6 = DOSBase), `cleanup` → `callregs()` |
| VBlank server `myroutine` | `is_Code` convention, load a4, return Z=1 (`moveq #0,d0`), shared variables `volatile` |
| `outmem` | `a7 = memstack; jmp memcont` → `longjmp` to the `setjmp` in `mainedit`. Must also work from within blitz2 (`extalloc`, `openreq`): the jump discards blitz2 frames (also blitz2-c frames and their a4) – original behaviour, BUG(orig). `memstack == 0` before the first loop → original crashes; C: avoiding the same crash is `DIFF:` (alert + exit), decision in phase A |

Direct hardware accesses (blitter `$DFF000`, bitplanes, `COLOR00`) are possible in
C with `<hardware/custom.h>` (`volatile struct Custom *custom`) – no
asm needed. Divisions `divu` with 16-bit quotient/remainder as explicit
`(UWORD)` expressions.

## 5 Host core and unit tests

* Preset `host`: `TED_HOST=ON`, native gcc/clang, builds `tedcore` (all
  `src/core/*.c`) + `tests/host/test_*.c`, `ctest`. Optionally
  `-fsanitize=address,undefined`.
* The core includes **only** `tedtypes.h`, `tedcore.h`, `hooks.h`,
  `state.h` and the core headers. A core header that needs `<exec/…>`/`<proto/…>`
  is an error (checked by the host build).
* `BUG(orig)` with memory consequences (reading past the end of line in the tokenizer,
  `detoka5` past `charbuff3`, NULL in `bloksave`/`gd03`, `join` FreeMem
  size): stay in the code; on the host the buffers are laid out contiguously as in the original
  (one allocation for `dosbuff`+`charbuff*`, chmap with
  following lines), so that the behaviour is the same. A macro `ORIG_HAZARD("…")`
  at these spots is empty on the Amiga and counts on the host, so that
  tests can recognise them and expect them specifically.
* Token list for tests: `tests/host/toklist.c` builds the linked list from
  `builtin_tokens.txt`, `tokens/deflibs.txt`, `tokens/acidlibs.txt` in
  the same order as `bb2tok.load_tokens()`; separators as blitz2
  (`colon`=':', `quote`='"', `label`='.', `remark`=';').

## 6 Verification

### (a) Host unit tests of the core

| Test | Check | Oracle |
|---|---|---|
| `test_detok` | every line of the 724 tokenised CD sources (list as `compilerlauf/tools/bb2corpus.py sources()`) with `detok` → text | `bb2tok.py detok` (fixtures from `tests/host/mkfixtures.py` in `build/host/fixtures`, not in the repo) |
| `test_tokit` | detokenised text line by line with `tokit2` → bytes | `bb2tok.py tok` (equal for **all** 724 – differential test; plus individual cases from the header doc of bb2tok: `x1print`, `$` hex quirk, `_.\`, quote/remark) and the 601 originals byte-equal as with `bb2tok check` |
| `test_loadsave` | file → `fileopen2` core (`doopen`/`readline`/`allocline`/`linka0`) → line list → `realsave` core → bytes | **round trip byte-identical** for all 724 tokenised sources; exceptions (e.g. line with 128 bytes, `bui7/marbles/…`) are listed and expected with reason; the 9 text `.bb2` with `tokenise` on against `bb2tok.py tok` (taking into account the `readline` bug "last line without LF") |
| `test_lines` | insert/delete/join/split/return/blokcopy on small lists; block coordinate rules from survey-02 H as table tests; `freeupa5` with 1 line | rules from the survey (derived from the asm) |
| `test_search` | `isithere2` (upper/lower case, token lines via detok), `replace2`, `dosearch` forward/backward | hand-made cases |
| `test_lineedit` | line buffer operations of `getline` (insert with loss of the last character, delete, tab, shift-tab, delete to end of line), `horizpos` | hand-made cases from survey-06 |
| `test_opts` | `loaddefs`/`savedefs` data part: 50-byte image ↔ globals | `defstart` from the listing (`tedlisting.py bytes defstart defend`) |
| `test_keymap` | rawkey special cases `$4C–$5F`, `germs` | listing (`rawed` bytes, `germs`) |

### (b) Instruction/byte checks against the listing

Since no original code is taken over as asm, this concerns:
* **Data**: `tools/checkdata.py` compares C data with the listing
  (`tedlisting.py bytes START END`): fonts/sprites/icon (`res.c`),
  options block `defstart`, `germs`, `jumptable`/`menutable` keys and
  flags, error texts, requester structures (fields except pointers; pointers
  via the symbol assignment). Source of the C bytes: the object with symbols
  (`m68k-amigaos-objdump -t`/`-s`) from the debug build.
* **Layout**: `_Static_assert` for `comdata` (−8…+94), edstruct copy (28
  bytes), `struct item`, options block; hunk layout via `checkhunk.py`.
* **Stubs**: `objdump -d` of the glue stubs against the register inputs/outputs
  defined in survey-06 C/H (review, not automated). For
  `TED_FILEREQ_BLOB=ON` additionally `tedlisting.py verify` on the
  embedded hunk.
* **Line references**: every function carries `ted.s:NNN`; a script checks
  that every code label from `ted.labels` occurs exactly once in `src/`
  (completeness as in blitz2-c phase B).

### (c) Emulator tests (later, only described here)

* Setup like `compilerlauf` (`bb2compile.py` runner): `blitz2:` directory
  with compiler (vasm build 1.95 **and** blitz2-c) + `ted` (reference vasm build
  from `build/ref` or TED-C). Same script against both TEDs → A/B.
* Inputs as synthetic events via a companion process following the
  pattern of `compilerlauf/tedstub/src/reqclose.h` (here `tests/corpus/amiga/reqclose.h`; script from `ENV:`,
  events via `input.device` `IND_WRITEEVENT`: RAWKEY with qualifier,
  menu selection via Amiga keyboard shortcuts or RAWMOUSE, gadget clicks at
  window coordinates). Requesters of TED (erase, error, file requester) are
  answered as with reqclose.
* Scenarios: load source → save (file byte-equal with the original
  TED); typing/join/split/block copy/search-replace → save →
  comparison A/B; COMPILER → CREATE EXECUTABLE → executable equal to that of
  tedstub/reference TED; `loadxtra`/`savextra` round trip; error line
  (`menu` → d0) sets the cursor.
* **Never blocking**: every session ≤ 90 s (watchdog in the runner, abort
  of the emulator), companion process with its own time limit, log with
  `REQCLOSELOG`-style appending (survives hangs), result always a
  file (OK/FAIL/TIMEOUT). Native chipset (no RTG), because TED draws planar
  and uses the blitter directly.

## 7 Procedure

1. **Phase A – headers** (one translator per chunk): `core/<name>.h` and
   `src/<name>.h` with all labels from `xrefs.md` and the routines of the
   own core file; the coordinator writes beforehand `tedtypes.h`,
   `tedcore.h`, `hooks.h`, `state.h`, `structs.h`, `glue.h`,
   `CONVENTIONS.md` (derived from blitz2-c with §1–§5 of this plan).
   Check: `--target headers` (Amiga) and a host syntax run over all
   core headers.
2. **Phase B – translation** in parallel; core first compilable on both
   presets. Chunk 07 uses `req2c.py`/`bin2c.py`; `filereq.c` = work on the
   shared freqlib core.
3. **Phase C – host tests** (§6 a) until green; clarify deviations (bug in the
   port or BUG(orig)).
4. **Phase D – data/layout check** (§6 b), then emulator tests (§6 c).

## 8 Open points

* Behaviour on `outmem` before the first main loop (original crashes).
* ~~Shared requester core with libs-c~~: done (variant C,
  today `shared/filereq/`).
* State of phase 2a and further open decisions: `PHASE2B-BRIEF.md` §7 (bb2 only, not taken over).
* `.clangd` for `src/core`: runs as m68k profile; a host profile for
  `tests/host` is entered.
* Whether `chmap` in the core necessarily needs chip RAM: no on the host; on the
  Amiga the UI side allocates it as in the original (blitter).
