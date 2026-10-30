# Architecture of Blitz Basic 2: blitz2, TED and the way to the executable

> As of 26.10.2026. Audience: developers who compare BB2 with AmiBlitz3
> and know 68k/AmigaOS, but not the internals of BB2.
>
> Basis: compiler `blitz2.s` v1.95 and editor `ted.s` (TED 1.1) from
> `archive/sources/blitz2-1.95/`, `archive/sources/ted-1.1/`, the C translation
> [`programs/blitz2/`](blitz2/bb2-readme.md) (same labels, line references per
> function), the libraries of the CD (`archive/cd/ultimate-blitz-basic-2.1/BLITZ2/ACIDLIBS`, `DEFLIBS`) and
> [`libs/`](libs/bb2-readme.md). The binaries of the CD are v2.10; the
> reference run over all 724 CD sources gave the same programs with v1.95 and v2.10
> except for one assembler encoding (bb2 `compilerlauf/README.md`, not taken over).
>
> Evidence: **[S]** from source (`blitz2.s:ZEILE label`, `ted.s:ZEILE label`;
> the C function is given as `datei.c funktion()`), **[B]** from binaries
> or a test run in Amiberry (example in section 6),
> **[Schluss]** inferred. What was neither read nor tried out is marked as
> "not checked".
>
> References to the AmiBlitz3 Compiler-Spec (`AmiBlitz3/Docs/Compiler-Spec/`)
> are given as "Spec 09 §9.2" etc.; the BigLib format is fully described there in chapter 09
> and only summarised here.

## Contents

1. [Overview](#1-overview)
2. [Process structure: who starts whom](#2-process-structure-who-starts-whom)
3. [Interface TED ↔ blitz2](#3-interface-ted--blitz2)
4. [Kinds of libraries](#4-kinds-of-libraries)
5. [Compilation flow and linking](#5-compilation-flow-and-linking)
6. [Example: three lines to the hunk](#6-example-three-lines-to-the-hunk)
7. [Notes for the comparison with AmiBlitz3](#7-notes-for-the-comparison-with-amiblitz3)
8. [Open points](#8-open-points)

---

## 1 Overview

Blitz 2 consists of a compiler program (`blitz2`) that loads the editor
(`ted`) as a code segment. The editor is the user interface, the
compiler runs only when the editor calls it via a menu item. There is
no separate linker and there are no object files: the compiler generates the
machine code directly in a memory buffer, copies the required
libraries along and writes the buffer as an executable file with **one**
code hunk – or runs it directly in memory.

```
  blitz2:  (assign, all files in archive/cd/ultimate-blitz-basic-2.1/BLITZ2/)
  ├── blitz2        compiler, started from CLI/WB                    86 KB
  ├── ted           editor, loaded by blitz2 via LoadSeg             59 KB
  ├── acidlibs      BigLib: command libs + system libs              219 KB
  ├── deflibs       BigLib: OS call libs (Open_ …) + third-party libs 55 KB
  ├── userlibs/     further libs, one per subdirectory
  └── defaultdbug   debugger UI, loaded by blitz2 via LoadSeg        32 KB

  Quelle.bb2 (tokenised)  +  Quelle.bb2.xtra (options, residents)
  *.res  (residents: precompiled NewTypes, macros, constants)
```

---

## 2 Process structure: who starts whom

### 2.1 blitz2 is the main program, TED a loaded segment [S]

What is started is **blitz2** (CLI or Workbench). blitz2 loads TED with
`LoadSeg("blitz2:ted")` and calls its init entry point; after that it hands
control to the main loop of TED and only gets it back
when the user leaves the editor. Everything runs in the **process of
blitz2** – TED is not a task of its own, but code in the same process.

```
CLI/WB
  │
  ▼
blitz2  wbheader/mainbit (blitz2.s:253)          main.c main()
  │     actualbegin/begin (blitz2.s:1353)        main.c begin()
  │       LoadSeg("blitz2:ted")
  │       jsr BADDR(seg)+8  ──────────────►  TED outside (ted.s:24)
  │          a1 = &edstruct                     copies edstruct (28 bytes),
  │                                             opens screen etc.
  │       ◄──────────── a0 = .tomain, a1 = &comdata, Z=1
  │       load libraries, allocate buffers,
  │       version requester (via comdata)
  │       LoadSeg("blitz2:defaultdbug")
  │       jsr (a0) with a0 = file name  ───►  TED .tomain (ted.s:87)
  │                                             editor main loop
  │                       ◄─── menu (d6,a5,d7)  menu "COMPILER"
  │       compile / savefile / Run ...  ───►    (return d0 = line)
  │                                             …
  │       ◄──────────── rts when leaving the editor
  │       clean up, UnLoadSeg, end
  ▼
```

* The hunk of TED begins with `bsr edstart` (`ted.s:21`): so `ted`
  also runs alone as an editor (without compiler). blitz2 calls `BADDR(seg)+8`, i.e.
  hunk offset 4 = `outside` (`ted.s:24`), and thereby skips this
  first instruction. `outside` recognises the compiler by a1 ≠ 0 (`external`).
* TED answers with a0 = main entry `.tomain`, a1 = `comdata`
  (table of the editor services, section 3.2) and Z=1 (`moveq #0,d0`).
* C port: `callregs()` in `glue.c` loads the registers, calls and reads them
  back; the callbacks of the editor go via register stubs
  (`REGSTUB(menu)`, `loadxtra`, `savextra`, `stopcode` in `glue.c`).

### 2.2 Start in detail [S]

| Step | blitz2.s | C port | Contents |
|---|---|---|---|
| 1 | `wbheader` (include), `mainbit` 253 | `main.c main()` | CLI: stack size from `4(a7)`, file name from the argument line; WB: WBStartup message, stack size from `.info`, file name from argument 2 → `progfile`, `stackfuck` |
| 2 | `begin` 1357 | `main.c begin()` | walk the token list, remember `lasttoken` (this is where the libs append their tokens), `makeasmtab` (table of the 119 assembler mnemonics), open `mathffp.library` |
| 3 | 1376–1390 | | buffers for constant evaluation (`constpcat`, 1024 bytes) and macro buffer (`macrolen`, default 8192) |
| 4 | 1392 | | `LoadSeg("blitz2:ted")`, init call (2.1) |
| 5 | 1405–1416 | | `int` = IntuitionBase of the editor (`comdata+44`), version requester via `comdata+0`; `makelibs` (sizes of `deflibs`/`acidlibs`, list of `userlibs/`), `loadlibs` (4.7), `makeiallox` (object buffer, 5.2) |
| 6 | 1418–1440 | | wait for "OKEE DOKEE" (`getinput`), signal bit for the PANIC mechanism (`AllocSignal`), `LoadSeg("blitz2:defaultdbug")` → `dseg` |
| 7 | 1441 | | `jsr (a0)` = editor main loop with a0 = `progfile` |

The `.xtra` file is **not** read by blitz2 at startup, but by TED when loading
a source, via the callback `loadxtra` (3.4). Residents are loaded
along with it.

### 2.3 End [S]

When the editor main loop returns (`blitz2.s:1448 .skip`):
`FreeSignal`, `UnLoadSeg(dseg)`, `freeemup` (compiler lists), `freeres`
(residents), `freelibs`, then the failure chain `fail3` … `fail0`
(`UnLoadSeg` TED, object buffer, macro buffer, dos, constant buffer,
mathffp), `flushlibs` (AllocMem of more than 256 MB, so that Exec removes unused
libraries). BUG(orig): the return value set in `returncode`
is lost in `wb_exit` (d0 is overwritten) – survey-01.

### 2.4 What runs in which task [S]

| Code | Task | Remark |
|---|---|---|
| TED, compiler, `savefile` | process of blitz2 | the compiler runs only within the callbacks `menu`, `loadxtra`, `savextra` |
| Compiled program on "RUN"/"COMPILE AND RUN" | own process "Blitz ][ Program Proc" (`CreateProc`, `blitz2.s:1605 menu10`) | code runs directly in the object buffer of the compiler |
| `directtrap`/`directdo` (direct mode of the debugger, evaluate expression) | in the **program** process | the debugger triggers `trap #15`, the compiler compiles the input in the context of the program (3.7) |
| `stopcode` | Exec exception of the blitz2 process | STOP/PANIC button (3.8) |
| Constant evaluation `evalconst2` | process of blitz2 | generated code is executed **in the compiler** (5.4) |

---

## 3 Interface TED ↔ blitz2

The interface has two directions: `edstruct` (blitz2 → TED, passed at init;
also contains the callbacks into the compiler) and `comdata` (TED →
blitz2, table of the editor services).

### 3.1 `edstruct` [S]

`blitz2.s:274 edstruct`, C: `main.c edstruct`. TED copies the 28 bytes into
its variables from `tokens` on (`ted.s:6269`).

| Offset | Contents | Use in TED |
|---|---|---|
| 0 | pointer to the token list (`tokens`, `blitz2.s:283`) | tokenizer `tokit` (`ted.s:4198`), detokenizer, help |
| 4 | menu texts (`blitz2.s:724 menus`): `"COMPILER"`, 10 entries of 23 characters + shortcut | menu "COMPILER" |
| 8 | `loadxtra` (`blitz2.s:15345`) | after loading a source |
| 12 | `savextra` (`blitz2.s:15412`) | after saving |
| 16 | `menu` (`blitz2.s:1498`) | every item of the menu "COMPILER" |
| 20 | 0 (cleanup, unused) | |
| 24 | characters `:` `"` `.` `;` | separator, string, label, comment (syntax colours/tokenizer) |

The token list is a linked list `{next.l, num.w, name\0, help\0,
even}` – first the 211 built-in words, then (appended by `fixlibs`)
the name blocks of all libraries. TED, debugger and `debuglib` walk it
directly; the layout must therefore be byte-exact.

### 3.2 `comdata`: services of the editor [S]

`ted.s:6030 comdata`; C wrappers `ed_*()` in `main.c`, offsets `CD_*` in
`main.h`. Call always `move.l comdata,aX / move.l off(aX),aX / jsr (aX)`.

| Off. | TED | Purpose for blitz2 |
|---|---|---|
| −8/−4 | `screenclose`/`screenopen2` | close/open the editor screen (`CloseEd`) |
| 0/4 | `openreq`/`closereq` | open/close a "requester" (Intuition requester structures serve only as a template, TED builds a window from them) |
| 8 | `getinput` | wait; d7 = −1 and d6 = gadget ID on a gadget event |
| 12 | `error` (`ted.s:3511`) | show error message a0 (modal) |
| 16/20 | `detokallz`/`tokall` | de-/tokenise the whole source (not used by v1.95) |
| 24 | `ezfreq` | file requester: a0 path, a1 name, a2 title → d0, Z=1 on cancel |
| 28 | `refgads` | redraw gadgets |
| 32 | `getinput2` | like 8, without waiting (d7 = 0: nothing) |
| 36…52 | data | bitplane for the progress bar, screen, IntuitionBase, colours |
| 56 | `tokone` (`ted.s:2636`) | tokenise one line (direct mode, calculator) |
| 60/64 | `openio`/`closeio` (`ted.s:2548`) | input/output window for the running program |
| 68 | `actgad` | activate string gadget |
| 72/76 | `pointeron`/`sleepon` | mouse pointer normal/busy |
| 80 | data `mywindow2` | editor window |
| 88 | `extalloc` (`ted.s:3049`) | **AllocMem** for the compiler (`doallocmem`, `blitz2.s:3379`); on failure TED jumps into its out-of-memory handling and does not return |
| 92/94 | data | bytes per bitplane row, requester window (its UserPort signal for STOP, 3.8) |

### 3.3 The menu callback `menu` [S]

For an item of the menu "COMPILER", TED calls `extmenu` (`ted.s:1542`):

```
d6 = menu item (0 …)         a5 = firstitem (first source line)
d7 = number of lines         jsr menu   (blitz2.s:1498, C: main.c menu_c())
→ d0.w = -1  cursor stays
         -2  redraw everything
         n   cursor to line n (error line, counted from 0)
```

`menu` stores `firstitem`/`numlines`, sets `menuret = -1` and jumps
via the table `menuadds` (`blitz2.s:741`):

| d6 | Menu item | Routine (blitz2.s) | C |
|---|---|---|---|
| 0 | COMPILE AND RUN | `menu0` 1595 → `compile`, then `menu10` | `main.c menu0()` |
| 1 | RUN | `menu10` 1605 | `main.c menu10()` |
| 2 | CREATE EXECUTABLE | `menu1` 1556 → `compile`, `savefile` | `main.c menu1()` |
| 3 | COMPILER OPTIONS | `menu4` 15456 | `data.c menu4()` |
| 4 | CREATE RESIDENT | `menu5` 4105 | `stmts.c menu5()` |
| 5 | VIEW NEWTYPES | `menu6` 15165 | `data.c menu6()` |
| 6 | CLI ARGUMENT | `menu9` 15979 | `data.c menu9()` |
| 7 | CALCULATOR | `menu7` 15739 | `data.c menu7()` |
| 8 | RELOAD ALL LIBS | `menu8` 1778 | `main.c menu8()` |
| 9 | LOAD DEBUG MODULE | `menu_d` 1518 | `main.c menu_d()` |

### 3.4 Loading and saving `.xtra` [S]

After loading a source, TED opens `NAME.xtra` and calls `loadxtra` with
**d7 = file handle, a6 = DOSBase** (`ted.s:710`), after saving
correspondingly `savextra` (`ted.s:795`). `loadxtra` (`blitz2.s:15345`,
C: `data.c loadxtra_c()`) reads the options directly into the
gadget structures of the options requester (the flags of the gadgets *are* the
option storage), the CLI argument, the maximum values per object library,
reallocates the object buffer (`makeiallox`) and loads the residents
(`loadres`, 4.5). Format of the file: [formats.md](formats.md).
The format as one structure with loader and writers, meant for blitz2
and TED once they are reworked (not used by them yet): `shared/xtra`
([formats.md](formats.md) §1.4). The caller does the I/O (source and
companion `NAME.xtra` into memory, `xtra_detect`, `xtra_load`); the
structure (300 bytes on the m68k) holds the numbers and views (offsets)
into the `.xtra` buffer and the kept source header, so it need not be a
global of blitz2.

### 3.5 Source lines: TED passes its line storage on directly [S]

There is no handing over of a copy: a5 points to the first line of the
doubly linked line list of TED (`ted.s:15 rsreset`):

```
+0 nextitem.l   +4 lastitem.l   +8 numchars.b   +9 chars[] … $00
```

TED keeps the lines **tokenised** in memory: when a line is left,
`tokit` (`ted.s:4198`) runs over it. The compiler reads from `+9` up to the
zero byte (`blitz2.s:2038 nextline`, `compile.c nextline()`):

* ASCII characters < `$80` unchanged (names, numbers, strings, operators).
* A byte ≥ `$80` starts a **token word** (big-endian, 2 bytes):
  * `$8001`–`$80D3` built-in words (bb2 `builtin_tokens.txt`, not taken over), among them
    `$803C`–`$80B2` the assembler mnemonics.
  * `$8000 | (lib·128 + i)` commands of the libraries, e.g. `Print` = `$DD81`
    (lib 187, entry 1), `Left$` = `$DC81` (lib 185/1). The compiler
    distinguishes lib tokens by `(token & $7F00) ≠ 0` (`blitz2.s:3105
    handletoken`).
* Lines are at most 127 characters long; tabs become spaces.

The tokenizer is rebuilt in bb2 `compilerlauf/tools/bb2tok.py`
(peculiarities in its header; bb2 only, not taken over).

### 3.6 Errors back to TED [S]

Every error location calls `jsr err` with the text as an inline string
(`blitz2.s:7641 err`, C: `parse.c err()`). `err`:

1. sets `anyerrs = -1` and **`menuret = linenumat`** (line number from 0),
2. closes the progress requester (`endstop`),
3. shows the text via `comdata+12`,
4. jumps back: `a7 = errstack`, `jmp (errcont)` – a longjmp. When
   compiling, `errcont = freeemup` (`compile` sets it, `blitz2.s:1917`),
   i.e. the whole compilation is aborted and cleaned up.

`menu` returns `menuret` to TED, TED puts the cursor on the line
(`ted.s:1542 extmenu` → `gotolined0`). **Only one** error is reported. Lines from
`INCLUDE` files do not increase `linenumat` (`nextline` reads them via
`readinc` and skips the counting); an error in an
include file therefore lands on the `INCLUDE` line [Schluss from the
code, not tried out].

### 3.7 Run, Create Executable and debugger

**CREATE EXECUTABLE** (`menu1`) [S]: file requester, `makeexec = -1`,
option "Make smallest code" forced on (section 5.1), `compile`,
on success `savefile` (5.10).

**RUN / COMPILE AND RUN** (`menu0`/`menu10`) [S]:

```
blitz2 process                              child process "Blitz ][ Program Proc"
menu10 (1605)
  STOP requester "EXECUTING"/"!PANIC!"
  CreateProc(procstart, stack=stackfuck) ──► runtheprog (1688, main.c runtheprog())
  Wait(panicmask)                              comdata+60 openio (console of TED)
     │                                         write the VBR table:
     │                                           trap #1 vector  = &tokens   (mailbox)
     │                                           trap #2 vector  = firsttype (mailbox)
     │                                           trap #15 vector = directtrap
     │                                         d2 = stack, d7 = 'bLtZ', a0 = CLI arg.
     │                                         jsr pcat   ← program in the object buffer
     │                                         comdata+64 closeio
     │◄──────────── Signal(us, panicmask) ──── stopit = 1, end of the process
  stopit = -1 (PANIC)? → RemTask(child)
```

So the program runs **without relocation at the address at which it was
compiled** (the object buffer contains absolute addresses, 5.2).
`procstart` is a fake segment list in the data area of blitz2
(`blitz2.s:1683`).

**Debugger** [S, lib part from libs/acidlibs]: if the option "Runtime Errors"
(`optreqga10`, internally `debugga`) is on, `dopass` additionally links in:

| Target | Libs | Function |
|---|---|---|
| RUN from the editor | `runerrlib` 65500, `debuglib` 64060, `dhandlerlib` 1 | full debugger runtime: `debuglib` takes over TRAP #0–#5 (reading the old trap #1 vector = token list in the process), `dhandlerlib` gets the address of `dseg` (loaded `defaultdbug`) in its init and connects the runtime with this UI |
| CREATE EXECUTABLE | `runerrlib` 65500, `rundebuglib` 64050 | minimal: TRAP #0 → AutoRequest with error text and program end; TRAP #1/#2/#4 only skip their parameters |

For this the compiler generates:

* per statement a **debug block** (`blitz2.s:2871 debugstuff`,
  `compile.c debugstuff()`): `trap #1`, then `dc.l` address of the TED line,
  `dc.w` offset of the statement in the line, `dc.w` distance to the next
  block (linked, last = 0). The line address points into the memory
  of TED – that is why executables with runtime checking are not byte-identical between two
  runs (bb2 `compilerlauf/README.md`, not taken over).
* `trap #4`/`#5` at procedure start/end, `trap #2`/`#3` around
  interrupt routines.
* check calls before or after lib routines (`fehler1`/`fehler2` from `!subs`,
  5.5) and the error check routines of the libs (area from `error` on, 4.6).
* at the program end `finalscheck` (`$C300`) and `moveq #3,d0 : trap #0`
  (`blitz2.s:2683 eopcode`) – in executables without `End` this makes
  an empty requester appear (bb2 `compilerlauf/tedstub/README.md`, not taken over).

Lib code reports runtime errors with `move.l #text,d0 : trap #0`
(`BLITZ_RUNERR` in `libs/include/blitzlib.h`).

**Direct mode, `trap #15`** [S]: evaluating/executing in the debugger
(`debuglib` `doevaluate`/`doexecute`) triggers `trap #15` in the child process with
18 bytes of parameters (line, locals, globals, result, Blitz mode).
`directtrap` (`blitz2.s:3805`, assembler in `glue_direct.c`) bends the
return address to `directdo` (3819, `stmts.c directdo_c()`): the
**compiler** now runs in the program process, tokenises the input via
`comdata+56`, compiles it with `doline` behind the program (`dirpcat`),
appends `jmp ret15add` and jumps into it. Errors go back to the program as text via
`errcont = dircont`.

### 3.8 STOP and PANIC (`stopcode`) [S]

During compiling ("COMPILING"/"STOP!") and while the program
runs ("EXECUTING"/"!PANIC!") a requester is open. `openstopreq`
(`blitz2.s:2526`) installs `stopcode` as the **Exec exception handler** of the
blitz2 process (`tc_ExceptCode`, `tc_SigExcept` = signal bit of the
UserPort of the requester window). When a message arrives at this window,
`stopcode` (`blitz2.s:2453`, `compile.c stopcode_c()`) sets `stopit = -1`
and signals `panicmask`.

* When compiling, `chkstop` (`blitz2.s:2847`) checks before every statement;
  `chkrealstop` asks via `getinput2` whether it really was the STOP button,
  then aborts like an error.
* When executing, the signal wakes `menu10`; on a real PANIC the
  child process is removed with `RemTask` **without cleanup**.
* blitz2 does not evaluate Ctrl-C. In `rundebuglib` there is a flag
  `ctrlc` that is never set (BUG(orig), `libs/acidlibs/rundebuglib.c`).

### 3.9 What of this is really needed: tedstub [B]

`tests/corpus/amiga/tedstub.c` replaces TED without a screen and compiles
all 724 CD sources with it. It shows the minimum: init entry at hunk+4,
`comdata` with meaningful `openreq`/`getinput`/`error`/`ezfreq`/`extalloc`
and data (`combitmap`, window with UserPort), line list as in TED,
`loadxtra` with d7/a6 and `menu` with d6 = 2. All other entries may
be empty (table `struct comdata_s` in [`shared/edstub/edstub.c`](../shared/edstub/edstub.c)).

---

## 4 Kinds of libraries

### 4.1 Overview

| Kind | Where | Numbers | Tokens? | How used | Goes into the program |
|---|---|---|---|---|---|
| **Built-in words** | in `blitz2` (`blitz2.s:283 tokens`) | lib 0 | `$8001`–`$80D3` | statement handler `atokens` (`blitz2.s:1298`) | only the generated code |
| **Command libraries** (BlitzLibs) | `acidlibs`, `deflibs`, `userlibs/` | 1–255 | `$8000 \| lib·128+i` | command/function in the source → `handlelib` | whole lib |
| **System libs** | `acidlibs` | 64040–65534 (as word, "negative": −1496…−2) | no names; "token" `$C000`–`$D5xx` internal | by the compiler (`tokejsr`) or by other libs (`ALibJsr`) | whole lib |
| **OS call libs** | `deflibs` | 200, 219–255 | e.g. `Open_` = lib 252 | entry with kind bit 2 → `stamigalib`: load registers, `jsr lvo(a6)` | only init/finit (OpenLibrary/CloseLibrary) |
| **Residents** | `*.res`, entered in the `.xtra` | – | – | NewTypes, macros, constants known in advance | nothing (declarations only) |

Being a "system lib" depends only on the number, not on the file:
`acidlibs` contains 57 command libraries **and** the 25 system libs as well as lib 1
(`dhandlerlib`) [B, bb2 `biglib.py list`]. `deflibs` of the CD contains 39 libs:
lib 2 and the OS call libs 200, 219–255 [B].

### 4.2 Built-in words [S]

59 statements and keywords (`NEWTYPE`, `End`, `If`, `For`, `Data`,
`Macro`, `CNIF` …, numbers 1–59), 118 assembler mnemonics + `EndAsm`
(60–178, for the inline assembler, table `asmmac`), a further 33 words
(`List`, `SetErr`, `Repeat`, `BitTst` … 179–211). Everything that is not
here – so also `Print`, `Left$`, `Str$` – comes from libraries.
Overview: bb2 `builtin_tokens.txt` (not taken over), [formats.md](formats.md).

### 4.3 Command libraries [S/B]

Normal BlitzLibs with name blocks; `fixlibs` appends their tokens to the
token list of blitz2 so that TED can tokenise them. Example:
`printlib` 187 (`Print`, `NPrint`, `Str$`, `Format` …),
`stringfunclib` 185 (`Left$` …). Object libs (screens, shapes …) additionally have
a `!libfin` extension with maximum and object size (4.6).

### 4.4 System libs [S/B]

Runtime code without names that the compiler links in itself – via
"token" numbers from `blitz2.s:13–251` (list in survey-01 C), e.g.:

| Lib | Number | Examples (token) |
|---|---|---|
| `varslib` | 65534 | allocate globals (`allocvars`), init sets a5 |
| `memlib` | 65530 | `localloc $C000` … memory management |
| `strings1lib` | 65235 | `copstring $C600` (copy string into the work buffer) |
| `strings2lib` | 65135 | `lastring $C700`/`astring $C701` (assign string variable) |
| `exitslib` | 65035 | `endjmp $C800` (`End`) |
| `ffplib` | 64935 | base of mathffp.library |
| `ldivlib`/`lmullib` | 64735/64835 | `longdiv $CB01`, `longmult $CA01` |
| `datalib` | 64535 | `Read`/`Restore` |
| `runerrlib`, `debuglib`, `rundebuglib` | 65500, 64060, 64050 | runtime checking, debugger (3.7) |

All 25 are rebuilt as C in [libs/acidlibs/](libs/bb2-readme.md) and checked against the
originals.

### 4.5 OS call libs (deflibs) [S/B]

A lib like `doslib` 252 contains practically only declarations: entries of
"kind 6" with base lib, LVO and register bytes, e.g. `Open_` = lib 252,
LVO −30, parameters in d1, d2 [B, bb2 `biglib.py dump`]. It has code only for
init (open the library, return the base) and finit (close): of the
5756 bytes of `doslib`, 44 end up in the program (section 6). A call
`Open_(n$,1005)` is compiled in `stamigalib` (`blitz2.s:3879`,
`stmts.c stamigalib()`): arguments onto the stack, fetch them back into the
registers, `move.l base(a5),a6 : jsr lvo(a6)`.
Other libs request such bases via `!libs` (e.g. `inputoutputlib`
the DOS base), that is why even a program without a `…_` command can contain a lib from
`deflibs`.

### 4.6 BigLib format (short version of Spec 09) [S/B]

```
BigLib file (acidlibs, deflibs) = { size.l, lib executable } … 0.l
Lib executable = HUNK_HEADER (32 bytes) | HUNK_CODE = lib image | HUNK_RELOC32 | HUNK_END
userlibs/…/file = one lib executable without size.l

Lib image:
+$00  moveq #0,d0 : rts        in memory: next pointer
+$04  lib number.w
+$0A  (compiler: a5 offset of the init return, −1 = not linked in)
+$0C  flags (bit 7 used, bit 6 allowed in constants)
+$0E  first token, +$10 last+1 (system libs; normal ones: by the compiler)
+$12  (compiler: offset of code start)
+$16  init  +$1A return (0/1 long/−1 word)  +$1C finit
+$20/+$24 (compiler: relocs)   +$28 (compiler: address in the program)
+$2C  error: from here error check code (copied only with debugger)
+$30  token entries: !astatement/!afunction/!acommand …
        forms: !args/!repargs  !libs{lib,reg…}  !subs{code,err1,err2}  !name
      !nullsub (routine without name)  !dumtoke (name only)
      OS entries (kind bit 2)
      !libfin  (−1.w, 0.l | object lib: toke,load,save,use,free,0,max,shift)
      code … [error: check routines]
```

Details (kind values, register codes `#ld0`/`#la6`/`#lpush`/`#pd0`,
repetition groups) are in Spec 09 §9.2 and in the macros
`archive/cd/ultimate-blitz-basic-2.1/EXTRAS/BLITZLIBS/LIBMACS.BB` or their C reproduction
`libs/include/blitzlib.h`. Additionally important for linking:

* **Inline routines:** if a routine begins with `$A000,len`, the
  compiler copies `len` bytes to the call site instead of `jsr`; `$A001,adr.l`
  selects a separate version for Blitz mode (`blitz2.s:11541 doajsr2`,
  Spec 09 §9.3.2).
* **`ALibJsr token[,form]`** in lib code: `jsr` with the placeholder
  `($8000|form)<<16 | token` (`BLibJsr`: `$C000|form`) and a relocation.
  When linking in, the compiler recognises the relocation by the set bit 15 of the
  target word (5.6).
* **Relocations:** only HUNK_RELOC32 into the own hunk; offsets < `$12`
  (header) are ignored, those in the error check area without debugger
  likewise.

### 4.7 Loading the libraries [S]

`loadlibs` (`blitz2.s:6374`, `asmlibs.c loadlibs()`) reads at startup
`acidlibs`, then `deflibs`, each in one piece (`loadlibgroup`), after that every
enabled file under `userlibs/<verzeichnis>/` (`loadblitzlibs`,
`loadalib`). `fixlibs` (6525) chains everything in `firstlib`, sets for
normal libs `first token = lib·128+1`, links the entries and appends
the name blocks to the token list (`lasttoken`). There is **no**
replacement of equal lib numbers as in AB3 (Spec 09 §9.1.1) – `findlib`
takes the first match [Schluss].

Note: bb2's `bb2tok.py` looks up names in the order deflibs before acidlibs;
v1.95 loads acidlibs first. For the tokens of the CD this makes no
difference (no duplicate names); 2.10 loads in the same order as
v1.95 (`docs/v2.10/blitz2.md`).

### 4.8 Residents [S]

A `.res` is an executable (loaded with LoadSeg) whose hunk contains precompiled
NewTypes, macros and constants – no code. Created
with "CREATE RESIDENT" (`blitz2.s:4105 menu5`), up to 8 of them loaded from
the list in the `.xtra` (`blitz2.s:4393 loadres`, `stmts.c loadres()`),
hooked into the lists `firsttype`/`firstmacro`/`firstconst`
(`fixtypes`/`fixmacs`/`fixconsts`), then checked for equal names
("Clash in Residents"). Format: [formats.md](formats.md) (same as AB3,
Spec 06 §6.1.1).

---

## 5 Compilation flow and linking

### 5.1 Passes [S]

`compile` (`blitz2.s:1917`, `compile.c compile()`):

```
compile
 ├─ dopass                         ← pass 1 with the buffer sizes from the options
 ├─ [option "Make smallest code"]: alloxadj → shrink buffers to the used size,
 │                                   dopass once more
 └─ [buffer overflowed (nomemleft)]: requester "Not Enough Object Memory":
        1 = +50 % and again, 2 = smallest, 3 = abort
```

`dopass` compiles the **whole program in one pass**: labels used before
their definition are collected as a reference list and filled in at the definition
(`blitz2.s:3011 handlelab2`). The second run of "Make
smallest" is needed because the addresses change with the buffer size
(all addresses in the code are absolute, 5.2). For CREATE EXECUTABLE
"smallest" is always on; in the example the log shows "COMPILING" twice [B].

### 5.2 The object buffer [S]

`makeallox` (`blitz2.s:3988`, `stmts.c makeallox()`) gets **one**
memory block (MEMF_CLEAR) and divides it into four areas, each with its own
write pointer:

```
pcat ──► program code      (pc)     option "Object Buffer" (16384)
libat ─► library code      (lib)    option "Libs Buffer"   (32768)
data1at► system data       (data1)  option "System Buffer"  (4096): string literals, tables
data2at► DATA statements   (data2)  option "Data Buffer"    (4096)
allat
```

All addresses that the compiler generates (jumps, `jsr` to lib code,
literals) are **absolute addresses in this buffer**. Every such location
goes into the reloc list `firstoff` (`blitz2.s:6997 addoff`). Therefore
RUN can execute the buffer directly, and `savefile` converts the addresses into hunk offsets when
writing (5.10). An overflow does not abort, but
sets `nomemleft` and stops writing (5.1).

### 5.3 Lines and statements [S]

```
dopass (2556)                  compile.c dopass()
 ├ setcvars, varoff=$8000, resetlibs, 4×NOP at pcat (room for jsr inits)
 ├ [debugga] uselib runerrlib + debuglib/dhandlerlib or rundebuglib
 └ passloop: nextline (2038) → doline (2780) → dostatement (2953)
      chkstop · expandmax (macros !name{…}) · debugstuff (trap #1)
      handlechar (2961):
        '.' / name without rest → label             handlelabel/handlelab2
        '#name=…'             → constant          handleconst
        Name{…}               → procedure call    handleproc
        Name …                → assignment (LET)  dolet2 (tokens2.c)
        token < $8000+60      → atokens[n] (built-in statement)
        token 60…178          → asmit (inline assembler)
        token with lib bits   → handlelib (3169) → dolibtoke (3181)
```

Statement handlers are jumped to via the jump table `atokens` (92 entries,
`blitz2.s:1298`) with the next character in d0. `CNIF`/`CSIF`
(conditional compilation) and `` ` `` macro parameters are already handled by `getchar`
(`blitz2.s:7042`) and `expandmax` (2121) respectively.

### 5.4 Expressions and code generation [S]

* `eval` (`blitz2.s:8857`, `eval.c eval()`) is an operator precedence
  parser with its own operator/precedence stack; it generates code directly into
  data register `D(regat)` (from d0 upwards; if they are not enough, via the
  stack). Types: 1 byte, 2 word, 3 long, 4 quick (16.16), 5 float (FFP),
  7 string; the default type is quick.
* Code is produced from **templates** (machine code snippets in the data area
  of the compiler, `blitz2.s:914 objects` … `1254`, also in `tokens1`/
  `tokens2`), which are copied (`pokecode`) or written as an opcode word with
  inserted registers/offsets (`pokewd`/`pokel`). In the
  C port they are `UWORD` arrays with the original bytes.
* Arithmetic that the 68000 cannot do directly (long division/multiplication,
  FFP, string comparison, MOD …) becomes a `jsr` to a system lib routine
  (`tokejsr`, `blitz2.s:11434`).
* **Constant expressions** (constants, `CNIF`, array bounds …) are evaluated by
  `evalconst2` (`blitz2.s:11880`, `libcall.c evalconst2()`) by generating
  code into a separate buffer and **executing it in the compiler**
  (`cfetchmode`: `jsr` directly into the lib code in the compiler's memory). That is why
  blitz2 only runs on a 68k.
* There is no optimiser across statements; sequences like
  `move.l d0,-(a7) : move.l (a7)+,d0` stay in place (section 6).

### 5.5 A library command is compiled [S]

For statements `handlelib` → `dolibtoke`, for functions
`libfunction` (`blitz2.s:10112`, `eval.c libfunction()`) → `dolibtoke`:

1. `findtoke` (`blitz2.s:6277`) looks up lib and entry, marks the lib
   as used (`uselib3`, 5.6).
2. `countpees` counts the arguments; `dolibtoke` chooses the **form** with
   a matching count (overloading, repetition groups of `!repargs`).
3. `fetchlibps` (`blitz2.s:5563`) evaluates the arguments: the first into
   d0, the next into d1 etc. (`regat`); arguments with `#push` and
   repetition groups go onto the stack; with `#unknown` (type 8) the
   type is passed along as a word. `numreps` remembers the number of repetitions.
4. **String arguments:** the value in the register is the address of the characters
   in the string work buffer (a3 advances during construction; `maineval2` terminates every string argument with `clr.b (a3)+`, `sbtolong`, a3 is behind the 0 afterwards), the **length** lies
   as a long on the stack. If the call has stack arguments (`stackuse` > 0),
   `dolibtoke` generates `lea stackuse(a7),a2` before it (template `preplibst`,
   `blitz2.s:1088`): a2 points above the stacked values, the routine reads
   them with `-(a2)`. With repetition groups `move.w #numreps,d7`
   (`movenumrep`) is added.
5. `userjsr` → `makelsub3` (`blitz2.s:11474`, `libcall.c makelsub3()`):
   * `fetchregs`/`toreg` (11647) generate, for every `!libs` pair, the loading
     of a value of another lib, mostly `move.l base(a5),a6` (base from
     the init result, a5-relative) or an object pointer from
     object number and table; `uselib` links the lib in in the process.
   * with debugger: `jsr fehler1` (check routine before).
   * `doajsr2` (11541): copy inline code or `jsr abs.l` to
     `address of the lib in the program + offset` (with reloc); `$A001` prefix for
     Blitz/Amiga mode.
   * with debugger: `jsr fehler2`.
6. After that `addq #n,a7` or `lea stackuse(a7),a7` (`fixlibst`) cleans up the
   stack.

The result is in d0; a string result is written by the routine from a3 on,
afterwards a3 points to the zero byte and d0 contains the length
(`libs/acidlibs/stringfunclib.c`, header).

Built-in commands `ALibJsr`/`BLibJsr`/`TokeJsr`/`SysJsr` (lib sources,
inline assembler) generate the same placeholders or calls directly
(`blitz2.s:3571 dolibjsr`, 3599 `dotokejsr`).

### 5.6 Which library routines go into the program [S]

**Always the whole library.** `uselib3` (`blitz2.s:5841`,
`asmlibs.c uselib3()`) copies, on the first use of a lib, its code
from the code start to the end – without debugger only up to `error` – to the
current `lib` pointer (order = order of first use)
and immediately processes:

* **Relocations:** every longword += target address − code start, offset into
  `firstoff`. Relocations whose target is a word with bit 15 are
  `ALibJsr` placeholders: `handlejsrs` (6158) → `maketjsr` (6189) enters
  `(token, form, patch address)` into `tokeslist`.
* **Global variable** for the init result (field `$1A` ≠ 0): next
  free a5 offset, word or long; offset into lib field `$0A`.
* **Object libs:** `maxslib` in addition, room for the object table in the
  globals (size from maximum × 2^shift, maximum from `.xtra`/options).
* **Dependencies:** the lists at the init and finit entries
  (`uselib2`, 6237) are linked in recursively; `!libs` pairs link their
  libs in when a call is compiled (`toreg`).

At the end of the pass `fixjsrs` (6003) resolves the collected
placeholders: `tokecode` (6047) returns the address of the target routine (and links
its lib in if not done yet). If the routine needs
register parameters from `!libs` or check calls, `tokecode` generates a
small stub behind the program (`… jsr routine : rts`); otherwise the
address is entered directly. In front of the stubs there is a `jmp` that
skips them.

### 5.7 Program end, inits and finits, startup code [S/B]

After the last line `dopass` (`blitz2.s:2612`ff.) appends:

```
pcat:   jsr inits ; nop              ← 4 NOPs from dopass, overwritten by makeinits
        <program>
        [debugger: trap #1 block, jsr finalscheck, moveq #3,d0 : trap #0]
endop:  [doamiga, interrupts off]
        jmp über_die_Stubs
        <stubs from fixjsrs>
über:   <finits>                     makefinits (11327): ascending lib number
        moveq #0,d0 : rts            ← program end
inits:  <inits>                      makeinits (11140): descending lib number
        rts
```

* **Startup code** does not exist as a fixed block: the program begins with
  `jsr inits`. The first to run (highest number) is `varslib` 65534: it
  gets the size of the globals in d0, gets memory (`AllocMem`,
  MEMF_CLEAR) and sets **a5 = block + $8000**; so the globals lie at
  negative 16-bit offsets `-32768(a5)` … (the compiler starts `varoff`
  at `$8000`) [B, example].
* Some inits get values in d0 from the compiler (`makeinits`):
  `varslib` size of the globals, `strings1lib` size of the string buffer
  (option "String Buffer", 10240), `exitslib` the address `endop`,
  `datalib` the start of data2, `debuglib` auto-run flag, `dhandlerlib`
  `&dseg`.
* The result of every init routine is written into its global variable
  (`move.l d0,off(a5)`).
* `End` is `jsr endjmp` (`exitslib`, `blitz2.s:13876 doend`): back to the
  saved stack and jump to `endop`, so that the finits also run on
  `End` in the middle of the program [Schluss from init parameter and code,
  routine not disassembled].
* If there is no init code, `makeinits` sets `noinits = 8` and
  `savefile` leaves out the 8 bytes at the start.

### 5.8 Register model at runtime [S/B]

| Register | Meaning |
|---|---|
| a5 | globals (block + $8000); init results of all libs, variables of the main program |
| a4 | locals of the current procedure (`link a4,#-n`, template `linka4`) |
| a3 | write pointer in the string work buffer |
| a2 | pointer above the stacked lib arguments (string lengths) on a lib call |
| a6 | library base for the next call (cached via `lasta6`) |
| d0–d7 | expression evaluation, lib arguments from d0 on; d7 = number of repetition groups |
| a7 | stack (Gosub, For frames, stack arguments) |

The lib routines preserve a3 (changed in a defined way), a4, a5, a7; what else they
may change is not uniform (Spec 09 §9.3, 12).

### 5.9 Runtime error checking on/off [S]

| | without "Runtime Errors" | with |
|---|---|---|
| Lib code | up to `error` | whole (check routines from `error` on) |
| Check calls `fehler1/fehler2` | no | yes |
| `trap #1` block per statement | no | yes (8 bytes + trap; in executables only skipped via `rundebuglib` – still costs one exception per statement) |
| Array/maximum checks, stack check | no | yes (`maxchk`, `chkstak` only on RUN) |
| Additional libs | – | `runerrlib` + `debuglib`/`dhandlerlib` (RUN) or `rundebuglib` (executable) |

In the example: 5500 instead of 7824 bytes file (section 6) [B].

### 5.10 Writing the executable (`savefile`) [S/B]

`savefile` (`blitz2.s:6800`, `parse.c savefile()`):

1. Every reloc location in `firstoff`: longword −= `pcat + noinits`
   (absolute buffer address → hunk offset).
2. Write the file:

```
HUNK_HEADER  $3F3, 0, 1 hunk, 0, 0, size
HUNK_CODE    $3E9, size   (objlen − noinits, rounded to longwords)
             ┌───────────────────────────────┐ 0
             │ jsr inits / nop               │
             │ program, end, stubs, finits,  │
             │ moveq #0,d0 : rts, inits      │
             ├───────────────────────────────┤ libat − pcat
             │ lib code (order of            │
             │ first use)                    │
             ├───────────────────────────────┤ data1at − pcat
             │ data1: string literals        │
             │  (len.l + characters + 0), …  │
             ├───────────────────────────────┤ data2at − pcat
             │ data2: DATA values            │
             └───────────────────────────────┘
HUNK_RELOC32 $3EC, n, hunk 0, n offsets, 0
[HUNK_SYMBOL $3F0: labels, if "Create Debug Info"]
HUNK_END     $3F2
```

There is **no** DATA or BSS hunk: variables and string buffers
are allocated at runtime by `varslib`/`strings1lib`, the data areas
lie in the code hunk. Because of "smallest" the four areas follow each other
without gaps. Optionally `savefile` writes an icon (`obj.info`).

---

## 6 Example: three lines to the hunk

Source (`hallo.bas`), compiled with the original v1.95 in Amiberry (bb2 tool, not taken over):

```
a$="Hallo"
Print Left$(a$,2)
End
```

```
python3 -I compilerlauf/tools/bb2compile.py --compiler 195 --no-runerr --run --tag doc-example hallo.bas
python3 -I compilerlauf/tools/bb2compile.py --compiler 195 --run --tag doc-example-dbg hallo.bas
```

Result [B]: 5500 bytes without, 7824 bytes with runtime checking (the
compiler's default); output of both programs `Ha` (`Print`
without line feed). The log shows two passes ("COMPILING" twice,
"smallest"). Result directories deleted afterwards.

### 6.1 TED lines [B]

```
61 24 3D 22 48 61 6C 6C 6F 22 00         a$="Hallo"
DD 81 20 DC 81 28 61 24 2C 32 29 00      Print Left$(a$,2)     Print=$DD81, Left$=$DC81
80 02 00                                 End                    $8002
```

`$DD81` = `$8000 | 187·128 + 1` (printlib, entry 1),
`$DC81` = `$8000 | 185·128 + 1` (stringfunclib). The space after `Print`
is kept, `a$` stays ASCII.

### 6.2 Hunk structure (without runtime checking) [B]

HUNK_HEADER (1 hunk) · HUNK_CODE 5132 bytes · HUNK_RELOC32 with 79 entries ·
HUNK_END (32 + 5132 + 332 + 4 = 5500 bytes).

| Offset | Contents |
|---|---|
| `$0000`–`$0007` | `jsr $C8` (inits) · `nop` |
| `$0008`–`$0091` | the three lines |
| `$0092`–`$009D` | `End` = `jsr $13EE` (exitslib) · `jmp $9E` (empty stub list) |
| `$009E`–`$00C7` | finits, `moveq #0,d0 : rts` |
| `$00C8`–`$0131` | inits, `rts` |
| `$0132`–`$13FF` | libraries (4814 bytes): |
| | `$0132` varslib · `$0170` strings1lib · `$01C2` memlib · `$0340` strings2lib · `$03C6` printlib (2260 bytes) · `$0C9A` inputoutputlib · `$0EFC` doslib (from **deflibs**) · `$0F28` stringfunclib · `$13A4` ffplib · `$13D2` exitslib |
| `$1400`–`$140B` | data1: `00000005 "Hallo" 00` + padding byte |

Lib boundaries determined by matching against the lib images from `acidlibs`/`deflibs`
(relocs masked). The three lines generate 138 bytes of code, the
libraries 4814 bytes: `Print` pulls in `printlib`, and via its dependencies
`inputoutputlib`, `doslib` and `ffplib`.

### 6.3 Disassembly with explanation [B]

Globals (a5-relative, 32 bytes, `move.l #32,d0` in the init):
`-$8000` varslib · `-$7FFC` **a$** · `-$7FF8` memlib · `-$7FF4`
string buffer (strings1lib) · `-$7FF0` DOSBase · `-$7FEC` stringfunclib ·
`-$7FE8` ffplib (mathffp base) · `-$7FE4` exitslib (`endop`).

```
0000  4EB9 000000C8   jsr     inits                ; reloc
0006  4E71            nop
      ; a$="Hallo"
0008  266D 800C       move.l  -$7FF4(a5),a3        ; a3 = start of string buffer
000C  207C 00001400   move.l  #lit_Hallo,a0        ; data1, reloc
0012  2F18            move.l  (a0)+,-(a7)          ; length 5 onto the stack, a0 = characters
0014  4EB9 0000017A   jsr     copstring            ; strings1lib $C600: to (a3)+
001A  421B            clr.b   (a3)+
001C  202D 800C       move.l  -$7FF4(a5),d0
0020  2640            move.l  d0,a3
0022  45ED 8004       lea     a$(a5),a2            ; target variable
0026  2C6D 8008       move.l  -$7FF8(a5),a6        ; memlib (memory for the string)
002A  4EB9 00000340   jsr     astring              ; strings2lib $C701: global string assignment
      ; Print Left$(a$,2)
0030  266D 800C       move.l  -$7FF4(a5),a3
0034  2F0B            move.l  a3,-(a7)             ; remember address of the substring
0036  45ED 8004       lea     a$(a5),a2
003A  7000 2F00       moveq #0,d0 / move.l d0,-(a7); length 0 in case a$ is empty
003E  4A92 6700000E   tst.l (a2) / beq.w 0050
0044  2052 5948 2E98  move.l (a2),a0 / subq #4,a0 / move.l (a0)+,(a7) ; length is before the characters
004A  4EB9 0000017A   jsr     copstring            ; copy a$ into the buffer
0050  421B            clr.b   (a3)+
0052  202F0004 2E9F   move.l 4(a7),d0 / move.l (a7)+,(a7) ; d0 = address, length stays on the stack
0058  223C 00000002   move.l  #2,d1                ; 2nd argument
005E  45EF 0004       lea     4(a7),a2             ; preplibst: a2 above the string length
0062  4EB9 00000FA2   jsr     Left$                ; stringfunclib, d0 = length of the result
0068  584F            addq.w  #4,a7                ; clean up length
006A  2F00 201F       move.l d0,-(a7) / move.l (a7)+,d0   ; (unoptimised)
006E  7207 3F01       moveq #7,d1 / move.w d1,-(a7)       ; type 7 = string (#unknown argument)
0072  2F00            move.l  d0,-(a7)             ; length
0074  421B            clr.b   (a3)+
0076  202D 800C 2F00  move.l -$7FF4(a5),d0 / move.l d0,-(a7) ; address (result is at the buffer start)
007C  45EF 000A       lea     10(a7),a2            ; preplibst, stackuse = 10
0080  3E3C 0001       move.w  #1,d7                ; movenumrep: 1 repetition
0084  2C6D 8018       move.l  -$7FE8(a5),a6        ; !libs: ffplib base into a6
0088  4EB9 00000764   jsr     Print                ; printlib
008E  4FEF 000A       lea     10(a7),a7            ; fixlibst
      ; End
0092  4EB9 000013EE   jsr     endjmp               ; exitslib $C800
0098  4EF9 0000009E   jmp     $9E                  ; endop: over the (empty) stub list
      ; finits, ascending lib number
009E  226D 8010 4EB9 00000F12   ; doslib 252: CloseLibrary(DOSBase)
00A8  226D 8018 4EB9 000013C2   ; ffplib 64935
00B2  4EB9 000001E0             ; memlib 65530: free all memory
00B8  203C 00000020 4EB9 0000015C ; varslib 65534: free globals
00C4  7000 4E75       moveq #0,d0 : rts            ; program end
      ; inits, descending lib number
00C8  203C 00000020 4EB9 00000132 2B40 8000  ; varslib(32): a5 = AllocMem(32)+$8000
00D8  4EB9 000001C8 2B40 8008                ; memlib
00E2  203C 00002800 4EB9 00000170 2B40 800C  ; strings1lib(10240): string buffer
00F2  203C 00000098 4EB9 000013DA 2B40 801C  ; exitslib(endop = $98)
0102  4EB9 000013A4 2B40 8018                ; ffplib: open mathffp
010C  4EB9 00000EFC 2B40 8010                ; doslib (deflibs): open dos
0116  4EB9 00000434                          ; printlib (no return value)
011C  4EB9 00000F72 2B40 8014                ; stringfunclib
0126  2C6D 8010 4EB9 00000D38                ; inputoutputlib, a6 = DOSBase
0130  4E75            rts
0132  ; varslib: 2F08 223C00010001 2C780004 4EAEFF3A … 2A40 9AFC8000 …
      ;   AllocMem(d0, MEMF_PUBLIC|MEMF_CLEAR); a5 = d0; suba.w #$8000,a5
```

Interpretations of the runtime routines (`copstring`, `astring`, who cleans up the length on
the stack at `$12`) come from the lib sources or from the
context; the routines themselves were not disassembled.

### 6.4 With runtime checking [B]

`runerrlib` and `rundebuglib` are added and are at the front of the lib area
(from `$0186` on, `dopass` links them in before the first line), `exitslib`
follows directly after as a dependency. Every statement begins with a
debug block:

```
0008  4E41            trap #1
000A  40089DF8        dc.l  address of TED line 1 (memory of the editor)
000E  0009            dc.w  offset of the statement (9 = start of chars[])
0010  0032            dc.w  distance to the next block ($3A − $08)
…
00A6  4E41 4007F330 0009 0010   ; line 3 (End)
00AE  4EB9 0000086A             ; jsr endjmp
00B6  4E41 …                    ; block at the program end
00BE  4EB9 000004B4             ; jsr finalscheck ($C300)
00C6  4E41 … 0000               ; last block, distance 0
00CE  7003 4E40                 ; moveq #3,d0 : trap #0 (eopcode)
```

---

## 7 Notes for the comparison with AmiBlitz3

* **Unchanged:** BigLib format, token numbers `lib·128+i`,
  calling convention (d0…, a2 stack for string lengths, a3 string buffer,
  a5/a4), linking in whole libs with init/finit, `ALibJsr` placeholders,
  inline prefixes `$A000`/`$A001`, globals from `-32768(a5)` on
  (Spec 09 §9.3–9.4, Spec 12).
* **One code hunk, absolute addresses in the buffer, execution in
  memory:** the structure "program · finits · inits · libs · data" in
  one hunk is the result of a compiler without a linker.
* **Editor as host UI, compiler as library:** in BB2 the
  process ownership lies with the compiler, the editor is a subroutine; there is
  exactly one error, passed as a line number to the editor.
* **Constant evaluation by executing generated code** – a
  reproduction on another host needs a 68k emulator for this or its
  own evaluation with bit-exact FFP/quick arithmetic.
* **Runtime checking = TRAP per statement**, with line addresses of the editor
  in the code; that is why debug executables are not reproducible.

---

## 8 Open points

* Exact function of `defaultdbug`: only known from `makeinits`. `dhandlerlib`
  (lib 1) is ported and described in `docs/libs/cmdlibs-port.md`: it
  starts the program at `dseg` as the debugger process and connects it
  with debuglib.
* Order `acidlibs`/`deflibs` when loading in v2.10: as in v1.95,
  acidlibs first (checked statically, `docs/v2.10/blitz2.md`).
* `End` → `exitslib` → `endop`: flow inferred from init parameter and code,
  routine not disassembled.
* Errors in include files on the `INCLUDE` line: inferred from `nextline`,
  not tried out.
* Who cleans up the stacked length for `a$="Hallo"` (presumably `astring`)
  – not checked.
