# Survey 06 – ted.s lines 5253–6285

Scope: cursor blinking/status line/props (`cursor`…`showhprop`), `refresh`, block boundary correction
(`inbloks`/`inblokf`/`blokfix`), the **line editor** `getline` (insert/delete characters, tab,
cursor left/right, Shift variants, delete to end of line), `horizpos`/`cursleft`/`cursrite`,
the **IDCMP loop** `getinput`/`getinput2`/`handled0` (rawkey via `rawed+32+4`, requester
keyboard Return/Esc, umlauts `germs`), `refme`, then the **whole data block** from 6022:
`mydata`/`comdata` table, editor globals, defaults block (file `l:blitzeditor.opts`),
`jumptable`/`menutable`, error texts, `edstruct` copy area.

Addresses (vasm reference listing, `tedlisting.py addr`): `cursor` $543E, `showrow` $5458,
`getline` $566C, `getinput2` $5BB8, `getinput` $5BD6, `handled0` $5C00, `germs` $5DAE,
`mydata` $5DBA, `comdata` $5DC2, `req_window` $5E20 (= comdata+94 ✓), `topskip` $5E24,
`jumptable` $61C2, `menutable` $6206, `tokens` $62F8, `remark` $6313, `rawed` $73E0.

Everything sits in the one code section; globals are read PC-relative and written absolute.

---------------------------------------------------------------------------------------------
## A. Routines

Conventions in this scope: "column" = screen column `cursx` (0..edcols-1);
"abs. column" = `cursx+cursoff` (0..chcols-1); "line no." = `curstop+cursy`.
Character cell = 1 byte wide (8 pixels), 2 bitplanes, `fonthite` raster lines of `bmapmul` bytes,
`fontmul` = bytes per text line in one plane.

### Display
- `cursor` (5253) = `cursor2` (5254) : two labels, same address. in: `cursx`,`scloc`,
  `bitmaps`. `calcphys` → a0/a1 = cell in plane 0/1; `not.b` over `fonthite` raster lines in
  both planes (XOR cursor, calling twice = off again). destroys d0,a0,a1. No return value.
- `showrow` (5263) : `numtext`="     \0" (5 spaces), d0=line no.+1 → `numtoascii`(d0,a0=numtext);
  output with `pstats` (a3=numtext) directly into the bitmap at `statsloc+5` ("Line:"). Tail `bra
  pstats`. destroys d0-d2,a0,a1,a3 (+qprint).
- `showvprop` (5275) : `showrow`, then NewModifyProp(newgads=vprop, mywindow, req=0,
  flags d0=13 (AUTOKNOB|FREEVERT|PROPBORDERLESS), HorizPot d1=-1, VertPot d2, HorizBody d3=-1,
  VertBody d4, NumGad d5=1). VertBody = (edrows<<16)/numlines or -1 if edrows>=numlines;
  VertPot = (line no.<<16)/(numlines-1) or -1 if line no.>=numlines-1 (no /0).
  **Tail `jmp newmodifyprop(a6)`** with a6=`int` → returns with a6=IntuitionBase.
  destroys d0-d5,a0-a2,a6.
- `showhprop` (5305) : `numtext`="   \0", column (abs.+1) at `statsloc+18` ("Column:");
  NewModifyProp(newgads2=hprop, flags 11 (AUTOKNOB|FREEHORIZ|PROPBORDERLESS), HorizPot =
  (abs.column<<16)/(chcols-1) or -1 at the end, VertPot -1, HorizBody = `prop2sinfo+6`
  (unchanged), VertBody -1, d5=1). Tail `jmp` as above.
- `refresh` (5337) : redraw the whole text area from `chmap` (horizontally from `cursoff`).
  `storepos`; scloc=chloc=cursy=0; per row cursx=0, a5=chmap+chloc+cursoff, edcols× `print3`,
  `godown`; `color`=normcol; **tail `bra restorepos`**. destroys d0,d1,d6,d7,a0,a5 (+print).
  Does not draw the cursor.

### Block bounds (block = btopx/btopy..bbotx/bboty, `bboty==-1` = no block)
- `inbloks` (5360) : Z=1 if and only if line no.==btopy and abs.column<=btopx (unsigned).
  out: Z, d2 = line no. (on line mismatch) or abs.column; d3=0 only at Z=1. Also called from
  chunk 2/3 (1821).
- `inblokf` (5372) : ditto with bboty/bbotx ("cursor <= last block character").
- `blokfix` (5384) : if btopx>=chcols: btopx=0, btopy++; if then btopy>bboty → tail `bra
  blokdis` (cancel block), otherwise continue like `blokfixb`. destroys d2.
- `blokfixb` (5393) : **second entry** (from `blokfix`, `char127`). bbotx<0 (signed):
  bbotx=chcols-1, bboty--, if bboty<btopy → `blokdis`. Otherwise: single-line block with
  bbotx<btopx (signed) → `blokdis`; bbotx>=chcols → bbotx=chcols-1 (see I.1: bug).
  Return via rts or tail `bra blokdis` (other chunk, l.3903).

### Line editor
- `getline` (5416) : **loop until an event arrives that is not handled locally.** in: nothing
  (globals). `anyalt`=0; `cursor` on; → `getline3`.
- `getline2` (5420) : return target of all local actions: `cursor` on, `showhprop`, falls
  into `getline3`.
- `getline3` (5422) : `getinput` → d7/d6; `cursor` off; work registers:
  a0/a1 = plane 0/1 + scloc (line start on screen), a2 = chmap+chloc (line in the
  character map, chcols bytes, indep. of cursoff), d0 = cursx. Dispatch:
  - d7<0 (gadget/menu/mouse) → `charhi`; bit 30 set (umlaut from `germs`) → clear, `.isasc`;
    d7<32 → `charlow`; d7>=127 → `charhi`; otherwise `.isasc`.
  - `.isasc` (insert, always insert mode): anyalt=d7; block: `inbloks`→btopx++,
    `inblokf`→bbotx++, `blokfix`; bitmap row from cursx 1 cell to the right (both planes,
    fonthite raster lines), chmap from abs.column 1 to the right (byte chcols-1 drops out),
    `print` d7 (colour = current `color`), `cursrite`, → getline2.
- `charlow` (5484) : d7=31 (cursor left, `$1F`):
  without Shift (`qualifier&3`) `cursleft`; with Shift: abs.column≠0 → d1=0 → `curspos`; abs.
  column==0 → `.tababove`: line no.==0 → getline2; otherwise count leading spaces of the previous line
  (`thisitem→lastitem(+4)→chars`, end at byte 0 → no movement) → `curspos`.
- `charnot31` (5521) : d7=30 (right): without Shift `cis30skip` (= `cursrite`); with Shift: d1 =
  1+index of the last non-space in a2 (max chcols-1, 0 for an empty line) → falls into
  `curspos`.
- `curspos` (5537) : **jump target** with d1 = abs. target column: `horizpos`, → getline2.
- `cis30skip` (5540) : `cursrite`, → getline2.
- `charnot30` (5543) : d7=8 (backspace): abs.column==0 → getline2; otherwise (a0-a2 saved)
  `cursleft`, d0=cursx, d7=8, → `char127` (anyalt becomes 8).
- `charnot8` (5555) : d7=9 (tab). d0=d2=abs.column.
  - with Shift ("delete tab"): anyalt=9; d0 back to the previous multiple of `tabstop` (divu,
    remainder 0), at <0 → getline2; block: d4=d2-d0, inbloks→btopx-=d4, inblokf→bbotx-=d4
    (no `blokfix`); copy chmap[d2..] to chmap[d0..], fill the rest with ' '; **push d0**,
    → `reline`. Also deletes non-spaces (I.3).
  - without Shift: d0 forward to the next multiple of tabstop (>=chcols → getline2, nothing);
    anyalt=9; **push d0**; d0=count; block (d2 saved): +d0 on btopx/bbotx, `blokfix`;
    chmap from d2 by d0 to the right (overflow drops out), gap ' '; falls into `reline`.
- `reline` (5645) : **stack-parameterised**: on entry expects the word (a7) = abs.
  target column. Draws the visible line (chmap+chloc+cursoff, edcols characters) with `print3`,
  cursx saved; pop d1 → `curspos`. Users: delete tab, insert tab, menu $f8e1.
- `charnot9` (5658) : all other d7<32 → `glinedone` (return to `mainedit`, among others 13, 27,
  28/29 cursor up/down, ctrl codes).
- `charhi` (5662) : d7=127 → falls into `char127`, otherwise `charnot127`.
- `char127` (5665) : delete under cursor (Del; also target of backspace). anyalt=d7; block:
  inbloks and abs.column≠btopx → btopx--; inblokf and bbotx+1<chcols → bbotx--; then
  `blokfixb` (only if inblokf was Z=1; at Z=0 → shift directly). Bitmap from cursx+1 by 1
  to the left, chmap from abs.column+1 to the left, chmap[chcols-1]=' '; last visible cell
  (cursx=edcols-1) redrawn with `print3` from chmap; → getline2.
- `charnot127` (5718) : d7=-1 and d6=1 (hprop gadget, down or up): d1 =
  (HorizPot `prop2sinfo+2` × chcols)>>16 → `curspos`. Otherwise:
- `charnotgad` (5726) : d7=-2 and d6=$f8e1 (menu 1, item 7, no sub; missing in `menutable` as $71
  – "delete to end of line"): anyalt=d6; from abs.column ' ' up to chcols; **push**
  abs.column; → `reline`. Block is not adjusted.
- `charnotmenu` = `glinedone` (5738/5740) : rts. **Only exit of getline.**
  out: d7 = event (see B/G), d6 = ID, `anyalt`≠0 if line changed, `qualifier`;
  d0,a0-a2 garbage. a6 = exec (from getinput) or int (from showhprop) – do not rely on it.

### Cursor movement
- `horizpos` (5742) : in d1 = abs. target column. d4=old cursoff. d1 < edcols-hscmarg →
  cursoff=0, cursx=d1. Otherwise d2=edcols/2, d0=d1-d2, d3=chcols-edcols: d0>d3 (signed) →
  cursoff=d3, cursx=d1-d3; otherwise cursoff=d0 (rounded up to even, then cursx=d2-1) or
  cursx=d2. If cursoff has changed → **tail `bra refresh`**, otherwise rts. Does not draw
  the cursor. destroys d0,d2-d4 (+refresh: d6,d7,a5). Also called from the `find` path (956).
- `cursleft` (5775) : in d0=cursx. cursx>=hscmarg → cursx--. Otherwise cursoff≠0 → cursoff-=2,
  `scrollrite` (bitmap 1 word = 2 cells), cursx++ (net 1 to the left). Otherwise cursx≠0 →
  cursx--, otherwise nothing. destroys d1 (+scrollrite).
- `cursrite` (5789) : in d0=cursx. cursoff+edcols>=chcols (right edge of the map visible):
  cursx<edcols-1 → cursx++ otherwise nothing. Otherwise: cursx<hscmarg or cursx<edcols-hscmarg →
  cursx++; otherwise cursoff+=2, `scrollleft`, cursx--. destroys d1,d2.
- `flash` (5810) : debug: 65536× d0 to COLOR00 ($dff180). d0 preserved. **Unused.**

### Input (IDCMP)
- `pickwindow` (5827) : out a2 = `req_window` if ≠0, otherwise `mywindow`. destroys d0.
- `getinput2` (5833) : **comdata+32**. a6=exec; port = pickwindow→UserPort (+86); one
  GetMsg; message → **tail `bne handled0`** (result as there); none → d7=0, d0=0 (Z=1).
  Processes at most 1 message; an ignored one likewise returns d7=0.
- `getinput` (5845) : **comdata+8**. a6=exec; `ginp1` (5852, internal loop head):
  pickwindow (anew on every pass, the requester may have closed meanwhile), WaitPort/GetMsg until
  a message, `handled0`, Z=1 → `ginp1`. out: d7/d6, d0=-1, Z=0, **a6=exec**. destroys
  d0,d1,a0-a2 (no movem saving – not even for blitz2 calls).
- `handled0` (5866) : in d0 = IntuiMessage, a6=exec. Replies (ReplyMsg) in all paths.
  out: d0=-1/Z=0 "understood" with d7 (and d6), or d0=0/Z=1 and d7=0 "ignored".
  | Class | Result |
  |---|---|
  | $20 GADGETDOWN / $40 GADGETUP | d7=-1, d6=GadgetID(+38). String gadget (type low byte +17 ==4) with code 9 (tab) → ignored. ENDGADGET (Activation +15 bit 2) and class $40 → ReplyMsg, **`closereq`**, d0=-1 (requester is already closed on return!) |
  | $2000 MENUVERIFY | ignored |
  | $100 MENUPICK | code -1 (MENUNULL) ignored, otherwise d7=-2, d6=code (NextSelect not evaluated) |
  | $8 MOUSEBUTTONS | code $68 SELECTDOWN → d7=-3, everything else → d7=-4 |
  | $400 RAWKEY | key-up (bit 7) ignored; `qualifier`=Qual(+26); d0=code,d1=qual → `bsr rawed+32+4` → d7=byte (0 → ignored). With requester: 13 → first gadget of the window list (+62) with ENDGADGET and BOOLGADGET (type 1) → `refme` (blink), treat class as $40 (→ closes). 27 → **last** such gadget, likewise; other keys ignored. Without requester: `germs` table, match → replacement code \| bit 30 |
  | $200 CLOSEWINDOW | like Esc (`.doesc`) |
  | otherwise | ignored |
- `refme` (6003) : in a0=gadget (comment "d0.b=pl…" is dead). Flags(+12)|=$80 (SELECTED),
  RefreshGList(a0, req_window, 0, 1), Delay(4), Flags&=~$80, RefreshGList. Saves
  d0-d1/a0-a2/a6 (movem).

### Data related to code (dispatch, details in C)
- `jumptable` (6171): searched by `mainedit` (l.156), `cmp.b` against d7 → **computed
  `jsr (a0)`** (flag byte on stack, `memstack`=a7, flag≠0 → `showvprop` afterwards).
- `menutable` (6199): searched by `menu` (l.1577) → `jsr (a0)`, flag≠0 → tail `showvprop`.

---------------------------------------------------------------------------------------------
## B. Persistent register conventions

- No register survives the main loop in this scope; everything goes via globals.
- **Return of `getinput`/`getinput2`/`getline`:** d7 = event, d6 = ID:
  d7 = 1..$FF key (zero-extended long; possibly **bit 30** = "printable replacement character" from
  `germs`, only relevant inside getline), 0 = nothing (only getinput2), -1 gadget (d6=ID),
  -2 menu (d6=Intuition menu code), -3 mouse pressed, -4 mouse released/other button.
  Key codes: ASCII via RawKeyConvert; cursor up/down/right/left $1C/$1D/$1E/$1F;
  F1–F10 $81–$8A; Help $8B; Del 127; Backspace 8; Tab 9; Return 13; Esc 27.
- Inside `getline3`…: a0/a1 = bitmap line start plane 0/1, a2 = chmap line, d0 = cursx,
  d1 = target column for `curspos`, d2 = abs. start column (tab), d7/d6 = event. `print`/`print3`
  save d2-d4/a2 (→ a2 and d7 survive `print3` in `reline`).
- a6: `getinput*` set/leave exec; `showvprop`/`showhprop` leave `int`;
  `refme` preserves a6.
- `inbloks`/`inblokf`: return only via the **Z flag** (callers `bne`).
- `handled0`/`getinput2`: return via d0 **and** Z flag (getinput tests `beq`).
- Called by blitz2 via comdata (this scope): `getinput` (+8), `getinput2` (+32) – **without**
  movem; blitz2 expects only d6/d7 (blitz2.s 1420 etc.). The entries +72/+76
  (`pointeron`/`sleepon`, other chunks) save a0-a1/d0-d3/a6.

---------------------------------------------------------------------------------------------
## C. Globals (defined 6022–6282)

### `germs` (6022), 12 bytes, `ngerms equ 6` (l.2)
Pairs (Latin-1 → font code): ä 228→26, ö 246→27, ü 252→28, Ä 196→29, Ö 214→30,
Ü 220→31. The result gets bit 30 (distinguishes from control codes 26..31 = cursor keys!).
So umlauts lie at 26..31 in the TED font.

### `mydata`/`comdata` (6027) – service/data table for blitz2 (a1 at init, l.81)
| Off | Label/content | Type | blitz2 uses |
|---|---|---|---|
| -8 | `screenclose` | Code | yes (CloseEd) |
| -4 | `screenopen2` | Code | yes |
| 0 | `openreq` | Code | yes |
| 4 | `closereq` | Code | yes |
| 8 | `getinput` | Code | yes |
| 12 | `error` | Code | yes |
| 16 | `detokallz` | Code | no |
| 20 | `tokall` | Code | no |
| 24 | `ezfreq` | Code | yes |
| 28 | `refgads` | Code | yes |
| 32 | `getinput2` | Code | yes |
| 36 | `combitmap` dc.l 0 | data (set 4698) | yes |
| 40 | `myscreen` dc.l 0 | data | yes |
| 44 | `extint` dc.l 0 | IntuitionBase (l.37) | yes |
| 48 | `colors` | pointer | (grep: no) |
| 52 | `halfcolors` | pointer | (grep: no) |
| 56 | `tokone` | Code | yes |
| 60 | `openio` | Code | yes |
| 64 | `closeio` | Code | yes |
| 68 | `actgad` | Code | yes |
| 72 | `pointeron` | Code | yes |
| 76 | `sleepon` | Code | yes |
| 80 | `mywindow2` dc.l 0 | editor window (4772) | yes |
| 84 | dc.l 0 → at init (external) `defaultreqga9+13` (l.82) | pointer to flags low byte of a default requester gadget ("Icons", bit 7) | grep: no |
| 88 | `extalloc` | Code | yes |
| 92 | `bmapmul` dc 0 | word: bytes/bitplane row | yes |
| 94 | `req_window` dc.l 0 | current requester window | yes |

→ C: **packed structure with fixed offsets**, long at offset 94 (only 2-aligned; with m68k-gcc
ok). `bmapmul`/`req_window` are at the same time normal editor globals.

### Editor globals (6057–6097), word = `dc`
| Name | Type | Init | Purpose |
|---|---|---|---|
| topskip, botskip | W | 0 | pixels occupied by Intuition at top/bottom |
| chcols | W | 128 | columns of the character map (= max. line length) |
| totsccols | W | 80 | character columns on screen in total |
| edrows | W | 0 | text lines on screen |
| pixrows, pixrows2 | W | 0 | editable pixel rows / ×fonthite |
| edcols | W | 0 | editable columns |
| mousecols | W | 0 | columns for "mice" (bookmark column) |
| bitmaps | 2×L | 0 | plane 0, plane 1 |
| cursx, cursy | W,W | 0 | cursor on screen – **copied as long** (storepos, 724, 3929) |
| scloc | L | 0 | byte offset of the cursor line in the plane |
| chloc | W | 0 | offset of the cursor line in chmap |
| cursx2,cursy2,scloc2,chloc2 | W,W,L,W | 0 | backup (storepos/restorepos) |
| cursoff, curstop | W,W | 0 | horiz. scroll offset / first visible line (`clr.l cursoff` 725) |
| color | W | 1 | current drawing colour |
| qualifier | W | 0 | qualifier of the last key |
| chmap | L | 0 | character map (chcols×edrows) |
| anyalt | W | 0 | line changed (≠0) |
| firstitem, enditem, thisitem | L | 0 | line list first/last/current |
| numlines | W | 0 | number of lines |
| bloktopx/y, blokbotx/y | 4×W | 0 | block in line coordinates (long pairs copied 2151ff.; **also misused as long pointer** 1754/1793 and as 1-byte buffer for Write 1694ff.) |
| btopx/y, bbotx/y | 4×W | 0 | block "real" (column/line); bboty=-1 = no block; long pairs (2013ff.) |
| invflag, inverse | W | 0 | inversion flags |
| memstack | L | 0 | a7 for the memory error longjmp (`outmem` → `memcont`) |
| filehand | L | 0 | open file, closed on memory error |
| memalt | W | 0 | memory changed → `printmem` |

### Defaults block `defstart`..`defend` (6099–6120), 50 bytes, **binary** from/to `l:blitzeditor.opts`
Fixed order (loaddefs/savedefs l.3062ff., `defaults` copies word by word):
fonthite 8, fonthite2 7 (=fonthite-1, dbf), fontmul 0, tokenise 0, makeicons 1, normcol 1,
labcol 2, tokcol 3, propcol 0, hscmarg 12, vscmarg 6, tabstop 2, palit 4×RGB word triples
(8,8,8 / 15,15,15 / 0,0,0 / 15,13,0), origscrows 0 (unused). → C: structure big-endian,
without padding, check `sizeof==50`.

### Other data
| Name | Type | Init | Purpose |
|---|---|---|---|
| defname | str | 'l:blitzeditor.opts' | defaults file |
| mpalit | 3×3 W | 15,15,0/15,0,0/0,0,0 | mouse pointer palette |
| statsloc | L | 0 | bitmap address of the status line |
| statstext | str | 'Line:      Column:    Largest Mem (K):      ' | status line |
| bltext | str | 'Block' | status display |
| numtext | 3×W | 0 | number buffer (also written as long) |
| defalt | W | 0 | defaults changed → save at the end |
| req | L | 0 | current requester (openreq) |
| allalt | W | 0 | source changed |
| (without label) | W | ' :' | **sentinel before filepath** (see G) |
| filepath | 192 B | "" | path of the source |
| filename | 64 B | "" | name of the source |
| filetext, filetext2 | str | | requester titles load/save |
| blokpath/blokname | 192/64 B | "" | path/name of the block |
| bloktext, bloktext2 | str | | requester titles block |
| blokop | W | 0 | block operation running |
| mywindow, myrast | L | 0 | main window, RastPort |
| charbuff..charbuff4, dosbuff | L | 0 | pointers into one AllocMem block (doslen 2048 + 3×(chcols+2)) |
| nummice, mice | L,L | 0 | bookmarks count/list |
| mousealt | W | 0 | bookmarks changed → showmice |
| tokeerr | 2 str | '?????','Token Unavailable' | replacement name/help (l.3482) |
| error1/2/3 | str | Couldn't open / reading / writing | error texts |
| returncode | W | 0 | exit code (-1 until normal end) |
| external | L | 0 | edstruct pointer from blitz2 (0 = standalone) |

### `jumptable` (6171) – table of code pointers
Entry 6 bytes: `.b key, .b flag(showvprop afterwards), .l routine` (long only 2-aligned);
end: byte 0.
| Code | Flag | Routine |
|---|---|---|
| 28 | 1 | cursup |
| 29 | 1 | cursdown |
| 13 | 1 | return |
| 255 (=-1) | 1 | gadget (vprop; does **not** check d6) |
| 254 (=-2) | 0 | menu |
| 253 (=-3) | 1 | mousedown |
| 139 ($8B Help) | 0 | help |
| 129–132 (F1–F4) | 0 | blockstart, blockend, blocksave, blokcopy2 |

### `menutable` (6199) – table of code pointers
Same format, end: byte 255. Key = `(item<<4)|menu` (from Intuition code: menu =
code&15, item = (code>>5)&15, subitem ignored; menu 4 → `extmenu`, l.1563).
| Menu | Items (flag) |
|---|---|
| 0 Project | 0 new(0), 1 fileopen(1), 2 filesave(0), 3 defaults(0), 4 doabout(0), 5 prtfile(0), 6 mycli(0), 7 closewb(0), 8 openwb(0), 9 escape(0) |
| 1 Edit | 0 blokcopy(1), 1 blokkill(1), 2 bloksave(0), 3 blokload(0), 4 blokforget(0), 5 insertline(1), 6 deleteline(1), [7 = $f8e1 in getline], 8 join(1), 9 blocktab(1), $a blockuntab(1), $b markindent(0) |
| 2 | 0 top(1), 1 bottom(1), 2 gotoline(1) |
| 3 Search | 0 find, 1 next, 2 previous, 3 replace, 4 replaceall (all 1) |
| 4 | external (COMPILER, blitz2 `menu`) |

### `edstruct` copy area (6267–6282)
`outside` (l.24) copies 14 words = 28 bytes from edstruct (a1) to `tokens`:
| Off | Label | Init (standalone) | Meaning |
|---|---|---|---|
| 0 | tokens | 0 | token list (≠0 → tokenise=1, getstuff) |
| 4 | menitems | 0 | menu texts COMPILER |
| 8 | loadxtra | 0 | callback after loading |
| 12 | savextra | 0 | callback after saving |
| 16 | menurout | 0 | menu callback (extmenu) |
| 20 | cleanup | 0 | callback in `doclean` |
| 24–27 | colon, quote, label, remark | 255 each | separators `:` `"` `.` `;` (255 = off) |
| 28 | dc.b 0 | | terminator (not copied along) |
`returncode` and `external` stand directly before it (not part of the copy).

---------------------------------------------------------------------------------------------
## D. Self-modifying code / data in the code / incbin entries

- No patching of instructions in this scope. All globals sit in the code section and
  are written (C: normal writable data).
- `comdata+84` is filled at init with an address **into the interior** of a gadget structure
  (`defaultreqga9+13`).
- `refme` writes into gadget structures (Flags +12) of the requesters.
- **`rawed` (incbin `rawed.obj`, l.7109, $73E0)**: AmigaDOS load file (HUNK_HEADER $3F3, 1 hunk,
  HUNK_CODE $9C long words = 624 bytes, **no relocs**, HUNK_END) → code from file offset 32,
  position-independent. Jump table:
  - `rawed+32+0` (`bra` init; call l.39): FindTask(0), AllocSignal(-1), builds MsgPort+IOStdReq
    in its own hunk (82 bytes zeroed, ln_Type 5, mn_Length $30), OpenDevice("console.device",
    unit -1, ioreq, 0) – **result unchecked**; ConsoleDevice = io_Device. Then builds two
    105-byte tables (codes 0..$68 without/with Shift) – **never used afterwards** (dead).
  - `rawed+32+4` (l.5928): in d0.w=rawcode, d1.w=qualifier; out d0.w = character (0 = none).
    $50–$59 → +$31 ($81–$8A, F keys), $4C–$4F → −$30 ($1C–$1F, cursor), $5F → $8B (Help),
    otherwise RawKeyConvert (console −48) with static InputEvent (Class 1 RAWKEY, Code, Qual,
    EventAddress 0 → no dead keys), buffer 1 byte, keymap 0 (default); return -1 → 0,
    otherwise first byte (with 0 characters: 0). Saves a0-a2/a6, destroys d1. Special keys
    ignore qualifiers.
  - `rawed+32+8` (l.128): `lea ioreq; move.l 4,a6; jmp CloseDevice(a6)`. Signal is not
    freed.
  Writes into its own data (InputEvent, buffer, device base) → C: completely new as a C module
  (`OpenDevice`/`RawKeyConvert`), no asm needed.

---------------------------------------------------------------------------------------------
## E. Heap/list structures (as far as used here)

- Line (`rsreset` l.14): `nextitem` L +0, `lastitem` L +4, `numchars` B +8, `chars` +9
  (alloc = length+9). `.tababove` reads `thisitem→lastitem→chars` and expects a **0 byte at the
  end** (cf. makeline/charstobuff in other chunks).
- `chmap`: chcols × edrows bytes, characters of the visible lines in full width (bit 7 =
  token colour for `print3`). Line y at chmap + y·chcols (= `chloc`).
- `dosbuff` block: doslen(2048) + 3·chcols + 6; charbuff = +2048, charbuff2/3/4 each +chcols+2.
- IntuiMessage: Class +20 (L), Code +24, Qualifier +26, IAddress +28. Window: FirstGadget +62,
  UserPort +86. Gadget: Flags +12 (W, $80 SELECTED), Activation low byte +15 (bit 2 ENDGADGET),
  GadgetType low byte +17 (1 BOOL, 4 STR), GadgetID +38. PropInfo: HorizPot +2, VertPot +4,
  HorizBody +6.

---------------------------------------------------------------------------------------------
## F. External calls

- exec: WaitPort, GetMsg, ReplyMsg (getinput*, handled0). (rawed: FindTask, AllocSignal,
  OpenDevice, CloseDevice.)
- intuition: NewModifyProp (−468; showvprop/showhprop, a6=`int`), RefreshGList (−432; refme).
- dos: Delay(4) (refme).
- console.device: RawKeyConvert (−48) in rawed.
- Hardware: `flash` writes COLOR00 $dff180 (unused). Otherwise direct bitmap write accesses
  (cursor, shifting characters in getline/char127, pstats/qprint).
- IDCMP main window (newwindow, l.6318): RAWKEY|GADGETUP|MENUPICK|MOUSEBUTTONS|GADGETDOWN,
  flags BACKDROP|BORDERLESS|ACTIVATE (no close gadget).
- Callbacks to blitz2 (data here, call elsewhere): `menurout` (extmenu 1542: d6=item,
  a5=firstitem, d7=numlines → d0.w), `loadxtra`/`savextra` (jsr (a5), d7=handle, a6=DOSBase,
  l.710/795), `cleanup` (doclean 2772, d0-d7/a0-a6 saved).

---------------------------------------------------------------------------------------------
## G. Special handling in C

1. **getline** is a state machine with jumps criss-cross (getline2/curspos/reline/
   char127 as join points). C: one loop `for(;;) { getinput(); switch… }`, `curspos(d1)`
   = `horizpos(d1); continue;`. Only exit = `glinedone`.
2. **`reline` reads its parameter from the stack** (word pushed by the caller, when inserting a tab
   d2 still lies on top of it in between). C: `reline(col)`.
3. **Multiple return** d7/d6 + d0/Z from getinput/handled0; bit 30 in d7 as flag; d7 negative
   for non-keys. C: return struct or globals `ev_code`/`ev_id`; bit 30 as separate flag.
   For blitz2 (comdata+8/+32) register glue is needed (d6/d7 out, a6=exec irrelevant).
4. **Byte comparison in mainedit/jumptable** (`cmp.b d0,d7`): -1/-2/-3 become 255/254/253 –
   in C compare `(UBYTE)code` accordingly, otherwise behaviour changes (see I.6).
5. **Tail calls with a6 change**: showvprop/showhprop end in `jmp NewModifyProp`, horizpos in
   `bra refresh`, refresh in `bra restorepos`, blokfix in `bra blokdis`, getinput2 in `bne
   handled0` – in C normal calls.
6. **Long aliasing of word pairs**: cursx/cursy, cursx2/cursy2, cursoff/curstop,
   btopx/btopy, bbotx/bboty, bloktopx/bloktopy, blokbotx/blokboty are copied/cleared as
   long; bloktopx(+y) moreover as pointer (1754) and byte buffer (1694). C: struct pairs
   or write out the accesses individually.
7. **Sentinel `dc ' :'` before `filepath`** (6139): `outside` checks `-2(a0)==':'` after copying;
   with an empty filepath this hits exactly this byte → no '/'. C: check "empty path" explicitly.
8. comdata as packed table with fixed offsets (incl. −8/−4 before the symbol, word at 92,
   long at 94); defaults block as 50-byte file image.
9. `memstack` longjmp (outmem → memcont) assumes the frame of mainedit → setjmp/longjmp.
10. `refme`/handled0 change gadget flags in foreign (blitz2) requester structures.
11. divu semantics: VertBody/VertPot/HorizPot use 32/16 division with 16-bit quotient; by
   construction no overflow (numerator < denominator) – in C `(UWORD)((ULONG)x<<16 / n)`.
   Tab logic uses `divu` only for the remainder (`x % tabstop`; tabstop=0 from the defaults file →
   division-by-0 trap in the original).

---------------------------------------------------------------------------------------------
## H. Classification

| Routine | Class | Remark |
|---|---|---|
| cursor/cursor2 | UI | bitmap XOR |
| showrow, showhprop, showvprop | UI | number format (numtoascii) KERN, rest UI |
| refresh | UI | |
| inbloks, inblokf, blokfix, blokfixb | KERN | pure logic on globals; blokdis tail = hook |
| getline/2/3, charlow…glinedone, reline, curspos | KERN+UI mixed | Separable: **line buffer operations** on the `a2` line (insert with loss of the last byte, delete, tab in/out, Shift-left/right target column, delete to end, block correction) are KERN; bitmap shifting + print/print3 + cursor as output hooks (`hook_shift_right(x)`, `hook_shift_left(x)`, `hook_drawcell(x)`, `hook_redraw_line()`); getinput as input hook |
| horizpos, cursleft, cursrite | KERN+hook | column/offset logic KERN; `refresh`/`scrollleft`/`scrollrite` as hooks |
| flash | (dropped) | unused, hardware |
| pickwindow, getinput, getinput2, handled0, refme | UI | comdata entries need GLUE (registers d6/d7) |
| rawed (+0/+4/+8) | UI | new in C; special key mapping ($4C–$5F) KERN-testable |
| jumptable/menutable dispatch | KERN | tables in C as struct arrays with function pointers |
| germs | KERN | table |

---------------------------------------------------------------------------------------------
## I. Peculiarities / possible bugs of the original

1. **`blokfixb` .ok2/.ok4 (5403–5413): wrong register.** With a multi-line block (btopy≠bboty)
   `bne .ok4` jumps with d2=**btopy** instead of bbotx; then `cmp chcols,d2`: if the block starts at
   line ≥ chcols (128), bbotx is set to chcols-1 (block end → line end), otherwise
   a bbotx≥chcols stays uncorrected. `blokfix` runs on **every** inserted character
   as soon as a block exists. → `BUG(orig)`.
2. Inserting (`.isasc`, insert tab) silently discards the character in column chcols-1, even if it
   is not a space (no "line full").
3. Shift-Tab (delete tab) deletes **arbitrary** characters up to the previous tab position, not only
   spaces; calls no `blokfix` afterwards; block bounds between new and old column
   are not shifted.
4. Menu $f8e1 (delete to end of line) does not adjust the block.
5. `horizpos`: if `hscmarg` > edcols/2 (defaults freely adjustable), d0=d1-edcols/2 becomes
   negative → negative `cursoff`. Branch `cursoff=chcols-edcols` can yield an **odd**
   cursoff, while cursleft/cursrite work in steps of 2 (word scroll).
6. Byte comparison in `mainedit`: Latin-1 characters $FD/$FE/$FF from RawKeyConvert (Alt combi-
   nations) would be executed as mousedown/menu/gadget; $81–$84/$8B (non-printable) analogously.
   Printable Latin-1 characters ≥ $A0 except the 6 umlauts are never inserted (they end up in
   mainedit, ignored). Umlauts only without an open requester.
7. `handled0` CLOSEWINDOW without `req_window` → `.doesc` reads `62(0)` (null pointer); practically
   unreachable, since the main window has no CLOSEWINDOW IDCMP.
8. `handled0` closes the requester itself on ENDGADGET (`closereq`) – the caller (also blitz2)
   gets d7=-1/d6=ID only afterwards. Esc selects the **last**, Return the **first**
   bool ENDGADGET.
9. `getinput2` returns d7=0 even when an ignored message was read and further ones
   are waiting (only one message per call).
10. `jumptable` entry `gadget` reacts to every gadget ID except hprop (1), does not check d6.
11. `refme` comment "d0.b=pl if select…": parameter unused.
12. `rawed`: OpenDevice failure unchecked (crash in RawKeyConvert with base 0), signal never
    freed, two 105-byte tables built and never read, unreachable `moveq #0,d0; rts`
    at the hunk end.
13. `flash` is dead debug code.
14. `char127` block correction: `blokfixb` only if `inblokf` hit; after `btopx--` alone
    no correction (harmless, since btopx > cursor column ≥ 0).
15. `origscrows` in the defaults block is used nowhere (occupies 2 bytes of the file).
16. MENUPICK: multiple selection (NextSelect) is ignored; MENUVERIFY is only replied to.
