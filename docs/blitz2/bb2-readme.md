# blitz2-c – Blitz-Basic-2-Compiler v1.95 in C

C-Übersetzung von `blitz2.s` (Blitz Basic 2 v1.95, Acid Software 1995,
Devpac-Assembler, ~16 000 Zeilen + Includes) aus
`../quelltext/amibl2src/files/amiblitz2source/bb2src.zip.dir/`.

Zweck: **Lesbarkeit für den Kompatibilitätsvergleich mit AmiBlitz3.** Deshalb
gilt Treue vor Eleganz: gleiche Algorithmen, gleiche Datenlayouts, gleiche
Eigenheiten, die originalen Labels als C-Namen, Zeilenverweise
`/* blitz2.s:NNNN */` an jeder Funktion.

## Bauen

Toolchain: bebbo `m68k-amigaos-gcc` in `/opt/amiga` (libnix, `-noixemul`,
68000); Bauen der Toolchain unter Arch/CachyOS:
[../toolchain.md](../toolchain.md).

```
cmake --preset amiga
cmake --build --preset amiga                    # -> build/amiga/blitz2
cmake --build --preset amiga --target headers   # passen alle Header zusammen?
cmake --build --preset amiga --target listing   # vasm-Listing des Originals
```

Globale Variablen werden a4-relativ adressiert (`-fbaserel`, Option
`BLITZ2_BASEREL`, Standard an): kürzere Befehle, weniger Relocations. Alle
Einsprünge von außen (Editor-Callbacks, `stopcode`, `trap #15`) laden a4 in
`glue*.c` (`GLUE_LOAD_A4`). Release-Builds werden ohne Symbole gelinkt.
Größe: 96 KB statt 127 KB (Original 2.10: 86 KB); CD-Referenzlauf und
Testkorpus danach unverändert gleich.

## Aufbau

Jede Datei entspricht einem Zeilenbereich von `blitz2.s` (in Quellreihenfolge):

| Datei | blitz2.s | Inhalt |
|---|---|---|
| `src/main.c` | 1–1911, wbheader, asmmac | Start, Tokenliste, edstruct, Editor-Wrapper `ed_*`, Menüs, Programmstart, Asm-Prüfroutinen |
| `src/compile.c` | 1912–3290 | `compile`, `dopass`, Makroexpansion, STOP-Requester, Statement-Dispatch (`atokens`) |
| `src/stmts.c` | 3291–4999 | Statements, Direktmodus, Objektpuffer, Residents, Asm-EA-Parser |
| `src/asmlibs.c` | 5000–6739 | Inline-Assembler, Parameter, `uselib`, Laden der Libraries |
| `src/parse.c` | 6740–8450 | Statics/Data, `savefile`, `getchar`, `err` + Fehlermeldungen, Namen, Variablen |
| `src/eval.c` | 8451–10683 | Code-Emitter `poke*`, `convtype`, Ausdrucksauswertung, Operatoren |
| `src/libcall.c` | 10684–12135 | Zahlen, Lib-Aufrufe, Includes |
| `src/tokens1.c` | 12136–13326 | eingebaute Befehle Teil 1 (DefType … If/While) |
| `src/tokens2.c` | 13327–14655 | eingebaute Befehle Teil 2 (Then … Dim) |
| `src/data.c` | 14656–Ende, options*.src, typereq, calcreq | Globals, Typen, Requester, Optionen, `.xtra` |
| `src/structs.h` | – | alle Heap-Knoten, Offsets per `_Static_assert` gegen das Original geprüft |
| `src/glue.c`, `src/glue_direct.c` | – | **der einzige Assembler**: Register-Thunk `callregs`, Einstiegsstubs für Editor/exec, VBR, Trap-#15-Handler des Direktmodus |

Jede `NAME.c` hat eine `NAME.h` (Vertrag für die anderen Teile);
`src/blitz2.h` bindet alle ein.

## Lesehilfen

* `BUG(orig): …` – Fehler/Eigenheit des Originals, bewusst beibehalten.
* `DIFF: …` – bewusste Abweichung, nur wo C das Gleiche nicht kann
  (Stacktricks, Registerrahmen); der Kommentar sagt, was das Original tat.
* Register, die im Original dauerhaft Zustand tragen, sind Globals:
  `srcptr` = a5 des Parsers, `numd3`/`numd6` = d3/d6 nach Zahlen-Fetch,
  `getvnamelen` = d2 nach `getvname`. Bedingungscodes als Ergebnis wurden zu
  Rückgabewerten (Abbildung steht im jeweiligen Header).
* Code-Templates (Maschinencode, den der Compiler ins erzeugte Programm
  kopiert) sind beschreibbare `UWORD`-Arrays mit den Originalbytes, erzeugt
  aus dem vasm-Listing (`tools/asmlisting.py`) und gegen das Binary geprüft.
* `CONVENTIONS.md` – Regeln der Übersetzung; `notes/` – Analysen je Abschnitt
  (`survey-NN.md`), Querverweise, Änderungsprotokoll, Reviews.

## Stand der Prüfung

Jeder Abschnitt wurde nach der Übersetzung von einem unabhängigen Durchgang
Routine für Routine gegen den Assembler geprüft (Flags, Sprünge,
erzeugte Opcodes, Templates, Datenlayouts, Verträge zwischen den Teilen);
Befunde und Korrekturen in `notes/review-NN.md`. Datenbereiche (Tokenliste,
Requester, Templates, Fehlertexte) wurden per Skript bytegenau mit dem
Original verglichen. **Nicht** geprüft ist das Laufzeitverhalten.

## IDE

CMake-Projekt direkt in CLion öffnen (die Toolchain wird automatisch
gesetzt). `.clangd` lässt clangd die Quellen als m68k parsen.

## Werkzeuge

* CMake-Ziel `listing` – vasm-Listing des Originals (Adressen + Opcodes),
  `build/amiga/blitz2.lst`.
* `tools/asmlisting.py` – Bytes/Templates eines Label-Bereichs als C-Array.
* `tools/xrefs.py` – welche Labels jedes Abschnitts andere Abschnitte nutzen.
* `tools/req2c.py` – Intuition-Requester-Daten → NDK-Structs.
* `cmake -P tools/checkfile.cmake src/NAME.c` – eine Datei mit den Flags aus
  `build/amiga/compile_commands.json` übersetzen (parallel nutzbar).

## Laufen lassen

Nicht getestet. Das Programm ist ein Ersatz für `blitz2:blitz2` und braucht
dieselbe Umgebung wie das Original: `blitz2:ted` (Editor, wird per LoadSeg
geladen und ruft den Compiler über `edstruct` zurück),
`blitz2:deflibs`, `blitz2:acidlibs`, `blitz2:userlibs/`,
`blitz2:defaultdbug` – alles in `../cd/blitz2/`. Test in Amiberry: `blitz2:`
auf eine Kopie von `cd/blitz2/` legen und dort `blitz2` durch
`build/amiga/blitz2` ersetzen.

Einschränkungen: Der erzeugte Code wird teils noch zur Compile-Zeit
ausgeführt (Konstantenfaltung, `evalconst2`) – der Compiler läuft also nur
auf einem 68k-Amiga bzw. Emulator. Das Binary auf der CD ist v2.10; der
Quelltext ist v1.95.
