# Survey 01 – ted.s lines 1–1015 (+ wbheader, aboutreq, specialreq, incbins)

Scope: equates/rs offsets, init entry `outside` (blitz2 and standalone), `edstart`
(= wbheader), fail chain, `mainedit` (main loop, jump table), mouse pointer/sleep pointer
with VBlank server, menus on/off, `blocktab`/`blockuntab`, `defaults`, `prtfile`,
`.xtra`/`.info`, `fileopen`/`filesave`/`realsave`, `doabout`, find/replace.

Addresses (hunk offset, from `ted.labels`): `outside` $0004, `edstart`=`wb_header` $0204,
`mainedit` $0428, `mouseinit` $04AA, `pointeron` $0504, `sleepon` $0534, `myroutine` $05FC,
`myint` $0632, `sleep` $0656, `pointer` $0916, `dumsleep` $0956, `finish` $09AE,
`fileopen` $0E72, `fileopen2` $0E7A, `filesave` $0F7A, `realsave` $0FB4, `srcinfo` $1074,
`aboutreq` $1200, `specialreq` $150C, `doabout` $194A, `find` $198E, `replaceall` $1A48,
`next` $1AA0, `dosearch` $1AAC, `replace2` $1B2C, `comdata` $5DC2, `rawed` $73E0, `flink` $8CBE.

Program = ONE code hunk; data (variables, Intuition structures, sprite data, icon)
sit in the middle of the code and are written to → in C everything is writable data.

---------------------------------------------------------------------------------------
## Equates / rs offsets (ted.s:2–20)

| Name | Value | Meaning |
|---|---|---|
| `ngerms` | 6 | 2 uses outside the scope |
| `cripple` | 0 | `ifeq cripple` around `filesave` (746) and at 1678: 0 = full version |
| `doslen` | 2048 | size of the DOS read buffer `dosbuff` |
| `nextitem` | 0 (.l) | next line |
| `lastitem` | 4 (.l) | previous line |
| `numchars` | 8 (.b) | length incl. terminating 0 (see `realsave`: writes `numchars` bytes, 0 or LF at `numchars-1`) |
| `chars` | 9 | characters (tokenised or ASCII), 0-terminated |
| `lenitem` | 9 | header size; allocation = `lenitem + d5` (d5 = length incl. 0), MEMF_PUBLIC |
| `sleepwait` | 1 (279) | animation divider of the sleep pointer (frame every 2 VBlanks) |

`numchars` is a byte → line max. 255 bytes; allocation size is not stored, it is reconstructed from
`numchars` (`freeall`, outside: `moveq #lenitem,d0 / add.b numchars` → byte
overflow from numchars ≥ 247, see I).

---------------------------------------------------------------------------------------
## A. Routines

### Start / init / end

- Line 22 `bsr edstart` : first instruction of the hunk (offset 0). Standalone only (DOS jumps to
  offset 0). blitz2 jumps directly to `outside` = hunk+4 (`jsr 8(BADDR(segadd))`).
- `edstart` (136) = `include wbheader` → `wb_header` (wb:4). In (standalone): CLI d0/a0 =
  arguments; WB: none. Sets `us`, `wbmess`, `dos` (via `dosopen`), `progfile` (file from
  CLI arg or WB arg[1]; previously misused as buffer for `<tool>.info`), d2 = stack (unused
  in TED). Ends in `wb_allgo` (wb:112) → falls through into line 137 `sub.l a1,a1 / rts` →
  back to `outside` with **a1 = 0** (= "not external").
  Stack: because of `bsr edstart`, `fromcli` reads with `4(a7)` the DOS return address instead of the
  stack size (d2 irrelevant). `dosopen` failure → `bra wb_exit` → its `rts` returns to line 24
  (`outside`) instead of ending the program (see I).
  - `fromcli` (wb:73): copies the argument up to ' ' or LF to `progfile` (no quote
    handling here; `a0toname` removes them later).
  - `wb_exit` (wb:88): ReplyMsg(wbmess) if ≠0 (Forbid/Permit), `rts`. Out: d0 = wbmess,
    **Z=1 if wbmess=0** (read as success by the external caller!).
  - `dosopen` (wb:97): OldOpenLibrary("dos.library") → `dos`; out d0, Z=1 on failure; a6=Exec.
- `outside` (24) : **init entry**, several entries (standalone by fall-through after
  `edstart`, external via `jsr` from blitz2). In: a1 = `&edstruct` (blitz2) or 0.
  `returncode`=-1; `external`=a1; if a1≠0: copies 14 words (28 bytes) to `tokens`…
  `remark` (ted.s:6269ff.: tokens, menitems, loadxtra, savextra, menurout, cleanup, colon,
  quote, label, remark) and `dosopen` (failure → `wb_exit`). Then `opengr` (→`fail2`),
  `openint` (→`fail3`; IntuitionBase also to `extint` = comdata+44), `bsr rawed+32`
  (console init in the incbin), AllocMem(doslen+3*chcols+6, MEMF_PUBLIC) → `dosbuff` (→`fail5`),
  distributes `charbuff`=dosbuff+doslen, `charbuff2/3/4` each +chcols+2. `loaddefs`, `clr defalt`,
  `screenopen` (N=1 → `openfailed`), `donew` (Z=1 → `allocfailed`), `printmem`, `mouseinit`,
  `pointeron`, `jsr flink` (relocates filereq). External: `getstuff` (menus/tokenise flag from
  edstruct), **out: a0 = `.tomain`, a1 = `comdata`, comdata+84 := `&defaultreqga9+13`,
  d0=0 → Z=1**, `rts` to blitz2 (screen stays open). Standalone: `.edmain`: a0=`progfile`,
  falls through into `.tomain`.
- `outside.tomain` (87) : **second entry from blitz2** (`jsr (a1)` with a0 = file name,
  blitz2's `progfile`). `a0toname` (→ `filepath`/`filename`); if `filename`≠"": builds
  `filepath` + ('/' if last character ≠ ':') + `filename` in `charbuff`, d7 = charbuff,
  `fileopen2`, `showvprop`, `showmice`, `printmem`. `.domain`: `bsr mainedit` (returns at
  program end), `mousefinit`, `clr returncode`, if `defalt`≠0 `savedefs`, falls through
  into the fail chain. End via `wb_exit` → `rts` = program end (standalone) or return
  to blitz2.
  Trick: empty `filepath` → `-2(a0)` reads the byte BEFORE `filepath` = ':' from `dc ' :'`
  (ted.s:6139) → no '/' inserted. Handle explicitly in C.
- Fail chain (fall-through, every label is also a `beq` target): `allocfailed` (116) `screenclose`
  → `openfailed` (118) FreeMem(dosbuff, same size) → `fail5` (128) `bsr rawed+32+8`
  (console cleanup) + `closeint` → `fail3` (130) `closegr` → `fail2` (131) `closedos` →
  `fail1` (132) d0 = ext.l `returncode` → `bra wb_exit` (d0 overwritten there, see I).
- `mainedit` (142) : main loop, returns only via `escape` (1016, outside). Per
  iteration: `clr mousealt/memalt`, `getline` (out: d7 = key/menu code, `anyalt`), a4 =
  `thisitem`; if `anyalt`: `allalt`=anyalt, `charstobuff`, `makeline` (new line replaces
  a4), with `tokenise` `showthisitem`. Then **computed jump** via `jumptable` (6171):
  entries 6 bytes `{dc.b code, dc.b vpropflag, dc.l routine}`, end = code byte 0;
  comparison `cmp.b d0,d7`; not found → `mainedit`. Found: flag byte via
  `move.b (a0)+,-(a7)` onto the stack (2 bytes!), `memstack` := a7, `jsr (a0)` with a4 =
  thisitem, d7 = code. Afterwards `memcont`.
- `memcont` (167) : also entered from `outmem` (3046: a7 := memstack, `bra memcont` =
  longjmp after memory error). If `memalt` `printmem`, if `mousealt` `showmice`, flag
  from the stack; ≠0 → `showvprop`. `bra mainedit`. `rts` in 179 unreachable.
- `funcs` (182) : empty label.

### Mouse pointer, sleep pointer, menus

- `mouseinit` (184) : AllocMem(finish-pointer = 152, MEMF_CHIP) → `pointerat` (no
  error check!), `dumat` = pointerat+64; copies `pointer`+`dumsleep` (38 longs) into chip RAM;
  `sleepcurr`=0.
- `mousefinit` (200) : ClearPointer(mywindow), tail `jmp` FreeMem(pointerat,152). Does not remove
  the interrupt server (assumes that `pointeron` ran before).
- `pointeron` (209) : **saves a0-a1/d0-d3/a6** (comdata+72, called by blitz2). If
  `sleepcurr`≠0: RemIntServer(INTB_VERTB=5, `myint`), `sleepcurr`=0. SetPointer(pointerat,
  height 14) via `setpoint` (→ also menus anew, see below).
- `sleepon` (223) : **saves a0-a1/d0-d3/a6** (comdata+76). Already active → `bra makemenus`
  (after movem, i.e. a6 is preserved, makemenus changes only a0/d0 – so a0/d0 are NOT
  preserved on this path!). Otherwise: `makedum(sleep)`, `sleepcurr`=sleep,
  `sleepdelay`=1, `setpoint`(dumat, height 20), AddIntServer(5, myint).
- `setpoint` (242) : in: a1 = sprite data (chip), d0 = height. SetPointer on `req_window`
  (if ≠0) and `mywindow`, width 16, XOffset −1, YOffset 0. a6 := `int`. Falls through into
  `makemenus`.
- `makemenus` (258) : `req_window`≠0 or `sleepcurr`≠0 → `menusoff`, otherwise `menuson`.
  Also called from 3668.
- `menuson` (263) / `menusoff` (271) : `bclr`/`bset #0,25(window)` on mywindow and possibly
  req_window = **Window.Flags bit 16 = WFLG_RMBTRAP** patched directly (right mouse button
  opens no menus). No OS call.
- `makedum` (281) : in a0 = sleep frame. Copies 20 longs from frame+4 to dumat+4 (control
  long at the front and end long at the back stay 0 from `dumsleep`).
- `myroutine` (291) : **VBlank interrupt code** (`is_Code` of `myint`). `sleepdelay`−1; <0 →
  =1, `sleepcurr`+=88, ≥ `sleepf` → `sleep`; `makedum`. Out: **d0=0, Z=1** (moveq) – server
  chain continues. Uses d0/a0/a1 (allowed). Absolute addresses (`subq #1,sleepdelay`).

### Block operations

- `blocktab` (327) : in a4/d7 from mainedit. `bboty`=−1 (no block) → `beepem`.
  `bloksetup2` (out: a5 = first block line, d6 = its number). Per line: `charbuff2` =
  `tabstop` spaces + line (detok if `tokenise`, otherwise copy up to 0), rest up to
  `charbuff3` with spaces (no 0 terminator → "screen line"), `thisitem`=a4,
  `makefirst` (charstobuff(a2=charbuff2) + makeline; a4 := new line), continue until
  d6 > `bboty`. `calcitem`, tail `bra showall`.
- `blockuntab` (361) : analogous; after copying, the first `tabstop` characters of
  **arbitrary content** are removed (`tst.b (a0)+` only checks for 0). Tail `bra showall`.

### Options

- `defaults` (401) : options requester (`defaultreq`). Saves `defstart..defend` (50 bytes,
  word by word) to `charbuff` (backup for cancel; offsets there: 0 fonthite, 6 tokenise,
  8 makeicons, 10 normcol …). Tokenise gadget `defaultreqga8` disabled (Flags+12 |= $100)
  if `tokens`=0. Reopen screen with `not copok` around it, `openreq`, `swapbords`.
  Loop `getinput` (d7=−1, d6=gadget ID):
  - d6 0…3: colour sliders (prop): a5 = `palit` + `propcol`*6; loop `propadj` +
    `getinput2` until d7≠0 (live SetRGB4).
  - d6 10…25: colour selection, 4 groups of 4 (d6−10: group = ((d6−10)>>1)&6 → word in
    `normcol/labcol/tokcol/propcol`), toggle border (`dispgadd0`, `swapd0bord`);
    group propcol (d6−10 ≥ 12) → `setprops`, `dispprops`.
  - d6 4…9: integer/check gadgets: checks `int1`<32, `int2`<10, `int3`<80 (unsigned,
    otherwise 0 + buffer `defaultreqla47..49` := "0" ($3000) + beep + clear SELECTED in ga4..ga6
    + RefreshGList(ga4, req_window, defaultreq, 3)). Applies immediately: font
    (`defaultreqga7+13` bit 7 → fonthite 6/fonthite2 5, otherwise 8/7; `fontmul` =
    fonthite*bmapmul), `tokenise` ← ga8 SELECTED, `makeicons` ← ga9 SELECTED, `hscmarg`,
    `vscmarg`, `tabstop` ← low word of int1..3.
  - d6 26 (OK): `color`=normcol, `defalt`=26 (→ savedefs at the end), `swapbords`; if
    font height changed: limit `cursy` to < `edrows` (walks `lastitem` back),
    `calcpos`; `screenclose`/`screenopen2`; if tokenise changed tail `detokall2` or
    `tokall2`, otherwise `rts`.
  - d6 ≥ 27 (cancel): `setdefs`(a5=charbuff backup), `colors`, `swapbords`,
    `screenclose`, tail `bra screenopen2`.

### Printing

- `prtfile` (570) : requester `prtreq`, `actgad(prtreqga1)` (string gadget with `prtname`
  "PRT:"). Gadget 2 = print, 3 = cancel. Open(prtname, 1006); per line (`numlines`, from
  `firstitem`): copy to `charbuff`, bytes with bit 7 = token (2 bytes) → `ESC[1m` +
  name (`findtoke`) + `ESC[22m`; line end 0 → LF; Write(d7, charbuff, len); Close. a6 =
  DOS over the whole loop (findtoke does not change a6). Errors on Write ignored.

### .xtra / .info

- `openmore` (664) : in a1 = extension. Appends it to the string at `xtraat`, remembers the
  old end in `xtranull`; out: d1 = xtraat (name for Open/Lock). a6 unchanged.
- `openxtra` (658) : in d2 = mode, **a6 = DOSBase (from the caller, from `doclose`)**.
  Open(name+".xtra", d2); out d7 = handle, **Z=1 if failed**.
- `openinfo` (641) : Lock(name+".info", ACCESS_READ=−2); exists → UnLock, d7=0 (Z=1);
  otherwise Open(…,1006) → d7, Z=1 on failure. So the icon is never overwritten.
- `closextra` (674) : `clr.b (xtranull)` – removes the extension again.
- `xtraat`, `xtranull`, `xtratxt`, `infotxt`: data (636–639).

### Loading / saving

- `fileopen` (678) : `erase` (confirmation; on cancel `addq #4,a7` → back to
  mainedit), `getfname` (file requester; cancel → `addq #4,a7`, likewise aborts the
  caller; d0=1 → `outmem`), d7 = name. Falls through into `fileopen2`.
- `fileopen2` (680) : in d7 = file name (path). `xtraat`=d7; `doopen` (failure: `addq #4,a7`
  + `bra error` → returns to the caller of fileopen2); `clr allalt`; `doclean`
  (callback `cleanup` from edstruct, saves d1-d7/a0-a6); `fillbuff` – 0 bytes → `doclose`,
  tail `bra donew`. Otherwise `sleepon`, `blokdis`, `freeall`, `numlines`=1, first line
  `readline`+`allocline` → `firstitem`=`thisitem`=`enditem`, `lastitem`=0; loop
  `readline` (N=1 = end) / `allocline` / link / `numlines`++. `doclose`. Callback
  `loadxtra` (if ≠0): a5 = routine, d2=1005, `openxtra`; success → **`jsr (a5)` with
  d7 = handle, a6 = DOSBase**; afterwards Close(d7) via **a6 after the callback** (callback must
  preserve d7/a6). `closextra`. Cursor/scroll to 0, `showhprop`, `showall`, `pointeron`,
  falls through into `showfname`.
- `showfname` (731) : writes "File: " + `filename` into `newsctit` (112-byte buffer, is
  the screen title), falls through into `showme`. Also from 821, 2727.
- `showme` (737) : ShowTitle(myscreen, TRUE) as tail `jmp` (title string was changed
  in place; ShowTitle forces the redraw). Also from 1556, 1601.
- `filesave` (745) : `getfname2` (a2 = "Name to save file as"; cancel trick as above), d7 =
  name, `xtraat`=d7. In `dosbuff`: name+".bak"; DeleteFile(.bak); Rename(name → name.bak).
  Falls through into `realsave`. (With `cripple`≠0 only `rts`.)
- `realsave` (769) : fall-through label only (no other callers). `doopen` d2=1006,
  `sleepon` (preserves a6=DOS), for each line: ASCII mode (`tokenise`=0): 0 at `numchars-1`
  temporarily replaced by LF; writes `numchars` bytes (tokenised: incl. 0 as
  line separator), restores the 0. Write<0 → `error(error3)`, abort, `allalt` stays
  set; otherwise `clr allalt`. `doclose`. Callback `savextra` analogous to loadxtra (d2=1006,
  d7 = handle, a6 = DOS). Icon: if `defaultreqga9+13` bit 7 (gadget "Icons" SELECTED –
  not the variable `makeicons`!) `openinfo` → Write(`srcinfo`, 396 bytes), Close,
  `closextra`. `pointeron`, tail `bra showfname`. `rts` 825 only with cripple.

### About requester

- `doabout` (834) : `openreq(aboutreq)`; gadget 1 (OKAY) → `rts`; gadget 2 (invisible
  area with text "Thanks go to:") → `openreq(specialreq)`, wait for its gadget 1
  (ENOUGH!), `rts`. No closereq (happens in getinput/openreq, outside).

### Find / replace

Globals outside: `findreqla16` = search string buffer, `findreqla17` = replace string,
`findreqsg1/2` = StringInfo (offset +8 BufferPos, +12 DispPos), `findreqga8+13` bit 7 =
match case.

- `find` (853) : BufferPos/DispPos of both StringInfos := 0; backup of la16 → `charbuff`,
  la17 → `charbuff2`; `openreq(findreq)`, `actgad(findreqga1)`. Gadget IDs: 1 = Return in
  the search gadget → activate replace gadget; 2 (Return in the replace gadget) and 3 → `bra next`;
  4 → `bra previous`; 5 → `bra replace`; 6 → `bra replaceall`; 8 (case switch) →
  reactivate search gadget; otherwise (cancel) copy strings back from charbuff/charbuff2,
  `rts`. All actions as tail calls.
- `next` (926) : a4 := nextitem(a4) (0 → `beepem`), d6=1, falls through into `dosearch`.
- `previous` (959) : a4 := lastitem(a4) (0 → beep), d6=−1, `bra dosearch`.
- `dosearch` (931) : in d6 = direction ±1, a4 = start line (already the neighbouring line).
  Empty search string → beep. `sleepon`; d7 = curstop+cursy+d6 (line number). Loop
  `isithere` (from column 0; out d0 = column, N=1 not found). Not found: d7+=d6,
  <0 or ≥ numlines → `beepem` (does pointeron). Found: `pointeron`, `gotolined0`
  (d0 = line, a4 = thisitem), `horizpos` (d1 = column), tail `bra showhprop`. Never searches
  in the current line behind the cursor.
- `replace` (965) : empty search string → beep; `replace2` on a4; Z=0 (nothing replaced) →
  beep, otherwise tail `bra showall`. Replaces ALL occurrences in the current line.
- `replaceall` (902) : empty search string → beep; requester `repall` (1 = from here (a4),
  2 = from start (`firstitem`), 3 = cancel). `sleepon`; each line: `thisitem`=a4,
  `replace2`, next. `calcitem`, `showall`, tail `bra pointeron`.
- `replace2` (971) : in a4 = line. Out: **Z=1 (d0=0) if ≥1 replacement**, otherwise d0=−1/Z=0;
  a4 = possibly new line (makeline), `thisitem` = a4. Procedure: `isithere` (out d0 = position,
  a1 = line text (detok in `charbuff` or `chars(a4)`)); `allalt`=−1; builds in `charbuff2`:
  prefix (d0 characters) + replace string + rest after len(search string), spaces up to
  `charbuff3`; d1 = position behind the replacement → stack; `charstobuff`(a2=charbuff2),
  `makeline`; d5 := position, `isithere2` from d5 (prevents endless loop when
  replace string ⊇ search string), continue as long as found.

---------------------------------------------------------------------------------------
## B. Register conventions

- **Main loop**: when calling a handler from `jumptable`: a4 = `thisitem`,
  d7 = code (byte; ASCII/control characters, menu codes `$IM`: high nibble = item, low nibble =
  menu, e.g. $10 fileopen, $20 filesave, $30 defaults, $50 prtfile, $40 doabout (menu 0);
  $03/$13/$23/$33/$43 find/next/previous/replace/replaceall; $91/$a1 blocktab/untab;
  $90 escape). Handlers may destroy all registers.
- File routines (`doopen`/`fillbuff`/`doread`/`readline`/`doclose`): **d7 = name before
  doopen, file handle afterwards**; a5 = read pointer in `dosbuff`; d6 = remaining bytes; d2 = mode
  (1005/1006) for doopen/openxtra; d5 = line length incl. 0 (readline → allocline);
  a6 = DOSBase after `calldos`/`doclose`, and openxtra/openinfo silently
  assume it.
- Search: d6 = direction, d7 = line number, a4 = line, d5 = start column (`isithere2`).
- `defaults`/requesters: `getinput` → d7 = −1 on gadget, d6 = GadgetID; `getinput2` → d7 = 0
  if nothing.
- a6: no global convention; `int(pc)`/`dos(pc)`/Exec (4) reloaded each time.
- Called by blitz2 via comdata and therefore register-preserving: `pointeron` (comdata+72),
  `sleepon` (+76) – movem a0-a1/d0-d3/a6 (d4-d7/a2-a5 are not touched anyway).
  Exception: sleepon path "already active" (`.done`) jumps after the movem to `makemenus`,
  which destroys a0/d0.
- Callbacks into blitz2 (from edstruct): `loadxtra`/`savextra`: in d7 = handle, a6 = DOSBase;
  must preserve d7/a6. `cleanup` (via `doclean`): TED saves d1-d7/a0-a6.
- `outside` external: in a1 = edstruct; out a0 = `.tomain`, a1 = `comdata`, d0 = 0, Z=1.
  `.tomain`: in a0 = file name (C string, possibly in "…").

---------------------------------------------------------------------------------------
## C. Globals in this scope

| Name (line) | Type | Init | Purpose |
|---|---|---|---|
| `int` (35, in macro `openint`) | .l | 0 | IntuitionBase – **sits in the code** behind `bra int+4` |
| `intname` (35) | String | "intuition.library" | in the code |
| `gr` / `grname` (33, macro `opengr`) | .l / String | 0 / "graphics.library" | GfxBase, in the code |
| `myint` (306) | struct Interrupt (22 B) | ln_Type=2 (NT_INTERRUPT), Pri 0, Name 0, is_Data 0, is_Code=`myroutine` | VBlank server |
| `pointerat` (311) | .l | 0 | chip copy of `pointer` |
| `dumat` (312) | .l | 0 | chip buffer for current sleep frame (= pointerat+64) |
| `sleepcurr` (313) | .l | 0 | current frame in `sleep`; ≠0 = sleep active (also menu lock) |
| `sleepdelay` (314) | .w | 0 | VBlank counter |
| `sleep` (316) | incbin sleep.bin, 704 B | – | 8 frames of 88 B (sprite: ctrl long, 20 lines×2 words, end long) |
| `pointer` (319) | incbin pointer.bin, 64 B | – | normal pointer, height 14 |
| `dumsleep` (322) | ds.b 88 | 0 | template for chip sleep frame (copied along) |
| `finish` (325) | Label | – | end of the chip block (pointer..finish = 152 B) |
| `xtraat` (636) | .l | 0 | pointer to current file name (for .xtra/.info) |
| `xtranull` (637) | .l | 0 | position of the appended extension |
| `xtratxt`/`infotxt` (638/639) | String | ".xtra"/".info" | 6 bytes each, code after them even |
| `bakname` (742) | String | ".bak" | |
| `srcinfo`..`srcinfof` (827) | incbin src.info, 396 B | – | icon for source files, written directly from the code hunk |
| wbheader: `wb_infoname`, `wb_dosname`, `dos` (.l), `us` (.l, own task), `wbmess` (.l), `progfile` (192 B) | | | startup code variables; `dos` is DOSBase for all of TED |

Intuition structures:
- `aboutreq` (aboutreq:1): struct Requester (Left 108, Top 2, 312×164), ReqGadget →
  `aboutreqga1` → `aboutreqga2` (linked via NextGadget), ReqBorder → 3 border chains
  (bo1→la2→bo2→la5; bo3 = border of the OK gadget), ReqText → IntuiText chain in1…in9
  (title "BLITZ BASIC 2", "Release 1.0", copyright, names). Gadgets: BOOL, Activation 5
  (RELVERIFY|ENDGADGET); ga1 ID 1 " OKAY! " (128,148,64×12), ga2 ID 2 without border, text
  "Thanks go to:" (108,86,104×8). No runtime change.
- `specialreq` (specialreq:1): Requester (4,2, 512×172), one gadget `specialreqga1` ID 1
  " ENOUGH! " (208,156,80×12), 3 border chains, 16 IntuiTexts. No runtime change.
- Requesters serve only as template for `openreq` (builds a window from them, outside).

Fields of foreign structures changed at runtime (defined outside):
- `defaultreqga8+12` (Gadget.Flags.w): |= $100 GFLG_DISABLED / &= ~$100; bit $80 SELECTED
  (tokenise) – word-wise. `defaultreqga7+13`, `defaultreqga9+13` (low byte of Flags) bit 7 =
  SELECTED (6-pixel font, icons) – byte-wise. `defaultreqga4..6+12`: SELECTED cleared.
- `defaultreqla47..49`: string buffer := "0".
- `findreqsg1/2 +8/+12`: StringInfo.BufferPos / DispPos := 0.
- `comdata+84` (long, init 0): := `&defaultreqga9+13` (blitz2 reads "create icons" through it).
- `newsctit`: screen title buffer, overwritten in place.
- Window.Flags (+24, byte +25 bit 0) of `mywindow`/`req_window`.

---------------------------------------------------------------------------------------
## D. Self-modifying code / data in the code

- Macros `opengr`/`openint` (33/35) place `gr`/`int` + names in the code flow (`bra x+4`
  skips them). No problem in C (normal globals).
- `bsr rawed+32` (39) / `bsr rawed+32+8` (128): entries into the incbin hunk `rawed.obj`:
  +32 = first code instruction after the hunk header (HUNK_HEADER…HUNK_CODE+length = 8 longs).
  Jump table: +0 init (open console.device), +4 rawkey→ASCII (at 5928), +8 cleanup.
  **rawed is NOT relocated** → must be position-independent.
- `jsr flink` (76): relocates the incbin `filerequest` (7953): reads code length at +28,
  adds the base (hunk+32) to every offset of the first HUNK_RELOC32 group. Only ONE group,
  count 0 would run 2^32 iterations. Executed on every `outside` (only once per
  load – double relocation would be fatal). No CacheClearU afterwards (68040 copyback!).
- `filerequest+36` (`ezfreq`, outside) = entry into the relocated code.
- No patching of instructions in this scope; sprite data are copied into the
  chip buffer in the interrupt (Intuition points directly to `dumat`, i.e. the animation continuously
  changes the data referenced by SetPointer).

---------------------------------------------------------------------------------------
## E. Heap/list structures

| Structure | Allocation | Fields | Routines |
|---|---|---|---|
| Line (`item`) | `lenitem`+d5 (d5 = length incl. 0), MEMF_PUBLIC via `alloc2` (failure → `outmem`) | +0 nextitem.l, +4 lastitem.l, +8 numchars.b, +9 chars[] 0-term. | allocline, makeline, freeall; list: `firstitem`, `enditem`, `thisitem`, `numlines` |
| Work buffer | AllocMem(doslen+3*chcols+6 = 2438 at chcols=128, PUBLIC) | +0 `dosbuff` (2048), `charbuff` (chcols+2), `charbuff2` (chcols+2), `charbuff3` (chcols+2), `charbuff4` = **block end (0 bytes)** | init in outside, freed in `openfailed`; charbuff3 serves as end mark for charbuff2 |
| Chip pointer block | 152 B MEMF_CHIP | +0 pointer (64), +64 sleep frame (88) | mouseinit/mousefinit/makedum |
| `.xtra` name | no heap: extension is appended to the name buffer supplied by the caller (`xtraat` = d7) and removed again by `closextra` | | openmore/closextra |
| `.bak` name | in `dosbuff` | | filesave |

Line format in the file: ASCII mode lines with LF; tokenised: raw bytes per line incl. 0.
Loading (`readline`, outside) detects LF → ASCII (+ tabs expanded to `tabstop`),
token bytes ≥ $80 are 2 bytes long.

---------------------------------------------------------------------------------------
## F. External calls

- exec: OldOpenLibrary (dos/graphics/intuition), CloseLibrary, AllocMem (dosbuff PUBLIC;
  pointer block CHIP), FreeMem, AddIntServer/RemIntServer (INTB_VERTB=5), Forbid/Permit/
  ReplyMsg/WaitPort/GetMsg (wbheader). `move.l 4,a6` absolute.
- dos: Open (1005/1006), Close, Read (wbheader, loaddefs/fillbuff), Write (prtfile, realsave,
  .info), DeleteFile, Rename (.bak), Lock/UnLock (.info test), CurrentDir (wbheader).
- intuition: SetPointer (mywindow, req_window), ClearPointer, ShowTitle, RefreshGList
  (`callint`); via helper routines openreq/getinput/actgad/screenopen.
- graphics: only via helper routines (propadj → SetRGB4).
- Interrupt: `myint`/`myroutine` VBlank server (runs only between sleepon and pointeron).
- Chip mem: pointer sprite data.
- Direct structure pokes: Window.Flags RMBTRAP.
- blitz2 callbacks: `loadxtra` (fileopen2), `savextra` (realsave), `cleanup` (doclean;
  in blitz2 = 0). `menurout`/`menitems` only via `getstuff` (outside).
- No `$dff000` access, no blitter in this scope.

---------------------------------------------------------------------------------------
## G. Special handling in C

- **Init entry `outside`** (hunk+4, no startup code before it!): blitz2 expects
  out a0 = `.tomain`, a1 = `comdata`, Z flag. Glue stub in asm: offset 0 `bsr edstart` (4 bytes),
  offset 4 entry; `.tomain` as second stub (a0 = name). Standalone start via libnix
  startup instead of wbheader possible, but then the hunk layout (blitz2: `jsr 8(seg)`)
  must still be right → own entry stub as first object.
- **comdata entries** are an asm convention: pointeron/sleepon must preserve all registers
  → glue wrapper with movem.
- **Callbacks loadxtra/savextra/cleanup**: TED → blitz2 with d7/a6 → glue stub
  (`call_xtra(fn, fh, DOSBase)`), cleanup with movem save.
- **VBlank server `myroutine`**: is_Code convention (a1 = is_Data, a5 = code), return Z=1;
  gcc does not guarantee Z → asm stub `moveq #0,d0; rts` around C function, or entirely in asm.
  Shared variables (`sleepcurr`, `sleepdelay`) `volatile`; access via A4 (baserel)
  not available in the interrupt → stub must load a4 (or variables absolute).
- **`jumptable`**: table {code, flag, fnptr} → C array of structs; `mainedit` flag from the
  stack → local variable.
- **`memstack`/`outmem`**: longjmp back to `memcont` → `setjmp` in mainedit; outmem can
  come from inside blitz2 (comdata `extalloc`) → jump across foreign frames (asm glue with
  saved a7, or setjmp buffer + glue that sets a7). Before the first iteration
  memstack = 0 (see I).
- **Stack tricks `addq #4,a7`** in getfname3/doopen/erase: cause the *caller* to abort
  (fileopen, fileopen2, filesave, realsave). In C: check return value and return.
  Caution realsave: doopen failure aborts after the rename (file then exists only as .bak).
- **Condition codes as return value**: outside (Z), screenopen (N), donew (Z), openxtra/openinfo
  (Z), fillbuff (Z via d6), readline (N), isithere (N, d0), replace2 (Z), wb_exit/dosopen (Z).
- **Byte/word aliasing**: Gadget.Flags word-wise (`+12`) and byte-wise (`+13`, bit 7 =
  $0080) – in C uniformly `Flags & GFLG_SELECTED`; comdata+84 must however still point to the
  low byte (big-endian +13). `int1+2` = low word of a long. `move #$3000,la47` =
  string "0". `move.b (a0)+,-(a7)` = 2 bytes of stack.
- **Layout dependency**: `dc ' :'` before `filepath` (empty-path trick in `.tomain`).
  `defstart..defend` is read/written as a block (`l:blitzeditor.opts`, 50 bytes)
  and read from the backup with fixed offsets (0, 6, 8, 10) → keep as packed structure
  big-endian.
- `newsctit` is changed in place as screen title (ShowTitle as refresh).
- Data structures with code pointers: `myint.is_Code`, `jumptable`, `comdata`.

---------------------------------------------------------------------------------------
## H. Classification

| Routine | Class | Remark / cut |
|---|---|---|
| `edstart`/wbheader | GLUE | replace by startup; argument parsing (`fromcli`) possible as KERN helper function |
| `outside`, `.tomain`, fail chain | GLUE+UI | entry stub asm; rest C (init/teardown) |
| `mainedit`, `memcont` | UI | dispatch core (table) host-testable, but getline/show* UI |
| `mouseinit`, `mousefinit`, `pointeron`, `sleepon`, `setpoint`, `makedum` | UI | pointeron/sleepon with glue wrapper for comdata |
| `makemenus`, `menuson`, `menusoff` | UI | |
| `myroutine`, `myint` | GLUE | interrupt |
| `blocktab`, `blockuntab` | KERN | line rebuild via detok/makefirst; `showall` at the end → hook `redraw_all()`; `bloksetup2`/`calcitem` are logic |
| `defaults` | UI | value check (int1..3) and setdefs backup separable |
| `prtfile` | UI + KERN | extract formatting "line → text with ESC[1m/ESC[22m" as KERN function; requester/Open UI |
| `openmore`, `closextra`, `openxtra`, `openinfo` | KERN | DOS Open/Lock can be abstracted |
| `fileopen` | UI | erase/getfname |
| `fileopen2` | KERN + hooks | load loop (doopen/readline/allocline) KERN; hooks: `sleepon` before, `pointeron`/`showall`/`showhprop`/`showfname` after, callbacks loadxtra/cleanup (GLUE) |
| `filesave` | UI + KERN | requester UI; .bak rename KERN |
| `realsave` | KERN + hooks | write loop KERN; error()/sleepon/pointeron/showfname hooks; savextra GLUE; writing .info KERN (srcinfo as byte array) |
| `showfname`, `showme` | UI | |
| `doabout` | UI | |
| `find`, `replaceall` | UI | requester logic; loop over lines in replaceall KERN-capable |
| `next`, `previous`, `dosearch` | KERN + UI | search (line counter, isithere) KERN; end `gotolined0`/`horizpos`/`showhprop`/`beepem` as hook "cursor_to(line,col)" or "beep()" |
| `replace` | KERN + UI | replace2 KERN, `showall`/`beepem` hook |
| `replace2` | KERN | uses charstobuff/makeline (KERN, if makeline does not draw) |

---------------------------------------------------------------------------------------
## I. Peculiarities / possible bugs of the original

1. **Failure of `outside` on external call reports success** (24–34, 132–134): fail chain
   ends in `wb_exit`; there `move.l wbmess,d0` (=0, since wbheader did not run) → Z=1 → blitz2
   considers init successful and uses a0/a1 (garbage). Moreover `returncode` is never delivered as
   exit code (d0 overwritten). BUG(orig).
2. Standalone, `dosopen` in wbheader fails: `wb_exit` `rts` returns to `outside`
   (because of `bsr edstart`), a1 ≠ 0 possible → init continues, possibly double ReplyMsg.
   Theoretical (dos.library is never missing). BUG(orig).
3. `fromcli` reads "stack size" from 4(a7) = DOS return address (because of bsr). Without consequence (d2
   unused).
4. wbheader: CurrentDir is never reset, .info handle not closed on short Read
   (as in the blitz2 survey).
5. `mouseinit`: AllocMem(CHIP) without error check → copy to address 0. BUG(orig).
6. `mousefinit` does not remove a VBlank server that may still be active (server then points to
   freed code/chip memory, if TED ends during sleep).
7. `sleepon`, path "already active": a0/d0 are destroyed by makemenus after the movem restore
   – comdata+76 then does not preserve all registers.
8. `outmem` before the first mainedit dispatch (e.g. out of memory while loading the
   start file in `.tomain` → `fileopen2`): `memstack` = 0 → a7 := 0. BUG(orig).
9. `charbuff4` points exactly to the end of the buffer (0 bytes). Unused, but a trap.
10. Buffer overflows without check (into charbuff3 or past the end of the allocation):
    `replace2` (long replacements), `blocktab` (tabstop + line > chcols+2),
    `prtfile` (expanded tokens + 8 bytes ESC per token in `charbuff`, only 130 bytes, only
    `charbuff2/3` behind it), `.tomain` (filepath up to 191 + filename up to 63 in charbuff).
    With blocktab/replace2 `charstobuff` additionally truncates silently to chcols
    (data loss at the line end).
11. `blockuntab` removes the first `tabstop` characters even if they are not spaces;
    with `tabstop`=0 every block line is completely replaced by spaces (a1 still points
    to the buffer start, `beq .loop3` fills from there). BUG(orig).
12. `defaults` allows `tabstop`=0 (only upper limit checked) → `readline` (`divu tabstop`)
    raises a division-by-zero exception when loading a file with TAB. BUG(orig).
13. `defaults` OK: `cursy` is limited with the old `edrows` before the screen is opened with the
    new font height (at 6→8 pixels possibly cursor below the window) –
    uncertain, check in screenopen/calcpos.
14. `prtfile` always treats bytes ≥ $80 as token, also in ASCII mode/standalone; there
    `tokens` = 0 → `findtoke` reads the list from address 0. Write errors are ignored;
    `numlines`=0 would run 65536 iterations (practically ≥1).
15. `realsave`: icon switch is queried at the gadget (`defaultreqga9+13`) instead of `makeicons`
    (equivalent as long as both are in sync). On write error, after `error`,
    `.xtra`/`.info` are still written. doopen failure after rename → only `.bak` remains.
16. `openxtra`/`openinfo` and the Close calls after the callbacks rely on a6 =
    DOSBase from `doclose` or from the callback (implicit convention).
17. `flink`: no cache flush after relocation (68040/060 copyback); only one reloc group.
18. `dosearch` never searches the rest of the current line; `next` on the last line
    beeps, even if the match were in the current line behind the cursor.
19. `a0toname` (2990, outside, used by `.tomain`) compares `d0` with ':'/'/' without loading d0
    → path/name separation random; result usually stays correct due to the reassembly
    in `.tomain`, but `filepath`/`filename` (requester default) wrong.
20. Open question for the escape chunk: `escape` (1016) pops `lea 12(a7)`, the stack balance
    from mainedit is 10 bytes (bsr mainedit 4 + flag 2 + jsr 4) – return from `freeall`
    would then be 2 bytes off. Check whether 2 extra bytes lie somewhere (getline?).
21. `freeall` (outside): size via `moveq #lenitem,d0 / add.b numchars` → byte overflow at
    numchars ≥ 247 → FreeMem with wrong size.
