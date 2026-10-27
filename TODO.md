# TODO

Stand 27.10.2026. Dieses Verzeichnis enthält nur den Build; die Tests und
Prüfungen liegen noch im Arbeitsrepository `bb2` (Übernahme von bb2
`7b5df74`). Ziel: alle Prüfungen, die bb2 vor dem Abbau von Python hatte,
wieder aktiv – hier im Build, als ctest oder als Werkzeug, das auf diesen
Build zeigt. Erst danach die Umstellung auf C23 (§3).

## 1 Prüfungen, die gegenüber bb2 noch fehlen

Schon wieder aktiv (hier): Library-Dateien (`tools/blitzlibcheck`, früher
`mkblitzlib.py`), TEDs Hunk-Aufbau (`tools/hunkcheck`, früher
`checkhunk.py`), `checklink`; Tests der Host-Werkzeuge.

**TED (bb2: `ted-c/`)**

1. **Referenz-Build von `ted.s`** (bb2: CMake-Ziel `ref`): `ted.s` mit vasm
   (`-devpac -nocase`) assembliert → `ref/ted`, Listing `ted.lst`, daraus
   `ted.map`/`ted.labels` (`tools/tedlisting.py`); ctest `ref_verify`
   (jedes Listing-Byte gegen das Binary). Voraussetzung für 2 und 3.
   Offen: woher die Originalquellen kommen (hier liegen nur die 5
   Ressourcen; Pfad als Cache-Variable auf bb2/`quelltext` oder Kopie
   unter einem eigenen Verzeichnis).
2. **`checkdata`** (bb2: `tools/checkdata.py`, 617 Zeilen): die Daten des
   C-Builds (`ted.sym`) gegen das Listing der Referenz – Requester,
   Menüs, Tabellen. Braucht einen Link mit Symbolen (`ted.sym`; `strip`
   kann den baserel-Daten-Hunk nicht, deshalb in bb2 ein zweiter Link).
3. **`checklabels`** (bb2: `tools/checklabels.py`): jedes Code-Label von
   `ted.s` genau einmal als `ted.s:NNN label` in `src/`.
4. **Host-Tests des TED-Kerns** (bb2: Preset `host`, `tests/host`, 13
   ctest): `src/core` nativ übersetzt (`TED_HOST`), Fixtures aus den
   CD-Quellen (`mkfixtures.py` + `bb2tok.py`). Hier bräuchte der Host-Teil
   wie `tools/` einen eigenen nativen Build (ExternalProject oder Preset).
5. **TED-A/B-Tests im Emulator** (bb2: `tools/tedab.py`, `teddump.py`,
   `tests/emu/*.drv`, Amiga-Werkzeuge `teddrive`, `teddrivetest`,
   `tedprobe` in `tests/amiga`): Original-TED gegen TED-C, mit blitz2 1.95
   und blitz2-c. Dazu die Host-Tests der Werkzeuge (`test_tedab.py`,
   `tdscript_host`).

**Libraries (bb2: `libs-c/`)**

6. **Befehlsvergleich der Assembler-Routinen** (bb2: ctest `asmcmp_*`,
   `tools/asmcmp.py` mit `objdump`): runerrlib, debuglib, memacclib gegen
   die Originale (`ref/*.obj`). Braucht die Original-Libraries als
   Referenz und `NAME.linked` (gibt es hier schon).
7. **Deklarationsvergleich mit den Originalen** (bb2: von Hand mit
   `biglib.py compare`): Kopf, Einträge, `!libfin` jeder Library gleich dem
   Original. Hier automatisch möglich: `tools/lib/blitzlib` liest die
   Token-Tabelle schon; fehlt der Vergleich und die Original-Libraries als
   Referenz.
8. **snitch-Tests im Emulator** (bb2: `tests/libs_tests`,
   `tools/amiga_test.py`): jede Library gegen ihr Original geladen „wie der
   Compiler“, Register-Erhalt, Ergebnisse (7 Testfälle, 19372 Assertions).

**Compiler und Ganzes (bb2: `compilerlauf/`)**

9. **Korpus** (bb2: `tools/corpus_check.py`, `bb2corpus.py`,
   `bb2compile.py`, `serialrun.py`, `corpus/`, `tedstub`, `reqtrap`,
   `reqclose`): blitz2-c gegen blitz2 1.95, mit Original- und C-Libraries,
   mit und ohne Laufzeitfehlerprüfung, Requester- und serielle Tests (201
   Prüfungen). Emulatorlauf über bb2 `tools/amiberry.cmake`.

**Offene Grundsatzfragen dazu**

* Wo die Tests künftig liegen: hier (Komponenten-Tests bei der
  Komponente, z. B. `libs/acidlibs/tests/`; übergreifende oben in
  `tests/`) oder vorerst in bb2, mit Werkzeugen, die auf
  `build/amiga/blitz2/` dieses Verzeichnisses zeigen.
* Die komplexen Python-Werkzeuge bleiben vorerst Python (Entscheidung vom
  27.10.2026); hier nur C/C++ und CMake. Ein Test, der Python braucht,
  bleibt deshalb vorerst in bb2.
* Referenzdaten (Original-Libraries, `ted.s`, CD-Quellen für Fixtures und
  Korpus): kopieren oder per Pfad auf bb2 verweisen.

## 2 Weitere offene Punkte

* **Neues Repository oder nicht, Git-Historie:** noch nicht entschieden;
  das lokale Repository hier lässt sich verwerfen (`rm -rf .git`).
* **`dhandlerlib` (Lib 1) portieren:** bis dahin `prebuilt/dhandlerlib.obj`
  (Original der CD).
* **Warnungen von `blitzlibcheck`:** 73 Warnungen (Verweise in den
  Fehlerprüfbereich, wie bei `mkblitzlib.py`) bei jedem Neubau der
  betroffenen Libraries – bei Bedarf hinter eine Option.
* **LTO für TED** als eigenes Experiment: Nutzen klein (ein Aufruf über die
  Grenze zu `filereq_core.c`, 12 Bytes); Risiken `glue_start.c` am
  Hunk-Anfang (`hunkcheck` prüft das), `-fbaserel`, Glue mit
  Inline-Assembler. Nicht für die Libraries (`-fno-toplevel-reorder`).
  ab3c nutzt LTO für seinen Amiga-Build (`AB3C_LTO`) – Erfahrungen dort
  ansehen.

## 3 Erst wenn §1 erledigt ist

* **C23** (`gnu23` statt `gnu11`): `static_assert` statt
  `_Static_assert`, `constexpr` für Konstanten. Abnahme: alle Produkte
  byte-identisch, alle Prüfungen aus §1 grün.
