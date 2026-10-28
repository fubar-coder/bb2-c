# Survey 07 – ted.s:6286–8072 (Intuition data, menus, requesters, incbins, `flink`)

Scope: almost only data. The only code: `flink` (7953) plus the entries into the
incbin blocks `rawed` and `filerequest`. Addresses from `build/ref/ted.labels` (here `build/amiga/programs/ted/ref/ted.labels`).

| Label | Address | Line |
|---|---|---|
| newscreen | $6316 | 6286 |
| newwindow | $63EE | 6316 |
| newgads | $648E | 6346 |
| menu1 | $653A | 6383 |
| findreq | $6D4A | 6906 |
| undobuffer | $7320 | 7105 |
| rawed (incbin, 660 B) | $73E0 | 7109 |
| repall | $7674 | 7111 |
| errorreq | $78C6 | 7195 |
| erasereq | $7A02 | 7239 |
| defaultreq | $7BFA | 7310 |
| alertstuff | $8C76 | 7948 |
| flink | $8CBE | 7953 |
| filerequest (incbin, 15916 B = $3E2C) | $8CE0 | 7967 |
| prtreq | $CB0C | 7971 |
| font6 (768 B) / font8 (1024 B) | $CDFA / $D0FA | 8070/8071 |

---

## A. Routines

| Routine | in | out | destroys | Notes |
|---|---|---|---|---|
| `flink` (7953) | – | – | d0–d2, a0, a1 | Once at startup (`jsr flink`, ted.s:76, before the branch external/internal). Relocates the incbin block `filerequest` at runtime (see D). Not idempotent. |
| `rawed+32` (= code+0) | – | – | d0–d2, a0–a3, a6 | Init: open console.device, build tables (ted.s:39). No return value, no error check. |
| `rawed+32+4` | d0.w = rawkey (key-down only, caller filters bit 7), d1.w = qualifier (ted.s:5926 `move 26(a1),d1`) | d0.w = character (0 = none), Z not reliable (caller does `move.b d0,d7 / beq`) | d1; saves a0–a2/a6 | Rawkey→ASCII (ted.s:5928). |
| `rawed+32+8` | – | – | a1, a6, d0/d1 | CloseDevice (ted.s:128 `fail5`). Tail `jmp` into exec. |
| `filerequest+36` (= code+4) | a0 = `reqstruct` {path.l, name.l, title.l, screen.l} | d0 = pointer to result (full path), **Z=1 ⇔ d0=0** (cancel) | only d0 (d1–a6 saved via MOVEM) | Entry from `ezfreq` (ted.s:3507) as **tail call** (`bra`), returns directly to its caller. |
| `filerequest+32` (= code+0) | – | – | – | Alternative entry with internal default struct (code+$B3E); **not** used by TED. |

Caller context (outside the scope, only for the interface):
* `ezfreq` (ted.s:3502): `a0`=path buffer, `a1`=name buffer, `a2`=title → writes
  `reqstruct+0/4/8`, `reqstruct+12` = screen (set in `screenopen` ted.s:4675),
  `lea reqstruct,a0`, `bra filerequest+36`. Destroys a3 (→ reqstruct+12) and a0.
  Exported as `comdata+24` to blitz2.
* `getfname3` (ted.s:3493): `beq .skip` → `cmp #1,d0 / beq outmem` otherwise
  `addq #4,a7` (discard caller!). See I (dead branch).

## B. Register conventions

* No persistent registers in the scope.
* `filerequest` saves d1–a6 itself (`movem.l d1-a6,-(sp)` at code+4, return
  via saved SP `moveal %pc@(0x628),sp`), returns result + flags via
  `move.l (sp)+,d0` (MOVEA/MOVEM/RTS leave CCR as is).
* `rawed+4` saves a0–a2/a6, otherwise only d0/d1.

---

## C. Globals / data structures

### C.1 Screen/window

| Label | Type | Init | Runtime changes |
|---|---|---|---|
| `newscreen` (6286) | `struct NewScreen` (36 B) | 0,0,640,200, Depth 2, Pens 1/2, Type `$100f` = CUSTOMSCREEN\|NS_EXTENDED, Font `screenfont`, Title `newsctit`, Extension `extnewscreen` | `+4` Width ← `scdata+12` (WB width & $fff0), `+6` Height ← computed (ted.s:4635/4658); `+6` read by `openreq`, `gotolined0` surroundings (2554, 3596) |
| `viewmode` (6288) | = `newscreen+12` ViewModes, init `$8004` | ← `scdata+44+32` = vp_Modes of the WB screen (4632) – so the initial value has no effect |
| `screenfont` (6293) | `struct TextAttr` | "topaz.font", 8, 0, 0 | – |
| `extnewscreen` (6301) | TagItem list | `SA_Pens`($8000003A)→`pens`, `SA_LikeWorkbench`($80000047)=1, `SA_Depth`($80000025)=2, TAG_END | – |
| `pens` (6310) | UWORD[10] DrawInfo pens | 1,2,2,1,2,3,1,0,3,~0 | – |
| `newsctit` (6312) | char[32+80] | 'Blitz Basic 2 - Ted Version 1.1' | screen title; `showfname` (732, 1598, 2730) writes `"File: "`+filename (≤63) into it |
| `newwindow` (6316) | `struct NewWindow` (48 B) | 0,0,640,200, Pens 0/1, IDCMP `$56C` = RAWKEY\|GADGETUP\|GADGETDOWN\|MENUPICK\|MOUSEBUTTONS, Flags `$1900` = BACKDROP\|BORDERLESS\|ACTIVATE, FirstGadget `newgads`, Type 15 | `+2` TopEdge ← `topskip` (=12), `+4` Width, `+6` Height (4630–4656) |
| `idcmp` (6318) | = `newwindow+10` | | label only, no reference found in the code |
| `winscreen` (6321) | = `newwindow+30` Screen | 0 | ← myscreen (4674) |

### C.2 Scrollers (window gadgets) and their borders

| Label | Type | Init / purpose |
|---|---|---|
| `newgads` (6346) | `struct Gadget`, Flags 4 (GADGIMAGE), Act 3 (GADGIMMEDIATE\|RELVERIFY), Type 3 PROP, ID **0** | vertical scroller, Next → `newgads2` |
| `proprend` (6354) | `struct Image` 20 B, 0 | AUTOKNOB buffer |
| `prop1sinfo` (6359) | `struct PropInfo` | Flags 13 = AUTOKNOB\|FREEVERT\|PROPBORDERLESS, HPot -1, VPot 0, HBody -1, VBody -1 |
| `newgads2` (6362) | `struct Gadget`, Act 1 RELVERIFY, PROP, ID **1** | horizontal scroller |
| `proprend2` (6370) | Image 20 B | |
| `prop2sinfo` (6375) | PropInfo, Flags 11 = AUTOKNOB\|FREEHORIZ\|PROPBORDERLESS, HBody $8000 | |
| `newgadbord`/`nextngad` (6324/6329) | 2× `struct Border` (Pens 1 or 2, JAM1, 3 points) + `ngc1`/`ngc2` (3 pairs each) | 3D frame around the vertical scroller, `DrawBorder` in `screenopen` (4779) |
| `newgadbord2`/`nextngad2` (6335/6340) | ditto + `ngc3`/`ngc4` | frame around the horizontal scroller (4784) |

Runtime (`screenopen`, ted.s:4721–4765) – **every opening sets anew**, because afterwards
it is shifted relatively (`addq #3,newgads+4` etc.):
* `newgads+4/+6/+8/+10` (Left/Top/Width/Height): `clr.l +4`, `+8 = $000A0000`, then
  Left = edcols*8 (+3), Top +2, Height = pixrows (−4).
* `newgads2+4/+6/+8/+10`: Width = edcols*8 (−6), Top = pixrows (+2), Height 4, Left +3.
* `prop2sinfo+6` HorizBody ← edcols·65536/chcols.
* `ngc1+10`, `ngc2+6/+10`, `ngc3+0/+10`, `ngc4+0/+4/+6/+10`, `newgadbord+0`,
  `nextngad+0`, `newgadbord2+2`, `nextngad2+2`.
* Read: `prop1sinfo+4` VertPot (ted.s:1470/1485, dragging → line = VPot·numlines>>16),
  `prop2sinfo+2` HorizPot (5722), `prop2sinfo+6` (5328). Set via
  `NewModifyProp(newgads/newgads2, mywindow, 0, flags 13/11, …, 1)` (5299, 5331).

### C.3 Menu bar

Equates: `mp1=2` (FrontPen), `mp2=1` (BackPen), `mj=1` (JAM2), `mwidth=160`.
Types: `struct Menu` (30 B = `menu2name-menu2`), `struct MenuItem` (34 B) directly followed
by its `struct IntuiText` (20 B, `mXiYname`: Pens 2/1, JAM2, Pos 2,2, Font 0) and
the text. Items: Width 160, Height 12, TopEdge 0,12,24,…; Flags `2+16+64[+4]` =
ITEMTEXT\|ITEMENABLED\|HIGHCOMP[\|COMMSEQ]; command byte + KludgeFill in `dc.b 'X '`.

**Chain of menus:** `menu1` → `menu4` → `menu3` → `menu2` → 0 (NextMenu); from this
MENUNUM: 0 PROJECT, 1 EDIT, 2 SOURCE, 3 SEARCH, 4 COMPILER (appended at runtime).
The item number is the **position in the chain, not the label number**
(`menu1item4` → `menu1item6`, `menu4item2` → `menu4item4` → `menu4item5` →
`menu4item3`). `menu` (ted.s:1565) forms the byte `item<<4 | menu` (only items 0–15) and
searches in `menutable` (ted.s:6199).

| Menu (Left/Width) | Index: text [key] (label) |
|---|---|
| PROJECT `menu1` (0/64) | 0 NEW (item1) · 1 LOAD [L] · 2 SAVE [S] · 3 DEFAULTS [?] (item4) · 4 ABOUT (item6) · 5 PRINT (item7) · 6 CLI [Z] · 7 CLOSE WORKBENCH · 8 OPEN WORKBENCH · 9 QUIT [Q] (item11) |
| EDIT `menu4` (72/40) | 0 COPY [C] (item1) · 1 KILL [K] (item2) · 2 BLOCK TO DISK (item4) · 3 INSERT FROM DISK (item5) · 4 FORGET [W] (item3) · 5 INSERT LINE [I] · 6 DELETE LINE [D] · 7 DELETE RIGHT [Y] · 8 JOIN [J] · 9 BLOCK TAB []] (itema) · 10 BLOCK UNTAB [[] (itemb) · 11 MARK INDENT [A] (itemc) |
| SOURCE `menu3` (120/56) | 0 TOP [T] · 1 BOTTOM [B] · 2 GOTO LINE [G] |
| SEARCH `menu2` (184/56) | 0 FIND [F] · 1 NEXT [N] (itema) · 2 PREVIOUS [P] · 3 REPLACE [R] · 4 REPLACE ALL |

Initially disabled (without ITEMENABLED): COPY, KILL (`2+64+4`), BLOCK TO DISK (`2+64`).

**Runtime patches:**
* `bloken` (ted.s:3890) `or #16,…+12` / `blokdis` (3903) `and #~16,…+12` on
  `menu4item1`, `menu4item2`, `menu4item4`, `menu4itema`, `menu4itemb` (offset 12 =
  MenuItem.Flags, bit ITEMENABLED). Directly in the attached strip, without
  ClearMenuStrip/OnMenu. BLOCK TAB/UNTAB are initially active, until the first `blokdis`
  (calls ted.s:690, 1655, 1805).
* `menu2+0` (NextMenu, init 0) ← the COMPILER menu (see below).

**COMPILER menu from `menitems` (= edstruct+4)** – `getstuff` (ted.s:2782), only if
`menitems ≠ 0`:
1. `ClearMenuStrip(mywindow)`.
2. `AllocMem(30, MEMF_PUBLIC)` (unchecked!), copy of `menu2` (word loop up to
   `menu2name`), `menu2.NextMenu` ← new; new: NextMenu 0, MenuName (+14) = `menitems`
   (pointer into blitz2 memory, no copy), LeftEdge (+4) = 248, Width (+8) =
   (strlen+1)·8.
3. Format `menitems`: `title\0` then per entry `text\0` + 1 byte shortcut (0 = none),
   end = 0 byte in place of a text (blitz2.s:724 `menus`: "COMPILER", 10 entries
   of 23 characters + key; non-final additionally `'PRINT TOKENS',0,0`).
4. Per entry `AllocMem(54, MEMF_PUBLIC)` (unchecked), copy of `menu2item1`+`m2i1name`
   (34+20 B), then: NextItem 0, Width (+8) = 200, TopEdge (+6) = 12·n, predecessor.NextItem
   or Menu.FirstItem (+18) ← new, ItemFill (+18) ← +34 (embedded IntuiText),
   IText (+46) ← text in `menitems`, Command (+26) ← shortcut, Flags (+12) COMMSEQ (4)
   set/cleared.
5. `SetMenuStrip(mywindow, menu1)` as tail `jmp`.
Freeing in `escape` (ted.s:1018): Menu (30) + all items (54) via FreeMem.
MENUNUM 4 → `extmenu` (ted.s:1542) → blitz2 `menu` (architecture.md §3.3).
`screenopen` reattaches `menu1` on every opening (4774), the COMPILER menu is preserved
via `menu2.NextMenu`.

### C.4 Requesters (common pattern)

Types exactly as in the NDK: `struct Requester` (112 B: Older, Left/Top/W/H, RelLeft/Top,
ReqGadget, ReqBorder, ReqText, Flags, BackFill, Kludge, ReqLayer, Pad1[32], ImageBMap,
RWindow, ReqImage, Pad2[32]), `struct Border` (16 B, Pens 1/2 or 2/1, JAM2(1), Count 3),
XY arrays (6 words), `struct IntuiText` (20 B, Pens 1/0, JAM2), `struct Gadget` (44 B),
`struct StringInfo` (36 B: here `dc.w 3 + dc.w 7 + dc.l 2` – the last 2 words of the
7 cover `Extension`, `dc.l 2` = LongInt, AltKeyMap), `struct PropInfo` (11 words),
`struct Image` as 5×`dc.l 0` (AUTOKNOB buffer). Each border pair = 2 borders + 2 XY.

**Important: TED does not use `Request()`.** `openreq` (outside, `comdata+0`) reads from the
requester only `+8` W/H, `+16` ReqGadget, `+20` ReqBorder, `+24` ReqText; opens its
own window `reqwindow` (W+12, H+15, centred, screen = myscreen), gadgets =
window gadgets; ReqText/ReqBorder are drawn via `PrintIText`/`DrawBorder` at
(6,12). **One-time shift** of all gadgets by (+6,+12) (`+4`,`+6`), marker
= `UserData` high word (`+40`) of the **first** gadget ← −1. That is why
GTYP_REQGADGET is rightly missing. Return of `getinput`: d7 = −1 on gadget, d6 = GadgetID.
All string gadgets share `undobuffer` (7105, `ds.b 192`; also `gotoreqsg1` ted.s:2391).

| Requester | Size | ReqBorder chain | ReqText chain | Gadgets (ID: kind, text) |
|---|---|---|---|---|
| `findreq` (6906) | 80,6 376×92 | bo1→la2→bo2→la5→bo3→la8→bo4→la11→0 (8 borders) | in1→in2→in3 ('REPLACE:', 'FIND:', 'Find/Replace') | 1 STR find (`findreqsg1`, buffer `findreqla16` 64 B, MaxChars 64, Flags $200 TABCYCLE, Act RELVERIFY) · 2 STR replace (`findreqsg2`, `findreqla17` 64 B, Act RELVERIFY\|ENDGADGET) · 3 NEXT · 4 PREVIOUS · 5 REPLACE · 6 REPLACE ALL · 7 CANCEL (Bool, Act 5) · 8 CASE SENSITIVE (Act $101 TOGGLESELECT\|RELVERIFY) |
| `repall` (7111) | 132,46 240×40 | bo1 (2) | in1 'Replace all.....' | 1 FROM HERE · 2 FROM TOP · 3 CANCEL |
| `errorreq` (7195) | 128,58 312×40 | bo1 (2) | in1, IText = **`errorpnt`** (runtime) | 1 ' OKAY ' |
| `erasereq` (7239) | 128,50 256×48 | bo1 (2) | in1→in2 ('This will erase altered file!', 'Continue?') | 1 ' OK ' · 2 ' CANCEL ' |
| `defaultreq` (7310) | 60,1 400×172 | bo1→…→bo10 (20 borders; bo8–10 LeftEdge 4 = slider frames) | in1→…→in13 (title, 'Use Small Font', 'Tokenise', 'Create Icons', 3× margin/tab, 'Text Colour:', **'' (formerly 'Label Colour:')**, 'Token Colour:', 'Red:', 'Green:', 'Blue:') | see below |
| `prtreq` (7971) | 60,46 360×68 | bo1→la2→bo2→la5→bo3→la8→0 (6) | in1→in2 ('PRINTER DEVICE:', 'Print Text') | 1 STR (`prtreqsg1`, buffer `prtname` 'PRT:'+59×0 = 64 B, MaxChars 64, Act RELVERIFY) · 2 ' PRINT ' · 3 ' CANCEL ' |

Buttons: Flags 0 (GADGHCOMP), Act 5 = RELVERIFY\|ENDGADGET, Type 1 BOOL, GadgetRender =
own border pair, GadgetText own IntuiText (4,2). ENDGADGET has no effect in the window.

**defaultreq gadgets (chain ga1…ga27, but `ga13 → ga18`: ga14–17 unlinked, 23 active):**

| ID | Gadget | Kind | Data |
|---|---|---|---|
| 1–3 | ga1–3 (75,120/132/144, 306×4) | PROP, Act 3, Image `defaultreqla35/39/43`, `defaultreqhp1–3` Flags 11, HPot (6<<16)/15, HBody 4096 (16 steps) | R/G/B of palette `propcol` |
| 4–6 | ga4–6 (20,54/66/78, 24×8) | STR, Act $801 = LONGINT\|RELVERIFY, MaxChars 3, buffer `defaultreqla47/48/49` (3 B: '12','6','2') | LongInt = **`int1`/`int2`/`int3`** (init 12, 6, **4**) = hscmarg, vscmarg, tabstop |
| 7–9 | ga7–9 (16,16/28/40, 32×12) | BOOL toggle, Act 257 = TOGGLESELECT\|RELVERIFY, text '  ' | Small Font, Tokenise, Create Icons (ga9 Flags 128 = SELECTED initially) |
| 10–13 | ga10–13 (280..352, 36, 16×12) | BOOL, Flags 2 GADGHIMAGE, Act 2 GADGIMMEDIATE, Render=SelectRender=`bo14–17`, text ' ' with BackPen 0..3 | colour "text" (group 0 → `normcol`) |
| 14–17 | ga14–17 (y 52) | ditto, `bo18–21` | group 1 `labcol` – **not in the chain** |
| 18–21 | ga18–21 (y 68) | ditto, `bo22–25` | group 2 `tokcol` |
| 22–25 | ga22–25 (16..312, 100, 72×12) | ditto, `bo26–29`, text 8 spaces, BackPen 0..3 | group 3 `propcol` (palette register for the sliders) |
| 26 | ga26 ' OK ' | button | |
| 27 | ga27 ' CANCEL ' | button | |

Evaluation in `defaults` (ted.s:401ff.): ID<4 → slider loop `propadj`; 10–25 →
group = ((ID−10)>>1)&6, only group 3 triggers `setprops`/`dispprops`; 4–9 →
validation + takeover; ≥26 → end (26 OK, 27 Cancel → `setdefs` with old values).

### C.5 Requester fields changed/read by the code (offset!)

| Field | Access | Location |
|---|---|---|
| `findreqsg1/2 +8` BufferPos, `+12` DispPos | `clr` | find 853–856 |
| `findreqla16/17` (find/replace text) | backup to charbuff/charbuff2 and copy back on CANCEL; read by search/replace | 857–896, 902, 932, 965, 984, 991, 3540 |
| `findreqga8+13` bit 7 (= Flags & SELECTED, low byte) | `btst` → case-sensitive | 3543, 3550 |
| `errorpnt` (= errorin1.IText) | ← a0 (error text) | error 3512 |
| `defaultreqga7/8/9 +12` Flags SELECTED ($80) | and/or | setdefs 3097–3108, getstuff 2784 |
| `defaultreqga8 +12` Flags DISABLED ($100) | or/and (disabled without token list) | 406, 409 |
| `defaultreqga7/8/9 +13` bit 7, `+12`&128 | read: fonthite, tokenise, makeicons | 506, 514, 518, 805 |
| `defaultreqga9+13` | **address** is exported to blitz2: `comdata+84` ← `#defaultreqga9+13` | ted.s:82 (no use of `84(aX)` found in blitz2.s) |
| `defaultreqga4/5/6 +12` SELECTED | cleared (loop, step `ga5-ga4` = **84 B**) + RefreshGList(3) | 495–503 |
| `defaultreqla47/48/49` | `move #$3000` ("0\0"), `numtoascii2` | 480–490, 3122–3128 |
| `int1/2/3` (LongInt) | `cmp.l`/`clr.l`; `+2` (low word) ↔ hscmarg/vscmarg/tabstop | 477–489, 522–524, 3134–3136 |
| `defaultreqhp1/2/3 +2` HorizPot | write (setprops, step `hp2-hp1` = **86 B**), read >>12 (propadj) | 3174–3185, 3200–3208 |
| `defaultreqbo14…bo29` +4/+5 (FrontPen/BackPen, both borders per pair) | swapped: `swapbords` (all 32 borders, step `la63-bo14` = **28 B**), `swapd0bord` (index·**56 B**) | 3238–3255 |
| `defaultreqga10…ga25` | `dispgadd0`: gadget = ga10 + n·**44 B**, RefreshGList(1) | 3229 |
| all requester gadgets `+4/+6/+40` | shifted/marked once by `openreq` | see C.4 |
| `prtname` | as DOS name to `Open(…,1006)` | 585 |

### C.6 Other data

| Label | Size | Content/purpose |
|---|---|---|
| `undobuffer` (7105) | 192 B | common UndoBuffer of all StringInfos |
| `rawed` (7109) | 660 B incbin | hunk file (see D/F) |
| `;rawmaker incbin rad:rawmaker.obj` (7107) | – | commented-out predecessor of `rawed`, not built |
| `alertstuff` (7948) | alert string | x=56 (word), y=22 (byte), 'Not enough memory for operation! - Click Mouse Button to Continue.', 0, continuation byte 0. `doalert` (3058): `DisplayAlert(RECOVERY_ALERT=0, alertstuff, 40)` |
| `filerequest` (7967) | 15916 B incbin | hunk file, relocated by `flink`; `;include rad:filerequester` = source not available |
| `font6` (8070) | 768 B | font of height **6** (not width!): 8 rows of 16 characters, each character 1 byte wide (8 px), row spacing 16 B. Character c: `font6 + (c&15) + (c>>4)·96` (`fontcalc` 5214) → 128 characters |
| `font8` (8071) | 1024 B | height 8: `font8 + (c&15) + (c>>4)·128` → 128 characters; sits at the hunk end |

---

## D. Self-modifying code / data in the code / incbin entries

### `flink` – runtime relocation of `filereq` (exact)

Layout of `filereq` (checked): `$3F3, 0, 1, 0, 0, $E46` (HUNK_HEADER, 1 hunk, 3654 long words),
`$3E9, $E46` (HUNK_CODE), code (14616 B) from file offset 32, `$3EC` (RELOC32) with **one**
block `n=312, hunk=0, Offsets…`, `0`, `$3F2` (HUNK_END).

```
flink	lea	filerequest+28(pc),a0  ; a0 → size word of HUNK_CODE
	move.l	(a0)+,d0               ; d0 = 3654 (long words), a0 = code start (+32)
	move.l	a0,a1                  ; a1 = base for offsets
	move.l	a0,d2                  ; d2 = runtime address of the code = relocation value
	lsl.l	#2,d0                  ; bytes
	lea	4(a0,d0.l),a0          ; behind code + $3EC id → counter n
	move.l	(a0),d0                ; d0 = n (312)
	addq	#8,a0                  ; skip n and hunk number
.loop	move.l	(a0)+,d1               ; offset
	add.l	d2,0(a1,d1.l)          ; *(ULONG*)(code+off) += code
	subq.l	#1,d0
	bne	.loop
	rts
```
Assumptions (hard-coded): exactly 1 hunk, header 7 long words, exactly one RELOC32 block,
target hunk 0 is ignored, n≠0 (n=0 → 2³² loop). No CacheClearU afterwards. Not
idempotent (second call = double relocation). The requester's data (window,
gadgets, buffers) also lie in the block → `filerequest` writes into TED's
code hunk at runtime.

### `rawed` (no relocation needed)
`rawed.obj`: HUNK_HEADER + HUNK_CODE 156 long words, **no RELOC32** (only PC-relative and
`$4.w`). Code from +32 (disassembly `rawed.obj.dsm`):
* +0 `bra init`, +4 `bra convert` (code+$10A), +8 `bra cleanup` (code+$20A).
* Data in the code: 'console.device', device base (code+28), 2-byte buffer (+32),
  `struct InputEvent` (+34, ie_Class=RAWKEY), tables code+56/+161 (105 B each:
  rawkey 0..$68 without/with qualifier 1), IOStdReq (48 B, code+$218), MsgPort (34 B).
* `init`: initialise IOStdReq/MsgPort (FindTask(0), AllocSignal(-1)),
  `OpenDevice("console.device", -1, io, 0)`, remember device base, fill both tables via
  `convert`. The tables are **not** used by TED (no reference to
  `rawed+32+56`).
* `convert`: special cases before RawKeyConvert, independent of the qualifier:
  $50–$59 (F1–F10) → $81–$8A; $4C–$4F (cursor) → $1C–$1F; $5F (Help) → $8B;
  otherwise `RawKeyConvert(&ie{Code=d0,Qualifier=d1}, buf, 1, NULL)`; result −1 → 0, otherwise
  buf[0] (buffer is cleared beforehand → 0 if no character).
* `cleanup`: `CloseDevice(io)` (tail `jmp`); the signal is never freed.

### `filerequest+36` vs. freqlib `dofreq`
* Code+0: `lea Code+$B3E,a0` (internal default struct with its own buffers) – falls through into
  code+4. Code+4: `movem.l d1-a6,-(sp)`, `a5 = a0`; struct: `+0` path buffer,
  `+4` name buffer, `+8` title, `+12` screen (0 → WBENCHSCREEN, otherwise CUSTOMSCREEN with
  screen; moreover `setheight` reads `screen+14` and at code+$88C `screen+76`
  **without null test**).
* freqlib.c: `freqst[4] = {path, file, title, screen}` and `freqcode()` = the same
  code (offsets there "in the binary", data block `fq_data`). TED's `reqstruct` has the
  same layout as `freqst`; `dofreq_c` is only the Blitz string wrapper around it.
  → In C: use `freqcode()` from libs/acidlibs/freqlib.c as common source, with
  parameter `ULONG st[4]` instead of static `freqst`; return pointer (0 = cancel), generate Z flag
  for asm callers (`comdata+24` = `ezfreq`) in the glue.
* Result = pointer into the requester data block (`RESULT`), used further by TED via
  d7; OK moreover copies path/name back into `filepath` (192 B) / `filename` (64 B).

---

## E. Heap/list structures

| Structure | Size | Responsible | Fields |
|---|---|---|---|
| COMPILER menu | 30 B, MEMF_PUBLIC | getstuff / escape | copy of `menu2`: +0 NextMenu=0, +4 Left=248, +8 Width, +14 MenuName→menitems, +18 FirstItem |
| COMPILER item | 54 B, MEMF_PUBLIC | getstuff / escape | +0 NextItem, +6 Top=12n, +8 Width=200, +12 Flags (COMMSEQ), +18 ItemFill→+34, +26 Command, +34 IntuiText (copy of `m2i1name`), +46 IText→menitems |
| AUTOKNOB images | 20 B static | Intuition | `proprend`, `proprend2`, `defaultreqla35/39/43` |

No further allocations in the scope (filereq internally allocates 48 B chip + 40-B nodes).

## F. External calls (from the scope or for these data)

* intuition: `OpenScreen(newscreen)` (tags V39: SA_LikeWorkbench), `OpenWindow(newwindow)`,
  `SetMenuStrip`/`ClearMenuStrip`, `DrawBorder(newgadbord[2])`, `NewModifyProp`,
  `RefreshGList` (defaultreq), `PrintIText`/`DrawBorder` (openreq), `DisplayAlert`.
* graphics: `SetRGB4(&screen->ViewPort, propcol, r,g,b)` (propadj).
* exec: AllocMem/FreeMem (menus), in rawed: FindTask, AllocSignal, OpenDevice,
  CloseDevice; console: `RawKeyConvert` (−48).
* filereq internally: dos/intuition/graphics via OldOpenLibrary (never closed),
  OpenWindow, Lock/Examine/ExNext, ActivateGadget …
* blitz2: `menitems` (edstruct+4) only read (pointers stay in blitz2 memory);
  `comdata+84` ← address of the "Create Icons" flag byte.
* No hardware access in the scope.

---

## G. What needs special handling in C

1. **Fixed strides across structs** (in C they must not run across separate
   statics): `defaultreqga4→5` 84 B, `defaultreqhp1→2` 86 B, `defaultreqga10→11` 44 B,
   `defaultreqbo14→15` 56 B, `defaultreqbo14→la63` 28 B (32 borders), `menu2item1→m2i1text`
   = MenuItem+IntuiText 54 B. Solution: pointer tables (`colgad[16]`, `colbord[32]`,
   `hp[3]`, `intgad[3]`) or one packed struct per group; menu template as
   `struct { struct MenuItem mi; struct IntuiText it; }`.
2. **Alias labels in the middle of structs:** `viewmode` = newscreen.ViewModes,
   `winscreen` = newwindow.Screen, `idcmp` = newwindow.IDCMPFlags, `errorpnt` =
   errorin1.IText, `int1..3` = defaultreqsg1..3.LongInt (code accesses `int1+2` =
   low word), `defaultreqhp1+2` = HorizPot, `nextngad` = second border. → `#define`.
3. **Byte aliasing on flags:** `btst #7,xxx+13` = `Flags & GFLG_SELECTED`;
   `comdata+84` must be `(UBYTE*)&defaultreqga9.Flags + 1` (big-endian, bit 7).
   `move #$3000,defaultreqla47` = `"0"`.
4. **UserData as marker** (openreq): gadget[0].UserData high word = −1 → in C
   `(ULONG)UserData` ≠ 0 or similar, but keep the layout the same for blitz2 requesters (blitz2
   calls `openreq` via comdata with its own requesters).
5. **`flink`/`filerequest`**: dropped in C if freqlib.c's `freqcode` is linked in
   (only one copy in the project). Glue for `ezfreq` as comdata entry: a0/a1/a2 →
   C, return d0 + Z flag (`tst.l d0`), a3 = `reqstruct+12` (the original leaves a3 like this).
   Alternatively asm incbin + flink in `glue*.c` (then add CacheClearU? No –
   keep the behaviour, only document it).
6. **`rawed`**: new in C (console.device RawKeyConvert) – special case mapping as pure
   function; init/cleanup as UI. Register interface only internal (callers in TED).
7. **Menu flags** are changed directly in the attached strip – likewise in C
   (do not introduce OnMenu calls, otherwise behavioural change on redraw).
8. The NewScreen tags (`SA_LikeWorkbench`, `SA_Pens`) need the NS_EXTENDED struct
   (`struct ExtNewScreen`).

### Suitability of bb2 `blitz2-c/tools/req2c.py`
Suitable for the six requesters (same directive signatures as blitz2's
`optreq`/`typereq`), with small extensions:
* Parts list: `ted.s` 6906–7104, 7111–7308, 7310–7946, 7971–8068; extend `EXPORT_RE`/`req` detection
  by `findreq|repall|errorreq|erasereq|defaultreq|prtreq`.
* Add `int1, int2, int3` to `CONT` (otherwise `defaultreqsgN` has only 3 statements →
  IndexError) and `errorpnt` (otherwise `errorin1`/`errorpnt` "unknown");
  then `#define int1 defaultreqsg1.LongInt`, `#define errorpnt errorin1.IText`.
* `ds.b` in the requester is already ignored (only st[1], st[2] read) – ok.
* `(6<<16)/15` in PropInfo: passed through unchanged – valid C expression (26214).
* `dc.b '',0` (defaultreqla30) → empty string, ok; comments `;dc.l …` ok.
* Gadget UserData becomes NULL, GadgetRender on `image`/`border` correct.
* **Not** covered: strides (G.1) – the generator produces separate statics; code must
  go via pointer tables (or the generator outputs groups as arrays).
* Menus (`Menu`/`MenuItem` with equates `mp1,mp2,mj,mwidth` and expressions `2+16+64+4`,
  `96+12`), NewScreen/NewWindow/window gadgets: new kinds needed or better by hand
  (4 menus, 31 items, 1 screen/window each, 2 props, 4 borders) – manageable.

---

## H. Classification

| Routine/block | Class | Remark |
|---|---|---|
| `flink` | GLUE (dropped with C file requester) | |
| `filerequest` (code) | UI | = freqlib.c `freqcode`, reuse |
| `rawed` init/cleanup | UI | console.device |
| `rawed` convert: special key mapping | KERN | pure function (rawcode→code), RawKeyConvert rest UI |
| Data screen/window/menu/requester | UI | |
| `font6`/`font8` | KERN data | as `const UBYTE[]` (e.g. via `xxd -i`) |
| `undobuffer`, search/replace buffers `findreqla16/17` | KERN data | read by the search core (isithere/replace2) → provide as normal buffers for host tests; abstract the case flag as bool instead of `findreqga8.Flags` |
| `int1..3`, `defaultreq*` flags | UI data | options → decouple in C via `hscmarg/vscmarg/tabstop/tokenise/makeicons` |

---

## I. Peculiarities / possible BUG(orig)

1. **`int3 = 4`, buffer shows '2'** (7837/7838), `tabstop` default 2: without a loaded
   defaults file `defaults` takes over `tabstop = int3` on every click on gadget 4–25
   → the tab stop silently jumps from 2 to 4.
2. **Slider initial value** `(6<<16)/15` does not match `palit` (8,8,8); `defaults` calls no
   `setprops` before the first display (only `setdefs` on loading / group 3 click).
3. **`getfname3`: `cmp #1,d0 / beq outmem` is dead code** – Z=1 only at d0=0. If the
   48-B chip allocation in the file requester fails, it returns the first long word of the
   uninitialised FIB buffer (Z usually 0) → TED takes garbage as file name pointer.
4. `flink`: no cache flush (68040/060 copyback: relocated long words possibly still in the
   data cache when the code is executed); not idempotent; only one RELOC block.
5. COMPILER menu: `AllocMem` unchecked (2 places); `escape` frees items without
   checking FirstItem for 0 (menu without entries → FreeMem(0)); freeing while the strip is still
   attached (before `freeall`).
6. COMPILER items width 200 with 23 characters (184 px) + COMMSEQ field → shortcut possibly overlaps
   the text (`;!*!` comment 2805).
7. Menu evaluation `and #$f0` after `lsr #1`: only items 0–15 (EDIT has 12 – ok), submenus
   not provided for; MENUNUM via `and #15`.
8. `defaultreqga13 → defaultreqga18`: ga14–17 + `bo18–21` + texts remain as dead data,
   but are still recoloured by `swapbords`/`setdefs` (harmless).
9. `rawed`: OpenDevice result unchecked (device base 0 → crash in convert);
   AllocSignal never freed; tables code+56/+161 are built, but never used.
10. `filerequest` reads `screen+14`/`screen+76` without null test – uncritical for TED, since
    `reqstruct+12` is always set after `screenopen` (called before → read access to
    address 14/76).
11. `font6`/`font8` cover only characters 0–127; characters ≥128 read behind the table
    (font8 sits at the hunk end) – check whether callers exclude this.
12. `viewmode` initial value `$8004` (HIRES|LACE) is always overwritten by the WB modes;
    `idcmp` label unused.
13. Menu flags (ITEMENABLED) are changed directly while the strip is attached (the OS rule
    demands ClearMenuStrip/ResetMenuStrip) – works, keep it.
