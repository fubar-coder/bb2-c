# Plan Phase 3: Emulator-A/B-Tests von TED-C

Grundlage: [PLAN.md](PLAN.md) §6 (c), [reference.md](reference.md),
[overview.md](overview.md) §4, [abweichungen.md](abweichungen.md) und die
vorhandene Emulator-Infrastruktur in `../compilerlauf` (Runner
`tools/bb2compile.py`, Stub und Requester-Werkzeuge in `tedstub/`,
[`../compilerlauf/README.md`](../../compilerlauf/README.md),
[`../compilerlauf/tedstub/README.md`](../../compilerlauf/tedstub/README.md))
sowie [`../ci-exploration.md`](../../ci-exploration.md).

Stand bei Planung (26.10.2026): Phase 2b abgeschlossen, `ctest --preset
amiga` (ref_verify, checkhunk, checkdata, checklabels, checklink) und
`ctest --preset host` (11 Tests, u. a. `test_loadsave`: 724 Dateien,
0 Fehler) grün. TED-C ist noch **nie gelaufen** – Phase 3 ist der erste
Start auf einem Amiga.

Ziel: **gleiches beobachtbares Verhalten** von TED-C (`build/amiga/ted`)
und Referenz-TED (`build/amiga/ref/ted`, vasm-Build von `ted.s`), gemessen
im Emulator mit beiden Compilern, ohne Bedienung, nie blockierend.
Nicht Ziel: Abgleich mit dem CD-Binary von TED (reference.md §1, eigener
Schritt), Leistungsmessung.

---

## 1 Ausgangslage

| Baustein | Pfad | Stand |
|---|---|---|
| Referenz-TED | `ted-c/build/amiga/ref/ted` (57060 Bytes) + `ted.lst`/`ted.map`/`ted.labels` | vorhanden, `ref_verify` grün |
| TED-C | `ted-c/build/amiga/ted` (69432 Bytes), Linkmap `build/amiga/ted.map`, Symbole `build/amiga/ted.sym` | gebaut, ungetestet |
| blitz2 v1.95 (vasm) | `compilerlauf/blitz2-195` | im Korpus bewährt |
| blitz2-c | `blitz2-c/build/amiga/blitz2` | im Korpus bytegleich mit 1.95 (724/724) |
| Libraries/Hilfen | `cd/blitz2/{deflibs,acidlibs,userlibs,defaultdbug,help,help.dat}` | von `bb2compile.prepare()` kopiert (außer `help`, `help.dat`) |
| Emulator-Start | `bb2/tools/amiberry.cmake` (ohne Fenster; Skript nach `AB3C:build/amiberry/run.script`, `user-startup` führt es aus, Ende über `RES:done` und `uaequit`, Zeitlimit beendet Amiberry; bis 27.10.2026 `../ab3c/tools/amiberry_run.py`) | bewährt; Konfiguration über `--config`/`AMIBERRY_CONFIG` |
| Sperre | `libs-c/build/.amiberry.lock` (`bb2compile.LOCK`) | ein Emulator gleichzeitig |
| Synthetische Eingaben | `compilerlauf/tedstub/src/reqclose.h` (IntuiMessages in den UserPort, `-DREQCLOSE_INPUT`: alte input.device-Variante), Starter `reqtrap.c` (AutoRequest/EasyRequest-Patch, `SystemTags`) | bewährt für Requester; **kein** Werkzeug für TED-Bedienung |
| Orakel ohne TED | `compilerlauf/tools/bb2tok.py` (Tokenizer wie TED), `tedstub` (übersetzt ohne Oberfläche), `corpus_check.mask_trap1()` | vorhanden |

Neu zu bauen sind der Begleitprozess `teddrive` (§4), der Runner
`tedab.py` (§5), der Dump-Decoder `teddump.py` (§6.1) und eine
Emulator-Konfiguration mit nativem Workbench-Screen (§3).

## 2 A/B-Aufbau

### 2.1 Vier Kombinationen

| Kürzel | TED | Compiler | Rolle |
|---|---|---|---|
| **R5** | Referenz (`build/amiga/ref/ted`) | blitz2-195 | **Referenz** (alles Original, nur vasm-assembliert) |
| **C5** | TED-C (`build/amiga/ted`) | blitz2-195 | **Pflicht**: prüft TED-C gegen den Original-Compiler (Register-Schnittstelle `comdata`, Rückrufe `menurout`/`loadxtra`/`savextra`/`cleanup` in Asm-Code) |
| **Cc** | TED-C | blitz2-c | **Pflicht**: Zielkombination (alles C); zwei `-fbaserel`-Programme mit je eigenem a4 (overview.md §4 Risiko 2) |
| **Rc** | Referenz | blitz2-c | Kontrolle: trennt bei einer Abweichung Cc↔R5 die Ursache (Compiler oder Editor); blitz2-c ist im Korpus schon gegen 1.95 bestätigt, daher nachrangig |

Jeder Vergleich ändert nur eine Größe: **R5↔C5** (Editor bei
Original-Compiler) ist der Haupt-A/B-Vergleich, **Rc↔Cc** derselbe mit dem
C-Compiler, **R5↔Rc** bestätigt den Compiler. Erwartung: alle vier gleich
bis auf die Masken in §6. Stufe 1 (§7.1) läuft nur R5 und C5; Rc/Cc kommen
dazu, sobald R5↔C5 sauber ist.

### 2.2 Aufbau im Emulator

Wie `bb2compile.prepare()` (`compilerlauf/tools/bb2compile.py`), aber mit
echtem TED (der Runner hat dafür schon `--ted PATH`), unter
`../ab3c/build/amiberry/ted-ab/TAG/` (Emulator: `AB3C:build/amiberry/ted-ab/TAG/`):

```
TAG/blitz2/       blitz2, ted, deflibs, acidlibs, userlibs/, defaultdbug, help, help.dat
TAG/L/            blitzeditor.opts (je Test vorgegeben oder leer)
TAG/orig/         unveränderte Testquellen (+ .xtra)
TAG/work/         Arbeitskopien, die TED lädt und speichert (je Lauf neu kopiert)
TAG/out/          Dumps, Logs, Ergebnisdateien, Executables
TAG/teddrive      der Begleitprozess
```

* **blitz2 startet TED selbst**: `blitz2:blitz2 AB3C:…/work/NAME.bb2` →
  blitz2 lädt `blitz2:ted`, ruft Hunk+4 (`outside`) und danach `.tomain`
  mit dem Dateinamen; TED lädt die Datei (wie im tedstub-Ablauf,
  `tedstub/README.md` „Ablauf“). Beenden über das Menü QUIT (`escape`)
  → TED kehrt zurück → blitz2 endet.
* **Kein Standalone-TED**: ohne blitz2 ist `tokens = 0` (abweichungen.md,
  Zeile `findtoke`), tokenisierte Quellen würden über die Adresse 0
  gelesen. Alle Tests laufen über blitz2.
* **`l:blitzeditor.opts` isolieren** (`defname`, src/core/state.c:60):
  `loaddefs`/`savedefs` lesen/schreiben `L:` der Systemplatte. Der Lauf
  setzt `Assign L: AB3C:…/TAG/L` + `Assign L: SYS:L ADD` (eigenes
  Verzeichnis zuerst, Schreiben geht dorthin) und am Ende `Assign L: SYS:L`.
  Ohne vorgegebene Datei startet TED mit `defstart` (Daten aus dem Listing,
  `checkdata`). Vor dem ersten Lauf prüfen, dass `SYS:L/blitzeditor.opts`
  auf der Platte **nicht** existiert (sonst gälte sie bei Tests ohne eigene
  Datei – `List SYS:L/blitzeditor.opts` im Gate-Lauf, §3.3).
* Testquellen: `.bas` aus `compilerlauf/corpus/tests` werden wie in
  `prepare()` mit `bb2tok.tokenise()` erzeugt, CD-Quellen aus `cd/` kopiert
  (mit `.xtra`).

## 3 Emulator: natives Chipset ist Pflicht

### 3.1 Befund

TED öffnet seinen Screen **als Kopie des Workbench-Modus**:
`calcscreen` (ted.s:4619, src/screen.c `calcscreen`) liest
`GetScreenData(…, WBENCHSCREEN)` und übernimmt Breite, Höhe und
`ViewPort.Modes`; `extnewscreen` (src/data.c) setzt zusätzlich
`SA_LikeWorkbench` (nur `SA_Depth` = 2 wird überschrieben). Danach zeichnet
TED **direkt** in `BitMap->Planes[0/1]` (CPU, `print3`) und mit dem Blitter
über `$DFF000` (`eraseline`, `scrollup`/`scrolldown`, `cls`; src/screen.c).

Die gemeinsame Testkonfiguration `~/Amiberry/Configurations/A1200-AB3.uae`
hat eine **RTG-Workbench** (uaegfx, `gfxcard_size=16`, 1680×1050, siehe
ci-exploration.md §2.1). TED würde dort einen RTG-Screen klonen: die
Bitplanes lägen im Grafikkartenspeicher, der Blitter kann sie nicht
adressieren (Zeichnen ins Leere oder in fremdes Chip-RAM), und mit 1050
Zeilen trifft der Dateirequester den bekannten Überlauf ab 605 Pixel
Fensterhöhe (reference.md §3.2, `libs-c/libs/freqlib.c`). **TED ist mit
dieser Konfiguration nicht testbar** – das gilt für beide TEDs gleich, ist
also kein A/B-Fehler, aber ein Blocker.

### 3.2 Testkonfiguration `A1200-AB3-AGA.uae` (B1 gelöst)

Vom Benutzer angelegt (27.10.2026): `~/Amiberry/Configurations/A1200-AB3-AGA.uae`
= `A1200-AB3.uae` ohne RTG-Karte (`gfxcard_size`/`gfxcard_type` entfernt)
mit eigener Hardfile `Workbench_OS3.2-AB3-AGA.vhd`; Workbench-Modus **PAL
High Res Laced, 256 Farben (Tiefe 8)**. Auswahl über `AMIBERRY_CONFIG`
(`tedab.py` setzt sie selbst, `--config` überschreibt). Konfiguration und
Hardfile gehören dem Benutzer und werden von den Werkzeugen nicht
verändert; eine eigene `A1200-TED.uae` gibt es nicht. Rauchtest des
Benutzers: `run.script`, `RES:`, `AB3C:`, `BB2:` funktionieren.

| Einstellung | Wert | Bemerkung |
|---|---|---|
| `chipset=aga`, `ntsc=false` | wie `A1200-AB3.uae` | PAL/AGA |
| Workbench-Modus | nativ, PAL High Res Laced, Tiefe 8 | TED klont Breite/Höhe/Modus (`calcscreen`), Tiefe immer 2 (`SA_Depth`); Gate 0 hält die tatsächlichen Werte fest (O-P2) |
| RTG-Karte | keine | kein Bildschirm kann auf uaegfx landen |
| `joyport0` | `mouse` (unverändert) | Weg D braucht die Mausposition nicht. **Risiko:** ein Klick der Host-Maus ins Amiberry-Fenster erzeugt echte MOUSEBUTTONS-Nachrichten für TED – während einer Sitzung nicht ins Emulatorfenster klicken |
| `chipmem_size` | 8 (unverändert) | mehr als ein echter A1200; ändert nur `AvailMem` (maskiert) |
| `cpu_speed=max`, JIT | wie bisher | Timing über „TED wartet“ (§4.4), nicht über Zeit |

### 3.3 Gate 0 (vor allen Tests)

Ein Lauf ohne TED: kleines Prüfprogramm (`tedprobe`, eigenes Programm
`tests/amiga/tedprobe.c`, Ziel `build/amiga/tests/tedprobe`) schreibt `GetScreenData(WBENCHSCREEN)` (Breite, Höhe, `ViewPort.Modes`,
`GetVPModeID`, Tiefe), `TypeOfMem(Planes[0])` (muss `MEMF_CHIP` sein),
`AvailMem(MEMF_CHIP)` und das Vorhandensein von `SYS:L/blitzeditor.opts`
nach `out/gate0.txt`. Der Runner bricht ab, wenn der Modus nicht nativ
oder die Größe nicht die erwartete ist. Dasselbe Ergebnis steht in jedem
Bildschirm-Dump (Kopf, §6.1), damit Dumps verschiedener Läufe nur bei
gleicher Geometrie verglichen werden.

## 4 Begleitprozess `teddrive`

### 4.1 Aufbau

`teddrive SKRIPT BEFEHL [ARGS]` – gebaut wie `reqclose` aus
`compilerlauf/tedstub/src/reqtrap.c` (dort: AutoRequest/EasyRequestArgs
gepatcht, Befehl über `SystemTags`, zweiter Prozess über
`CreateNewProcTags`, Ende über Stop-Flag und Signal). Neu ist der zweite
Prozess: statt der autonomen Requester-Schleife von `reqclose.h` spielt er
ein **Skript** ab. Übernommen werden aus `reqclose.h` die erprobten Teile:
Fenstersuche unter `LockIBase` in den Fensterlisten aller Screens,
`rc_valid`, `rc_send` (Forbid + LockIBase, Fenster erneut prüfen,
`PutMsg` einer statischen `ExtIntuiMessage` mit eigenem Reply-Port und
`IDCMPWindow`), Gadgetsuche über IntuiText, Logpuffer. Dafür
`reqclose.h` so aufteilen, dass die Helfer ohne `rc_main` einbindbar sind
(Besitz compilerlauf; `reqclose` muss danach bytegleich oder mit
`reqclosetest` unverändert grün sein), oder die Helfer als Kopie mit
Herkunftsvermerk in `ted-c/tests/amiga/` – Entscheidung beim Umsetzen
(O-P1, §10).

Umgesetzt (Abweichungen von diesem Abschnitt, Begründung in
[PHASE3-LOG.md](PHASE3-LOG.md)): Aufruf `teddrive SKRIPT PRÄFIX BEFEHL
[ARGS]` statt `ENV:TEDDRIVE`/`ENV:TEDDRIVELOG` (Log `PRÄFIXlog`, Ergebnis
`PRÄFIXresult`, Dumps `PRÄFIXNAME.scr`); der AutoRequest/EasyRequest-Patch
kommt vom unveränderten `reqtrap` (`reqtrap teddrive …`), `REQUESTER:`-Zeilen
stehen daher in der `.stdout`-Datei; Skriptsyntax in
`tests/amiga/tdscript.h` (auf dem Host gegen `tedab.py` geprüft).

Ort: `ted-c/tests/amiga/teddrive.c` (+ `teddrive.h`), CMake-Ziel im Preset
`amiga` (`build/amiga/tests/teddrive`), wie `reqtrap` ohne C-Laufzeit
übersetzbar. Der AutoRequest-Patch bleibt: DOS-Requester („Please insert
volume …“) und Alerts über EasyRequest dürfen den Lauf nicht anhalten; ihr
Text erscheint als `REQUESTER: …` im Log.

Skript: Pfad in `ENV:TEDDRIVE` (oder erstes Argument); Log angehängt an
`ENV:TEDDRIVELOG` (Datei auf `AB3C:`, `Open`/`Seek`/`Write`/`Close` je
Zeile wie das tedstub-Log, übersteht Hänger und Abstürze).

### 4.2 Wie TED Eingaben liest – was injiziert werden muss

TED liest **nur IDCMP** des aktuellen Fensters: `getinput`/`getinput2`
(ted.s:5845/5833, src/input.c) holen aus `pickwindow()->UserPort`
(`req_window`, sonst `mywindow`), `handled0` (ted.s:5866) wertet aus:

| Klasse | was TED liest | Injektion |
|---|---|---|
| `IDCMP_RAWKEY` | `Code` (Key-Up ignoriert), `Qualifier` → `rawed_convert` (RawKeyConvert mit eigener InputEvent, F1–F10 → `$81–$8A`, Cursor → `$1C–$1F`, Help → `$8B`) | **direkte IntuiMessage** (wie `rc_send`): `Class`, `Code`, `Qualifier`; Text über `MapANSI` (keymap.library V36) in Rawkey+Qualifier, damit die eingestellte Tastaturbelegung stimmt (TED benutzt sie über console.device) |
| `IDCMP_MENUPICK` | nur `Code` (`FULLMENUNUM`), `menu`/`extmenu` rechnen Menü/Punkt selbst um (src/edit.c `menu`), `NextSelect` ignoriert | **direkte IntuiMessage**, `Code = menu | item<<5 | 31<<11` |
| `IDCMP_GADGETUP/DOWN` | `IAddress` → `GadgetID`, `GadgetType`, `Activation & ENDGADGET` | **direkte IntuiMessage** mit `IAddress` = Gadget (wie reqclose) |
| Return / Esc im Requester | `handled0`: Return → erstes ENDGADGET (Bool), Esc → letztes ENDGADGET | RAWKEY `$44` / `$45` – prüft TEDs eigenen Pfad mit |
| `IDCMP_MOUSEBUTTONS` | nur `Code` (SELECTDOWN/UP); die **Position** liest `getmousexy` (ted.s:4015) aus `myscreen->MouseX/MouseY` | Zeiger muss wirklich stehen → `input.device` `IND_WRITEEVENT` `IECLASS_NEWPOINTERPOS` (`IESUBCLASS_PIXEL`, Screen = TEDs Screen), dann Knopf als RAWMOUSE-Ereignis oder direkte MOUSEBUTTONS-Nachricht |
| Amiga-Tastenkürzel (`COMMSEQ`, z. B. Amiga-S) | erzeugt Intuition aus RAWKEY+Amiga | nur über `input.device` (RAWKEY mit `IEQUALIFIER_RCOMMAND`) |

Daraus zwei Wege:

* **Weg D (direkt, Stufe 1/2)**: Tasten, Menüwahl, Gadgets, Requester-
  Antworten als IntuiMessages in den UserPort. Deterministisch, ohne
  input.device; reqclose hat genau diese Technik im Testemulator bewährt.
* **Weg I (input.device, Stufe 3)**: nur für Mausposition, Klicks ins
  Textfeld/die Prop-Gadgets und Amiga-Kürzel. Die frühere input.device-
  Variante von reqclose legte im Testemulator den input-Task lahm – das
  war auf der **RTG**-Workbench (tedstub/README.md „reqclose“, letzter
  Punkt); auf dem nativen Screen neu prüfen: **Gate I** = kleiner
  `teddrivetest` (eigener Screen, Zeiger setzen, Klick, Kürzel, Ergebnis
  „expected“/„MISMATCH“ wie `reqclosetest`), Weg I nur, wenn das Gate
  grün ist.

### 4.3 Requester

| Requester | Erkennung | Antwort |
|---|---|---|
| TEDs eigene (`openreq`, src/reqs.c: Fenster `reqwindow` **ohne Titel** auf TEDs Screen): erase, error, find, repall, goto, defaults, about | neues Fenster auf TEDs Screen; Kennung = Liste der Gadget-Texte und -IDs (IntuiText), z. B. erasereq, errorreq aus src/data.c | `gad ID` (GADGETUP auf ENDGADGET → `handled0` schließt), `key $44`/`$45` |
| blitz2-Requester über comdata `openreq` (Versionsrequester „OKEE DOKEE“, „COMPILING“, Fehler über `error` comdata+12) | wie oben (laufen durch TEDs `openreq`; Gadgets werden einmalig +6/+12 verschoben, abweichungen-relevant O6) | Versionsrequester: `gad 1` bzw. Return |
| Dateirequester (freqlib-Kern, `ezfreq`) | Fenstertitel = Titeltext: „Name of file to load“, „Name to save file as“ (src/core/state.c:72/73), „Name of Executable to create“ (blitz2) | `freq ok` = GADGETUP auf „OK“ (wie `reqclose ok TITEL`; Pfad/Name sind vorbelegt mit `filepath`/`filename` der geladenen Datei); `freq ok NAME` schreibt vorher den Namen in den Puffer des Namens-Stringgadgets |
| unerwartet | jedes andere neue Fenster | Log mit allen Gadgets (wie `rc_dumpgadgets`), Test = FAIL, Rettungsfolge §4.5 |

**String-/Integer-Gadgets** (Such-/Ersetztext `findreqsg1/2`, Zeile im
goto-Requester, Tabstop/Farben im defaults-Requester): Tippen über Weg D
erreicht sie nicht (die Bearbeitung macht Intuition). `str ID "text"`
schreibt unter Forbid + LockIBase in `StringInfo->Buffer`, setzt
`NumChars`, `BufferPos = DispPos = 0` und bei `GACT_LONGINT` auch
`LongInt` (TED liest z. B. `int3 = defaultreqsg3.LongInt`, src/data.h:79),
dann `RefreshGList`; danach GADGETUP auf das Stringgadget oder auf den
Knopf. Gleich für A und B, also für den Vergleich neutral.

### 4.4 Synchronisation: „TED wartet“

Jeder Schritt wartet, bis TED die vorige Eingabe verarbeitet hat – nie
feste Pausen. Das Reply (`ReplyMsg`) genügt **nicht**: `handled0` antwortet
vor dem Ausführen der Routine. Kriterium `idle` (unter `Forbid` gelesen):

* Task des Fensters (`UserPort->mp_SigTask`) in `TS_WAIT`,
* `tc_SigWait` enthält `1 << UserPort->mp_SigBit` (TED steht in
  `WaitPort` von `getinput`, nicht in `Delay` von `refme`/`actgad` – dort
  wartet er auf das DOS-Signal),
* Port leer, keine eigene Nachricht unbeantwortet,
* das dreimal in Folge im 40-ms-Takt (`Delay(2)` wie reqclose).

Busy-Polls (`mousedown`-Ziehen, Kompilieren mit STOP-Abfrage über
`getinput2`) sind nie `idle`; dort wartet das Skript auf das Ergebnis
(neues Fenster, Fenster weg, Prozess beendet).

### 4.5 Skriptsprache (Vorschlag)

```
; Kommentar
timeout 15                 Zeitgrenze für jedes folgende wait (s)
wait screen                TEDs Screen offen (Titel "Blitz Basic 2"/"File: ") und idle
wait req LABEL [LABEL…]    neues Fenster auf TEDs Screen mit diesen Gadget-Texten, TED idle
wait freq TITLE            Dateirequester mit diesem Titel
wait gone                  oberstes Requester-Fenster geschlossen, TED idle
wait idle
wait exit                  blitz2 beendet (SystemTags kehrt zurück)
key CODE [QUAL]            RAWKEY (hex), danach idle
type "TEXT"                MapANSI → RAWKEY-Folge, idle nach jeder Taste; \n = Return
menu M I                   MENUPICK (Menü M, Punkt I; COMPILER = Menü 4)
gad ID                     GADGETUP im obersten TED-Fenster
str ID "TEXT"              Stringgadget füllen (§4.3)
freq ok|cancel [NAME]
pointer X Y | click X Y    nur Weg I (Gate I)
dump NAME                  Bildschirm nach out/NAME.scr (§6.1)
state NAME                 interner Zustand nach out/NAME.st (§6.2, Stufe 2)
mem NAME                   AvailMem(MEMF_ANY/CHIP) ins Log
log "TEXT"
```

Jede Zeile wird **vor** der Ausführung ins Log geschrieben (`step N …`),
das Ergebnis danach (`ok`, `timeout`, gesehene Fenster mit Gadgets). Läuft
ein `wait` in die Zeitgrenze: Rettungsfolge – Esc in jedes TED-Fenster,
`freq cancel`, Menü QUIT (`menu 0 9`), auf den erase-Requester Gadget 1
(`erase2` beendet nur bei `d6 == 1`, src/reqs.c) – dann `RESULT FAIL schritt grund`; blockiert auch das, beendet der
Emulator-Watchdog die Sitzung (§5).

Ergebnis je Lauf: `out/RUN.result` mit genau einer Zeile `OK`,
`FAIL schritt grund` – geschrieben von teddrive; fehlt die Datei nach der
Sitzung, setzt der Runner `TIMEOUT` (bzw. `CRASH`, wenn der Lauf begonnen,
aber nicht beendet wurde und die Sitzung früh endete).

## 5 Runner `tedab.py` und Sitzungen

Ort `ted-c/tools/tedab.py` (importiert `compilerlauf/tools/bb2compile.py`
für `prepare`-Teile, `LOCK`, `AB3C`, `amiga_path`, und `corpus_check.mask_trap1`).

```
python3 -I tools/tedab.py [--combos R5,C5,Rc,Cc] [--test GLOB] [--timeout 90] [--keep]
  -> results/ted-ab/COMBO/TEST/…   (Dateien, Dumps, Logs, .result)
  -> results/ted-ab/report.txt     (Matrix Test × Vergleich: SAME/DIFF/KNOWN/FAIL/TIMEOUT)
```

* **Lauf** = (Test, Kombination). Skriptteil je Lauf:
  ```
  Echo "RUN t03-join C5 start" >>AB3C:…/out/session.log
  Assign blitz2: AB3C:…/C5/blitz2
  Assign L: AB3C:…/C5/L
  Assign L: SYS:L ADD
  Copy AB3C:…/orig/x.bb2#? AB3C:…/C5/work/ QUIET
  SetEnv TEDDRIVE AB3C:…/tests/t03-join.drv
  SetEnv TEDDRIVELOG AB3C:…/C5/out/t03-join.log
  Stack 40000
  AB3C:…/teddrive blitz2:blitz2 AB3C:…/C5/work/x.bb2 >AB3C:…/C5/out/t03-join.stdout
  Echo "rc=$RC" >>AB3C:…/C5/out/t03-join.stdout
  Assign L: SYS:L
  Assign blitz2:
  Echo "RUN t03-join C5 end" >>AB3C:…/out/session.log
  ```
* **Sitzung** = mehrere Läufe hintereinander in **einem** Emulatorstart
  (Boot ≈ 15 s, `tools/amiberry.cmake`), `--timeout 90` (wie
  `bb2compile.BATCH_TIMEOUT`). Der Runner packt nach gemessener Laufzeit
  (Startwert: 10 s je Lauf) höchstens so viele Läufe, dass ≤ 60 s erwartet
  werden; die Kombinationen eines Tests liegen in derselben Sitzung
  (gleicher Systemzustand für A und B).
* **Hänger**: Watchdog = Zeitlimit von `tools/amiberry.cmake` (beendet Amiberry,
  Teilergebnisse bleiben in `AB3C:`). Der Runner liest `session.log`: der
  Lauf mit `start` ohne `end` wird `TIMEOUT`, die übrigen werden in einer
  neuen Sitzung wiederholt (Muster `bb2corpus.run_session`). Nach zwei
  TIMEOUTs desselben Tests in derselben Kombination kein dritter Versuch.
* Läufe ändern das System nicht dauerhaft: `L:` wird zurückgesetzt, alle
  Dateien liegen auf `AB3C:`; ein abgestürzter TED hinterlässt höchstens
  Speicher (nächster Lauf = neue Sitzung, frischer Boot).

## 6 Was verglichen wird

| Gegenstand | Quelle | Vergleich | Maske/Hinweis |
|---|---|---|---|
| gespeicherte Quelle | `work/NAME.bb2` nach SAVE | A↔B **bytegleich**; zusätzlich gegen Orakel (Rundreise: = Original; getippter Text: = `bb2tok.tokenise(erwarteter Text)`) | – |
| `.bak` | `work/NAME.bb2.bak` (filesave: Delete + Rename, src/main.c `filesave`) | Existenz und Inhalt = alte Datei | – |
| `.xtra` | `savextra`-Rückruf beim Speichern | A↔B bytegleich; R5↔Rc zeigt, ob blitz2-c dieselben Optionen schreibt | – |
| `.info` | `srcinfo` (396 Bytes, reference.md §3) bei `makeicons` | bytegleich (Icon-Position wird von TED nicht gesetzt) | – |
| Executable | COMPILER → CREATE EXECUTABLE | A↔B bytegleich; zusätzlich = Executable von tedstub für dieselbe Quelle und denselben Compiler | mit „Runtime Errors“: `mask_trap1` (Adresse der Editorzeile hinter `trap #1`/`#4` hängt vom Speicher ab); bevorzugt `.xtra` mit abgeschalteter Option (wie `bb2compile --no-runerr`, Bytes 2–5) |
| Fehlerzeile → Cursor | `error` (comdata+12) → errorreq, dann `menurout` → d0 → `gotolined0` (src/edit.c) | Requester-Kennung gleich; danach Statuszeile „Line:“/„Column:“ und Cursor im Dump gleich und = erwartete Zeile | – |
| Bildschirm | `dump` (§6.1) nach festgelegten Schritten | Bitplanes bytegleich | Feld „Free“ (Statuszeile, `printmem`: `statsloc+38`, 5 Zeichen) |
| Requester-Folge | teddrive-Log: jedes neue Fenster mit Titel und Gadget-Texten | Folge gleich | Fensterzeiger/Adressen nicht vergleichen |
| Optionsdatei | `TAG/L/blitzeditor.opts` nach DEFAULTS → OK (`savedefs`, 50 Bytes) | bytegleich, = erwartetes Abbild aus `defstart` mit Änderung | – |
| Screen-Titel | `Screen->Title` im Dump-Kopf (`newsctit`: „File: NAME“, Hilfetext nach Help) | gleich | – |
| Speicherbilanz | `mem` vor dem Start und nach `wait exit` | Differenz A↔B gleich (Lecks wie `join`-FreeMem ohne `lenitem` treten in beiden auf) | absolute Werte nicht (Programmgrößen verschieden); Stufe 2 |
| Rückgabe von blitz2 | `rc=` in `.stdout` | gleich | – |

### 6.1 Bildschirm-Dump

**Mechanismus**: teddrive wartet auf `idle`, dann unter `Forbid` (TED
zeichnet nur im eigenen Task, der wartet; der Blitter ist nach
`WaitBlit`/`DisownBlitter` frei): TEDs Screen suchen (Fenster des
blitz2-Prozesses, Titel), `BitMap = Screen->RastPort.BitMap`, Datei
`out/NAME.scr` schreiben:

```
Kopf (Text, 1 Zeile je Feld): magic TEDSCR1, Width, Height, Depth,
  BytesPerRow, Rows, ModeID, MouseX, MouseY, Title, Anzahl Fenster auf
  dem Screen (mit Lage/Größe), TypeOfMem(Planes[0])
dann Depth × BytesPerRow × Rows Bytes (Plane 0, Plane 1)
```

(640×512×2: 80 KB je Dump.) Das ist robust, braucht keine Adressen in TED
und ist für beide TEDs gleich. Offene TED-Requester sind Fenster auf
demselben Screen und stehen mit im Bild – erwünscht (Requester-Inhalt
gleich), Lage steht im Kopf.

**Auswertung** (`ted-c/tools/teddump.py`): PNG für Menschen; Text-
Dekodierung des Editorbereichs mit den TED-Zeichensätzen (`chars8.b`/
`chars.b` aus `quelltext/amibl2src/files/amiblitz2source/bb2src.zip.dir/tedt`,
wie `bin2c.py`; Lage: ab Zeile `topskip` = 12, `fonthite` 8 oder 6,
Zeichen `(c&15) + (c>>4)*128 + r*16` bzw. `*96`, reference.md §3) → Text
je Zeile + Attribut (invers = Block/Cursor, Plane 1), damit ein DIFF als
lesbarer Zeilenunterschied erscheint; Statuszeile (`statsloc`, Texte
„Line:“, „Column:“, „Block“) dekodiert, „Free“ maskiert. Vergleich:
Planes bytegleich nach Maske, sonst Textdiff + Differenzbild.

**chmap** (TEDs Zeichenkarte, `chmap`, chcols × edrows, Chip-RAM) ist
damit nicht nötig: TED führt sie parallel zu den Bitplanes (gleiche
Zeichen; DIFF „chmap-Blits“: in TED-C mit der CPU statt dem Blitter), die
Dekodierung liefert für den sichtbaren Bereich dieselbe Information. Wer sie direkt will: §6.2.

### 6.2 Interner Zustand (Stufe 2, optional)

Für Fehlersuche: `state NAME` liest TEDs Variablen (`chmap`-Inhalt,
`cursx`/`cursy`/`cursoff`/`curstop`, `numlines`, `btopx…bboty`,
`allalt`, `tabstop`, `tokenise`) und die Zeilenliste ab `firstitem`
(`struct item`: Länge + Bytes – das In-Memory-Abbild der Quelle, ohne
Speichern vergleichbar).

TED veröffentlicht keine Adressen (`comdata` kennt nur blitz2). Einstieg
über den **VBlank-Server `myint`** (ted.s:306, src/main.c:22;
`AddIntServer(INTB_VERTB, &myint)`): teddrive durchläuft unter `Disable`
die Serverliste `SysBase->IntVects[INTB_VERTB].iv_Data`, nimmt Knoten mit
`ln_Type = NT_INTERRUPT`, `ln_Name = NULL`, `is_Data = NULL`, berechnet
die Hunkbasis `= &myint − OFF(myint)` und prüft sie mit einer Kennung im
selben Hunk (`deftitname` „Blitz Basic 2 - Ted Version 1.1“ bzw. ein
anderes konstantes Datum, das im selben Hunk liegt). Die Offsets erzeugt
der Runner je TED als Beschreibung `TED.desc`:

* Referenz: `build/amiga/ref/ted.labels` (ein Hunk; z. B. `myint` `$632`,
  `chmap` `$5E5A`, `cursx` `$5E3E`, `curstop` `$5E54`, `firstitem`
  `$5E60`, `numlines` `$5E6C`, `defstart` `$5E8C`),
* TED-C: `build/amiga/ted.map` / `ted.sym` (`-fbaserel`: Variablen und
  `myint` im Daten-Hunk; Offsets relativ zu dessen Anfang).

Feldbreiten nach `structs.h`/`state.h` (Amiga-Layout ist per
`_Static_assert` an das Original gebunden). Stufe 1 braucht das nicht.

## 7 Testliste

Quellen: `T` = `compilerlauf/corpus/tests/NAME.bas` (vom Runner mit
`bb2tok` tokenisiert), `S` = `compilerlauf/tedstub/tests/syntaxerr.bb2`
(Fehler in Zeile 2, `b=(a+`), `K` = CD-Quelle aus `cd/`. Jeder Test
beginnt mit `wait req "OKEE DOKEE"` → `gad 1` (Versionsrequester von
blitz2) und `wait screen`, endet mit QUIT (`menu 0 9`) und ggf. erase-
Requester, `wait exit`; dazwischen `dump` an den genannten Stellen.

Menünummern aus `menutable` (src/input.c): PROJECT 0 (NEW 0, LOAD 1, SAVE 2,
DEFAULTS 3, ABOUT 4, PRINT 5, CLI 6, CLOSE WB 7, OPEN WB 8, QUIT 9), EDIT 1
(COPY 0, KILL 1, BLOCK SAVE 2, BLOCK LOAD 3, FORGET 4, INSERT LINE 5,
DELETE LINE 6, JOIN 8, BLOCK TAB 9, UNTAB 10, MARK INDENT 11), SOURCE 2
(TOP 0, BOTTOM 1, GOTO 2), SEARCH 3 (FIND 0, NEXT 1, PREVIOUS 2,
REPLACE 3, REPLACE ALL 4), COMPILER 4 (blitz2; CREATE EXECUTABLE = Punkt 2
wie im tedstub). Tasten: F1/F2 Blockanfang/-ende (`$50`/`$51` → 129/130),
F3 Block speichern, F4 Block kopieren, Help `$5F`, Cursor `$4C–$4F`.

### 7.1 Stufe 1 (R5 ↔ C5, nur Weg D)

| Nr | Test | Quelle | Schritte | Verglichen |
|---|---|---|---|---|
| 00 | Start/Ende | T `t01_int` | laden, `dump start`, QUIT | Dump, Requester-Folge, rc; **Rauchtest** – erst wenn er in C5 läuft, lohnt der Rest |
| 01 | Rundreise | T `t01_int`, T `t08_tokquirk`, K 2–3 kleine tokenisierte Quellen (z. B. `bui5/makefile/includes/finish_mouse.bb`, `bui6/mapslib/mikesmaplib.bb`) | laden, SAVE (`freq ok`), QUIT | `.bb2` = Original, `.bak`, `.xtra`, `.info`, Dump |
| 02 | Tippen + Speichern | T `t01_int` | BOTTOM, `type` mehrere Zeilen (Schlüsselwörter, Strings, `;`-Kommentar, `$`-Hex, Tab), SAVE | `.bb2` A↔B und = `bb2tok.tokenise` des erwarteten Textes; Dumps nach dem Tippen |
| 03 | Join/Split | T `t02_float` | Cursor auf Zeile n, Spalte k, Return (Split); JOIN (`menu 1 8`); SAVE | Datei, Dumps, Speicherbilanz (Join-Leck BUG(orig), gleich in A und B) |
| 04 | Block kopieren/löschen | T `t04_arr` | F1/F2 über 3 Zeilen, Cursor weiter, COPY (`menu 1 0`) bzw. F4, KILL (`menu 1 1`), FORGET; SAVE | Datei, Dumps (Blockmarkierung invers) |
| 05 | Suchen/Ersetzen | T `t03_str` | FIND (`menu 3 0`), `str` Such-/Ersetztext, NEXT; REPLACE; REPLACE ALL (repall-Requester), SAVE | Datei, Dumps, Requester-Folge |
| 06 | Kompilieren über Menü | T `t01_int` (`.xtra` ohne Runtime Errors) | `menu 4 2`, `freq ok` („Name of Executable to create“), warten auf Ende von „COMPILING“, QUIT | Executable A↔B und = tedstub-Ergebnis (corpus `results/blitz2-195-…`), Requester-Folge |
| 07 | Fehlerzeile | S | `menu 4 2`, `freq ok`, `wait req` errorreq → `gad 1`, `dump` | Cursor/Statuszeile = Zeile 2, Dump A↔B |
| 08 | loadxtra/savextra | K Quelle mit eigener `.xtra` | laden (loadxtra), SAVE unter neuem Namen (`freq ok NAME2`), QUIT; zweiter Lauf lädt NAME2 und speichert wieder | `NAME2.xtra` = Original-`.xtra`; zweite Rundreise unverändert |
| 09 | Optionen | – (neue Datei) | DEFAULTS (`menu 0 3`), `str` Tabstop 4, OK; QUIT; zweiter Lauf mit dieser `L/blitzeditor.opts` | Optionsdatei A↔B, Dump im zweiten Lauf (Tab-Breite) |

### 7.2 Stufe 2 (alle vier Kombinationen, breiter)

* Rundreise über eine größere Auswahl der 724 CD-Quellen (Sitzungen voll
  packen; die Host-Rundreise `test_loadsave` ist schon grün – hier zählt
  der echte Ein-/Ausgabeweg mit `dos.library`, `.bak`, `.info`).
* Textdateien (9 Klartext-`.bb2` der CD) mit `tokenise` an: Ergebnis =
  `bb2tok tok` unter Beachtung von `readline` (letzte Zeile ohne LF bleibt
  untokenisiert, BUG(orig), fileio.c:215).
* Zeichensatz 6 Pixel (`fonthite` über DEFAULTS), Fensteraufteilung,
  Rollen über Fensterende (`scrollup`/`scrolldown`, Blitter), TOP/BOTTOM/GOTO.
* Block in Datei / aus Datei (BLOCK SAVE/LOAD, Dateirequester mit
  `blokpath`/`blokname`), BLOCK TAB/UNTAB, MARK INDENT.
* NEW mit Rückfrage (erase-Requester ja/nein), LOAD einer zweiten Datei.
* Help-Taste auf einem Token (Titelzeile = Hilfetext, im Dump-Kopf); nicht
  mit rechter Amiga-Taste (startet `blitz2:help`).
* Lange Zeilen (127/128 Zeichen, Umbruch in `readline`), Umlaute (`germs`,
  Bit 30), Latin-1 `$FD–$FF` (cmp.b-Dispatch, overview.md §4 Risiko 6).
* Kompilieren aller `corpus/tests/*.bas` über das Menü (Executables gegen
  `compilerlauf/results/…`), Fehlerzeilen aus `reference/195.tsv`
  (Quellen mit `error`, Zeile → Cursor).
* `state`-Vergleich (§6.2) und Speicherbilanz für alle Stufe-1-Tests.
* Speichermangel: `outmem` während des Editierens (longjmp auf `memcont`) –
  nur mit einem Speicherfresser im Begleitprozess, eigene Sitzung.

### 7.3 Stufe 3 (Weg I, nach Gate I)

Mausklick setzt Cursor, Ziehen markiert Block (auch nach oben – im Original
„currently FUCKED“, src/mouse.c `mousedown`), Klick in die Labelspalte
(`gomouse`), Prop-Gadgets (`gadget`, busy poll), Amiga-Kürzel (Amiga-S,
Amiga-L, Amiga-?), CLOSE WB/OPEN WB.

## 8 Fälle mit besonderer Erwartung

Grundregel: `BUG(orig)` ist in C nachgebildet → **A und B gleich** (auch
gleich falsch). Abweichungen nur bei den `DIFF:`-Zeilen von
[abweichungen.md](abweichungen.md) mit „beobachtbar: ja/nur …“.

| Fall | Quelle | Erwartung im Test |
|---|---|---|
| `tabstop = 0` (DEFAULTS akzeptiert 0, BUG(orig) src/main.c:748) → `divu` durch 0 in `readline` | PHASE2B-BRIEF O3; seit Phase 2c echtes `divu.w` (src/glue.c `divu_w`) | beide: Trap (Guru 5, Division by zero) → Lauf CRASH/TIMEOUT. **KNOWN-BOTH**, eigener Lauf am Ende einer Sitzung |
| beschädigte Optionsdatei, Farbwert ≥ 4 (`swapd0bord`) | abweichungen.md „`swapd0bord`“ | beide tauschen Bytes mit den Original-Schrittweiten hinter `defaultreqbo14`; die Daten dort sind in C andere Variablen → Folgen können abweichen. **KNOWN-DIFF**, nur Stufe 2 |
| CLI (`mycli` → `openio`) | seit 2c: Z-Flag von `WBenchToFront` über `callregs` (src/mouse.c) | „gleich“ |
| `outmem` vor der ersten Hauptschleife (`memstack == 0`) | seit 2c: `memcont_nostack` (src/glue.c) setzt a7 = 0 und läuft memcont wie das Original | beide stürzen ab (Absturzstelle kann sich unterscheiden: die gerufenen C-Routinen benutzen den Stapel anders) – nur Stufe 2, KNOWN-BOTH |
| `join`: FreeMem ohne `lenitem` (9 Bytes je Join), `freethis`/`freeall` per `add.b` ab `numchars` ≥ 247 | lines.c:407, tokit.c:303 | gleiche Speicherbilanz A↔B (Leck in beiden) |
| `join` ohne Längenprüfung, `split` hängt Präfixlänge an (I.3/I.4) | lines.c:382, lines.c:1036 | nur mit kurzen Zeilen in Stufe 1; lange Zeilen in Stufe 2 mit Erwartung „gleich“ (Speicherüberschreiben kann beide gleich treffen – bei CRASH in beiden: KNOWN-BOTH) |
| `gotolined0` ohne Bereichsprüfung (GOTO/Fehlerzeile ≥ `numlines`, I.13) | lines.c:602, edit.c:110 | nicht in Stufe 1; in Stufe 2 Erwartung „beide gleich“ (NULL-Lauf) – KNOWN-BOTH |
| ASCII-Block bekommt ein 0-Byte je Zeile (`tokenise` aus) | loadsave.c:52 | BLOCK SAVE ohne Tokenisieren: Datei enthält `text\0\n` in A und B |
| UNTAB löscht beliebige Zeichen | loadsave.c:102 | Datei gleich in A und B, ≠ „vernünftigem“ Ergebnis |
| `readline`: letzte Zeile ohne LF untokenisiert, Byte ≥ `$80` = Token auch im Text | fileio.c:215 | Orakel `bb2tok` mit derselben Ausnahme (wie `test_loadsave`) |
| Block endet bei Umlaut (`getbchar` N-Flag) | fileio.c:715 | gleich in A und B |
| leere Datei laden: alte Zeilen verloren (kein `freeall`) | loadsave.c:237 | Speicherbilanz gleich |
| Speicherfehler beim Schreiben: `allalt` bleibt, `.xtra`/`.info` trotzdem | loadsave.c:317 | nur mit vollem Datenträger, Stufe 2 optional |
| Executable mit „Runtime Errors“ | tedstub/README.md „Bekannt“ | Adressen hinter `trap #1`/`#4` verschieden → maskieren; leerer „OKAY!“-Requester beim Ausführen (nur relevant, wenn Programme gestartet werden – hier nicht) |
| Dateirequester ab 605 Zeilen | reference.md §3.2 | durch 640×512 ausgeschlossen; Screens > 604 Zeilen nie testen |
| Sanduhr-Animation (`myroutine`, VBlank) | src/main.c | Sprite, nicht in den Bitplanes – kein Einfluss auf Dumps |

Der Bericht unterscheidet: `SAME`, `DIFF`, `KNOWN-DIFF` (Zeile in
abweichungen.md), `KNOWN-BOTH` (beide gleich fehlerhaft/abgestürzt),
`FAIL`/`TIMEOUT` (Lauf nicht vollständig). Erwartungen stehen je Test in
`tests/emu/NAME.expect` neben dem Skript `tests/emu/NAME.drv`.

## 9 Voraussetzungen und Blocker

| # | Punkt | Art | Was zu tun ist |
|---|---|---|---|
| B1 | ~~RTG-Workbench der Testkonfiguration~~ | **gelöst** (27.10.) | `A1200-AB3-AGA.uae` des Benutzers (§3.2), nativer PAL-Hires-Laced-Workbench, Tiefe 8; Gate 0 prüft es in jeder Sitzung (§3.3) |
| B2 | Begleitprozess `teddrive` | umgesetzt (Weg D) | `tests/amiga/teddrive.c`, Selbsttest `teddrivetest` (Gate D); `reqclose.h` unverändert eingebunden (O-P1) |
| B3 | Runner `tedab.py`, Decoder `teddump.py`, Testskripte | umgesetzt (Gate, Test 00) | §5, §6.1; Host-Tests `tests/tools/` (ctest host `tedab_tools`, `tdscript`); weitere `tests/emu/*.drv` nach Test 00 |
| B4 | Mausposition/Kürzel nur über input.device, das im Testemulator den input-Task lahmlegte (RTG) | Risiko für Stufe 3 | Gate I auf dem nativen Screen; Stufe 1/2 kommen ohne aus |
| B5 | Synchronisation „idle“ nicht erprobt (TS_WAIT/SigWait) | Risiko | erst mit Test 00 an R5 erproben (Referenz muss sicher laufen), Log jeder Wartezeit |
| B6 | TED-C ist noch nie gestartet | Risiko | Test 00 zuerst mit R5 (Werkzeug prüfen), dann C5; bei Absturz: Enforcer/MuForce im Testabbild wäre hilfreich (Treffer ins Log), sonst Sitzungs-TIMEOUT und Logschritt als Anhaltspunkt |
| B7 | `help`, `help.dat` werden von `bb2compile.prepare()` nicht kopiert | erledigt | `tedab.py` kopiert sie in jede Kombination |
| B8 | `SYS:L/blitzeditor.opts` auf der Hardfile unbekannt | klein | Gate 0 meldet sie; `Assign L:`-Reihenfolge (§2.2) |
| B9 | ~~O1/O3~~: in Phase 2c erledigt (beide wie das Original: Absturz bzw. Trap, §8) | – | – |
| B10 | ~~README~~: in Phase 2c nachgezogen | – | – |
| B11 | Laufzeit: Boot ≈ 15 s, TED-Start und Requester-Takt unbekannt | Planung | nach Test 00 messen, Sitzungsgröße in `tedab.py` daraus |
| B12 | CI: Hardfile/ROM lizenzpflichtig, Emulatorlauf nur lokal/self-hosted (ci-exploration.md §2.2/2.3) | Rahmen | Phase 3 lokal; CI später mit derselben Konfiguration |

## 10 Ablauf (Arbeitspakete)

1. **Konfiguration** (B1, erledigt: `A1200-AB3-AGA.uae` des Benutzers),
   `tedprobe` → Gate 0 grün (nativer Modus, Planes in Chip-RAM, Höhe
   unter 605).
2. **teddrive** Weg D + `dump` + Log + `.result`; Entscheidung **O-P1**
   (reqclose.h teilen oder kopieren). Selbsttest `teddrivetest` (eigenes
   Fenster mit Gadgets/Stringgadget, prüft RAWKEY/MENUPICK/GADGETUP/`str`/
   `idle`) – Gate D, analog `reqclosetest`.
3. **tedab.py** (Staging, Sitzungen, Wiederholung nach Hänger, Bericht) und
   **teddump.py**.
4. Test 00 mit **R5** (Werkzeugprobe am Original), dann **C5**; erst danach
   01–09 (Stufe 1).
5. Rc/Cc dazu (Stufe 2a), dann die breiteren Tests (Stufe 2b), `state`
   (§6.2).
6. Gate I (input.device auf nativem Screen) → Stufe 3.
7. Ergebnis in `notes/abweichungen.md` (neue beobachtete DIFFs, Spalte
   „beobachtbar?“ mit Testnummer), Stand in `README.md`.

Entschiedene Punkte (Koordinator, 27.10.2026):

* **O-P1**: `reqclose.h` wird **nicht** kopiert und nicht aufgeteilt:
  `teddrive.c` bindet die Datei unverändert aus
  `compilerlauf/tedstub/src` ein (eine Quelle wie `freqlib_req.inc`) und
  benutzt Logpuffer (`rc_cat`/`rc_hex`/`rc_dec`), `rc_valid`, `rc_label`,
  `rc_copytitle` und die statischen Nachrichtenpuffer samt Reply-Port
  (`rc_msg`/`rc_busy`/`rc_port`/`rc_replies`); der Rest (eigene Schleife
  von reqclose) bleibt ungenutzt (`-Wno-unused-function`). Senden mit
  Qualifier (`td_send`) ist eine Abwandlung von `rc_send` in teddrive.c.
  `reqclose` selbst ist unberührt (bytegleich).
* **O-P2**: maßgeblich ist der tatsächliche Workbench-Modus (erwartet
  640×512 laced, Tiefe 8); Gate 0 hält ihn fest, Dumps werden nur bei
  gleicher Geometrie verglichen (`teddump.compare`).
* **O-P3**: Ergebnisse in `ted-c/results/ted-ab/` wie `compilerlauf/results/`
  – nur Text (Logs, `.result`, dekodierte Dumps, Prüfsummen, `report.txt`);
  Binärdumps und gespeicherte Dateien unter `ted-c/build/ted-ab/` (nicht
  in git).

Umsetzung und Abweichungen vom Plan: [PHASE3-LOG.md](PHASE3-LOG.md).
