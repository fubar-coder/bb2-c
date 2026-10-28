# Review phase 2c – sample at instruction level

Subject: six groups of routines, line by line against `ted.s` (lines =
listing lines). Looked for: signed/unsigned comparisons
(`bcs/bhi` vs. `blt/bgt`), `.w`/`.l` widths, `dbf` counts (n+1),
returns via flags, register values that live on into later code,
order of side effects. Sources not changed.

Result up front: **no real mistranslation found.** Three small
points (C UB in a hazard path, an `addq.w` width, a
consistency question with `add.w chloc`) are listed below as finding "minor";
none is visible in normal operation.

## 1 getline dispatch (`src/input.c` getline, `src/core/lineedit.c` gl_*, dispatch in `src/main.c` mainedit / `src/edit.c` menu)

Checked:

* Entry `getline`/`getline2`/`getline3` (5416–5431): `clr anyalt`,
  `cursor`, order `cursor`/`showhprop`/`getinput`/`cursor`; `a0/a1 =
  bitmaps+scloc` (`add.l`), `a2 = chmap + chloc` (`add.w`, C `(WORD)chloc`),
  `d0 = cursx`.
* Classification (5435–5442): `tst.l d7 / bmi`, `bclr #30 / bne .isasc`
  (bit is cleared), `cmp #32 / bcs` and `cmp #127 / bcc` unsigned
  on the low word – C `(UWORD)d7` correct.
* `.isasc` (5444–5482): `anyalt = d7.w`; block adjustment
  `inbloks`→`btopx+1`, `inblokf`→`bbotx+1`, `blokfix`; bitmap shifting
  (`edcols` loop, `bcc`, inner `dbf fonthite2` = fonthite2+1
  passes, `add.w bmapmul`, `sub.w fontmul`); chmap shifting from
  `cursx(register)+cursoff(fresh)`; `print`, `cursrite` with `d0 = cursx`.
  In C the chmap shifting comes before the bitmap shifting (gl_isasc before
  shiftright) – independent memory, `d0`/`a0`/`a1` are saved before the
  block adjustment (asm: `inbloks`/`inblokf`/`blokfix` change only
  d2/d3, `blokdis` saves d0/a0–a2). OK.
* `charlow` 31 (5484–5519): shift test `and #3`, `.tababove` (line 0 →
  getline2, `lastitem` = 4(a0), count from −1, empty line → getline2). The
  C guard "−1 = nothing" does not collide with valid columns. OK.
* `charnot31` 30 (5521–5541): `bpl` loop, `bcs curspos`, `subq`. OK.
* `charnot30` 8 (5543–5553): `or cursoff / beq`, `cursleft`, `d0 = cursx`
  anew, `d7 = 8`, to `char127`. OK.
* `charnot8` 9 (5555–5657): insert (`addq`, `bcc getline2`, `divu` remainder
  via `moveq #0/move d0,d1` = zero-extended), `anyalt` only after
  a tab stop was found; delete: `anyalt` **before** the loop,
  `subq/bmi`, block adjustment without `blokfix`; `.zap` loop (`bmi` on
  d3 signed, `bcs` d3<d2 unsigned), `.done` loop
  (`bmi`, `bcc`); `.loop5/.loop6` with `bcs`. `reline`: `add.w cursoff,a2`,
  `dbf edcols-1`, then `curspos` with the saved target column. OK.
* `char127` (5662–5716): `cmp btopx,d2` uses d2 = column from `inbloks`
  (C: `abscol() != (UWORD)btopx`), `bbotx+1 / bcc .skipmo`, `blokfixb` only
  after `inblokf`; shifting to the left; last visible cell from
  `a2[edcols+cursoff-1]` saved with `cursx`. OK.
* `charnot127`/`charnotgad` (5718–5737): `cmp #-1,d7`/`cmp #-2,d7` as
  word comparison (C `0xFFFF`/`0xFFFE`), `cmp #1,d6`, `HorizPot*chcols` →
  `swap`; `$f8e1` → delete rest of line, `anyalt = d6`. OK.
* `glinedone`: returns d7 (and d6) unchanged. `horizpos` (5742,
  `bcc` unsigned, `bgt` signed, `btst #0`), `cursleft`
  (5775), `cursrite` (5789): all comparisons cast correctly.
  `refresh` (5337) destroys d6/d7 – but is always followed by `getline2` →
  `getinput`, hence without consequence (C accordingly).
* `inbloks`/`inblokf`/`blokfix`/`blokfixb` (5360–5414): `bhi`, `bcs`,
  `bls`, `bpl`, `blt` (bbotx/btopx `WORD`) correct. "eq" return via
  `moveq #0,d3`: d3 is not read afterwards by any caller.
* Dispatch `mainedit` (142–177): `cmp.b d0,d7` – C compares
  `(UBYTE)d7` (so also Latin-1 `$FF` → gadget, survey-06 I.6, like the
  original). Table contents `jumptable` (6171) and `menutable` (6199)
  equal entry by entry (code, flag, routine, order,
  end markers 0 and 255 respectively). d6/d7 survive `charstobuff`/`makeline`/
  `showthisitem` in the asm as well. `menu` (1565): `and #15`, `lsr #1/and #$f0`
  (word), `cmp.b #4` → `extmenu` with the not yet or-ed d6,
  `cmp.b d6,d0`. OK.

Verdict: **OK.**

Finding minor (consistency, not reachable):
`src/core/lineedit.c:29` `chrow()` computes `chmap + chloc` (unsigned),
the asm `add chloc(pc),a2` (5430) sign-extends the word; so does
`src/input.c` (`(WORD)chloc`). Difference only from chloc ≥ 32768
(chmap is too small for that). Proposal: `return chmap + (WORD)chloc;`.

## 2 handled0 (`src/input.c`)

Checked (5866–6001):

* Classes as `cmp.l` (`$20`, `$40`, `$2000`, `$100`, `$8`, `$400`, `$200`).
* `.yesgad2`: `d6 = GadgetID` (38), `d7 = -1`; `cmp.b #4,17(a0)` =
  low byte of GadgetType (C `(UBYTE)`), `cmp #9,24(a1)` code; `btst
  #2,15(a0)` = low byte Activation & ENDGADGET; `cmp #$20,d1` as word (in the
  `.khere` path only the low word of d1 = `$40` is set – C sets d1
  wholly, same result). Order `ReplyMsg` → `closereq` → `d0=-1`.
* MENUPICK: `cmp #-1,d0` (MENUNULL) → `.again`, otherwise d6 = code, d7 = −2.
* MOUSEBUTTONS: `$68` → −3, everything else → −4.
* RAWKEY: `btst #7` → `.again`; `qualifier` before `rawed`; `moveq #0,d7 /
  move.b d0,d7 / beq .again`; in the requester `cmp #13,d0` / `cmp #27,d0`
  as word on the return value of rawed (C: `c`, UWORD – fits, rawed
  delivers a whole word). `.retloop` (first) and `.escloop` (last)
  BOOL ENDGADGET from `62(a0)` = FirstGadget. `germs`: `dbf ngerms-1` =
  ngerms pairs, hit → byte | bit 30 (C `germs_map`).
* CLOSEWINDOW → `.doesc` (hazard marked for `req_window = 0`).
* `.done`/`.again`: "ne" ⇔ d7 ≠ 0 in all `.done` paths (d7 is never
  0 there), hence the C return "d7" instead of flag is equivalent; `.again`
  sets d7 = 0. `getinput`/`getinput2` (5833–5864) accordingly.
* `refme` (6003): `or #$80,12(a0)`, RefreshGList(…,1), `Delay(4)`,
  `and #$ff7f`, RefreshGList again. OK.

Verdict: **OK.**

## 3 tokit / tokit2 / putchar_ / alpha (`src/core/tokit.c`, tokit2 in `src/core/fileio.c`)

Checked (4195–4343):

* `tokit4`: raw byte comparison with 32 (`$A0` ≠ space), `subq/bpl`
  (word); empty line: `addq #4,a7` (start discarded), `clr.b (a1)` without
  counting on, d5 = 1. `tokit2` → `tokit3` (first `subq`). OK.
* Main loop: `bclr #7` on every read; quote test before `invflag` test
  (quote = 255 never hits after `bclr` – as original); order `_` `.`
  `\` → remark → `$`; hex loop with `bcs/bhi/bls/bcs/bcc/bls`
  (all unsigned, values < $80). `.nothex` outputs the current character
  unchecked.
* `.tsttoke`: `subq #1,a2`, node layout next(0)/num(4)/name(6), `or.b
  #32` on both sides, `.found` checks the following character (`alpha` or `_` →
  next node), `.notoke` reads the character anew.
* `.putina3`: first write the two bytes (high byte | $80, low byte),
  then `add.w d4,d0` (d4 = a4−a2), `cmp d1,d0 / bhi pchover2`.
* `putchar`: `addq.w`, `cmp d1,d0 / bhi pchover` (unsigned) –
  macro `putchar_()` with `goto enddo` corresponds to `addq #4,a7` + `pchover2`.
* `enddo`: `clr.b (a1)+`, d5 = a1 − start.
* `alpha`: `bcs 65`, `bhi 122`, `bcs 91` → yes, `bcs 97` → no.

Verdict: **OK.**

## 4 readline (`src/core/fileio.c`)

Checked (3325–3453):

* `doread`: `subq.w d6 / bmi`, `move.b` + `cmp d0,d0` (N=0), `fillbuff`
  with Z return (`move.l d0,d6` or `moveq #0,d6`).
* Flags after `move.l (a7)+,a0`: the destination is an address register → flags
  remain those of `doread`; so `bmi .oops` / `bpl .loop` test doread.
  C accordingly.
* Token pair (`tst.b / bpl`): second byte unchecked, EOF in between →
  `.oops`, then directly `.nottab` (no LF/TAB test for the 2nd byte).
* LF → 0 and d4 = −1; `move.b d0,(a0)+ / beq .skip3`.
* TAB: `subq #1,a0`; column `sub.l`, counter `addq.w` (C: only low word
  incremented), `divu tabstop` on the whole longword, remainder test; spaces
  are written **before** the overflow test (BUG(orig) documented).
* `.nottab`: `cmp.l charbuff3,a0 / bcs .ok` otherwise `a0 = charbuff3−3`,
  `.oops`.
* `.oops`: `clr.b (a0)+`, d5 = length incl. 0, d0 = 0; `.skip3`: d0 = 1,
  no additional 0 byte. `.sit`: `tst d4`, `tokenise`, `subq.w #1,d5 /
  bpl`, `mytoke` (d1 = d5.w, in place), d0 saved via the stack
  (mytoke does not save d0).

Verdict: **OK.**

## 5 Blitter scrolls (`src/screen.c`) and chmap_* (`src/core/tokit.c`)

Checked:

* `getblitter` (4964): OwnBlitter, WaitBlit, AFWM/ALWM = $FFFF,
  BLTCON1 = 0. `putblitter`: WaitBlit, DisownBlitter.
* `scrollup` (4346–4431): d6 = edrows−d7−1 before `getblitter`; BLTCON0
  $9F0 before the chmap blit; AMOD/DMOD = blitmod; d4 = `mulu`(d6,fonthite)
  `lsl.w #6` | edcols/2; d2 = fonthite<<6 | edcols/2; d3 = `mulu`
  (edrows−1, fontmul). `scup2`: `cmp #64,d4 / bcs` (unsigned),
  `add.l d5`, `add.w fontmul` (C `(WORD)fontmul`), DPT → APT → SIZE →
  WaitBlit; fill line: DPT, ADAT = 0, block area (`bcs`/`bhi`) →
  $FFFF, then BLTCON0 $1F0, SIZE, WaitBlit. Second plane with another
  $9F0. Write order identical.
* `scrolldown` (4434–4528): `getblitter` **before** the d6 computation (like
  C), `beq eraseline`; $9F0 / BLTCON1 = 2 (descending); `scdown2`: `add.l
  d5`, `sub.w fontmul`, DPT/APT/SIZE/WaitBlit.
* `eraseline` (4479): AMOD, DMOD, then `move.l #$1f00000,$040` (BLTCON0
  before BLTCON1, like C), ADAT 0/$FFFF (`cursy` line = d7+curstop), both
  planes with WaitBlit; only reached from scrolldown (blitter already
  owned).
* `cls` (4986): BLTCON0 $1F0, chmap fill, ADAT 0, `cls2`
  (DPT, DMOD = blitmod, SIZE without waiting), WaitBlit, `cls2`, putblitter.
* chmap counterparts: `chmap_scrollup` – D = chmap + `mulu`(row,chcols),
  A = D + chcols, size `d6<<6 | (chcols/2 & 63)`, modulo 0; last line
  via `add.w` (C `(WORD)(UWORD)…`) with `chcols/2 | 64` (without `and #63`).
  `chmap_scrolldown` – last word pointer, A = D − chcols, size without
  `and #63` (one line more with even d6, reading before chmap
  caught), then `chmap_eraseline` (`and #63` | 64). `chmap_cls` –
  `edrows<<6 | (chcols/2 & 63)`. Height 0 = 1024, width 0 = 64 words
  (blt_rows/blt_bytes). Descending word copy ≡ descending byte copy with
  even distance chcols.

Verdict: **OK.**

Finding minor 5a (C UB only in the hazard case):
`src/core/tokit.c:379` and `src/core/tokit.c:407`: `(UWORD)(d6 << 6)` with
`WORD d6`; for d6 < 0 (row ≥ edrows, ORIG_HAZARD case, survey-05 I.13)
left-shifting a negative number is undefined in C. asm (4364/4453):
`lsl #6,d0` on the word. Proposal (both places):
`(UWORD)((UWORD)d6 << 6)`.

Finding minor 5b (width, practically not reachable):
`src/screen.c:132` `d5 = pixrows22 - (ULONG)(blitmod + 2);` – asm
4469–4472: `moveq #0,d0 / move blitmod,d0 / addq #2,d0 / sub.l d0,d5`;
`addq` is `.w`, i.e. (blitmod+2) is truncated to 16 bits. Difference only
for blitmod ≥ $FFFE. Proposal: `d5 = pixrows22 - (ULONG)(UWORD)(blitmod +
2);` (the same spot in `scrollrite`, `src/screen.c:533`, has the same
pattern, asm 5023–5027 – same correction; not part of the sample).

## 6 openreq / closereq (`src/reqs.c`)

Checked (3580–3670):

* `req = a0`; width/height via `move.l 8(a0)` + `add #12` / `add #15`;
  left = (edcols·8 − width) `lsr #1`, top = (newscreen.Height − height)
  `lsr #1` – unsigned (C `(UWORD)`), correct.
* `req_gads = ReqGadget (16)` before the test; `tst 40(a1)` = high word of
  UserData of the **first** gadget, `move #-1,40(a1)` sets only the
  high word (C `| 0xFFFF0000`), then `addq #6,LeftEdge` / `add #12,TopEdge`
  for the whole chain (do-while, like `bne.s .loop` after the first
  pass). Once-only only via the first gadget – as original.
* `req_screen = myscreen`, OpenWindow, error → `outmem`; ring swap
  `req_old2 ← req_old ← req_window ← new`; PrintIText(ReqText = 24) and
  DrawBorder(ReqBorder = 20) at (6,12) on `RPort` (50); `bra pointeron`.
* `closereq`: only with `req_window`: empty port (86), ClearPointer,
  CloseWindow, swap back, `req_old2 = 0`, `flushmw`; `makemenus` always.

Verdict: **OK.** (Note without need for change: a requester without
gadgets – `ReqGadget = 0` – reads address 40 in the original; C dereferences
NULL there as well. Not reachable in TED; the header comment already mentions it,
an `ORIG_HAZARD` would be ineffective in the Amiga part.)

## Summary of the proposals

| # | File:line | asm | Change |
|---|---|---|---|
| 1 | `src/core/lineedit.c:29` | 5430 `add chloc(pc),a2` | `return chmap + (WORD)chloc;` (consistency) |
| 5a | `src/core/tokit.c:379`, `:407` | 4365, 4454 `lsl #6` | `(UWORD)((UWORD)d6 << 6)` (no UB for d6 < 0) |
| 5b | `src/screen.c:132` (and `:524`) | 4471 `addq #2,d0` (.w) | `(ULONG)(UWORD)(blitmod + 2)` |

## Implemented (coordinator, phase 2c)

All three proposals are adopted: `chrow()` with `(WORD)chloc`,
`(UWORD)((UWORD)d6 << 6)` in `chmap_scrollup`/`chmap_scrolldown`,
`(ULONG)(UWORD)(blitmod + 2)` in `scrolldown` and `scrollrite`.

Further findings of the integration review (outside this sample,
details in [comdata.md](comdata.md), [deviations.md](deviations.md),
[changes.md](changes.md)):

* **32-bit division**: `showvprop`/`showhprop` (input.c) and the
  file requester (`fq_divu_w`) used C `/`/`%` → libnix20
  `__udivsi3`/`__umodsi3` → `UtilityBase` (never opened) = crash on every
  call. Now `divu_w` = `divu.w` in src/glue.c, ctest `checklink`.
* **ezfreq** (comdata+24): a3 after the call is in the original the address
  `reqstruct+12`, not its contents – corrected.
* **refgads** (comdata+28): d0 as whole register to `RefreshGList`.
* **openio** (comdata+60, `mycli`): d0/Z of `WBenchToFront` as in the
  original (via `callregs`, `REGSTUB_CCR`).
* Guard checks (`tokens == 0`, `freeall`, `freeupa5`,
  `chmap_scrolldown`) only in the host build; `swapd0bord` ≥ 16 and
  `outmem` with `memstack == 0` as in the original.
