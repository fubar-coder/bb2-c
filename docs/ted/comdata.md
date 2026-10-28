# comdata and callbacks: register interface original ↔ glue ↔ C

As of phase 2c. Source: `ted.s` (lines below), [architektur.md](../../architektur.md) §3.2/3.3,
call sites in `blitz2.s` (CD source) and `../blitz2-c/src/main.c`
(`ed_*`). Glue: `src/glue.c` (`REGSTUB`, `REGSTUB_CCR`, adapters `*_c`).

## 1 Mechanics

* **`REGSTUB(name)`**: `movem.l d0-d7/a0-a6,-(sp)`, a4 = TED's a4,
  `name_c(frame)`, all registers back from the frame (as changed by the adapter),
  `tst.l d0`, `rts`. Result: all registers except the
  explicit outputs are preserved (line "comdata entry points" in
  [abweichungen.md](abweichungen.md), category **B**); the flags are those
  of `tst.l d0`.
* **`REGSTUB_CCR(name)`** (new in 2c): as above, but the adapter returns
  the CCR with which the entry point returns (`move.w …,ccr` after the
  `movem`), for routines whose flags do not correspond to `tst.l d0`.
* Callbacks to blitz2 go via `callregs` (registers out of/into a
  frame, returns CCR).

## 2 comdata (TED → blitz2)

"Original" = what the asm routine leaves in registers/flags; "used by
blitz2" = what the call sites in `blitz2.s` read afterwards.

| Off. | Routine (ted.s) | Input | Original: output / destroyed | used by blitz2 | Glue/C | Finding 2c |
|---|---|---|---|---|---|---|
| −8 | `screenclose` 4895 | – | d0/d1/a0/a1/a6 destroyed | nothing (1609) | REGSTUB, `screenclose()` | OK |
| −4 | `screenopen2` 4661 | – | d0… destroyed (screenopen result discarded) | nothing (1664) | REGSTUB, `screenopen2()` | OK |
| 0 | `openreq` 3580 | a0 = requester template | tail `bra pointeron`: d0 = leftover of `DrawBorder` (undefined; failure → `outmem`, never returns with 0), a6 = IntuitionBase | nothing (1973, 2538, 4586, 15170, 15489, 15747, 15987) | REGSTUB, `openreq(a0)`; d0 = d0 of the caller | OK (d0 has no defined value in the original; blitz2-c `ed_openreq` reads d0 – never checked there) |
| 4 | `closereq` 3643 | – | saves d0-d1/a0-a2/a6 itself | nothing (2471) | REGSTUB | OK |
| 8 | `getinput` 5845 | – | d7.l = event (−1 gadget, −2 menu, −3/−4 mouse, key), d6.w = gadget ID/menu number (upper word stays), Z=0; a6 = exec, a0-a2/d0/d1 destroyed | d6.w, d7.w (`cmp #-1,d7`, `cmp #1,d6`) | REGSTUB, `getinput(&d6)`: d7 whole, d6 via `SETW`, d0 = −1 (Z=0) | OK |
| 12 | `error` 3511 | a0 = text | d7 saved; d6 = 1, otherwise destroyed | nothing (7667 saves d7/a6 itself) | REGSTUB, `error(a0)` | OK |
| 16 | `detokallz` 2670 | – | destroyed | – (v1.95 does not call) | REGSTUB | OK |
| 20 | `tokall` 2648 | – | destroyed | – | REGSTUB | OK |
| 24 | `ezfreq` 3502 | a0 path, a1 name, a2 title | d0 = name/0, Z=1 on 0 (requester ends with `move.l`), **a3 = `reqstruct+12`** (address; filerequest+36 saves d1-a6) | Z (`beq`), d0 (1526, 1564, 4127) | REGSTUB (`tst.l d0` = same Z), `ezfreq()` | **corrected**: a3 was the *contents* of `reqstruct[3]`, now the address |
| 28 | `refgads` 2629 | a0 gadget, d0.l count | `jmp refreshglist`: d0/d1/a0/a1 destroyed, a1 = req_window, a2 = 0, a6 = IntuitionBase | nothing; 15510/15628 call three times via a5 (preserved in both) | REGSTUB, `refgads(a0, LONG d0)` | **corrected**: d0 now the whole register (previously `UWORD`, 0xFFFF instead of −1 when upper word ≠ 0) |
| 32 | `getinput2` 5833 | – | nothing there: d7 = 0, Z=1; otherwise like getinput (Z=0 if understood) | d7.l (`tst.l d7`), d6.w (2832, 15187) | REGSTUB, d0 = d7 ? −1 : 0 | OK |
| 36…44 | data `combitmap`, `myscreen`, `extint` | | | | `struct mydata` (structs.h) | OK |
| 48 | `colors` 4940 | – | destroyed | – | REGSTUB | OK |
| 52 | `halfcolors` 4917 | – | destroyed | – | REGSTUB | OK |
| 56 | `tokone` 2636 | a0 = line (0-terminated) | tokenised in place; d5 = length (tokit2), a1/a2 … destroyed; line ≥ 32768 bytes: `rts` without tokit2 (d5 unchanged) | nothing (3841) | REGSTUB, `SETW(d5, tokone(a0))` | OK (case ≥ 32768: d5.w = 0 instead of unchanged – not reachable, blitz2 lines < 256) |
| 60 | `openio` 2548 | – | failure: d0 = 0, Z=1; success: d0 and CCR of `WBenchToFront`, a6 = IntuitionBase | nothing (1697) | **REGSTUB_CCR**, `openio(&ccr)` calls `WBenchToFront` via `callregs` (LVO −342) | **corrected** (previously handle + `tst.l`; DIFF dropped) |
| 64 | `closeio` 2597 | – | destroyed | nothing (1722) | REGSTUB | OK |
| 68 | `actgad` 2409 | a0 gadget | d0 = `ActivateGadget` result | nothing (15176) | REGSTUB, d0 | OK (flags: original those of intuition, here `tst.l d0`; not used) |
| 72 | `pointeron` 209 | – | saves a0-a1/d0-d3/a6 | – (blitz2 1490 `jmp`) | REGSTUB | OK |
| 76 | `sleepon` 223 | – | saves a0-a1/d0-d3/a6; path "already active" destroys a0/d0 via `makemenus` | – (1494) | REGSTUB | **B** (line `sleepon` in abweichungen.md) |
| 80/84 | data `mywindow2`, (gadget address) | | | | structs.h | OK |
| 88 | `extalloc` 3049 | d0 size, d1 flags, **a6 of the caller** (`jsr allocmem(a6)`) | d0 = memory, Z=0; failure: `closereq` + `outmem` (does not return) | d0 (`doallocmem` 3379 = `jmp`) | REGSTUB, `extalloc(d0, d1)` with `AllocMem` via SysBase | OK (blitz2 calls with a6 = exec; a different a6 would hit a different library in the original – not reachable) |
| 92/94 | data `bmapmul`, `req_window` | | | 1939, 2546 | `struct mydata` | OK |

Residual risk for phase 3: the stub preserves registers that the original
destroys. At the call sites above (every `comdata` site in `blitz2.s`,
each time the following instructions reviewed, not the entire further
control flow) no destroyed register is read before being reloaded;
blitz2-c passes/reads only the column "used by blitz2". An A/B test
with blitz2 v1.95 (PHASE3-PLAN.md) covers the rest.

## 3 Callbacks (TED → blitz2)

| Callback | ted.s | Original | C (`glue.c`) | Finding |
|---|---|---|---|---|
| `menurout` | 1542 `extmenu` | `lsr #4,d6`, a5 = `firstitem`, d7.w = `numlines`, a4 = `thisitem` (handler convention); d0.w back (−1/−2/line) | `call_menurout(d6 >> 4)`: d6/d7 upper word 0 (leftovers in the original), a4/a5 as above, return `(WORD)d0` | OK |
| `cleanup` | 2771 `doclean` | `jsr (cleanup)`, all registers saved | `call_cleanup()`: registers 0, TED's registers saved via `callregs` | OK |
| `loadxtra`/`savextra` | 710/795 | d7 = handle, a6 = DOSBase, a5 = routine | `call_xtra`: d7, a5, a6 as original | OK |
| VBlank `myint` | 306 | `is_Code`: a1 = is_Data, return d0 = 0 (Z=1) | `myint_stub`: a4 saved/loaded, `moveq #0,d0` | OK |
| Hunk+4 `outside` / `.tomain` / `edstart` | 24/87/136 | see glue.h | glue_start.c | unchanged (2a, checkhunk) |
