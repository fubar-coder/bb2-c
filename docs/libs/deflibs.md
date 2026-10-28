# deflibs in C: AmigaOS-Aufrufdeklarationen

`blitz2:deflibs` (`cd/blitz2/deflibs`) enthält keine Befehle, sondern die
Deklarationen der AmigaOS-Aufrufe (`Open_`, `AllocMem_`, `OpenWindow_` …):
39 Libraries, eine je AmigaOS-Library/-Resource. Sie sind jetzt als C-Quellen
unter `deflibs/NAME.c` reproduzierbar; `cmake --build --preset amiga
--target deflibs` baut daraus `build/amiga/deflibs-c` (BigLib, gleiche
Reihenfolge wie das Original).

## Aufbau einer deflibs-Library

| Teil | Original | C |
|---|---|---|
| Kopf | wie `!libheader`, aber 4 Nullbytes statt `MOVEQ #0,d0:RTS`; `init`/`finit` zeigen auf die beiden Nullsub-Einträge, `return` = 1 (Long), `error` = 0 | `BLITZ_OSLIBHEADER(lib, init, BL_LONGRET, finit)` |
| je Aufruf ein Eintrag Typ 6 | `Dc 6,0,0:Dc baselib,lvo:Dc.b regs,$ff:Even:Dc.l 0:Dc 0:Dc.b name,0,help,0:Even` | `BLITZ_OSCALL(baselib, lvo, "Name_", "(help)", d1, d2, …)` |
| init | Nullsub ohne `!libs` | `BLITZ_NULLSUB_AT(init, libinit, 0, 0)` |
| finit | Nullsub, `!libs eigene Lib,#la1` (eigene Basis in a1) | `BLITZ_NULLSUB_AT(finit, libfinit, 0, 0, LIB_x, BL_LA(1))` |
| Ende | `!libfin`, dann Code und der AmigaOS-Name | `BLITZ_LIBFIN()`, C-Funktionen, `libname[]` |

Registercodes (blitz2.s `stamigalib`): Bit 4 = Adressregister, Bits 0–2 =
Nummer; in C als `d0`…`d7`, `a0`…`a6` geschrieben (`BL_OSR_*`). Den Aufruf
selbst erzeugt der Compiler (Argumente in die Register, a6 = das von `init`
gelieferte, vom Compiler aufbewahrte Basisregister, `JSR lvo(a6)`).

Varianten (vom Generator am Code erkannt, sonst Abbruch):

* **Library** (28×): `OpenLibrary(name, 0)` mit Endlosschleife, finit
  `CloseLibrary(a1)`.
* **Resource** (8×: ciaa, ciab, disk, misc, potgo, battclock, battmem,
  card): `OpenResource(name)` mit Endlosschleife, finit nur `RTS`.
* **CrM.library** (200): einmal `OpenLibrary`, Basis oder 0; finit schließt
  nur bei Basis ≠ 0. Als einzige nicht als Einzeldatei auf der CD.
* **Ohne init/finit** (`doslib2` 249, `graphicslib2` 250): nur weitere
  Aufrufe mit der Basis von 252 bzw. 254.

Dateinamen = `LIB_`-Namen aus `include/libnums.h`. Ausnahmen mit lokalem
`#define`: 225 `mathtranslib` (`LIB_mathtransoslib`; in `libnums.bb` wird
`#mathtranslib` später auf 192 umdefiniert, die Befehls-Lib), 219
`colorwheellib` (dort `#cdtvlib`, reserviert), 2 `xprzmodemlib` (dort nicht
vorhanden). CMake-Ziele heißen `deflib_NAME` (sonst Kollision mit der
Befehls-Lib `mathtranslib`).

## BUG(orig): die init-Schleife

Alle Library-/Resource-Inits (außer CrM) sind

```
init:  MOVE.l  4,a6
       LEA     name(pc),a1
       MOVEQ   #0,d0
       JSR     OpenLibrary(a6)        ; Resourcen: OpenResource
       TST.l   d0
       BEQ     init+6                 ; Mitte des LEA!
       RTS
```

* Schlägt das Öffnen fehl, kehrt `init` nie zurück. Das Sprungziel ist das
  Displacement-Wort des `LEA`; es läuft als `ORI.b #0,(a0)+` (das `MOVEQ`
  ist dessen Immediate) und ruft `OpenLibrary` erneut mit d0 = 0 und dem
  a1, das der fehlgeschlagene Aufruf hinterlassen hat. C behält die
  Endlosschleife (`deflib.h`), allerdings jedes Mal mit dem Namen.
* **Resourcen:** Sie wurden aus der Library-Vorlage gemacht, deren finit
  6 Bytes länger ist; das `LEA` wurde nicht angepasst und zeigt 6 Bytes in
  den Namen (`"ciab.resource"` → `"esource"`, `"potgo.resource"` →
  `"resource"`). `OpenResource` scheitert also immer, init hängt. Ein
  Programm, das einen Aufruf von ciaa/ciab/disk/misc/potgo/battclock/
  battmem/card benutzt, bleibt beim Start stehen. In C erhalten:
  `deflib_openresource(libname + DEFLIB_RESOURCE_SKEW)`.

Der übrige Code unterscheidet sich nur in der Form: GCC sichert a6 (das
Original zerstört es), ruft `CloseLibrary` per `JSR` statt `JMP`, die
Länge des Codeteils ändert sich (deshalb sind Programme, die eine
deflibs-Library benutzen, mit `deflibs-c` nicht bytegleich).

## Entstehung und Reihenfolge

* `tools/mkdeflibs.py` (einmalig, nur Standardbibliothek): liest
  `cd/blitz2/deflibs`, prüft Kopf, Einträge und den Code gegen die
  bekannten Vorlagen und schreibt `deflibs/NAME.c`; gibt die Reihenfolge
  für `DEFLIBS_ORDER` in `CMakeLists.txt` aus. Danach sind die C-Dateien
  die Quelle; `deflibs/deflib.h` (gemeinsamer init-Code) ist von Hand.
* Die 38 Dateien in `cd/extras/blitzlibs/amigalibs/` sind byte-identisch
  mit den Einträgen von `deflibs` (CrM fehlt dort).
  `makedeflibs.bb2` hängt einfach alle Dateien der Unterverzeichnisse von
  `blitzlibs:` in `ExNext`-Reihenfolge aneinander; die Reihenfolge in
  `deflibs` ist genau die FFS-Hash-Reihenfolge der Dateinamen
  (`(len·13…+toupper(c)) & $7FF mod 72` steigt monoton: 0, 0, 4, 12, …, 71;
  `CrM.library1` passt mit Hash 26 an seine Stelle).
* `tools/biglib.py join OUT LIB…` setzt die BigLib-Datei zusammen.

## Prüfungen

* `tools/biglib.py compare` Original gegen C, alle 39 Libraries:
  **39/39 Deklarationen identisch** (Kopf, alle Einträge, Relocs des
  Deklarationsteils); Reihenfolge identisch.
* Emulator (`compilerlauf/tools/corpus_check.py --deflibs
  libs-c/build/amiga/deflibs-c`, acidlibs aus den committeten C-Libs,
  eine Emulator-Sitzung je Lauf): ohne t4*/t6*/t7* mit 37 C-Libs
  **53 Prüfungen, 0 Unterschiede**; erneut mit 40 C-Libs (dazu printlib,
  inputoutputlib, editlib), ohne t6*/t7*/t8*: **66 Prüfungen, 0
  Unterschiede**. Die Korpus-Tests benutzen keine
  `_`-Aufrufe direkt, aber die Befehls-Libs ziehen über `!libs` doslib,
  intuitionlib und graphicslib (z. B. fileiolib, environlib, intlib);
  deren C-init/finit laufen also in jedem Test.
* CD-weit (`compilerlauf/tools/bb2corpus.py --compiler 195 --deflibs …`):
  Tag `195-cdeflibs`, 6 Sitzungen: 498 ok, 217 Fehler, 9 übersprungen –
  **Ergebnis, Zeile und Meldung bei allen 724 Quellen wie
  `reference/195.tsv`**, keine Abstürze. 85 der 498 Executables sind
  bytegleich (maskierter Hash); die übrigen enthalten den init/finit-Code
  einer deflibs-Library und unterscheiden sich daher erwartungsgemäß.
