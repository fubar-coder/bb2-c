# Survey 03 – ted.s lines 1981–3062

Scope: mouse in the text area (`mousedown`, dragging/scrolling, `gomouse` = click into the
label column), goto requester (`gotoreq`, `gotoline`), F1/F2 block marks
(`blockstart`/`blockend`), open/close Workbench, console window for running
programs (`openio`/`closeio`, comdata+60/64), CLI (`mycli`), comdata services
(`refgads`, `tokone`, `tokall`, `detokallz`, `actgad`, `extalloc`), help key (`help`),
cleanup callback (`doclean`), building the COMPILER menu (`getstuff`), label column
("mousables": `makemice`/`freemice`/`remember*`/`mforget2`/`showmice`), `a0toname`,
`printmem`, memory error handling (`alloc2`/`outmem`/`extalloc`/`doalert`).

Everything sits in the one code section; data in the code (`gotoreq*`, `ioname`, `helphex`,
`newctop` …) are written at runtime → in C writable (non-`const`) data.

Local labels: `oneloop` (2290) is a top-level label **in the middle of `mousedown`**; the
following `.skip`, `.skip2`, `.nben` therefore belong to the scope `oneloop`, not
`mousedown` (mind this in the translation, there are `.skip`/`.skip2` of the same name before).

---------------------------------------------------------------------------------------------
## A. Routines

Convention "handler": entry via `jumptable`/`menutable` from `mainedit` (ted.s:156–171):
`jsr (a0)` with a4 = `thisitem`, d7 = key code/event, d6 = menu/gadget no.; on the
stack above lies the flag word (redraw vprop); `memstack` = a7 before the `jsr`.
Return via `rts` to `memcont`.

### Mouse

- `gomouse` (1981) : click into the label column. **Not a subroutine**: only reached via `bcc gomouse`
  from `mousedown` (2007) (tail branch, `rts` goes to `mainedit`). in: d2 = column
  (screen, characters), d3 = row (screen) from `getmousexy`. Ignored if
  d2 < `edcols`+2 (2 columns gap) or d3 ≥ `nummice` (word access to `dc.l`, see G).
  Walks d3+1 nodes of the `mice` list (`lea mice,a0` / `move.l (a0),a0` / `dbf`), searches
  the line pointer `4(node)` in the line list from `firstitem` (index in d0) and jumps
  **`beq gotolined0`** (tail call, d0 = line index from 0). Not found → `rts`.
  Index of the label column = screen row, the list is not scrolled.
- `mousedown` (2001) : `jumptable` code 253 (flag 1). Text selection with the mouse.
  in: a4 = `thisitem`. out: nothing; ends via tail call `bra showall` / `bra cursor2`
  or `rts`. Procedure:
  1. `getmousexy` (4015: d0 = MouseX px, d1 = MouseY px, d2 = column, d3 = row; N=1
     if outside the text area) – N → `rts`. d2 ≥ `edcols` → `gomouse`.
     curstop+d3 ≥ `numlines` → `rts`.
  2. **Stack frame**: push `btopx/btopy` (as `.l`), `bbotx/bboty` (as `.l`). If a
     block exists (`bboty` ≠ −1): `blokdis` (sets `bboty`=−1, `bstx`=−1) + `showall`.
  3. Anchor: `bloktopx/y` = `blokbotx/y` = (cursoff+d2, curstop+d3); `showprops2`;
     d5/d6 = last end position (column absolute, row screen); `invflag`=0;
     push `curstop`, `cursoff`, a4.
  4. Loop (`.loop`): `invertarea`, then **`getinput2` (non-blocking) → busy poll**.
     d7 = −4 (SELECTUP, set in `handled0`, ted.s:5915) → `.mouseup`.
     Otherwise `getmousexy`: outside and MouseY < `topskip` → `.mup` (scroll one line
     up: `scrdownall`, a4 = lastitem, curstop−1, `showtopitem`, d6+1; comment
     "currently FUCKED"); outside below → one line down (`scrupall`, a4 = nextitem,
     curstop+1, `showbotitem`, d6−1), only if curstop+edrows < numlines. Afterwards
     correction of the inversion if the selection lies above the anchor.
     Inside: d2 += cursoff (absolute column), row limited to numlines−1;
     MouseX = 0 (d0) → scroll horizontally left (cursoff −= 2, `scrollrite`), column −
     cursoff ≥ edcols → scroll right (cursoff += 2, `scrollleft`), each with
     temporary `btop/bbot` = selection, `sortblox`, afterwards `blokdis`.
  5. `.skip2`: position unchanged → `.loop`. Otherwise set `blokbotx/y`, determine direction to the
     anchor (`.above`/`.below`); same direction as `invflag` → invert only the difference
     (`orderem` + `invertarea2`, ±1 correction); change of direction (`.fix`) →
     flip `invflag`, invert old area anchor…old (clamped to the screen) and new area
     anchor…new.
  6. `.mouseup`: `newctop/newcoff` = current curstop/cursoff; pop a4 → `thisitem`,
     `cursoff`, `curstop` (= **scroll position before dragging is restored**).
     `blokbotx` < 0 → end of the previous line (`blokboty`−1, x = chcols−1) or x = 0 if
     already on the anchor line; ≥ chcols → chcols−1.
     Selection > 1 character: `bloken`, discard saved btop/bbot (**`addq #8,a7`**),
     btop/bbot = selection (`.l` copy), `sortblox`, `showhprop`, `bra showall`.
     `.one` (click without dragging): btop/bbot back from the stack (old block remains!),
     cursor to the anchor: cursx = bloktopx − cursoff; `oneloop` (2290) walks from
     `thisitem` (line curstop+cursy) via next/last to line `bloktopy`, sets cursy,
     `thisitem`, `calcpos`. Old block present → `bloken` + `bra showall`; otherwise
     `bra showall` if curstop/cursoff have changed compared to `newctop/newcoff`,
     otherwise `bra cursor2`.
- `oneloop` (2290) : only an internal loop of `mousedown` (see above), no entry of its own.

### Goto / block marks / Workbench

- `actgad` (2409) : comdata+68 and internal (574, 869, 899, 2428). in: a0 = gadget. `Delay(8)`
  (dos, a6 = dos), then `ActivateGadget(a0, req_window, NULL)`. out: d0 = BOOL from Intuition.
  destroys: a1, a2 (=0), a6 (= int), d0/d1. a2 from the caller is ignored (always 0).
- `gotoline` (2420) : `menutable` $22. Sets StringInfo `BufferPos` (sg1+8) and `DispPos`
  (sg1+12) = 0, `gotoint` (LongInt) = 0, buffer = "", `openreq(gotoreq)`, `actgad(ga1)`.
  Loop `getinput` until d7 = −1; d6 = 1 (Return in the string) / 2 (OK) → `.ok`, 3 (CANCEL)
  → `rts`; others → continue. No `closereq` here: `getinput` closes the requester itself for
  gadgets with ENDGADGET (Activation bit 2) (ted.s:5888–5894); all three have it.
  `.ok`: d0 = `gotoint`; 0 or > numlines (word comparison!) → `beq/bhi beepem` (tail);
  otherwise d0−1 → `bra gotolined0` (tail).
- `blockstart` (2458) : `jumptable` 129 (F1). `bstx` = cursoff+cursx, `bsty` = curstop+cursy.
- `blockend` (2468) : `jumptable` 130 (F2). `bstx` < 0 → `bmi beepem` (tail). First writes
  **`move d0,btopx` with undefined d0** (key code byte from mainedit, see I).
  End = cursor (d0,d1). Equal to start → `blokdis` + `bra showall`. Otherwise: end x = 0 and
  y > 0 → end = (chcols, y−1); then x−1 (exclusive end) → `bbotx/bboty`, `btopx/btopy`
  = start; `sortblox`, `bloken`, `bra showall`.
- `closewb` (2501) : `menutable` $70. `CloseWorkBench()`. (commented out: patching the screen
  type.)
- `openwb` (2507) : `menutable` $80. `OpenWorkBench()`, `ScreenToFront(myscreen)`,
  tail `jmp ActivateWindow(mywindow)` (a6 = int from `callint`).

### Console window / CLI

- `findio` (2513) : in: a0 = name (without length), d0 = length, `dos` = DOSBase. Walks
  `DOSBase->dl_Root(34)->rn_Info(24, BPTR)->di_DevInfo(4, BPTR)` and the
  DevInfo chain (`dvi_Next` @0, `dvi_Name` BSTR @40), comparing length + characters
  (case-sensitive). out: d0 = 0, **Z=1 if found**; d0 = −1, Z=0 otherwise.
  destroys: d1, a1, a2, a3. No Forbid/LockDosList.
- `calciorows` (2541) : in: `gr` = GfxBase. out: d0 = 200 if `GfxBase->DisplayFlags`
  bit 0 (NTSC, byte @207) is set, otherwise 256. destroys a1.
- `openio` (2548) : comdata+60 (blitz2.s:1697 before RUN) and `mycli`. Patches `hiteasc`
  (3 spaces), height d0 = `newscreen+6` (height of the editor screen) if
  `screensopen`, otherwise `calciorows`; `numtoascii` writes the digits into `hiteasc`.
  `FindTask(0)` → a3, saves `pr_CIS` (156) → `oldih`, `pr_COS` (160) → `oldoh`.
  `findio("NEWCON",6)`: found → `Open("NEWCON:0/0/640/<h>/Blitz 2 CLI Window", 1006)`,
  otherwise `Open("CON:…")`. Handle → `myhandle`; 0 → `rts` with **Z=1**. Otherwise
  pr_CIS = pr_COS = handle, `WBenchToFront()`, `rts` – **Z undefined afterwards** (flags
  from Intuition; `mycli` tests `beq`!). destroys d0–d3, a0–a3, a6.
  No `movem` – blitz2 itself saves nothing special (asm callers only call
  `calcvbr` afterwards).
- `closeio` (2597) : comdata+64 (blitz2.s:1722) and `mycli`. `FindTask(0)`, pr_CIS/pr_COS
  = `oldih/oldoh`, `Close(myhandle)` (even if 0, see I). If `screensopen`:
  `ScreenToFront(myscreen)` + tail `jmp ActivateWindow(mywindow)`; otherwise `rts`.
- `mycli` (2616) : `menutable` $60. `openio`; Z=1 → `rts`. `Execute("", myhandle, 0)`
  (d1 = `nullcom` = empty string, d2 = input, d3 = 0) = interactive shell in the window until
  EndCLI; then tail `bra closeio`.

### comdata services: gadgets / tokeniser

- `refgads` (2629) : comdata+28. in: a0 = gadget, d0 = count. Tail
  `jmp RefreshGList(a0, req_window, NULL, d0)`.
- `tokone` (2636) : comdata+56. in: a0 = 0-terminated line. d1 = strlen, a2 = a1 = a0
  (**in-place**), `bpl tokit2` (tail; always taken up to 32767 characters). out (from
  `tokit2`): line tokenised, d5 = length incl. 0. destroys practically everything except a4.
- `tokall` (2648) : comdata+20. `tokenise` ≠ 0 → `bne beepem` (tail). `not tokenise`
  (→ $FFFF). Falls through into
- `tokall2` (2653) : also entered from 557 (defaults requester). For all `numlines`
  lines (counter as word on the stack): d1 = numchars−1 (length without 0; < 0 → skip
  line), `tokit2(chars → charbuff)`, `thisitem` = a4, `makeline` (replaces the line,
  a4 = new line). Afterwards tail `bra detokallc` (= `calcitem` + `showall`).
- `detokallz` (2670) : comdata+16. `tokenise` = 0 → `beq beepem` (tail). `clr tokenise`.
  Falls through into
- `detokallz2` (2673) : also from `detokall2`. For each line `detok(chars → charbuff)`,
  d5 = length+1, `makeline`. **Does not redraw and leaves `thisitem` = last line**
  (only `detokallc` corrects it). `rts`.
- `detokall` (2689) : without reference (dead entry). Checks/clears `tokenise`, falls into
- `detokall2` (2692) : entry from 556 (defaults). `bsr detokallz2`, falls into
- `detokallc` (2694) : `calcitem` + tail `bra showall`. Also target of `tokall2`.

### Help, cleanup, menu

- `help` (2697) : `jumptable` 139 (HELP). in: a4 = `thisitem`. `tokenise` = 0 → `beepem`.
  Walks the tokenised line up to the cursor column (cursoff+cursx); counts
  display columns (token = length of the name). Remembers the **last token that begins at or before
  the cursor column** (not necessarily under the cursor): `helptoke` = token no.,
  d1 = name (`findtoke` → a3 = node+6), a2 = help text (after the name).
  No token → tail `beq showfname`. Otherwise `newsctit` = name + (" " unless help begins
  with "(") + help, `ShowTitle(myscreen, TRUE)`. If `qualifier` & $80 (right
  Amiga key): `helphex` = 4 hex digits of `helptoke`, tail
  `jmp Execute("blitz2:help XXXX", 0, 0)` (synchronous).
- `doclean` (2771) : internal (684 fileopen, 1035, 1595 new). Calls `cleanup`
  (edstruct+20, 0 with blitz2) with `movem.l d1-d7/a0-a6` saved; d0 likewise
  saved → preserves **all** registers.
- `getstuff` (2781) : once from `outside` (79), only if `external`. `tokens` ≠ 0 →
  `tokenise` = 1, `defaultreqga8+12` |= $80 (gadget flag SELECTED = check mark
  "Tokenise"). `menitems` ≠ 0 → `ClearMenuStrip(mywindow)`, builds from `menitems`
  (edstruct+4) a menu "COMPILER" as copy of `menu2` + per entry a copy of
  `menu2item1`+IntuiText (see E), hangs it on **`menu2.NextMenu`** (static data
  patched), tail `jmp SetMenuStrip(mywindow, menu1)`. Format `menitems`:
  `title\0 { text\0 shortcutbyte(0 = none) }* \0`.

### Label column ("mice")

- `makemice` (2850) : from `showmice`. For each line: first non-space =
  `label` character (edstruct+26, ".") → `remember2(a0 = line)`; in doing so sets
  `mousealt` = d1 (label character, ≠ 0). Saves a1. destroys d0, d1, a0, a6.
- `freemice` (2869) : from `showmice` and 1036. Frees `nummice` nodes (8 bytes),
  `mousealt` = old count, `nummice` = 0; `mice` stays dangling (only used via the counter).
- `remember` (2883) : from `makeline` (3822) and 3372, in: a0 = line (ignored). Only
  `mousealt` = −1 → after the handler `memcont` rebuilds the list completely (`showmice`).
- `remember2` (2887) : in: a0 = line. `AllocMem(8, MEMF_PUBLIC)` (not cleared);
  failure → `beq outmem` (with a0 still on the stack – irrelevant, outmem resets a7).
  Appends the node **at the end** (list walked with `nummice`−1 steps), `4(node)` = a0,
  `nummice`+1. **`next` of the new node stays uninitialised.** Saves d0/d1/a1/a2.
- `mforget2` (2912) : from 3717, 4595. Only `mousealt` = −1 (like `remember`).
- `showmice` (2916) : from `outside` (105), `.tomain` (172 = memcont, if `mousealt`).
  `label` = 255 (no blitz2) → `rts`. `freemice`, `makemice`. No labels and
  `mousecols` ≠ 0 → `mousecols` = 0, `screenclose`, tail `bra screenopen2`. Labels and
  `mousecols` = 0 → `mousecols` = 12, `screenclose`, `screenopen2` (redraws everything).
  Then for row 0…edrows−1: label text (with `tokenise` first `detok` to **`charbuff`**)
  without leading spaces up to 0/`colon`/space, max. `mousecols` characters, rest with
  spaces; output via `qprint` into `bitmaps+4` (second plane) from byte
  `edcols+2`, line spacing `fontmul`.

### Miscellaneous

- `a0toname` (2990) : from `.tomain` (87). in: a0 = path (possibly in `"`). Is supposed to split into `filepath`
  (directory) and `filename` (name); but compares **d0 instead of the copied
  character** (see I). Effectively: `filepath` = "", `filename` = whole string (closing
  `"` removed). Works because the word `dc ' :'` (6139) stands before `filepath`: `.tomain`
  checks `-2(a0)` = ':' and then appends no '/'.
- `printmem` (3019) : init (73, 106), `memcont` (169, if `memalt`). `AvailMem(MEMF_LARGEST|
  MEMF_PUBLIC)` / 1024, `numtext` = "     \0", `numtoascii` (lower word only!), tail
  `bra pstats` at `statsloc+38`. destroys d0–d2, a0, a1, a3, a6.
- `alloc2` (3033) : internal (1381, 1872, 1899, 3353, 3809 `makeline`). in: d0 = size,
  d1 = flags, **a6 = ExecBase**. `memalt` = −1, `AllocMem`. out: d0 ≠ 0, Z=0. Failure →
  `beq outmem` (does not return).
- `outmem` (3038) : **never returns**. `doalert`; `filehand` ≠ 0 → `Close` (without
  clearing `filehand`); `curstop` = `cursy` = 0, `thisitem` = `firstitem`, `showall`;
  **`move.l memstack,a7` + `bra memcont`** (longjmp into the main loop, discards all
  frames in between, also those of blitz2). Targets: 1154, 1406, 1779, 2894, 3036, 3054,
  3498, 3620 (`openreq`).
- `extalloc` (3049) : comdata+88 (`doallocmem`, blitz2.s:3379, ~10 callers). in: d0, d1,
  **a6 = ExecBase** (caller sets `move.l 4.w,a6`). Success → `rts` with d0, Z=0.
  Failure → `closereq` (only topmost requester window, harmless if none) +
  `bra outmem` – **leaves the compiler in the middle of the run**.
- `doalert` (3057) : tail `jmp DisplayAlert(0 = RECOVERY_ALERT, alertstuff, 40)`.

---------------------------------------------------------------------------------------------
## B. Persistent register conventions

- Handlers from `jumptable`/`menutable`: a4 = `thisitem` on entry (`help` reads
  `chars(a4)`, `mousedown` scrolls a4 along and saves/restores it). d7 = event code.
- Inside `mousedown`: d2 = current column (**absolute**, cursoff already added) or
  screen column directly after `getmousexy`; d3 = screen row; d5/d6 = last
  end position (same coordinates); `invflag` = selection lies before the anchor;
  stack frame (from top): a4.l, cursoff.w, curstop.w, bbotx/bboty.l, btopx/btopy.l.
  `invertarea` takes (d0,d1)–(d2,d3) with x = column in the character bitmap (chcols wide),
  y = screen row.
- a6 = ExecBase as entry condition for `alloc2`/`extalloc`; `calldos`/`callint` macros
  load a6 themselves (`dos`/`int`). Many routines leave a6 = int/dos.
- `memstack` = a7 in the main loop dispatch; `outmem` resets to it.
- comdata routines of this scope (`openio`, `closeio`, `tokone`, `tokall`, `detokallz`,
  `refgads`, `actgad`, `extalloc`) save **no** registers (no `movem`); the only
  `movem` saving all registers: `doclean` (callback to the outside), `remember2`
  (d0/d1/a1/a2), `makemice` (a1). blitz2 asm calls `extalloc` with a6 = Exec and expects
  only d0 afterwards.

---------------------------------------------------------------------------------------------
## C. Globals (defined in this scope)

| Name | Line | Type | Init | Purpose |
|---|---|---|---|---|
| `newctop` | 2318 | w | 0 | curstop on release (mousedown) |
| `newcoff` | 2319 | w | 0 | cursoff on release |
| `subs` | 2322 | Label | – | marker, unused |
| `gotoreq` … `gotoreqga3` | 2324–2407 | Intuition data | – | goto requester (see below) |
| `gotoint` | 2394 | l,l | 0 | = StringInfo.LongInt (+28) / AltKeyMap (+32) of `gotoreqsg1` |
| `gotoreqla8` | 2395 | 6 B | 0 | buffer of the string gadget |
| `ioname` | 2446 | 3 B | "NEW" | prefix, **without 0**, runs on into `ioname2` |
| `ioname2` | 2447 | 12 B | "CON:0/0/640/" | without 0, runs on into `hiteasc` |
| `hiteasc` | 2448 | str | "   /Blitz 2 CLI Window" | height (3 characters) is patched |
| `oldih`/`oldoh` | 2451/2452 | l | 0 | saved pr_CIS/pr_COS |
| `myhandle` | 2453 | l | 0 | console FileHandle |
| `bstx` | 2455 | w | −1 | F1 block start x (−1 = none; `blokdis` sets −1) |
| `bsty` | 2456 | w | 0 | F1 block start y |
| `nullcom` | 2614 | l | 0 | empty string for `Execute` |
| `helptoke` | 2767 | w | 0 | last token no. for help |
| `helpex` | 2768 | str | "blitz2:help " | **without 0**, runs on into `helphex` |
| `helphex` | 2769 | 5 B | "1234",0 | hex token no., patched |

Goto requester (Intuition structures, only as template for `openreq`):
- `gotoreq`: `struct Requester` (112 B): Left/Top 140/58, 184×40; ReqGadget = `gotoreqga1`,
  ReqBorder = `gotoreqbo1`, ReqText = `gotoreqin1`.
- Borders (12 B each + coordinates 3 pairs): `bo1`→`la2`→`bo2`→`la5` (frame 184×40, 3D),
  `bo3`→`la10` (OK frame 40×12), `bo4`→`la14` (CANCEL 72×12). The `gotoreqlaN` are partly
  coordinate fields, partly borders themselves (`la2`, `la5`, `la10`, `la14`).
- IntuiTexts: `in1` "LINE TO GOTO:" (12,6), `in2` " OK ", `in3` " CANCEL ".
- Gadgets (44 B): `ga1` string (Type 4, Activation $805 = LONGINT|ENDGADGET|RELVERIFY,
  124,6, 48×8, ID 1, SpecialInfo `gotoreqsg1`) → `ga2` bool (ID 2, OK) → `ga3` bool (ID 3).
- `gotoreqsg1`: `struct StringInfo` (36 B): Buffer `gotoreqla8`, UndoBuffer `undobuffer`
  (7105), MaxChars 6; LongInt = `gotoint`.
- Runtime write accesses: `gotoline` → sg1+8 (BufferPos), sg1+12 (DispPos), `gotoint`,
  `gotoreqla8[0]`. `openreq` (3605ff) → `UserData` (+40, word) of the first gadget = −1
  as "already moved", and **once** LeftEdge(+4) += 6, TopEdge(+6) += 12 of all
  gadgets of the chain. Intuition writes Buffer/LongInt/Flags.

Used from outside and important here: `nummice` (6163, **`dc.l`, but only used as a
word** = upper word), `mice` (6164, head pointer), `mousealt` (6165, w), `memalt`,
`memstack`, `filehand`, `tokenise`, `bloktop*/blokbot*/btop*/bbot*` (6085–6092,
adjacent, read pairwise as `.l`), `invflag`, `numtext` (6 B), `newsctit` (32+80 B),
`filepath` (192 B, preceded by sentinel `' :'`), `filename` (64 B).

---------------------------------------------------------------------------------------------
## D. Self-modification / data in the code

- No patches of instructions. No entries into `incbin` blocks in this scope.
- Written data in the code section: `newctop/newcoff`, goto requester (see C, also
  by `openreq` and Intuition), `hiteasc` (height), `oldih/oldoh/myhandle`, `bstx/bsty`,
  `helphex`, `helptoke`; `menu2.NextMenu` (6554, by `getstuff`),
  `defaultreqga8+12` (gadget flags, `getstuff`).
- String concatenation by adjacency: `ioname`+`ioname2`+`hiteasc` yields
  "NEWCON:0/0/640/hhh/Blitz 2 CLI Window", `ioname2` alone "CON:…";
  `helpex`+`helphex` = "blitz2:help XXXX". In C: `sprintf`/constants.
- `findio` reads DOS-internal structures directly (DosInfo/DevInfo, BPTR/BSTR).

---------------------------------------------------------------------------------------------
## E. Heap/list structures

- **Mice node** (8 B, `AllocMem(8, MEMF_PUBLIC)`, `remember2`): +0 `next.l` (for the
  last one **uninitialised**), +4 `line.l` (pointer to line item). Head `mice`, count
  `nummice` (word in the upper part of the `dc.l`). All traversals count via `nummice`,
  except `showmice` (reads `next` of the last node once, see I). Rebuilt completely on every
  change (`mousealt`).
- **Line item** (lines 16–20): +0 `nextitem.l`, +4 `lastitem.l`, +8 `numchars.b`
  (length **incl.** 0 terminator), +9 `chars[]` (0-terminated, tokens = 2 bytes with
  bit 7 in the first). `lenitem` = 9. Newly created by `makeline` (3803) from `charbuff`.
- **COMPILER menu** (`getstuff`, MEMF_PUBLIC, never freed, not checked for NULL):
  `struct Menu` 30 B (copy of `menu2`): NextMenu = 0, LeftEdge = 248, Width = (strlen+1)*8,
  MenuName = pointer into `menitems` (blitz2 memory), FirstItem chain from +18.
  Per entry 54 B (copy of `menu2item1` 34 B + IntuiText 20 B): NextItem 0, TopEdge = 12*i,
  Width 200, Flags (+12) ITEMTEXT|ITEMENABLED|HIGHCOMP, COMMSEQ (4) only with shortcut,
  ItemFill (+18) = own IntuiText (+34), Command (+26) = shortcut, IText (+46) = pointer into
  `menitems`.
- `charbuff`: work buffer of the current line; overwritten by `tokall`, `detokallz2` and
  **`showmice`** (with `tokenise`).

---------------------------------------------------------------------------------------------
## F. External calls

| Library | LVO | Where / what for |
|---|---|---|
| exec | FindTask(0) | `openio`, `closeio` (pr_CIS @156, pr_COS @160) |
| exec | AllocMem | `remember2` (8, PUBLIC), `getstuff` (30/54, PUBLIC), `alloc2`, `extalloc` |
| exec | FreeMem | `freemice` |
| exec | AvailMem(LARGEST\|PUBLIC) | `printmem` |
| dos | Delay(8) | `actgad` (let the window become active) |
| dos | Open(…, 1006 = MODE_NEWFILE) / Close | `openio` / `closeio`, `outmem` (`filehand`) |
| dos | Execute | `mycli` ("", h, 0), `help` ("blitz2:help XXXX", 0, 0) |
| intuition | ActivateGadget, RefreshGList | `actgad`, `refgads` |
| intuition | CloseWorkBench, OpenWorkBench, WBenchToFront | `closewb`, `openwb`, `openio` |
| intuition | ScreenToFront, ActivateWindow | `openwb`, `closeio` |
| intuition | ShowTitle | `help` |
| intuition | ClearMenuStrip, SetMenuStrip | `getstuff` |
| intuition | DisplayAlert | `doalert` |
| intuition | DisplayBeep | indirectly via `beepem` (1054) |

- Direct structure accesses: DOSBase `dl_Root` (34) → DosInfo (24) → DevInfo (4/40);
  GfxBase `DisplayFlags` (206/207); Screen MouseY/MouseX (16/18, in `getmousexy`).
- Callback to blitz2: `cleanup` (edstruct+20) in `doclean` – 0 with blitz2.
- Called by blitz2: comdata+16 `detokallz`, +20 `tokall`, +28 `refgads`, +56 `tokone`,
  +60 `openio`, +64 `closeio`, +68 `actgad`, +88 `extalloc`.
- Timing: `mousedown` polls without waiting (`getinput2`) – scroll speed while dragging
  = CPU speed. `actgad`: 8 ticks delay.
- No hardware access, no interrupts. Output into the bitplanes via `qprint`,
  `invertarea` (other chunks).

---------------------------------------------------------------------------------------------
## G. Special handling in C

- **`outmem` = longjmp**: `memstack`/`memcont` as `setjmp` in the dispatch of `mainedit`;
  `outmem` must also work from inside blitz2 code (via `extalloc`) → discards
  blitz2 frames. In the C port: `longjmp` across the `callregs`/REGSTUB boundary (blitz2-c
  `glue.c`); blitz2 state (compiler flags, open files, `Forbid`?) stays
  inconsistent as in the original. Before the first main loop `memstack` = 0.
- comdata entries need register stubs (glue): `openio` (Z flag return, for C
  better: return handle ≠ 0), `extalloc` (d0/d1/a6 in, d0 out), `tokone` (a0),
  `refgads` (a0,d0), `actgad` (a0 → d0), `tokall`/`detokallz` (no parameters).
- Condition code returns: `findio` (Z=1 found), `openio` (Z, see I), `alloc2`/`extalloc`
  (Z=0), `getmousexy` (N).
- Tail calls galore (`beepem`, `showall`, `cursor2`, `gotolined0`, `showfname`, `closeio`,
  `detokallc`, `tokit2`, `pstats`, Intuition `jmp`); fall-throughs `tokall`→`tokall2`,
  `detokallz`→`detokallz2`, `detokall`→`detokall2`→`detokallc`.
- `mousedown`: manual stack frame with `addq #8,a7` discarding and two different
  pop paths → in C local variables (saved btop/bbot, curstop/cursoff/thisitem).
- Word pairs as long word: `move.l bloktopx,btopx` (x+y), `move.l blokbotx,bbotx`,
  `cmp.l` of (x:y) for equality, `move.l cursx,-(a7)` in `showprops2` → in C structure
  `{WORD x, y;}` or copy both fields; do not change the order of the variables 6085–6092
  in case other chunks use `.l` as well.
- `nummice` is `dc.l`, but only used word-wise (upper word, big endian) → in C
  `UWORD nummice` (the lower word is dead space; check whether `.l` is read anywhere).
- `tokenise`: values 0/1 (`getstuff`) and $FFFF (`tokall`: `not`); only ≠ 0 counts –
  except in 553 (`cmp tokenise,6(a0)`): there comparison with the gadget value → C: reproduce
  values exactly.
- `numtoascii` works only on the lower word (`printmem` > 64 MB, `openio`).
- Unsigned comparisons (`bcs/bcc/bhi`) vs. signed (`blt/bgt/ble`) mixed in `mousedown`/`sortblox`/
  `orderem` (column −1 possible!) – take over exactly.

### H. Classification

| Routine | Class | Remark |
|---|---|---|
| `gomouse` | UI | finding line index from line pointer would be KERN (helper function `line_index(item)`) |
| `mousedown`, `oneloop` | UI | block coordinate normalisation (`.mouseup`: blokbotx<0/≥chcols) separable as KERN helper |
| `gotoline` | UI | check 1…numlines KERN-trivial |
| `actgad`, `refgads` | UI | comdata stub needed (GLUE) |
| `blockstart`, `blockend` | KERN + hook | coordinate logic KERN; `blokdis`/`bloken`/`showall`/`beepem` as UI hooks |
| `closewb`, `openwb` | UI | |
| `findio` | UI (OS-internal) | DOS structures; in C `LockDosList`/`FindDosEntry` or 1:1 |
| `calciorows`, `openio`, `closeio`, `mycli` | UI | `openio`/`closeio` comdata → GLUE stub |
| `tokone` | KERN | GLUE stub for comdata; in-place tokeniser |
| `tokall`/`tokall2`, `detokallz`/`detokallz2`, `detokall*`, `detokallc` | KERN + hook | line loop KERN; `beepem`, `calcitem`+`showall` as redraw hook; `makeline` calls `remember` (flag only) |
| `help` | KERN + UI | token search at column (name/help text/token no.) KERN, host-testable; `ShowTitle`/`Execute` UI |
| `doclean` | GLUE | callback to blitz2 with register saving |
| `getstuff` | UI | parsing of `menitems` KERN-capable |
| `makemice`, `freemice`, `remember`, `remember2`, `mforget2` | KERN | list + AllocMem; `outmem` as error hook |
| `showmice` | UI | label extraction (text up to `:`/space, max. 12) separable as KERN helper |
| `a0toname` | KERN | pure string logic (reproduce or mark the bug) |
| `printmem` | UI | `AvailMem` + status line |
| `alloc2` | KERN | AllocMem abstraction; failure → `outmem` (longjmp) |
| `outmem` | GLUE | longjmp + UI (alert, showall) |
| `extalloc` | GLUE | comdata stub, does not return on failure |
| `doalert` | UI | |

---------------------------------------------------------------------------------------------
## I. Peculiarities / possible bugs of the original

1. `a0toname` (2996ff): `cmp.b #':',d0` / `cmp.b #'/',d0` test **d0** instead of the copied
   character (`move.b (a0)+,(a1)+` does not load d0). Normally harmless (filepath = "",
   filename = everything, sentinel `' :'` before `filepath`). But d0 on entry =
   return value of `LoadSeg("blitz2:defaultdbug")` (blitz2.s:1441): if its
   low byte happens to be $3A (':') or $2F ('/'), the file name is destroyed (filename = ""
   or only the last character). BUG(orig).
2. `blockend` (2472): `move d0,btopx` with undefined d0, before anything at all
   is computed; in the path "start = end" the garbage stays in `btopx` (but the block is then
   off via `blokdis`). Without effect, but reproduce/mark.
3. `blockend`: cursor at (0,0) as end → `bbotx` = −1 (no line wrap special case
   for line 0). Likewise end x = 0 with y > 0 → end = (chcols−1, y−1).
4. `remember2`: `next` of the new node not initialised (MEMF_PUBLIC without CLEAR).
   `showmice` reads after the last node `move.l (a4),a4` (a4 = garbage) and on the
   next pass again `(a4)` from a **garbage address** (only protected by `cmp #0,a4`)
   → possible address error (odd address) on 68000. BUG(orig).
5. `outmem` closes `filehand`, but does not clear it → a later `outmem` without a new
   `doopen` closes the same handle again.
6. `outmem` before the first main loop (`memstack` = 0): e.g. `extalloc` from blitz2 init
   (`makeiallox`) or `openreq` failure at the version requester → `a7 = 0` → crash.
7. `extalloc` → `outmem` in the middle of a compiler run: blitz2 frames are discarded, memory/
   files of the compiler are left dangling.
8. `openio`: success return Z depends on the flags after `WBenchToFront`; `mycli`
   relies on it (`beq .skip`). Moreover: height = editor screen height, although the
   window opens on the Workbench screen (interlace editor on non-interlace WB →
   Open can fail).
9. `closeio`: `Close(myhandle)` even if `openio` failed (handle 0); critical under 1.3.
   `findio` without Forbid/LockDosList.
10. `gotoline`: `gotoint` (long) compared against `numlines` only word-wise; 65537 → line 1.
    `lea gotoreq,a2` before `actgad` without effect (actgad sets a2 = 0).
11. `mousedown`: scrolling up while dragging marked by the author as "currently FUCKED";
    after release the **scroll position before dragging** is restored
    (`curstop/cursoff/thisitem` from the stack), the display jumps back. Duplicate,
    never satisfied test `curstop+edrows ≥ numlines` (2049–2052 / 2055–2058). Busy polling without delay.
    Labels `.loop2`, `.ok`, `.ok2` unused.
12. Mouse click without dragging with an existing block: `blokdis` sets `bstx` = −1 → a start mark
    set via F1 is lost.
13. `help`: shows the last token that begins **before or at** the cursor column (even
    if the cursor stands further right on text). Unknown token → `findtoke` returns
    `tokeerr`, a2 then points behind this string (garbage as help text). `newsctit`
    (112 B) is filled with name+help without length check.
14. `detokallz2`: no check whether the detokenised line fits into `charbuff` or
    stays < 255 characters (`numchars` is a byte). `detokallz` (comdata+16) leaves
    `thisitem` on the last line and does not redraw.
15. `tokone` tokenises in-place (a1 = a2) – only correct as long as the output never becomes longer
    than the input.
16. `getstuff`: `AllocMem` without NULL check, menu memory is never freed.
17. `showmice`: result of `screenclose`/`screenopen2` not checked; overwrites
    `charbuff` (detokenising the label lines).
18. `printmem`: value > 65535 KB is truncated by `numtoascii` (lower word).
19. `makemice`: `move d1,mousealt` sets the flag to the label character (word value with
    undefined upper byte) – only ≠ 0 counts, harmless.
