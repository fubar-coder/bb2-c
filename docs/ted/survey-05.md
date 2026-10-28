# Survey 05 – ted.s lines 4198–5252

Scope: tokeniser `tokit` (+ entry `tokit2`/`tokit3` from chunk 04), `putchar`/`pchover`,
`alpha`/`alphad0`, vertical blitter scrolling (`scrollup`/`scrolldown`/`eraseline`),
cursor line arithmetic, line alloc/free (`allocnull`/`freethis`/`freeall`), `donew`,
opening/closing the screen incl. gadget patches and user copper list (`makecop`), colours,
blitter bracket (`getblitter`/`putblitter`), `cls`, horizontal scrolling (`scrollrite`/
`scrollleft`), own text output (`print*`, `qprint`, `fontcalc`) with the fonts `chars.b`/
`chars8.b`, `calcpos`/`calcphys`.

Addresses (reference listing): tokit $460E, tokit3 $4618, tokskip $4626, putchar $4772,
alpha $4786, scrollup $47B6, scrolldown $48D8, eraseline $495C, allocnull $4A50, donew $4A7E,
calcscreen $4B8A, screenopen $4C3A, makecop $4EAA, screenclose $4F7A, getblitter $5048,
cls $50A8, scrollrite $511C, scrollleft $51D0, printa0 $52C6, print2 $5340, print4 $5364,
qprint $53B6, fontcalc $53CE, calcphys $5424.

---------------------------------------------------------------------------------------------
## A. Routines

### Tokeniser

Context (chunk 04, needed here): `charstobuff` (4173) pushes `a1` (destination buffer start), sets
`d1 = chcols-1` and jumps in via **`bne tokit`** (no `bsr`!) if `tokenise`≠0 → the
`rts` of `enddo`/`tokit` returns directly to the caller of `charstobuff`.
`tokit2` (4195) pushes `a1` itself and jumps to `tokit3`. Common exit `enddo` (4190,
chunk 04): `clr.b (a1)+ ; d5 = a1 - (a7)+ ; rts`.

- `tokit`=`tokit4` (4198/4202) : in: a2=source, a1=destination, d1.w=**index of the last character**,
  stack: [destination start][return]. Searches backwards for the last character ≠ ' ' (comparison on the **raw**
  byte, $A0 does not count as space). All spaces → `addq #4,a7` (destination start gone),
  `clr.b (a1)`, d5=1, rts (a1 points AT the 0, not behind it – unlike enddo!).
  Otherwise → `tokskip`. Several entries (tokit/tokit4 identical, tokit3).
- `tokit3` (4204) : entry from `tokit2`: first `subq #1,d1` → callers of tokit2 pass
  d1 = **length** (not last index). Callers: `tokone` (d1=strlen), `tokall` (d1=numchars-1
  = strlen without NUL), `mytoke` (d1=d5).
- `tokskip` (4210) : main loop. Pushes a4 (stack: [a4][destination start][ret]). d0.w=0 =
  number of consumed source characters, `invflag`=0 (quote mode). Local states `.loop2`,
  `.quoteon`, `.pch`, `.hendrix`, `.ppp2`, `.remloop`, `.notrem`, `.hexloop`, `.nothex`,
  `.tsttoke`, `.loop3/.loop4` (name comparison), `.next`, `.notoke`, `.unto2`, `.found`,
  `.putina3`. **No normal end**: it is left only via `putchar`→`pchover` or
  `.found`→`pchover2` → `bra enddo`. out: d5 = output length incl. NUL, a1 = behind NUL,
  a4 restored; destroys d0,d2,d3,d4,a2,a3. `.remloop` is an endless loop that only
  ends through pchover.
- `putchar` (4319) : `(a1)+ = d2`, d0++, if d0 > d1 (unsigned, `bhi`) → `pchover`.
  **May only be called inside tokskip** (stack assumption).
- `pchover` (4324) : `addq #4,a7` (discards return of putchar) → `pchover2` (4325):
  `move.l (a7)+,a4` → `bra enddo`. Non-local exit across two levels.
- `alpha` (4328) : in d2.b ; out Z=1 if 'A'–'Z' or 'a'–'z' (via `cmp d2,d2`), otherwise Z=0;
  destroys nothing. Also used by search (3552).
- `alphad0` (4340) : like alpha for d0 (exg). User: search routine 3545.

Tokeniser algorithm (exact):
1. `.loop2`: d2=(a2)+ & $7F. d2==`quote` → `.quoteon`: invflag=~invflag, putchar, continue.
2. `.hendrix`: invflag≠0 → putchar (characters in quotes raw). Letter → `.tsttoke`.
3. `.ppp2` (non-letter): `_` `.` `\` → `.unto2`. ==`remark` → copy rest up to d1.
   ==`$` → `.hexloop`: copy '$' and following hex digits. Then `.nothex`: current
   character putchar **unchecked** (also quote/remark/`_`!), read next: quote→`.quoteon`,
   no letter→`.ppp2`, letter→`.tsttoke`. All other characters (digits, operators,
   spaces) likewise go via `.nothex`.
4. `.tsttoke`: a2 back to word start, walk token list from `tokens` linearly. Node:
   +0 next (l), +4 token word (w), +6 name (NUL-term.; in blitz2 it is followed by the
   help text). Comparison `or.b #32` on both sides. Name ended → `.found`: character after it
   (& $7F) letter or `_` → next node (digits may follow). Match → output
   `(Hi | $80), Lo`, d0 += name length, d0>d1 → end (token has already been written),
   a2 behind name, `.loop2`.
5. No match (`.notoke`) → `.unto2`: the first character and all following **letters** are
   copied; quote → `.quoteon`; first non-letter character → `.ppp2` (so a digit after an
   identifier ends it: `x1print` → x 1 Print).

#### Comparison with bb2 `compilerlauf/tools/bb2tok.py`
bb2tok.py reproduces the state machine correctly (incl. `.nothex` quirk, `or $20`,
digits after names, `_` `.` `\`). Not modelled:
- **Quote/remark characters are runtime variables** (`quote`, `remark`, copied from edstruct in
  `outside` ted.s:24ff, blitz2.s:280 `':".;'`). Initial value 255 → without blitz2 (standalone)
  there is no quote and no remark mode (byte & $7F can never be 255). bb2tok
  hard-codes `"`/`;`.
- **Token list = linked list from blitz2 at runtime** (`tokens`), order as linked
  there (incl. libs loaded at runtime). bb2tok uses fixed tables. A node with an
  empty name would match immediately (`.loop4`: first byte 0 → `.found`); bb2tok filters via
  regex `\S+`.
- **Reading beyond d1**: name comparison (`.loop4`) and `.found` read from a4 without
  length check. Harmless with source = line buffer (NUL-terminated). With source = chmap line
  (`charstobuff`, 128 columns, no terminator) and text up to column 127, reading continues **into the next
  chmap line** (e.g. "…Pri" + next line "nt…" → token Print, then
  pchover2). bb2tok treats the line end as a space.
- **Trailing space trim on the raw byte**: `$A0` at the line end is not removed, but is output
  as ' '. bb2tok: `rstrip(b' ')` before masking → identical. (ok)
- **Bit 7 is cleared everywhere**, also in strings/comments (ä=228 → 'd'). Reason: in the
  chmap bit 7 marks "token colour" (print2/tokprint). Umlauts are mapped on input via
  `germs` (6036) to 26–31. bb2tok masks as well (ok), the umlaut mapping is missing there.
- **In-place tokenisation**: `tokone` and `mytoke` call with a1==a2. Works because
  tokens are ≥2 characters long (output never overtakes input). A 1-character token name would
  overwrite the next source character before it is read.
- **Empty list** (`tokens`=0): `.loop3` reads nodes at address 0 (name from address 6 =
  ExecBase bytes). Only prevented because `tokenise` is only set if tokens≠0
  (`getstuff` 2781) – `tokall` (via comdata) does not check this.
- `invflag` is also used by the block marking (2033, 2201–2221) as an entirely different flag;
  tokit sets it to 0/toggles it (side effect on global state).
- Empty line: asm `a1` afterwards points to the NUL (not behind it); length d5=1 like enddo.

### Vertical scrolling (blitter, a5=$dff000, a6=GfxBase)
- `scrupall` (4345) : d7=0, **falls through** into `scrollup`.
- `scrollup` (4346) : in d7.w = first screen row; shifts rows d7+1..edrows-1 up by
  one, clears the last row. d6=edrows-d7-1 (no check; d7≥edrows → d6<0 →
  giant blit). chmap: blit A=chmap+(d7+1)*128 → D=chmap+d7*128, mods 0. Bitplanes each via
  `scup2`. Fill last chmap row with $2020. Tail `bra putblitter`. destroys d0–d7,a0,a1,
  **a5**, a6.
- `scup2` (4407) : internal; a0=plane. Copy (if height≠0, `cmp #64,d4`), then fill the last row with
  BLTADAT=0 or $FFFF (if screen row curstop+edrows-1 lies in the block btopy..bboty),
  BLTCON0=$1F0. Tail `jmp waitblit(a6)`.
- `scrdownall` (4433) : d7=0, falls through into `scrolldown`.
- `scrolldown` (4434) : in d7 ; rows d7..edrows-2 one down (descending, BLTCON1=2),
  then **falls through** into `eraseline` (at d6=0 directly `beq eraseline`). d5=offset of the last word
  of the edit area (pixrows22-blitmod-2).
- `scdown2` (4523) : internal, one plane blit, tail `jmp waitblit`.
- `eraseline` (4479) : in d7, blitter already owned (only jump target/fall-through). Fills
  bitplane row d7 (both planes) with 0 or $FFFF (block), chmap row d7 with $2020.
  **d7 is multiplied by chcols (destroyed)**. Tail `bra putblitter`.

### Cursor/position
- `godown` (4530) / `goup` (4538) : cursy±1, scloc±=fontmul (long), chloc±=chcols. No
  range check. destroys d0. Multiple tail call target (1064, 1225).
- `storepos` (4976) / `restorepos` (4981) : cursx+cursy (as long) / scloc / chloc ↔ *2 copies.
  restorepos frequently a tail target (`bra restorepos`, `beq restorepos` 4067).
- `calcpos` (5234) : scloc=cursy*fontmul, chloc=cursy*chcols. destroys d1.
- `calcphys` (5244) : out a0=bitmaps[0]+scloc+cursx, a1=bitmaps[1]+scloc+cursx (cursx as
  word, sign-extended).

### Line list
- `allocnull` (4547) : AllocMem(lenitem+1=10, MEMF_PUBLIC) ; out d0=a0=item, **Z=1 on
  failure** (tst.l), otherwise Z=0 (last `move #$100`). Init next=last=0, word $0100 at
  numchars → numchars=1, chars[0]=0. memalt=-1. a6=ExecBase.
- `donew` (4560) : "clear everything": color=normcol, `blokdis`, cursx/cursy/scloc/chloc/curstop/
  cursoff/allalt=0, filepath/filename="", numlines=1, `allocfirst` (→allocnull + **cls**),
  showhprop, showvprop; out d0=-1 → **always Z=0**. Comment "z=1 if problem" is wrong
  (failure of allocnull is ignored). Does NOT free old lines (callers: freeall before).
- `freethis` (4582) : frees `thisitem` (does not unlink!). First non-space character ==
  `label` → `mforget2` (only sets mousealt=-1). FreeMem(a1, lenitem + numchars) with
  **`add.b`** (see I). memalt=size. Tail `jmp freemem`.
- `freeall` (4602) : frees numlines items from firstitem (via nextitem, read before FreeMem).
  `dbf` with numlines-1 → numlines=0 would be 65536 iterations. List globals are left as they are.

### Screen
- `calcscreen` (4619) : GetScreenData(scdata,84,WBENCHSCREEN,0); width &$FFF0 (comment says
  "multiple of 8", actually 16). topskip=12, botskip=8 hard-coded. Sets newwindow.TopEdge, viewmode
  (=WB ViewPort.Modes, scdata+76), newscreen/newwindow Width, totsccols=width/8,
  edcols=totsccols-mousecols-2, edrows=(H-topskip-botskip)/fonthite-1, pixrows=edrows*fonthite,
  newwindow.Height=pixrows+fonthite+botskip+1 (status line), newscreen.Height=+topskip.
- `screenopen` (4668) : calcscreen, OpenScreen → myscreen, winscreen (=NewWindow.Screen),
  reqstruct+12. Reads RastPort.BitMap (Screen+88): bmapmul=BytesPerRow, blitmod=bpr-edcols,
  pixrows2 (word, truncated)/pixrows22 (long)=bpr*pixrows, fontmul=bpr*fonthite,
  combitmap=plane0, bitmaps[0..1]=plane0/1+topskip*bpr, statsloc=plane1 row
  (topskip+pixrows+botskip+1). chmap=AllocMem(chcols*edrows, **MEMF_CHIP**). Patches gadgets/
  borders (see C), OpenWindow → mywindow/mywindow2, SetMenuStrip(menu1), myrast=Window.RPort,
  DrawBorder×2, `colors`, `pstats` (status line text), mousealt=-1, save pr_WindowPtr (task+184)
  → oldmywindow and set it to mywindow, `pointeron`, screensopen=-1, `makecop`.
  out: **d0=0/Z=1 = OK**, d0=-1/Z=0 = failure. Failure paths `.wfail/.cfail/.sfail` (see I).
- `screenopen2` (4661) : screenopen, showvprop, showhprop, tail `bra showall`. Ignores
  failure of screenopen. **Called by blitz2** (`mydata`+4 = comdata-4).
- `makecop` (4823) : only if `copok`=0. col2 = RGB4 of colour 2 from palit. AllocMem(12,
  PUBLIC|CLEAR) = struct UCopList → `coplist`; UCopperListInit(ucl,24); for row 0..11:
  CWait(ucl,v,0),CBump, CMove(ucl,$184 (COLOR02),cols[v]),CBump; CWait(10000,255) (without CBump).
  ucl->Next = vp->UCopIns; vp->UCopIns = ucl (Screen+44+20); tail `jmp RemakeDisplay`
  (Intuition). Colour gradient in pen 2 over the title bar. destroys a5,a6,d6,d7.
- `pstats` (4880) : in a3=NUL string, a1=destination (1 plane); per character qprint, a1++. Tail target
  (3031, 3901, 5273).
- `freechmap` (4888) : FreeMem(chmap, chcols*edrows); a6=Exec afterwards. Tail `jmp freemem`.
- `screenclose` (4895) : screensopen=0, pr_WindowPtr back, freechmap, flushmw, ClearPointer,
  ClearMenuStrip, CloseWindow, tail `jmp CloseScreen` (also frees the UCopList; `coplist`
  stays dangling, unused). **Called by blitz2** (`mydata`+0 = comdata-8).
- `mwait` (4913) : busy wait until CIA-A PRA ($BFE001) bit 7 = 0 (fire button port 2).
  **No caller** – dead debug code.
- `halfcolors` (4917) : SetRGB4 colours 0–3 with halved palit values. From blitz2 (comdata+52).
- `colors` (4940) : SetRGB4 colours 0–3 from palit, then 17–19 from mpalit (mouse pointer sprite).
  From blitz2 (comdata+48) and internally. Both: no register saving (d0–d3,d6,d7,a0,a4–a6).

### Blitter / clearing / horizontal scrolling
- `getblitter` (4964) : OwnBlitter, WaitBlit, **a5=$DFF000**, BLTAFWM/ALWM=$FFFF, BLTCON1=0.
  out: a5=custom, a6=GfxBase (callers rely on this for `jsr waitblit(a6)`).
- `putblitter` (4973) : WaitBlit, tail `jmp DisownBlitter`.
- `cls` (4986) : chmap completely with $2020 (blitter fill), then edit area of both planes
  (`cls2`, pixrows rows × edcols/2 words) with 0. Cursor stays. Tail `bra putblitter`.
  `cls2` (5006) only starts the blit (no wait; caller waits).
- `scrollrite` (5019) : bitplanes by 1 word (2 characters) to the right, descending
  (BLTCON=$09F0_0002), then redraw the left 2 columns from chmap+cursoff (print3), row by
  row. Caller must have adjusted cursoff before. a5 is afterwards reused as
  chmap pointer. color=normcol, tail `bra restorepos`.
- `scrollleft` (5069) : analogous to the left (ascending, BLTCON1 from getblitter=0), redraw the right 2
  columns (edcols-2, edcols-1).
- `scrite2`/`scleft2` (5064/5108) set A/D and **fall/jump** into `hscmods` (5112):
  mods=blitmod+2, BLTSIZE=pixrows<<6 | (edcols-2)/2 (starts the blit).

### Text output
- `print3` (5125) : d0 with bit 7 → color=tokcol otherwise normcol, clear bit 7, a0=`print4`,
  tail into `printa0`. Only draws, does not write into chmap.
- `print` (5132) : a0=`print2`, **falls through** into `printa0`. "print d0 at cursx,cursy in color".
- `printa0` (5135) : in d0=character, a0=drawing routine. movem d2-d4/a2. Determines `inverse`
  (0/-1) from the block bounds (y=curstop+cursy unsigned, x=cursoff+cursx **signed**):
  y<btopy → no; y==btopy: x<btopx → no, otherwise if y<bboty → yes, otherwise x>bbotx → no/yes;
  btopy<y<bboty → yes; y==bboty: x≤bbotx → yes; y>bboty → no; bboty==-1 → no block.
  Then `prinv`: `jsr (a0)` (computed call). destroys d0,d1,a0,a1.
- `prinv` (5163) : label in the middle of printa0, internal only.
- `print2` (5167) : writes d0 (|$80 if color≠normcol) to chmap[chloc+cursoff+cursx],
  **falls through** into `print4`.
- `print4` (5178) : calcphys, fontcalc; per glyph row: plane0 = glyph & (color bit0 ? FF:0)
  ^ inverse, plane1 = glyph & (color bit1 ? FF:0) ^ inverse. fonthite2+1 rows, glyph stride
  16, bitmap stride bmapmul. CPU write accesses into chip RAM.
- `qprint` (5205) : d0 character, a0 destination, 1 plane, without colour/inverse. destroys d0,d1,a0,a2.
- `fontcalc` (5214) : in d0 (0–127) ; out a2=glyph. font8: +(c&15)+(c>>4)*128;
  font6 (if fonthite==6): +(c&15)+(c>>4)*96. destroys d0,d1.

---------------------------------------------------------------------------------------------
## B. Register conventions

- Tokeniser: a2=source (running), a1=destination (running), d1.w=last index, d0.w=counter, d2=character,
  a3=token node, a4=lookahead (saved), stack holds destination start (for d5). Output d5.
- Blitter routines: **a5=$DFF000, a6=GfxBase** between getblitter and putblitter; all
  subroutines (`scup2`, `scdown2`, `cls2`, `hscmods`, `eraseline`) assume this.
  Thus a5 is destroyed in all scroll routines.
- d7 = screen row for scrollup/scrolldown/eraseline (is destroyed).
- Character output: d0=character; printa0 preserves d2-d4/a2 (movem), rest free.
- Exec calls via `move.l 4,a6`; afterwards a6=ExecBase remains (relevant for .cfail/.wfail).
- No routine here has `movem` saving for blitz2 calls: `screenclose`, `screenopen2`
  (mydata), `colors`, `halfcolors` (comdata) are called by blitz2 and destroy almost all
  registers incl. a5/a6 → check the caller's expectation in blitz2 (glue).
- `tokone` (comdata+56, chunk 04) → `tokit2`: likewise without saving; `mytoke` saves.

---------------------------------------------------------------------------------------------
## C. Globals in this scope

| Name | Line | Type/Init | Purpose |
|---|---|---|---|
| `scdata` | 4617 | ds.b 84 | copy of WB screen (GetScreenData); +12 Width (patched &$FFF0), +14 Height, +76 ViewPort.Modes |
| `blitmod` | 4666 | w 0 | blitter modulo = BytesPerRow-edcols |
| `oldmywindow` | 4811 | l 0 | saved pr_WindowPtr |
| `screensopen` | 4812 | w 0 | -1 if screen open (read 2555, 2606) |
| `coplist` | 4814 | l 0 | UCopList (only written) |
| `cols` | 4816 | 11 w | colour gradient $357…$246 for the title bar |
| `col2` | 4819 | w 0 | 12th entry of `cols` = real colour 2 (at runtime) |
| `copok` | 4821 | w 0 | ≠0 → no makecop (toggled in `defaults` 411/413) |
| `pixrows22` | 5017 | l 0 | bpr*pixrows as long |

They sit in between code (data in the code hunk), in C normal globals.

Structures patched at runtime (defined outside, offsets important):
- `newscreen` (NewScreen): +4 Width, +6 Height; `viewmode` (= +12 ViewModes).
- `newwindow` (NewWindow): +2 TopEdge=topskip, +4 Width, +6 Height; `winscreen` (+30 Screen).
- `reqstruct+12` = screen pointer.
- `newgads` (Gadget, vert. prop right): +4 LeftEdge=edcols*8+3, +6 TopEdge=2,
  +8 Width=10, +10 Height=pixrows-4.
- `newgads2` (Gadget, horiz. prop bottom): +4 LeftEdge=3, +6 TopEdge=pixrows+2,
  +8 Width=edcols*8-6, +10 Height=4.
- `prop2sinfo+6` (PropInfo.HorizBody) = edcols*65536/chcols.
- Border XY tables: `ngc1+10`, `ngc2+6`, `ngc2+10` = pixrows-1; `ngc3`,`ngc4`,`ngc4+4` =
  edcols*8-1; `ngc3+10`,`ngc4+6`,`ngc4+10` = 7. Borders `newgadbord`+0 / `nextngad`+0 =
  edcols*8 (LeftEdge), `newgadbord2+2` / `nextngad2+2` = pixrows (TopEdge).
- Order matters: newgads+4/+10 are read first (for borders), then adjusted with `addq/subq`
  – screenopen is idempotent only because of the preceding absolute reassignment.

Globals read (defined outside): quote/remark/label/colon (bytes, init 255, from edstruct),
tokens, invflag, inverse, color (word; bits tested via `color+1`), normcol, tokcol,
fonthite/fonthite2/fontmul, bmapmul, chcols (128, never changed), edcols, edrows, pixrows,
chmap, bitmaps[2], cursx/cursy/scloc/chloc(+2), cursoff, curstop, btopx/btopy/bbotx/bboty,
palit/mpalit, memalt, mousecols (0/12), totsccols, topskip/botskip, statsloc, combitmap.

---------------------------------------------------------------------------------------------
## D. Self-modifying code / data in the code

- No writing to instructions in this scope.
- PC-relative data in the code hunk are written: `scdata` (also patched via `and`),
  `blitmod`, `col2` (extends the table `cols` by one entry – `cols`+`col2` must be one
  array [12] in C), `oldmywindow`, `screensopen`, `coplist`, `pixrows22`.
- Patched Intuition structures (see C).
- Fonts `font6`/`font8` = `incbin chars.b` (768 B) / `chars8.b` (1024 B), directly one after the other:
  128 characters each, 16 characters per "font row", glyph row stride 16 bytes, 8 px wide.
- No entries into incbin code.

---------------------------------------------------------------------------------------------
## E. Heap/memory structures

- **Line item** (offsets from rs definition lines 16–20): +0 nextitem (l), +4 lastitem (l),
  +8 numchars (b, **incl. NUL**), +9 chars[]. lenitem=9. Allocation numchars+9, MEMF_PUBLIC.
  Empty item: 10 bytes, numchars=1, chars="\0" (word write access $0100 at +8).
- **chmap**: edrows × chcols(128) bytes, **chip RAM** (processed by the blitter). Row r =
  chmap+r*128; visible columns cursoff..cursoff+edcols-1. Byte = character, bit 7 = drawn in
  non-normal colour (token). Empty = ' '.
- **Bitplanes**: screen BitMap, 2 planes, bitmaps[i] = plane i + topskip*bpr; character cell
  8 px × fonthite, byte address = bitmaps[i] + cursy*fontmul + cursx. Status line in plane 1
  from `statsloc`.
- **UCopList** 12 bytes (Next, FirstCopList, CopList), AllocMem PUBLIC|CLEAR, belongs to the
  ViewPort after linking in (CloseScreen frees it).
- `memalt` is set on every alloc/free (-1, 2 or size) → display of free memory.

---------------------------------------------------------------------------------------------
## F. External calls / hardware

- exec: AllocMem, FreeMem, FindTask(0) (set/reset pr_WindowPtr @184).
- intuition: GetScreenData, OpenScreen, CloseScreen, OpenWindow, CloseWindow, SetMenuStrip,
  ClearMenuStrip, ClearPointer, DrawBorder, RemakeDisplay.
- graphics: OwnBlitter, DisownBlitter, WaitBlit, SetRGB4, UCopperListInit (-594), CWait (-378),
  CMove (-372), CBump (-366) – the last four by number, not by equate.
- **Direct hardware access**:
  - Blitter via `$DFF000` (a5): BLTCON0 $040 (values $09F0 = A→D, $01F0 = D=BLTADAT fill),
    BLTCON1 $042 (0 / 2=DESC), BLTAFWM/ALWM $044, BLTAPT $050, BLTDPT $054, BLTSIZE $058,
    BLTAMOD $064, BLTDMOD $066, BLTADAT $074. Always between OwnBlitter/DisownBlitter, every
    blit finished with WaitBlit (clean). BLTSIZE OCS only (10 bits height, 6 bits width).
  - Blitter fill/copy also on **chmap** (character memory, not only graphics).
  - CPU writes glyphs directly into the screen bitplanes (print4, qprint) → only planar
    chip RAM bitmaps; no RTG.
  - CIA-A `$BFE001` bit 7 in `mwait` (dead).
  - Copper only via OS (UCopList), colour register COLOR02 ($184) via CMove.
- Timing: no delays/VBlank; only busy wait in `mwait` (unused) and WaitBlit.
- Callbacks into blitz2: none in this scope. Called by blitz2: screenclose, screenopen2,
  colors, halfcolors (and indirectly tokit via tokone/tokall).

---------------------------------------------------------------------------------------------
## G. Special handling in C

- Tokeniser: non-local exit (pchover: `addq #4,a7`, stack pop across levels) → in C as
  loop with `goto done`/return value; putchar as macro/inline with length check.
  Entries tokit (d1=last index, destination start on stack) vs. tokit2 (d1=length) as two
  C functions on one core; charstobuff jumps in via `bne` (no return there).
  Return: length incl. NUL (d5) **and** end pointer a1 (different for an empty line).
  Quote/remark characters and token list as parameters (host test!). Reading beyond d1
  (chmap case): reproduce or document deliberately as `BUG(orig)`.
- `alpha`: Z flag return → `bool`.
- Blitter routines: possible in C with `custom.h` (`custom->bltcon0` …) + OwnBlitter/WaitBlit,
  no inline asm needed. Recommendation: chmap blits (copy row, fill with $20) in C as
  memmove/memset in a KERN layer, bitplane blits separate (UI). Caution: chmap blits
  currently run in parallel with/before the bitplane blits – order irrelevant (independent data).
- BLTSIZE width 0 = 64 words is exploited (chcols=128 → chcols/2=64 → `&63`=0); at two
  places the `&63` is missing (see I) – treat chcols as constant 128 in C.
- Byte/word aliasing: `color` (word) bits tested via `color+1`; numchars initialised via word access
  together with chars[0]; `cursx` long access clears/copies cursx+cursy
  (cursx2/cursy2 likewise) → keep layout `cursx,cursy` or `cursx2,cursy2` as a pair or
  both fields explicitly.
- `inverse` word, but used as byte mask `eor.b` (0/$FF).
- Computed call `jsr (a0)` in printa0 → function pointer.
- Flag returns: allocnull (Z=1 failure), screenopen (Z=1 success, d0=0), donew (always
  "success"), alpha (Z).
- blitz2 entries (screenclose, screenopen2, colors, halfcolors) need glue with the
  register expectation of blitz2 (check there).
- Mixture of signed/unsigned in printa0 (y: bcs/bhi, x: blt/bgt/ble) take over exactly.

---------------------------------------------------------------------------------------------
## H. Classification

| Routine | Class | Remark |
|---|---|---|
| tokit/tokit2/tokit3/tokit4/tokskip | KERN | pure; token list+quote/remark as input |
| putchar/pchover/pchover2 | KERN | internal to the tokeniser |
| alpha/alphad0 | KERN | |
| godown/goup/storepos/restorepos/calcpos | KERN (state) | pure arithmetic on cursor globals, host-testable if fontmul/chcols are set |
| allocnull/freethis/freeall | KERN | AllocMem/FreeMem can be abstracted; freethis→mforget2 only sets a flag |
| donew | KERN+UI | hook: blokdis, cls (via allocfirst), showhprop/showvprop as callbacks |
| fontcalc | KERN | pure address computation, font data needed |
| print2 | KERN+UI | chmap writing (KERN) separable from drawing (print4) |
| scrollup/scrolldown/eraseline/cls | UI (+KERN part) | chmap part separable as memmove/memset |
| scrollrite/scrollleft/hscmods/scrite2/scleft2 | UI | |
| print/print3/printa0/prinv/print4/qprint/pstats/calcphys | UI | block inverse decision in printa0 is pure logic (separable) |
| getblitter/putblitter | UI | hardware; C with custom.h |
| calcscreen/screenopen/screenopen2/screenclose/makecop/freechmap | UI | calcscreen geometry testable as pure function |
| colors/halfcolors | UI | called by blitz2 → GLUE wrapper |
| mwait | UI | dead code (CIA) |

---------------------------------------------------------------------------------------------
## I. Peculiarities / possible bugs (BUG(orig))

1. **screenopen failure paths call CloseScreen with a6=ExecBase** (4806/4807): `.cfail` after
   AllocMem (a6=Exec) and `.wfail` after freechmap (a6=Exec) → `jsr -66(a6)` = exec vector
   instead of intuition CloseScreen. Risk of crash on chip RAM shortage/OpenWindow failure.
2. `screenopen2` ignores the failure of screenopen and keeps drawing (showvprop/showall).
3. `makecop`: AllocMem result not checked; last CWait without CBump (CEND incomplete);
   UCopperListInit with 24 for 24 instructions + end wait (tight).
4. `scrolldown` chmap blit (4453–4457): BLTSIZE without `&63` → `d6<<6 | 64` → with even d6
   one row too many (at d7=0 reads one row before chmap). Without consequence, because eraseline clears row
   d7 afterwards. `scrollup` 4400–4403 likewise without `&63`, correct only because 64|64=64.
5. `freethis`/`freeall`: size `moveq #lenitem,d0 ; add.b numchars,d0` – byte addition,
   overflows at numchars ≥ 247 (FreeMem with wrong size). Allocation (1378 etc.) uses
   word `add`.
6. `donew`: comment "z=1 if problem", but always returns d0=-1/Z=0; allocnull failure
   (firstitem stays old/invalid) is swallowed.
7. `halfcolors`/`colors`: `moveq #0,d2` twice instead of `d3` (typo; upper half of d3
   undefined, SetRGB4 only uses the lower 4 bits).
8. `calcscreen`: comment "multiple of 8", mask $FFF0 = 16. No lower bound for edrows.
   At width ≥ 1040 px edcols becomes ≥ 128 = chcols → `divu` (prop2sinfo) overflows,
   chmap rows too short for the visible area.
9. `scrollup`/`eraseline`: with a block the new row is inverted over the **whole width**
   ($FFFF), without btopx/bbotx – at the first/last block line possibly wrongly inverted cells
   beyond the line end, until redrawn.
10. Tokeniser reads beyond the line end during name comparison (chmap case: into the next
    chmap row, see A); `tokens`=0 → access from address 0.
11. `fontcalc` without range check: characters ≥128 read behind `chars.b` into `chars8.b` or
    beyond `chars8.b` (print2/print4 do not mask bit 7, only print3 does).
12. `freeall` with numlines=0 → dbf underflow (65536 frees).
13. `scrollup`/`scrolldown` without check d7 < edrows (negative d6 → huge BLTSIZE).
14. `pixrows2` only stored as word (bpr*pixrows > 65535 truncated; not read in
    this scope).
15. `mwait` tests $BFE001 bit 7 (joystick fire port 2, not left mouse button bit 6); dead
    code.
16. After `tokit` with an empty line a1 points to the NUL, otherwise behind it –
    inconsistency if a caller keeps using a1.
