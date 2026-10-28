# Übersetzungskonventionen ted.s → C (Phase 2b)

Ableitung von [`../../blitz2-c/CONVENTIONS.md`](../../blitz2-c/CONVENTIONS.md)
mit den Punkten von [PLAN.md](PLAN.md) §1–§5. Wo hier nichts steht, gilt
blitz2-c. Code-Kommentare englisch, Dokumentation deutsch.

## 1 Dateien und Besitz

| Chunk | Zeilen | Kern (`src/core/`, host-testbar) | Amiga (`src/`) |
|---|---|---|---|
| 01 | 1–1015 (+wbheader, aboutreq, specialreq) | `loadsave.c/.h` | `main.c/.h` |
| 02 | 1016–1980 | `lines.c/.h` | `edit.c/.h` |
| 03 | 1981–3062 | `services.c/.h` | `mouse.c/.h` |
| 04 | 3063–4197 | `fileio.c/.h` | `reqs.c/.h` |
| 05 | 4198–5252 | `tokit.c/.h` | `screen.c/.h` |
| 06 | 5253–6285 | `lineedit.c/.h` (+ Daten in `state.c`) | `input.c/.h` |
| 07 | 6286–8072 | `keymap.c/.h` (+ `germs`, `fontcalc`) | `data.c/.h`, `rawkey.c/.h`, `filereq.c/.h` |

Gemeinsam, **Besitz des Koordinators** (nicht ohne Absprache ändern, §9):
`core/tedtypes.h`, `core/tedcore.h`, `core/hooks.h`, `core/state.h`,
`core/state.c`, `src/structs.h`, `src/glue.h`, `src/glue.c`,
`src/glue_start.c`, `src/ted.h`, `src/filereq.c`, `CMakeLists.txt`,
`tests/host/hostenv.h`, `tests/host/stubs.c`, `tools/*`, und
`../shared/filereq/` (gemeinsam mit libs-c).

Die Chunk-Header (`core/<x>.h`, `src/<x>.h`) enthalten bereits **jede
Routine** des Chunks mit Prototyp und Kommentar (Ein-/Ausgänge, asm-
Registerzuordnung, BUG(orig)-Hinweise), die `.c`-Dateien jede Funktion mit
leerem Körper (`/* TODO(2b) */`). Übersetzer füllen die Körper; die
Signaturen sind abgestimmt – Änderungen nur nach §9.

## 2 Kern und Amiga-Teil

* **Kern** (`src/core/*.c`) inkludiert nur `tedtypes.h`, `tedcore.h`,
  `hooks.h`, `state.h` und Kern-Header – nie `<exec/…>`, `<proto/…>`,
  `structs.h`, `ted.h`. Er baut auf dem Host (`TED_HOST`). Alles
  Betriebssystem läuft über `hooks.h`.
* **Amiga-Teil** (`src/*.c`) inkludiert `ted.h` (Umbrella) und die NDK-
  Header, die er braucht (`<proto/exec.h>`, `<proto/intuition.h>` …).
  Ausnahme: `filereq.c` (eigene Einheit, s. dort).
* **Mischroutinen** (Survey-Tabelle H „KERN+UI“): Der Kernteil steht im Kern
  unter dem Original-Labelnamen; die Ausgabe ruft er danach als Hook (Hook =
  UI-Routine mit Original-Labelnamen, implementiert im Amiga-Teil ihres
  Chunks). Wo die UI-Routine überwiegt, bleibt sie im Amiga-Teil unter dem
  Labelnamen und der Kernteil ist eine neue Funktion (`help_core`,
  `prtfile_line`, `gl_*`, `chmap_*`, `germs_map`, `rawspecial`) – im
  Kommentar als `ted.s:NNN (label, core part)` gekennzeichnet.
* Reihenfolge „erst Daten ändern, dann zeichnen“ bleibt; ein Hook ändert
  keinen Kernzustand außer wo in hooks.h als „state:“ vermerkt.

## 3 Namen, Kommentare, Marker

* Jedes Label bleibt Bezeichner (`tokit`, `gd03`, `blokcopy`). Konflikte mit
  C/libc: `_` anhängen (`return_`, `putchar_`, `sleep_` (Daten)).
* **Jede Funktion beginnt mit `/* ted.s:NNN label */`**; Labels innerhalb
  einer Funktion (goto-Ziele, Fail-Kette, `done2`, `getline2`, `ginp1` …)
  bekommen an ihrer Stelle einen Kommentar `/* ted.s:NNN label */`. Die
  Gerüste sammeln diese Marker am Dateiende – beim Übersetzen an die
  richtige Stelle verschieben. `tools/checklabels.py` (ctest `checklabels`)
  verlangt jedes Code-Label **genau einmal** in `src/**/*.c`. Verweise, die
  kein Definitionsort sind, anders schreiben (`ted.s:570 (prtfile, …)`,
  „ted.s line 128“).
* Original-Kommentare (englisch) übernehmen, `/* @NNN */` an Hauptblöcken
  langer Funktionen.
* `BUG(orig): …` – absichtlich übernommener Fehler (alle aus den Surveys,
  Abschnitt I). `DIFF: …` – unvermeidbare Abweichung mit Begründung; jede
  neue Stelle als Zeile in [abweichungen.md](abweichungen.md).
* `ORIG_HAZARD("…")` (tedcore.h) an Stellen, wo das Original außerhalb
  seiner Puffer liest/schreibt oder NULL benutzt und C das nachbildet
  (Amiga: leer; Host: gezählt).

## 4 Typen und Zustand

* Exec-Typen (Kern: `tedtypes.h`), Größen wie im asm (`dc` = 16 Bit,
  `dc.l` = 32 Bit); Vorzeichen nach Gebrauch, bei gemischten Vergleichen
  Cast am Vergleich (`(WORD)`/`(UWORD)`) wie `bcs/bhi` vs. `blt/bgt`.
* **Globals bleiben Globals**, Name und Initialwert wie im Original
  (`state.h` + Chunk-Header). Wortpaare, die das asm als Langwort kopiert/
  vergleicht, werden ausgeschrieben (beide Felder). Byte-/Wort-Aliasing
  (Gadget `+13` Bit 7 = `Flags & GFLG_SELECTED`, `numchars`+`chars[0]` als
  Wort) explizit. Optionsblock: `struct defblock defstart` mit Makros
  `fonthite`, `tabstop` … – **keine lokalen Variablen dieser Namen**.
* Register-Zustand über mehrere Aufrufe: `rd` (Laden: d7/a5/d6), `bc`
  (Blockcursor: a5/d6/d5), `ev_d7`/`ev_d6` (Ereignis beim Dispatch).
  Sonst Parameter/Rückgabewerte, Zuordnung im Header-Kommentar
  (`asm: Z=1 -> TRUE`).
* Handler aus `jumptable`/`menutable`: `void name(void)`, asm a4 = `thisitem`
  beim Eintritt (`struct item *a4 = thisitem;`).
* Token-Wörter immer `(hi << 8) | lo` aus Bytes; Strukturen nur über
  Felder. Divisionen `divu`: explizit 16-Bit-Quotient/Rest; Division durch 0
  im Original (z. B. `tabstop = 0`) bleibt (BUG(orig), ORIG_HAZARD davor).

## 5 Kontrollfluss

* Tail-Call `bra x` → `x(); return;`; Durchfall → Aufruf + Kommentar
  `/* falls through into x */`.
* `addq #4,a7`-Abbrüche (`erase`, `getfname3`, `getbname`, `doopen`) →
  Rückgabewert; **jeder Aufrufer prüft und kehrt sofort zurück**.
* `escape` → `edquit = TRUE` (mainedit kehrt zurück). `outmem` → `longjmp`
  (NORETURN; `memstack` = `jmp_buf *` von mainedit).
* Tokenizer-Ausstieg `pchover` → `goto`/`return` im Tokenizer.

## 6 Amiga-spezifisch

* **Inline-Assembler nur in `src/glue*.c`** (Koordinator). Blitter/Hardware
  mit `<hardware/custom.h>` in C, Reihenfolge und Breite der Zugriffe wie im
  Original.
* **Kein libnix-Startcode** (`-nostartfiles`): Bibliotheksbasen setzt
  `outside_c`/`edstart_c` selbst (`SysBase`, `DOSBase`, `IntuitionBase`,
  `GfxBase` in main.c). Keine libnix-Funktionen benutzen, die Konstruktoren
  brauchen (stdio, malloc, `printf` …); erlaubt: `memcpy`/`memset`/`strlen`
  usw. und Exec/DOS-Aufrufe.
* `-fbaserel`: Globals a4-relativ; jeder Einsprung von außen lädt a4 (Glue).
  Code, der von blitz2 aufgerufen wird, nur über die Stubs (`glue.h`).
* Rückrufe nach blitz2 nur über `call_menurout`, `call_cleanup`,
  `call_loadxtra`, `call_savextra` (glue.c, `callregs`).

## 7 Daten

* Requester (`findreq`, `repall`, `errorreq`, `erasereq`, `defaultreq`,
  `prtreq`, `gotoreq`, `aboutreq`, `specialreq`): Initialisierer mit
  `tools/req2c.py` (Fork von blitz2-c, Erweiterungen survey-07 G) – in 2b zu
  erstellen; Menüs, NewScreen/NewWindow von Hand. Alias-Labels als
  `#define` (`int1`, `errorpnt`, `gotoint`, `viewmode` …), Schrittweiten über
  Zeigertabellen (data.h).
* Binär-Includes: `src/core/res.c` bindet die Originaldateien beim Build
  mit `#embed` ein (Größen per `_Static_assert`), nie Kopien ins
  Repository.
* Datenprüfung: `tools/checkdata.py` – neue Daten dort in `CHECKS`
  eintragen.

## 8 Prüfen

* Amiga: `cmake --build --preset amiga` (ohne Warnungen soweit sinnvoll),
  `--target headers`, `ctest --preset amiga` (`ref_verify`, `checkhunk`,
  `checkdata`, `checklabels`).
* Host: `cmake --build --preset host`, `ctest --preset host`; mit
  Sanitizern `cmake --preset host -DTED_SANITIZE=ON`. Test einzeln mit
  Ausgabe: `build/host/tests/host/test_tokit build/host/fixtures -v`.
* Listing der Referenz: `tools/tedlisting.py build/amiga/ref/ted.lst …`
  (`addr`, `line`, `bytes`, `carray`).

## 9 Gemeinsame Header ändern

Fehlt etwas in einem gemeinsamen Header (Hook, Global, Feld, Typ) oder
passt eine Signatur eines **fremden** Chunk-Headers nicht: nicht selbst
ändern, sondern dem Koordinator melden (Datei, gewünschte Änderung, Grund,
asm-Beleg). Der Koordinator ändert, trägt eine Zeile in
[changes.md](changes.md) ein und informiert die anderen. Den **eigenen**
Chunk-Header darf man ergänzen (neue static-freie Hilfsfunktion), aber
bestehende Signaturen nur nach Absprache ändern – andere Chunks rufen sie.
