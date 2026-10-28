# TODO

Stand 27.10.2026. Dieses Repository ist die Referenz für den Code
(Entscheidung vom 27.10.2026). Es enthält bisher nur den Build; die Tests
und Prüfungen liegen noch im Arbeitsrepository `bb2` (Übernahme von bb2
`7b5df74`) und müssen auf dieses Repository umgestellt oder hierher
geholt werden. Ziel: alle Prüfungen, die bb2 vor dem Abbau von Python hatte,
wieder aktiv – hier im Build, als ctest oder als Werkzeug, das auf diesen
Build zeigt. Erst danach die Umstellung auf C23 (§3).

## 1 Prüfungen, die gegenüber bb2 noch fehlen

Schon wieder aktiv (hier): Library-Dateien (`tools/blitzlibcheck`, früher
`mkblitzlib.py`), TEDs Hunk-Aufbau (`tools/hunkcheck`, früher
`checkhunk.py`), `checklink`; Tests der Host-Werkzeuge.

**TED (bb2: `ted-c/`)**

1. ~~**Referenz-Build von `ted.s`**~~ erledigt (27.10.2026): Ziel
   `ted_ref`, Originalquellen in `archive/sources/ted-1.1/` (auch für
   `#embed`, `resources/` entfällt), `tools/vasmlisting` statt
   `tedlisting.py` (`map`, `labels`, `verify`); `ref/ted`, Listing, Map
   und Labels byte-identisch zu bb2; `ref_verify` ist jetzt eine Prüfung
   im Build-Schritt. Bisher (bb2: CMake-Ziel `ref`): `ted.s` mit vasm
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
4. **Host-Tests des TED-Kerns** – begonnen (27.10.2026): Host-Seite
   `host/`, Kern als `tedcore_host`; `test_state`, `test_lines_blocks`,
   `test_lines_edit` auf snitch portiert (`programs/ted/tests/host`,
   gemeinsame Hilfen in `lines_helpers.hpp`). Als Nächstes die Tests mit
   Testdaten (Token-Liste, CD-Quellen). Orakel dafür erledigt
   (28.10.2026): `programs/ted/tests/oracle` (Rahmen `shared/edstub`,
   Ziel `ted-oracle`) → `tests/data/ted-oracle.sha256` (724 Quellen,
   715 tokenisiert) und `tests/data/ted-tokens.txt`. Gegen bb2tok
   geprüft: Laden/Speichern aller 724 gleich; detok/tokall weichen in
   84 Dateien nur bei unbekannten Tokens ab (`<$XXXX>` gegen `?????`).
   Die Token-Liste des Compilers hat dieselben 1821 Einträge wie bb2tok,
   aber in **anderer Reihenfolge** (bb2tok: eingebaute, deflibs,
   acidlibs). Offen: die 7 Tests detok, tokit, loadsave, fileio, search,
   services_core, lineedit_keys portieren – Vergleich per SHA-256 gegen
   das Manifest, ohne bb2tok-Ausnahmen; `toklist.c` liest dann
   `ted-tokens.txt`. Bisher (bb2: Preset `host`, `tests/host`, 13
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

* Referenz-Build von `blitz2.s` 1.95 erledigt (28.10.2026): Ziel
  `blitz2_ref`, Quellen in `archive/sources/blitz2-1.95/`; `ref/blitz2`
  byte-identisch zu bb2 `compilerlauf/blitz2-195`, `verify` 13592 Zeilen.

9. **Korpus** (bb2: `tools/corpus_check.py`, `bb2corpus.py`,
   `bb2compile.py`, `serialrun.py`, `corpus/`, `tedstub`, `reqtrap`,
   `reqclose`): blitz2-c gegen blitz2 1.95, mit Original- und C-Libraries,
   mit und ohne Laufzeitfehlerprüfung, Requester- und serielle Tests (201
   Prüfungen). Emulatorlauf über bb2 `tools/amiberry.cmake`.

**Grundlage für 5, 8, 9: Emulator-Bibliothek `tools/lib/amiga_emulation`**
– **umgesetzt** (27.10.2026, mit `tools/amiga_run` und Tests: vier Läufe
parallel, RTG, Steuer-Socket, Zeitlimit mit Screenshot). Noch offen: die
bb2-Werkzeuge (Korpus, TED-A/B, snitch) laufen weiter über bb2
`tools/amiberry.cmake` mit `AB3C:`; sie auf `amiga_run`/`TEST:` umstellen
oder die Tests hierher holen. Entwurf: C-Library auf dem Host,
`amiga_emulation_start(Änderungen gegenüber der Standard-Konfiguration,
Skript, Zeitlimit)` → Handle; `amiga_emulation_command()` (Steuer-Socket),
`amiga_emulation_dir()` (Host-Verzeichnis = `TEST:`),
`amiga_emulation_join()` (fertig / Zeitlimit / vorzeitig beendet). Ersetzt
bb2 `tools/amiberry.cmake` und die Steuerung in den Python-Werkzeugen.

* Standard-Konfiguration `tools/lib/amiga_emulation/default.uae` (aus der
  AGA-Konfiguration, bereinigt; RTG-Variante = Änderungen) und
  `amiga-emulation/` mit README (erwartete Dateien, Prüfsummen,
  Bedingungen) liegen vor (27.10.2026); ROM und angepasste Hardfiles hat
  der Benutzer abgelegt, beide Images geprüft. Offen: CMake meldet
  fehlende Dateien.
* Laufwerk der Tests heißt **`TEST:`** (nicht mehr `AB3C:`, das ist ein
  anderes Projekt); beide Hardfiles sind angepasst (27.10.2026). Vorschlag für die
  `user-startup` (Abfrage abgesichert – ein nicht eingebundenes Laufwerk
  gäbe sonst den Requester „Datenträger einlegen“):

  ```
  Assign >NIL: TEST: EXISTS
  If NOT WARN
    If EXISTS TEST:run.script
      Execute TEST:run.script
    EndIf
  EndIf
  ```
  Die heutigen Zeilen für `SPEC:`, `AB3C:` und `Assign add Blitz3: AB3:`
  entfernen oder genauso absichern.
* **Hardfile schreibgeschützt** einbinden (`hardfile2=ro,…`,
  `uaehf0=hdf,ro,…`): geprüft am 27.10.2026 – bootet, `T:`/`ENV:` liegen
  im RAM, zwei Instanzen teilen sich dasselbe Image, das Image bleibt
  unverändert. Ein Schreibversuch auf `SYS:` öffnet aber „Datenträger
  Workbench ist schreibgeschützt“ und hängt: Tests schreiben nur nach
  `TEST:` oder `RAM:` (Requester abfangen: reqtrap aus bb2 oder Zeitlimit).
* **Parallel:** je Lauf eigenes `TEST:`-Verzeichnis und eigene
  `XDG_*`-Verzeichnisse inklusive `XDG_RUNTIME_DIR`; zwei Instanzen liefen
  gleichzeitig. Dann kann ctest Emulator-Tests mit `-j` fahren.
* **Steuer-Socket** (Amiberry 8.4, `$XDG_RUNTIME_DIR/amiberry.sock`):
  eine Textzeile je Befehl, **Argumente durch Tabs getrennt** (mit
  Leerzeichen wird die ganze Zeile als Befehlsname gelesen und in
  Großbuchstaben gewandelt), Antwort `OK`/`ERROR` + Tab-Felder. Geprüft:
  `GET_STATUS`, `READ_MEM 4 4` (ExecBase), `SCREENSHOT <pfad>` (PNG, auch
  ohne Fenster), `SET_WARP 1`/`GET_WARP`, `DEBUG_STATUS`, `QUIT` (beendet
  sofort – Ersatz für `uaequit`). Laut Hilfe außerdem `SEND_KEY`,
  `SEND_MOUSE`, `WRITE_MEM`, Breakpoints, `SAVESTATE`/`LOADSTATE`.

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

* **Git-Historie und Remote:** dieses Repository ist die Referenz
  (entschieden); offen, ob es mit dem frischen Anfang bleibt oder die
  Historie der Dateien aus bb2 übernimmt, und wohin es gepusht wird.
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

* **Lizenz-Kopfzeilen** (SPDX-Kennung `GPL-3.0-or-later WITH
  LicenseRef-BlitzBasic2-Runtime-exception` o. ä.) in den Quelldateien –
  noch nicht gesetzt; die Lizenz gilt über `LICENSE` und die README.
* ~~**CD-Daten**~~ gelöst (28.10.2026): nicht im Repository,
  `cmake/fetch-originals.cmake` lädt CD (archive.org) und Aminet-Archiv
  mit fester SHA-256 nach `archive/cd/`, `archive/aminet/`.

## 3 Erst wenn §1 erledigt ist

* **C23** (`gnu23` statt `gnu11`): `static_assert` statt
  `_Static_assert`, `constexpr` für Konstanten. Abnahme: alle Produkte
  byte-identisch, alle Prüfungen aus §1 grün.
