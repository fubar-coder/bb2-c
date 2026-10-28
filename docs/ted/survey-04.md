# Survey 04 – ted.s:3063–4197

Scope: options (`l:blitzeditor.opts`), options requester helpers, file I/O and
line list (loading), token lookup/detokeniser, file requester bridge, modal
requester windows (`openreq`/`closereq`/`error`), search (`isithere`),
block helpers, line output (`showa2item`/`tokprint`), `charstobuff`.
Ends before `tokit2` (4195, only the entry is mentioned).

Abbreviations: `item` = line node (see E), `chmap` = character map of the
screen (`chcols` bytes per line), `PUBLIC` = MEMF_PUBLIC (1).

---

## A. Routines

### Options / options requester

- `loaddefs (3063)` : in – ; out – ; destroys d0–d3/d7/a0–a6, a5 ; Open(`defname`, 1005 MODE_OLDFILE), Read(defend-defstart = 50 bytes) raw to `defstart`, Close; return value of Read unchecked (partial/short read leaves the rest unchanged). Then a5=`defstart`, **tail `bra setdefs`** (even if Open fails).
- `savedefs (3078)` : Open(`defname`, 1006 MODE_NEWFILE), Write 50 bytes from `defstart`, **tail `jmp close(a6)`**; errors silently ignored.
- `setdefs (3092)` : in a5 = 50-byte options block (layout like `defstart`, see C) ; out – (tail `bra setprops`) ; destroys d0–d2, a0–a2, a5 (+25 words). Procedure:
  1. `tokens`==0 → `clr 6(a5)` (tokenise cleared in the **source block**!).
  2. Set gadget flags (offset +12, bit 7 = SELECTED): `defaultreqga7` ⇐ `(a5)==6` (6-pixel font), `ga8` ⇐ `6(a5)<<7` (tokenise), `ga9` ⇐ `8(a5)<<7` (makeicons) – value is ORed unchecked via `lsl #7` (see I).
  3. Colour selection gadgets: for i=0..3 (normcol, labcol, tokcol, propcol): `swapd0bord(alt_i | 4*i)`, `swapd0bord(neu_i | 4*i)` (old = current globals from `normcol`, new = `10(a5)+2i`) → old button "up", new one "pressed".
  4. `numtoascii2` of hscmarg/vscmarg/tabstop (18/20/22(a5)) into the string gadget buffers `defaultreqla47/48/49`.
  5. Copy 25 words a5 → `defstart` (a5 afterwards +50).
  6. `int1/2/3+2` (StringInfo.LongInt, low word) ⇐ hscmarg/vscmarg/tabstop.
  7. d0=propcol, `bra setprops`.
- `numtoascii2 (3140)` : like `numtoascii`, plus `clr.b (a0)` (a0 stays on the 0).
- `numtoascii (3144)` : in d0.w (unsigned), a0 = buffer ; out a0 behind the last digit, **no** null byte ; destroys d0–d2/a1. Decimal without leading zeros, 0 → "0". Used globally (2558, 3027, 5269, 5310).
- `powersof10 (3165)` : data in the code, `dc 10000,1000,100,10,1`.
- `setprops (3167)` : in d0 = colour register 0..3 ; sets `HorizPot` (PropInfo+2) of `defaultreqhp1..3` = `v*65536/15` from `palit[d0*3+k]`, v==15 → $FFFF (otherwise divu overflow) ; destroys d0–d2/a0/a1. Data only, no OS.
- `dispprops (3189)` : RefreshGList(`defaultreqga1`, `req_window`, `defaultreq`, 3) ; **tail `jmp`** ; a6=int.
- `propadj (3196)` : in a5 = `palit` entry (3 words r,g,b), d4 = colour register ; reads `HorizPot>>12` of the 3 prop gadgets; if ≠ (a5)/2/4(a5): write values back and **tail** `SetRGB4(&screen->ViewPort (myscreen+44), d4, r, g, b)` ; otherwise rts. No CC result.
- `dispgadd0 (3228)` : in d0 = index (0..) from `defaultreqga10`, step `ga11-ga10` ; RefreshGList(gad, req_window, defaultreq, 1), tail `jmp`.
- `swapd0bord (3238)` : in d0.w = button index (colour | 4*category, 0..15) ; swaps FrontPen/BackPen (Border+4/+5) of **both** borders of the button (`bo14 + d0*56` and `+28`) ; internal `bsr .skip` + fall-through (routine runs through `.skip` twice). Destroys d0 (mulu), d1, a0.
- `swapbords (3247)` : swaps pens of all 32 borders from `defaultreqbo14` (step 28) – restores the "all up" state after cancel/rebuild or inverts it.

### Confirmation / error / requesters

- `erase (3257)` : `bsr erase2`; on ne **`addq #4,a7` → returns into the caller of the caller** (aborts e.g. `fileopen`). On eq normal rts.
- `erase2 (3262)` : in `allalt` (text changed) ; out **Z=1 if OK** (not changed or gadget ID 1 chosen), otherwise Z=0 ; on OK additionally `mousealt=-1`. Opens `erasereq` via `openreq`, loop `getinput` until d7==-1 (gadget); closing is done by `handled0` (ENDGADGET). Z=1 via `cmp d6,d6`.
- `error (3511)` : **comdata+12** ; in a0 = error text (→ `errorpnt`, IText of `errorreq`) ; saves only d7 ; waits for gadget ID 1 (other gadgets: keep waiting) ; destroys d0–d6/a0–a2/a6 (getinput). Always returns.
- `openreq (3580)` : **comdata+0** ; in a0 = Intuition `Requester` structure (template only) ; out – (comment "return 0 if couldn't open" is wrong: failure → **`beq outmem`**, does not return, sets a7=`memstack`) ; destroys d0/d1/a0/a1/a6. Procedure: `req`=a0; NewWindow `reqwindow` W/H = Req.Width+12 / Height+15; Left=(edcols*8−W)/2, Top=(newscreen.Height−H)/2 (`lsr`, see I); FirstGadget = Req.ReqGadget (+16); **one-time patch relocation** of the gadgets (+6 x, +12 y), marked via UserData high word of the 1st gadget (+40 = -1); OpenWindow; window stack `req_old2←req_old←req_window←new`; PrintIText(rp, Req.ReqText(+24), 6, 12); DrawBorder(rp, Req.ReqBorder(+20), 6, 12); **tail `bra pointeron`**.
- `closereq (3643)` : **comdata+4** ; `movem.l d0-d1/a0-a2/a6` saved ; if `req_window`≠0: empty UserPort (GetMsg/ReplyMsg), ClearPointer, CloseWindow, stack back (`req_window←req_old←req_old2←0`), `flushmw` ; **always** `makemenus`. No CC result.
- `flushmw (3672)` : empties UserPort of `mywindow` ; destroys d0/a0–a2/a6. Also called from 4903.

### File requester bridge

- `getbname (3485)` : in a2 = title ; a0=`blokpath`, a1=`blokname` → `getfname3`.
- `getfname (3489)` : a2=`filetext`, falls through into `getfname2`.
- `getfname2 (3490)` : in a2 = title ; a0=`filepath`, a1=`filename`, falls through into `getfname3`.
- `getfname3 (3493)` : `bsr ezfreq`; ne → d7 = d0 (pointer to full file name) and rts ; Z=1 and d0==1 → **`beq outmem`** (no memory) ; otherwise (cancel) **`addq #4,a7` → caller of the caller**. Three entries (getfname/getfname2/getfname3 + getbname).
- `ezfreq (3502)` : **comdata+24** ; in a0 path buffer, a1 name buffer, a2 title ; fills `reqstruct` {a0,a1,a2,0}, a0=`reqstruct`, **tail `bra filerequest+36`** (code in `incbin filereq`, see D) ; out d0 (name / 0 / 1), Z=1 on cancel/error (set by the filereq code).

### File I/O and loading into the line list (core)

Register convention during a load: **d7 = FileHandle, a5 = read pointer in `dosbuff`, d6 = remaining bytes in the buffer** (the caller must not touch any of the three between `doopen` and `doclose`).

- `doopen (3307)` : in d7 = file name, d2 = mode (1005/1006) ; out d7 = handle, `filehand` = handle ; on failure **`addq #4,a7` + tail `bra error`** (error1 text) → returns after the message into the caller of the caller. Buffer is **not** filled (despite the comment) – caller calls `fillbuff`.
- `doclose (3302)` : `filehand=0`, **tail** Close(d7).
- `fillbuff (3337)` : a5=`dosbuff`, d6 = Read(d7, dosbuff, doslen=2048) ; out **Z=1 if 0 bytes (EOF) or error** ; error (d0<0): `error(error2)`, d6=0. Destroys d0–d3/a0/a1/a6.
- `doread (3325)` : out d0.b = next byte, **N=0,Z=1** (`cmp d0,d0`) ; EOF: d0=-1, **N=1**. Callers only test `bmi`. Refills via `fillbuff` when needed.
- `readline (3375)` : reads a line into `charbuff2` ; out d5 = length **incl. null byte** (possibly after tokenisation), d0 + CC: **N=1 (d0=-1): nothing more read (EOF)** ; **Z=1 (d0=0): line without line end** (EOF in the middle of the line or buffer overflow) ; **Z=0,N=0 (d0=1): normal line**. Rules:
  - Byte with bit 7 → token pair: take over this and the **next byte unchecked** (also 0/10/9).
  - LF (10) → becomes 0, sets d4=-1 ("ASCII file, tokenise") and ends the line ; a 0 byte ends the line without d4 (tokenised format).
  - TAB (9) → spaces up to column ≡ 0 mod `tabstop` (column = bytes since line start, token pairs count 2).
  - Overflow: end at `charbuff3`; then a0=`charbuff3-3` → line truncated (max. 127 characters + 0), the rest of the file line becomes the next line.
  - Tokenise only if d4≠0 **and** `tokenise`≠0: d5-- (without 0), `mytoke` (in place in charbuff2, new d5 incl. 0).
  - CR is not handled (stays in the text).
  - CC return comes from `move (a7)+,d0` at the end.
- `mytoke (3447)` : saves d1–d4/d6–d7/a0–a6 ; d1=d5 (length without 0), a1=a2=`charbuff2` → `tokit2` (4195; in place) ; out d5 = new length incl. 0 ; d0 destroyed.
- `allocline (3349)` : in d5 = length incl. 0, `charbuff2` ; AllocMem(d5+lenitem, PUBLIC) via `alloc2` (failure → `outmem`, does not return) ; sets `nextitem=0`, `numchars=d5.b`, copies up to and incl. 0 ; **`lastitem` stays uninitialised** (caller sets it: `fileopen` explicitly, `linka0`) ; out a0 = new item. First non-space == `label` → **tail `beq remember`** (stub: `mousealt=-1`). Destroys d0/d1/a1/a6.
- `linka0 (3277)` : in a0 = new item, a4 = predecessor ; links a0 in behind a4, `numlines++`, possibly `enditem=a0` (then does **not** set `nextitem(a0)` – relies on allocline/allocnull) ; out a4 = a0, `thisitem` = a0. Destroys d0/a1.
- `bloadadd (3293)` : appends `charbuff2` (token-pair-aware, incl. 0) to a3 ; out a3 points **to** the null byte. Only from `blokload`.

### Token lookup / detokeniser (core)

- `detok (3455)` : in a0 = tokenised line (0-terminated), a1 = destination ; out destination 0-terminated, a1 on the 0, a0 behind source ; destroys d0/a3. No length limit (see I). Recursion only via `bra detok` (loop).
- `findtoke (3472)` : in d0.w = token word (bit 15 is cleared) ; out **a3 = pointer to name** (0-terminated) ; unknown → `tokeerr` ("?????"). List `tokens` (from edstruct): node {+0 next.l, +4 number.w, +6 name}. Requires `tokens`≠0. No CC result.
- `detoka5 (3759)` : in a5 = item ; detokenises `chars(a5)` to `charbuff3+1`, **without 0 terminator**, length byte `charbuff3[0] = n+1` ; destroys d0/a0/a1/a3. Local `bsr .loop` (subroutine within the subroutine).

### Search

- `isithere (3523)` : d5=0, falls through into `isithere2`.
- `isithere2 (3530)` : in a4 = item, d5 = start column, search text `findreqla16` (64 bytes, 0-terminated), option "match case" = bit 7 of `findreqga8+13` (gadget flags SELECTED) ; tokenised → first `detok` to `charbuff` ; out **d0.w = found column (N=0) or -1 (N=1)** ; destroys d0/d2/a0–a3/**a5**. Comparison: letters (according to `alpha`/`alphad0`) upcased with `and #$DF` if not case-sensitive. Empty search text → immediate match at d5.

### Line list: freeing, replacing, navigation

- `freeupa5 (3684)` : in a5 = item ; unlink (`firstitem`/`enditem` adjusted, `numlines--`), label line → `mforget2` (stub), FreeMem(numchars+lenitem), `memalt`=size ; **tail `jmp freemem`**. Special case `numlines==1`: `afirst2` (new empty line becomes first/last/this), then free a5, `numlines` stays 1. **`thisitem` is otherwise not adjusted** (caller). a1 = freed node.
- `calcitem (3783)` : a4 = `firstitem` + (curstop+cursy) steps, `thisitem=a4` ; no null check ; destroys d0.
- `makeline2 (3794)` : d5 = a3−charbuff+1, `bra makeline`.
- `makefirst (3799)` : a2=`charbuff2`, `charstobuff`, falls through into `makeline`.
- `makeline (3803)` : in a4 = item to replace (**must be == `thisitem`**), `charbuff` = new content, d5 = length incl. 0 ; allocate new item (alloc2/outmem), label → `remember`, link in place of a4 (first/end updated), copy text, **`freethis`** (frees `thisitem`), a4=`thisitem`=new. Destroys d0/d1/a0/a1/a6.
- `allocfirst (3041)` : `afirst2`, **tail `bra cls`**.
- `afirst2 (4043)` : `allocnull` (10 bytes, empty line) ; out Z=1 on failure (then nothing set), otherwise first/end/this = a0, Z=0.

### Block

Block coordinates (globals, line number absolute = curstop+cursy, column in detokenised characters): `btopx/btopy`, `bbotx/bboty`; `bboty==-1` = no block.

- `bloksetup2 (3727)` : in a4 = this item ; out **a5 = item of line `btopy`, d6 = btopy, d7 = curstop+cursy** ; walks via next/last to the block start line (unsigned comparisons). Ends via global label `found (3750)` (only rts, no foreign entry).
- `bloksetup (3752)` : `bloksetup2`, d5=`btopx` ; tokenise → **tail `bra detoka5`**.
- `getbchar (3849)` : state **a5 = item, d6 = line, d5 = column (0-based before increment)** ; out d0 = character and CC: **N=1 (d0=-1): block end**, **Z=1 (d0=0): line end** (a5=next, d6++, d5=0 – caller must call `detoka5` again with tokenise), otherwise character. End = (d6>bboty) or (d6==bboty && d5>bbotx); `d5>=chcols` → line change. Behind the line text: `blokop`≠0 → line change, otherwise spaces (padding up to chcols).
- `getbchar2 (3865)` : second entry without block end check (from 1617/1636).
- `newline (3884)` : global label, only from getbchar.
- `bloken (3890)` : set block menu items (`menu4item1/2/4/a/b`, Flags+12 bit 4 ITEMENABLED), `bltext`="block", falls through into `blokprnt2` ; destroys registers (pstats).
- `blokprnt2 (3899)` : a1=`statsloc`+44, **tail `bra pstats`** (status line).
- `blokdis (3903)` : opposite; `bstx=-1`, `bboty=-1`, `bltext`=5 spaces ; saves d0/a0–a2 only around blokprnt2.
- `blokadj (3919)` : in d0 = inserted line number ; if block active and d0≤bboty: bboty++, and if d0≤btopy: btopy++ (unsigned).
- `showprops2 (3929)` : in d2/d3 = column/line ; sets cursx=clamp(d2−cursoff, 0, chcols−1), cursy=d3 temporarily, `showhprop`+`showvprop`, restores cursx/cursy (as long) and d2/d3/d5.
- `sortblox (3947)` : normalises btop ≤ bbot (line unsigned, column signed), loads d0–d3 = btopx,btopy,bbotx,bboty (before the swap!; ordered after the swap).
- `orderem (3965)` : the same only in registers d0/d1 ↔ d2/d3.
- `invertarea (3974)` : `orderem`, falls through into `invertarea2`.
- `invertarea2 (3977)` : in d0,d1 → d2,d3 (column/line, d0/d2 absolute columns) ; inverts character by character (`not.b`) directly in **both** bitplanes `bitmaps[0..1]`, `fonthite2+1` pixel lines per character, running-text area (beyond line end up to `edcols`, then next line from column 0) ; columns <cursoff skipped. Destroys d0–d4/a0/a1. No OS – **direct chip RAM access**.
- `getmousexy (4015)` : reads `myscreen` MouseY(+16)/MouseX(+18) ; out d0=x, d1=y, d2=x>>3, d3=(y−topskip)/fonthite (**high word = remainder of divu!**) and N=0 ; outside the text area: d3=-1, **N=1**.

### Screen output of the lines

- `showall (4050)` : `storepos`, `cls`, from `thisitem` cursy lines back (no null check), shows up to `edrows` lines (godown), **tail `bra restorepos`** ; destroys **d7**, d0/d1/a2.
- `showtopitem (4073)` : output the line in screen row 0 anew (cursy steps back) ; if a predecessor is missing → nothing. Tail restorepos.
- `showbotitem (4088)` : output the line in screen row edrows−1, if present. Tail restorepos.
- `showthisitem (4102)` : a2=`thisitem`, falls through into `showa2item`.
- `showa2item (4104)` : in a2 = item, cursy/chloc = target row ; writes `chmap[chloc + d2]` and draws via `print` (cursx=d3, `color`) ; d2 = character map column, d3 = screen column ; 1st character == `label` → `normprint` (not as token), otherwise everything via `tokprint` ; saves cursx on the stack. Fall-through/`beq` into `endofline`.
- `endofline (4124)` : if the line is in the block (btopy..bboty): fill the rest of the screen row with spaces via `print` ; `cursx` from the stack (stack pair with showa2item – **can only be jumped to from showa2item**). Does **not** clear the rest of `chmap`.
- `normprint (4142)` : `chmap[d2]=d0`, falls through `normprint2`.
- `normprint2 (4143)` : only if d2≥cursoff and d3<edcols: `print` at cursx=d3, d3++.
- `tokprint (4152)` : tokenise && bit7 → token word from d0 and next byte (a2++), `findtoke`, `color=tokcol`, name character by character: `chmap[d2]=c|$80` (bit 7 marks token characters), `normprint2`, d2++ ; afterwards `color=normcol`, d2-- ; otherwise `normprint`. **Destroys a5** (exg trick, a5 = name pointer afterwards).
- `charstobuff (4173)` : in a2 = source (chmap line, `chcols` bytes) ; tokenise → **tail `bra tokit`** (with stack argument (a7)=charbuff, d1=chcols−1, a1=charbuff) ; otherwise copy to `charbuff`, trailing spaces trimmed, 0 appended (label `enddo 4190`, also end of `tokit`) ; out d5 = length incl. 0. **Stack coupling**: `move.l a1,-(a7)` here, `sub.l (a7)+,d5` in `enddo` – tokit must end in `enddo` with exactly this stack word.
- `tokit2 (4195)` : `move.l a1,-(a7)`, `bra tokit3` (boundary, rest in the next chunk).

---

## B. Persistent register conventions

| Register | Meaning | Valid in |
|---|---|---|
| d7 | FileHandle (or before that file name for `doopen`) | doopen…doclose, readline, fillbuff |
| a5 | read pointer in `dosbuff` | doread/fillbuff/readline (loading) |
| d6 | remaining bytes in `dosbuff` | doread/fillbuff/readline |
| d5 | line length incl. 0 | readline → allocline; charstobuff → makeline |
| a4 | current/previous item | linka0, makeline, isithere, bloksetup2, calcitem |
| a5, d6, d5 | block cursor: item, line, column | bloksetup → getbchar(2) → detoka5 |
| a2/a3/d2/d3 | output: source, chmap line, chmap column, screen column | showa2item/normprint/tokprint |
| a6 | not preserved; macros `calldos`/`callint` load dos/int afresh | everywhere |

Collisions: a5 is at the same time file pointer (loading) and block item (block save 1683ff. uses `bloksetup` + `doopen` – `fillbuff` is not called there, writing does not go via a5). `tokprint` and `isithere2` destroy a5.

Called by blitz2 (comdata): `openreq` (+0, **saves nothing**), `closereq` (+4, `movem d0-d1/a0-a2/a6`), `error` (+12, saves only d7), `ezfreq` (+24, filereq code saves d1–d7/a0–a6 via `movem` at code start+4). blitz2 wrappers must assume that openreq/error destroy d0–d6/a0–a6.

---

## C. Globals defined in this scope

| Label | Line | Type | Init | Purpose |
|---|---|---|---|---|
| `powersof10` | 3165 | 5×w | 10000..1 | numtoascii |
| `reqstruct` | 3509 | 4×l | 0 | parameters for filereq code {path, name, title, 0} |
| `reqwindow` | 3565 | NewWindow (48 B) | see below | template requester window |
| `req_gads` | 3569 | l (in reqwindow, +18) | 0 | NewWindow.FirstGadget |
| `req_screen` | 3572 | l (reqwindow+30) | 0 | NewWindow.Screen |
| `req_old`, `req_old2` | 3577/3578 | l | 0 | stack of open requester windows (depth 3 incl. `req_window`) |

`reqwindow`: Left/Top/Width/Height are overwritten on every `openreq` (offsets 0/2/4/6); IDCMP = $20|$40|$200|$400 (GADGETDOWN, GADGETUP, CLOSEWINDOW, RAWKEY); Flags = 2|8|$1000 (WINDOWDRAG, WINDOWCLOSE, ACTIVATE); Type 15 (CUSTOMSCREEN); title 0.

Defined outside, used substantially here (for headers):
- Options block `defstart..defend` (ted.s:6099, 50 bytes = 25 words, **file format of `l:blitzeditor.opts`**, raw, big-endian):
  `+0 fonthite(8)` `+2 fonthite2(7)` `+4 fontmul(0)` `+6 tokenise(0)` `+8 makeicons(1)` `+10 normcol(1)` `+12 labcol(2)` `+14 tokcol(3)` `+16 propcol(0)` `+18 hscmarg(12)` `+20 vscmarg(6)` `+22 tabstop(2)` `+24 palit 4×(r,g,b)` = (8,8,8)(15,15,15)(0,0,0)(15,13,0) `+48 origscrows(0)`.
- Buffers (ted.s:41–63, one AllocMem): `dosbuff` 2048, then `charbuff`, `charbuff2`, `charbuff3` each `chcols+2` (=130), `charbuff4` = end of the allocation (size 0!, see I).
- Options requester data (ted.s:7310ff.): `defaultreqhp1..3` PropInfo (Flags 11, HorizPot +2 is written), `defaultreqga1` list of the 3 prop gadgets, `defaultreqga4..6` integer string gadgets with StringInfo (`int1..3` = StringInfo.LongInt, `defaultreqla47..49` buffers, MaxChars 3), `defaultreqga7/8/9` toggle gadgets (Flags +12 bit 7 SELECTED patched), `defaultreqga10..` colour buttons (step `ga11-ga10`), borders `defaultreqbo14..` 16 buttons × 2 borders of 28 bytes (Border 16 + 6 coordinate pairs), pens at +4/+5 are swapped.
- Menu: `menu4item1/2/4/a/b` Flags (+12) bit 4.
- Search requester: `findreqla16` (search text), `findreqga8` Flags (+12/+13 bit 7).

---

## D. Self-modifying code / data in the code

- **`bra filerequest+36`** (3507): entry into `incbin filereq` (ted.s:7967). File = AmigaDOS hunk: 24 B HUNK_HEADER + HUNK_CODE ($3E9) + length → code begins at +32. Code+0 = `lea $b3c(pc),a0` (built-in default parameter block), **code+4 (= +36) = `movem.l d1-d7/a0-a6,-(a7)`, `move.l a0,a5`** → ezfreq passes its own block in a0. Relocated at runtime by `flink` (7953); in C: rewrite the file requester or load the hunk at runtime (GLUE).
- **`openreq` patches the passed requester data**: gadget Left/Top of all gadgets +6/+12 on first opening, marker `UserData` high word (Gadget+40) of the first gadget = -1. Also affects requesters from blitz2 (comdata+0) – the structures are altered after the first call.
- Static structures written at runtime (not code): `reqwindow`, gadget flags (`ga7/8/9`+12), PropInfo HorizPot, StringInfo LongInt/buffer, border pens, MenuItem flags, `bltext`. All read via `(pc)` addressing, but written absolutely – in C normal `static` data.
- `powersof10`, `reqstruct`, `reqwindow`, `req_old*` sit between code (PC-relative).
- No writing to instructions in this scope.

---

## E. Heap/list structures

**Line node (item)** (ted.s:16–20, `lenitem` = 9):

| Off | Size | Field | Meaning |
|---|---|---|---|
| 0 | l | `nextitem` | next line / 0 |
| 4 | l | `lastitem` | previous line / 0 |
| 8 | b | `numchars` | length **incl.** null byte (≤255; FreeMem size = numchars+9!) |
| 9 | n | `chars` | text, tokenised (token = 2 bytes, 1st byte bit 7), 0-terminated |

- Allocation `numchars+lenitem`, MEMF_PUBLIC, via `alloc2` (sets `memalt=-1`, on 0 → `outmem`). Empty line `allocnull`: 10 bytes, `numchars=1`, `chars[0]=0` (word write access `$100` at offset 8).
- Global list heads: `firstitem`, `enditem`, `thisitem`, `numlines` (word). Invariant: never empty (at least one line; `freeupa5` creates a replacement).
- Inserting: `linka0` (after a4), replacing: `makeline` (a4==thisitem), removing: `freeupa5`/`freethis`.
- Label lines (first non-space == `label`, default 255 = off): `remember`/`mforget2` are only stubs here (`mousealt=-1`); the mouse label list is later rebuilt completely (`showmice`, 2916).

**`dosbuff` buffer**: 2048 bytes, state (a5, d6) in registers, `filehand` global (for `outmem` cleanup).

**charbuff areas** (130 bytes each, contiguous):
- `charbuff`: line to insert/replace (makeline), detok destination of `isithere`.
- `charbuff2`: read line (readline, in-place tokit), source of `makefirst`.
- `charbuff3`: length byte + detokenised block line (`detoka5`), overflow limit of readline.

**Token list** (`tokens`, from edstruct): singly linked {next.l, number.w, name\0}.

**Requester window stack**: `req_window` / `req_old` / `req_old2` (no heap structure, windows from Intuition).

---

## F. External calls

| Lib | LVO | Where | Purpose |
|---|---|---|---|
| dos | Open/Read/Write/Close | loaddefs, savedefs, doopen, fillbuff, doclose | options file, source text, block |
| exec | AllocMem (via alloc2/allocnull), FreeMem | allocline, makeline, freeupa5 | line nodes |
| exec | GetMsg/ReplyMsg | closereq, flushmw | emptying ports |
| intuition | OpenWindow, CloseWindow, ClearPointer, PrintIText, DrawBorder | openreq, closereq | requester windows |
| intuition | RefreshGList | dispprops, dispgadd0 | options requester (with `defaultreq` as requester argument!) |
| graphics | SetRGB4 | propadj | palette (ViewPort = Screen+44) |
| Filereq | `filerequest+36` | ezfreq | file selection |

- Internal UI calls from this scope: `getinput` (erase2, error), `pointeron` (openreq), `makemenus` (closereq), `print`, `cls`, `storepos`/`restorepos`, `godown`, `pstats`, `showhprop`/`showvprop`, `outmem`, `remember`/`mforget2`, `freethis`, `allocnull`, `alpha`/`alphad0`, `tokit`/`tokit2`.
- **Direct memory access**: `invertarea2` writes via `not.b` into the bitplanes (`bitmaps`, chip RAM), `getmousexy` reads screen fields (+16/+18), `showa2item` writes into `chmap`. No `$dff000`, no blitter, no interrupts in this scope.
- Callbacks into blitz2 (edstruct): none in this scope (`loadxtra` only in `fileopen`, outside).
- comdata entries here: +0 openreq, +4 closereq, +12 error, +24 ezfreq.

---

## G. Special handling in C

1. **Abort via the stack** (`addq #4,a7`): `erase`, `getfname3`, `doopen` (there + `bra error`). In C: return value (bool) and `if (!erase()) return;` in every caller (fileopen, blokload, …). Check exactly which callers are affected (the callers' callers are partly menu handlers with stack setup).
2. **`outmem`** sets `a7=memstack` and jumps to `memcont` (main loop): non-local jump → `longjmp` (in `alloc2`, `openreq`, `getfname3`). Caution: a call via comdata from blitz2 (openreq) would discard the blitz2 stack → in C own error path (return 0) for the comdata wrapper.
3. **CC returns**: readline (N/Z three-valued → `enum {RL_EOF=-1, RL_NOEOL=0, RL_OK=1}`), doread (−1/byte), fillbuff (bool), erase2 (bool), isithere2 (int, −1), getbchar (−1/0/character – character ≥$80 in non-token mode: see I), getmousexy (bool + 4 outputs), afirst2 (pointer/NULL), ezfreq (Z from foreign code).
4. **Register state across calls** (d7/a5/d6 when loading, a5/d6/d5 during block traversal): in C as struct `struct rdstate {BPTR fh; UBYTE *p; LONG left;}` or `struct bcur {item *a5; UWORD y, x;}`.
5. **`charstobuff` → `tokit` → `enddo`** share a stack word (buffer start); in C: tokit returns the length, `charstobuff` calls it normally.
6. **`showa2item`/`endofline`** share the saved `cursx` on the stack → in C one function.
7. comdata wrappers (asm stub, GLUE): openreq/closereq/error/ezfreq with a0/a1/a2 parameters, register saving as in the original (closereq: d0–d1/a0–a2/a6; error: only d7 – stub may save more).
8. `ezfreq` → filereq code (incbin): GLUE or reimplementation (asl/reqtools would be a behavioural change).
9. Pitfalls:
   - `numchars` is a **byte**, length incl. 0; FreeMem size computed from it → keep `UBYTE` in C.
   - `move #$100,numchars(a0)` = byte pair write (numchars=1, chars[0]=0).
   - `getmousexy` d3: high word contains remainder (divu) – use only the word.
   - `move.l cursx,-(a7)` saves cursx **and** cursy (adjacent words) – in C both.
   - Token word: `(hi<<8|lo) & 0x7FFF` (findtoke `bclr #15`).
   - Comparisons: btopy/bboty comparisons unsigned (`bcs/bhi/bls`), columns partly signed (`bge`, `bgt`, `ble`); `bboty==-1` → $FFFF unsigned (blokadj checks explicitly beforehand).
   - `setdefs` `lsl #7` on flag words: only 0/1 allowed.
   - Gadget flags bit 7 is tested partly at byte +13 (`btst #7`), partly at word +12 (`$80`) – identical (big-endian).
   - `tokprint` destroys a5, `showall` destroys d7.
   - `setprops` divu overflow at 15 → keep special case $FFFF.

---

## H. Classification

| Routine | Class | Remark / cut point |
|---|---|---|
| loaddefs, savedefs | KERN (file) | afterwards `setdefs` (UI part) |
| setdefs | UI | data part (copy to defstart, LongInt) separable: `defs_apply(src)` + hook `defs_ui_sync()` for gadget flags/borders/buffers/props |
| numtoascii(2) | KERN | |
| setprops | UI data (no OS) | formula KERN-capable |
| dispprops, dispgadd0, propadj | UI | |
| swapd0bord, swapbords | UI data | |
| erase, erase2, error | UI | abort semantics see G1 |
| openreq, closereq, flushmw | UI | |
| getbname/getfname*/ezfreq | UI/GLUE | filereq incbin |
| doopen, doclose, fillbuff, doread | KERN (file) | error → `error` (UI) via callback |
| readline | **KERN** | tabstop, tokenise as parameters; mytoke = tokeniser core |
| mytoke | KERN | |
| allocline, linka0, bloadadd | **KERN** | `remember` = set flag (hook) |
| detok, findtoke, detoka5 | **KERN** | |
| isithere(2) | **KERN** | search text/option as parameters instead of gadget data |
| freeupa5, calcitem, makeline(2), makefirst | **KERN** | `memalt`/`mousealt` flags = display hints; makefirst depends on charstobuff |
| afirst2 | KERN | |
| allocfirst | KERN + UI | split off `cls` |
| bloksetup(2), getbchar(2), newline | **KERN** | |
| sortblox, orderem, blokadj | KERN | |
| bloken, blokdis, blokprnt2 | UI | separate block state (`bboty=-1`, `bstx=-1`) from menu/status line |
| showprops2 | UI | |
| invertarea(2), getmousexy | UI | |
| showall, showtopitem, showbotitem, showthisitem, showa2item, endofline, normprint(2), tokprint | UI | filling `chmap` (character map, bit 7 = token character) is core logic: `render_line(item, chmap_row)` + `print` callback |
| charstobuff | KERN | source is chmap line (buffer) – host-testable |

---

## I. Peculiarities / possible bugs (BUG(orig) candidates)

1. **readline: last line without LF is not tokenised** (3424ff.): d4 is only set in the LF path (→ `.skip3`); in the `.oops` path (EOF/overflow) d4 is always 0. ASCII files without terminating LF: last line stays plain text; likewise truncated overlong lines.
2. **readline: bytes ≥$80 in ASCII files** (ISO Latin-1 umlauts) are treated as token pair: the following byte is taken over unchecked, also LF/0/TAB → line break after an umlaut at the line end is lost (two lines merge), TAB is not expanded.
3. **readline: token start byte directly before EOF** → line ends `…$8x,0`; `detok`/`tokprint` read the null byte as token low byte and run past the line end.
4. **readline: `divu tabstop`** – `tabstop` comes unchecked from the options file or the integer gadget (`int3`); 0 → division-by-zero trap.
5. **readline: tab expansion before the overflow check** – can write up to `tabstop−1` bytes beyond `charbuff3` (into charbuff3, harmless, except with a large tabstop: then past the end of the allocation).
6. **readline: buffer overflow returns Z=1 ("no EOL")** – `blokload` (1772ff.) aborts loading on Z=1 (`done4`): a too long line in the block ends the block import; the rest of the file is ignored. `fileopen`, by contrast, keeps loading.
7. **CR (13) is not removed** (DOS/Windows files keep CR in the text).
8. **detok / detoka5 without length check**: a tokenised line of up to 127 bytes can detokenise to >130 bytes → `isithere2` overwrites `charbuff2`; `detoka5` writes behind `charbuff3` into `charbuff4`, which **lies at the end of the allocation** (ted.s:41–63 reserves only 3×(chcols+2)) → heap overwrite. Length byte `charbuff3[0]` overflows at >254.
9. **getbchar (non-token mode)**: `move.b chars-1(a5,d5),d0` sets N for characters ≥$80 → callers interpret umlauts as **block end** (bmi). Token mode (`tst d0` on word) is correct.
10. **isithere2: start column d5 > line length** → search begins behind the null byte, reads up to the next 0 in memory (possible false matches/reading foreign data). Check depending on the callers (938/974: continue searching after a match).
11. **freeupa5 at numlines==1**: if `allocnull` fails, `firstitem/enditem/thisitem` stay on the node just freed (dangling pointer).
12. **makeline frees `thisitem`, but replaces a4** – tacit precondition a4==thisitem; every caller with a4≠thisitem would have leak + use-after-free.
13. **allocline leaves `lastitem` uninitialised**; `linka0` does not set `nextitem(a0)` when inserting at the list end (relies on allocline).
14. **openreq**: comment "return 0 if couldn't open" wrong – failure → `outmem` (stack reset; fatal from blitz2 via comdata). Centring with `lsr` → with a requester wider than `edcols*8` or taller than the screen: huge coordinate → OpenWindow fails. Requester without gadgets: `tst 40(a1)` with a1=0 reads address 40. More than 3 nested requesters: the oldest window is lost from the stack (leak). `req` is not reset on `closereq`.
15. **Gadget relocation in openreq** is marked once via UserData of the 1st gadget; if a gadget set is shared among several requesters or has UserData≠0, the shift is omitted/doubled.
16. **RefreshGList with `defaultreq` as requester argument** (dispprops, dispgadd0), although the gadgets hang in a window (openreq builds a window, not a requester) – apparently works, but is formally wrong.
17. **setdefs**: `clr 6(a5)` modifies the source block (with loaddefs = `defstart` itself, on cancel of the options requester = copy in `charbuff`); flags via `lsl #7` without masking (value ≠0/1 from a damaged options file sets foreign gadget flag bits). `fontmul`, `fonthite`, `origscrows` are taken over raw from the file (fontmul depends on `bmapmul` at save time).
18. **loaddefs** checks neither Read result nor file size; **savedefs** does not report errors.
19. **showall/showtopitem** assume that cursy predecessors exist (showall without null check).
20. **bloken/blokdis** change MenuItem flags without ClearMenuStrip/SetMenuStrip (common in 1.3, but formally unclean).
21. `numlines` is a word (>65535 lines overflows) – theoretical.
22. Tab column computation in readline counts token pairs as 2 columns (wrong indentation in tokenised files with TABs).
