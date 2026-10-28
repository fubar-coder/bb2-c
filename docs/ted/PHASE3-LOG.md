# Phase 3 – Arbeitsprotokoll (Emulator-A/B-Tests)

Laufendes Protokoll zur Umsetzung von [PHASE3-PLAN.md](PHASE3-PLAN.md).
Regeln für den Emulator (Koordinator): ein Emulator gleichzeitig
(`bb2compile.LOCK`), eine Sitzung für viele Tests (60 s, höchstens 90 s,
sonst aufteilen), nichts darf blockieren (jede Wartestelle mit Zeitgrenze,
Eingaben synthetisch), jeder Treiber mit eigenem Selbsttest-Gate vorneweg,
vor jedem Lauf alle Host-Prüfungen, **kein Emulatorstart ohne Freigabe
des Koordinators**, neue Treiber zuerst nur mit den Gates.

## 27.10.2026 – Werkzeuge (nur Host, kein Emulatorlauf)

Ausgangslage geprüft: `ctest --preset amiga` 5/5, `ctest --preset host`
11/11 grün.

### Gate 0: `tedprobe` (`tests/amiga/tedprobe.c`)

`tedprobe AUSGABE` schreibt „key value“-Zeilen: Systemversionen,
`GetScreenData(84 Bytes, WBENCHSCREEN)` wie TEDs `calcscreen` und die
daraus folgende TED-Geometrie (`ted_edrows_f8/f6`, `ted_height_f8/f6`),
Workbench-Screen (`GetVPModeID`, Modusname, `BMA_DEPTH`, `BMA_FLAGS`,
`BytesPerRow`, `TypeOfMem(Planes[0])`, interleaved), freier Speicher,
`SYS:L/blitzeditor.opts` und `L:blitzeditor.opts`, Stack, Tastaturbelegung
und `MapANSI` einer Probe. Letzte Zeile `gate0 OK` (rc 0) oder
`gate0 FAIL` (rc 5, WARN): Planes nicht im Chip-RAM, Tiefe < 2, TEDs
Screen höher als 604 Zeilen, Workbench schmaler als 640.

### `teddrive` (Weg D) und Gate D

* `tests/amiga/teddrive.c`: `teddrive SKRIPT PRÄFIX BEFEHL [ARGS]`, Ablauf
  und Skriptsprache im Kopfkommentar. Hauptprozess startet den Befehl mit
  `SystemTags`, ein zweiter Prozess spielt das Skript. Programm-Screen =
  erster neuer Screen mit Backdrop-Fenster; Ziel jeder Eingabe = jüngstes
  offenes Requester-Fenster, sonst das Hauptfenster (TEDs `pickwindow`).
  „idle“ = Task des Ziel-Ports in `TS_WAIT` mit dem Port-Signal in
  `tc_SigWait`, Port leer, alle eigenen Nachrichten beantwortet, dreimal
  im 20-ms-Takt. Jede Wartestelle hat eine Zeitgrenze (`timeout`,
  Standard 15 s); bei einem Fehler sofort `FAIL schritt grund` in die
  Ergebnisdatei, dann die Rettungsfolge (höchstens 8 Runden/15 s:
  Hauptfenster → Menü QUIT, Dateirequester → Cancel/CLOSEWINDOW,
  TED-Requester → Gadget 1 als ENDGADGET, sonst Esc).
* **O-P1** (Entscheidung Koordinator): `reqclose.h` unverändert
  eingebunden (Include-Pfad `compilerlauf/tedstub/src`, CMake-Cache
  `TEDSTUB_SRC`), keine Kopie, keine Aufteilung. `reqclose` ist unberührt.
* `tests/amiga/tdscript.h`: die reine Skriptsyntax (Wörter, Strings mit
  `\n \t \xHH`, Zahlen, Label-Vergleich) ohne AmigaOS-Header – auf dem
  Host übersetzt (`tests/tools/tdscript_host.c`, ctest `tdscript`) und
  gegen `tedab.split_line` geprüft (gleiche Wörter für jede Zeile jeder
  `.drv`-Datei).
* `tests/amiga/teddrivetest.c` (Gate D): liest Eingaben wie TED (eigener
  Screen mit `SA_LikeWorkbench` + Tiefe 2, Backdrop-Hauptfenster mit Menü,
  Requester als titellose Fenster, `Wait` nur auf den Port des jüngsten
  Fensters, Antwort vor der Verarbeitung, Tasten über `RawKeyConvert`).
  Zwei Skripte: `tests/emu/gate-d.drv` (RAWKEY, `type` mit MapANSI,
  MENUPICK, `wait req`, `str` Text und LONGINT, GADGETUP, `wait gone`,
  Busy-Schritt 1 s – in der Zeit darf keine Eingabe kommen –, eine
  Wartestelle, die ablaufen muss (`expectfail`), zwei Dumps mit einem
  Rechteck in Farbe 3, QUIT) und `tests/emu/gate-d-rescue.drv` (scheitert
  absichtlich in Schritt 6, die Rettung muss das Programm beenden).
  Selbst begrenzt durch einen 20-s-Timer.

### `tedab.py`, `teddump.py`

* `tools/teddump.py`: Dumpformat lesen/schreiben, TED-Geometrie aus der
  Screenhöhe, Text-Dekodierung des Editorbereichs (Zeichen + Attribut:
  Farbe 1–3, invers a–c) und der Statuszeile (nur Ebene 1, `qprint`),
  Maske „Free“ (`statsloc+38`, 5 Zeichen), Vergleich (SAME/DIFF mit
  Textdiff), PNG (nur Standardbibliothek).
* `tools/tedab.py`: `lint`, `plan`, `gate`, `run`, `report`. **Ohne `--go`
  startet nichts** (nur Staging + Skript ausgeben). Mit `--go` zuerst die
  Host-Prüfungen (Build und ctest beider Presets, Lint), dann **genau eine**
  Sitzung unter `bb2compile.LOCK`, `--timeout` 60 (über 90 verweigert),
  `AMIBERRY_CONFIG` = `A1200-AB3-AGA.uae`. Sitzungsskript: Assign `TEDAB:`
  (kurze Zeilen, Shell ≤ 255 Zeichen), **Reboot-Sperre** (`TEDAB:booted`:
  ein Absturz mit Neustart würde das Skript sonst ein zweites Mal
  ausführen), Gate 0 → Gate D (main, rescue) → Läufe; jedes Gate mit
  `If WARN`/`If VAL $tdg GT 0` vor dem Rest.
* Host-Tests `tests/tools/test_teddump.py`, `test_tedab.py` mit
  künstlichen Daten (Dumps mit echten TED-Zeichensätzen, Gate-Ergebnisse,
  Sitzungslog, A/B-Vergleich, Staging in ein Temp-Verzeichnis); ctest
  host `tedab_tools`. Stand: `ctest --preset host` 13/13, `ctest --preset
  amiga` 5/5 grün; `teddrive`, `teddrivetest`, `tedprobe` ohne Warnungen.

### Beim Host-Test gefundene und behobene Fehler der neuen Werkzeuge

* teddrive: Skriptschleife brach an der ersten Leerzeile ab.
* teddrive: `\x` ohne zwei Hex-Ziffern übersprang das schließende `"`
  (Selbsttest `tdscript`); jetzt C und Python gleich: nur `\xHH` mit zwei
  Hex-Ziffern.
* teddrive: Reply-Port wurde auch bei unbeantworteter Nachricht gelöscht –
  jetzt bleibt er dann bestehen (Log-Zeile).
* tedprobe: `MapANSI`-Pufferlänge ist in Bytes (Autodoc), nicht in Paaren.
* teddump: Leerzeichen-Glyphe wurde als Steuerzeichen dekodiert (26 leere
  Glyphen im Zeichensatz) – druckbare Codes haben jetzt Vorrang.

### Abweichungen vom Plan (§4/§5)

| Plan | umgesetzt | Grund |
|---|---|---|
| Skript/Log über `ENV:TEDDRIVE`/`ENV:TEDDRIVELOG` | Argumente `SKRIPT PRÄFIX` | keine globalen Variablen zwischen Läufen aufzuräumen |
| AutoRequest-Patch in teddrive | `reqtrap teddrive …` (unverändertes `reqtrap`) | erprobter Code ohne Kopie; `REQUESTER:` steht in `.stdout` statt im teddrive-Log |
| `tedprobe` Teil von teddrive | eigenes Programm | Gate 0 ohne Treiber; der Dumpkopf enthält dieselben Screen-Felder |
| idle im 40-ms-Takt | 20 ms (`Delay(1)`) | Tippen: je Taste Antwort + 3 idle-Prüfungen |
| `Copy orig → work` im Emulator | Arbeitskopien vom Host vorbereitet, je Lauf ein eigenes Verzeichnis | weniger Befehle in der Sitzung |
| `joyport0=none`, `chipmem_size=4` | Konfiguration des Benutzers unverändert (`mouse`, 8) | Vorgabe: Konfiguration nicht ändern; Risiko Host-Mausklick (§3.2) |

Offen für Test 00: Label des Versionsrequesters (laut tedstub-Log
`" OKEE DOKEE "`, Vergleich ohne Leerzeichen am Rand), Dauer je Lauf
(Schätzung 12 s).

## Emulator-Läufe

### Lauf 1 (27.10.2026): nur Gates – `tedab.py gate --go`

Freigabe des Koordinators, genau einmal ausgeführt. Emulator rc 0,
Sitzungslog: `SESSION start, GATE0 OK, GATED OK, SESSION end`.
Ergebnisse: `results/ted-ab/` (gate/, session.log, report.txt),
Dumps `build/ted-ab/gd.gd-*.scr`.

* **Gate 0 OK**: Kickstart/Workbench 47.13 (OS 3.2), AGA (`ChipRevBits0`
  $1F), PAL. Workbench 640×512, **Tiefe 8**, Mode-ID $00029004
  „PAL:HighRes Interlace“, Planes im Chip-RAM (`TypeOfMem` $703).
  TED-Geometrie: Screen 640×509, `edcols` 78, `edrows` 60 (Font 8) bzw.
  81 / Höhe 513 (Font 6) – beide unter 605. Chip frei 3,8 MB, Fast 133 MB.
  `SYS:L/blitzeditor.opts` und `L:blitzeditor.opts` gibt es nicht (Tests
  ohne eigene Optionsdatei starten mit `defstart`). Stack 40000 kommt im
  Programm an. Tastaturbelegung `d`; MapANSI braucht für alle Proben eine
  Taste.
* **Gate D main OK** (Ergebnis `OK`, 10/10 Ereignisse „expected“, 0
  Abweichungen, 5,2 s): RAWKEY, `type "Ab1\n"` → RawKeyConvert „Ab1\r“
  (Shift-Qualifier aus MapANSI), MENUPICK, `str` Text/LONGINT, GADGETUP,
  `wait gone`, Busy-Schritt (Rückkehr nach 1,1 s, keine Eingabe
  dazwischen), `expectfail` nach 2 s, Dumps 640×512×2 mit Rechteck.
  Zeiten: Taste ≈ 0,16 s, Menü/Gadget ≈ 0,16 s, `wait screen` 0,22 s.
* **Gate D rescue OK** (Ergebnis `FAIL step 6 timeout`, Rettung: Gadget 1,
  Menü QUIT, „rescue: program ended“, 2,9 s).

Überraschungen / Befunde:

* **Interleaved Bitmaps**: Die Workbench-Bitmap ist interleaved
  (`BytesPerRow` 640 = 80 × 8), und der mit `SA_LikeWorkbench` + Tiefe 2
  geklonte Screen ebenfalls (`bytesperrow` 160, `interleaved 1` im Dump).
  TED rechnet mit `bmapmul = BytesPerRow` und `Planes[0/1]`; das passt zu
  interleaved (Zeilenabstand 160, Blitter-Modulo `BytesPerRow − edcols`),
  ist aber eine Umgebung, die TED 1991 nie gesehen hat. Gilt für R5 und
  C5 gleich (kein A/B-Unterschied), Dumps vergleichen nur gleiche
  Geometrie. Zu beachten bei Bildfehlern in Test 00.
* `cybergraphics.library` ist geladen (OS-3.2-Hardfile), obwohl keine
  RTG-Karte konfiguriert ist; ohne Wirkung auf den nativen Workbench.
* Das Hauptfenster von teddrivetest wurde mit `idcmp $00000000`
  protokolliert, obwohl Nachrichten ankamen: Die Momentaufnahme fiel in
  `OpenWindowTags` (Fenster schon in der Liste, IDCMP noch nicht gesetzt).
  Nur Log, keine Wirkung.
* Auswertungsfehler in `tedab.py` (nicht im Emulator): `eval_gate` suchte
  die Dumps in `results/` statt in `build/ted-ab/` und meldete „gates
  FAILED“; behoben, Bericht neu erzeugt (`gates OK`). Log-Zeile von
  `expectfail` ohne Leerzeichen vor der Zeit – behoben.

Danach (nur Host): teddrive schreibt bei Erfolg zusätzlich `PRÄFIXok`;
`tedab.py` führt die weiteren Kombinationen eines Tests nur aus, wenn die
erste (Referenz) OK war (`If EXISTS …/ok`, sonst `RUN … skipped`); Test 00
mit `timeout 10`. ctest amiga 5/5, host 13/13.

### Lauf 2 (27.10.2026): Test 00 – `tedab.py run --test 00-start --go`

Freigabe des Koordinators, genau einmal. Emulator rc 0, Sitzung ≈ 26 s
(mit Boot). Gates wieder grün (teddrive mit `ok`-Marke). **R5 OK, C5 OK**
– TED-C ist zum ersten Mal gelaufen: blitz2 lädt ihn, Versionsrequester
(Gadget 1 „ OKEE DOKEE “), Screen „Blitz Basic 2 - Ted Version 1.1“
640×512×2, Quelle geladen und angezeigt, QUIT ohne Rückfrage, blitz2 rc 0
in beiden. Schritte im teddrive-Log: Versionsrequester nach 0,30 s,
Screen idle 0,58 s, QUIT bis Programmende 0,74 s (R5 und C5 gleich).

Dekodierter Start-Dump (`results/ted-ab/runs/00-start/*/start.txt`):
Quelltext zeilengenau, Schlüsselwörter (Tokens) in Farbe 3, Statuszeile
„Line:1 Column:1 Largest Mem (K):…“. Die Darstellung auf dem
interleaved Screen ist korrekt (Befund aus Lauf 1 ohne Folgen).

A/B: **DIFF, 22 Bytes – nur der Screen-Titel**: er enthält den Pfad der
geladenen Datei, und der war je Kombination verschieden
(`TEDAB:runs/00-start/R5/…` gegen `…/C5/…`). Statuszeile „Largest Mem“
verschieden (63721 gegen 63829 K) – durch die Maske „Free“ abgedeckt.
Sonst bytegleich. Requester-Folge gleich, rc gleich. Die Fensterflags des
Versionsrequesters im Log ($300A gegen $1300A, `WFLG_RMBTRAP`) sind ein
Zeitpunkt-Artefakt der Momentaufnahme: beide TEDs setzen das Bit direkt
nach `openreq` (`menusoff`, ted.s:272/276); kein Verhaltensunterschied.

Behoben (nur Host): jeder Lauf bekommt sein Verzeichnis als `TEDRUN:`
(gleiche Pfade in allen Kombinationen, also gleicher Titel);
`collect` löscht Binärdateien früherer Tests nicht mehr.

### Vorbereitung Stufe 1 (nur Host)

* Testskripte `tests/emu/01a…01d` (Rundreise: t01_int, t08_tokquirk,
  CD `finish_mouse.bb`, `mikesmaplib.bb`), `02-type`, `03-join`,
  `04-block`, `05-find`, `06-compile`, `07-error`, `08-xtra`
  (`let.bb2` mit eigener `.xtra`), `09-opts` (ohne Datei, DEFAULTS,
  Tabstopp-Gadget 6 = 8, OK = Gadget 26, `savedefs` nach `TEDRUN:L`).
  Gadget-IDs/Labels aus `src/data.c` (findreq 1–8, repall 1–3,
  erasereq/defaultreq „ OK “/„ CANCEL “, errorreq „ OKAY “).
* teddrive: `freq ok "NAME" "PATH"` (Pfad-Gadget = oberstes
  Stringgadget). Für 06/07 nötig, weil die Vorbelegung von blitz2s
  Dateirequester unbekannt ist; die Lage der Freq-Gadgets wird vor der
  Sitzung mit 06/07 im Log von Test 01 geprüft.
* tedab: `@roundtrip` (gespeicherte Datei und `.bak` = Original), Schätzung
  je Lauf aus den Messungen (3–10 s statt 10–12 s); alle Läufe einer
  Sitzung hängen am Erfolg des ersten Laufs (Werkzeugproblem → Rest
  übersprungen statt Watchdog), weitere Kombinationen eines Tests am
  Erfolg der Referenz. Plan Stufe 1: 4 Sitzungen (~49/55/59/53 s).

### Lauf 3 (27.10.2026): Stufe 1, Sitzung 1 – Rundreisen 01a–01d

`tedab.py run --test '0[1-9]*' --session 1 --go`, Freigabe des
Koordinators, genau einmal. Emulator rc 0, 46,6 s mit Boot. Gates grün.
CD-Quellen nur als Kopien gestagt (md5 der CD-Dateien vorher/nachher
gleich).

* **01a (t01_int), 01b (t08_tokquirk), 01c (CD finish_mouse.bb): R5 OK,
  C5 OK, R5↔C5 SAME** – gespeicherte Datei, `.bak`, `.info` (396 Bytes,
  `srcinfo`), `.xtra` (1206 Bytes, von blitz2 `savextra` neu geschrieben),
  Dumps „start“/„saved“, Requester-Folge, rc. Orakel `@roundtrip`:
  gespeicherte Datei und `.bak` = Original in allen sechs Läufen.
  Lauf ≈ 1,7 s (teddrive-Zeit), davon Dateirequester 0,4 s.
* Dateirequester „Name to save file as“: 288×472 auf TEDs Screen, 10
  Gadgets; OK/DRIVES/PARENT/CD/CANCEL, Stringgadgets unten verankert
  (`GFLG_RELBOTTOM`): „PATH:“ bei −49, Dateiname bei −33. teddrives
  Zuordnung (Pfad = oberstes, Datei = unterstes Stringgadget) stimmt –
  `freq ok "NAME" "PATH"` für 06/07 ist damit belegt.
* **01d (CD mikesmaplib.bb): R5 FAIL step 4 timeout, C5 übersprungen.**
  Ursache im Testskript, nicht in TED: die `.xtra` nennt Residents
  `blitzlibs:libmacs.res`/`blitzoffs.res`; `blitzlibs:` gibt es nicht →
  zwei DOS-Requester „Nochmal versuchen|Abbrechen“ (von reqtrap mit
  Abbrechen beantwortet, `REQUESTER:` in stdout) und zwei blitz2-
  Fehlerrequester „ OKAY “, die das Skript nicht erwartete. Die Rettung
  beantwortete beide, danach öffnete TED seinen Screen neu (andere
  Geometrie: Labelspalte, Prop-Gadget bei x 531) und endete mit QUIT.
  Skript angepasst (zweimal `wait req "OKAY"` / `gad 1`); 01d bleibt als
  Rundreise mit Fehlerpfad (comdata `error`, `loadxtra`).

Nur Host danach: `--test` nimmt eine Liste von Mustern; Schätzungen nach
Messung (01/08: 3 s, 02: 9 s, 03: 4 s). Nächste Sitzung: 01d, 02, 03
(~57 s geplant).

### Lauf 4 und 5 (27.10.2026): Sitzung A (01d + 03) und B (02)

Vorgabe des Koordinators: geplante Sitzungslänge ≤ 50 s (`PLAN_LIMIT` in
tedab.py, Planer-Budget 50 − Boot − Gates). Beide Sitzungen je einmal
gestartet; Berichte unter `results/ted-ab/sessions/`.

* **A (37,8 s): 01d R5/C5 OK, SAME; 03 R5/C5 OK, SAME.** 01d mit den zwei
  erwarteten OKAY-Requestern (fehlende Residents) und dem Neuaufbau des
  Screens, Rundreise-Orakel erfüllt. 03: Split und JOIN ergeben wieder die
  Originaldatei (gespeicherte Datei = `.bak`), Cursor danach Zeile 4.
  Speicher vor/nach: −8 Bytes in R5 **und** C5 (Join-Leck BUG(orig),
  9 Bytes FreeMem ohne `lenitem`, auf 8 gerundet) – gleich, wie erwartet.
* **B (18,7 s): Gate D FAIL, Test 02 nicht gelaufen.** teddrivetest
  bekam statt der synthetischen Eingaben zehnmal echtes RAWKEY $21 („s“)
  mit Qualifier $0200 (`IEQUALIFIER_REPEAT`): eine im Emulator gehaltene
  Taste mit Autorepeat. teddrive selbst arbeitete richtig (Programm endete
  über die fremden Eingaben, „program exited“), das Gate hat den Rest wie
  vorgesehen gesperrt. Ursache vermutlich Host-Tastatur: Amiberry holt sich
  beim Start den Fokus; wird in dem Moment getippt, bleibt die Taste im
  Emulator gedrückt (kein Key-up). Kein Werkzeug- oder TED-Fehler.
  Abhilfe (nur Host): tedprobe liest die Tastaturmatrix
  (`keyboard.device` `KBD_READMATRIX`, Zeile `keys_down`) und lässt Gate 0
  scheitern, solange eine Taste gedrückt ist – dann beginnt keine Eingabe
  an teddrivetest oder TED. Bitte während der Sitzungen nicht tippen.

### Kopfloser Emulator (Koordinator, 27.10.2026)

Ab jetzt läuft Amiberry standardmäßig in einem verschachtelten virtuellen
KWin (`kwin_wayland --virtual --socket tedab-PID --width 1280 --height 1024
--exit-with-session inner.sh`; im inneren Skript `DISPLAY`,
`XDG_ACTIVATION_TOKEN`, `DESKTOP_STARTUP_ID` entfernt,
`SDL_VIDEO_DRIVER=wayland`, rc von amiberry_run in eine Datei, weil kwins
rc nicht der innere ist). Kein Fokusraub, keine Host-Eingaben. Äußere
Zeitgrenze = Sitzungs-Timeout + 20 s, danach wird die Prozessgruppe
beendet. `--visible` = alter Weg. Umgesetzt in `tedab.py`
(`run_emulator`, `inner_script`, `kwin_command`; Dateien unter
`build/ted-ab-run/`), `../ab3c` und `compilerlauf/tools` unverändert. Die
Tastaturmatrix-Prüfung in Gate 0 bleibt.

### Lauf 6–8 (27.10.2026): Sitzungen C (02), D (04, 05), E (06) – kopflos

Je einmal, Emulator rc 0, Gates grün, `keys_down` leer.

* **C (43,5 s): 02 R5/C5 OK, SAME.** TEDs Return teilt die Zeile nicht,
  sondern öffnet eine neue Zeile darunter (der zuerst getippte Text steht
  vor der alten Zeile 0) – in beiden gleich; Testbeschreibung korrigiert.
  Orakel: gespeicherte Datei = `bb2tok.tokenise` des erwarteten Textes
  einschließlich der Leerzeile nach dem letzten Return (949 Bytes, R5 und
  C5). Tippen ≈ 0,16 s je Taste, Lauf 8,4 s.
  Nachtrag zu 03: auch dort öffnete Return eine Leerzeile, JOIN entfernte
  sie wieder – daher Ergebnis = Original.
* **D (41,2 s): 04 R5/C5 OK, SAME; 05 R5/C5 OK, SAME.** 04: Block Zeilen
  1–3 invers markiert, COPY + KILL verschieben ihn hinter „Dim m.l(3,4)“.
  05: Find-Requester (8 Gadgets), NEXT, REPLACE, REPLACE ALL mit
  repall-Requester „FROM TOP“ – alle `NPrint` → `Print` (gleiche
  Dateigröße, beides Tokens).
* **E (33,4 s): 06 R5/C5 OK, SAME.** blitz2s Dateirequester „Name of
  Executable to create“, `freq ok "t01_int" "TEDRUN:work"`, COMPILING-
  Requester, zurück im Editor nach 1,4 s. Executable 10680 Bytes in R5 und
  C5 **bytegleich mit `compilerlauf/results/corpus-ref/t01_int`**
  (blitz2 1.95 + tedstub, ohne Runtime Errors). Dazu `.info` des
  Executables (383 Bytes) und neu geschriebene `.xtra` (1226 Bytes), beide
  A↔B gleich.

### Lauf 9 (27.10.2026): Sitzung F (07, 08) – kopflos

35,1 s, rc 0, Gates grün. **07 R5/C5 OK, SAME; 08 R5/C5 OK, SAME** – aber:

* 07: Fehlerrequester „ OKAY “ nach dem Kompilieren, danach steht der
  Cursor (inverse Zelle) auf Zeile 2 (`b=(a+`) – richtig. Die Statuszeile
  zeigt weiter „Line:1“: TED aktualisiert sie nach dem Sprung zur
  Fehlerzeile nicht. In R5 und C5 gleich (Verhalten des Originals, kein
  DIFF).
* **08: es wurde nichts gespeichert** – weder `let2.bb2` noch `.bak`/`.info`,
  `.xtra` unverändert (savextra nicht gerufen), in R5 **und** C5; kein
  Fehlerrequester (also kein `doopen`-Fehler) → der Dateirequester lieferte
  offenbar „Abbruch“ oder einen anderen Namen. Unterschied zu 01a (Speichern
  ging): teddrive schrieb den Namen in das Dateinamen-Gadget, das der
  Requester beim Öffnen mit `ActivateGadget` aktiviert (die Stringgadgets
  von Gate D sind nie aktiv). Werkzeugfehler (Test-Orakel nicht erfüllt) →
  nach Vorgabe Halt, Sitzung G nicht gestartet.
  Nur Host: teddrive protokolliert jetzt die Puffer aller Stringgadgets
  (bei neuem Fenster, nach `str`, vor dem Knopf in `freq`, mit „(active)“
  bei `GFLG_SELECTED`); 08 macht nach dem Speichern einen Dump (Titel =
  Dateiname). Nächste Sitzung: 00 (Wiederholung mit `TEDRUN:`), 08
  (Diagnose), 09.

### Erwartungen auf dem Host (Koordinator: „Ergebnis OK allein ist kein Pass“)

`tedab.py`: `; @expect KIND ARGS` (`same`, `bak`, `exists`, `absent`,
`tok NAME EXPECTFILE` = `bb2tok.tokenise`, `equals NAME PFAD`, `lexists`),
`@roundtrip` = `same` + `bak`; ein Test ohne `@roundtrip`/`@expect`/
`@nofiles` wird abgelehnt. Ein Lauf ist nur bestanden, wenn teddrive OK
meldet **und** alle Erwartungen gelten, sonst `FAIL (expectations)`.
Erwartungen für alle Tests eingetragen (01: Datei, `.bak`, `.info`,
`.xtra`; 02/04/05: Textorakel `tests/emu/0x-*.expect` über bb2tok; 03:
= Original; 06: Executable = `compilerlauf/results/corpus-ref/t01_int`;
07: kein Executable, Quelle unverändert; 08: `let2.bb2` = `let.bb2`,
`.xtra`/`.info` vorhanden; 09: `L/blitzeditor.opts`). Gegen die
vorhandenen Ergebnisse 01–07 geprüft: alle erfüllt; das alte 08 wird FAIL.

### Lauf 10 (27.10.2026): Sitzung H (00, 08, 09) – kopflos

35,4 s, rc 0, Gates grün.

* **00 R5/C5 OK, SAME** – mit `TEDRUN:` ist auch der Screen-Titel gleich.
* **09 R5/C5 OK, SAME** – DEFAULTS (Screen wird geschlossen/neu geöffnet,
  teddrive folgt), Gadget 6 (vorher „2“) = „8“, OK; `L/blitzeditor.opts`
  50 Bytes, in R5 und C5 gleich.
* **08 R5/C5 FAIL (expectations)** (A/B gleich). Diagnose aus dem neuen
  Log: TED belegt das Dateinamen-Gadget mit dem **vollen** Namen
  `TEDRUN:work/let.bb2` vor, `PATH:` ist leer (der Requester liefert
  PATH + Name). `freq ok "let2.bb2"` ersetzte das durch einen relativen
  Namen; TED speicherte `let2.bb2` (+ `.xtra`, `.info`) **relativ zum
  aktuellen Verzeichnis** des Sitzungsskripts (Titel „File: let2.bb2“) –
  auf keinem Host-Laufwerk gefunden, also im Emulator, vermutlich `SYS:`
  der Benutzer-Hardfile (Sitzungen F und H, gleicher Name, überschrieben).
  Werkzeugfehler mit Nebenwirkung auf die Hardfile – gemeldet.
  Behoben (Host): 08 mit vollem Namen; Lint verbietet `freq ok NAME` mit
  relativem Namen ohne PATH; jeder Lauf mit `CD TEDRUN:work` (relative
  Pfade landen im erfassten Laufverzeichnis), danach `CD TEDAB:`; Sitzung
  schreibt ihr Startverzeichnis nach `session.cd`. Bereinigung als
  einmaliger Vorspann `tests/emu/cleanup-let2.ados` (`--pre`): löscht nur
  `let2.bb2`, `let2.bb2.xtra`, `let2.bb2.info` relativ zum Startverzeichnis
  (= wo TED sie schrieb), Liste vorher/nachher nach `cleanup.txt` – nur mit
  Freigabe.

### Lauf 11 (27.10.2026): Sitzung I (08 + Ortsbestimmung) – kopflos

Vorgabe: **nichts löschen** (die Hardfile gehört dem Benutzer). Vorspann
nur lesend (`tests/emu/locate-let2.ados` über `--pre`: `CD` und
`List let2.bb2#? QUICK` nach `cleanup.txt`).

* **08 R5/C5 OK, SAME, Erwartungen erfüllt**: `let2.bb2` = `let.bb2`,
  `let2.bb2.xtra` und `.info` geschrieben (mit vollem Namen
  `TEDRUN:work/let2.bb2` im Dateinamen-Gadget).
* `cleanup.txt`: Startverzeichnis der Sitzung ist **`Workbench:`** (=
  `SYS:` der Benutzer-Hardfile `Workbench_OS3.2-AB3-AGA.vhd`); dort liegen
  vier Dateien aus den Sitzungen F und H: `let2.bb2`, `let2.bb2.bak`
  (H hat Fs Datei umbenannt), `let2.bb2.xtra`, `let2.bb2.info` (zusammen
  1 KB, 10 Blöcke). Nicht gelöscht – Entscheidung des Benutzers.

## Stand nach Stufe 1 (27.10.2026)

Konsolidierter Bericht `results/ted-ab/report.txt` (`tedab.py report
--all` über `results/ted-ab/sessions/*.session.log`; das Log der ersten
Stufe-1-Sitzung `s0-01` aus der damaligen Ausgabe wiederhergestellt, da
die Archivierung erst danach eingeführt wurde):

| Test | R5 | C5 | R5↔C5 | Orakel |
|---|---|---|---|---|
| 00 Start/Ende | OK | OK | SAME | – (`@nofiles`) |
| 01a–01d Rundreise (2 Korpus-, 2 CD-Quellen, eine mit fehlenden Residents) | OK | OK | SAME | Datei und `.bak` = Original, `.info`/`.xtra` da |
| 02 Tippen | OK | OK | SAME | = bb2tok(erwarteter Text) |
| 03 Return + JOIN | OK | OK | SAME | = Original; Join-Leck −8 Bytes in beiden |
| 04 Block | OK | OK | SAME | = bb2tok(Block verschoben) |
| 05 Suchen/Ersetzen | OK | OK | SAME | = bb2tok(alle `NPrint` → `Print`) |
| 06 Kompilieren | OK | OK | SAME | Executable = `compilerlauf/results/corpus-ref/t01_int` |
| 07 Fehlerzeile | OK | OK | SAME | kein Executable; Cursor Zeile 2 (Statuszeile bleibt „Line:1“ – Original) |
| 08 `.xtra` | OK | OK | SAME | `let2.bb2` = `let.bb2`, `.xtra`/`.info` |
| 09 Optionen | OK | OK | SAME | `L/blitzeditor.opts` (50 Bytes) |

Ergebnis: **TED-C verhält sich in allen Stufe-1-Tests beobachtbar wie die
Referenz** (Bildschirm bytegleich bis auf die maskierte Speicheranzeige,
Dateien bytegleich, Requester-Folge und rc gleich). Keine neue Zeile für
`abweichungen.md`. Beobachtetes Originalverhalten (in beiden gleich):
Return teilt keine Zeile, sondern öffnet eine neue darunter; die
Statuszeile wird nach dem Sprung zur Fehlerzeile nicht aktualisiert;
Dateinamen-Gadget mit vollem Namen vorbelegt, `PATH:` leer.

Werkzeugstand: 11 Emulator-Sitzungen, alle ≤ 50 s geplant (tatsächlich
19–47 s), keine Hänger, kein Absturz. Eine Sitzung durch eine gehaltene
Host-Taste gesperrt (Gate D) – seither kopflos im verschachtelten KWin und
Tastaturmatrix-Prüfung in Gate 0. Ein Werkzeugfehler mit Nebenwirkung
(relativer Dateiname in 08 → vier Dateien in `Workbench:`), behoben; die
Dateien liegen noch dort.

Offen für Stufe 2a: Rc/Cc (blitz2-c mit beiden TEDs).

## Stufe 2a (27.10.2026): Rc/Cc – blitz2-c mit beiden TEDs, kopflos

Vorbereitung (nur Host): Standard-Kombinationen eines Tests = alle vier
(`--combos` wählt, in dieser Reihenfolge: Referenz zuerst, `Rc` vor
`Cc`); jede Sitzung archiviert Log und Bericht selbst nach
`results/ted-ab/sessions/sNN-…` (Archivnamen zweistellig umbenannt,
damit `report --all` in der richtigen Reihenfolge liest). blitz2-c:
`build/amiga/blitz2` aus dem eingecheckten Stand (`ninja: no work to do`).

Sechs Sitzungen (`--combos Rc,Cc --session 1…6`), je einmal, je ≤ 49 s
geplant, alle Gates grün, keine Hänger:

| Sitzung | Tests | Rc | Cc | Rc↔Cc |
|---|---|---|---|---|
| s09 | 00, 01a, 01b, 01c | OK | OK | SAME |
| s10 | 01d, 02 | OK | OK | SAME |
| s11 | 03, 04 | OK | OK | SAME |
| s12 | 05, 06 | OK | OK | SAME |
| s13 | 07, 08 | OK | OK | SAME |
| s14 | 09 | OK | OK | SAME |

Alle Erwartungen erfüllt – auch 06: das über TEDs Menü mit **blitz2-c**
erzeugte Executable ist bytegleich mit `corpus-ref/t01_int` (blitz2 1.95).

Konsolidiert (`report --all`, alle vier Kombinationen): zunächst DIFF in
R5↔Rc und C5↔Cc bei 01–07 – Auswertungsartefakt: teddrive hängt seit
Sitzung H an Stringgadget-Zeilen ` buffer "…"` an, `requesters()` nahm
das als Label (die Stufe-1-Logs 01–07 stammen vom älteren teddrive).
Parser korrigiert (Label = erstes "…" nach der Größe, Puffer wird nicht
verglichen, Host-Test dazu). Danach **52 von 52 Vergleichen SAME** (13
Tests × R5↔C5, Rc↔Cc, R5↔Rc, C5↔Cc).

Ergebnis Stufe 2a: TED-C und Referenz-TED verhalten sich auch mit
blitz2-c gleich; blitz2-c verhält sich mit beiden TEDs wie blitz2 1.95
(Bildschirm, Dateien, Requester-Folge, rc, Executable). Die zwei
`-fbaserel`-Programme mit je eigenem a4 (overview.md §4 Risiko 2) zeigen
in diesen Tests kein Problem.

## Stufe 2b (27.10.2026) – kopflos, R5/C5

Vorbereitung (nur Host):
* `tests/emu/gen2b.py` erzeugt die Familien: `20a–20i` Rundreisen über
  CD-Quellen (tokenisiert, `.xtra` ohne Residents, 96 Bytes bis 120 KB),
  `21a/21b` Textquellen (= `build/host/fixtures/tok/716/717.bb2` des
  Host-Tests), `29-cmp-*` Kompilieren von t02–t13 über das Menü
  (= `compilerlauf/results/corpus-ref`). Von Hand: `22-scroll` (BOTTOM,
  TOP, 62× abwärts, GOTO 30), `23-font6`, `24-blockfile` (BLOCK
  SAVE/LOAD), `25-tab` (TAB/UNTAB/MARK INDENT), `26-newload` (NEW mit
  erase-Requester, LOAD), `27-help` (Help-Taste), `28-long` (Zeile > 127
  Zeichen, Umlaute, $FD–$FF, getippte Umlaute; Quelle
  `tests/emu/src/long_umlaut.bas`, Quellart `E`).
* tedab: `@opts` (Optionsdatei nach `L/` stagen), Erwartung `opts`
  (Inhalt der Optionsdatei; Generator gegen TEDs eigene Datei aus 09
  geprüft: bytegleich), `@font 6`, Planer verweigert Sitzungen über
  50 s; Lint im Preflight liest die `.drv` als Latin-1 (vorher brach
  `--go` am UTF-8-Lesen von `28-long.drv` ab, ohne Emulatorstart).
* **09 korrigiert**: TED übernimmt die Zahlenfelder des DEFAULTS-
  Requesters nur beim GADGETUP des Stringgadgets selbst (wie Return; in
  `defaults()` verlässt OK = Gadget 26 die Schleife vor `tabstop =
  int3`, asm und C gleich). Stufe 1 hatte deshalb tabstop 2 gespeichert
  (in R5 und C5 gleich, SAME, aber Testziel verfehlt – die schwache
  Erwartung `lexists` hat es nicht bemerkt). Jetzt `gad 6` nach `str`,
  Erwartung `opts … tabstop=8`.

Sitzungen (`--test '09*,2*' --combos R5,C5 --session N`):

| Sitzung | Tests | R5 | C5 | R5↔C5 |
|---|---|---|---|---|
| s15 | 09 (neu), 20a, 20b | OK | OK | SAME (09: Optionsdatei mit tabstop 8) |
| s16 | 20c–20f | OK | OK | SAME |
| s17 | 20g, 20h | OK | OK | SAME |
| s18 | 20i (120 KB, 4762 Zeilen), 21a | OK | OK | SAME |
| s19 | 21b | OK | OK | SAME |
| s20 | 22 | OK | OK | SAME |
| s21 | 23, 24 | **CRASH** (R5) | nicht gelaufen | – |

**s21: der Referenz-TED (R5) stürzte beim Start mit der Optionsdatei
`fonthite 6` ab** – vor dem Versionsrequester. teddrive-stdout:
`REQUESTER: EasyRequest title "Software-Fehler" … gadgets "Später
neustarten|Gleich neustarten" -> answered 0` (reqtrap: 0 = rechter Knopf
= Neustart). Der Emulator startete neu, die Reboot-Sperre griff
(„SESSION second boot - skipped“), Sitzung sauber beendet (rc 0), C5 und
24 nicht gelaufen. Nach Vorgabe Halt.

Hypothese (nicht bestätigt): mit Font 6 ergibt TEDs `calcscreen` einen
Screen von **513** Zeilen (81 × 6 + 6 + 9 + 12, Gate 0
`ted_height_f6 513`), der Workbench hat **512**. Begrenzt OS 3.2 den
geklonten Screen auf 512 Zeilen, scheitert `OpenWindow` des
Hauptfensters (Top 12 + Höhe 501 = 513) → `wfail` → `cfail` = bekannter
BUG(orig) survey-05 I.1 (O2: nachgebildet): `jsr closescreen(a6)` mit
a6 = ExecBase ruft `Exception()` → Absturz. Dann wäre der Fall
KNOWN-BOTH (TED-C bildet den Fehler absichtlich nach).

### Emulator ohne Fenster statt verschachteltem KWin (Koordinator, 27.10.2026)

Der KWin-Weg (Abschnitt „Kopfloser Emulator“) ist ersetzt. Amiberry 8.4
(SDL3) läuft mit `SDL_VIDEO_DRIVER=offscreen`, `SDL_AUDIO_DRIVER=dummy`
und ohne `DISPLAY`/`WAYLAND_DISPLAY` ganz ohne Fenster; Fokusraub und
Host-Eingaben sind damit ausgeschlossen, ein Compositor und die Wächter
für die Plasma-Sitzung entfallen. Gemeinsam für alle Werkzeuge in
`bb2/tools/amiberry.py`: abgeleitete Konfiguration unter
`build/amiberry/amiberry.uae` (Amiberry-Verzeichnis verlegt,
`use_gui=no`, `sound_output=none`), eigene XDG-Verzeichnisse, äußere
Zeitgrenze + 20 s mit Beenden der Prozessgruppe. Prüflauf: `tedab.py gate
--go` mit `A1200-AB3-AGA.uae` – Gate 0 OK (640×512, Tiefe 8, Modus
$00029004, Chip-Planes), Gate D main und rescue OK.

### Emulatorlauf als CMake-Skript mit `uaequit` (Koordinator, 27.10.2026)

`bb2/tools/amiberry.cmake` ersetzt `../ab3c/tools/amiberry_run.py` und den
Python-Teil von `tools/amiberry.py` (jetzt nur noch Aufruf des Skripts).
Das Sitzungsskript endet mit `AB3C:build/amiberry/bb2-tools/uaequit`
(`tools/uaequit.c`, beim ersten Lauf gebaut): uaelib-Trap `$F0FF60`,
Funktion 13 `ExitEmu`. Amiberry beendet sich kurz danach selbst (der Aufruf
kehrt noch zurück, daher als letzter Befehl); `execute_process(TIMEOUT)`
beendet Hänger. Prüfläufe: `amiga_test.py` (7 Testfälle), `bb2compile.py
--run t01_int`, `tedab.py gate --go` (Gate 0 und D OK), `serialrun.py`
(TCP-Port über `OPTIONS`, `t64_serialrx` gleich).

### `sound_output=interrupts` statt `none` (Koordinator, 27.10.2026)

Mit dem Emulator ohne Fenster hing in jeder Sitzung jeder TED-Lauf nach dem
ersten an Schritt 4 (`wait screen` nach „OKEE DOKEE“: der Start-Requester
schloss nicht) – beim Original-TED wie bei TED-C, reproduzierbar
(Sitzung `01c`, `06`, `08` mit R5/C5: nur `01c`/R5 OK). Ursache war
`sound_output=none` in der abgeleiteten Konfiguration (vermutlich ohne
emulierte Audio-Interrupts). Mit `sound_output=normal` und mit
`sound_output=interrupts` (Interrupts emuliert, keine Ausgabe; zusammen mit
`SDL_AUDIO_DRIVER=dummy`) laufen alle sechs Läufe OK. `tools/amiberry.cmake`
setzt jetzt `interrupts`. Der Korpus (`corpus_check`, 201 Prüfungen) war
davon nicht betroffen.

