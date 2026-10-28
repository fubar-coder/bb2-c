# Binärformate von Blitz Basic 2.1

> Stand 26.10.2026. Grundlage: Kopie der Blitz-2.1-CD unter `cd/`
> (4310 Dateien). Ziel: offene Fragen für den geplanten Konverter
> `.bb2` → `.ab3` klären (vgl. `ab3c/docs/bb2-converter/bb2-format.md`,
> `bb2-corpus.md`).
>
> Belege: **[B]** Binär-Auswertung der CD-Dateien (Skripte unten),
> **[S]** Assembler-Quelltext von Blitz 2 v1.95 und TED 1.1
> (`ab3c/tests/external/aminet/amibl2src/files/amiblitz2source/bb2src.zip.dir/blitz2.s`,
> `ted.s`, `options.src`), **[C]** AB3-Quelltext
> (`AmiBlitz3/Sourcecodes/Amiblitz3/Compiler/AmiBlitz3.ab3`). Nichts davon
> ist in Amiberry verifiziert. CD-Programme wurden nicht ausgeführt.

Werkzeuge (Python, mit `-I` aufrufen) in `tools/`:

| Skript | Zweck |
|---|---|
| `hunk.py` | minimaler Hunk-Leser (Code/Data/BSS, Reloc32) |
| `xtra_dump.py [--stats] <pfad>…` | `.xtra` zerlegen bzw. Statistik über alle |
| `res_dump.py [-s] <datei.res>…` | Typen, Makros, Konstanten einer `.res` |
| `src_scan.py <builtin_tokens.txt> <wurzel> [--list]` | Quelltexte klassifizieren und Byte für Byte prüfen |
| `bb2_tokens.py <blitz2> [out] [-v]` | eingebaute Schlüsselwörter aus dem Compiler ziehen |

Ein wichtiger Fund vorab: `blitz2.s` (v1.95) und `ted.s` (TED 1.1) aus
`amibl2src` passen zur CD. TED 1.1 meldet sich mit demselben Text
„Blitz Basic 2 - Ted Version 1.1“ wie `cd/blitz2/ted`. Die
Optionstabelle, die Token-Liste und die Umlauttabelle im Compiler 2.10
haben genau den Aufbau aus `blitz2.s`. Die Quelltexte deuten die Binärdaten
deshalb zuverlässig.

---

## 1 `.xtra`-Begleitdatei

### 1.1 Aufbau

**[S]** `loadxtra`/`savextra` und die Tabelle `optspnts` (blitz2.s
Z. 15288–15450) lesen bzw. schreiben Blöcke: Ein Wort gibt die Größe an,
danach folgen die Zielzeiger bis 0. **[B]** Im Compiler 2.10 liegt dieselbe
Tabelle bei Code-Offset `$F918`: Block 1 mit 2 Zeigern, Block 2 mit **14**,
Block 4 mit 6, `$220` mit 1, `$180` mit 1, dann Ende. Daraus ergibt sich
dieses Layout (Big Endian):

| Offset | Länge | Inhalt |
|---|---|---|
| 0 (`$000`) | 1 | `vers`: 0 = alt (ohne CLI-Block), 1 = mit CLI-Block. `savextra` schreibt immer 1. |
| 1 (`$001`) | 1 | Ausführbare Dateien mit Icon (Low-Byte der Gadget-Flags, `$80` = an) |
| 2 (`$002`) | 14 × 2 | Schalter, je die Gadget-Flags (`$0080` = GFLG_SELECTED = an, `$0000` = aus), in dieser Reihenfolge: |
| | | +2 Runtime Errors (Fehler-Debugger), +4 Debugger, +6 Long Arrays, +8 Expand Macros, +10 Quoted Macros, +12 Make Smallest Code, +14 Debug Info für Executables, +16 Auto Run, +18 Interrupt Checking, +20 Assembler Checking, +22 Overflow Checking, +24 `optreq2ga5` (Bedeutung offen, immer `$80`), +26 Vorgabe Interrupt Checking, +28 Vorgabe Overflow Checking |
| 30 (`$01E`) | 6 × 4 | Puffergrößen in Bytes: Object, Libs, System, Data, Macro, String (Long). Vorgaben in `options.src`: 16384, 16384/32768, 4096, 4096, 8192, 10240 |
| 54 (`$036`) | 8 × 68 | Residents: je 64 Bytes Pfad (0-terminiert) und 4 Bytes Zeiger (Laufzeitwert, beim Laden gelöscht: `clr.l 64(a0)`) |
| 598 (`$256`) | 192 | Pfad des Executables (0-terminiert) |
| 790 (`$316`) | 192 | Name des Executables (0-terminiert; z. B. `c:makedeflibs`, `nil:`) |
| 982 (`$3D6`) | 128 | nur bei `vers` ≠ 0: CLI-Argument (0-terminiert) |
| 982 bzw. 1110 (`$456`) | n × 4 | bis Dateiende: Paare `lib.w`, `max.w` (Objekt-Maximum je Lib), in der Reihenfolge der geladenen Libs, **ohne** Endekennung |

Fester Teil: 982 Bytes (vers 0) bzw. 1110 Bytes (vers 1). Ist `vers` = 0,
setzt `loadxtra` Vorgaben: Auto Run an, Interrupt/Overflow nach Vorgabe,
Assembler Checking aus.

### 1.2 Prüfung über alle 575 `.xtra` der CD **[B]**

`xtra_dump.py --stats cd`:

* Größen 1054–1250 Bytes. Alle sind ≡ 2 (mod 4), passen also ohne Rest zu
  982/1110 + 4n. Bei keiner Datei bleibt ein Rest.
* `vers` = 1 in 571 Dateien, `vers` = 0 in 4 (Größe 1054/1066:
  `examples/blitzmode/blits.xtra`, `amigamode/medtest.bb.xtra`,
  `arexx/roger2/3.rexx.xtra`).
* Alle Schalter sind nur `$0000` oder `$0080`, das Icon-Byte nur `$00`/`$80`.
* Residents an Offset 54 belegt (Byte genau, z. B. `blitzlibs:amigalibs.res`
  in `bss2/reddebugger/examples/tokenlist.bb.xtra`). Vorkommende Namen:
  `blitzlibs:amigalibs.res` (35), `blitzlibs:libmacs.res` (30),
  `blitzlibs:blitzoffs.res` (15), `blitzlibs:bb2objtypes.res` (8),
  `libincs:…` (10), `blitz2:residents/arexx.res` (2), `pgc.res` (2) u. a.
  104 Dateien haben im Zeigerfeld einen Wert ≠ 0, also Speichermüll.
* CLI-Argument gesetzt in 15 Dateien (`'setpa c:'`, `'asl.h'`, `'commands'`
  …). Die Bytes 116–127 des CLI-Blocks sind in **allen** Dateien 0.
* Paare: Lib-Nummern 4–201, keine Lib 0, unsortiert (Lib-Kettenreihenfolge).
  Ausreißer: `internet/erase/erase_v1.0.bb2.xtra` mit `63337:32782`
  (`$F769`). Das ist wohl eine Fremd-Lib oder Müll. AB3 überspringt
  unbekannte Libs (`findlibnoerr`, `a1` = 0 → weiter).
* Puffer: 225 Dateien mit den Vorgaben 16384/32768/4096/4096/8192/10240.
  Die übrigen haben abweichende Werte (z. B. Object 346, System 24, Data 0).
  Ob 0 „Vorgabe“ heißt, ist offen.
* 15 `.xtra` haben keine Quelldatei daneben (z. B. `extras/libsdev/acidlibsrc/*.xtra`).

**Vertrauen: hoch.** Der Aufbau stimmt bei allen 575 Dateien Byte für Byte.

### 1.3 Vergleich mit AB3 `read_xtrafile` **[C]**

| Punkt | Blitz 2.1 | AB3 |
|---|---|---|
| 1-Byte-Block | vers, Icon | gleich (`xtra_versionnumber`, `set_create_iconfile`) |
| 2-Byte-Block | **14** Einträge | bis Commit `bb485acf` (05.01.2026) **14**; seit `bdae3f9e` (08.01.2026, „changed comdata structure … reorganized settings for debugger“) **15**. Nach `set_use_debugger` wurde laut Kommentaren ein `default_obsolete` („ohne inhalt“) eingefügt; der frühere Platz „quoted macros“ ist jetzt `set_FuncOptimize`, am Ende kam `set_create_dbgfile` hinzu (geprüft mit `git show bdae3f9e^`). Ab dem 2. Schalter ist alles um 2 Bytes verschoben. |
| Puffer, Residents, Exe-Pfad | 24 + 544 + 384 | gleich |
| CLI-Block (vers ≠ 0) | 128 Bytes Text | 116 Text + Stack.l + Versionsinfo.l + Anzahl Builds.l; der Stack wird auf `MIN_STACKSIZE` angehoben |
| Paare | bis Dateiende | bis Dateiende **oder** Lib 0 (`;new`). Danach liest `LoadXtra` (CLIModeSupport.ab3 Z. 364) den PED-Block `XtraExtend`. Beim Schreiben hängt AB3 4 Füllbytes an. |

Folgen für den Konverter und für AB3:

* **Verschiebung um 2 Bytes:** Das aktuelle AB3 liest eine
  Blitz-2-`.xtra` ab Offset 32 statt 30 falsch. Die Puffer sind dann
  verdreht, und die Resident-Namen verlieren die ersten zwei Zeichen
  (`blitzlibs:…` → `itzlibs:…`). Selbst eine AB3-Datei mit 14 Einträgen
  (`AmiBlitz3-Original/…/Tests/68k_crash_newtype.xtra`, 1296 Bytes) passt
  noch zum 14er-Layout. Das ist ein Kandidat für Kapitel 11 und muss in
  Amiberry geprüft werden.
* Die 12 Bytes Stack/Version/Builds sind bei allen CD-Dateien 0. AB3
  setzt den Stack dann auf das Minimum. Das ist unschädlich.
* Für die `.ab3`-Kopfzeilen kann der Konverter die Werte direkt übernehmen:
  Schalter, Puffer, Residents, Exe-Name, CLI-Argument und Maxima.

---

## 2 `.res` (Resident-Dateien)

### 2.1 Format **[S]** **[B]**

Blitz 2.1 hat dasselbe Format wie AB3 (Compiler-Spec 06 §6.1.1). `menu5`
(„make resident“, blitz2.s Z. 4105 ff.) schreibt eine Hunk-Datei:
`HUNK_HEADER` mit 1 Hunk, `HUNK_CODE`, optional `HUNK_RELOC32` (Hunk 0)
und `HUNK_END`. Aufgefüllt wird mit `'ZAP',0` auf Langwort. Im Code-Hunk
stehen nacheinander:

1. **Typen** (`restypes`): `count.w`. Je Typ zuerst die Feldsätze (`next.l`,
   +4 Offset, +10 Typnummer.w, +14 Länge.b, +15 Name), dann der Typsatz
   (+4 erstes Feld, +10 Länge.b, +11 Name). Typnummern ≥ 7 werden beim
   Laden um `rescnt` verschoben, wie in AB3 (`fixtypes`).
2. **Makros** (`resmacs`): `count.w`, Sätze `next.l`, `body.l`, `len.w`,
   `count.w`, `nameend.b`, Name, Rumpf (tokenisiert, Anweisungen mit `:`
   verbunden).
3. **Konstanten** (`resconsts`): `count.w`, Label-Sätze mit `mode.l` = 1,
   `value.l` an +8, Länge.b an +$12 und Name an +$13.

`loadres`/`fixtypes`/`fixmacs`/`fixconsts`/`checkclash` sind Befehl für
Befehl die Vorlage der gleichnamigen AB3-Routinen. **Unterschiede: keine
gefunden.** Die Token-Wörter in Makrorümpfen benutzen die
Blitz-2.1-Nummern (z. B. `BlitWait`: `$8057` = `BTST`, `$8050` = `BNE`).
Für Lib-Tokens gilt also dasselbe Abbildungsproblem wie bei Quelltexten.

### 2.2 Inhalt der 18 `.res` auf der CD **[B]**

`res_dump.py` liest alle bis zum Hunk-Ende (höchstens 2 Füllbytes Rest).

| Datei | Typen | Makros | Konstanten | Beispiele |
|---|---|---|---|---|
| `extras/blitzlibs/amigalibs.res` (auch `bum9b`, `bum10a`) | 359 | 98 | 4551 | Typen `Node`, `List`, `Resident`; Makros `IsListEmpty`, `OFFSET`, `INITBYTE`, `BADDR`; `#INCLUDE_VERSION=$28`, `#NT_TASK=1` |
| `extras/blitzlibs/bb2objtypes.res` (auch bum9b/10a) | 23 | 0 | 0 | `rgbcomp`, `palette`, `bitmap`, `shape`, `slice`, `window`, `screen` … |
| `extras/blitzlibs/blitzoffs.res` (auch bum9b/10a) | 0 | 0 | 313 | `#_OpenIntuition=-30`, `#_AddGadget=-42` … |
| `extras/blitzlibs/copmacs.res` | 0 | 9 | 0 | `BlitWait`, `DataBlock`, `BitMaps`, `AmigaBitMaps`, `StenBitMaps`, `SetSprite0/1`, `SetJmp0/1` |
| `extras/blitzlibs/libmacs.res` | 0 | 23 | 242 | Makros `pchk`, `echk`, `libheader`, `astatement`, `afunction`, `args`, `subs`, `name`, `copwait`; `#wordret=-1`, `#long=3`, `#ld0`…`#la6`, `#ud0`… |
| `bum9b/…/libmacs.res` / `bum10a/…/libmacs.res` | 0 | 23 | 249 / 252 | spätere Fassungen |
| `extras/blitzlibs/libnums.res` | 0 | 23 | 240 | **enthält libmacs-Inhalt**, nicht die Lib-Nummern (Fehler auf der CD) |
| `bum9b/…/libnums.res`, `bum10a/…/libnums.res` | 0 | 0 | 164 / 167 | `#varslib=$FFFE`, `#wbstartuplib=$FFFD`, `#memlib=$FFFA` … |
| `bum10a/blitzlibs/asmmacs.res` | 0 | 12 | 0 | `SBRA`, `SBCC` … `SBLE`, `LEASCALE` |
| `bui8/pgc/pgc/pgc.res` | 3 | 0 | 35 | Typen `main`, `baddytype`, `baddy`; `#_BLOCKWIDTH=$A` |

**Vertrauen: hoch** für das Format; dass sich AB3 beim Laden genauso
verhält, ist nur aus dem Code [C] erschlossen.

---

## 3 Quelltexte `.bb2`/`.bb`

### 3.1 Bestand **[B]**

`src_scan.py`: Geprüft wurden alle `*.bb2`/`*.bb` und alle Dateien mit
`.xtra` daneben, zusammen 734.

| Art | Anzahl |
|---|---|
| tokenisiert `.bb2` | 606 |
| tokenisiert `.bb` | 109 |
| tokenisiert, andere Endung (`.1`, `.include`, `.h`, `.i` ×2, `.src`, ohne) | 7 |
| **Text** (LF, kein `$00`) `.bb2` | 9, alle in `internet/tritonblitz/` (`triton.bb2`, `memo.bb2` …) |
| Text, kein Blitz (`.rexx` ×2, `startup-sequence`, mit `.xtra` daneben) | 3 |

Die 722 tokenisierten Dateien haben 122 837 Zeilen und 2,78 MB.

### 3.2 Aufbau **[B]** bestätigt

* **Kein Header.** Keine Datei beginnt mit `BB22` oder einer anderen
  Kennung. 487 Dateien beginnen mit der Zeile `;` (`3B 00`), 38 mit einer
  leeren Zeile (`00`). Der Rest beginnt direkt mit Code oder Kommentar.
* **Alle 722 enden mit `$00`.** Zeilen sind Bytes bis `$00`. Die längste
  Zeile hat **128 Bytes** (`bui7/marbles/source/the_game10.bb2`); TED
  begrenzt die Zeile auf seinen Puffer.
* **Token-Wörter:** Außerhalb von Strings beginnt jedes Byte ≥ `$80` ein
  Wort. 82 348 Token-Wörter stehen im Code, 179 in Kommentaren. Alle
  eingebauten Wörter (< `$8100`) stehen in der Liste aus Abschnitt 5; es gibt
  kein unbekanntes. Lib-Tokens kommen aus 100 Libs. Am häufigsten sind
  Lib 138 (2601), 187 (2241), 120 (1803) und **185 (1670)** sowie 186 (305).
  Diese Lib-Nummern bildet AB3 per `replacedat` um (bb2-corpus.md §5).
* **Keine Tabs, CR, LF** und **keine Bytes ≥ `$80` in Strings**. TED
  wandelt Tabs beim Laden in Leerzeichen um (s. 4.2).
* **Umlaut-Bytes `$18`–`$1F`: kommen auf der CD nirgends vor**, weder in
  Strings noch in Kommentaren. Die Zuordnung ist nur aus TED und dem
  Compiler belegt (Abschnitt 4).
* **Fremde Steuerbytes:** genau 5 × `$01` außerhalb von Strings:
  `extras/libsdev/userlibsrc/elmorehardwarelib.bb` Z. 96/97 (Zeile nur
  aus `$01`) und `ripacklib.bb` Z. 367/368 in Labels `no_\x01ber`,
  `get_\x01ber` (vermutlich „über“ aus einem anderen Editor). Im Compiler
  ist `$01` das Ende-Zeichen einer `~…~`-Einfügung (`getchar`, blitz2.s),
  `$02` der USEPATH-Rücksprung.
* `$807F` (`MOVEP`) und `$80AD` (`TAS`) kommen nicht vor. Das ist wichtig,
  weil AB3 dort `Chip`/`BSS` hat (5.2).

### 3.3 `.bb` gegenüber `.bb2`

Das Format ist dasselbe; die Endung hat keine Bedeutung. `.bb` steht
überwiegend bei Lib-Quellen (`extras/libsdev/userlibsrc` 25,
`extras/blitzlibs/*.bb`) und älteren Beispielen. TED 2.1 (`extras/newteditor`)
benutzt das Dateimuster `(#?.bb|#?.bb2)`, TED 1.1 und der Compiler prüfen die
Endung gar nicht. `.xtra` wird an den vollen Namen gehängt (`x.bb.xtra`).
**Vertrauen: hoch.**

---

## 4 Editor TED

### 4.1 Umlauttabelle **[B]** **[S]**

`cd/blitz2/ted` (TED 1.1), Datei-Offset `$5DCE`, Label `germs` in ted.s
Z. 6022:

```
E4 1A  F6 1B  FC 1C  C4 1D  D6 1E  DC 1F
ä→$1A  ö→$1B  ü→$1C  Ä→$1D  Ö→$1E  Ü→$1F
```

Der Tastatur-Handler (ted.s Z. 5976) ersetzt das Latin-1-Zeichen einer
Taste durch das Steuerbyte. Dieselbe Tabelle (sechs Paare, **ohne** ß/©)
steht in allen TEDs der CD:

| Datei | Version | Offset |
|---|---|---|
| `blitz2/ted`, `bum9a/blitz2/ted`, `bum10b/blitz2/ted` | Ted Version 1.1 | `$5DCE` |
| `extras/newteditor/ted` | TED V2.1 (01.11.1995) | `$77A8` |
| `internet/newted/ted` | TED V2.1 (27.10.1995) | `$7770` |
| `bss3/superted/programs/ted`, `bum10a/ted223/ted` | SuperTED 2.50 / 2.23 | `$84BE` / `$7E34` |

`ß`→`$19` und `©`→`$18` (bb2-corpus.md §3.2) kommen erst im späteren
`ted2.bb2` (AmiBlitz 2) dazu.

**Compiler 2.10:** Die Tabelle `gs` (`ds.b 26` gefolgt von
`E4 F6 FC C4 D6 DC`; `gs` bei Code-Offset `$7BEE`, die sechs Bytes bei `$7C08`) bildet `$1A`–`$1F` auf
**ä ö ü Ä Ö Ü** ab, passt also zu TED. Laut blitz2.s (`getchar3`) gilt das
für jedes Zeichen < 32 **in Anführungszeichen**: `$1A`–`$1F` werden zu
Latin-1, `$00`–`$19` zu 0. Blitz 2.1 hat Umlaute in String-Literalen also
wahrscheinlich als Latin-1 ausgegeben. AB3s `gs` (`F6 E4 FC D6 C4 DC`,
ä/ö vertauscht) ist demnach eine **spätere Abweichung** und kein Erbe von
Blitz 2.1. Für den Konverter heißt das: In Strings und Kommentaren
`$1A`–`$1F` → ä ö ü Ä Ö Ü (Latin-1 `E4 F6 FC C4 D6 DC`).

### 4.2 Laden und Speichern **[S]**, TED 1.1

* **Laden** (`fileopen`, `readline` ted.s Z. 3375): Ein Byte mit gesetztem
  Bit 7 nimmt das Folgebyte ungeprüft mit (Token). Eine Zeile endet an
  `$00` **oder an LF**. Gibt es LF, gilt die Datei als ASCII; die Zeile wird
  tokenisiert, wenn die Option „Tokenise“ an ist. Tab wird zu Leerzeichen
  bis zum nächsten Tabstop. CR wird nicht gesondert behandelt. Zu lange
  Zeilen werden am Pufferende abgeschnitten. Danach lädt TED `<name>.xtra`
  über die Compiler-Routine `loadxtra` (`edstruct`).
* **Speichern** (`filesave`): Zuerst wird die alte Datei in `<name>.bak`
  umbenannt. Danach wird jede Zeile mit ihrem Abschlussbyte geschrieben:
  `$00`, oder **LF, wenn „Tokenise“ aus ist**. Dann folgt `<name>.xtra`
  (`savextra`) und auf Wunsch `<name>.info`. TED 2.1 hat zusätzlich „Save
  ASCII…“. Text-`.bb2` sind also ein reguläres TED-Format.
* **`BB22`:** kommt in keinem Programm der CD vor, weder in TED, SuperTED,
  `blitz2` noch in den Libs. Gefunden wurde es nur als Daten (`$BB22` in
  `displaylib.bb2`, ein Sample). Der Kopf `BB22 00 08 00 13 00 01 …` aus
  `amibl2src/…/RWELibSourc/…/Anim-Library3.bb` stammt also nicht von
  Blitz 2.1 oder TED, sondern wohl von einem Editor von Reflective Images.

---

## 5 Compiler `blitz2`

### 5.1 Version **[B]**

`cd/blitz2/blitz2` (86 216 Bytes, ein Code-Hunk mit 73 568 Bytes und 3149
Relocs): „Blitz Basic 2 Version 2.10“, „(C)opyright 1995 - Acid Software“,
„Release Version“. Die Befehle liegen in `blitz2:acidlibs`, `deflibs` und
`userlibs/` (releasenotes).

### 5.2 Eingebaute Schlüsselwörter **[B]** **[S]**

Aufbau wie `tokens` in blitz2.s: eine verkettete Liste von Einträgen
`dc.l next` (reloziert), `dc.w nummer`, `name,0`, `hilfetext,0`, `even`.
Token = `$8000 + nummer`. Im Compiler 2.10 beginnt die Liste bei Code-Offset
`$87A` (Datei-Offset `$89A`). Verwiesen wird darauf aus `edstruct` (`$85E`)
und von zwei Code-Stellen.

**Ergebnis:** 210 Einträge `$8001`–`$80D3`. Es fehlt nur `$80B6` (in blitz2.s
auskommentiert: `Ern`). Die Liste steht in `builtin_tokens.txt`
(`TOKEN $xxxx name`):

* `$8001`–`$803B`: Sprache (`NEWTYPE`, `End`, `Let`, `Dim` … `CNIF`, `CSIF`,
  `CELSE`, `CEND`, `CERR`, `Then`, `Else`, `Even4`, `Even8`, `Dc`, `Ds`,
  `Even`, `Dcb`, `GetReg`, `IncBin`, `Free`, `Use`, …, `ALibJsr`, `TokeJsr`,
  `BLibJsr`)
* `$803C`–`$80B2`: 68000-Mnemonics `ABCD` … `UNLK`, `EndAsm`
* `$80B3`–`$80D3`: `List`, `SetErr`, `ClrErr`, `ErrFail`, `Addr`, `MOD`, `Pi`,
  `Repeat`, `Until`, `PutReg`, `Pop`, `INCDIR`, `EndIf`, `Wend`, `SysJsr`,
  `WBStartup`, `Maximum`, `On`, `Off`, `Forever`, `Restore`, `Exchange`,
  `USEPATH`, `CloseEd`, `NoCli`, `BitTst`/`Set`/`Clr`/`Chg`, `Used`,
  `Runerrson`, `Runerrsoff`

**Vergleich mit AB3** (`ab3c/build/host-debug/tests/tokens.txt`): 204
gleich, 14 verschieden.

| Token | Blitz 2.1 | AB3 |
|---|---|---|
| `$8014` | `Jimi` (Platzhalter) | `SYNTAX` |
| `$803B` | `Jimi` (Platzhalter) | `EXTB` |
| `$807F` | **`MOVEP`** | **`Chip`** |
| `$80AD` | **`TAS`** | **`BSS`** |
| `$80D2`/`$80D3` | `Runerrson`/`Runerrsoff` | `RunErrsOn`/`RunErrsOff` (nur Schreibweise) |
| `$80D4`–`$80DB` | – | `USELASTPATH`, `RegLoad`, `RegSave`, `Object`, `USEPATH2`, `LastRunErrsMode`, `KEEP`, `OPTIMIZE` |

Ein Blitz-2.1-Quelltext mit `MOVEP` oder `TAS` würde in AB3 also still zu
`Chip`/`BSS`. Der Konverter muss diese beiden Tokens als Text ausgeben (auf
der CD kommen sie nicht vor). `Jimi` ist nur ein Platzhalter.

---

## Offene Punkte

1. **AB3 liest `.xtra` mit 15 Zwei-Byte-Schaltern** (seit `bdae3f9e`,
   08.01.2026). Blitz 2.1, AmiBlitz 2 und ältere AB3 schreiben 14. Das
   verschiebt alles ab Offset 30 um 2 Bytes; auch Resident-Namen werden
   abgeschnitten. In Amiberry prüfen und als Issue in Kapitel 11 aufnehmen.
2. Bedeutung von `optreq2ga5` (Offset 24, immer `$80`) und der
   Puffer-Werte 0 bzw. ungewöhnlich kleiner Puffer-Werte.
3. String-Umlaute: Dass Blitz 2.1 `$1A`–`$1F` in String-Literalen über
   `getchar3`/`gs` in Latin-1 wandelt, steht im Quelltext von v1.95; die
   Tabelle steht auch im Binary 2.10. Ob das Kompilieren von Strings wirklich
   über `getchar3` läuft, sollte eine Probe mit dem echten 2.10 in Amiberry
   klären (`NPrint` mit `$1A`–`$1F`).
4. `BB22`-Kopf: auf der CD nicht vorhanden; Herkunft (Reflective Images /
   RI-Editor?) weiterhin offen.
5. Lib-Tokens: Die Zuordnung der 100 benutzten Lib-Nummern zu Befehlsnamen
   braucht `acidlibs`/`deflibs` (BlitzLib-Format, Kapitel 09) bzw. AB3s
   `replacedat`. Nicht Teil dieser Auswertung.
6. `extras/blitzlibs/libnums.res` enthält den Inhalt von `libmacs.res`; die
   richtige Fassung liegt auf `bum9b`/`bum10a`.
7. Die `$01`-Bytes in `ripacklib.bb` und `elmorehardwarelib.bb`: Herkunft
   unklar. Der Konverter sollte sie melden.
