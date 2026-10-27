# blitzbasic2-c – Hinweise für Claude

C-Übersetzung von Blitz Basic 2: Compiler `blitz2` (v1.95, `blitz2.s`),
Editor `ted` (TED 1.1, `ted.s`) und die Libraries `acidlibs`/`deflibs`,
gebaut für AmigaOS (68000) mit m68k-amigaos-gcc. Aufbau, Bauen und die
Prüfungen im Build: `README.md`. Offene Punkte: `TODO.md`.

**Dieses Repository ist die Referenz** für den Code (Entscheidung vom
27.10.2026); bb2 baut nicht mehr maßgeblich. Es enthält bisher **nur den
Build**. Tests, Testkorpus, Referenz-Builds, Analysewerkzeuge, Notizen zur
Übersetzung und die Originalquellen liegen im Arbeitsrepository `../bb2`
(Übernahme von bb2 `7b5df74`); dort stehen auch die Begründungen für
Code-Kommentare wie `BUG(orig)`, `DIFF`, `ted.s:NNN` oder `blitz2.s:NNN`.

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
  Code-Umbau), funktional prüfen – solange die Tests noch in bb2 liegen,
  mit den bb2-Werkzeugen gegen `build/amiga/blitz2/` dieses Repositorys
  (Korpus, snitch-Tests, TED-A/B) – und die Abweichung im Commit nennen.
* **Original-Fehler nicht beheben**, nur dokumentieren (`BUG(orig)` im
  Code, Verzeichnis der Abweichungen in bb2); `DIFF` nur, wo technisch
  nötig.
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

## Git

* Ein Thema pro Commit; trennbare Arbeiten nie zusammenfassen.
  Commit-Texte und Code-Kommentare englisch, Dokumentation deutsch.
* Vor einem Commit `git status` prüfen: nur die Dateien des Themas
  stagen (vorgemerkte `git mv` rutschen sonst mit).
* Noch kein Remote. Später gilt: kein force-push, gepushte Historie nicht
  umschreiben.
