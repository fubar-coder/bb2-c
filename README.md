# Blitz Basic 2 in C

Der Compiler **blitz2** (Blitz Basic 2 v1.95), der Editor **ted** (TED 1.1)
und die Libraries, die blitz2 beim Start liest (**acidlibs**, **deflibs**),
als C-Übersetzung der Originalquelltexte – gebaut für AmigaOS mit dem
m68k-amigaos-gcc.

Dieses Verzeichnis enthält nur, was zum Bauen nötig ist. Tests, Testkorpus,
Referenz-Builds, Analysewerkzeuge und Notizen zur Übersetzung liegen im
Arbeitsrepository `bb2` (Stand der Übernahme: bb2 `7b5df74`).

## Voraussetzungen

* CMake ≥ 3.28 und Ninja
* m68k-amigaos-gcc (bebbo/AmigaDev, getestet mit gcc 16.2) im `PATH`, sonst
  `-DAMIGA_TOOLCHAIN_PREFIX=…` oder gleichnamige Umgebungsvariable;
  gcc ≥ 15 wegen `#embed`
* ein C-Compiler für den Host (für die Werkzeuge in `tools/`)

## Bauen

```
cmake --preset amiga
cmake --build --preset amiga
cmake --install build/amiga        # optional: build/amiga/install/blitz2/
```

Ergebnis in `build/amiga/blitz2/` – der Inhalt des `blitz2:`-Verzeichnisses:

| Datei | Inhalt |
|---|---|
| `blitz2` | der Compiler |
| `ted` | der Editor (blitz2 lädt ihn als `blitz2:ted`) |
| `acidlibs` | die 83 Befehls-Libraries |
| `deflibs` | die 39 Deklarations-Libraries der AmigaOS-Aufrufe |

Preset `amiga-debug`: derselbe Code (`-Os`) mit Symbolen, ohne Strip.

## Prüfungen im Build

Der Build prüft seine Ergebnisse selbst; schlägt eine Prüfung fehl, gilt
das Ergebnis als nicht gebaut, und nichts, was davon abhängt, wird gebaut
(beim nächsten Lauf wird erneut geprüft):

* jede Library-Datei (`tools/blitzlibcheck`, im selben Schritt wie Link und
  Strip): ein Code-Hunk, Relocs nur dorthin, Library-Kopf, Token-Tabelle
  bis zum Ende lesbar. Verweise aus dem normalen Code in den
  Fehlerprüfbereich sind nur eine Warnung (ohne Debugger zeigen sie hinter
  das Code-Ende; die Originale tun das auch). `acidlibs`/`deflibs` entstehen
  nur aus geprüften Libraries.
* Referenz-Build von blitz2: das Original `archive/sources/blitz2-1.95/blitz2.s`
  mit vasm nach `build/amiga/programs/blitz2/ref/` (`blitz2`, Listing,
  `blitz2.map`, `blitz2.labels`), im selben Schritt `vasmlisting verify`.
* Referenz-Build von TED: das Original `archive/sources/ted-1.1/ted.s` mit
  vasm (wie Devpac) nach `build/amiga/programs/ted/ref/` (`ted`, Listing,
  `ted.map`, `ted.labels`); derselbe Schritt prüft, dass Listing und
  Binary übereinstimmen (`vasmlisting verify`). Referenz für den Port ist
  dieser Build, nicht das TED-Binary der CD (ein anderer Build).
* `ted` nach jedem Link (POST_BUILD): Hunk-Aufbau (`tools/hunkcheck`:
  Offset 0 `bsr.w`, Offset 4 `outside`) und `programs/ted/checklink.cmake`
  (keine libgcc-/libnix-Hilfsfunktionen, die `UtilityBase` bräuchten) und
  `tools/checklabels`: jedes Code-Label von `ted.s` genau einmal als
  `ted.s:NNN label` in `programs/ted/src/` (Vollständigkeit der
  Übersetzung; eine geänderte Kommentarzeile baut das Objekt neu, also
  läuft die Prüfung auch dann).

## Tests

Einige Tests brauchen Originalmaterial, das nicht im Repository liegt (die
CD, das Aminet-Archiv der Quellen): einmal

```
cmake --build --preset amiga --target fetch-originals
```

(lädt etwa 40 MB, siehe `archive/README.md`). Ohne diese Daten werden die
betroffenen Tests übersprungen.

Gebaut wird mit dem Preset `amiga` (Cross-Compiler für AmigaOS; die
Werkzeuge und Host-Tests dafür baut es mit dem nativen Compiler mit).
Die Test-Presets sagen, **wo** die Tests laufen:

```
ctest --preset host       # auf dem Host, ohne Emulator (Sekunden)
ctest --preset emulator   # in Amiberry (Label emulator)
ctest --preset all        # beides
```

* **host:** die Werkzeuge mit absichtlich kaputten Eingaben, die
  Host-Tests des TED-Kerns (nativ übersetzt, snitch) und die Prüfung der
  vorgebauten Libraries. Einzeln: `ctest --test-dir build/amiga/host`.
  Mit `-DHOST_SANITIZE=ON` wird die Host-Seite mit AddressSanitizer und
  UBSan gebaut.
* **emulator:** Läufe in Amiberry; ohne Amiberry oder ohne die Daten in
  `amiga-emulation/` werden sie übersprungen.

Die Orakel-Daten der TED-Tests (`tests/data/ted-oracle.sha256`: SHA-256
von detok, tokit2 und Laden/Speichern für jede Quelle der CD;
`tests/data/ted-tokens.txt`: die Token-Liste) erzeugt der Original-TED
selbst: `programs/ted/tests/oracle` lädt den Referenz-Build von `ted.s` als
Bibliothek, unter dem Original-blitz2 1.95 in Amiberry (braucht die CD):

```
cmake --build --preset amiga --target ted-oracle     # etwa 40 s
cmake -DORACLE_LIMIT=5 build/amiga                   # Probelauf, tests/data bleibt
```

## Emulator

`tools/lib/amiga_emulation` führt AmigaDOS-Skripte in Amiberry aus –
ohne Fenster, isoliert (eigenes Arbeitsverzeichnis, eigene
XDG-Verzeichnisse, schreibgeschütztes Hardfile), deshalb auch parallel:
`amiga_emulation_start()` (Konfiguration = `default.uae` plus Änderungen,
Skript, Zeitlimit), `amiga_emulation_command()` (Steuer-Socket von
Amiberry, z. B. `SCREENSHOT`, `READ_MEM`), `amiga_emulation_join()`. Das
Skript sieht sein Verzeichnis als `TEST:`. Von Hand:

```
echo 'Version >TEST:v.txt' | build/amiga/host-tools/amiga_run/amiga_run -
amiga_run [--rtg] [--visible] [--timeout SEC] [--set KEY=VALUE] [--unset KEY]
          [--mount VOL=DIR] [--mount-ro VOL=DIR] [--dir WORKDIR] SCRIPT|-
```

Voraussetzung: Amiberry ≥ 8.4 im `PATH`, Kickstart-ROM und Hardfiles in
`amiga-emulation/` (siehe dortige README).

Das erste Konfigurieren schreibt `.clangd` (aus `cmake/clangd.in`, mit den
Include-Pfaden der gefundenen Toolchain; nicht im Repository). Nach einem
Wechsel der Toolchain `.clangd` löschen und neu konfigurieren; abschalten
mit `-DBLITZ2_CLANGD=OFF`.

## Struktur

```
CMakeLists.txt, CMakePresets.json
archive/                     unveränderte Originale (Quellen, Referenzdaten),
                             siehe archive/README.md
cmake/m68k-amigaos.cmake     Toolchain (die einzige)
cmake/BlitzLibrary.cmake     blitz_library(), biglib_join()
programs/blitz2/             Compiler (src/: Abschnitte von blitz2.s + Glue)
programs/ted/                Editor (src/: Amiga-Teil, src/core/: OS-freier
                             Kern)
libs/include/                blitzlib.h & Co. (Konventionen der Libraries)
libs/blitzlib.ld             Linker-Skript: ein Code-Hunk, Kopf an Offset 0
libs/acidlibs/               eine C-Datei je Library; prebuilt/: noch nicht
                             portierte Libraries im Original (dhandlerlib)
libs/deflibs/                eine C-Datei je AmigaOS-Library
shared/filereq/              Dateirequester von freqlib und TED (eine Quelle,
                             von beiden mit eigenen Optionen übersetzt)
shared/edstub/               Rahmen für Programme, die blitz2 als blitz2:ted
                             lädt (Einsprünge, comdata, ohne Bildschirm)
host/                        die Host-Seite: alles, was mit dem nativen
                             Compiler gebaut wird (Werkzeuge, Host-Tests),
                             als ExternalProject (build/<preset>/host)
programs/ted/tests/host/     Host-Tests des TED-Kerns (snitch)
programs/ted/tests/oracle/   TED-Orakel: Original-TED als Bibliothek (m68k)
tests/data/                  erzeugte Testdaten (Orakel-Hashes, Token-Liste)
tools/                       Host-Werkzeuge:
  lib/                       die Logik als Libraries: hunk (Hunk-Format
                             lesen), blitzlib (Kopf, Token-Tabelle,
                             Prüfregeln), biglib (BigLib schreiben/lesen),
                             hunkcheck (Bytes im ersten Hunk),
                             amiga_emulation (Skripte in Amiberry),
                             vasmlisting (vasm-Listings lesen), sha256,
                             checklabels (Labels von ted.s im C-Code)
  biglib/, blitzlibcheck/,   die Werkzeuge: je ein main.c um lib/
  hunkcheck/, amiga_run/,
  vasmlisting/, checklabels/
  tests/                     Tests von lib/ mit Eingaben aus dem Speicher
```

Die Libraries werden mit den Blitz-Konventionen übersetzt
(`-ffixed-a3/a4/a5`: Strings, Locals und Globals des Blitz-Programms;
freestanding; ein Hunk), blitz2 und TED als normale Programme mit
`-fbaserel` (a4 = Basis ihrer globalen Daten). Deshalb übersetzen
`libs/acidlibs` (freqlib) und `programs/ted` den gemeinsamen Dateirequester
je selbst; jede Seite liefert dafür ihre `filereq_env.h`.

## Lizenz

GPL-3.0-or-later (`LICENSE`) mit einer Laufzeit-Ausnahme
(`RUNTIME-LIBRARY-EXCEPTION`): Programme, die blitz2 übersetzt, dürfen den
Code, den blitz2 aus den Libraries (`acidlibs`, `deflibs`) und seinen
Code-Vorlagen in sie kopiert, unter beliebiger Lizenz weitergeben;
Compiler, Editor und Libraries selbst bleiben unter der GPL.

Grundlage ist der Quelltext von Blitz Basic 2 und TED 1.1, den Acid
Software als „under GNU“ freigegeben hat (ohne Versionsnummer – nach GPL 2
§ 9 darf man dann jede Version wählen). Die Ausnahme geben die Beteiligten
dieses Projekts für die Rechte, die sie haben; für die übersetzten
Originalteile kann sie mangels erreichbarer Rechteinhaber niemand
verbindlich erteilen. Was über die Lizenz der Originale bekannt ist, steht
in `archive/README.md`.

