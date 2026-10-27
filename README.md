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
* `ted` nach jedem Link (POST_BUILD): Hunk-Aufbau (`tools/hunkcheck`:
  Offset 0 `bsr.w`, Offset 4 `outside`) und `programs/ted/checklink.cmake`
  (keine libgcc-/libnix-Hilfsfunktionen, die `UtilityBase` bräuchten).

`ctest --preset amiga` führt die Tests der Host-Werkzeuge aus (mit
absichtlich kaputten Eingaben) und prüft die vorgebauten Libraries; die
Tests mit dem Emulator haben das Label `emulator` (`-LE emulator` lässt sie
weg) und werden ohne Amiberry oder ohne die Daten in `amiga-emulation/`
übersprungen.

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
cmake/m68k-amigaos.cmake     Toolchain (die einzige)
cmake/BlitzLibrary.cmake     blitz_library(), biglib_join()
programs/blitz2/             Compiler (src/: Abschnitte von blitz2.s + Glue)
programs/ted/                Editor (src/: Amiga-Teil, src/core/: OS-freier
                             Kern, resources/: Originaldateien für #embed)
libs/include/                blitzlib.h & Co. (Konventionen der Libraries)
libs/blitzlib.ld             Linker-Skript: ein Code-Hunk, Kopf an Offset 0
libs/acidlibs/               eine C-Datei je Library; prebuilt/: noch nicht
                             portierte Libraries im Original (dhandlerlib)
libs/deflibs/                eine C-Datei je AmigaOS-Library
shared/filereq/              Dateirequester von freqlib und TED (eine Quelle,
                             von beiden mit eigenen Optionen übersetzt)
tools/                       Host-Werkzeuge (eigener nativer Build):
  lib/                       die Logik als Libraries: hunk (Hunk-Format
                             lesen), blitzlib (Kopf, Token-Tabelle,
                             Prüfregeln), biglib (BigLib schreiben/lesen),
                             hunkcheck (Bytes im ersten Hunk),
                             amiga_emulation (Skripte in Amiberry)
  biglib/, blitzlibcheck/,   die Werkzeuge: je ein main.c um lib/
  hunkcheck/, amiga_run/
  tests/                     Tests von lib/ mit Eingaben aus dem Speicher
```

Die Libraries werden mit den Blitz-Konventionen übersetzt
(`-ffixed-a3/a4/a5`: Strings, Locals und Globals des Blitz-Programms;
freestanding; ein Hunk), blitz2 und TED als normale Programme mit
`-fbaserel` (a4 = Basis ihrer globalen Daten). Deshalb übersetzen
`libs/acidlibs` (freqlib) und `programs/ted` den gemeinsamen Dateirequester
je selbst; jede Seite liefert dafür ihre `filereq_env.h`.
