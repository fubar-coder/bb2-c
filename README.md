# Blitz Basic 2 in C

Der Compiler **blitz2** (Blitz Basic 2 v1.95), der Editor **ted** (TED 1.1)
und die Libraries, die blitz2 beim Start liest (**acidlibs**, **deflibs**),
als C-Übersetzung der Originalquelltexte – gebaut für AmigaOS mit dem
m68k-amigaos-gcc.

Dieses Verzeichnis enthält nur, was zum Bauen nötig ist. Tests, Testkorpus,
Referenz-Builds und Analysewerkzeuge liegen im Arbeitsrepository `bb2`
(Stand der Übernahme: bb2 `7b5df74`). Die Notizen zur Übersetzung
(Analyse der Originale, Abweichungen) stehen in `docs/`.

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
* `ted.sym` (derselbe Link mit Symbolen) nach jedem Link: `tools/checkdata`
  vergleicht die Daten des C-Builds mit dem Referenz-Build von `ted.s`
  (`programs/ted/checkdata.tsv`: 534 Einträge – Fonts, Mauszeiger,
  Optionsblock, Texte, Screen, Fenster, Gadgets, Menüs, Requester;
  relozierte Langworte der Referenz übersprungen). `ted` wird erst danach
  gelinkt.

## Tests

Einige Tests brauchen Originalmaterial, das nicht im Repository liegt (die
CD, das Aminet-Archiv der Quellen): einmal

```
cmake -P cmake/fetch-originals.cmake
```

(lädt etwa 40 MB, siehe `archive/README.md`; braucht kein
Build-Verzeichnis). In einem schon konfigurierten Build geht auch
`cmake --build --preset amiga --target fetch-originals`. Ohne diese Daten
werden die betroffenen Tests übersprungen.

Gebaut wird mit dem Preset `amiga` (Cross-Compiler für AmigaOS; die
Werkzeuge und Host-Tests dafür baut es mit dem nativen Compiler mit).
Die Test-Presets sagen, **wo** die Tests laufen:

```
ctest --preset host       # auf dem Host, ohne Emulator (Sekunden)
ctest --preset emulator   # in Amiberry (Label emulator)
ctest --preset all        # beides
```

* **host:** die Werkzeuge mit absichtlich kaputten Eingaben, die
  Host-Tests des TED-Kerns (nativ übersetzt, snitch), die Prüfung der
  vorgebauten Libraries, der Deklarationsvergleich (`declarations_*`:
  Kopf, Einträge, `!libfin` jeder gebauten Library gegen das Original der
  CD, gleiche Libraries in gleicher Reihenfolge; `tools/blitzlibcompare`)
  und der Befehlsvergleich (`asmcmp_*`: die in Assembler gebliebenen
  Routinen von runerrlib, debuglib, memacclib gegen die Originale der CD,
  mit `objdump` der Toolchain; `tools/asmcmp`). Die beiden Vergleiche
  werden ohne CD übersprungen. Die Host-Seite einzeln:
  `ctest --test-dir build/amiga/host`.
  Mit `-DHOST_SANITIZE=ON` wird die Host-Seite mit AddressSanitizer und
  UBSan gebaut.
* **emulator:** Läufe in Amiberry; ohne Amiberry oder ohne die Daten in
  `amiga-emulation/` werden sie übersprungen. `libs_emulator`: das
  Testprogramm `libs/tests/libs_tests` (snitch, m68k) lädt jede acidlib
  dieses Builds und ihr Original der CD wie der Compiler und vergleicht
  Ergebnisse, Register und Speicher (ohne CD übersprungen; Auswahl:
  `cmake ... -DFILTER="[mathstr]" -P libs/tests/emulator.cmake`). Darunter
  der Korpus
  (`tests/corpus/`): `corpus` (67 Testprogramme ohne Laufzeit-
  Fehlerprüfung), `corpus_runerr` (131 mit), `corpus_req` (3 mit
  blockierenden Requestern, hinter der Sperre `reqclosetest`),
  `corpus_serial` / `corpus_serial_runerr` (ein Programm, das über die
  emulierte serielle Schnittstelle mit einer Gegenstelle auf dem Host
  spricht; verglichen werden auch die Bytes, die es über die serielle
  Schnittstelle sendet). Je
  Durchgang übersetzen drei Konfigurationen dieselben Quellen und führen
  die Programme aus: ref (Original-blitz2 1.95, Libraries der CD), port
  (blitz2-c, Libraries der CD: Executable gleich, Ausgabe gleich), clib
  (blitz2-c mit den C-Libraries: Ausgabe gleich). Ohne CD übersprungen.
  `CORPUS_PARALLEL` (Standard 3 unter Linux, sonst 1) begrenzt die
  gleichzeitigen Emulatoren, `CORPUS_TIMEOUT` eine Sitzung; Berichte in
  `build/<preset>/tests/corpus/NAME.txt`. Die Durchgänge nicht mit
  `ctest -j` parallel starten: jeder nutzt selbst bis zu
  `CORPUS_PARALLEL` Emulatoren. `corpus_cd`: alle 724 Quellen der CD mit
  blitz2-c übersetzt (wie bb2s bb2corpus: `.xtra` der Quellen, Assigns auf
  CD-Verzeichnisse, Verzeichnis der relativen INCLUDEs) und mit
  `tests/data/blitz2-cd.tsv` verglichen (Ergebnis, Fehlerzeile und
  -meldung, Größe, Hash mit maskierten trap-Blöcken). Die Tabelle erzeugt
  das Original-blitz2 1.95:

  ```
  cmake --build --preset amiga --target blitz2-cd-reference   # etwa 35 s
  ```

  TED-A/B (`tests/ted-ab/`): `ted_ab` spielt die Skripte
  `tests/ted-ab/scripts/*.drv` mit `teddrive` gegen TED, in vier
  Kombinationen (R5: Original-TED + blitz2 1.95, C5: TED-C + 1.95, Cc:
  TED-C + blitz2-c, Rc: Original-TED + blitz2-c), prüft die Erwartungen
  der Skripte und vergleicht die Läufe paarweise (Ergebnis, Requester,
  Bildschirm-Dumps, Dateien). Braucht den ref-Lauf von `corpus` (Fixture,
  ctest startet ihn mit). Auswahl: `-DTED_AB_FILTER=02-*,29-cmp-*`,
  `-DTED_AB_COMBOS=R5,C5`; Bericht in `build/<preset>/tests/ted-ab/
  ted_ab.txt`, etwa 13 Minuten. Bekannte Fehler (`; @known GRUND` im
  Skript) laufen nicht mit und stehen als KNOWN im Bericht; ein exakter
  Name im Filter führt sie trotzdem aus. `ted_ab_lint` prüft die Skripte
  ohne Emulator.

Die Orakel-Daten der TED-Tests (`tests/data/ted-oracle.sha256`: SHA-256
von detok, tokit2 und Laden/Speichern für jede Quelle der CD;
`tests/data/ted-tokens.txt`: die Token-Liste) erzeugt der Original-TED
selbst: `programs/ted/tests/oracle` lädt den Referenz-Build von `ted.s` als
Bibliothek, unter dem Original-blitz2 1.95 in Amiberry (braucht die CD):

```
cmake --build --preset amiga --target ted-oracle     # etwa 40 s
cmake -DORACLE_LIMIT=5 build/amiga                   # Probelauf, tests/data bleibt
```

## Lesbare Quellen der Libraries

Die Blitz-Quellen der Libraries (`bblibsrc.lha` im Aminet-Archiv,
entpackt nach `archive/aminet/amibl2src/bblibsrc/`) sind tokenisiert. Der
Build legt sie als Text ab, so wie TED sie zeigt – **Zeile N der Datei =
Zeile N in TED**; auf diese Zählung beziehen sich Zeilenangaben wie
„bblibsrc/windowslib Zeile 1277“ in den Issues:

```
cmake -P cmake/fetch-originals.cmake          # einmal (siehe Tests)
cmake --build --preset amiga --target bblibsrc
build/amiga/bblibsrc/NAME                     # z. B. windowslib, palettelib.aga
```

Gleicher Name wie im Archiv (ohne Endung, wo das Archiv keine hat);
ausgelassen sind `NAME.xtra` (TEDs Editorzustand) und die Daten-/
Objektdateien `*.bin`, `*.inc`, `*.obj`. Das Ziel gehört zu `ALL`, sobald
das Archiv da ist; ohne Archiv meldet es nur, dass es übersprungen wird.
Die Dateien sind byte-gleich zu bb2 `libs-c/bb2src/NAME.bb` (Verweise
`windowslib.bb:NNN` meinen also dieselbe Zeile); bb2 hatte nur 82 der
102. Mehrere Fassungen derselben Library: `palettelib`/`palettelib.aga`,
`displaylib`/`displaylibfuk`/`olddisplaylib`, `gtlib`/`oldgtlib`,
`switchlib`/`switchlib.ok`, `wbstartuplib`/`wbstartuplibnew`,
`2dlib`/`sis2dlib.bb`.

Das Werkzeug dahinter ist `teddetok` (`programs/ted/tools/teddetok`, auf
dem C-Kern von TED wie `tedtok`): lädt eine Quelle wie TED (`fileopen2`)
und schreibt jede Zeile detokenisiert. Ein Token, das TED nicht kennt
(TED zeigt `?????`), ist ein Fehler, die Datei wird dann nicht
geschrieben; eine Zeile, die länger ist als TEDs Zeilenpuffer, teilt TED
beim Laden – `teddetok` meldet das und zählt wie TED.

```
build/amiga/host/ted-tools/teddetok/teddetok tests/data/ted-tokens.txt IN OUT
build/amiga/host/ted-tools/teddetok/teddetok tests/data/ted-tokens.txt --check IN...
```

ctest `teddetok_*` prüft es mit kleinen Eingaben (auch kaputten: unbekanntes
Token, Token-Byte am Dateiende) und `teddetok_bblibsrc` alle Quellen von
`bblibsrc/` (ohne Archiv übersprungen).

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
libs/tests/                  libs_tests: die Libraries gegen ihre Originale
                             im Emulator (m68k, snitch)
shared/filereq/              Dateirequester von freqlib und TED (eine Quelle,
                             von beiden mit eigenen Optionen übersetzt)
shared/edstub/               Rahmen für Programme, die blitz2 als blitz2:ted
                             lädt (Einsprünge, comdata, ohne Bildschirm)
host/                        die Host-Seite: alles, was mit dem nativen
                             Compiler gebaut wird (Werkzeuge, Host-Tests),
                             als ExternalProject (build/<preset>/host)
programs/ted/tests/host/     Host-Tests des TED-Kerns (snitch)
programs/ted/tools/          Host-Werkzeuge auf dem TED-Kern: tedtok
                             (tokenisieren wie TED), teddetok (Quelle wie
                             TED sie zeigt; Ziel bblibsrc)
programs/ted/tests/oracle/   TED-Orakel: Original-TED als Bibliothek (m68k)
tests/data/                  erzeugte Testdaten (Orakel-Hashes, Token-Liste,
                             CD-Referenz von blitz2 1.95)
tests/ted-ab/                TED-A/B: Original-TED gegen TED-C im Emulator;
                             amiga/: teddrive, teddrivetest, tedprobe
                             scripts/: die Tests (.drv), src/: Quellen,
                             runner/: tedab (die Läufe), teddump
                             (Bildschirm-Dumps), beide Host; host/: Tests
tests/corpus/                Korpus: blitz2-c gegen blitz2 1.95 im Emulator;
                             amiga/: tedstub (blitz2:ted für Batch-Läufe),
                             reqtrap, reqclose, reqclosetest
                             tests*/: die Testprogramme (Klartext, .bas),
                             runner/: corpusrun (ein Durchgang), cdrun (die
                             CD durch einen Compiler), beide Host
tools/                       Host-Werkzeuge:
  lib/                       die Logik als Libraries: hunk (Hunk-Format
                             lesen), blitzlib (Kopf, Token-Tabelle,
                             Prüfregeln), biglib (BigLib schreiben/lesen),
                             hunkcheck (Bytes im ersten Hunk),
                             amiga_emulation (Skripte in Amiberry),
                             vasmlisting (vasm-Listings lesen), sha256,
                             checklabels (Labels von ted.s im C-Code),
                             checkdata (Daten gegen den Referenz-Build),
                             asmcmp (Assembler-Routinen gegen Originale);
                             blitzlib vergleicht auch Deklarationen
  biglib/, blitzlibcheck/,   die Werkzeuge: je ein main.c um lib/
  blitzlibcompare/
  hunkcheck/, amiga_run/,
  vasmlisting/, checklabels/,
  checkdata/, asmcmp/
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

