# Changes to shared headers

One line per change to a file owned by the coordinator
([CONVENTIONS.md](CONVENTIONS.md) §1/§9): date, file, change, reason
(chunk, asm evidence).

| Date | File | Change | Reason |
|---|---|---|---|
| 2026-10-26 | all | phase 2a: headers, skeletons, glue, resources, tests created | PHASE2B-BRIEF.md |

## 2b: core functions for the screen passes (agent A, coordinator)

* `fileio.h`: `chmap_showa2item(struct item *, void (*draw)(UBYTE))`,
  `chmap_showall`, `chmap_showtopitem`, `chmap_showbotitem` (ted.s:4050–4104,
  without drawing; `draw` may be NULL). The host stubs (`tests/host/stubs.c`)
  call them with `NULL` (O4).
* `tests/host/toklist.c`: token numbers without bit 15 (blitz2 stores them that way,
  `findtoke`: `bclr #15,d0`). `mkfixtures.py`: new oracle `tokall/`.
* `tests/host/CMakeLists.txt`: tests link `tedhost`/`tedcore` as a link group.

## 2c: integration review (coordinator)

| Date | File | Change | Reason |
|---|---|---|---|
| 2026-10-26 | `src/glue.c`, `src/glue.h` | new `divu_w` (asm: `divu.w`, real trap/overflow), `memcont_nostack` (outmem with `memstack == 0`), `REGSTUB_CCR` (stub returns the CCR of the C adapter), `GLUE_VAR` (global in asm, a4-relative) | item 1/2/3: libnix `__udivsi3` needs `UtilityBase`; O1 |
| 2026-10-26 | `src/core/fileio.h`/`.c` | `divu_w` in the core only for `TED_HOST`; comment `sortblox` (*r = the **ordered** values), `bloken` writes `"block"` (lower case) | item 1, report of agent A |
| 2026-10-26 | `src/core/hooks.h` | comment `blokprnt2` (`bltext`, not "Block") | item 5 |
| 2026-10-26 | `src/core/lineedit.c` | `tabrem` via `divu_w`; `chrow` with `(WORD)chloc` | item 1, review-2c.md |
| 2026-10-26 | `src/input.c` | `showvprop`/`showhprop` via `divu_w` | item 1 (`__umodsi3`) |
| 2026-10-26 | `src/filereq.c` | `#define fq_divu_w divu_w` (`shared/filereq/filereq_core.c` unchanged) | item 1 |
| 2026-10-26 | `src/mouse.h`/`.c` | `openio(UWORD *ccr)` → `ULONG` (d0 + CCR of `WBenchToFront` via `callregs`), `mycli` checks Z; `refgads(…, LONG d0)` (whole register); `outmem` → `memcont_nostack` when `memstack == 0` | item 2/3 |
| 2026-10-26 | `src/glue.c` | `ezfreq_c`: a3 = `&reqstruct[3]` (address, not contents); `openio` with `REGSTUB_CCR` | item 3, comdata.md |
| 2026-10-26 | `src/core/tokit.c`, `src/core/fileio.c` | guard checks `tokens == 0`, `freeall`, `freeupa5`, `chmap_scrolldown` only for `TED_HOST`; `(UWORD)d6 << 6` | item 2, review-2c.md |
| 2026-10-26 | `src/reqs.c` | `swapd0bord` index ≥ 16 like the original (original strides) | item 2 |
| 2026-10-26 | `src/screen.c` | `pixrows22 - (UWORD)(blitmod + 2)` (addq.w) in scrolldown/scrollrite | review-2c.md |
| 2026-10-26 | `tools/checklink.py` (here `programs/ted/checklink.cmake`), `CMakeLists.txt` | ctest `checklink`: no library/arithmetic helpers in the linked ted (map) | item 1 |
| 2026-10-26 | `tests/host/CMakeLists.txt` | `TED_XFAIL_TESTS` no longer a cache entry (old build kept the 2a value, green tests ran red as WILL_FAIL) | item 4 |
