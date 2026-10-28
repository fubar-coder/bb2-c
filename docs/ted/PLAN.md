# Plan: TED 1.1 (`ted.s`) → C

Grundlage: [overview.md](overview.md), [survey-01 … 07](overview.md#1-chunks),
[reference.md](reference.md), [xrefs.md](xrefs.md). Verfahren wie beim
Compiler-Port (`blitz2-c/CONVENTIONS.md`): lesbares C, **Treue vor Eleganz**,
originale Labelnamen als C-Bezeichner, `/* ted.s:NNN label */` an jeder
Funktion, `BUG(orig)` / `DIFF:`-Marker, Inline-Asm nur in `src/glue*.c`,
m68k-amigaos-gcc (`/opt/amiga`), 68000, libnix, `-fbaserel`.

Neu gegenüber blitz2-c: Der **Kern** (Zeilenspeicher, Tokenizer/Detokenizer,
Laden/Speichern, Blockoperationen, Suchen/Ersetzen, Zeilenpuffer-Logik des
Zeileneditors) wird so geschrieben, dass er **auch nativ auf dem Host**
übersetzt und dort gegen `bb2tok.py` und die CD-Quellen getestet werden kann.

Referenz für alle Prüfungen ist der **vasm-Build von `ted.s` und sein Listing**
(`build/ref`, reference.md §1); ein Abgleich mit dem CD-Binary ist nicht Teil
dieses Plans.

---

## 1 Grundsätze

1. **Eine Routine, ein Ort.** Jede Routine wird genau einmal übersetzt. Ist
   sie laut Survey-Tabelle H `KERN`, kommt sie in die Kern-Datei ihres Chunks
   (`src/core/`), sonst in die Amiga-Datei des Chunks (`src/`).
   Mischroutinen (`KERN+UI`) werden **an der in H genannten Stelle** geteilt:
   Kernteil in `core/` (gleicher Labelname), die Ausgabe danach als Aufruf
   eines Hooks (§3.3). Die Reihenfolge „erst Daten ändern, dann zeichnen“
   bleibt dabei erhalten.
2. **Kein Byte-Aliasing im Kern.** Wortpaare (`cursx/cursy`, `btopx/btopy` …),
   die das Original als Langwort kopiert/vergleicht, werden ausgeschrieben
   (beide Felder). Token-Wörter immer `(hi << 8) | lo` aus Bytes. Strukturen
   werden nur über Felder angesprochen, nie über Offsets – auf dem Amiga
   sichern `_Static_assert(offsetof(...))` das Layout, auf dem Host ist es egal.
3. **Globals bleiben Globals** (wie im Original, Namen gleich), Register-
   Zustände über mehrere Aufrufe werden Globals oder kleine Structs (survey-04
   G.4: Lesezustand `d7/a5/d6`, Blockcursor `a5/d6/d5`). Condition-Code-
   Ergebnisse werden Rückgabewerte; die Zuordnung steht im Header-Kommentar
   (`/* asm: N=1 EOF, Z=1 no EOL */`).
4. **Nicht-lokale Rücksprünge**: `addq #4,a7`-Abbrüche (`erase`, `getfname3`,
   `doopen`, …) → `BOOL`-Rückgabe, jeder Aufrufer prüft und kehrt zurück;
   `escape` → Rückgabe „Editor beenden“ an `mainedit`; Tokenizer-Ausstieg
   (`pchover`) → `goto`/Rückgabe; `outmem` → `longjmp` (§4).

## 2 Datei-Layout

```
ted-c/
  CMakeLists.txt, CMakePresets.json (amiga, host), cmake/m68k-amigaos.cmake, .clangd
  tools/   tedlisting.py, xrefs.py               (vorhanden; Referenz-Build: CMake-Ziel ref)
           req2c.py      Requester-Daten → C (Fork von blitz2-c/tools/req2c.py, survey-07 G)
           checkhunk.py  Hunk+4 = outside (wie compilerlauf/tedstub)
           checkdata.py  C-Daten gegen das Listing (§6 b)
  src/core/            Kern: kein AmigaOS-Header, baut auch auf dem Host
    tedtypes.h         UBYTE/UWORD/… (Amiga: <exec/types.h>, Host: <stdint.h>)
    tedcore.h          Umbrella des Kerns: struct item, struct tokennode, Konstanten
    hooks.h            alles, was der Kern von außen braucht (§3.3)
    state.c/.h         Editor-Globals (ted.s:6057–6120, 6267–6282), chmap/charbuff*
    loadsave.c/.h      Chunk 01: Lade-/Schreibschleife, .bak/.xtra/.info, Suchen/Ersetzen, blocktab, Druckformat
    lines.c/.h         Chunk 02: insert/delete/join/split/return-Kern, gotolined0-Arithmetik, Blockregeln, blokload/bloksave, menu-Lookup
    services.c/.h      Chunk 03: tokall/detokall-Schleifen, tokone, help-Suche, mice-Liste, a0toname, alloc2
    fileio.c/.h        Chunk 04: doread/fillbuff/readline/allocline/linka0, detok/findtoke/detoka5, isithere2, freeupa5/makeline/calcitem, getbchar/sortblox, charstobuff, numtoascii
    tokit.c/.h         Chunk 05: tokit*/putchar/alpha, allocnull/freethis/freeall, goup/godown/calcpos/storepos, chmap-Scroll als memmove
    lineedit.c/.h      Chunk 06: Zeilenpuffer-Operationen von getline, horizpos/cursleft/cursrite-Logik, inbloks/blokfix, jumptable/menutable, germs
    keymap.c/.h        Chunk 07: Sondertasten-Mapping aus rawed (+$10A), Fontdaten-Zugriff
    res.c              #embed der Originaldateien: sleep, pointer, srcinfo, font6, font8
  src/                 Amiga-Teil (UI, OS, Glue)
    ted.h              Umbrella: tedcore.h + structs.h + glue.h + alle Chunk-Header
    structs.h          comdata/mydata-Tabelle, edstruct-Kopie, Optionsblock (50 Bytes), OFFS-Asserts
    glue.h             struct m68kregs, REGSTUB, GLUE_LOAD_A4, callregs (aus blitz2-c übernommen)
    glue_start.c       Hunk-Anfang, outside/.tomain, Standalone-Start, comdata-Stubs, VBlank-Stub, outmem
    main.c/.h          Chunk 01 (UI-Teil)       edit.c/.h   Chunk 02
    mouse.c/.h         Chunk 03                 reqs.c/.h   Chunk 04
    screen.c/.h        Chunk 05                 input.c/.h  Chunk 06
    data.c/.h          Chunk 07: Screen/Window/Menüs/Requester (teils generiert)
    rawkey.c/.h        rawed.obj in C (console.device)
    filereq.c/.h       Dateirequester: Kern aus libs-c/libs/freqlib.c, TED-Schnittstelle ezfreq
  tests/host/          test_*.c + Hilfen (Tokenliste aus den BB2.1-Tabellen, Stubs für hooks.h)
  notes/               reference, overview, survey-NN, xrefs, PLAN, (Phase 2:) changes.md
```

Die Kern-Dateien sind nach Chunk geschnitten (nicht nach Thema), damit
parallele Übersetzer nie dieselbe Datei bearbeiten; ihre Namen nennen das
Hauptthema. Querverweise zwischen Chunks laufen nur über Header
(`notes/xrefs.md` = Pflichtinhalt jedes Chunk-Headers, wie bei blitz2-c).
`state.c`/`structs.h`/`glue*`/`hooks.h`/`tedcore.h` gehören dem Koordinator
(Änderungen wie in `blitz2-c/CONVENTIONS.md` §9 mit `notes/changes.md`).

## 3 Gemeinsame Header

### 3.1 `core/tedcore.h`

```c
/* ted.s:15 rsreset - one source line; blitz2 reads this list directly (a5) */
struct item {
    struct item *nextitem;      /* +0 */
    struct item *lastitem;      /* +4 */
    UBYTE numchars;             /* +8 length incl. the 0 byte; FreeMem size = numchars + lenitem */
    UBYTE chars[];              /* +9 tokenised text, 0-terminated */
};
#define lenitem 9               /* asm constant, NOT sizeof(struct item) on the host */

/* blitz2 token list (edstruct+0): {next.l, num.w, name\0, help\0, even} */
struct tokennode { struct tokennode *next; UWORD num; char name[]; };
```

Auf dem Amiga: `_Static_assert(offsetof(struct item, chars) == 9)` usw.
(gcc m68k richtet `UWORD` auf 2 aus – passt). Speichergrößen rechnen mit
`lenitem` (Original-Semantik, inkl. des `join`-Bugs), auf dem Host allokiert
`ted_alloc()` intern `offsetof(struct item, chars) + n`.

### 3.2 `core/state.h` (Chunk 06 definiert, alle benutzen)

Alle Editor-Globals, die der Kern braucht: `firstitem`, `enditem`,
`thisitem`, `numlines`, `cursx/cursy`, `cursoff/curstop`, `scloc/chloc`,
`cursx2…`, `btopx/btopy/bbotx/bboty`, `bloktopx…`, `chcols`, `edrows`,
`edcols`, `tokenise`, `tabstop`, `makeicons`, `fonthite…`, `chmap`,
`charbuff…charbuff4`, `dosbuff`, `filehand`, `allalt`, `anyalt`,
`mousealt`, `memalt`, `invflag`, `qualifier`, sowie die edstruct-Kopie
(`tokens`, `menitems`, `loadxtra`, `savextra`, `menurout`, `cleanup`,
`colon`, `quote`, `label`, `remark`) und der Optionsblock `defstart`
(50-Byte-Dateiabbild, big-endian; Zugriff über Getter, damit der Host nicht
vom Byte-Layout abhängt). Die `comdata`-Tabelle (−8…+94, gepackt, mit Daten
`bmapmul`/`req_window`, die zugleich Globals sind) liegt in `structs.h` /
`input.c`: die Globals sind dort `#define bmapmul (mydata.bmapmul)`; die
Code-Zeiger der Tabelle setzt `outside` zur Laufzeit (DIFF: im Original
statisch – ohne Wirkung, blitz2 liest sie erst nach dem Init).

### 3.3 `core/hooks.h` – die Naht zwischen Kern und Amiga

Reine Funktionsdeklarationen; die Amiga-Dateien implementieren sie, auf dem
Host `tests/host/stubs.c` (zählt Aufrufe, prüft Reihenfolge):

| Gruppe | Hooks |
|---|---|
| Speicher | `ted_alloc(size, flags)`, `ted_free(p, size)`; Fehlschlag = `outmem()` (NORETURN, §4) |
| Dateien | `ted_open(name, mode)`, `ted_read`, `ted_write`, `ted_close`, `ted_rename`, `ted_delete` (Amiga: dos.library 1:1, Host: stdio) |
| Meldungen | `error(msg)` (comdata+12), `beepem()`, `erase()` (Rückfrage, BOOL) |
| Anzeige | `showall()`, `showthisitem()`, `showtopitem()`, `showbotitem()`, `scrollup(row)`, `scrolldown(row)`, `scrupall()`, `scrdownall()`, `cls()`, `refresh()`, `showhprop()`, `showvprop()`, `pstats(...)`, `bloken()`/`blokprnt2()`, `showfname()`, `sleepon()`/`pointeron()` |
| Rückrufe blitz2 | `call_loadxtra(fh)`, `call_savextra(fh)`, `call_cleanup()`, `call_menurout(d6, first, n)` (Amiga: `callregs`, Host: Stub) |

Hook-Namen = Original-Labels, damit Aufrufe im Kern wie im Asm aussehen.
Wo ein Kern-Teil eine UI-Routine nur „zur Hälfte“ braucht (z. B. `blokdis`:
Zustand + Statuszeile), wird die Routine geteilt: Zustandsänderung im Kern
(gleicher Name), Anzeige als Hook `blokprnt2()`.

### 3.4 Daten und Binär-Includes

* `sleep.bin`, `pointer.bin`, `src.info`, `chars.b`, `chars8.b` →
  `src/core/res.c` bindet die Originaldateien beim Build mit `#embed` ein
  (`const UBYTE sleep_[704]` …; bis 27.10.2026 erzeugte `tools/bin2c.py`
  daraus `build/*/gen/res.c`). Keine Kopien im Repository.
  `mouseinit` kopiert sie wie im Original in Chip-RAM.
* Requester (`findreq`, `repall`, `errorreq`, `erasereq`, `defaultreq`,
  `prtreq`, `aboutreq`, `specialreq`, `gotoreq`): `tools/req2c.py` (Fork mit
  den Erweiterungen aus survey-07 G) erzeugt die Initialisierer; feste
  Schrittweiten (84/86/44/56/28/54 Bytes) werden durch Zeigertabellen ersetzt
  (DIFF, gleiches Verhalten). Menüs, NewScreen/NewWindow von Hand.
* `rawed.obj` → `rawkey.c` (Entscheidung in reference.md §3.1),
  `filereq` → `filereq.c` mit dem gemeinsamen Requester-Kern
  `shared/filereq/filereq_core.c` („Variante C“, reference.md §3.2; in Phase 2a
  als `libs-c/libs/freqlib_req.inc` umgesetzt, seit 27.10.2026 eigene
  Übersetzungseinheit).

## 4 Was Asm bleibt (`glue*.c`)

Kein Original-Code bleibt Asm (der einzige echte Asm-Fremdkörper, die
eingebetteten Hunks, wird portiert). Neu zu schreibende Stubs:

| Stub | Grund |
|---|---|
| Hunk-Anfang `bra _edstart` / Hunk+4 `_outside` | blitz2 ruft `BADDR(seg)+8`; vor `outside` darf kein Startcode stehen → `glue_start.c` als erstes Objekt, `-nostartfiles`, `checkhunk.py` nach jedem Build |
| `_outside` | a1 = edstruct → C; Rückgabe a0 = `.tomain`-Stub, a1 = `&comdata`, Z-Flag (BUG(orig): Erfolg auch bei Fehlschlag, survey-01 I) |
| `_tomain` | a0 = Dateiname; merkt den Stackrahmen für `outmem` |
| `_edstart` | Standalone-Start (wbheader-Ersatz: CLI-Argument / WBStartup, eigene `SysBase`/`DOSBase`, da kein libnix-Startcode) |
| comdata-Einsprünge (−8…+88) | Registerkonvention je Dienst (survey-06 C, survey-03/04/05 B): `REGSTUB`-Muster aus blitz2-c, Rückgaben d0/d6/d7/a0 und Z/N; jeder Stub lädt TEDs a4 (`GLUE_LOAD_A4`) |
| Rückrufe nach blitz2 | `menurout` (d6/a5/d7 → d0), `loadxtra`/`savextra` (d7 = fh, a6 = DOSBase), `cleanup` → `callregs()` |
| VBlank-Server `myroutine` | `is_Code`-Konvention, a4 laden, Rückgabe Z=1 (`moveq #0,d0`), gemeinsame Variablen `volatile` |
| `outmem` | `a7 = memstack; jmp memcont` → `longjmp` auf den `setjmp` in `mainedit`. Muss auch aus blitz2 heraus funktionieren (`extalloc`, `openreq`): der Sprung verwirft blitz2-Frames (auch blitz2-c-Frames und deren a4) – Original-Verhalten, BUG(orig). `memstack == 0` vor der ersten Schleife → Original stürzt ab; C: gleicher Absturz vermeiden ist `DIFF:` (Alert + Ende), Entscheidung in Phase A |

Direkte Hardware-Zugriffe (Blitter `$DFF000`, Bitplanes, `COLOR00`) sind in
C mit `<hardware/custom.h>` (`volatile struct Custom *custom`) möglich – kein
Asm nötig. Divisionen `divu` mit 16-Bit-Quotient/Rest als explizite
`(UWORD)`-Ausdrücke.

## 5 Host-Kern und Unit-Tests

* Preset `host`: `TED_HOST=ON`, native gcc/clang, baut `tedcore` (alle
  `src/core/*.c`) + `tests/host/test_*.c`, `ctest`. Optional
  `-fsanitize=address,undefined`.
* Der Kern inkludiert **nur** `tedtypes.h`, `tedcore.h`, `hooks.h`,
  `state.h` und die Kern-Header. Ein Kern-Header, der `<exec/…>`/`<proto/…>`
  braucht, ist ein Fehler (prüft der Host-Build).
* `BUG(orig)` mit Speicherfolgen (Lesen hinter dem Zeilenende im Tokenizer,
  `detoka5` hinter `charbuff3`, NULL in `bloksave`/`gd03`, `join`-FreeMem-
  Größe): bleiben im Code; die Puffer liegen auf dem Host wie im Original
  zusammenhängend (eine Allokation für `dosbuff`+`charbuff*`, chmap mit
  Folgezeilen), damit das Verhalten gleich ist. Ein Makro `ORIG_HAZARD("…")`
  an diesen Stellen ist auf dem Amiga leer und zählt auf dem Host mit, damit
  Tests sie erkennen und gezielt erwarten können.
* Tokenliste für Tests: `tests/host/toklist.c` baut die verkettete Liste aus
  `builtin_tokens.txt`, `tokens/deflibs.txt`, `tokens/acidlibs.txt` in
  derselben Reihenfolge wie `bb2tok.load_tokens()`; Trenner wie blitz2
  (`colon`=':', `quote`='"', `label`='.', `remark`=';').

## 6 Verifikation

### (a) Host-Unit-Tests des Kerns

| Test | Prüfung | Orakel |
|---|---|---|
| `test_detok` | jede Zeile der 724 tokenisierten CD-Quellen (Liste wie `compilerlauf/tools/bb2corpus.py sources()`) mit `detok` → Text | `bb2tok.py detok` (Fixtures von `tests/host/mkfixtures.py` in `build/host/fixtures`, nicht im Repo) |
| `test_tokit` | detokenisierter Text Zeile für Zeile mit `tokit2` → Bytes | `bb2tok.py tok` (gleich für **alle** 724 – differenzieller Test; dazu Einzelfälle aus der Kopfdoku von bb2tok: `x1print`, `$`-Hex-Quirk, `_.\`, Quote/Remark) und die 601 Originale byte-gleich wie bei `bb2tok check` |
| `test_loadsave` | Datei → `fileopen2`-Kern (`doopen`/`readline`/`allocline`/`linka0`) → Zeilenliste → `realsave`-Kern → Bytes | **Rundreise byte-identisch** für alle 724 tokenisierten Quellen; Ausnahmen (z. B. Zeile mit 128 Bytes, `bui7/marbles/…`) werden aufgelistet und mit Grund erwartet; die 9 Text-`.bb2` mit `tokenise` an gegen `bb2tok.py tok` (unter Beachtung von `readline`-Bug „letzte Zeile ohne LF“) |
| `test_lines` | insert/delete/join/split/return/blokcopy auf kleinen Listen; Blockkoordinaten-Regeln aus survey-02 H als Tabellentests; `freeupa5` mit 1 Zeile | Regeln aus dem Survey (aus dem Asm abgeleitet) |
| `test_search` | `isithere2` (groß/klein, Token-Zeilen über detok), `replace2`, `dosearch` vorwärts/rückwärts | Handfälle |
| `test_lineedit` | Zeilenpuffer-Operationen von `getline` (Einfügen mit Verlust des letzten Zeichens, Löschen, Tab, Shift-Tab, Löschen bis Zeilenende), `horizpos` | Handfälle aus survey-06 |
| `test_opts` | `loaddefs`/`savedefs`-Datenteil: 50-Byte-Abbild ↔ Globals | `defstart` aus dem Listing (`tedlisting.py bytes defstart defend`) |
| `test_keymap` | Rawkey-Sonderfälle `$4C–$5F`, `germs` | Listing (`rawed`-Bytes, `germs`) |

### (b) Instruktions-/Byte-Prüfungen gegen das Listing

Da kein Original-Code als Asm übernommen wird, betrifft das:
* **Daten**: `tools/checkdata.py` vergleicht C-Daten mit dem Listing
  (`tedlisting.py bytes START END`): Fonts/Sprites/Icon (`res.c`),
  Optionsblock `defstart`, `germs`, `jumptable`/`menutable`-Schlüssel und
  Flags, Fehlertexte, Requester-Strukturen (Felder außer Zeigern; Zeiger
  über die Symbolzuordnung). Quelle der C-Bytes: das Objekt mit Symbolen
  (`m68k-amigaos-objdump -t`/`-s`) aus dem Debug-Build.
* **Layout**: `_Static_assert` für `comdata` (−8…+94), edstruct-Kopie (28
  Bytes), `struct item`, Optionsblock; Hunk-Layout per `checkhunk.py`.
* **Stubs**: `objdump -d` der Glue-Stubs gegen die in survey-06 C/H
  festgelegten Register-Ein/Ausgänge (Review, kein Automat). Für
  `TED_FILEREQ_BLOB=ON` zusätzlich `tedlisting.py verify` auf den
  eingebetteten Hunk.
* **Zeilenverweise**: jede Funktion trägt `ted.s:NNN`; ein Skript prüft,
  dass jedes Code-Label aus `ted.labels` genau einmal in `src/` vorkommt
  (Vollständigkeit wie in blitz2-c Phase B).

### (c) Emulatortests (später, hier nur beschrieben)

* Aufbau wie `compilerlauf` (`bb2compile.py`-Runner): `blitz2:`-Verzeichnis
  mit Compiler (vasm-Build 1.95 **und** blitz2-c) + `ted` (Referenz-vasm-Build
  aus `build/ref` bzw. TED-C). Gleiches Skript gegen beide TEDs → A/B.
* Eingaben als synthetische Ereignisse über einen Begleitprozess nach dem
  Muster von `compilerlauf/tedstub/src/reqclose.h` (Skript aus `ENV:`,
  Ereignisse per `input.device` `IND_WRITEEVENT`: RAWKEY mit Qualifier,
  Menüwahl über Amiga-Tastenkürzel bzw. RAWMOUSE, Gadget-Klicks an
  Fensterkoordinaten). Requester von TED (Erase, Error, Dateirequester) werden
  wie bei reqclose beantwortet.
* Szenarien: Quelle laden → speichern (Datei byte-gleich mit dem Original-
  TED); Tippen/Join/Split/Block kopieren/Suchen-Ersetzen → speichern →
  Vergleich A/B; COMPILER → CREATE EXECUTABLE → Executable gleich dem von
  tedstub/Referenz-TED; `loadxtra`/`savextra`-Rundreise; Fehlerzeile
  (`menu` → d0) setzt den Cursor.
* **Nie blockierend**: jede Sitzung ≤ 90 s (Watchdog im Runner, Abbruch
  des Emulators), Begleitprozess mit eigener Zeitgrenze, Log mit
  `REQCLOSELOG`-artigem Anhängen (übersteht Hänger), Ergebnis immer eine
  Datei (OK/FAIL/TIMEOUT). Natives Chipset (kein RTG), weil TED planar zeichnet
  und den Blitter direkt benutzt.

## 7 Ablauf

1. **Phase A – Header** (je Chunk ein Übersetzer): `core/<name>.h` und
   `src/<name>.h` mit allen Labels aus `xrefs.md` und den Routinen der
   eigenen Kern-Datei; Koordinator schreibt vorher `tedtypes.h`,
   `tedcore.h`, `hooks.h`, `state.h`, `structs.h`, `glue.h`,
   `CONVENTIONS.md` (Ableitung von blitz2-c mit §1–§5 dieses Plans).
   Prüfung: `--target headers` (Amiga) und ein Host-Syntaxlauf über alle
   Kern-Header.
2. **Phase B – Übersetzung** parallel; Kern zuerst kompilierbar auf beiden
   Presets. Chunk 07 nutzt `req2c.py`/`bin2c.py`; `filereq.c` = Arbeit am
   gemeinsamen freqlib-Kern.
3. **Phase C – Host-Tests** (§6 a) bis grün; Abweichungen klären (Bug im
   Port oder BUG(orig)).
4. **Phase D – Daten-/Layoutprüfung** (§6 b), dann Emulatortests (§6 c).

## 8 Offene Punkte

* Verhalten bei `outmem` vor der ersten Hauptschleife (Original stürzt ab).
* ~~Gemeinsamer Requester-Kern mit libs-c~~: erledigt (Variante C,
  heute `shared/filereq/`).
* Stand Phase 2a und weitere offene Entscheidungen: [PHASE2B-BRIEF.md](PHASE2B-BRIEF.md) §7.
* `.clangd` für `src/core`: läuft als m68k-Profil; ein Host-Profil für
  `tests/host` ist eingetragen.
* Ob `chmap` im Kern zwingend Chip-RAM braucht: nein auf dem Host; auf dem
  Amiga alloziert die UI-Seite sie wie im Original (Blitter).
