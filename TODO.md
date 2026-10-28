# TODO

Stand 28.10.2026. Dieses Repository ist die Referenz für den Code
(Entscheidung vom 27.10.2026; Übernahme von bb2 `7b5df74`). Build,
Referenz-Builds, Orakel und die Host-Tests von TED sind hier; die übrigen
Tests und Prüfungen liegen noch im Arbeitsrepository `bb2` und werden
hierher geholt. Ziel: alle Prüfungen, die bb2 vor dem Abbau von Python
hatte, wieder aktiv – hier im Build oder als ctest. Erst danach die
Umstellung auf C23 (§3).

## 1 Prüfungen, die gegenüber bb2 noch fehlen

Schon wieder aktiv (hier): Library-Dateien (`tools/blitzlibcheck`, früher
`mkblitzlib.py`), TEDs Hunk-Aufbau (`tools/hunkcheck`, früher
`checkhunk.py`), `checklink`; Tests der Host-Werkzeuge.

**TED (bb2: `ted-c/`)**

1. ~~**Referenz-Build von `ted.s`**~~ erledigt (27.10.2026): Ziel
   `ted_ref`, Originalquellen in `archive/sources/ted-1.1/`,
   `tools/vasmlisting` statt `tedlisting.py` (`map`, `labels`, `verify`);
   `ref/ted`, Listing, Map und Labels byte-identisch zu bb2; `verify` ist
   eine Prüfung im Build-Schritt.
2. ~~**`checkdata`**~~ erledigt (28.10.2026): `tools/checkdata` (statt bb2
   `tools/checkdata.py`, Ausgabe zeilengleich), Tabelle
   `programs/ted/checkdata.tsv` (534 Einträge), POST_BUILD von `ted.sym`
   (zweiter Link mit Symbolen wie in bb2); `ted` wird erst danach gelinkt.
   Ein fehlendes C-Symbol ist jetzt ein Fehler (in bb2 „todo“).
3. ~~**`checklabels`**~~ erledigt (28.10.2026): `tools/checklabels`
   (statt bb2 `tools/checklabels.py`, Ausgabe byte-identisch) als
   POST_BUILD von `ted`: 278 Code-Labels, alle genau einmal.
4. **Host-Tests des TED-Kerns** – erledigt (28.10.2026): alle 10 Tests von
   bb2 `ted-c/tests/host` auf snitch portiert (`programs/ted/tests/host`,
   Kern als `tedcore_host`, Hilfen `lines_helpers.hpp`, `tempdir.hpp`,
   `oracle.hpp`, `corpus.hpp`). Die Korpus-Tests detok, tokit, loadsave
   vergleichen exakt per SHA-256 mit dem Original-TED
   (`programs/ted/tests/oracle`, Rahmen `shared/edstub`, Ziel
   `ted-oracle` → `tests/data/ted-oracle.sha256`, 724 Quellen, 715
   tokenisiert; Token-Liste `tests/data/ted-tokens.txt`), ohne die
   bb2tok-Ausnahmen; ohne CD übersprungen. `TED_TEST_DUMP=DIR` schreibt
   abweichende Ergebnisse zum Vergleich mit einem Orakel-Lauf. Gegen
   bb2tok geprüft: Laden/Speichern aller 724 gleich; detok/tokall weichen
   in 84 Dateien nur bei unbekannten Tokens ab (`<$XXXX>` gegen `?????`).
   Die Token-Liste des Compilers hat dieselben 1821 Einträge wie bb2tok,
   aber in **anderer Reihenfolge** (bb2tok: eingebaute, deflibs,
   acidlibs).
5. **TED-A/B-Tests im Emulator** (bb2: `tools/tedab.py`, `teddump.py`,
   `tests/emu/*.drv`, Amiga-Werkzeuge `teddrive`, `teddrivetest`,
   `tedprobe` in `tests/amiga`): Original-TED gegen TED-C, mit blitz2 1.95
   und blitz2-c. Dazu die Host-Tests der Werkzeuge (`test_tedab.py`,
   `tdscript_host`).

**Libraries (bb2: `libs-c/`)**

6. ~~**Befehlsvergleich der Assembler-Routinen**~~ erledigt (28.10.2026):
   `tools/asmcmp` (statt bb2 `tools/asmcmp.py`, Ausgabe gleich, auch bei
   Abweichungen), ctest `asmcmp_runerrlib`, `asmcmp_debuglib`,
   `asmcmp_memacclib` mit den Routinen-Listen aus bb2; Originale aus der
   CD (`BLITZ2/ACIDLIBS`, gleich bb2 `ref/*.obj`), Symbole aus
   `NAME.linked` (`HUNK_SYMBOL`, kein `nm`).
7. ~~**Deklarationsvergleich mit den Originalen**~~ erledigt
   (28.10.2026): `tools/blitzlibcompare` (Regeln von bb2 `biglib.py
   compare`, bisher von Hand), ctest `declarations_acidlibs`,
   `declarations_deflibs` gegen `BLITZ2/ACIDLIBS`, `DEFLIBS` der CD:
   83 + 39 Libraries, Deklarationen identisch, gleiche Reihenfolge (mit
   `biglib.py compare` gegengeprüft: 122 identisch).
8. ~~**snitch-Tests im Emulator**~~ erledigt (28.10.2026): `libs/tests/`
   (`libs_tests`, m68k-snitch-Programm aus bb2 `libs-c/tests`, unverändert
   bis auf Kommentare), ctest `libs_emulator` (`emulator.cmake` statt
   `tools/amiga_test.py`): jede acidlib dieses Builds gegen ihr Original,
   das `biglib extract` zur Testzeit aus `BLITZ2/ACIDLIBS` der CD holt
   (gleich bb2 `ref/*.obj`). 7 Testfälle, 19372 Assertions wie bb2, etwa
   18 s; Ausgabe gleich bb2s Referenzprotokoll bis auf eine Stackadresse.
   Ein verändertes Byte in `ldivlib` lässt den Test scheitern.

**Compiler und Ganzes (bb2: `compilerlauf/`)**

* Referenz-Build von `blitz2.s` 1.95 erledigt (28.10.2026): Ziel
  `blitz2_ref`, Quellen in `archive/sources/blitz2-1.95/`; `ref/blitz2`
  byte-identisch zu bb2 `compilerlauf/blitz2-195`, `verify` 13592 Zeilen.

9. ~~**Korpus**~~ erledigt (28.10.2026, Phasen 1-5 unten): bb2s
   `tools/corpus_check.py`, `bb2corpus.py`, `bb2compile.py`,
   `serialrun.py`, `corpus/`, `tedstub`, `reqtrap`, `reqclose` sind hier in
   C/CMake (`tests/corpus/`); `ctest --preset emulator` etwa 3 Minuten.

   **Plan (28.10.2026, abgestimmt):** Ablage `tests/corpus/` (übergreifend).
   1. ~~Amiga-Werkzeuge~~ erledigt (28.10.2026): `tests/corpus/amiga/`
      `tedstub` auf `shared/edstub` (nur Batchmodus; ein Einzeljob ist eine
      Liste mit einem Eintrag), `reqtrap`, `reqclose`, `reqclosetest`
      (byte-gleich zu bb2). Probelauf mit blitz2 1.95: Ergebnisse, Fehler-
      zeile und Hashes wie bb2 `reference/195.tsv`.
   2. ~~Testquellen~~ erledigt (28.10.2026): `tests/corpus/tests*/*.bas`
      (135); tokenisiert mit `tedtok` (C-Kern von TED, `fileopen2` +
      `realsave`, echte Token-Liste), das Textzeilen über 129 Zeichen
      ablehnt (TED schneidet sie). 15 solche Zeilen in 9 Quellen aus bb2
      geteilt (`NPrint a,…,z` → `Print a,…` / `NPrint …,z`, gleiche
      Ausgabe); danach alle 135 byte-gleich zu bb2tok. Statt bb2s
      `default.xtra` (Herkunft unklar) schreibt der Stub mit `runerr=off`
      die Startoptionen des Compilers ohne Fehlerprüfung/Debugger.
   3. ~~Prüfmatrix~~ erledigt (28.10.2026): `tests/corpus/runner`
      (`corpusrun`, C auf `amiga_emulation`), ctest `corpus`,
      `corpus_runerr`, `corpus_req` (Label emulator): **201 Prüfungen, 0
      Abweichungen**, etwa 100 s (3 Emulatoren parallel; seriell etwa
      dreimal so lang). clib = C-acidlibs und C-deflibs. Die Programme laden
      Klänge und Module von der CD über `BB2:` (wie in bb2). Programmausgaben
      byte-gleich zu bb2s Läufen bis auf `t13_env` (liest absichtlich die
      Umgebung: Workbench 640×512×8 statt 640×256×2).
   4. ~~CD-Referenzlauf~~ erledigt (28.10.2026): `tests/corpus/runner/cdrun`,
      Ziel `blitz2-cd-reference` (blitz2 1.95, etwa 35 s) →
      `tests/data/blitz2-cd.tsv`: 498 übersetzt, 217 Fehler, 9 Textquellen;
      alle 724 Zeilen gleich bb2 `reference/195.tsv`, reproduzierbar. ctest
      `corpus_cd` (blitz2-c): 724 gleich, etwa 30 s.
   5. ~~Serielltest~~ erledigt (28.10.2026): `tests/corpus/runner/serial.c`
      (Gegenstelle mit POSIX-Sockets und pthread, Protokoll wie bb2s
      `serialrun.py`), Pass-Schlüssel `serial=on`: je Konfiguration eigener
      Emulator mit eigenem Port (`serial_port=TCP://127.0.0.1:PORT`) und
      eigener Gegenstelle; verglichen werden auch die Leitungsbytes, ein
      `TIMEOUT` in der Ausgabe zählt als Abweichung. ctest `corpus_serial`,
      `corpus_serial_runerr` (dieser prüft auch: Leitung der Referenz gleich
      der ohne Fehlerprüfung): `t64_serialrx` 0 Abweichungen, je etwa
      17 s. bb2 hat von seinen Seriell-Läufen nur den Bericht aufbewahrt,
      ein Byte-Vergleich damit war nicht möglich.
   Parallele Emulatorläufe nur mit **konfigurierbarer Obergrenze** (1 muss
   immer gehen): Windows-Amiberry nutzt durchnummerierte Named Pipes,
   macOS einen festen Socket unter `/tmp`; die heutige Isolation über
   `XDG_RUNTIME_DIR` gibt es nur unter Linux.

**Grundlage für 5, 8, 9: Emulator-Bibliothek `tools/lib/amiga_emulation`**
– **umgesetzt** (27.10.2026, mit `tools/amiga_run` und Tests: vier Läufe
parallel, RTG, Steuer-Socket, Zeitlimit mit Screenshot). Erster Nutzer:
das TED-Orakel (`ted-oracle`, `amiga_run` mit zusätzlichen Laufwerken).
Noch offen: Korpus, TED-A/B und snitch-Tests laufen in bb2 weiter über
`tools/amiberry.cmake` mit `AB3C:`; beim Hierherholen auf
`amiga_emulation` umstellen (für den Korpus-Stub gibt es den Rahmen
`shared/edstub`).

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

**Grundsätze dazu**

* Tests liegen hier bei ihrer Komponente (z. B. `programs/ted/tests/`,
  später `libs/acidlibs/tests/`), übergreifende oben in `tests/`;
  erzeugte Testdaten in `tests/data/` (so umgesetzt, 28.10.2026).
* Referenzdaten: Originalquellen in `archive/sources/`, CD und
  Aminet-Archiv über `fetch-originals` (Original-Libraries:
  `archive/cd/…/BLITZ2/DEFLIBS`, `ACIDLIBS`).
* Die komplexen Python-Werkzeuge bleiben vorerst Python (Entscheidung vom
  27.10.2026); hier nur C/C++ und CMake. Ein Test, der Python braucht,
  bleibt deshalb vorerst in bb2.

## 2 Weitere offene Punkte

* **`-h` / `--help` für alle aufrufbaren Programme** (Wunsch vom
  28.10.2026): Nutzungstext nach stdout, Rückgabe 0; heute kennt es keines.
  Betroffen: die Host-Werkzeuge `tools/*` (amiga_run, asmcmp, biglib,
  blitzlibcheck, blitzlibcompare, checkdata, checklabels, hunkcheck,
  vasmlisting), `programs/ted/tools/tedtok`, `tests/corpus/runner`
  (corpusrun, cdrun) und die Amiga-Programme `reqtrap`, `reqclose`,
  `reqclosetest`. Nicht dabei: `tedstub` und `tedoracle` (lädt nur
  blitz2 als `blitz2:ted`, aus der Shell gestartet enden sie mit 20).
  Offen: `blitz2` und `ted` selbst – sie bilden das Verhalten der Originale
  nach (Argument = Datei), eine Hilfe wäre ein `DIFF`.

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
