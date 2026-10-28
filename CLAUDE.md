# blitzbasic2-c – Hinweise für Claude

C-Übersetzung von Blitz Basic 2: Compiler `blitz2` (v1.95, `blitz2.s`),
Editor `ted` (TED 1.1, `ted.s`) und die Libraries `acidlibs`/`deflibs`,
gebaut für AmigaOS (68000) mit m68k-amigaos-gcc. Aufbau, Bauen und die
Prüfungen im Build: `README.md`. Offene Punkte: `TODO.md`.

**Dieses Repository ist die Referenz** für den Code (Entscheidung vom
27.10.2026, Übernahme von bb2 `7b5df74`); bb2 baut nicht mehr maßgeblich.
Hier liegen Build, Referenz-Builds (`ted_ref`, `blitz2_ref`), die
Originalquellen (`archive/`), das TED-Orakel und die Host-Tests von TED.
Noch in `../bb2`: Korpus, TED-A/B- und Library-Tests im Emulator,
Analysewerkzeuge. Die Notizen zur Übersetzung liegen in `docs/` (aus bb2
`ee6174b`, unverändert, Pfade im Aufbau von bb2) – dort stehen auch die
Begründungen für Code-Kommentare wie `BUG(orig)`, `DIFF`, `ted.s:NNN` oder
`blitz2.s:NNN`. Stand der Übernahme: `TODO.md`.

## Bauen und prüfen

```
cmake --preset amiga && cmake --build --preset amiga   # Ergebnis: build/amiga/blitz2/
ctest --preset host                                    # Tests ohne Emulator (Sekunden)
ctest --preset emulator                                # Tests in Amiberry
```

* Toolchain über `PATH` (`m68k-amigaos-gcc`, gcc ≥ 15 wegen `#embed`),
  sonst `AMIGA_TOOLCHAIN_PREFIX`. Ninja.
* Shell ist **fish**: Schleifen, `$(...)`, Heredocs in `bash -c '…'` oder
  `bash <<'EOF'` packen.
* Ein sauberer Neubau hat **keine Compiler-Warnungen**; erwartet sind nur
  die 73 Warnungen von `blitzlibcheck` (Verweise in den
  Fehlerprüfbereich, wie bei den Originalen).
* `.clangd` erzeugt CMake beim ersten Konfigurieren (nicht in git).
* Emulator: Kickstart-ROM und Hardfiles liegen lokal in `amiga-emulation/`
  (nicht in git, siehe dortige README); Standard-Konfiguration
  `tools/lib/amiga_emulation/default.uae`. Hardfiles schreibgeschützt:
  Tests schreiben nur nach `TEST:` oder `RAM:` (auch Systemprogramme wie
  `ShowConfig` schreiben auf `SYS:` und hängen dann).
* **Host-Seite** (`host/`, ExternalProject, nativer Compiler): Werkzeuge
  und Host-Tests. Host-Tests liegen bei ihrer Komponente
  (`programs/ted/tests/host/`), sind snitch-Programme (C++20, C-Header in
  `extern "C"`), **ein Programm je Testdatei** (der TED-Kern hat globalen
  Zustand). Zum Prüfen auch mit `-DHOST_SANITIZE=ON` bauen.
* Emulatorläufe nur über `tools/lib/amiga_emulation` bzw. `amiga_run`
  (isoliert, parallel möglich); Steuer-Socket: Argumente durch Tabs
  getrennt. Tests mit Emulator bekommen das ctest-Label `emulator` und
  den Rückgabewert 77 (übersprungen), wenn Amiberry oder die Daten fehlen.

## Regeln

* **Abnahme bei jedem Umbau:** `blitz2`, `ted`, `acidlibs`, `deflibs` und
  alle Library-Dateien byte-identisch zum Stand davor (Build des vorigen
  Commits dieses Repositorys). Ist eine Abweichung gewollt (z. B.
  Code-Umbau), funktional prüfen – mit den Tests hier
  (`ctest --preset all`) und, solange sie noch in bb2 liegen, mit den
  bb2-Werkzeugen gegen `build/amiga/blitz2/` dieses Repositorys (Korpus,
  snitch-Tests, TED-A/B) – und die Abweichung im Commit nennen.
* **`archive/`** enthält unveränderte Originale (Quellen wie
  `sources/ted-1.1/`, Referenzdaten) – nie bearbeiten; ein neuer Stand ist
  ein neues Verzeichnis mit Versionsnummer, die Herkunft steht in
  `archive/README.md`. Daraus Abgeleitetes entsteht im Build-Verzeichnis
  oder als erzeugte Testdaten unter `tests/data/`. Material ohne klare
  Lizenz (CD) wird nicht eingecheckt, sondern mit
  `cmake/fetch-originals.cmake` geladen (`archive/cd/`, `archive/aminet/`,
  ignoriert); Tests, die es brauchen, werden ohne es übersprungen.
* **Original-Fehler nicht beheben**, nur dokumentieren (`BUG(orig)` im
  Code, Verzeichnis der Abweichungen in `docs/*/abweichungen.md`);
  `DIFF` nur, wo technisch nötig. Gilt nur für die Portierung: Ist sie
  abgeschlossen (einschließlich des Abgleichs mit blitz2 2.10), dürfen
  Original-Fehler gezielt behoben werden (je Fehler ein Issue, als
  gewollte Abweichung gekennzeichnet).
* **Nur C/C++ und CMake.** Kein Python, keine Shell-Skripte im Build. Was
  Compiler und Linker selbst können, ohne Werkzeug (`#embed` +
  `_Static_assert`, `strip`); Text und Steuerung als CMake; Binärformate
  und Logik als C-Werkzeug in `tools/` (Logik in `tools/lib/`, Werkzeug =
  `main.c`, Tests in `tools/tests/` mit Eingaben aus dem Speicher).
* **Prüfungen im Build:** im selben Build-Schritt wie das Ergebnis (weiteres
  `COMMAND` der Custom Command bzw. `POST_BUILD` am Target). Schlägt die
  Prüfung fehl, gilt das Ergebnis als nicht gebaut, und nichts davon
  Abhängige wird gebaut. Keine temporären Dateien, keine Stamp-Dateien.
  Jede Prüfung braucht einen Test mit absichtlich kaputter Eingabe.
* **Zwei Übersetzungswelten:** Libraries mit den Blitz-Konventionen
  (`cmake/BlitzLibrary.cmake`: `-ffixed-a3/a4/a5`, freestanding,
  `-fno-toplevel-reorder`, ein Hunk über `libs/blitzlib.ld`); blitz2 und
  TED als Programme mit `-fbaserel` (a4 = Basis der Globals). Code für
  beide Seiten (`shared/`) übersetzt jede Seite selbst, mit eigener
  Umgebungs-Header (`filereq_env.h`).
* `libs/acidlibs` wird in der Reihenfolge der Original-Datei
  zusammengefügt (`ACIDLIBS_ORDER`); `prebuilt/` enthält Originale, die
  noch nicht portiert sind (`dhandlerlib`).

## Lizenz

GPL-3.0-or-later mit `RUNTIME-LIBRARY-EXCEPTION` (README, Abschnitt
„Lizenz“); Lizenzlage der Originale in `archive/README.md`. Neuer Code
steht unter derselben Lizenz; keinen Code mit unklarer oder
GPL-inkompatibler Lizenz übernehmen.

## Git

* Ein Thema pro Commit; trennbare Arbeiten nie zusammenfassen.
  Commit-Texte und Code-Kommentare englisch, Dokumentation deutsch.
* Vor einem Commit `git status` prüfen: nur die Dateien des Themas
  stagen (vorgemerkte `git mv` rutschen sonst mit).
* Remote `origin` (GitHub `fubar-coder/bb2-c`): kein force-push,
  gepushte Historie nicht umschreiben.
