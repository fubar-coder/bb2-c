# TED 1.1 (`ted.s`) – overview for the C port

Source: `archive/sources/ted-1.1/ted.s`
(8072 lines, Devpac, 68000) + includes `myoffs` (LVOs), `wbheader` (startup code,
the same as in blitz2), `aboutreq`, `specialreq` (requester data).
Reference build and binary includes: [reference.md](reference.md).
Cross-references between the chunks: [xrefs.md](xrefs.md) (bb2 `tools/xrefs.py`).
Brief of the surveys: `SURVEY-BRIEF.md` (bb2 only, not taken over). Plan: [PLAN.md](PLAN.md).

## 1 Chunks

| # | Lines | Size | Survey | Contents |
|---|---|---|---|---|
| 01 | 1–1015 + `wbheader` (114), `aboutreq` (113), `specialreq` (138) | ~1380 | [survey-01](survey-01.md) | line `rs` offsets, init entry `outside` (hunk+4) and `.tomain`, standalone start `edstart`, fail chain, `mainedit`/`memcont` (main loop, `jumptable` dispatch), mouse pointer/hourglass with VBlank server, menus on/off, `blocktab`/`blockuntab`, options requester `defaults`, printing `prtfile`, `.xtra`/`.info`, loading/saving (`fileopen*`, `filesave`, `realsave`), About, search/replace |
| 02 | 1016–1980 | 965 | [survey-02](survey-02.md) | `escape` (exit), cursor up/down, `markindent`, insert/delete line, `join`/`split`, `return`, `top`/`bottom`, scroll gadgets, `gotolined0`, `extmenu` (callback into blitz2), `menu` dispatch (`menutable`), `new`, block operations (kill/save/load/copy/forget) |
| 03 | 1981–3062 | 1082 | [survey-03](survey-03.md) | mouse (`gomouse`, `mousedown`: click/drag/scroll), goto requester, `blockstart`/`blockend`, open/close WB, console window `openio`/`closeio` (for running programs), comdata services (`refgads`, `tokone`, `tokall`, `detokallz`, `actgad`), help (token help from the token list), `doclean`, `getstuff` (build COMPILER menu from `menitems`), label column "mice", `outmem`/`alloc2`/`extalloc` |
| 04 | 3063–4197 | 1135 | [survey-04](survey-04.md) | options file `l:blitzeditor.opts`, options requester helpers, file I/O (`doopen`/`doread`/`fillbuff`/`readline`/`allocline`), `detok`/`findtoke`, file requester bridge `ezfreq`, `error`, `openreq`/`closereq` (requester as window), search `isithere2`, line list (`freeupa5`, `makeline`, `calcitem`), block helpers (`getbchar`, `sortblox`, `invertarea`), line output (`showall`, `showa2item`, `tokprint`), `charstobuff` |
| 05 | 4198–5252 | 1055 | [survey-05](survey-05.md) | **tokenizer `tokit`**, `alpha`, scrolling (blitter), allocating/freeing lines (`allocnull`, `freethis`, `freeall`, `donew`), screen (`calcscreen`, `screenopen`, `makecop`, `screenclose`), colours, `getblitter`/`putblitter`, `cls`, horizontal scrolling, own text output (`print*`, `fontcalc`), `calcpos` |
| 06 | 5253–6285 | 1033 | [survey-06](survey-06.md) | cursor, prop gadgets, `refresh`, block boundaries (`blokfix`), **line editor `getline`**, `horizpos`, `getinput`/`handled0` (IDCMP, rawkey via `rawed`), data block: `comdata` (−8…+94), editor globals, options block `defstart` (= file format of the options file), `jumptable`/`menutable`, error texts, `edstruct` copy |
| 07 | 6286–8072 | 1787 (almost only data) | [survey-07](survey-07.md) | NewScreen/NewWindow/gadgets, menu bar (PROJECT/EDIT/SOURCE/SEARCH), requesters `findreq`, `repall`, `errorreq`, `erasereq`, `defaultreq`, `prtreq`, `undobuffer` (only string gadget undo, TED 1.1 has **no** editor undo), `rawed` (incbin), `flink` + `filerequest` (incbin), fonts |

Chunk 07 is large, but 90 % Intuition structures; they can be generated with
an extended version of bb2 `blitz2-c/tools/req2c.py` (survey-07 G; not taken over).

## 2 Architecture of TED in one picture

```
blitz2 (main program)                      TED (LoadSeg segment)
 jsr BADDR(seg)+8 ─ a1=edstruct ─────────► outside (hunk+4): copy 28 bytes of edstruct,
                                           screen/window, rawed init, flink, buffers
 ◄──────── a0=.tomain, a1=comdata, Z=1 ──
 jsr (a0) ─ a0=file name ────────────────► .tomain → load file → mainedit loop:
                                             getline (line editor, chmap = line)
                                             → charstobuff/tokit → makeline
                                             → jumptable[d7] (key/menu/mouse/gadget)
 menu (d6,a5=firstitem,d7=numlines) ◄───── extmenu (menu COMPILER) ── d0 = line/−1/−2
 loadxtra/savextra (d7=fh,a6=dos)   ◄───── fileopen2 / realsave
 cleanup (edstruct+20)              ◄───── doclean (value 0 → not called)
 comdata+0…+94 (openreq, getinput,  ─────► services; some do not return on error
   error, ezfreq, tokone, extalloc …)       (outmem → longjmp to memcont)
```

Central data models:

* **Line list**: doubly linked, `{nextitem.l, lastitem.l, numchars.b
  (length incl. 0), chars[] tokenised, 0}`, AllocMem(`numchars+9`,
  MEMF_PUBLIC). Heads `firstitem`, `enditem`, `thisitem`, `numlines` (word).
  Never empty. blitz2 reads this list directly (layout fixed).
* **chmap**: character map of the screen (`chcols` = 128 bytes per line,
  chip RAM, scrolled with the blitter). The **line currently being edited
  exists only in chmap** (bit 7 = character from a token name); on
  leaving, `charstobuff` → `tokit` → `makeline` turn it back into a
  list node. `join`, `split`, `blokload`, `blokcopy` read/write chmap.
  → chmap belongs in the host core.
* **Cursor/block state**: `cursx/cursy`, `cursoff/curstop`, `scloc/chloc`,
  `btopx/btopy/bbotx/bboty` (`bboty = -1` = no block) – word pairs that the
  code often copies/compares as a longword.
* **Token list** of blitz2 (`{next.l, num.w, name\0, help\0, even}`),
  separators `colon`/`quote`/`label`/`remark` from edstruct (standalone: 255 =
  off, then no quote/remark mode in the tokenizer).
* **Options block** `defstart..defend` (50 bytes) = file format of
  `l:blitzeditor.opts`.

## 3 Cross-section of the surveys

### 3.1 Register and control flow conventions

* Main loop: a4 = `thisitem`, d7 = event code (key 1…$FF, bit 30 =
  umlaut from `germs`; −1 gadget, −2 menu, −3/−4 mouse), d6 = ID. `jumptable`
  (6-byte entries `{code.b, flag.b, fn.l}`) is searched via `cmp.b`
  (−1/−2/−3 = $FF/$FE/$FD – Latin-1 `$FD–$FF` collide, survey-06 I).
* Loading: d7 = FileHandle, a5 = read pointer, d6 = remaining bytes (`doread`).
  Block pass: a5 = item, d6 = line, d5 = column (`getbchar`).
  Output: a2/a3/d2/d3 (`showa2item`). Tokenizer: a1/a2/d1/d5.
* **Non-local returns**: `outmem` (a7 = `memstack`, `jmp memcont` – also
  from within blitz2 via `extalloc`/`openreq`); `addq #4,a7` in `erase`,
  `getfname3`, `doopen` (abort of the caller); `escape` `lea 12(a7)`
  (stack depth dispatch 4+2 + menu 4+2); `pchover` in the tokenizer; `mousedown`
  (`addq #8,a7`); `reline` reads parameters from the stack; `charstobuff`/`tokit`/
  `enddo` share a stack word.
* **Condition codes as result** everywhere (readline three-valued N/Z,
  `getbchar`, `alpha`, `allocnull`, `erase2`, `openio`, `screenopen` …) →
  return values in C.
* blitz2 entry points in comdata partly save nothing (`openreq`, `error` only
  d7, `screenopen2`, `colors`) – the C stubs may save more.

### 3.2 OS and hardware

| Area | Use |
|---|---|
| exec | AllocMem/FreeMem (lines, buffers, menus; chip for pointer/chmap), AddIntServer(VERTB) for the hourglass animation, FindTask/AllocSignal, OpenDevice/CloseDevice (console), GetMsg/ReplyMsg |
| dos | Open/Read/Write/Close, Rename (.bak), Lock/CurrentDir, DeleteFile, Execute (help, CLI), Delay, DOS list (`findio`) |
| intuition | OpenScreen (ExtNewScreen with tags), OpenWindow, menus, requesters **as separate windows** (never `Request()`), NewModifyProp, RefreshGList, SetPointer/ClearPointer, ShowTitle, DisplayAlert, WBenchToFront/CloseWorkBench/OpenWorkBench |
| graphics | OwnBlitter/WaitBlit/DisownBlitter, SetRGB4, UCopList (colour gradient in the title bar), sprites via SetPointer |
| console.device | only `RawKeyConvert` (in `rawed.obj`) |
| Hardware directly | blitter via `$DFF000` (scrolling, cls, chmap), CPU writes glyphs directly into the bitplanes (planar, no RTG), `flash` writes `$DFF180` (unused), `mwait` reads CIA `$BFE001` (dead code) |

### 3.3 Self-modification and data in the code

* No patching of instructions. But: `flink` relocates the embedded
  `filereq` hunk at runtime (once, without cache flush); `openreq`
  shifts the gadgets of every requester on first opening (+6/+12) and
  remembers this in the UserData of the first gadget – **also for requesters of
  blitz2**.
* Much data lies PC-relative between the code and is written to
  (Intuition structures, flags, buffers) – in C normal, writable statics.
* Fixed strides across structure boundaries (84/86/44/56/28/54 bytes,
  survey-07 G.1) and alias labels in the middle of structures (`int1..3`, `errorpnt`,
  `viewmode`, `winscreen`, `idcmp`).
* Sentinel word `' :'` before `filepath`; word pairs as longword; `bloktopx`
  as word, byte buffer and pointer; `nummice` `dc.l`, used as word.

### 3.4 Classification (summary of the H tables)

| Class | Routines (selection) |
|---|---|
| CORE (host-testable) | tokenizer `tokit*`/`alpha`/`mytoke`/`tokone`, `detok`/`findtoke`/`detoka5`/`charstobuff`, help search, loading `readline`/`doread`/`allocline`/`linka0`, write loop `realsave`, `.bak`/`.xtra`/`.info` logic, options file, line list (`allocnull`, `makeline`, `freeupa5`, `freethis`, `freeall`, `calcitem`), core parts of `insertline`/`deleteline`/`join`/`split`/`return`/`gotolined0`, block logic (`blokadj`, `sortblox`, `orderem`, `inbloks`, `blokfix`, `getbchar`, `bloksetup`, `blokcopy`, `bloksave`/`blokload`, `blocktab`/`blockuntab`, `markindent`), search/replace (`isithere2`, `dosearch`, `replace2`), line buffer operations of `getline`, `horizpos`/cursor arithmetic, `mice` list, `a0toname`, print formatting, special key mapping of `rawed`, `germs`, dispatch tables |
| UI | screen/window/menus/requesters, drawing (`print*`, `showall`, scroll blits, `cursor`, `invertarea`), props, mouse, IDCMP (`getinput`/`handled0`), console window, pointer |
| GLUE (asm stub) | hunk start + `outside`/`.tomain`, all comdata entry points (register convention), callbacks to blitz2 (`menurout`, `loadxtra`, `savextra`, `cleanup`), VBlank server `myint`, `outmem` longjmp across foreign frames |

All surveys find: the core routines change the data first and
draw afterwards – the output can be split off as hook calls at the end.
Exceptions (interleaved): `return` (scroll with temporarily changed
`edrows`/`pixrows`), `freeonly`/`allocfirst` (`cls`), `gadget`/`mousedown`
(polling loops), `getline` (drawing after every key press).

## 4 Risks

1. **`outmem` as longjmp across blitz2** (comdata `extalloc`,
   `openreq`): discards compiler frames (also those of blitz2-c, incl. its
   `errjmp` chain). Before the first main loop `memstack = 0` → crash.
   Must be solved in C exactly like this (or deliberately as a `DIFF`).
2. **Register interface to two compilers**: TED-C must run with blitz2 1.95
   (asm, vasm build) **and** blitz2-c; every stub loads its own a4
   (`-fbaserel`), callbacks to blitz2 via `callregs`.
3. **Hunk layout**: hunk+4 = `outside`, no libnix startup code before it; own
   standalone start (wbheader replacement) → `-nostartfiles`, set library bases
   oneself.
4. **Interrupt code** (`myroutine`, VBlank): glue with a4 loading, `volatile`,
   Z flag return; writes into chip RAM sprite data.
5. **Hardware/timing**: direct blitter (`$DFF000`) between
   OwnBlitter/DisownBlitter, CPU drawing directly into planar bitplanes,
   chmap in chip RAM and also processed by the blitter. Runs only on native
   chipset screens (no RTG) – test in the emulator only that way. Busy polls
   (`gadget`, `mousedown` via `getinput2`) and `Delay`.
6. **Rawkey/console**: `rawed` delivers F keys/cursor/Help as own codes
   (`$81–$8B`, `$1C–$1F`), qualifier separately, umlauts via `germs` with bit 30;
   `OpenDevice` errors undetected; `cmp.b` dispatch turns Latin-1 `$FD–$FF`
   into mouse/menu/gadget. Emulator tests must generate real RAWKEY IntuiMessages with
   qualifier.
7. **Stack tricks** (`addq #4,a7`, `lea 12(a7)`, `reline` parameters,
   `charstobuff`↔`enddo`) → return status in C; danger of forgetting abort paths
   (list of the affected callers in survey-01/02/04 G).
8. **Byte/word aliasing** and big-endian assumptions (word pairs as long,
   gadget flags `+13`, `comdata+84` points to a flags low byte) – in the
   host build the core must use **none** of these aliasings.
9. **Original bugs with memory consequences** (must be kept as `BUG(orig)`,
   but disturb sanitizers on the host): `join` frees with the wrong
   size (`+lenitem` missing), `detoka5` writes past `charbuff3`
   (end of allocation), reading via NULL in `bloksave`/`gd03`, tokenizer reads
   past the end of line, `tabstop = 0` → division by zero. Host tests
   need a mode that detects these spots instead of crashing.
10. **File requester**: reusing `libs/acidlibs/freqlib.c` requires
    that the requester core there can be cleanly separated from the lib shell;
    its known overflow with tall windows affects TED just the same.

## 5 Most important BUG(orig) candidates (selection, details in the surveys, section I)

* `join` FreeMem without `lenitem` (confirmed in the reference listing, `$1FE0`) – survey-02.
* `blokfixb` compares `btopy` instead of `bbotx` with `chcols` – survey-06.
* `readline`: last line without LF never tokenised; Latin-1 byte before LF
  swallows the line break; too long a line ends `blokload` – survey-04.
* `blockuntab` deletes non-spaces; `tabstop = 0` – survey-01/06.
* `a0toname` compares d0 instead of the character – survey-03.
* `screenopen` error paths call CloseScreen with a6 = ExecBase;
  `freethis`/`freeall` size via `add.b` – survey-05.
* `int3` starts with 4, buffer shows "2" (tabstop) – survey-07.
* `outside` failure still reports success to blitz2 (Z=1) – survey-01.
