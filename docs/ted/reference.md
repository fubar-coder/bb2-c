# Referenz-Build von TED 1.1 und die Binär-Includes

## 1 Referenz

**Referenz für den Port ist der vasm-Build des Quelltexts `ted.s` (TED 1.1)
und sein Listing** – nicht das Binary der CD. Den Abgleich mit den
CD-Versionen (TED, Compiler, Libraries) macht ein eigener, späterer Schritt;
er ist hier ausdrücklich nicht Gegenstand.

```
cmake --build --preset amiga --target ref
→ build/amiga/ref/ted        vasm-Build (Hunk-Executable, 57060 Bytes)
→ build/amiga/ref/ted.lst    vasm-Listing (-Lall)
→ build/amiga/ref/ted.map    TSV: Adresse  Datei:Zeile  Bytes  Quelltext (7280 Zeilen)
→ build/amiga/ref/ted.labels TSV: Adresse  Label  Datei:Zeile (1103 Symbole)
```

Aufruf wie beim Compiler (`compilerlauf/blitz2-195`):
`vasmm68k_mot -quiet -devpac -nocase -m68000 -Fhunkexe` (`-nocase` wie
`GenAm.opts`), Warnungen 40/41/51/62 aus wie beim Listing von blitz2-c (CMake-Ziel `listing`).
Assembliert wird `tedt/ted.s`: Es unterscheidet sich von `ted.s` nur in den
Include-Pfaden (Zeile 12 `e:myoffs`, Zeile 136 `e:wbheader` – `e:` war eine
Devpac-Zuweisung); alle Includes und Binärdateien in `tedt/` sind gleich denen
in `bb2src.zip.dir/`, die Zeilennummern sind gleich (Listing-Zeile =
`ted.s:NNN`).

Ergebnis: ein Hunk (HUNK_CODE, Abschnitt `CODE` 0–`$D4FA`, `$353F`
Langwörter), 1245 Relocations (vasm schreibt sie als `HUNK_RELOC32SHORT`),
keine Fehler. Listing und Binary stimmen überein (`tedlisting.py verify`:
7280 Zeilen, 0 Abweichungen; läuft als `ctest` `ref_verify`).

## 2 Werkzeuge (`ted-c/tools/`)

| Datei | Zweck |
|---|---|
| `tedlisting.py LST …` | `map`, `labels`, `addr LABEL`, `line ADDR`, `bytes A B`, `carray NAME A B` (konstantes `UWORD`-Array mit `ted.s:NNN`-Kommentaren, für Tabellen/Daten), `verify EXE` (jedes Listing-Byte gegen den Code-Hunk eines Binaries, relozierte Langwörter ausgenommen) |
| `xrefs.py ORIGDIR` | Querverweise zwischen den Chunks → `notes/xrefs.md` |

CMake: Ziel `ref` (in `ALL`: vasm-Build mit Listing, dann `ted.map`/`ted.labels` und `verify`; bis 27.10.2026 `tools/mkref.sh`), `listing`; `ctest` führt `ref_verify` aus.

## 3 Die Binär-Includes

| Label (Zeile) | Datei | Bytes | Adresse | Art | Entscheidung |
|---|---|---|---|---|---|
| `sleep` (316) | `sleep.bin` | 704 | `$656` | Daten: 8 Sprite-Bilder der Sanduhr („busy“), je 88 Bytes = 2 Kontrollwörter + 20 Zeilen × 2 Wörter + Endwort | **Daten** – Ressource |
| `pointer` (319) | `pointer.bin` | 64 | `$916` | Daten: Sprite-Bild des normalen Zeigers (14 Zeilen, 16 px, 2 Bitplanes) | **Daten** – Ressource |
| `srcinfo` (827) | `src.info` | 396 | `$1074` | Daten: `DiskObject` (Magic `$E310`, Version 1) – Icon, das beim Speichern als `NAME.info` geschrieben wird | **Daten** – Ressource |
| `rawed` (7109) | `rawed.obj` | 660 | `$73E0` | **Code**: komplette AmigaDOS-Hunk-Datei (Header 32 Bytes, 624 Bytes Code, keine Relocations), Einsprünge `rawed+32+0/4/8` | **portieren** (C, `rawkey.c`) |
| `filerequest` (7967) | `filereq` | 15916 | `$8CE0` | **Code**: komplette Hunk-Datei (14616 Bytes Code+Daten, 312 RELOC32), zur Laufzeit von `flink` (7953) selbst reloziert, Einsprung `filerequest+36` = Code+4 | **portieren** durch Übernahme von `libs-c/libs/freqlib.c` (s. 4.2) |
| `font6` (8070) | `chars.b` | 768 | `$CDFA` | Daten: Zeichensatz 8 Pixel breit, 6 hoch, 128 Zeichen in 8 Blöcken zu 16 (Zeile r von Zeichen c bei `(c&15) + (c>>4)*96 + r*16`, `fontcalc` 5214) | **Daten** – Ressource |
| `font8` (8071) | `chars8.b` | 1024 | `$D0FA` | Daten: Zeichensatz 8×8 Pixel, 128 Zeichen (`(c&15) + (c>>4)*128 + r*16`) | **Daten** – Ressource |

Die reinen Daten bleiben binär: Sie werden beim Build aus den Originaldateien
in C-Arrays umgesetzt (PLAN.md §3.4), nicht abgetippt und nicht als Kopie ins
Repository gelegt. Sprite-Daten liegen in TED **nicht** im Chip-RAM:
`mouseinit` (184) kopiert `pointer … finish` in einen `AllocMem(MEMF_CHIP)`-
Block; das bleibt so (C: normales `const`-Array + dieselbe Kopie).

### 3.1 `rawed.obj` – Rawkey → ASCII

Kein Quelltext vorhanden: Weder in `quelltext/` noch auf der CD gibt es
`rawed.s`/`rawmaker` (`ted.s:7107` verweist auskommentiert auf
`rad:rawmaker.obj`); `bb2src.zip.dir/rawed.obj.dsm` ist eine
DSM-Disassemblierung (1988er Disassembler, also schon beim Autor vorhanden).
Dieselbe Umsetzungsroutine (Bytefolge ab Code+`$10A`) steckt auch in
`cd/blitz2/acidlibs`, `defaultdbug`, `help` und mehreren PD-Programmen –
offenbar eine Standardroutine der Autoren; eine Quelle dazu liegt in
`bblibsrc` nicht vor.

Aufbau (Offsets relativ zu `rawed+32`):

| Off. | Inhalt |
|---|---|
| `$000` | `bra $174` – **init** (`ted.s:39 bsr rawed+32`) |
| `$004` | `bra $10A` – **convert** (`ted.s:5928 bsr rawed+32+4`) |
| `$008` | `bra $20A` – **finit** (`ted.s:128 fail5 bsr rawed+32+8`) |
| `$00C` | `"console.device",0` |
| `$01C` | `ConsoleDevice`-Basis (l) · `$020` Ausgabepuffer (w) · `$022` `InputEvent` (22 Bytes, `ie_Class` = 1 = RAWKEY) |
| `$038`/`$0A1` | 2 Tabellen à 105 Bytes (Rawkey `$00–$68` ohne/mit Shift) |
| `$10A` | convert: ein d0.w = Rawkey, d1.w = Qualifier; `$50–$59` (F1–F10) → d0+`$31` = `$81–$8A`; `$4C–$4F` (Cursor) → d0−`$30` = `$1C–$1F`; `$5F` (Help) → `$8B`; sonst `RawKeyConvert(ie, buf, 1, NULL)` (console −48), Rückgabe das eine Zeichen, 0 bei −1 (Überlauf) oder ohne Zeichen. aus: d0.w = d1.w = Zeichen; a0–a2/a6 gesichert |
| `$174` | init: 82 Bytes ab `$218` löschen, `IOStdReq` (`$218`, `mn_Length` 48) + `MsgPort` (`$248`, `mp_SigTask` = `FindTask(0)`, `mp_SigBit` = `AllocSignal(-1)`), `OpenDevice("console.device", -1, io, 0)` (Ergebnis nicht geprüft), Basis = `io_Device`; dann beide Tabellen über convert füllen |
| `$20A` | finit: `CloseDevice(io)` |
| `$26A` | `moveq #0,d0 / rts / $000F` – Rest (gleicher Rest wie am Ende von `filereq`: Zufallsinhalt des Assemblerpuffers) |

Auffälligkeiten (später `BUG(orig)`): Die beiden Tabellen werden gebaut,
aber nie gelesen (weder in rawed noch in `ted.s`, das nur `+32/+36/+40`
benutzt); das Signalbit wird nie freigegeben; F-Tasten/Cursor/Help ignorieren
den Qualifier (Shift/Alt kommen über `qualifier`, das `ted.s` selbst
auswertet); ein Fehlschlag von `OpenDevice` wird nicht erkannt (dann ruft
convert eine Nullbasis).

**Entscheidung: portieren.** 624 Bytes reiner Exec/Console-Code ohne
Tricks, nur PC-relative Daten; in C mit `OpenDevice`/`RawKeyConvert`
(`<devices/conunit.h>`, `proto/console.h`) direkt nachbildbar. Asm würde
nur eine Register-Schnittstelle (d0/d1→d0) erhalten, die ausschließlich
TED-intern ist. Die Tabellen werden mitgebaut (gleiche Seiteneffekte:
105×2 `RawKeyConvert`-Aufrufe), aber als ungenutzt markiert.

### 3.2 `filereq` – Dateirequester

Kein Quelltext (`ted.s:7969 ;include rad:filerequester`), **aber der Code
ist derselbe wie im Dateirequester von Blitz’ `freqlib`** (`FileRequest$`,
`bblibsrc/freqlib` bindet `libincs:filereq.s` ein): Code+Daten von `filereq`
(14616 Bytes) stimmen mit `cd/blitz2/acidlibs` ab Offset `$114FA` bis auf
7 Bytes überein, alle in Füllbytes hinter Gadget-Texten (`"OK",0,$06` statt
`"OK",0,$00` usw.) bzw. im Rest am Hunkende. Dieser Requester ist in
`libs-c/libs/freqlib.c` bereits nach C übersetzt (aus der Disassemblierung,
mit allen BUG(orig)-Vermerken, u. a. Schreiben hinter den Datenblock bei
Fenstern ab 605 Pixel Höhe – das betrifft TED auf hohen Bildschirmen genauso).

Einbindung in TED:

* `flink` (7953, einmal beim Start aus `outside`, Zeile 76) reloziert den
  eingebetteten Hunk **selbst**: `a0 = filerequest+28` (Hunklänge), Basis =
  Code-Anfang (`filerequest+32`), dann die RELOC32-Liste hinter dem Code:
  `add.l basis,(code+offset)` für alle 312 Einträge. Das ist
  selbstmodifizierender Code im Code-Hunk von TED (nur einmal, vor jeder
  Benutzung; nicht wiedereintrittsfest, ein zweiter Aufruf würde doppelt
  relozieren).
* `ezfreq` (3502, auch `comdata+24` für blitz2): `reqstruct` = {a0 Pfad, a1
  Name, a2 Titel, Screen} (das vierte Langwort setzt `screenopen`, ted.s:4675 – der Requester öffnet auf TEDs Bildschirm), `a0 = reqstruct`, `bra filerequest+36` (= Code+4,
  überspringt das `lea default(pc),a0` am Code-Anfang). Dasselbe Einsprung-
  und Strukturformat benutzt `freqlib` (`bsr freqcode+4`).

**Entscheidung: portieren durch Wiederverwendung („Variante C“, Benutzer,
Phase 2a umgesetzt).** Eine Quelle für beide Programme, von jedem mit den
eigenen Optionen übersetzt (seit 27.10.2026 als eigene Übersetzungseinheit;
bis dahin `libs-c/libs/freqlib_req.inc`, per `#include` eingebunden):

* `shared/filereq/filereq_core.c` – der Requester-Kern: Code, Datenblock
  `fq` (`struct fq_data`: NewWindow, Gadgets, Borders, IntuiTexts,
  StringInfo/PropInfo/Images als NDK-Strukturen, Variablen und Puffer im
  Layout der freqlib, jeder Offset per `_Static_assert` geprüft; Zeiger als
  statische Initialisierer `&fq.…` = dieselben Relocs). Schnittstelle in
  `filereq_core.h`: Parameterblock `ULONG fq_args[4]` = {Pfad, Name, Titel,
  Screen} (= TEDs `reqstruct`), Einsprung `ULONG fq_request(void)`; alles
  andere `static`. Jedes Programm liefert `filereq_env.h`
  (exec-Deklarationen, `fq_divu_w` mit DIVU.w-Semantik). Zweimal übersetzt,
  weil die Optionen unvereinbar sind: in der Library `-ffixed-a3/a4/a5`,
  freestanding, ein Hunk; in TED `-fbaserel` (a4 = Basisregister der
  Globals) und libnix.
* `libs-c/libs/freqlib.c` – nur die BlitzLib-Hülle (`dofreq`/`freqchk`);
  `libs-c/libs/filereq_env.h`: `blitzlib.h`, `blitzexec.h`,
  `fq_divu_w` = `bl_divu_w`. Die Deklarationen sind identisch zum Original
  (`biglib.py compare ref/freqlib.obj …`). Mit `struct fq_data` statt des
  Assembler-Blocks ist der Datenblock (Bytes + Relocs) gleich geblieben,
  der Code aber kürzer: das Byte-Array `fq_data[]` hatte für GCC
  Ausrichtung 1, jeder Wort-/Langwortzugriff wurde aus Einzelbytes
  zusammengesetzt (freqlib $4A00 → $3F20, TEDs filereq-Code $1600 → $CA4
  Bytes; Host-Vergleich in unicorn: 27 Szenarien gleich).
* `ted-c/src/filereq.c` – `filerequest(reqstruct)` kopiert den Block nach
  `fq_args` und ruft `fq_request()`; `ezfreq` (reqs.c) bildet darauf die
  Registerschnittstelle ab (comdata+24: d0, Z). `ted-c/src/filereq_env.h`:
  `<proto/exec.h>`, `fq_divu_w` = `divu_w` (glue.c; keine der drei
  Divisionen kann überlaufen).

`flink` entfällt (`DIFF:`, [abweichungen.md](abweichungen.md)). Der
Datenblock ist der der freqlib: die 7 abweichenden Füllbytes von TEDs Kopie
werden nicht übernommen (nie gelesen; ebenfalls in abweichungen.md). Der
bekannte Überlauf bei Fenstern ab 605 Pixel Höhe schreibt in TED über
`fq.line[63]` hinaus hinter `fq` in die folgenden Daten des Data-Hunks
(BUG(orig), wie in freqlib). Die frühere Option `TED_FILEREQ_BLOB` (Original-Hunk per
`.incbin`) wird nicht gebaut.

### 3.3 Andere TED-Versionen auf der CD (nicht Referenz)

`cd/internet/newted/ted`, `cd/bum10a/ted223/ted`, `cd/bui5/superted/ted`,
`cd/bss3/superted/programs/ted`, `cd/extras/newteditor/ted`,
`cd/bum9a/bum9examples2/harddiskusers/ted` sind andere (spätere) Editoren.
`amiblitz2source/ted2.bb2` ist der AmiBlitz2-Editor (Blitz-Quelltext mit
Asm, StormWizard-Oberfläche) und keine Übersetzung von `ted.s`
(keine gemeinsamen Labels wie `tokit`, `comdata`); als Referenz für das
Verhalten von TED 1.1 ungeeignet.
