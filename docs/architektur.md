# Architektur von Blitz Basic 2: blitz2, TED und der Weg zum Executable

> Stand 26.10.2026. Zielgruppe: Entwickler, die BB2 mit AmiBlitz3 vergleichen
> und 68k/AmigaOS kennen, aber nicht die Interna von BB2.
>
> Grundlage: Compiler `blitz2.s` v1.95 und Editor `ted.s` (TED 1.1) aus
> `quelltext/amibl2src/files/amiblitz2source/bb2src.zip.dir/`, die C-Übersetzung
> [`blitz2-c/`](blitz2-c/README.md) (gleiche Labels, Zeilenverweise je
> Funktion), die Libraries der CD (`cd/blitz2/acidlibs`, `deflibs`) und
> [`libs-c/`](libs-c/README.md). Die Binaries der CD sind v2.10; der
> Referenzlauf über alle 724 CD-Quellen ergab mit v1.95 und v2.10 gleiche
> Programme bis auf eine Assembler-Codierung ([compilerlauf/README.md](compilerlauf/README.md)).
>
> Belege: **[S]** aus Quelltext (`blitz2.s:ZEILE label`, `ted.s:ZEILE label`;
> die C-Funktion steht als `datei.c funktion()` dabei), **[B]** aus Binaries
> oder einem Testlauf in Amiberry (Beispiel in Abschnitt 6),
> **[Schluss]** gefolgert. Was weder gelesen noch ausprobiert wurde, ist als
> „nicht geprüft“ markiert.
>
> Verweise auf die AmiBlitz3-Compiler-Spec (`AmiBlitz3/Docs/Compiler-Spec/`)
> stehen als „Spec 09 §9.2“ usw.; das BigLib-Format ist dort in Kapitel 09
> vollständig beschrieben und wird hier nur zusammengefasst.

## Inhalt

1. [Überblick](#1-überblick)
2. [Prozessstruktur: wer startet wen](#2-prozessstruktur-wer-startet-wen)
3. [Schnittstelle TED ↔ blitz2](#3-schnittstelle-ted--blitz2)
4. [Library-Arten](#4-library-arten)
5. [Übersetzungsablauf und Linken](#5-übersetzungsablauf-und-linken)
6. [Beispiel: drei Zeilen bis zum Hunk](#6-beispiel-drei-zeilen-bis-zum-hunk)
7. [Hinweise für den Vergleich mit AmiBlitz3](#7-hinweise-für-den-vergleich-mit-amiblitz3)
8. [Offene Punkte](#8-offene-punkte)

---

## 1 Überblick

Blitz 2 besteht aus einem Compiler-Programm (`blitz2`), das den Editor
(`ted`) als Code-Segment nachlädt. Der Editor ist die Oberfläche, der
Compiler läuft nur, wenn der Editor ihn über einen Menüpunkt aufruft. Es gibt
keinen separaten Linker und keine Objektdateien: Der Compiler erzeugt den
Maschinencode direkt in einem Speicherpuffer, kopiert die benötigten
Libraries dazu und schreibt den Puffer als ausführbare Datei mit **einem**
Code-Hunk – oder führt ihn im Speicher direkt aus.

```
  blitz2:  (Assign, alle Dateien in cd/blitz2/)
  ├── blitz2        Compiler, wird von CLI/WB gestartet            86 KB
  ├── ted           Editor, von blitz2 per LoadSeg geladen         59 KB
  ├── acidlibs      BigLib: Befehls-Libs + System-Libs             219 KB
  ├── deflibs       BigLib: OS-Aufruf-Libs (Open_ …) + Fremd-Libs   55 KB
  ├── userlibs/     weitere Libs, je Unterverzeichnis
  └── defaultdbug   Debugger-Oberfläche, von blitz2 per LoadSeg     32 KB

  Quelle.bb2 (tokenisiert)  +  Quelle.bb2.xtra (Optionen, Residents)
  *.res  (Residents: vorübersetzte NewTypes, Makros, Konstanten)
```

---

## 2 Prozessstruktur: wer startet wen

### 2.1 blitz2 ist das Hauptprogramm, TED ein nachgeladenes Segment [S]

Gestartet wird **blitz2** (CLI oder Workbench). blitz2 lädt TED mit
`LoadSeg("blitz2:ted")` und ruft dessen Init-Einsprung auf; danach übergibt
es die Kontrolle an die Hauptschleife von TED und bekommt sie erst zurück,
wenn der Benutzer den Editor verlässt. Alles läuft im **Prozess von
blitz2** – TED ist kein eigener Task, sondern Code im selben Prozess.

```
CLI/WB
  │
  ▼
blitz2  wbheader/mainbit (blitz2.s:253)          main.c main()
  │     actualbegin/begin (blitz2.s:1353)        main.c begin()
  │       LoadSeg("blitz2:ted")
  │       jsr BADDR(seg)+8  ──────────────►  TED outside (ted.s:24)
  │          a1 = &edstruct                     kopiert edstruct (28 Bytes),
  │                                             öffnet Bildschirm usw.
  │       ◄──────────── a0 = .tomain, a1 = &comdata, Z=1
  │       Libraries laden, Puffer anlegen,
  │       Versionsrequester (über comdata)
  │       LoadSeg("blitz2:defaultdbug")
  │       jsr (a0) mit a0 = Dateiname  ───►  TED .tomain (ted.s:87)
  │                                             Editor-Hauptschleife
  │                       ◄─── menu (d6,a5,d7)  Menü „COMPILER“
  │       compile / savefile / Run ...  ───►    (Rückkehr d0 = Zeile)
  │                                             …
  │       ◄──────────── rts beim Beenden des Editors
  │       Aufräumen, UnLoadSeg, Ende
  ▼
```

* Der Hunk von TED beginnt mit `bsr edstart` (`ted.s:21`): So läuft `ted`
  auch allein als Editor (ohne Compiler). blitz2 ruft `BADDR(seg)+8`, also
  Hunk-Offset 4 = `outside` (`ted.s:24`), und überspringt damit diese
  erste Anweisung. `outside` erkennt den Compiler an a1 ≠ 0 (`external`).
* TED antwortet mit a0 = Haupteinsprung `.tomain`, a1 = `comdata`
  (Tabelle der Editor-Dienste, Abschnitt 3.2) und Z=1 (`moveq #0,d0`).
* C-Port: `callregs()` in `glue.c` lädt die Register, ruft und liest sie
  zurück; die Rückrufe des Editors gehen über Register-Stubs
  (`REGSTUB(menu)`, `loadxtra`, `savextra`, `stopcode` in `glue.c`).

### 2.2 Start im Detail [S]

| Schritt | blitz2.s | C-Port | Inhalt |
|---|---|---|---|
| 1 | `wbheader` (Include), `mainbit` 253 | `main.c main()` | CLI: Stackgröße aus `4(a7)`, Dateiname aus der Argumentzeile; WB: WBStartup-Nachricht, Stackgröße aus `.info`, Dateiname aus Argument 2 → `progfile`, `stackfuck` |
| 2 | `begin` 1357 | `main.c begin()` | Token-Liste durchlaufen, `lasttoken` merken (hier hängen die Libs ihre Tokens an), `makeasmtab` (Tabelle der 119 Assembler-Mnemonics), `mathffp.library` öffnen |
| 3 | 1376–1390 | | Puffer für Konstantenauswertung (`constpcat`, 1024 Bytes) und Makropuffer (`macrolen`, Standard 8192) |
| 4 | 1392 | | `LoadSeg("blitz2:ted")`, Init-Aufruf (2.1) |
| 5 | 1405–1416 | | `int` = IntuitionBase des Editors (`comdata+44`), Versionsrequester über `comdata+0`; `makelibs` (Größen von `deflibs`/`acidlibs`, Liste von `userlibs/`), `loadlibs` (4.7), `makeiallox` (Objektpuffer, 5.2) |
| 6 | 1418–1440 | | warten auf „OKEE DOKEE“ (`getinput`), Signalbit für den PANIC-Mechanismus (`AllocSignal`), `LoadSeg("blitz2:defaultdbug")` → `dseg` |
| 7 | 1441 | | `jsr (a0)` = Editor-Hauptschleife mit a0 = `progfile` |

Die `.xtra`-Datei liest **nicht** blitz2 beim Start, sondern TED beim Laden
einer Quelle über den Rückruf `loadxtra` (3.4). Residents werden dabei
mitgeladen.

### 2.3 Ende [S]

Kehrt die Editor-Hauptschleife zurück (`blitz2.s:1448 .skip`):
`FreeSignal`, `UnLoadSeg(dseg)`, `freeemup` (Compiler-Listen), `freeres`
(Residents), `freelibs`, dann die Fehlerkette `fail3` … `fail0`
(`UnLoadSeg` TED, Objektpuffer, Makropuffer, dos, Konstantenpuffer,
mathffp), `flushlibs` (AllocMem über 256 MB, damit Exec unbenutzte
Libraries entfernt). BUG(orig): Der in `returncode` gesetzte Rückgabewert
geht in `wb_exit` verloren (d0 wird überschrieben) – survey-01.

### 2.4 Was läuft in welchem Task [S]

| Code | Task | Bemerkung |
|---|---|---|
| TED, Compiler, `savefile` | Prozess von blitz2 | Compiler läuft nur innerhalb der Rückrufe `menu`, `loadxtra`, `savextra` |
| Übersetztes Programm bei „RUN“/„COMPILE AND RUN“ | eigener Prozess „Blitz ][ Program Proc“ (`CreateProc`, `blitz2.s:1605 menu10`) | Code läuft direkt im Objektpuffer des Compilers |
| `directtrap`/`directdo` (Direktmodus des Debuggers, Ausdruck auswerten) | im **Programm**-Prozess | der Debugger löst `trap #15` aus, der Compiler übersetzt die Eingabe im Kontext des Programms (3.7) |
| `stopcode` | Exec-Exception des blitz2-Prozesses | STOP-/PANIC-Knopf (3.8) |
| Konstantenauswertung `evalconst2` | Prozess von blitz2 | erzeugter Code wird **im Compiler** ausgeführt (5.4) |

---

## 3 Schnittstelle TED ↔ blitz2

Die Schnittstelle hat zwei Richtungen: `edstruct` (blitz2 → TED, beim Init
übergeben; enthält auch die Rückrufe in den Compiler) und `comdata` (TED →
blitz2, Tabelle der Editor-Dienste).

### 3.1 `edstruct` [S]

`blitz2.s:274 edstruct`, C: `main.c edstruct`. TED kopiert die 28 Bytes in
seine Variablen ab `tokens` (`ted.s:6269`).

| Offset | Inhalt | Verwendung in TED |
|---|---|---|
| 0 | Zeiger auf die Token-Liste (`tokens`, `blitz2.s:283`) | Tokenizer `tokit` (`ted.s:4198`), Detokenizer, Hilfe |
| 4 | Menütexte (`blitz2.s:724 menus`): `"COMPILER"`, 10 Einträge zu 23 Zeichen + Tastenkürzel | Menü „COMPILER“ |
| 8 | `loadxtra` (`blitz2.s:15345`) | nach dem Laden einer Quelle |
| 12 | `savextra` (`blitz2.s:15412`) | nach dem Speichern |
| 16 | `menu` (`blitz2.s:1498`) | jeder Punkt des Menüs „COMPILER“ |
| 20 | 0 (Cleanup, unbenutzt) | |
| 24 | Zeichen `:` `"` `.` `;` | Trenner, String, Label, Kommentar (Syntaxfarben/Tokenizer) |

Die Token-Liste ist eine verkettete Liste `{next.l, num.w, name\0, help\0,
even}` – erst die 211 eingebauten Wörter, dann (von `fixlibs` angehängt)
die Namensblöcke aller Libraries. TED, Debugger und `debuglib` laufen sie
direkt ab; das Layout muss deshalb bytegenau sein.

### 3.2 `comdata`: Dienste des Editors [S]

`ted.s:6030 comdata`; C-Wrapper `ed_*()` in `main.c`, Offsets `CD_*` in
`main.h`. Aufruf immer `move.l comdata,aX / move.l off(aX),aX / jsr (aX)`.

| Off. | TED | Zweck für blitz2 |
|---|---|---|
| −8/−4 | `screenclose`/`screenopen2` | Editor-Bildschirm schließen/öffnen (`CloseEd`) |
| 0/4 | `openreq`/`closereq` | „Requester“ öffnen/schließen (Intuition-Requester-Strukturen dienen nur als Vorlage, TED baut daraus ein Fenster) |
| 8 | `getinput` | warten; d7 = −1 und d6 = Gadget-ID bei Gadget-Ereignis |
| 12 | `error` (`ted.s:3511`) | Fehlermeldung a0 anzeigen (modal) |
| 16/20 | `detokallz`/`tokall` | ganze Quelle de-/tokenisieren (von v1.95 nicht benutzt) |
| 24 | `ezfreq` | Dateirequester: a0 Pfad, a1 Name, a2 Titel → d0, Z=1 bei Abbruch |
| 28 | `refgads` | Gadgets neu zeichnen |
| 32 | `getinput2` | wie 8, ohne Warten (d7 = 0: nichts) |
| 36…52 | Daten | Bitplane für den Fortschrittsbalken, Screen, IntuitionBase, Farben |
| 56 | `tokone` (`ted.s:2636`) | eine Zeile tokenisieren (Direktmodus, Rechner) |
| 60/64 | `openio`/`closeio` (`ted.s:2548`) | Ein-/Ausgabefenster für das laufende Programm |
| 68 | `actgad` | String-Gadget aktivieren |
| 72/76 | `pointeron`/`sleepon` | Mauszeiger normal/beschäftigt |
| 80 | Daten `mywindow2` | Editorfenster |
| 88 | `extalloc` (`ted.s:3049`) | **AllocMem** für den Compiler (`doallocmem`, `blitz2.s:3379`); bei Fehlschlag springt TED in seine Speicherfehler-Behandlung und kehrt nicht zurück |
| 92/94 | Daten | Bytes je Bitplane-Zeile, Requester-Fenster (dessen UserPort-Signal für STOP, 3.8) |

### 3.3 Der Menü-Rückruf `menu` [S]

TED ruft bei einem Punkt des Menüs „COMPILER“ `extmenu` (`ted.s:1542`):

```
d6 = Menüpunkt (0 …)         a5 = firstitem (erste Quellzeile)
d7 = Anzahl Zeilen           jsr menu   (blitz2.s:1498, C: main.c menu_c())
→ d0.w = -1  Cursor bleibt
         -2  alles neu zeichnen
         n   Cursor auf Zeile n (Fehlerzeile, gezählt ab 0)
```

`menu` speichert `firstitem`/`numlines`, setzt `menuret = -1` und springt
über die Tabelle `menuadds` (`blitz2.s:741`):

| d6 | Menüpunkt | Routine (blitz2.s) | C |
|---|---|---|---|
| 0 | COMPILE AND RUN | `menu0` 1595 → `compile`, dann `menu10` | `main.c menu0()` |
| 1 | RUN | `menu10` 1605 | `main.c menu10()` |
| 2 | CREATE EXECUTABLE | `menu1` 1556 → `compile`, `savefile` | `main.c menu1()` |
| 3 | COMPILER OPTIONS | `menu4` 15456 | `data.c menu4()` |
| 4 | CREATE RESIDENT | `menu5` 4105 | `stmts.c menu5()` |
| 5 | VIEW NEWTYPES | `menu6` 15165 | `data.c menu6()` |
| 6 | CLI ARGUMENT | `menu9` 15979 | `data.c menu9()` |
| 7 | CALCULATOR | `menu7` 15739 | `data.c menu7()` |
| 8 | RELOAD ALL LIBS | `menu8` 1778 | `main.c menu8()` |
| 9 | LOAD DEBUG MODULE | `menu_d` 1518 | `main.c menu_d()` |

### 3.4 `.xtra` laden und speichern [S]

TED öffnet nach dem Laden einer Quelle `NAME.xtra` und ruft `loadxtra` mit
**d7 = Dateihandle, a6 = DOSBase** (`ted.s:710`), nach dem Speichern
entsprechend `savextra` (`ted.s:795`). `loadxtra` (`blitz2.s:15345`,
C: `data.c loadxtra_c()`) liest die Optionen direkt in die
Gadget-Strukturen des Optionsrequesters (die Flags der Gadgets *sind* die
Optionsspeicher), das CLI-Argument, die Maximum-Werte je Objekt-Library,
legt den Objektpuffer neu an (`makeiallox`) und lädt die Residents
(`loadres`, 4.5). Format der Datei: [formate.md](formate.md).

### 3.5 Quellzeilen: TED gibt seinen Zeilenspeicher direkt weiter [S]

Es gibt keine Übergabe einer Kopie: a5 zeigt auf die erste Zeile der
doppelt verketteten Zeilenliste von TED (`ted.s:15 rsreset`):

```
+0 nextitem.l   +4 lastitem.l   +8 numchars.b   +9 chars[] … $00
```

TED hält die Zeilen **tokenisiert** im Speicher: Beim Verlassen einer Zeile
läuft `tokit` (`ted.s:4198`) über sie. Der Compiler liest ab `+9` bis zum
Nullbyte (`blitz2.s:2038 nextline`, `compile.c nextline()`):

* ASCII-Zeichen < `$80` unverändert (Namen, Zahlen, Strings, Operatoren).
* Ein Byte ≥ `$80` beginnt ein **Token-Wort** (big-endian, 2 Bytes):
  * `$8001`–`$80D3` eingebaute Wörter (`builtin_tokens.txt`), darunter
    `$803C`–`$80B2` die Assembler-Mnemonics.
  * `$8000 | (lib·128 + i)` Befehle der Libraries, z. B. `Print` = `$DD81`
    (Lib 187, Eintrag 1), `Left$` = `$DC81` (Lib 185/1). Der Compiler
    unterscheidet Lib-Tokens an `(token & $7F00) ≠ 0` (`blitz2.s:3105
    handletoken`).
* Zeilen sind höchstens 127 Zeichen lang; Tabs werden zu Leerzeichen.

Der Tokenizer ist in `compilerlauf/tools/bb2tok.py` nachgebaut
(Eigenheiten dort im Kopf).

### 3.6 Fehler zurück an TED [S]

Jede Fehlerstelle ruft `jsr err` mit dem Text als Inline-String
(`blitz2.s:7641 err`, C: `parse.c err()`). `err`:

1. setzt `anyerrs = -1` und **`menuret = linenumat`** (Zeilennummer ab 0),
2. schließt den Fortschritts-Requester (`endstop`),
3. zeigt den Text über `comdata+12`,
4. springt zurück: `a7 = errstack`, `jmp (errcont)` – ein longjmp. Beim
   Übersetzen ist `errcont = freeemup` (`compile` setzt es, `blitz2.s:1917`),
   d. h. der ganze Übersetzungsvorgang wird abgebrochen und aufgeräumt.

`menu` gibt `menuret` an TED zurück, TED setzt den Cursor auf die Zeile
(`ted.s:1542 extmenu` → `gotolined0`). Es wird **nur ein** Fehler gemeldet. Zeilen aus
`INCLUDE`-Dateien erhöhen `linenumat` nicht (`nextline` liest sie über
`readinc` und überspringt das Hochzählen); ein Fehler in einer
Include-Datei landet daher auf der `INCLUDE`-Zeile [Schluss aus dem
Code, nicht ausprobiert].

### 3.7 Run, Create Executable und Debugger

**CREATE EXECUTABLE** (`menu1`) [S]: Dateirequester, `makeexec = -1`,
Option „Make smallest code“ zwangsweise an (Abschnitt 5.1), `compile`,
bei Erfolg `savefile` (5.10).

**RUN / COMPILE AND RUN** (`menu0`/`menu10`) [S]:

```
blitz2-Prozess                              Kind-Prozess "Blitz ][ Program Proc"
menu10 (1605)
  STOP-Requester "EXECUTING"/"!PANIC!"
  CreateProc(procstart, stack=stackfuck) ──► runtheprog (1688, main.c runtheprog())
  Wait(panicmask)                              comdata+60 openio (Konsole von TED)
     │                                         VBR-Tabelle beschreiben:
     │                                           Trap-#1-Vektor  = &tokens   (Postfach)
     │                                           Trap-#2-Vektor  = firsttype (Postfach)
     │                                           Trap-#15-Vektor = directtrap
     │                                         d2 = Stack, d7 = 'bLtZ', a0 = CLI-Arg.
     │                                         jsr pcat   ← Programm im Objektpuffer
     │                                         comdata+64 closeio
     │◄──────────── Signal(us, panicmask) ──── stopit = 1, Ende des Prozesses
  stopit = -1 (PANIC)? → RemTask(Kind)
```

Das Programm läuft also **ohne Relocation an der Adresse, an der es
übersetzt wurde** (der Objektpuffer enthält absolute Adressen, 5.2).
`procstart` ist eine falsche Segmentliste im Datenbereich von blitz2
(`blitz2.s:1683`).

**Debugger** [S, Lib-Teil aus libs-c]: Ist die Option „Runtime Errors“
(`optreqga10`, intern `debugga`) an, bindet `dopass` zusätzlich ein:

| Ziel | Libs | Funktion |
|---|---|---|
| RUN aus dem Editor | `runerrlib` 65500, `debuglib` 64060, `dhandlerlib` 1 | volle Debugger-Laufzeit: `debuglib` übernimmt TRAP #0–#5 (liest dabei den alten Trap-#1-Vektor = Token-Liste), `dhandlerlib` bekommt im Init die Adresse von `dseg` (geladenes `defaultdbug`) und verbindet die Laufzeit mit dieser Oberfläche |
| CREATE EXECUTABLE | `runerrlib` 65500, `rundebuglib` 64050 | minimal: TRAP #0 → AutoRequest mit Fehlertext und Programmende; TRAP #1/#2/#4 überspringen nur ihre Parameter |

Der Compiler erzeugt dazu:

* je Anweisung einen **Debug-Block** (`blitz2.s:2871 debugstuff`,
  `compile.c debugstuff()`): `trap #1`, dann `dc.l` Adresse der TED-Zeile,
  `dc.w` Offset der Anweisung in der Zeile, `dc.w` Abstand zum nächsten
  Block (verkettet, letzter = 0). Die Zeilenadresse zeigt in den Speicher
  von TED – deshalb sind Executables mit Laufzeitprüfung zwischen zwei
  Läufen nicht bytegleich (compilerlauf/README.md).
* `trap #4`/`#5` am Prozedurbeginn/-ende, `trap #2`/`#3` um
  Interrupt-Routinen.
* Prüfaufrufe vor bzw. nach Lib-Routinen (`fehler1`/`fehler2` aus `!subs`,
  5.5) und die Fehlerprüf-Routinen der Libs (Bereich ab `error`, 4.6).
* am Programmende `finalscheck` (`$C300`) und `moveq #3,d0 : trap #0`
  (`blitz2.s:2683 eopcode`) – in Executables ohne `End` erscheint dadurch
  ein leerer Requester (tedstub/README.md).

Laufzeitfehler meldet Lib-Code mit `move.l #text,d0 : trap #0`
(`BLITZ_RUNERR` in `libs-c/include/blitzlib.h`).

**Direktmodus, `trap #15`** [S]: Auswerten/Ausführen im Debugger
(`debuglib` `doevaluate`/`doexecute`) löst im Kind-Prozess `trap #15` mit
18 Byte Parametern aus (Zeile, Locals, Globals, Ergebnis, Blitz-Modus).
`directtrap` (`blitz2.s:3805`, Assembler in `glue_direct.c`) biegt die
Rücksprungadresse auf `directdo` (3819, `stmts.c directdo_c()`) um: Der
**Compiler** läuft jetzt im Programm-Prozess, tokenisiert die Eingabe über
`comdata+56`, übersetzt sie mit `doline` hinter das Programm (`dirpcat`),
hängt `jmp ret15add` an und springt hinein. Fehler gehen über
`errcont = dircont` als Text an das Programm zurück.

### 3.8 STOP und PANIC (`stopcode`) [S]

Während des Übersetzens („COMPILING“/„STOP!“) und während das Programm
läuft („EXECUTING“/„!PANIC!“) ist ein Requester offen. `openstopreq`
(`blitz2.s:2526`) trägt `stopcode` als **Exec-Exception-Handler** des
blitz2-Prozesses ein (`tc_ExceptCode`, `tc_SigExcept` = Signalbit des
UserPorts des Requester-Fensters). Kommt eine Nachricht an dieses Fenster,
setzt `stopcode` (`blitz2.s:2453`, `compile.c stopcode_c()`) `stopit = -1`
und signalisiert `panicmask`.

* Beim Übersetzen prüft `chkstop` (`blitz2.s:2847`) vor jeder Anweisung;
  `chkrealstop` fragt per `getinput2`, ob es wirklich der STOP-Knopf war,
  dann Abbruch wie ein Fehler.
* Beim Ausführen weckt das Signal `menu10`; bei echtem PANIC wird der
  Kind-Prozess mit `RemTask` **ohne Aufräumen** entfernt.
* Strg-C wertet blitz2 nicht aus. In `rundebuglib` gibt es ein Flag
  `ctrlc`, das nie gesetzt wird (BUG(orig), `libs-c/libs/rundebuglib.c`).

### 3.9 Was davon wirklich nötig ist: tedstub [B]

`compilerlauf/tedstub/` ersetzt TED ohne Bildschirm und übersetzt damit
alle 724 CD-Quellen. Er zeigt das Minimum: Init-Einsprung an Hunk+4,
`comdata` mit sinnvollen `openreq`/`getinput`/`error`/`ezfreq`/`extalloc`
und Daten (`combitmap`, Fenster mit UserPort), Zeilenliste wie TED,
`loadxtra` mit d7/a6 und `menu` mit d6 = 2. Alle übrigen Einträge dürfen
leer sein (Tabelle in [tedstub/README.md](compilerlauf/tedstub/README.md)).

---

## 4 Library-Arten

### 4.1 Übersicht

| Art | Wo | Nummern | Tokens? | Wie benutzt | Kommt ins Programm |
|---|---|---|---|---|---|
| **Eingebaute Wörter** | in `blitz2` (`blitz2.s:283 tokens`) | Lib 0 | `$8001`–`$80D3` | Statement-Handler `atokens` (`blitz2.s:1298`) | nur der erzeugte Code |
| **Befehls-Libs** (BlitzLibs) | `acidlibs`, `deflibs`, `userlibs/` | 1–255 | `$8000 \| lib·128+i` | Befehl/Funktion im Quelltext → `handlelib` | ganze Lib |
| **System-Libs** | `acidlibs` | 64040–65534 (als Wort, „negativ“: −1496…−2) | keine Namen; „Token“ `$C000`–`$D5xx` intern | vom Compiler (`tokejsr`) oder von anderen Libs (`ALibJsr`) | ganze Lib |
| **OS-Aufruf-Libs** | `deflibs` | 200, 219–255 | z. B. `Open_` = Lib 252 | Eintrag mit Art Bit 2 → `stamigalib`: Register laden, `jsr lvo(a6)` | nur Init/Finit (OpenLibrary/CloseLibrary) |
| **Residents** | `*.res`, in der `.xtra` eingetragen | – | – | NewTypes, Makros, Konstanten vorab bekannt | nichts (nur Deklarationen) |

Die Zuordnung „System-Lib“ hängt nur an der Nummer, nicht an der Datei:
`acidlibs` enthält 57 Befehls-Libs **und** die 25 System-Libs sowie Lib 1
(`dhandlerlib`) [B, `biglib.py list`]. `deflibs` der CD enthält 39 Libs:
Lib 2 und die OS-Aufruf-Libs 200, 219–255 [B].

### 4.2 Eingebaute Wörter [S]

59 Anweisungen und Schlüsselwörter (`NEWTYPE`, `End`, `If`, `For`, `Data`,
`Macro`, `CNIF` …, Nummern 1–59), 118 Assembler-Mnemonics + `EndAsm`
(60–178, für Inline-Assembler, Tabelle `asmmac`), weitere 33 Wörter
(`List`, `SetErr`, `Repeat`, `BitTst` … 179–211). Alles, was nicht hier
steht – also auch `Print`, `Left$`, `Str$` –, kommt aus Libraries.
Übersicht: `builtin_tokens.txt`, [formate.md](formate.md).

### 4.3 Befehls-Libs [S/B]

Normale BlitzLibs mit Namensblöcken; ihre Tokens hängt `fixlibs` an die
Token-Liste von blitz2, damit TED sie tokenisieren kann. Beispiel:
`printlib` 187 (`Print`, `NPrint`, `Str$`, `Format` …),
`stringfunclib` 185 (`Left$` …). Objekt-Libs (Screens, Shapes …) haben
zusätzlich eine `!libfin`-Erweiterung mit Maximum und Objektgröße (4.6).

### 4.4 System-Libs [S/B]

Laufzeitcode ohne Namen, den der Compiler selbst einbindet – über
„Token“-Nummern aus `blitz2.s:13–251` (Liste in survey-01 C), z. B.:

| Lib | Nummer | Beispiele (Token) |
|---|---|---|
| `varslib` | 65534 | Globals anlegen (`allocvars`), Init setzt a5 |
| `memlib` | 65530 | `localloc $C000` … Speicherverwaltung |
| `strings1lib` | 65235 | `copstring $C600` (String in den Arbeitspuffer kopieren) |
| `strings2lib` | 65135 | `lastring $C700`/`astring $C701` (String-Variable zuweisen) |
| `exitslib` | 65035 | `endjmp $C800` (`End`) |
| `ffplib` | 64935 | Basis von mathffp.library |
| `ldivlib`/`lmullib` | 64735/64835 | `longdiv $CB01`, `longmult $CA01` |
| `datalib` | 64535 | `Read`/`Restore` |
| `runerrlib`, `debuglib`, `rundebuglib` | 65500, 64060, 64050 | Laufzeitprüfung, Debugger (3.7) |

Alle 25 sind in [libs-c/](libs-c/README.md) als C nachgebaut und gegen die
Originale geprüft.

### 4.5 OS-Aufruf-Libs (deflibs) [S/B]

Eine Lib wie `doslib` 252 enthält praktisch nur Deklarationen: Einträge
„Art 6“ mit Basis-Lib, LVO und Registerbytes, z. B. `Open_` = Lib 252,
LVO −30, Parameter in d1, d2 [B, `biglib.py dump`]. Code hat sie nur für
Init (Library öffnen, Basis zurückgeben) und Finit (schließen): Von den
5756 Bytes von `doslib` landen 44 im Programm (Abschnitt 6). Ein Aufruf
`Open_(n$,1005)` wird in `stamigalib` (`blitz2.s:3879`,
`stmts.c stamigalib()`) übersetzt: Argumente auf den Stack, in die
Register zurückholen, `move.l base(a5),a6 : jsr lvo(a6)`.
Andere Libs fordern solche Basen über `!libs` an (z. B. `inputoutputlib`
die DOS-Basis), deshalb kann auch ein Programm ohne `…_`-Befehl eine Lib aus
`deflibs` enthalten.

### 4.6 BigLib-Format (Kurzfassung von Spec 09) [S/B]

```
BigLib-Datei (acidlibs, deflibs) = { size.l, Lib-Executable } … 0.l
Lib-Executable = HUNK_HEADER (32 Bytes) | HUNK_CODE = Lib-Image | HUNK_RELOC32 | HUNK_END
userlibs/…/datei = ein Lib-Executable ohne size.l

Lib-Image:
+$00  moveq #0,d0 : rts        im Speicher: next-Zeiger
+$04  Lib-Nummer.w
+$0A  (Compiler: a5-Offset der Init-Rückgabe, −1 = nicht eingebunden)
+$0C  Flags (Bit 7 benutzt, Bit 6 in Konstanten erlaubt)
+$0E  erstes Token, +$10 letztes+1 (System-Libs; normale: vom Compiler)
+$12  (Compiler: Offset Code-Beginn)
+$16  init  +$1A return (0/1 Long/−1 Word)  +$1C finit
+$20/+$24 (Compiler: Relocs)   +$28 (Compiler: Adresse im Programm)
+$2C  error: ab hier Fehlerprüf-Code (nur mit Debugger kopiert)
+$30  Token-Einträge: !astatement/!afunction/!acommand …
        Formen: !args/!repargs  !libs{lib,reg…}  !subs{code,err1,err2}  !name
      !nullsub (Routine ohne Namen)  !dumtoke (nur Name)
      OS-Einträge (Art Bit 2)
      !libfin  (−1.w, 0.l | Objekt-Lib: toke,load,save,use,free,0,max,shift)
      Code … [error: Prüfroutinen]
```

Einzelheiten (Art-Werte, Registercodes `#ld0`/`#la6`/`#lpush`/`#pd0`,
Wiederholgruppen) stehen in Spec 09 §9.2 und in den Makros
`cd/extras/blitzlibs/libmacs.bb` bzw. deren C-Nachbau
`libs-c/include/blitzlib.h`. Zusätzlich wichtig für das Linken:

* **Inline-Routinen:** beginnt eine Routine mit `$A000,len`, kopiert der
  Compiler `len` Bytes an die Aufrufstelle statt `jsr`; `$A001,adr.l`
  wählt eine eigene Fassung für den Blitz-Modus (`blitz2.s:11541 doajsr2`,
  Spec 09 §9.3.2).
* **`ALibJsr token[,form]`** in Lib-Code: `jsr` mit dem Platzhalter
  `($8000|form)<<16 | token` (`BLibJsr`: `$C000|form`) und einer Relocation.
  Beim Einbinden erkennt der Compiler die Relocation am gesetzten Bit 15 des
  Zielwortes (5.6).
* **Relocations:** nur HUNK_RELOC32 in den eigenen Hunk; Offsets < `$12`
  (Header) werden ignoriert, solche im Fehlerprüf-Bereich ohne Debugger
  ebenfalls.

### 4.7 Laden der Libraries [S]

`loadlibs` (`blitz2.s:6374`, `asmlibs.c loadlibs()`) liest beim Start
`acidlibs`, dann `deflibs` je am Stück (`loadlibgroup`), danach jede
eingeschaltete Datei unter `userlibs/<verzeichnis>/` (`loadblitzlibs`,
`loadalib`). `fixlibs` (6525) verkettet alles in `firstlib`, setzt bei
normalen Libs `erstes Token = lib·128+1`, verlinkt die Einträge und hängt
die Namensblöcke an die Token-Liste (`lasttoken`). Es gibt **keine**
Ersetzung gleicher Lib-Nummern wie in AB3 (Spec 09 §9.1.1) – `findlib`
nimmt die erste passende [Schluss].

Hinweis: `bb2tok.py` sucht Namen in der Reihenfolge deflibs vor acidlibs;
v1.95 lädt acidlibs zuerst. Für die Tokens der CD macht das keinen
Unterschied (keine doppelten Namen), geprüft ist die Reihenfolge von 2.10
nicht.

### 4.8 Residents [S]

Eine `.res` ist ein Executable (mit LoadSeg geladen), dessen Hunk vorab
übersetzte NewTypes, Makros und Konstanten enthält – kein Code. Erzeugt
mit „CREATE RESIDENT“ (`blitz2.s:4105 menu5`), geladen bis zu 8 Stück aus
der Liste in der `.xtra` (`blitz2.s:4393 loadres`, `stmts.c loadres()`),
in die Listen `firsttype`/`firstmacro`/`firstconst` eingehängt
(`fixtypes`/`fixmacs`/`fixconsts`), dann auf Namensgleichheit geprüft
(„Clash in Residents“). Format: [formate.md](formate.md) (gleich AB3,
Spec 06 §6.1.1).

---

## 5 Übersetzungsablauf und Linken

### 5.1 Durchläufe [S]

`compile` (`blitz2.s:1917`, `compile.c compile()`):

```
compile
 ├─ dopass                         ← Durchlauf 1 mit den Puffergrößen aus den Optionen
 ├─ [Option "Make smallest code"]: alloxadj → Puffer auf die benutzte Größe
 │                                   verkleinern, dopass noch einmal
 └─ [Puffer übergelaufen (nomemleft)]: Requester "Not Enough Object Memory":
        1 = +50 % und neu, 2 = smallest, 3 = Abbruch
```

`dopass` übersetzt das **ganze Programm in einem Durchgang**: Vorwärts
verwendete Labels werden als Verweisliste gesammelt und bei der Definition
nachgetragen (`blitz2.s:3011 handlelab2`). Der zweite Lauf von „Make
smallest“ ist nötig, weil sich mit der Puffergröße die Adressen ändern
(alle Adressen im Code sind absolut, 5.2). Für CREATE EXECUTABLE ist
„smallest“ immer an; im Beispiel zeigt das Log zweimal „COMPILING“ [B].

### 5.2 Der Objektpuffer [S]

`makeallox` (`blitz2.s:3988`, `stmts.c makeallox()`) holt **einen**
Speicherblock (MEMF_CLEAR) und teilt ihn in vier Bereiche mit je eigenem
Schreibzeiger:

```
pcat ──► Programmcode      (pc)     Option "Object Buffer" (16384)
libat ─► Library-Code      (lib)    Option "Libs Buffer"   (32768)
data1at► System-Daten      (data1)  Option "System Buffer"  (4096): String-Literale, Tabellen
data2at► DATA-Anweisungen  (data2)  Option "Data Buffer"    (4096)
allat
```

Alle Adressen, die der Compiler erzeugt (Sprünge, `jsr` auf Lib-Code,
Literale), sind **absolute Adressen in diesem Puffer**. Jede solche Stelle
kommt in die Reloc-Liste `firstoff` (`blitz2.s:6997 addoff`). Daher kann
RUN den Puffer direkt ausführen, und `savefile` rechnet die Adressen beim
Schreiben auf Hunk-Offsets um (5.10). Ein Überlauf bricht nicht ab, sondern
setzt `nomemleft` und schreibt nicht weiter (5.1).

### 5.3 Zeilen und Anweisungen [S]

```
dopass (2556)                  compile.c dopass()
 ├ setcvars, varoff=$8000, resetlibs, 4×NOP an pcat (Platz für jsr inits)
 ├ [debugga] uselib runerrlib + debuglib/dhandlerlib bzw. rundebuglib
 └ passloop: nextline (2038) → doline (2780) → dostatement (2953)
      chkstop · expandmax (Makros !name{…}) · debugstuff (trap #1)
      handlechar (2961):
        '.' / Name ohne Rest  → Label            handlelabel/handlelab2
        '#name=…'             → Konstante         handleconst
        Name{…}               → Prozeduraufruf    handleproc
        Name …                → Zuweisung (LET)   dolet2 (tokens2.c)
        Token < $8000+60      → atokens[n] (eingebaute Anweisung)
        Token 60…178          → asmit (Inline-Assembler)
        Token mit Lib-Bits    → handlelib (3169) → dolibtoke (3181)
```

Statement-Handler werden über die Sprungtabelle `atokens` (92 Einträge,
`blitz2.s:1298`) mit dem nächsten Zeichen in d0 angesprungen. `CNIF`/`CSIF`
(bedingte Übersetzung) und `` ` ``-Makroparameter behandelt schon `getchar`
(`blitz2.s:7042`) bzw. `expandmax` (2121).

### 5.4 Ausdrücke und Code-Erzeugung [S]

* `eval` (`blitz2.s:8857`, `eval.c eval()`) ist ein Operator-Präzedenz-
  Parser mit eigenem Operator-/Präzedenzstapel; er erzeugt direkt Code in
  Datenregister `D(regat)` (ab d0 aufwärts; reichen sie nicht, über den
  Stack). Typen: 1 Byte, 2 Word, 3 Long, 4 Quick (16.16), 5 Float (FFP),
  7 String; Standardtyp ist Quick.
* Code entsteht aus **Vorlagen** (Maschinencode-Schnipsel im Datenbereich
  des Compilers, `blitz2.s:914 objects` … `1254`, außerdem in `tokens1`/
  `tokens2`), die kopiert (`pokecode`) oder als Opcode-Wort mit
  eingesetzten Registern/Offsets geschrieben werden (`pokewd`/`pokel`). Im
  C-Port sind es `UWORD`-Arrays mit den Originalbytes.
* Arithmetik, die der 68000 nicht direkt kann (Long-Division/Multiplikation,
  FFP, String-Vergleich, MOD …), wird ein `jsr` auf eine System-Lib-Routine
  (`tokejsr`, `blitz2.s:11434`).
* **Konstante Ausdrücke** (Konstanten, `CNIF`, Array-Grenzen …) wertet
  `evalconst2` (`blitz2.s:11880`, `libcall.c evalconst2()`) aus, indem es
  Code in einen separaten Puffer erzeugt und **im Compiler ausführt**
  (`cfetchmode`: `jsr` direkt in den Lib-Code im Compilerspeicher). Deshalb
  läuft blitz2 nur auf einem 68k.
* Es gibt keinen Optimierer über Anweisungen hinweg; Folgen wie
  `move.l d0,-(a7) : move.l (a7)+,d0` bleiben stehen (Abschnitt 6).

### 5.5 Ein Library-Befehl wird übersetzt [S]

Für Anweisungen `handlelib` → `dolibtoke`, für Funktionen
`libfunction` (`blitz2.s:10112`, `eval.c libfunction()`) → `dolibtoke`:

1. `findtoke` (`blitz2.s:6277`) sucht Lib und Eintrag, markiert die Lib
   als benutzt (`uselib3`, 5.6).
2. `countpees` zählt die Argumente; `dolibtoke` wählt die **Form** mit
   passender Anzahl (Überladung, Wiederholgruppen von `!repargs`).
3. `fetchlibps` (`blitz2.s:5563`) wertet die Argumente aus: das erste nach
   d0, das nächste nach d1 usw. (`regat`); Argumente mit `#push` und
   Wiederholgruppen kommen auf den Stack; bei `#unknown` (Typ 8) wird der
   Typ als Wort mitgegeben. Die Zahl der Wiederholungen merkt `numreps`.
4. **String-Argumente:** Der Wert im Register ist die Adresse der Zeichen
   im String-Arbeitspuffer (a3 läuft beim Aufbau mit; `maineval2` schließt jedes String-Argument mit `clr.b (a3)+` ab, `sbtolong`, a3 steht danach hinter der 0), die **Länge** liegt
   als Long auf dem Stack. Hat der Aufruf Stack-Argumente (`stackuse` > 0),
   erzeugt `dolibtoke` davor `lea stackuse(a7),a2` (Vorlage `preplibst`,
   `blitz2.s:1088`): a2 zeigt über die gestapelten Werte, die Routine liest
   sie mit `-(a2)`. Bei Wiederholgruppen kommt `move.w #numreps,d7`
   (`movenumrep`) dazu.
5. `userjsr` → `makelsub3` (`blitz2.s:11474`, `libcall.c makelsub3()`):
   * `fetchregs`/`toreg` (11647) erzeugen für jedes `!libs`-Paar das Laden
     eines Werts einer anderen Lib, meist `move.l base(a5),a6` (Basis aus
     dem Init-Ergebnis, a5-relativ) oder einen Objektzeiger aus
     Objektnummer und Tabelle; `uselib` bindet die Lib dabei ein.
   * mit Debugger: `jsr fehler1` (Prüfroutine vorher).
   * `doajsr2` (11541): Inline-Code kopieren oder `jsr abs.l` auf
     `Adresse der Lib im Programm + Offset` (mit Reloc); `$A001`-Präfix für
     Blitz-/Amiga-Modus.
   * mit Debugger: `jsr fehler2`.
6. Danach räumt `addq #n,a7` bzw. `lea stackuse(a7),a7` (`fixlibst`) den
   Stack ab.

Das Ergebnis steht in d0; ein String-Ergebnis schreibt die Routine ab a3,
danach zeigt a3 auf das Nullbyte und d0 enthält die Länge
(`libs-c/libs/stringfunclib.c`, Kopf).

Eingebaute Befehle `ALibJsr`/`BLibJsr`/`TokeJsr`/`SysJsr` (Lib-Quellen,
Inline-Assembler) erzeugen dieselben Platzhalter bzw. Aufrufe direkt
(`blitz2.s:3571 dolibjsr`, 3599 `dotokejsr`).

### 5.6 Welche Library-Routinen ins Programm kommen [S]

**Immer die ganze Library.** `uselib3` (`blitz2.s:5841`,
`asmlibs.c uselib3()`) kopiert beim ersten Gebrauch einer Lib ihren Code
ab Code-Beginn bis zum Ende – ohne Debugger nur bis `error` – an den
aktuellen `lib`-Zeiger (Reihenfolge = Reihenfolge der ersten Verwendung)
und verarbeitet sofort:

* **Relocations:** jedes Langwort += Zieladresse − Code-Beginn, Offset in
  `firstoff`. Relocations, deren Ziel ein Wort mit Bit 15 ist, sind
  `ALibJsr`-Platzhalter: `handlejsrs` (6158) → `maketjsr` (6189) trägt
  `(token, form, Patch-Adresse)` in `tokeslist` ein.
* **Globale Variable** für das Init-Ergebnis (Feld `$1A` ≠ 0): nächster
  freier a5-Offset, Word oder Long; Offset nach Lib-Feld `$0A`.
* **Objekt-Libs:** `maxslib` dazu, Platz für die Objekttabelle in den
  Globals (Größe aus Maximum × 2^shift, Maximum aus `.xtra`/Optionen).
* **Abhängigkeiten:** die Listen an den Init- und Finit-Einträgen
  (`uselib2`, 6237) werden rekursiv eingebunden; `!libs`-Paare binden ihre
  Libs beim Übersetzen eines Aufrufs ein (`toreg`).

Am Ende des Durchgangs löst `fixjsrs` (6003) die gesammelten Platzhalter
auf: `tokecode` (6047) liefert die Adresse der Ziel-Routine (und bindet
deren Lib ein, falls noch nicht geschehen). Braucht die Routine
Registerparameter aus `!libs` oder Prüfaufrufe, erzeugt `tokecode` einen
kleinen Stub hinter dem Programm (`… jsr routine : rts`); sonst wird die
Adresse direkt eingetragen. Vor den Stubs steht ein `jmp`, der sie
überspringt.

### 5.7 Programmende, Inits und Finits, Startcode [S/B]

Nach der letzten Zeile hängt `dopass` (`blitz2.s:2612`ff.) an:

```
pcat:   jsr inits ; nop              ← 4 NOPs aus dopass, von makeinits überschrieben
        <Programm>
        [Debugger: trap #1-Block, jsr finalscheck, moveq #3,d0 : trap #0]
endop:  [doamiga, Interrupts aus]
        jmp über_die_Stubs
        <Stubs aus fixjsrs>
über:   <Finits>                     makefinits (11327): aufsteigende Lib-Nummer
        moveq #0,d0 : rts            ← Programmende
inits:  <Inits>                      makeinits (11140): absteigende Lib-Nummer
        rts
```

* **Startcode** gibt es nicht als festen Block: Das Programm beginnt mit
  `jsr inits`. Als erstes läuft (höchste Nummer) `varslib` 65534: Sie
  bekommt in d0 die Größe der Globals, holt Speicher (`AllocMem`,
  MEMF_CLEAR) und setzt **a5 = Block + $8000**; die Globals liegen also bei
  negativen 16-Bit-Offsets `-32768(a5)` … (der Compiler beginnt `varoff`
  bei `$8000`) [B, Beispiel].
* Einige Inits bekommen vom Compiler Werte in d0 (`makeinits`):
  `varslib` Größe der Globals, `strings1lib` Größe des String-Puffers
  (Option „String Buffer“, 10240), `exitslib` die Adresse `endop`,
  `datalib` den Beginn von data2, `debuglib` Auto-Run-Flag, `dhandlerlib`
  `&dseg`.
* Das Ergebnis jeder Init-Routine wird in ihre globale Variable geschrieben
  (`move.l d0,off(a5)`).
* `End` ist `jsr endjmp` (`exitslib`, `blitz2.s:13876 doend`): zurück zum
  gesicherten Stack und Sprung nach `endop`, so dass die Finits auch bei
  `End` mitten im Programm laufen [Schluss aus Init-Parameter und Code,
  Routine nicht disassembliert].
* Gibt es keinen Init-Code, setzt `makeinits` `noinits = 8` und
  `savefile` lässt die 8 Bytes am Anfang weg.

### 5.8 Registermodell zur Laufzeit [S/B]

| Register | Bedeutung |
|---|---|
| a5 | Globals (Block + $8000); Init-Ergebnisse aller Libs, Variablen des Hauptprogramms |
| a4 | Locals der aktuellen Prozedur (`link a4,#-n`, Vorlage `linka4`) |
| a3 | Schreibzeiger im String-Arbeitspuffer |
| a2 | Zeiger über die gestapelten Lib-Argumente (Stringlängen) beim Lib-Aufruf |
| a6 | Library-Basis für den nächsten Aufruf (gecacht über `lasta6`) |
| d0–d7 | Ausdrucksauswertung, Lib-Argumente ab d0; d7 = Zahl der Wiederholgruppen |
| a7 | Stack (Gosub, For-Rahmen, Stack-Argumente) |

Die Lib-Routinen erhalten a3 (definiert verändert), a4, a5, a7; was sie
sonst verändern dürfen, ist nicht einheitlich (Spec 09 §9.3, 12).

### 5.9 Laufzeit-Fehlerprüfung an/aus [S]

| | ohne „Runtime Errors“ | mit |
|---|---|---|
| Lib-Code | bis `error` | ganz (Prüfroutinen ab `error`) |
| Prüfaufrufe `fehler1/fehler2` | nein | ja |
| `trap #1`-Block je Anweisung | nein | ja (8 Byte + Trap; in Executables über `rundebuglib` nur übersprungen – kostet trotzdem eine Exception je Anweisung) |
| Array-/Maximum-Prüfungen, Stackprüfung | nein | ja (`maxchk`, `chkstak` nur bei RUN) |
| Zusatz-Libs | – | `runerrlib` + `debuglib`/`dhandlerlib` (RUN) bzw. `rundebuglib` (Executable) |

Im Beispiel: 5500 statt 7824 Bytes Datei (Abschnitt 6) [B].

### 5.10 Schreiben des Executables (`savefile`) [S/B]

`savefile` (`blitz2.s:6800`, `parse.c savefile()`):

1. Jede Reloc-Stelle in `firstoff`: Langwort −= `pcat + noinits`
   (absolute Pufferadresse → Hunk-Offset).
2. Datei schreiben:

```
HUNK_HEADER  $3F3, 0, 1 Hunk, 0, 0, Größe
HUNK_CODE    $3E9, Größe  (objlen − noinits, auf Langwörter gerundet)
             ┌───────────────────────────────┐ 0
             │ jsr inits / nop               │
             │ Programm, Ende, Stubs, Finits,│
             │ moveq #0,d0 : rts, Inits      │
             ├───────────────────────────────┤ libat − pcat
             │ Lib-Code (Reihenfolge der     │
             │ ersten Verwendung)            │
             ├───────────────────────────────┤ data1at − pcat
             │ data1: String-Literale        │
             │  (len.l + Zeichen + 0), …     │
             ├───────────────────────────────┤ data2at − pcat
             │ data2: DATA-Werte             │
             └───────────────────────────────┘
HUNK_RELOC32 $3EC, n, Hunk 0, n Offsets, 0
[HUNK_SYMBOL $3F0: Labels, wenn "Create Debug Info"]
HUNK_END     $3F2
```

Es gibt **keinen** DATA- oder BSS-Hunk: Variablen und String-Puffer
werden zur Laufzeit von `varslib`/`strings1lib` angelegt, die Datenbereiche
liegen im Code-Hunk. Wegen „smallest“ schließen die vier Bereiche lückenlos
aneinander an. Optional schreibt `savefile` ein Icon (`obj.info`).

---

## 6 Beispiel: drei Zeilen bis zum Hunk

Quelle (`hallo.bas`), übersetzt mit dem Original v1.95 in Amiberry:

```
a$="Hallo"
Print Left$(a$,2)
End
```

```
python3 -I compilerlauf/tools/bb2compile.py --compiler 195 --no-runerr --run --tag doc-example hallo.bas
python3 -I compilerlauf/tools/bb2compile.py --compiler 195 --run --tag doc-example-dbg hallo.bas
```

Ergebnis [B]: 5500 Bytes ohne, 7824 Bytes mit Laufzeitprüfung (die
Voreinstellung des Compilers); Ausgabe beider Programme `Ha` (`Print`
ohne Zeilenvorschub). Das Log zeigt zwei Durchläufe („COMPILING“ zweimal,
„smallest“). Ergebnisverzeichnisse danach gelöscht.

### 6.1 TED-Zeilen [B]

```
61 24 3D 22 48 61 6C 6C 6F 22 00         a$="Hallo"
DD 81 20 DC 81 28 61 24 2C 32 29 00      Print Left$(a$,2)     Print=$DD81, Left$=$DC81
80 02 00                                 End                    $8002
```

`$DD81` = `$8000 | 187·128 + 1` (printlib, Eintrag 1),
`$DC81` = `$8000 | 185·128 + 1` (stringfunclib). Das Leerzeichen nach `Print`
bleibt erhalten, `a$` bleibt ASCII.

### 6.2 Hunk-Aufbau (ohne Laufzeitprüfung) [B]

HUNK_HEADER (1 Hunk) · HUNK_CODE 5132 Bytes · HUNK_RELOC32 mit 79 Einträgen ·
HUNK_END (32 + 5132 + 332 + 4 = 5500 Bytes).

| Offset | Inhalt |
|---|---|
| `$0000`–`$0007` | `jsr $C8` (Inits) · `nop` |
| `$0008`–`$0091` | die drei Zeilen |
| `$0092`–`$009D` | `End` = `jsr $13EE` (exitslib) · `jmp $9E` (leere Stub-Liste) |
| `$009E`–`$00C7` | Finits, `moveq #0,d0 : rts` |
| `$00C8`–`$0131` | Inits, `rts` |
| `$0132`–`$13FF` | Libraries (4814 Bytes): |
| | `$0132` varslib · `$0170` strings1lib · `$01C2` memlib · `$0340` strings2lib · `$03C6` printlib (2260 Bytes) · `$0C9A` inputoutputlib · `$0EFC` doslib (aus **deflibs**) · `$0F28` stringfunclib · `$13A4` ffplib · `$13D2` exitslib |
| `$1400`–`$140B` | data1: `00000005 "Hallo" 00` + Füllbyte |

Lib-Grenzen per Abgleich mit den Lib-Images aus `acidlibs`/`deflibs`
(Relocs maskiert) bestimmt. Die drei Zeilen erzeugen 138 Bytes Code, die
Libraries 4814 Bytes: `Print` zieht `printlib`, über deren Abhängigkeiten
`inputoutputlib`, `doslib` und `ffplib` mit.

### 6.3 Disassembly mit Erläuterung [B]

Globals (a5-relativ, 32 Bytes, `move.l #32,d0` im Init):
`-$8000` varslib · `-$7FFC` **a$** · `-$7FF8` memlib · `-$7FF4`
String-Puffer (strings1lib) · `-$7FF0` DOSBase · `-$7FEC` stringfunclib ·
`-$7FE8` ffplib (mathffp-Basis) · `-$7FE4` exitslib (`endop`).

```
0000  4EB9 000000C8   jsr     inits                ; reloc
0006  4E71            nop
      ; a$="Hallo"
0008  266D 800C       move.l  -$7FF4(a5),a3        ; a3 = Anfang String-Puffer
000C  207C 00001400   move.l  #lit_Hallo,a0        ; data1, reloc
0012  2F18            move.l  (a0)+,-(a7)          ; Länge 5 auf den Stack, a0 = Zeichen
0014  4EB9 0000017A   jsr     copstring            ; strings1lib $C600: nach (a3)+
001A  421B            clr.b   (a3)+
001C  202D 800C       move.l  -$7FF4(a5),d0
0020  2640            move.l  d0,a3
0022  45ED 8004       lea     a$(a5),a2            ; Ziel-Variable
0026  2C6D 8008       move.l  -$7FF8(a5),a6        ; memlib (Speicher für den String)
002A  4EB9 00000340   jsr     astring              ; strings2lib $C701: globale String-Zuweisung
      ; Print Left$(a$,2)
0030  266D 800C       move.l  -$7FF4(a5),a3
0034  2F0B            move.l  a3,-(a7)             ; Adresse des Teilstrings merken
0036  45ED 8004       lea     a$(a5),a2
003A  7000 2F00       moveq #0,d0 / move.l d0,-(a7); Länge 0, falls a$ leer
003E  4A92 6700000E   tst.l (a2) / beq.w 0050
0044  2052 5948 2E98  move.l (a2),a0 / subq #4,a0 / move.l (a0)+,(a7) ; Länge steht vor den Zeichen
004A  4EB9 0000017A   jsr     copstring            ; a$ in den Puffer kopieren
0050  421B            clr.b   (a3)+
0052  202F0004 2E9F   move.l 4(a7),d0 / move.l (a7)+,(a7) ; d0 = Adresse, Länge bleibt auf dem Stack
0058  223C 00000002   move.l  #2,d1                ; 2. Argument
005E  45EF 0004       lea     4(a7),a2             ; preplibst: a2 über die Stringlänge
0062  4EB9 00000FA2   jsr     Left$                ; stringfunclib, d0 = Länge des Ergebnisses
0068  584F            addq.w  #4,a7                ; Länge abräumen
006A  2F00 201F       move.l d0,-(a7) / move.l (a7)+,d0   ; (unoptimiert)
006E  7207 3F01       moveq #7,d1 / move.w d1,-(a7)       ; Typ 7 = String (#unknown-Argument)
0072  2F00            move.l  d0,-(a7)             ; Länge
0074  421B            clr.b   (a3)+
0076  202D 800C 2F00  move.l -$7FF4(a5),d0 / move.l d0,-(a7) ; Adresse (Ergebnis steht am Pufferanfang)
007C  45EF 000A       lea     10(a7),a2            ; preplibst, stackuse = 10
0080  3E3C 0001       move.w  #1,d7                ; movenumrep: 1 Wiederholung
0084  2C6D 8018       move.l  -$7FE8(a5),a6        ; !libs: ffplib-Basis nach a6
0088  4EB9 00000764   jsr     Print                ; printlib
008E  4FEF 000A       lea     10(a7),a7            ; fixlibst
      ; End
0092  4EB9 000013EE   jsr     endjmp               ; exitslib $C800
0098  4EF9 0000009E   jmp     $9E                  ; endop: über die (leere) Stub-Liste
      ; Finits, aufsteigende Lib-Nummer
009E  226D 8010 4EB9 00000F12   ; doslib 252: CloseLibrary(DOSBase)
00A8  226D 8018 4EB9 000013C2   ; ffplib 64935
00B2  4EB9 000001E0             ; memlib 65530: allen Speicher freigeben
00B8  203C 00000020 4EB9 0000015C ; varslib 65534: Globals freigeben
00C4  7000 4E75       moveq #0,d0 : rts            ; Programmende
      ; Inits, absteigende Lib-Nummer
00C8  203C 00000020 4EB9 00000132 2B40 8000  ; varslib(32): a5 = AllocMem(32)+$8000
00D8  4EB9 000001C8 2B40 8008                ; memlib
00E2  203C 00002800 4EB9 00000170 2B40 800C  ; strings1lib(10240): String-Puffer
00F2  203C 00000098 4EB9 000013DA 2B40 801C  ; exitslib(endop = $98)
0102  4EB9 000013A4 2B40 8018                ; ffplib: mathffp öffnen
010C  4EB9 00000EFC 2B40 8010                ; doslib (deflibs): dos öffnen
0116  4EB9 00000434                          ; printlib (kein Rückgabewert)
011C  4EB9 00000F72 2B40 8014                ; stringfunclib
0126  2C6D 8010 4EB9 00000D38                ; inputoutputlib, a6 = DOSBase
0130  4E75            rts
0132  ; varslib: 2F08 223C00010001 2C780004 4EAEFF3A … 2A40 9AFC8000 …
      ;   AllocMem(d0, MEMF_PUBLIC|MEMF_CLEAR); a5 = d0; suba.w #$8000,a5
```

Deutungen der Laufzeit-Routinen (`copstring`, `astring`, wer die Länge auf
dem Stack bei `$12` abräumt) stammen aus den Lib-Quellen bzw. dem
Zusammenhang, die Routinen selbst wurden nicht disassembliert.

### 6.4 Mit Laufzeitprüfung [B]

`runerrlib` und `rundebuglib` kommen hinzu und stehen vorn im Lib-Bereich
(ab `$0186`, `dopass` bindet sie vor der ersten Zeile ein), `exitslib`
folgt als Abhängigkeit direkt danach. Jede Anweisung beginnt mit einem
Debug-Block:

```
0008  4E41            trap #1
000A  40089DF8        dc.l  Adresse der TED-Zeile 1 (Speicher des Editors)
000E  0009            dc.w  Offset der Anweisung (9 = Beginn von chars[])
0010  0032            dc.w  Abstand zum nächsten Block ($3A − $08)
…
00A6  4E41 4007F330 0009 0010   ; Zeile 3 (End)
00AE  4EB9 0000086A             ; jsr endjmp
00B6  4E41 …                    ; Block am Programmende
00BE  4EB9 000004B4             ; jsr finalscheck ($C300)
00C6  4E41 … 0000               ; letzter Block, Abstand 0
00CE  7003 4E40                 ; moveq #3,d0 : trap #0 (eopcode)
```

---

## 7 Hinweise für den Vergleich mit AmiBlitz3

* **Gleich geblieben:** BigLib-Format, Token-Nummern `lib·128+i`,
  Aufrufkonvention (d0…, a2-Stapel für Stringlängen, a3 String-Puffer,
  a5/a4), Einbinden ganzer Libs mit Init/Finit, `ALibJsr`-Platzhalter,
  Inline-Präfixe `$A000`/`$A001`, Globals ab `-32768(a5)`
  (Spec 09 §9.3–9.4, Spec 12).
* **Ein Code-Hunk, absolute Adressen im Puffer, Ausführen im
  Speicher:** Die Struktur „Programm · Finits · Inits · Libs · Daten“ in
  einem Hunk ist das Ergebnis eines Compilers ohne Linker.
* **Editor als Host-Oberfläche, Compiler als Bibliothek:** In BB2 liegt die
  Prozesshoheit beim Compiler, der Editor ist ein Unterprogramm; Fehler
  gibt es genau einen, als Zeilennummer an den Editor.
* **Konstantenauswertung durch Ausführen erzeugten Codes** – ein
  Nachbau auf einem anderen Host braucht dafür einen 68k-Emulator oder eine
  eigene Auswertung mit bitgenauer FFP/Quick-Arithmetik.
* **Laufzeitprüfung = TRAP je Anweisung**, mit Zeilenadressen des Editors
  im Code; deshalb sind Debug-Executables nicht reproduzierbar.

---

## 8 Offene Punkte

* Genaue Funktion von `dhandlerlib` (Lib 1) und `defaultdbug`: nur aus
  `makeinits` (bekommt `&dseg`) und dem Lib-Kopf (Init/Finit, braucht
  DOS- und Graphics-Basis) bekannt – nicht geprüft.
* Reihenfolge `acidlibs`/`deflibs` beim Laden in v2.10 (v1.95: acidlibs
  zuerst) – nicht geprüft.
* `End` → `exitslib` → `endop`: Ablauf aus Init-Parameter und Code
  gefolgert, Routine nicht disassembliert.
* Fehler in Include-Dateien auf der `INCLUDE`-Zeile: aus `nextline`
  gefolgert, nicht ausprobiert.
* Wer bei `a$="Hallo"` die gestapelte Länge abräumt (vermutlich `astring`)
  – nicht geprüft.
