# Survey 02 – ted.s lines 1016–1980

Scope: `escape` … `blokforget` (end before `gomouse`, 1981). Content: quitting, cursor
up/down (incl. page-wise with Shift), `markindent`, insert/delete line,
`join`/`split`/`return`, `top`/`bottom`, V-prop gadget, `gotolined0`/`gotolined02`,
external menu (`extmenu` → blitz2 `menurout`), menu dispatch (`menu` via `menutable`),
`new`, block operations (`blokkill`, `blocksave`/`bloksave`, `blokload`, `blokcopy`,
`blokforget`).

Addresses (vasm reference, `ted.labels`): escape 1BA8, cursdown 1BEE, beepem 1C20,
cursdownok 1C30, movecdown 1C36, cursup 1C68, markindent 1CC4, insertline 1D64,
deleteline 1DBA, freelast 1E54, freeonly 1EA6, join 1EAE, split 2002, return 215A,
top 2232, bottom 2238, gadget 2242, gotolined0 22AA, gotolined02 22B8, gd03 22BA,
extmenu 2362, menu 2396, new 23D6, deftitname 23F8, blokkill 2412, blocksave 2502,
bloksave 2514, blokload 25DC, done3 2670, done2 2684, done4 268C, done 269C,
blalldone 26BE, blokcopy2 26D6, blokcopy 26DE, bcopyok 272C, blokforget 2914.

Important foreign routines (semantics only as far as needed, details in the other surveys):
`makeline` 3803 (replaces item a4 by a new item from `charbuff`/d5, frees the old one via
`freethis`, sets `thisitem`=a4=new, adjusts `firstitem`/`enditem`),
`makefirst` 3799 (= `charstobuff(a2=charbuff2)` + `makeline`), `charstobuff` 4173
(a2 → `charbuff`, reads exactly `chcols` characters, trailing spaces cut off,
NUL, d5 = length **incl.** NUL; with `tokenise` via `tokit`), `freethis` 4582 (frees
**`thisitem`**, not a4!), `freeupa5` 3684 (unlink+free a5, `numlines--`;
at `numlines==1` new empty item via `afirst2`), `allocnull` 4547, `linka0` 3277,
`allocline` 3349, `bloksetup`/`bloksetup2` 3752/3727, `getbchar`/`getbchar2` 3849/3865,
`detoka5` 3759, `blokadj` 3919, `blokfix` 5384, `blokdis` 3903, `bloken` 3890,
`calcitem` 3783, `alloc2`/`outmem` 3033/3038, `erase`/`erase2` 3257.

---------------------------------------------------------------------------------------------
## A. Routines

Convention: a4 = current item (`thisitem`) on entry (set by `mainedit`,
ted.s:145). All routines here are called from `jumptable` (6171) or `menutable` (6199)
via `jsr (a0)`, return values/CCs are irrelevant for the caller, except where
stated. "→ X" = `bra X` (tail call).

- `escape` (1016) : menu QUIT ($90). `erase` (query if `allalt`; cancel → `erase`
  does `addq #4,a7`, i.e. return directly after `menu`). Frees the external menu
  (see E), `doclean` (callback `cleanup`), `freemice`, `clr allalt`, then
  **`lea 12(a7),a7`** (discards: return into `menu` 4 + flag byte from `menu` 2 +
  return into `mainedit` 4 + flag byte from `mainedit` 2) and → `freeall`; its `rts`
  returns behind `bsr mainedit` (ted.s:101) = editor end. **Only reachable via
  `menu`** (stack depth hard-wired). Does not return to the caller.
- `cursdown` (1041) : Shift (`qualifier&3`): d0=curstop+cursy+edrows, min(numlines−1)
  → `gotolined02`. Otherwise: `nextitem(a4)`=0 → `beepem`; otherwise falls through into
  `cursdownok`.
- `beepem` (1054) : `pointeron`, → `DisplayBeep(myscreen)` (jmp into intuition). Used by
  many routines (also outside, e.g. 328, 903, 2440) as tail target; once
  via `bsr` (494). No return value.
- `cursdownok` (1058) : in: d0=next item. `thisitem`=d0 (a4 stays old!), falls into
- `movecdown` (1061) : if curstop+edrows ≥ numlines or cursy+1+vscmarg < edrows →
  `godown` (cursor arithmetic only); otherwise `curstop++`, `scrupall`, → `showbotitem`.
  Second entry from `return`.
- `cursup` (1074) : Shift: d0=curstop+cursy; <edrows → 0, otherwise −edrows → `gotolined02`.
  Otherwise `lastitem(a4)`=0 → `beepem`; `thisitem`=prev; curstop=0 or cursy>vscmarg →
  `goup`; otherwise `curstop--`, `scrdownall`, → `showtopitem`.
- `markindent` (1098) : menu $b1. d0 = indentation (leading spaces) of line a4; empty
  line (chars[0]=0) → `beepem`. Walks up/down as long as lines are empty (only
  spaces) or indentation ≥ d0 → d1=upper, d2=lower line. Block := (0,d1)–
  (chcols−1,d2), `bloken`, → `showall`.
- `insertline` (1153) : menu $51. `allocnull` (Z=1 → `outmem`, non-local), `allalt=-1`,
  `blokadj(cur)`, `numlines++`, link new item **before** a4 (possibly `firstitem`),
  `thisitem`=new, d7=cursy → `scrolldown`.
- `deleteline` (1173) : menu $61. Block adjustment (see G/H), then: no successor →
  `freelast`. Otherwise `numlines--`, `freethis` (= a4), unlink, `thisitem`=successor,
  d7=cursy, `scrollup`, → `showbotitem`.
- `freelast` (1208) : (only fall-through/bra from `deleteline`) delete last line: no
  predecessor → `freeonly`. Otherwise `numlines--`, `freethis`, `thisitem`=`enditem`=prev,
  `nextitem(prev)=0`; curstop≠0: `curstop--`, `scrollup`, `scrdownall`, → `showtopitem`;
  otherwise `scrollup`, → `goup`.
- `freeonly` (1226) : only line: `freethis`, → `allocfirst` (new empty item +
  `cls`). `numlines` stays 1.
- `join` (1229) : menu $81. No successor → `beepem`. a5=successor. d0 = length of the
  current screen line (`chmap+chloc`, without trailing spaces). Block adjustment. Builds in
  `charbuff2`: chmap line[0..d0) + successor (raw or `detok`), pad up to
  `charbuff3`; `charstobuff`, `makeline` (replaces a4). Unlink successor
  (`enditem` if needed), `FreeMem(a1, numchars)` (**wrong size**, see I), `numlines--`,
  → `showall`.
- `split` (1313) : Shift+Return (from `return`). d0=cursoff+cursx. Block adjustment.
  Copies chmap line from d0 to `charbuff2` and **overwrites it in `chmap` with
  spaces**; `charstobuff(a2=chmap line)`+`makeline` (current line shortened);
  `charstobuff(charbuff2)`, `alloc2` new item, copy, link **after** a4
  (`enditem` if needed), `numlines++`, → `showall`. Cursor stays.
- `return` (1402) : key 13. Shift → `split`. Otherwise `allocnull` (→ `outmem`),
  `allalt`, `blokadj(cur+1)`, `numlines++`, empty item **after** a4 (`enditem` if needed),
  `thisitem`=new, `cursx=0`; if `cursoff`≠0: `clr cursoff`, `refresh`. `showhprop`.
  If cursy+1+vscmarg ≥ edrows: `curstop++`, **temporarily** `edrows`=cursy+1,
  `pixrows`=edrows*fontmul (saved on stack), `scrupall`, reset, `rts`.
  Otherwise d7=cursy+1, `scrolldown`, → `movecdown`. No line splitting, no auto-indent.
- `top` (1453) / `bottom` (1456) : menu $02/$12, d0=0 or numlines−1 → `gotolined0`.
- `gadget` (1460) : event 255 (V-prop). edrows ≥ numlines → `rts`. Remembers line in
  **`bloktopx`** (scratch!), `cursor` (XOR). Loop: d0 = VertPot
  (`prop1sinfo+4`)·numlines>>16; changed → `cursor`, `gotolined02`, `cursor`,
  `showrow`; `getinput2` (polls, **busy loop**, consumes the message) until d7≠0;
  then `cursor`, last position → `gotolined02`.
- `gotolined0` (1490) : in: d0.w = target line (0-based, **must be < numlines**),
  a4 = current item. Push −1 (update H-prop), `cursx=0` (cursoff stays).
  → `gd03`.
- `gotolined02` (1495) : as above, push 0 (no H-prop). Falls into
- `gd03` (1496) : push old `curstop`; walk a4 from the current index to d0 (up
  or down, without NULL check), `thisitem`=a4. New curstop/cursy: target<vscmarg
  or numlines≤edrows → curstop=0, cursy=target; target ≥ numlines−edrows+vscmarg →
  curstop=numlines−edrows, cursy=target−curstop; otherwise cursy=vscmarg,
  curstop=target−vscmarg. `calcpos`; curstop changed → `showall`; flag≠0 →
  `showhprop` (tail). Pop both words. out: a4=`thisitem`=target item.
- `extmenu` (1542) : from `menu` for menu 4. `menurout`=0 → `rts`. Otherwise d6.w=d6>>4
  (item number), a5=`firstitem`, d7.w=`numlines`, `jsr (menurout)` → d0.w; `showme`;
  a4=`thisitem`; d0=−2 → `showall`; d0=−1 → `rts`; otherwise → `gotolined0`.
- `menu` (1565) : event 254, in: d6.w = Intuition menu code. d0=d6&15 (menu),
  d6=(d6>>1)&$f0 (item 0–15 in the high nibble). Menu 4 → `extmenu` (before the `or`). Otherwise
  key = item<<4|menu, linear in `menutable` (6 bytes per entry: key.b,
  flag.b, address.l; end $FF). Found: **flag byte on stack** (`move.b …,-(a7)`,
  2 bytes), `jsr (a0)` (computed jump), flag≠0 → `showvprop` (tail).
- `new` (1593) : menu $00. `erase` (cancel → back into `menu`), `doclean`, `freeall`,
  copies `deftitname` to `newsctit` (screen title), `showme`, → `donew`.
- `blokkill` (1607) : menu $11 (also Shift-131 via `blocksave`). No block → `beepem`.
  `blokop=0`, `allalt`, `bloksetup` (a5/d6 = block start). Prefix columns 0..btopx−1
  (`getbchar2`, padded with spaces) → `charbuff2`. Lines btopy..bboty−1 deleted via
  `freeupa5`. For a multi-line block `detoka5` (unconditionally, even without
  `tokenise`). Append suffix columns bbotx+1..chcols−1, pad. btopx=0 and
  bbotx=chcols−1 → last line also `freeupa5`; otherwise a4=a5, `makefirst`. `blokdis`;
  cursor line ≥ numlines → curstop=numlines−1, cursy=0, `calcitem`, `showall`,
  → `gotolined02`; otherwise `calcitem`, → `showall`.
- `blocksave` (1671) : key 131. Shift → `blokkill`, → `showvprop`. Otherwise falls through into
- `bloksave` (1677) : menu $21. No block → `beepem`. `getbname` (cancel: `addq #4,a7`
  → return to the caller of bloksave; d0=1 → `outmem`), d7=name, `doopen(1006)`
  (failure: `addq #4,a7` + `error`), d7=handle. `bloksetup`, `blokop=-1` (line end
  at text end instead of spaces). First line from btopx character by character (`Write` 1 byte each,
  buffer = **byte in `bloktopx`**), LF; intermediate lines via `detoka5` in one piece + LF
  (only here `Write` errors are checked → `error3`); last line character by character up to bbotx.
  → `doclose`. The `rts` (1736) is only reachable in the `cripple` version.
- `blokload` (1738) : menu $31. `blokop=0`, `getbname`, a4=`thisitem`, `doopen(1005)`,
  `fillbuff` (0 bytes → `doclose`). `mousealt`, `allalt`. **`bloktopx` as long word** =
  line end in chmap. Prefix chmap[0..cursor) (bit 7 cleared) → `charbuff`
  (a3), pointer to cursor position on stack. First line: `readline`; without EOL →
  `done2`; otherwise `bloadadd` + `makeline2` (replaces a4). Further lines via
  `allocline`+`linka0`. End: see `done*`. Suffix (rest of the chmap line, bit 7
  cleared, limited by `charbuff2`) to `charbuff` → `blalldone`.
- `done3` (1778) : EOF after LF: empty line (`allocnull`/`linka0`), a3=`charbuff`, → `done`.
- `done2` (1783) : first line without EOL: `bloadadd`, → `done`.
- `done4` (1785) : last line without EOL: `allocline`+`linka0` (immediately replaced by
  `makeline` in `blalldone` – redundant), a3=`charbuff`, `bloadadd`, falls into
- `done` (1791) : `doclose`, pop cursor pointer, copy suffix; falls into
- `blalldone` (1801) : `charstobuff(a2=charbuff → charbuff, in-place)`, `makeline`,
  `calcitem`, `blokdis`, → `showall`.
  (`done`, `done2..4`, `blalldone` are **global** labels only for `blokload`.)
- `blokcopy2` (1808) : key 132: `bsr blokcopy`, → `showvprop`.
- `blokcopy` (1811) : menu $01. No block → `beepem`. Cursor in the block (between
  start and end, `inbloks`/`inblokf`) → `beepem`. Continues in
- `bcopyok` (1832) : `bloksetup`; prefix chmap[0..cursor) → `charbuff2` (d1=length),
  **push d1/a1** (a1 = chmap pointer at cursor). Append block characters via `getbchar`
  (discarded beyond chcols). Line change: pad, `charstobuff`, `alloc2`,
  link new item **before a4** (`firstitem` if needed), `numlines++`; intermediate lines
  copied raw (tokenised, without detok) and likewise before a4. Last line from
  column 0 anew in `charbuff2`. Block end (mi): **pop d1/a1** (d1 = old prefix length!),
  append suffix chcols−d1 characters from chmap, `makefirst` (replaces a4). Shift block
  position if copy before/on block (see H). `calcitem`, → `showall`.
- `blokforget` (1976) : menu $41. No block → `beepem`; `blokdis`, → `showall`.

Tail calls into foreign routines: `freeall`, `gotolined0/02`, `godown`, `goup`,
`showbotitem`, `showtopitem`, `scrolldown`, `allocfirst`, `showall`, `showhprop`,
`showvprop`, `donew`, `doclose`, `outmem` (via `beq`), DisplayBeep (jmp LVO).

---------------------------------------------------------------------------------------------
## B. Register conventions

- **a4 = current item** on entry (from `thisitem`, ted.s:145). Routines mostly set
  only `thisitem`, not a4 (e.g. `cursdownok`, `insertline`) – `mainedit` reloads
  a4 every round. Exception: `gotolined0/02` run with a4 and leave
  a4=target; `makeline` sets a4=new item.
- **d7 on entry** = event code (jumptable key), **d6** = gadget/menu code.
  Inside: d7 = start line for `scrollup`/`scrolldown`; in `bloksave`/`blokload`
  d7 = DOS file handle (name before), d6 = characters in `dosbuff`, a5 = read pointer.
- Block cursor (`bloksetup`): a5 = item, d6 = line, d5 = column, `getbchar` returns
  d0, **N=1 block end, Z=1 line end** (line/a5 then already advanced).
- a6: after `doopen`/`calldos` = DOSBase (bloksave then uses `write(a6)` directly);
  `move.l 4,a6` before every FreeMem/`alloc2`.
- No `movem` saving in this scope; `extmenu` saves nothing before the callback
  (blitz2 `menu` must do this itself; after the callback only a4 is reloaded from
  `thisitem`).
- Stack convention dispatch: `mainedit` pushes flag byte (2) and sets `memstack`=a7
  before `jsr`; `menu` pushes another flag byte (2). Depending on this: `escape`
  (`lea 12`), `erase`/`getbname`/`doopen` (`addq #4` = return one level higher),
  `outmem` (a7=`memstack`, → `memcont`).

---------------------------------------------------------------------------------------------
## C. Globals defined in this scope

| Name | Type | Init | Purpose |
|---|---|---|---|
| `deftitname` (1604) | char[26] + even | `'Blitz Basic 2 Editor V1.0',0` | title at `new` |

Otherwise only code. Foreign globals used (for info): `bloktopx` (6085, word) is misused
here as scratch (see G); `menutable` (6199) is a code pointer table
(30 entries of 6 bytes, $FF end) – interpreted here.

`menutable` assignment (key item<<4|menu, flag=showvprop afterwards):
Menu 0: $00 new(0) $10 fileopen(1) $20 filesave(0) $30 defaults(0) $40 doabout(0)
$50 prtfile(0) $60 mycli(0) $70 closewb(0) $80 openwb(0) $90 escape(0).
Menu 1: $01 blokcopy(1) $11 blokkill(1) $21 bloksave(0) $31 blokload(0) $41
blokforget(0) $51 insertline(1) $61 deleteline(1) $81 join(1) $91 blocktab(1)
$a1 blockuntab(1) $b1 markindent(0).
Menu 2: $02 top(1) $12 bottom(1) $22 gotoline(1).
Menu 3 (SEARCH): $03 find $13 next $23 previous $33 replace $43 replaceall (all 1).
Menu 4: external (`extmenu`).

`jumptable` (6171, same format, end 0): 28 cursup(1), 29 cursdown(1),
13 return(1), 255 gadget(1), 254 menu(0), 253 mousedown(1), 139 help(0),
129 blockstart(0), 130 blockend(0), 131 blocksave(0), 132 blokcopy2(0).

---------------------------------------------------------------------------------------------
## D. Self-modifying code / data in the code

- No patching of instructions in this scope.
- `new` overwrites `newsctit` (6312, in the code segment, PC-relative via `lea`): title
  becomes `deftitname` (shorter, fits).
- `bloksave`: `move.l #bloktopx,d2` – absolute (relocated) address of a code variable
  as Write buffer.
- `escape` frees the external menu built at runtime by `getstuff` (2786) via AllocMem
  (copy of `menu2` data).
- No entries into `incbin` blocks here (indirectly: `getbname` → `ezfreq` →
  `filerequest+36`).

---------------------------------------------------------------------------------------------
## E. Heap/list structures

**Line item** (rsreset ted.s:15): `+0 nextitem.l`, `+4 lastitem.l`, `+8 numchars.b`
(length **incl.** NUL), `+9 chars[]` (tokenised if `tokenise`, token = 2 bytes with
bit 7 in the first). Allocation `numchars+lenitem` (lenitem=9), MEMF_PUBLIC (d1=1).
Globals: `firstitem`, `enditem`, `thisitem`, `numlines` (word). Doubly linked,
`lastitem(first)=0`, `nextitem(end)=0`. Line numbers are stored nowhere,
but computed as curstop+cursy or by walking the list.

- `allocnull`: 10 bytes, `move #$100,numchars` → numchars=1, chars[0]=0.
- Creating/linking here: `insertline` (before a4), `return` (after a4), `split` (after
  a4), `blokcopy` (before a4), `blokload` (`linka0`: after a4, a4 advances). Replacing:
  `makeline` (join, split, blokkill, blokload, blokcopy). Deleting: `freethis`
  (deleteline/freelast/freeonly), `freeupa5` (blokkill), FreeMem directly (join).

**External menu** (from `getstuff`, freed in `escape`): `menu2` (6554) is the
Menu structure "SEARCH"; its `NextMenu` (+0) points to a copy of the menu header
(30 bytes = `menu2name-menu2`, Menu struct) with `FirstItem` (+18) → chain of
54-byte blocks (= `m2i1text-menu2item1`: MenuItem 34 + IntuiText 20, linked via
+0 NextItem). `escape` frees only this one menu, does not reset `menu2.NextMenu`
and calls no `ClearMenuStrip`.

**Character buffers** (allocation ted.s:41: doslen(2048)+3·chcols+6): `charbuff`,
`charbuff2`, `charbuff3` each chcols+2 bytes; `charbuff4` points to the end (unused).
`charbuff3[0]` = length+1 after `detoka5`, text from `charbuff3+1`. In `join`/`split`/
`blokkill`/`blokcopy` `charbuff2` serves as line building buffer, padding "up to
`charbuff3`" = chcols+2 characters.

**chmap**: screen character map, line = `chmap+chloc`, chcols bytes, bit 7 =
display flag (cleared on takeover with `bclr #7`/`and #127`). For the
current line chmap is the **authoritative copy** (item possibly outdated, see
`anyalt` in `mainedit`): `join`, `split`, `blokload`, `blokcopy` read there; `split`
writes there.

**Mice list**: nodes 8 bytes (`next.l`, `item.l`), `freemice` in `escape`. Rebuilt after
every command if `mousealt` (`showmice`), `remember`/`mforget2` only set
`mousealt`.

---------------------------------------------------------------------------------------------
## F. External calls

- exec: `FreeMem` (escape: menu blocks; join: item), `AllocMem` via `alloc2`/`allocnull`.
- dos: `Write` (bloksave, directly `jsr write(a6)`), indirectly `Open`/`Read`/`Close`
  (`doopen`/`fillbuff`/`doclose`).
- intuition: `DisplayBeep` (beepem), indirectly `ShowTitle` (`showme`).
- **Callback into blitz2**: `extmenu` → `menurout` (edstruct+16, blitz2.s:1498):
  in d6.w = menu item, a5 = `firstitem`, d7.w = `numlines`; out d0.w: −1 nothing,
  −2 `showall`, otherwise line number → `gotolined0`. Upper words of d6/d7 undefined
  (`lsr.w`/`move.w`). The callback can call `extalloc` (comdata+88); failure →
  `outmem` sets a7=`memstack` and discards the blitz2 stack frames.
- `doclean` → `cleanup` (edstruct+20, normally 0) in `escape`/`new`.
- V-prop: reads `prop1sinfo+4` (PropInfo.VertPot) directly while Intuition changes it.
- Input: `getinput2` (GetMsg without Wait) in the `gadget` loop; event codes 254/255.
- `qualifier&3` = Shift left/right (rawkey qualifier of the last key).
- Blitter indirectly via `scrollup`/`scrolldown`/`scrupall`/`scrdownall` (`getblitter`).

---------------------------------------------------------------------------------------------
## G. What needs special handling in C

- **Dispatch stack**: `escape` (`lea 12(a7)`), `erase`/`getbname`/`doopen`
  (`addq #4,a7`), `outmem` (a7=`memstack`). In C: handlers return status
  (`ED_ABORT`, `ED_QUIT`) or `setjmp/longjmp` for `outmem` (point in `mainedit`
  before the handler call). `escape` → return "quit editor" to `mainedit`.
- **`menurout` call**: register callback into blitz2 (d6/a5/d7 → d0) → asm stub in
  `glue*.c`; must set all registers defined from there and expect nothing.
- `menutable`/`jumptable`: in C struct array `{uint8 key, flag; void (*fn)(void);}`;
  flag byte as local variable instead of stack.
- `gotolined0`/`gotolined02`: two entries with pushed flag → C function with
  parameter `int update_hprop`; `gd03` internal.
- Fall-throughs: `cursdown`→`cursdownok`→`movecdown`, `blocksave`→`bloksave`,
  `done4`→`done`→`blalldone`, `blokcopy`→`bcopyok`, `deleteline`→`freelast`→`freeonly`
  (only via `beq`). Multiple entries `movecdown` (return), `beepem` (global).
- **`bloktopx` aliasing**: word variable, in `gadget` as word scratch, in `bloksave`
  as 1-byte write buffer (high byte!), in `blokload` as **long word** (overwrites
  `bloktopx`+`bloktopy`). Collides with a running mouse block marking (2023ff.). In C
  own local variables; possibly destroy `bloktopx/bloktopy` deliberately after blokload
  (behavioural equivalence) or BUG(orig) marker.
- Byte push on a7 (`move.b …,-(a7)`) occupies 2 bytes – for the stack depths in `escape`
  this is relevant.
- CCs across levels: `getbchar` (N/Z), `allocnull` (Z=1 failure), `erase2` (Z=1 OK),
  `inbloks`/`inblokf` (Z=1 via `moveq #0,d3`, otherwise Z from `cmp`/`bhi`),
  `readline` (N=1 nothing, Z=1 without EOL), `fillbuff` (Z=1 0 bytes).
- Signedness: comparisons almost all **unsigned** (`bcs/bcc/bhi/bls`) on words;
  `bboty=-1` = "no block" is checked with `cmp #-1`. `markindent`/`join` use
  signed `blt`/`bmi`.
- `numchars` is a byte; `join` adds via `add.b` (overflow at 255 possible).
- Reading through NULL: `bloksave` (last block line = last text line, bbotx behind
  text end → `detoka5` on a5=0), `gd03` (target ≥ numlines). Silently
  "working" on the Amiga, catch in C/host.

---------------------------------------------------------------------------------------------
## H. Classification

| Routine | Class | Remark / cut for host core |
|---|---|---|
| escape | UI/GLUE | stack trick; menu freeing + `freeall` (KERN part: `freeall`) |
| cursdown/cursup | UI | core part only `thisitem`=next/prev and page arithmetic |
| cursdownok/movecdown | UI | scroll decision (curstop/vscmarg) purely arithmetic |
| beepem | UI | hook `ui_beep()` |
| markindent | KERN | list only; afterwards `bloken`+`showall` → hook `ui_block_changed`, `ui_redraw_all` |
| insertline | KERN+UI | core: alloc, `blokadj`, numlines, link before a4, thisitem. Afterwards `scrolldown(cursy)` → hook `ui_line_inserted(row)` |
| deleteline | KERN+UI | core: block adjustment, unlink/free, thisitem. UI per case: middle `scrollup`+`showbotitem`; last line `scrollup`+(`scrdownall`+`showtopitem` or `goup`); only line `allocfirst` (contains `cls`!). Core returns the case |
| freelast/freeonly | KERN+UI | as above; `allocfirst` = `afirst2` (KERN) + `cls` (UI) |
| join | KERN+UI | input = chmap line (host buffer). Core up to `numlines--`; afterwards `showall` |
| split | KERN+UI | like join; writes chmap (must stay in the core, since chmap is the current line) |
| return | KERN+UI | core: alloc, `blokadj(cur+1)`, link, thisitem, cursx=0. UI: `refresh`, `showhprop`, scroll with temporary edrows/pixrows, `scrolldown`. `curstop++` is state, belongs to the core |
| top/bottom | KERN | → gotolined0 |
| gadget | UI | busy loop with event polling |
| gotolined0/02/gd03 | KERN+UI | core: item walk + curstop/cursy computation (pure). UI: `calcpos`, `showall` on curstop change, `showhprop` |
| extmenu | GLUE | register callback; afterwards UI (`showme`, `showall`) |
| menu | KERN | table search pure; call + `showvprop` UI |
| new | KERN+UI | `erase` (requester), `freeall`/`donew` (donew mixes `blokdis`, `allocfirst`→`cls`, `showhprop/showvprop`) |
| blokkill | KERN+UI | core up to and incl. `blokdis` state (bboty=−1); `blokdis` draws status line + menu flags → hook. End `showall`/`gotolined02` |
| blocksave | UI | qualifier switch |
| bloksave | KERN | file writing (can be abstracted: `Write`); `getbname` (requester), `error` (requester) → callbacks |
| blokload, done*, blalldone | KERN | read file; input chmap line/cursor column; requester via callback; end `blokdis`+`showall` |
| blokcopy/blokcopy2/bcopyok | KERN+UI | core: copy + block shift; `showall`/`showvprop` afterwards |
| blokforget | KERN+UI | `blokdis` + `showall` |

In general: the core operations can be separated if (1) chmap/chloc/chcols/
cursx/cursoff/curstop/cursy lie in the core as editor state (no OS),
(2) `blokdis`/`bloken` are split into state change (bboty=−1/bstx=−1, menu flag bits) and
display (`blokprnt2`), (3) redraw calls at the end remain as hooks
(`ui_scroll_up(row)`, `ui_scroll_down(row)`, `ui_scr_up_all()`,
`ui_scr_down_all()`, `ui_show_top_item()`, `ui_show_bot_item()`,
`ui_redraw_all()`, `ui_refresh()`, `ui_show_hprop/vprop()`, `ui_beep()`) –
in all routines of this scope drawing happens **after** the list change,
only interleaving: `return` (scroll with temporarily changed edrows/pixrows after
`curstop++`), `freeonly` (`cls` in `allocfirst`), `gadget` (loop).
`goup`/`godown`/`calcpos` only change variables (cursy, scloc, chloc) – leave them in the
core (scloc is a bitmap offset, harmless as a number).

**Block coordinate logic** (pure, easily testable; btopx/btopy/bbotx/bboty, bboty=−1 no
block):
- `blokadj(d0)`: d0 ≤ bboty → bboty++; d0 ≤ btopy → btopy++ (insertline: cur,
  return: cur+1).
- deleteline: cur < btopy → both −1; btopy ≤ cur ≤ bboty → bboty−1, if then
  bboty < btopy: bboty=0, then both −1 (→ bboty=−1, btopy−1); cur > bboty → nothing.
- join (cur < bboty, d1=cur+1, d0=length of current line): d1<btopy → both −1;
  d1=btopy=bboty → btopx+=d0, bbotx=btopx+width, `blokfix`, both −1;
  d1=btopy<bboty → btopx+=d0, `blokfix`, both −1; d1=bboty>btopy → bbotx+=d0,
  `blokfix`, bboty−1; otherwise bboty−1.
- split (d0=column): cur>bboty nothing; cur<btopy both +1; btopy<cur<bboty bboty+1;
  cur=btopy<bboty: d0>btopx → bboty+1, otherwise btopx−=d0, both +1;
  cur=bboty>btopy: d0>bbotx nothing, otherwise bbotx−=d0, bboty+1;
  cur=btopy=bboty: d0>bbotx nothing; d0>btopx → bbotx−=d0, bboty+1; otherwise both x −d0,
  `blokfix`, both +1.
- blokcopy (only if copy before/on block end): d1=bboty−btopy; bbotx=chcols−1 →
  d1++, and if cur≥btopy btopx−=d0; otherwise cur<btopy nothing further; cur=bboty (=btopy)
  → x both +(bbotx−btopx+1), `blokfix`; cur=btopy<bboty → btopx+=bbotx−d0+1,
  `blokfix`; then btopy+=d1, bboty+=d1.

---------------------------------------------------------------------------------------------
## I. Peculiarities / possible bugs of the original

1. **`join` FreeMem with wrong size** (1305–1309): `moveq #0,d0; add.b numchars(a1),d0`
   – `+lenitem` (9) is missing. Assembled like this in the binary (1FE0: `7000 d0290008`). Per
   join 9 bytes (rounded) remain as a leak. Comment "was move.b" hints at a
   later fix. BUG(orig).
2. `join` frees the appended line without `mforget2` → `mousealt` not set;
   if the line had a label, a mice node with a dangling pointer remains until the
   next rebuild.
3. `join`/`split`/`blokcopy`/`blokload`: no length check. Result lines > chcols
   are silently truncated to chcols by `charstobuff`; `join` (prefix + `detok`
   of the following line) and `blokload` (`bloadadd`) can write beyond `charbuff2` into
   `charbuff3`, `join` with expanding tokens even up to the end of the allocation.
4. `blokcopy` `.oneline`: `movem.l (a7)+,d1/a1` restores the **prefix length**,
   not the current length in `charbuff2` → always chcols−prefix
   suffix characters are appended; the line is too long by the fragment length and is
   truncated (mostly only trailing spaces, with long lines real text loss).
5. After `blokfix` → `blokdis` (bboty=−1), `join` (−1), `split`/`blokcopy` (+d1) keep changing
   bboty → bboty=−2/0…: "ghost block" without activated menu items.
6. `deleteline` on a single-line block sets bboty to −1 via `clr`+`sub`, without
   `blokdis` (menu items/status text "block" remain, `bstx` unchanged).
7. `escape`: external menu is freed while the menu strip still hangs on the window
   (no `ClearMenuStrip`), `menu2.NextMenu` not zeroed. If `menitems` has a
   title but no items, the copy still contains `FirstItem=menu2item1` (static!)
   → FreeMem on program data (getstuff 2808ff.).
8. `blokkill` calls `detoka5` even without `tokenise`; `bloksave` `.bok` likewise. With
   bytes ≥$80 in non-token mode `findtoke` runs with `tokens`=0 from address 0.
9. `bloksave`: only the `Write` of the whole intermediate lines is checked; single-byte writes
   are not. Writes byte-wise unbuffered (slow). Possibly reads past the list end
   (see G).
10. `blokload`: prefix/suffix from chmap are plain text, loaded line tokenised;
    afterwards `charstobuff` in-place over `charbuff` (source=destination) – with `tokenise`
    mixed content is sent through `tokit` again (check whether `tokit`
    passes token bytes through). `done4` allocates the last line, which `blalldone`
    replaces immediately (superfluous alloc/free). `bloktopx/bloktopy` are destroyed.
11. `gadget`: busy polling without `WaitPort`; the terminating message (e.g. key)
    is swallowed.
12. `gotolined0` sets `cursx=0`, but not `cursoff` (commented out 1492) – with a
    horizontally scrolled view the cursor is at column cursoff.
13. `gd03`: no range check; d0 ≥ numlines (or negative, e.g. return value <−2
    from `menurout`) runs through NULL.
14. `menu`: only menu 0–15/item 0–15 distinguishable (bit 4 of the menu number, bits 9–10
    of the item number ignored); sub-items ignored.
15. `new` sets the title to "Blitz Basic 2 Editor V1.0", although the start title
    is "Blitz Basic 2 - Ted Version 1.1" (cosmetic).
16. `split`/`blokcopy` create new lines via `alloc2` without `remember` check
    (if the new line begins with a label, `mousealt` is not set).
17. `freelast` with curstop≠0: `scrollup` and immediately `scrdownall` (double
    blitter operation; cosmetic only).
18. `join` `.nextbot`: `cmp btopy(pc),d1; beq .nextboth` is dead code (d1≠btopy
    guaranteed there).
