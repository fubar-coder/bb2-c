# Befehls-Libs in C: Befunde je Library

Stand der Portierung der "einfachen" Befehls-Libraries aus `acidlibs`
(Übersicht und Teststand: `../README.md`). Quelle jeweils
`bb2src/NAME.bb` (detokenisiert aus `bblibsrc`), Referenz das CD-Binary
`ref/NAME.obj`. `BUG(orig)` = Eigenheit des Originals, absichtlich
übernommen (für den Vergleich mit AmiBlitz3).

## Allgemein

* **Aufrufkonvention der Befehle** (aus Quelltexten und `blitz2.s`
  `preplibst`): Argumente in d0-d5; Argumente, die der Compiler auf den
  Stack legt (String-Längen, Array-Ende und -Typ), liest die Routine über
  a2 = Stackzeiger vor dem Ablegen (`MOVE.l -(a2),dN`). Die Originale
  lassen a2 danach vermindert stehen. Alle C-Fassungen bekommen a2 als
  Register-Parameter der jeweiligen Routine (`BL_REG(a2)`), lesen mit
  negativen Offsets und lassen a2 unverändert (DIFF); eine globale
  Registervariable für a2 gibt es nirgends. Begründung: der Compiler setzt
  a2 vor jedem Aufruf neu (`preplibst`: `LEA N(a7),a2`), liest es danach
  nie (er räumt den Stack über a7 ab), und Lib-Code verlässt sich darauf,
  dass a2 über ALibJsr-Aufrufe erhalten bleibt (windowslib `dowtitle`,
  screenslib); die libsdev-Dokumentation beschreibt a2 nur als Eingabe.
  Siehe `architektur.md` 5.5 und `cd/extras/libsdev/documentation`.
* **String-Ergebnisse** ab a3, d0 = Länge; a3 zeigt danach normalerweise
  auf das Nullbyte (`blitz_string` in C). Eine **fehlende abschließende 0**
  ist praktisch unsichtbar: Zuweisung, Print und Verkettung arbeiten mit
  der Länge, und wo der Compiler einen String als Adresse/Argument
  weitergibt, hängt er selbst `clr.b (a3)+` an (`sbtolong`/`ceos`,
  blitz2.s:8767–8771, C `eval.c:567`) – a3 steht dabei hinter den Daten.
  Die Einträge „ohne 0“ unten (Peeks$, RexxError, ReadSerialString,
  BoxString) sind daher Eigenheiten ohne beobachtbare Folge; sichtbar wird
  dagegen ein a3, das *hinter* der 0 steht (GetRexxCommand/GetResultString,
  FileRequest$).
* **Registererhalt:** Nach AmiBlitz3-Spec darf eine Lib-Routine alles
  außer a4/a5/a7 ändern. Wie bei den System-Libs erhalten die C-Fassungen
  trotzdem mindestens die Register, die das Original erhält (Thunks,
  geprüft in `[cmd]`).
* **Gemeinsame Hilfen:** was mehrere Ports brauchen, steht in
  `include/blitzlib.h`/`blitzhelp.h` statt lokal: `BLITZ_ALIBJSR_A6`
  (ALibJsr mit a6, vallib), `BLITZ_ERRCHK_THUNK`/`BLITZ_ERRCHK_MESSAGE`
  (Fehlerprüfungen in C mit Meldungen im Fehlerprüfbereich, linklistlib,
  vallib), `BLITZ_BASEJSR1/2` + `LVO_SP…` (mathffp), `BL_SETW`/`BL_SETB`/
  `BL_EXTL`/`BL_SWAP`, `bl_divu_w`/`bl_divs_w` (datelib, modlib),
  `BL_POPL`/`BL_POPW` (a2-Cursor, stringfunclib, vallib), `TOK_…`/`MEMBASE_…`
  (memlib, floatquicklib usw.). Neue Ports benutzen diese statt eigener
  Varianten.
* **Inline-Routinen** (`Dc $a000,len`): bleiben Assembler-Text
  (`BLITZ_INLINE`); hinter dem Inline-Code steht in C ein RTS, das
  Original hat keines (der Compiler kopiert nur `len` Bytes).

## 180 memacclib (Poke, Peek, Peeks$, Call)

Binary = Quelle. Poke/Peek sind die einzigen `!ustatement`/`!ufunction`
von BB2: Typ in d0 (1/2/3/4/5/7), Adresse in d1, Daten in d2.

* `Peek.b`/`Peek.w` ersetzen nur das untere Byte/Wort von d0 (Rest =
  obere Bits des Typs, beim Compiler 0); Typvergleich als Wort, Typ 0
  wie Byte, alle anderen außer 2 und 7 wie Long.
* `Peek$` kopiert bis einschließlich 0, a3 danach auf der 0.
* `Peeks$(a,n)`: **BUG(orig)** schreibt keine abschließende 0 (Ergebnis
  nur über d0 = n; ohne sichtbare Folge, s. Allgemein); n wird als vorzeichenlose 32-Bit-Zahl gezählt
  (negativ = ~4 GB). Ein Nullbyte innerhalb der n Zeichen bleibt drin.
* `Poke.s`: kopiert die Länge des Strings (vom Stack) + 0, eingebettete
  Nullbytes inklusive.
* `Call`: `MOVE.l d0,a0:JMP (a0)` - bleibt Assembler, weil die aufgerufene
  Routine alle Register unverändert bekommt und frei ändern darf.
* `Poke a,x` ohne Typ = Quick (`Poke a,-77` schreibt $FFB30000).

## 110 sortlib (Sort, SortUp, SortDown)

Binary = Quelle. Rekursiver Hoare-Quicksort ("Blitz 1 Sort algorithm"),
Richtung über ein Wort im Code (Init = SortUp). Typ vom Compiler (Wort auf
dem Stack): 1 Byte, 2 Word, 5 Float (mathffp `SPCmp`, a6 = ffplib),
7 String (strcomplib `$CE02`), alles andere (3 Long, 4 Quick) als Long.

* **BUG(orig):** bei Word-Arrays wird das Pivot-Element mit `MOVE.l`
  gelesen und mit `CMP.w` verglichen - Pivot ist das Element **nach** der
  Mitte. Bei Arrays mit mindestens 2 Elementen liegt es noch im Bereich,
  das Ergebnis ist richtig; bei einem einzelnen Element (`Dim a.w(0)`)
  liegt es hinter dem Array: die Vorwärtssuche hält auf diesem Wort, die
  Rückwärtssuche liest ggf. Wörter vor dem Array; danach i > j, kein
  Tausch, keine Rekursion – nur Lesen, Ergebnis richtig (nicht im Korpus).
* String-Elemente, die nie zugewiesen wurden (Zeiger 0), lesen ihre Länge
  von -4(0).
* Rekursionstiefe im schlechtesten Fall O(n); die C-Fassung braucht je
  Ebene mehr Stack als die 12 Bytes des Originals.
* Assembler nur für die Auswertung des CCR von `SPCmp` (das Original
  verzweigt direkt auf die Flags); strcomplib liefert -1/0/1 in d0 und
  denselben Wert im CCR, dort genügt d0.

## 192 mathtranslib (ACos … HTan, `^` auf Float)

Binary = Quelle; gültige Lib-Nummer ist die zweite Definition
`#mathtranslib=192` in libnums.bb. Alle 14 Routinen sind Inline-`JSR lvo(a6)`
mit a6 = mathtrans-Basis (Init-Wert).

* **BUG(orig):** Init versucht `OldOpenLibrary("mathtrans.library")` endlos,
  wenn die Library fehlt.
* Randwerte (Log(0), Sqr(-1), ACos(2), 10^19 …) liefert mathtrans selbst,
  z. B. Log(0) = -9.223371E+18; nicht Sache der Lib.

## 194 utillib (CreateMsgPort, DeleteMsgPort, Wait, TaskName$)

Binary = Quelle (bum8). Ports liegen in einem Block von $2A Bytes
(memlib globalloc): +0 next, +4 prev (eigene Port-Liste, Kopf
`PortListStart`), +8 MsgPort; der Name in einem eigenen Block
{Größe.l, Zeichen, 0}. Finit löscht alle noch vorhandenen Ports. Der
Init-Wert ist die **Adresse** der Variablen `signals` (für andere Libs:
`#utillib,#la0`, dann `btst sigbit,(a0)`).

* **BUG(orig)** `TaskName$` lässt a3 **hinter** dem Nullbyte stehen
  (kein `SUBQ #1,a3`); d0 = Länge stimmt, im Korpus ohne sichtbare Folgen
  (`TaskName$(0)+"|"` ist richtig). Task ohne Namen liest ab Adresse 0.
* `CreateMsgPort` gibt mit a3 = Name-Argument zurück ("Protect BB2 String
  area"); die Namenslänge kommt aus -4(a2) des Aufrufers; kopiert werden
  (Länge & $FFFF)+1 Bytes (`DBRA`). Leerer Name: Port nicht öffentlich,
  `ln_Name` = 0.
* **BUG(orig):** ein Fehlschlag von `AllocSignal(-1)` (-1) wird nicht
  erkannt (`TST.l d0:BEQ` prüft auf Signal 0), `mp_SigBit` = $FF;
  `DeleteMsgPort` gibt dann `FreeSignal(255)` (in C ausdrücklich
  null-erweitert, die NDK-Inline nimmt BYTE). Der nie erreichte
  Fehlerzweig würde Port-Adresse (Block+8) und $22 statt Block und $2A
  freigeben.
* Speicherfehler → Laufzeitfehler "Failed to allocate memory for Port"
  (`TRAP #0`); das Original läuft danach in seinen Meldungstext, die
  C-Fassung kehrt mit 0 zurück. Nicht testbar.
* `DeleteMsgPort` prüft nichts: anstehende Nachrichten bleiben
  unbeantwortet, fremde Ports zerstören die Liste.
* `Wait` wartet auf **alle** belegten Signale der Task (`tc_SigAlloc`),
  Ergebnis auch in `signals`. Im Test vorher mit Inline-Assembler
  `Signal(FindTask(0),SIGBREAKF_CTRL_F)`.

## 193 environlib (WBWidth … WBExtViewMode, Processor, cr_vbr, ExecVersion)

Binary = Quelle. Alles wird im Init (a6 = intuition) einmal ermittelt:
`GetScreenData(buf,346,WBENCHSCREEN,0)` (Ergebnis ungeprüft), Breite
+12, Höhe +14, Tiefe aus RastPort.BitMap (+88) → Depth (+5).

* Der Long bei +76 ist `ViewPort.Modes` (oberes Wort, WBViewMode) und
  SpritePriority/ExtendedModes (unteres Wort, WBExtViewMode).
* `Processor` = Anzahl Shifts, bis `AttnFlags & 15` null ist (68020 → 2).
* **BUG(orig):** `cr_vbr` wird nur ab Processor ≥ 2 gelesen (`MOVEC VBR`
  im Supervisor-Modus über SuperState/UserState), auf dem 68010 bleibt es
  0, obwohl er ein VBR hat. `MOVEC` als `.word` (68000-Assembler).
* Die Wort-Funktionen laden mit `MOVE.w`, das obere Wort von d0 bleibt -
  in C über `BLITZ_THUNK_POST(... "move.w d0,2(sp)")` nachgebildet.
* Werte sind umgebungsabhängig, in jedem Lauf derselben Amiberry-
  Konfiguration aber gleich (A1200: 640x512x8, Modes $C004, 68020,
  VBR 0, Exec 47).

## 78 datelib

C-Übersetzung von bblibsrc/datelib, Binary = Quelle. Die Wort-/Byte-Arithmetik des Originals ist registergenau nachgebildet (`BL_SETW`/`BL_SETB`, DIVU/DIVS als Befehl mit `bl_divu_w`/`bl_divs_w` wie in modlib), damit auch ungültige Eingaben gleich ausgehen. Erhaltene Eigenheiten (BUG(orig)): Tag 366 eines 4-Jahres-Zyklus erscheint als „32/12“ des Schaltjahres; WeekDay rechnet `SUBQ.w #2`, NumDays `ADDQ.w`/`SUBQ.w` ohne Übertrag (NumDays("31/12/1977") = 65535); Monat 0 liest das Tick-Wort des letzten DateStamp, Monate 14/15 die zwei Wörter hinter der Tabelle (nachgebildet), Monate > 15 lesen fremden Speicher (nicht nachbildbar); zweistellige Jahre ohne „/“ lesen über das Stringende. Test: t30_date (Ausgaben gleich, Executables mit --no-runerr bytegleich).

## 128 linklistlib

CD-Binary älter als die Quelle (SetItem/CountList/CountItem fehlen) – portiert nur die 14 Tokens + init des Binarys, Code sonst gleich der Quelle. Interrupt-Sperre ($dff09a) wie im Original, Listenköpfe als Knoten, Item-Stack über memlib globalloc/globfree. Fehlerprüfungen in C im Fehlerprüfbereich (`BLITZ_ERRCHK_THUNK`, Meldungen als Assembler-Daten mit `BLITZ_ERRCHK_MESSAGE`). BUG(orig): SortList vergleicht mit CMP.l/BMI (Vorzeichen der Differenz, ±2·10^9 bleibt unsortiert), ClearList hängt die Items in umgekehrter Reihenfolge in die Freiliste, gelöschte Items behalten ihre Daten. DIFF: SortList zerstört a3 nicht. Tests: t31_list sowie die Laufzeitfehler-Tests t32–t36 in corpus/tests-runerr (nur mit Fehlerprüfung lauffähig); alle Ausgaben gleich.

## 185 stringfunclib

C, Deklarationen identisch mit dem CD-Binary (Quelle = Binary). a2 (Längenstapel der String-Argumente) ist Register-Parameter jeder Routine, die Längen liest; gelesen wird über einen lokalen Zeiger, a2 bleibt unverändert (DIFF, siehe Allgemein). Inline bleiben Chr$ und Len (Assembler-Text). Der Init-Wert ist wie im Original die Adresse von movestring (a0/a3 werden weitergezählt; keine andere Lib der CD benutzt ihn). Das Original lässt a2 je nach Fall an verschiedenen Stellen stehen (String$ mit Anzahl ≤ 0 und Replace$ mit leerem Suchstring nehmen weniger Längen vom Stapel, Cvi/Cvl/Cvq gar keine); die C-Fassung lässt a2 immer unverändert (DIFF, Ergebnisse gleich). Das Inline-Len bleibt `MOVE.l -(a2),d0` wie im Original. BUG(orig) erhalten: Right$ rechnet mit Wort-Offset; Instr mit Startpos ≤ 0 vergleicht vor dem String; Mid$ mit riesiger Länge kopiert zu viel. Test: `t20_strfunc` (alle 28 Tokens, Ausgabe gleich mit Original-Libs).

## 120 vallib

C, Deklarationen identisch. Int/Sgn/Abs/Val und intchk bekommen a2 als Register-Parameter und lesen Typwort bzw. Länge mit negativem Offset; a2 bleibt unverändert, das Original lässt es um 2 bzw. 4 vermindert stehen (DIFF, siehe Allgemein). Alle Float-Rechnungen laufen über mathffp mit denselben Aufrufen und Operanden. Inline bleiben QFrac, QWrap, QAbs, VPos, True und False; die arc-Tabelle von QAngle stammt aus dem Binary, DIVU samt Überlauf ist in C nachgebildet. Rnd liest die Hardware in derselben Reihenfolge; die Folge hängt vom Timing ab, daher nur Bereichsprüfung. BUG(orig) erhalten: Sgn eines Floats ist immer 1 (SPTst prüft D1 = Typwort); Val liest nach „-“, „$“ und „%“ ein Byte hinter dem String (normalerweise die abschließende 0); Val zählt die Länge als Wort; ab 32 Nachkommastellen fällt der Bruchteil weg; Abs(Quick 0) läuft über `$80`. Test: `t21_val` (alle 16 Tokens, Ausgabe gleich).

## 124 fileiolib

C, Deklarationen identisch (Binary = Quelle, März 1995). Objekt-Lib „File“ (32 Bytes, Maximum 5, Free = CloseFile); Objektzeiger je nach Token in a1/a2/a3 ($1180/$1280/$1380). Die gepufferte DOS-Schicht (myread/mywrite/myseek/myflush) ist C mit der Wortarithmetik des Originals; ohne Arbeit (Länge 0, Position im Puffer) kommt d0 unverändert zurück (FileSeek/Put/Get vergleichen dann die Dateinummer bzw. Feldadresse mit -1). Die Handler für inputoutputlib (FileInput/FileOutput) behalten die BRA-Tabelle vor der Einsprungadresse (-8/-4/0) als kurzen Assemblerblock, rufen C über Thunks. BUG(orig): Eof am Dateiende lässt die Pufferposition um 1 zu hoch stehen (der DOS-Seek schlägt fehl) – Loc/Lof danach +1 (t50: Lof 32 bei 31 Bytes; NPrint wertet von rechts aus, Eof zuerst); Exists = 0 auch für leere Dateien, -1 für Schubladen; CloseFile gibt die Fields frei, lässt Zeiger und Satzlänge stehen; Fields addiert, Put/Get übertragen in umgekehrter Reihenfolge; Lof flusht nicht; OpenFile auf eine offene Datei mit Fields liest die Namenslänge von -4(0) (nachgebildet, nicht getestet). DIFF: err1/err2/err3/err6 und die Meldung von err7 springt das Original aus dem normalen Code in den Fehlerprüfbereich (ohne Debugger abgeschnitten) – hier im normalen Code; a2, d4-d7, a3 bleiben unverändert. Tests: t50_fileio, t53_fileran, t54–t57 (tests-runerr).

## 106 cliargslib

C, Deklarationen identisch. Daten von wbstartuplib in a0. FromCLI lädt nur das untere Wort von d0 (Thunk). BUG(orig): keine Prüfung der Argumentnummer ohne Debugger; ParPath$ ruft vom CLI NameFromLock(0) auf (Lock 0 = Wurzel des Boot-Dateisystems; welchen Text dos.library dafür liefert, z. B. „SYS:“, ist nicht geprüft). DIFF: nach NameFromLock wird d0 = 0 geprüft statt des CCR von dos.library. Tests: t51_cliargs (der Korpus-Runner übergibt keine Argumente; mit Argumenten lokal geprüft), t58/t59 (tests-runerr).

## 76 banklib

C, Deklarationen identisch. Objekt-Lib „Bank“ (16 Bytes: pos, size, memtype; Maximum 5), kein Free. LoadBank fragt fileiolib Exists (ALibJsr `TOK_exists`, a6 = dos). InitBank speichert Größe/Typ vor dem Allozieren (ohne Folge im Original: memlib kehrt bei Speichermangel nicht zurück, erst mit der C-memlib, K); BUG(orig): FreeBank lässt memtype stehen; LoadBank alloziert neu bei anderem Typ oder wenn die Datei nicht kleiner ist (Vorzeichen der Differenz) und liest Bank-Größe Bytes; fehlende oder leere Datei: nichts. Test: t52_bank.

## 164 inputoutputlib

C, Deklarationen identisch, Binary = Quelle. Kanäle sind Handler-Adressen mit den Einsprüngen -8 (String mit 0), -4 (String a0/d0 bzw. Zeile lesen) und 0 (Zeichen); vor dem Handler stehen BRA.w. dosetin/dosetout (+3/+4) legen den alten Handler auf einen Ring von 16 Longs (Index & 63) und zählen; init legt auf beide Stapel den Handler 0. Die fünf Verteiler (+5…+9, `JSR off(a6)` mit a6 = Handler) bleiben Assembler, weil sie alle Register unverändert an fremde Handler weitergeben; der Rest ist C, dos über proto/dos.h. Default-Eingabe = `Read(Input(),a0,d0+1)-1` (Dateiende -1, Fehler -2). DIFF: Der Einzelzeichen-Einsprung von defin ist im Original `BRA err1` in den Fehlerprüfbereich (ohne Debugger ein Sprung hinter die Lib); in C stehen Trap und Meldung im normalen Code. BUG(orig): Meldung „Defualt“; mehr Pops als Pushes nehmen ohne Debugger alte Ring-Einträge, ein PopOutput direkt nach dem Start setzt den Handler auf 0. Tests: t41_io, t43–t45 (Laufzeitfehler).

## 187 printlib

C, Deklarationen identisch. `libincs:ffptoascii` fehlt auf der CD; die Routine ist aus dem Binary übersetzt ($658–$9D3: 7 Ziffern durch Subtraktion von Zehnerpotenzen, Tabelle 1E19…1E-28, Rundung über $80 unter der Mantisse). Variablen, Puffer und Tabelle liegen wie im Original hintereinander, damit Überläufe gleich ausgehen. Ausgabe über inputoutputlib (`TOK_iooutsingle`/`TOK_iooutstring1`); Str$/UStr$ setzen einen eigenen Handler (strprint, schreibt nach a3). FloatMode 0 = Festkomma für 1E-9 ≤ |x| < 1E7. BUG(orig) erhalten: Quick-Bruchteil, der auf 10000 rundet, verliert den Übertrag („2.“ für $0002FFFF), keine führende 0 (`.5`); Format verliert den Übertrag über die erste Stelle (999.96 mit „###.#“ → „000.0“), rundet Zahlen ohne Vorkommastelle (|x| < 1) unabhängig vom Format nie, ohne „-“/„+“ fällt das Vorzeichen weg, bei Überlauf „?“; Format über 32 Zeichen überschreibt formlen/formlead; FloatMode -1 schreibt bei Zahlen < 1E-17 hinter den Puffer; Str$ eines Strings gibt das untere Byte der Adresse aus, jedes Str$ legt zwei Einträge auf den Ausgabestapel. Der Compiler kennt Exponenten nur bis 18 (`1E-19` → „Numeric Over Flow“). DIFF: a2 bleibt unverändert. Test: t40_print (alle Typen, Randwerte, FloatMode -1/0/1/2/-5, 23 Format-Zeichenketten).

## 186 editlib

C, Deklarationen identisch, Binary = Quelle. Edit$ füllt den Default mit Leerzeichen auf n Zeichen auf und liest darüber (`TOK_iogetstring`). Edit benutzt einen eigenen Puffer von 80 Bytes, den Default per UStr$ (`TOK_ustr`) und das Ergebnis per Val (`TOK_val`), beide mit a2 auf dem Block `pars` {Länge.l, Typ.w = 5} (`BLITZ_ALIBJSR_A2A6`). BUG(orig): Inkey$(0) liefert als Länge das d6 des Aufrufers; bei Edit(Wert,n) verliert der Default durch das Auffüllen seine letzte Ziffer (UStr$ lässt a3 ohne 0 hinter der Ziffer, das `SUBQ #1,a3` ist für String-Argumente gedacht; Edit$ ist nicht betroffen); Edit ruft UStr$/Val mit dem a6 des Aufrufers (funktioniert nur, wenn a6 noch die ffplib-Basis ist); Edit(80) liest 81 Bytes; am Dateiende liefert DefaultInput die Länge -1. DIFF: a2 bleibt unverändert. Tests: t41_io (DefaultInput über einen Kindprozess mit umgeleiteter Eingabe), t42_edit (FileInput), t46–t49 (Laufzeitfehler).

## 74 crunchlib (`#romuluscrunchlib`)

C, Deklarationen identisch, Binary = Quelle. Imploder (Implode/Deplode), Crunch-Mania-Decruncher (CrM!/CrM2) und PowerPacker-Decruncher (PP20) registergenau übersetzt (Breiten byte/word/long, Schiebezähler mod 64, DBF auf dem Low-Word); zusätzlich host-seitig in unicorn (68020) gegen den Original-Code geprüft: Implode-Ausgabe, Deplode und alle Decruncher bytegleich auf 330+ Datensätzen und 400 verfälschten Eingaben. Implode läuft immer mit „Mode 8“ (kleinste Tabelle, Fenster 128), schreibt dabei COLOR00 und fragt die linke Maustaste ab (Abbruch → -1). BUG(orig): Implode-Suche liest bis zu 254 Bytes hinter die Daten (`im30` bis 15, Längenmessung `im32`/`im33` bis 254, nicht addiert; Ergebnis unabhängig davon); Ergebnis 0 lässt den Puffer teilweise überschrieben zurück; nach $4012 Literalen wird der Rest unkomprimiert gespeichert (Grenze des Imploder-Formats, kein Fehler); CrM-Überlappkopie mit CrLen<4 kopiert 4 GB, CrMDecrunch mit einem Argument überschreibt MinSecDist Bytes vor dem Puffer; ReadTab/ReadD1_SUB mit 0 laufen 65536-mal. DIFF: verfälschte CrM2-Daten können über die Tabellen hinaus in den Lib-Code greifen (Ergebnis abhängig vom Lib-Layout). Test: t60_crunch (Testdaten für PP20/CrM aus einem eigenen Encoder).

## 92 arexxlib

C, Deklarationen identisch, Binary = Quelle. BB2-ArgStrings (memlib, ra_Size mit Bit 31 = „von Blitz“), RexxMsgs (128 Bytes, ln_Name „REXX“). Fehler (Speicher, kein REXX-Port, keine rexxsyslib) zeigen einen recoverable Alert und liefern den Fehlercode (0/4/8). BUG(orig): ReplyRexxMsg prüft mit `BTST #17,28(a0)` (Speicher-BTST = Bit 25 = RXFUNC) statt RXFF_RESULT; ClearRexxMsg/FillRexxMsg ohne Grenze 16; Index (n-1)*4 als Wort (n=0 = rm_Action); GetRexxCommand/GetResultString lassen a3 hinter der 0, GetResultString lässt rm_Result2 auf den freigegebenen String zeigen; RexxError ohne abschließende 0 (unsichtbar, s. Allgemein); SendRexxCommand ohne REXX-Port: Alert bei gesperrten Interrupts. DIFF: ReplyRexxMsg mit einer Nicht-RexxMsg kehrt zurück (Original: Absturz); -8(a2) wird nicht nach -4(a2) kopiert. Test: t61_arexx mit eigenem Port „REXX“ (Priorität 100), RexxMast nicht nötig. Nicht getestet: echter ARexx-Host, Fehlerpfade (Alert wartet auf die Maus).

## 90 seriallib

C, Deklarationen identisch, Binary = Quelle. allio: 8 Units × 256 Bytes (Lese-Hälfte +0, Schreib-Hälfte +128, je Request-Zeiger, MsgPort, IOExtSer). BUG(orig): CloseSerial/SetSerialBuffer/SetSerialLens/SetSerialParams/SerialEvent rechnen unit*128, die anderen unit*256 (nur Unit 0 stimmig); ReadSerialMem liest über den Schreib-Request; WriteSerialString-Flag 2 = TrueString und DoIO zugleich (Flag 1 ungeprüft), ReadSerial „wait“ = Flag 2; ReadSerialString ohne 0 (unsichtbar, s. Allgemein); CloseSerial AbortIO ohne WaitIO, Schreib-Request bleibt; scheitert AllocSignal der Schreib-Hälfte, bleibt das Device offen; „Serial Device Not Open“ per TRAP #0 auch ohne Debugger. DIFF: ReadSerial lässt a3 unverändert. Test: t62_serial (Unit 0 ohne Gegenstelle), t63_serialerr in tests-runerr, t64_serialrx in `compilerlauf/corpus/tests-serial` mit Gegenstelle (`compilerlauf/tools/serialrun.py`: Amiberry `serial_port=TCP://…`, Antwortprogramm auf dem Host): ReadSerial ohne/mit Flag 1/mit Wartebit 2 (blockierend), ReadSerialString mit Daten, ReadSerialMem (über den Schreib-Request), SerialEvent 1/2/beide, WriteSerialString asynchron (Länge -1 sendet die abschließende 0 mit) und Adressform, SetSerialLens 7 Bit, SetSerialParams (Baud, EOF-Modus: wirkt nicht auf ReadSerialMem, da der Schreib-Request die alten Flags hat), SetSerialBuffer 64 mit Überlauf (Lesen liefert SerErr_BufOverflow 12). Ergebnis: Leitungsbytes in allen vier Konfigurationen gleich, Ausgabe gleich bis auf den DIFF a3 (zwei ReadSerial in einer Anweisung: im Original überschreibt das zweite Lesen den dazwischen in rb1 gebauten String-Teil, `97b98`; der Test liest jetzt getrennt, danach 0 Abweichungen). Nicht getestet: Units > 0 mit Gegenstelle, 7WIRE/Handshake, Parität, Break.

## 158 screenslib

C, Deklarationen identisch, Binary = Quelle. Objekt {Screen.l, DBufInfo.l}; eigene Screens bekommen UserData „bLtZ“, nur die schließt CloseScreen/Finit. Screen n,mode geht über einen falschen a2-Stapel (`dummydata`) in die lange Form; Breite auf 16 aufgerundet, ExtNewScreen mit SA_DisplayID/SA_Pens, Font = aktueller IntuiFont. ShowBitMap ab V39 mit ChangeVPBitMap, davor Copperliste/MakeVPort (nur portiert, nicht ausgeführt). Inline bleiben ViewPort, Show/Hide/Beep/MoveScreen, SMouseX/Y. BUG(orig): freescreen wird mit a2 = Argumentstapel als Graphics-Basis aufgerufen (mit DBufInfo ruft FreeDBufInfo ins Leere); FindScreen/WbToScreen geben vorher das Objekt frei (ein gefundener Blitz-Screen wird geschlossen); ScreenTags kopiert eine Tagliste ohne Grenze in 32 Longs; SA_DisplayID bekommt nur das untere Wort des Viewmode; err1 springt aus dem normalen Code in den Fehlerprüfbereich (nachgebildet). Tests: t81_screen, t86–t89.

## 142 fontlib

C, Deklarationen identisch, Binary = Quelle. BlitzFont = TextFont-Zeiger, nur 8x8 und nicht proportional; Init lädt topaz 8 in Objekt 0. BitMapOutput zeichnet über Handler (-8/-4/0) ins Bitmap; BitMapInput ist ein Zeileneditor auf dem Bitmap (`doinput1` bleibt Assembler, springt zum alten Handler). BUG(orig): Locate y ist Quick, CursX/CursY = Cursor/8 als Quick; Zeichen > LoChar+127 fallen weg; Tiefe 0 lässt die Plane-Schleifen 65536-mal laufen; ein falscher Font wird nicht geschlossen; BitMapInput wartet ohne Ende auf 13. DIFF: kehrt der Trap in calcoff zurück, zeichnet C nichts; das unbenutzte `bitmapchk` fehlt. BitMapInput ist nur im Blitz-Modus erlaubt und nicht getestet. Tests: t82_font, t84.

## 134 intuifontlib

C, CD-Binary älter als die Quelle (Token `Text` fehlt) – portiert ist der Stand des Binarys. Objekt = TextAttr + Font + Pad (16 Bytes); Init-Wert = Adresse von `textattr`. LoadFont kopiert den Namen mit maxslib und ruft OpenDiskFont; `loadchk` prüft das CCR (Assembler-Zeile, Thunks setzen Z). BUG(orig): Stil geht als Wort nach +6 (LoadFont …,9,1 → style 0, flags 1); die alte Namenskopie wird nie freigegeben. Tests: t82_font, t85.

## 130 rawkeylib

C, Deklarationen identisch. `libincs:rawcode` fehlt auf der CD; alles außer unconvert ist aus dem Binary übersetzt. Init öffnet console.device und füllt zwei Tabellen (105 Einträge); F1–F10 → 129–138, Cursortasten → 28–31, Help → 139, sonst RawKeyConvert. BUG(orig): qconvertkey indexiert mit Code & 255 über die Tabellen hinaus in den eigenen Code (die 151 Code-Bytes des Originals sind als Daten angehängt). Kein eigener Laufzeittest (ein direkter ALibJsr-Aufruf aus BASIC hing schon mit den Original-Libs); indirekt über windowslib (Tasten, Edit$ in t72) geprüft.

## 138 windowslib

C, Deklarationen identisch (75 Tokens). Objekt-Lib „Window“ (32 Bytes), ein gemeinsamer MsgPort für alle Fenster, eigene Ereignisliste (24-Byte-Knoten). Eingabe-/Ausgabe-Handler für inputoutputlib mit BRA-Tabelle (-4 Zeileneditor, -8/-4 Strings) als kurzer Assemblerblock, sonst C. BUG(orig): Menus/MenusOn/MenusOff gehen in 16-Byte-Schritten über die 32-Byte-Objekte; String-Gadget-down erzeugt ein zusätzliches „Gadget up“; MENUPICK hängt ein weiteres Ereignis für NextSelect an (Qualifier/IAddress uninitialisiert); Event vergleicht Klassen als Wort; Qualifier (#long) setzt nur das untere Wort; WCls ab y=1; WindowTags behält alte Usertags, &TagList ohne Grenze in 32 Longs; Titel werden nie freigegeben; WMove/WSize warten aktiv; CloseWindow verwirft die Ereignisse aller Fenster. DIFF: err6/err7 aus normalem Code; a3, a4, d7 und der Argumentstapel bleiben unverändert. Tests t70–t72 (Ereignisse als selbst gebaute IntuiMessages vor dem Warten, Edit$ mit vorab eingereihten Tasten und Timer-Watchdog), t75/t76/t78/t79. Nicht getestet: echte Intuition-Eingabe, SetEventFilter, Befehle mit Shapes/Bitmaps, Fenster mit Gadgetlisten.

## 136 menuslib

C, Deklarationen identisch. Objekt-Lib „MenuList“ (4 Bytes). BUG(orig): SubItem-Ersetzen verschiebt die folgenden Unterpunkte um (Nummer − alte Höhe); MenuState auf ein Menü setzt Bit 4; MenuChecked mit Unterpunkt springt in die Menüschleife zurück (nur 0/0 korrekt); ShapeItem-Flags = oberes Wort des Langworts; Flag-Test als Wortvergleich; ein Shortcut setzt Flags vor dem ODER auf 4; MenuTitle-Breite = 8·Länge + 8. Tests t73, t71, t77.

## 108 mouselib

C, Deklarationen identisch. Fast alles nur im Blitz-Modus (err1 = 1). BUG(orig): Mouse On vertauscht die JOY0DAT-Bytes; der Interrupt lässt a5 stehen; die Sprite-Nummer von Pointer ist Objekt-Offset >> 4. Test t74 (Blitz-Modus ohne Mouse On; Interrupt und Pointer mit Sprite nicht testbar).

## 162 iffiolib

C, Deklarationen identisch, Binary = Quelle. Keine eigenen Tokens; Routinen für andere Libs (ilbmifflib, animlib, audiolib, palettelib, Lib 154): +1 iffopen (a0 = Name), +2 iffclose, +3 iffbyte, +4 iffword, +5 ifflong, +6 ifffind (d0 = ID → Chunklänge), +7 iffskip (d0 = Anzahl **Bytes**, die Quelle sagt „long words“), +8 iffread (d0 Bytes nach a0). init (a0 = dos) holt den 2048-Byte-Puffer per globalloc, Init-Wert = Pufferadresse. Ergebnis im CCR (Z = ok, sonst d0 = -1/N), iffbyte/iffword ersetzen nur d0.b/d0.w; die Thunks setzen das CCR per `tst.w` aus einer Statusvariablen. Alle Register außer d0 (iffread: a0) bleiben erhalten. BUG(orig): charsleft = unteres Wort des Read-Ergebnisses (Lesefehler → 65535, Pufferinhalt gilt als Daten); ifffind bricht bei „BODY“ ab und liest byteweise; iffskip/iffread mit 0 zählen ab 2^32; iffread liest ab Pufferende per Read direkt nach a0 (a0 nicht weitergezählt). Tests indirekt über t100 und t103.

## 161 ilbmifflib

C, Deklarationen identisch, Binary = Quelle. Für andere Libs: +1 ilbminit (a0 = Name → d0 = -1/0, a0 = iffstuff, Datei bleibt offen), +2 ilbmgetbyte (ByteRun1-dekodiert), +3 ilbmbitmap (a0 = Name, a1 = BitMap). ViewMode: pageWidth ≥ 480 → $8000, pageHeight ≥ 300 → |4. BUG(orig): ILBMInfo schließt die Datei auch, wenn sie nicht geöffnet wurde (Close(0)); ilbmbitmap ignoriert ein fehlgeschlagenes ilbminit und vergleicht die Ebene mit dem Wort BitMap+4; DecodeILBM rundet Chunklängen nicht auf, schreibt alle Ebenen des ILBM über die Ebenenzeiger der BitMap (auch hinter Planes[]), beschneidet ByteRun1-Läufe nicht am Zeilenende. Fehlerroutinen im normalen Code, Meldungen im Fehlerprüfbereich (wie im Original). ILBMWidth/Height/Depth laden nur d0.w. Tests: t100, t103.

## 114 iffmakelib

C, Deklarationen identisch, Binary = Quelle. Die vier Befehle sind nur im Amiga-Modus erlaubt: ungerade Codeadresse in !subs (blitz2.s doajsrrout), in C `AMIGAONLY()`. ILBM: BMHD, CMAP (obere Bytes der 32-Bit-Werte), BODY je Zeile und Ebene ByteRun1, Längen per Seek nachgetragen. SaveScreen: Exec ≥ 39 → GetRGB32 je Farbe, sonst ColorTable 4 Bit → 32 Bit. BUG(orig): Ebenenschleife vergleicht mit dem Wort BitMap+4; Zeilen-/Ebenenschleifen laufen mindestens einmal; savearow liest ein Byte hinter die Zeile; CMAP mit ungerader Länge ohne Füllbyte; Palette mit 0 Farben → 65536 Farben; „Can't open File“ im normalen Code mit Meldung im Fehlerprüfbereich. Tests: t100, t104, t107.

## 132 brexxlib

C, Deklarationen identisch, Binary = Quelle. init öffnet input.device und hängt einen Input-Handler mit Priorität 51 ein (C-Funktion, ruft memlib globalloc aus der input.device-Task wie das Original). Tapes = Listen von InputEvents (22 Bytes). BUG(orig): TapeTrap ohne Nummer fällt in TapeTrap Tape# durch; Anhängen an ein Tape löscht mode; ClickButton übergibt dem „Loslassen“ das d0 des ersten Aufrufs (unter TapeTrap mit QuietTrap On „links drücken, RECHTS loslassen“, im Original nachgewiesen); AbsMouse und RelMouse erzeugen beide POINTERPOS; PlayWait startet das Tape neu; Type: unbekanntes Zeichen → Code $FFFF; XStatus enthält Bit 15. DIFF: „Can't Open File“ (SaveTape/LoadTape) im normalen Code statt Sprung in den Fehlerprüfbereich. Tests: t101 (Ereignisse nur auf Tapes), t106.

## 117 blitzoslib

C, Deklarationen identisch, Binary = Quelle (Code von Hand mit Quelle und Disassembly verglichen). Fehlerprüffeld `1` in !subs = nur im Blitz-Modus; im Amiga-Modus erlaubt sind nur InitPage, FreePage, AddBox, SetBox Box#,Value, SetBoxString, SetPagePens, ChangeBox. `!libs …,$0604` = d6 = Offset der Objektvariablen + 4. Zeichnen über BLibJsr an 2dlib, fontlib, scrolllib, Shapes, mouselib, blitzkeyslib (Makro BCALL). BUG(orig): Box-Nummern per ASL.w #5/ADDA.w; SelectBox Box# ungeprüft; SpotBox ohne Seitenprüfung; String-Box mit 8–15 Pixel Breite füllt 65536 Leerzeichen; BoxString ohne Grenze rückwärts, ohne 0 (unsichtbar, s. Allgemein); BoxPrint überschreibt die String-Länge -4(a2) mit der Scroll-Größe (folgenlos, der Argumentbereich gehört nur zu diesem Aufruf); DeSelectBox löscht `selected` nicht. DIFF: Koordinaten an Boxf/Line/Locate als vorzeichenerweiterte Wörter. Kein Emulatortest: mit der Original-Lib stürzten ein Blitz-Modus-Test und ein Fehlerpfad-Test ab; Ursache nicht gefunden.

## 116 audiolib

C, Deklarationen identisch, Binary = Quelle (der VBlank-Code zwischen den beiden CELSE ist nicht im Binary). Sound-Objekt {Daten, Periode, Länge in Worten, Repeat-Position, Repeat-Länge, „cL“}. Alles geht direkt an Paula; Sound/LoopSound schalten den DMA ab und warten 12 Zeilenwechsel von VHPOSR. IFF 8SVX liest iffiolib (Fehler im CCR, lokales Makro `IFFCALL`). Periode = 3579440 / samplesPerSec mit DIVU (Überlauf bleibt stehen). BUG(orig): sndchk prüft bei SetPeriod die Periode als Kanalmaske (Periode > 15 → „Illegal Chanel Mask“ mit Debugger, t115); LoadSound mit zu langem Sample lässt die Datei offen; eine „cL“-Marke aus DecodeSound bleibt bei späterem DecodeSound in Fast-RAM stehen; DiskPlay endet nie, wenn die Dateilänge ein Vielfaches der Pufferlänge ist; Sprünge nach err5 aus dem normalen Code gehen wie im Original in den Fehlerprüfbereich.

**Compiler/Lib-Zusammenspiel (BUG(orig), für AmiBlitz3 wichtig):** Objekt-Funktionen wie PeekSound laden über `!libs …,$1380` das Objekt nach a3 – a3 ist aber zugleich der String-Zeiger. Steht so ein Aufruf mitten in einem String-Ausdruck (`s$=s$+Str$(PeekSound(0,i))`, `NPrint "x",PeekSound(..)`), schreibt der Rest des Ausdrucks seine Strings ins Objekt (Original: „Unable to Free Memory“, ohne Debugger Guru 8000 0020). Tests verwenden solche Funktionen nur in einfachen Zuweisungen. DiskPlay nicht getestet (wartet aktiv auf Audio-Interrupts). Tests: t110, t114, t115, t119 (nur Nulldaten bzw. Lautstärke 0).

## 96 trackerlib

C, Binary = Quelle. NoiseTracker-Player (mt_init/mt_music/mt_end) in C; mt_music im VBlank-Interrupt über intlib $C100/$C101 (Blitz/Amiga-Variante). Assembler bleiben die zwei DBF-Warteschleifen und die Prefixe. mt_init löscht das erste Long jedes Samples. BUG(orig): mt_init prüft die Positionen 0–126 (127 nie); ein neues Maximum an Position 126 lässt den Zähler von 0 auf $FFFF laufen (Suche ~64 KB weiter, `CLR.l` hinter das Modul); Lautstärke-Wort enthält das Finetune-Byte; Repeat × 2 als Wort; nach D/B wird das Songende mit dem ersten Pattern-Byte verglichen; Cxx schreibt die Lautstärke als Byte; Sample-Nummern > 31 lesen hinter der Tabelle; LoadModule einer fehlenden Datei gibt den FileInfoBlock nicht frei, bei einer Schublade zusätzlich den Lock nicht. Tests: t111 (Prüfsummen vor/nach mt_init, Play/Stop sofort), t116, t117.

## 82 medlib

C, Binary = Quelle (MED-V3-Player von T. Kinnunen; SYNTH, RELVOL, IFFMOCT, HOLD, PLAYMMD0; kein Interrupt, PlayMed = ein Takt), auf Registerebene übersetzt (d0–d7/a0–a5 als statische Variablen). Datenbasis, 16 Periodentabellen und zwei Sprungtabellen aus dem Binary im Original-Layout. BUG(orig): Bxx speichert das ganze Wort von d4; 7xy mit x = 0 springt in den Vibrato-Code; bei 1Fxx geht das Retrigger-Nibble verloren; Synth-Periodengrenze setzt d1 statt d5; Relocation liest nach einem MIDI-Dump den nächsten Modulzeiger von (Expansion-Adresse AND $FFFF0000); pushnewvals mit lückenhafter Kanalmaske: höhere Kanäle bekommen keine Folgewerte (Track/Bit bleiben gepaart); Tracks 4–15 schreiben über ihre 20-Byte-Struktur hinaus; JumpMed ohne Modul schreibt nach Adresse 44–47 (Exception-Vektor 11); GetMed* lesen und löschen. DIFF: LoadMedModule und SetMedVolume ändern a3 nicht. Tests: t112 (lambada.med von der CD, 700 Takte), t118.

## 88 speaklib

C, Binary = Quelle. narrator_rb byte-genau (88 Bytes, V37-Teil an ungerader Adresse = VoiceLoc). Fehlt translator.library oder narrator.device, gibt es auch ohne Debugger „Failed To Open Speech Library“ (TRAP im normalen Code). BUG(orig): Translate$ schreibt die Phoneme über das Argument (Länge mit abschließender 0, a3 dahinter); PhoneticSpeak lässt a3 auf dem Argument; Finit schließt Device und Signal auch nach fehlgeschlagenem Init; SetVoice begrenzt mit Wortvergleichen. Im Emulator (OS 3.2) fehlen translator.library und narrator.device → t113 (tests-runerr) prüft nur den Init-Fehler; Translate$/SetVoice/VoiceLoc nicht vergleichbar.

## 140 gadgetslib

C, Deklarationen identisch, Binary = Quelle. Objekt-Lib GadgetList (4 Bytes), Gadgets/IntuiTexts/Borders/StringInfo/PropInfo aus memlib, Texte über maxslib. Host-Vergleich (unicorn) gegen das Original: OS-Aufrufe und Heap bytegleich; t90_gadgets in allen vier Konfigurationen gleich. BUG(orig): freegadget gibt SpecialInfo nie frei (Typ durch `MOVEQ #22` überschrieben); StringGadget bei Borders Off schreibt nach 0/2/36/38; Maxlen 0 löscht 64 KB; GadgetStatus/ButtonId mit fremdem oberem Wort; findgadget ohne Endeprüfung.

## 141 gtlib (`#mygadtoolslib`)

C nach dem CD-Binary (Quelle ist ein anderer Stand; DetachGTList, GTGimmeCludge, GTSetHighlight, GTFreeGadget nur im Binary). Tag-Listen statisch im Original-Layout, daher gleiche Eigenheiten: GTPalette-Farbe landet im TAG_MORE-Zeiger, optionale Werte bleiben stehen, GTBevelBox nie vertieft, GTShape überschreibt das GadTools-Button-Image. **GTGetAttrs/GTSet…/GTChangeList/GTSetHighlight lassen a3 auf ihrer Tag-Liste stehen** – Strings danach in derselben Anweisung überschreiben sie (BUG(orig), C gleich). Host-Vergleich identisch; kein Emulator-Test: der Original-Lauf stürzte am Programmende ab (Guru 8100 0005), Ursache nicht gefunden.

## 80 asllib (`#myasllib`)

C, Deklarationen identisch; Binary weicht von der Quelle ab (zusätzlicher Font-Tag ASL_TB+17=256, Pattern "" statt 0, Freigabe mit FreeFileRequest -36) – Binary ist Referenz. BUG(orig): ASLScreenRequest liefert den freigegebenen Requester; ASLPathRequest$ kopiert fr_File an die Adresse in d2; ohne asl.library ist die Länge die Titeladresse. Host-Vergleich identisch; kein stabiler Emulator-Test (Schließ-Task im BASIC-Test nicht zuverlässig).

## 152 freqlib

C: Wrapper aus der Quelle, der Requester (libincs:filereq.s, nicht auf der CD) aus der Disassemblierung; seine Daten als Datenblock mit den Original-Offsets (Bytes + Relocs aus dem Binary), inzwischen als `struct fq_data fq` (NDK-Strukturen, Variablen, Puffer; jeder Offset per `_Static_assert` gegen die Adresse im Binary geprüft; Datenblock bytes- und relocgleich, Code kürzer, weil GCC den Assembler-Block als Byte-Array mit Ausrichtung 1 sah; Host-Vergleich unicorn 27 Szenarien gleich). **Aufteilung (für TED, „Variante C“):** der Requester-Kern (Code, Datenblock `fq`, Parameterblock `freqst[4]`, Einsprung `freqcode()`) steht in `libs/freqlib_req.inc`, alles `static`, ohne blitzlib.h; `freqlib.c` ist nur noch die BlitzLib-Hülle (`dofreq`/`freqchk`) und definiert vor dem `#include` `fq_divu_w` (= `bl_divu_w`). Dieselbe Datei bindet `../ted-c/src/filereq.c` ein (TEDs eingebetteter Hunk `filereq` ist derselbe Code). Was der Einbinder vorher bereitstellen muss, steht am Kopf der .inc. `freqlib.obj` ist nach der Aufteilung bytegleich (`cmp` gegen den Stand davor; `biglib.py compare` mit ref: Deklarationen identisch). Host-Vergleich in 7 Szenarien identisch. BUG(orig): path$/file$ ohne neue Länge, Libraries nie geschlossen, Datenträgerliste ohne Sperre der DOS-Liste (die Beschriftungen <ASN>/<VOL> sind richtig: `dl_Type` 1 = DLT_DIRECTORY → „<ASN>“, 2 = DLT_VOLUME → „<VOL>“), CD-Gadget ändert das aktuelle Verzeichnis und löscht das obere Wort des UndoBuffer-Zeigers des Pfad-Gadgets ($111A statt DispPos), ParentDir-Locks bleiben offen. Der FileInfoBlock muss langwort-ausgerichtet sein (BPTR) – in C ausdrücklich ausgerichtet (der erste Emulator-Lauf hing deshalb). BUG(orig): das Fenster zeigt (Höhe − 85) / 8 Zeilen ohne Grenze, es gibt aber nur 64 Zeilengadgets (bis zum Ende des Datenblocks $39DA); ab Fensterhöhe 605 (erstes Fenster = Screenhöhe − 40, also jeder Screen ab 645 Zeilen) schreibt das Layout die weiteren Zeilen hinter den Datenblock in das folgende Programm. Im Host-Harness gemessen (Screen 1050: Fenster 1010, 115 Zeilen, ~7,2 KB hinter $39DA; Screen 256: nichts); Stack, a2–a6, pr_WindowPtr, Locks und Speicher sind beim Rücksprung in Ordnung. Auf der RTG-Workbench (1680×1050) des Testemulators stürzt das Programm daher direkt nach dem Requester ab (Abbruch wie OK, t93/t94 mit reqclose: Reboot, früher „ILLEGAL Instruction“). Auch Namen über 34 Zeichen überschreiben ihre 40-Byte-Listenknoten. Tests t93 (Abbruch) und t94 (OK) in `compilerlauf/corpus/tests-req` über reqclose, deshalb auf einem eigenen 320×256-Screen (Fenster 216, 16 Zeilen; im Harness ohne Überlauf, Abbruch wie OK).

## 160 bitmaplib

C, Deklarationen identisch, Binary = Quelle. Objekt-Lib „BitMap“ (64 Bytes: struct BitMap + Zusatzfelder +52 interleaved, +54 Ebenenabstand, +56 Breite, +58 Höhe, +60 Farben, +62 0/1 geliehen/-1 eigen). BUG(orig): Größen als MULU-Wortprodukte; CludgeBitMap lässt den Ebenenabstand stehen (keine Library der CD liest +54 einer nicht-interleaved Bitmap); CopyBitMap ohne Used-Setzen, Chip-Speicher ungelöscht. Tests t120, t124, t125.

## 156 palettelib

C nach dem CD-Binary (neuer als die Quelle: Red/Green/Blue #long, make4bitrgbs nimmt das untere Byte, AGARed/Green/Blue prüfen den Screen und maskieren die Farbnummer). Objekt-Lib „Palette“ (128 Bytes, Daten im LoadRGB32-Format, 14 Zyklen). Blitz-/Amiga-Varianten ($a001). RGB im Amiga-Modus springt wie das Original direkt in SetRGB4 (a1 unverändert): mit a1 = ViewPort speicherte die graphics.library des Emulators für Grün 8 den Wert $8C statt $88 (t122, Diagnosezeile „raw“) – ein Beispiel, wie ein „unbenutztes“ Register das Ergebnis eines OS-Aufrufs ändert. BUG(orig): FadePalette/MixPalette lesen 2 Bytes zu früh; DecodePalette setzt nur das obere Byte; PaletteRange mit Start=Ende teilt durch 0; StopCycle setzt cycon nicht zurück; InitPalette ohne Zyklus-Endmarke. DIFF: Laufzeitfehler aus normalem Code statt Sprung in den Fehlerprüfbereich. Tests t121–t123, t126–t129.

## 157 fadelib

C, Deklarationen identisch (`libheader{157`). Nur Blitz-Modus; 16 Schritte auf den Copper-Farbmoves der benutzten Slice. BUG(orig): `calcfade` schiebt mit `LSR #4` das ganze Wort d0 (beim Aufruf Palette# × 128, die Objektadresse des Compilers): Bits 8–11 landen im Rot-, Bits 12–15 im Grün-Bruchteil der **ersten** Farbe, danach ist das obere Byte leer (Palette 0/1 nicht betroffen; seit der Korrektur registergenau nachgebildet, vorher bekam in C jeder Bruchteil die Bits); die Prüfungen testen das obere Wort der Zeiger. Test t123 (ASyncFade On + DoFade, ohne Timing; nur Palette 0, deckt den Fehler nicht ab).

## 150 blitlib / 146 bblitlib / 148 qblitlib / 122 stencillib

C, Deklarationen identisch, Binaries = Quellen (+ Include bliterrs). Die Routinen programmieren den Blitter direkt (volatile Zugriffe in Reihenfolge und Breite des Originals); Blitz-/Amiga-Varianten ($a001): Amiga-Modus mit OwnBlitter/Warten/DisownBlitter. bblitlib sichert den Hintergrund in Datensätze im Buffer (UnBuffer, ShowStencil über Stencil als B), qblitlib merkt sich die Blits (UnQueue löschen bzw. aus Bitmap kopieren), stencillib = ODER aller Ebenen bzw. Cookie dazu (SBlit). BUG(orig): kein Clipping (nur die Prüfungen), Tiefe 0 = 65536 DBF-Durchläufe, Block auf Bytepositionen, BlitColl über alle Bitmap-Ebenen, Überlauf von Buffer/Queue nur in den Prüfungen, Free… lassen Zeiger stehen. DIFF: b_doblitex gibt seinen Registerzustand nicht an andere Libs zurück; BlitColl im Amiga-Modus mit oberem Wort 0. Tests t130–t134 (t134 im Blitz-Modus), t135–t139 (tests-runerr). Hinweis Testbau: `BitMap n,…` macht die neue Bitmap zur benutzten – ein BBlit mit mehr Ebenen als die Bitmap schreibt ohne Fehlerprüfung über den Ebenenzeiger 0 in den unteren Speicher.

## 154 shapeslib

C, Deklarationen identisch, Binary = Quelle (inkl. ANIM-Lader). Shape-Block 32 Bytes (+0 Breite … +14 Daten CHIP, +18 Cookie, +22/+24/+26/+28 Größen, +30 "cL" = Daten nicht eigen). GetaShape (Amiga: OwnBlitter um die Blitz-Routine) lässt bitmaplib eine Bitmap anlegen und kopiert per scrolllib hinein. LoadShape liest ILBM über ilbmifflib/iffiolib; jedes "ANHD" (Op 5, Interleave 1) liefert ein Delta-Frame im NÄCHSTEN Shape-Objekt. BUG(orig): MULU-Low-Words und ADDA.w-Schritte; BODY ungeprüft; ANIM-Frames ohne Maximum-Prüfung, XOR-Bit nur aus dem ersten ANHD, nur `depth` DLTA-Zeiger; "cL" wird von CopyShape/LoadShapes übernommen und von freeashape nicht gelöscht (nie freigegeben); DecodeShapes stoppt still beim ersten unpassenden Block, unter 2 MB in place; DecodeShapes mit Adresse 0 springt in den Fehlerprüfbereich (err10); ShapeWidth/Height laden nur d0.w. Tests: t140, t144 (ANIM im Test erzeugt), t145/t146/t149.

## 118 shapetrixlib

C, Deklarationen identisch, Binary = Quelle; `libincs:qrot` fehlt auf der CD, die Sinustabelle stammt aus dem Binary. XFlip schiebt jede Ebene mit dem Blitter und spiegelt dann Bytes und Bits per CPU; YFlip tauscht Zeilen; Rotate (Quick 0..1 = 256 Schritte, Rückrotation jedes Zielpixels); Scale (Größe per lmullib, Schritt per ldivlib, je Zielpixel die Farbe mit der größten Helligkeitssumme). BUG(orig): XFlip wartet nach dem letzten Blit nicht, bevor die CPU spiegelt – das Ergebnis hängt vom Timing ab (Blitter gegen CPU); mit dem schnelleren C-Code unterschied sich eine 1-Wort-Shape, dieser Fall ist aus t141 entfernt, die C-Fassung wartet wie das Original nicht; Helligkeitstabelle behält alte Werte (folgenlos, die Farbwahl vergleicht nur 0..numcols); Scale liest am Rand über die Shape hinaus. Test: t141.

## 112 spriteslib

C, Deklarationen identisch, Binary = Quelle. Sprite-Block 16 Bytes; SpriteMode 0/1/2 = 16/32/64 Pixel. ShowSprite/InFront/InFrontF/InFrontB nur Blitz-Modus (ungerade Fehlerprüfadresse), nicht getestet. BUG(orig): Größen beim Freigeben/Speichern mit dem AKTUELLEN SpriteMode (harmlos, memlib nimmt die gespeicherte Länge); SpriteMode > 2 liest hinter die Breitentabelle; GetaSprite liest ganze Sprite-Breiten; ShowSprite schneidet oben mit 4 Bytes je Zeile ab und addiert das ganze Long-Register. Tests: t142, t147.

## 104 collslib

C, Deklarationen identisch, Binary = Quelle. ShapesHit/ShapeSpriteHit/SpritesHit/RectsHit: Rechtecktest mit vorzeichenbehafteten Wortvergleichen. SetColl/SetCollOdd/SetCollHi schreiben CLXCON und lesen CLXDAT. SColl/PColl/DoColl nur Blitz-Modus, nicht getestet. BUG(orig): SColl rechnet die CLXDAT-Bitnummer (BTST aufs obere Byte, Bits 9–14 = Sprite-Paare, gewollt) nur richtig, wenn Kanal 0/1 nicht das zweite Argument ist: SColl(2,0)/(4,0)/(6,0) testen Bit 10/11/12 statt 9/10/11, zwei Kanäle desselben Paars ein fremdes Bit; SetColl mit 0 Ebenen läuft 65536 Mal. Tests: t143, t148.

## 100 scrolllib / 119 2dlib / 121 polylib

C, Deklarationen identisch mit den CD-Binaries. scrolllib = Quelle bis auf clsnotint (Binary: Long-Operationen beim Aufteilen der Ebenengröße, übernommen). 2dlib = Quelle `2dlib` (nicht sis2dlib.bb). polylib = Quelle polylib (#chunkylib=121; renderlib ist dieselbe Datei mit 87). 2dlib und polylib sind auf Registerebene übersetzt (statische d0–d7/a0–a6, Operandengrößen wie im Original, Vergleiche mit 68000-Flag-Semantik inkl. Überlauf). Blitter-Routinen sind Blitz/Amiga-Varianten ($A001); polylib besitzt den Blitter nie (wie das Original). Die Füllschleife von FloodFill (yofill) ist das Original-Assembler, weil sie die offenen Zeilen unbegrenzt auf dem 68000-Stack hält. Alle Routinen hinterlassen a3 wie das Original.

BUG(orig): Box addiert den Bitmap-Ursprung doppelt; Circle/Circlef mit Punkten über dem oberen Rand: MULU des negativen y, Plot weit außerhalb; Poly mit 1 Punkt: 65535 Durchläufe; Polyf/BitMapPoly: Überlauf bei mehr als 16 Punkten bzw. mehr als 10 Kanten pro Seite; die Statement-Form von AllocFill nimmt als Bitmap a3 = String-Puffer; FreeFill lässt den Pufferzeiger stehen; Tiefe 0: 65536 bzw. 256 Ebenen-Durchläufe; Scroll schreibt die Breite des 960er-Splits auf den Argumentstack zurück; linker/oberer ClipBlit- bzw. rechter Scroll-Rand: der Blitter liest/schreibt ein Wort vor/hinter der Zeile unverändert zurück; polylib: keine Orientierungsprüfung, RenderToBitmap b,d + RenderPoly vertauscht die Bitmaps, RenderPoly ohne RenderTo… zeichnet auf a3 = String-Puffer, RenderToChunky-Farben > 14 lesen hinter der Farbtabelle, ChunkyVlin ohne CLIP mit y2 < y1 schreibt etwa 64K Wörter. DIFF (Speicherlayout, unvermeidbar): was ein Überlauf von polypts/l1 trifft (Original: eigener Code, C: die Daten dahinter). Tests: t150–t157, t158/t159 (tests-runerr). Testbau: Kreise nie über den oberen Rand, Polyf ≤ 16 Punkte, steile Kanten in polylib (DIVS-Überlauf). Nebenbei: t110–t112 sind jetzt stumm (Samples nach den Prüfsummen genullt).

## 190 gameiolib

C, Deklarationen identisch, Binary = Quelle. Joyx/Joyy/Joyr/Joyb/Gameb lesen JOY0DAT/JOY1DAT, CIAA PRA und POTGOR direkt (Amiga- und Blitz-Modus); Joyb testet mit BTST auf das Byte $DFF016 (POTGOR Bit 10/14). BUG(orig): Gameb Port 0 springt per DBF in die Schleife von Port 1 und verlässt sie über deren Ausgang (POTGO $3000 statt $0300); Joyr behält das obere Wort von Joyys MOVEQ. Test: t160 (ohne Eingabe; Port 0 gegen eigene Dekodierung von JOY0DAT, Gameb mit Disable), t165. Nicht getestet: echte Eingaben.

## 98 blitzkeyslib

C, Deklarationen identisch (Binary hat 6 unbenutzte Bytes mehr). Init-Wert = keyboard.device-Basis; Matrix ab $136 (V36+) bzw. $106, Tastenpuffer +$70/+$72/+$74. BUG(orig): BlitzKeys Off setzt nur das Flag zurück (Handler bleibt); ison per NOT; Eintrag -8 des Handlers ist kein BRA; Wortergebnisse behalten das obere Wort von d0. Test: t161 (BlitzQualifier, RawStatus 0–127, BlitzRepeat, HandleRawKey). Nicht getestet: BlitzKeys (Blitz-Modus), BlitzKey (schreibt den Leseindex von keyboard.device).

## 94 animlib

C, Deklarationen identisch, Binary = Quelle. Objekt „Anim“ (32 Bytes), BODY und DLTA-Knoten über memlib, Datei über ilbmifflib/iffiolib. BUG(orig): BODY immer als ByteRun1; freeanim liest den Knoten nach dem Freigeben; Umlauf auf Delta 2; „Can't Open Anim File“ per BEQ in den Fehlerprüfbereich, err6 läuft in err1 weiter; a3 bleibt auf dem letzten Knoten bzw. hinter den Daten (nachgebildet). DIFF: a2 unverändert; err6/err7 per JSR statt Bcc (ein Inline-BRA mit `__builtin_unreachable()` ließ GCC die Pfade verschmelzen). Test: t162 (selbst erzeugtes ANIM in RAM:, Ergebnis = Host-Modell), t168.

## 144 blitzcoplib

C, Deklarationen identisch mit dem Binary (Quelle hat `Help` zusätzlich, CNIF #ddftest=1). Slices bauen eine gemeinsame Copperliste; Slice/ColSplit/CustomCop als $A001-Routinen. BUG(orig): unbenutzte Slice-Zeiger werden mitverschoben; leerer CustomCop-String kopiert 2^32 Longs; hwait als Wort, benutzt wird das obere Byte. Test: t163 (Amiga-Modus, Copperliste mit maskierten Planezeigern), t166. Nicht getestet: ShowBlitz, DisplayOff/On, Blitz-Varianten.

## 143 displaylib

C, Deklarationen identisch, Binary = Quelle. Objekt „CopList“ (128 Bytes), Moduswerte aus der Tabelle (Auflösung × Fetch, 64 Bytes je Eintrag). BUG(orig): alle Fehler sind Sprünge aus dem normalen Code in den Fehlerprüfbereich (per JSR nachgebildet); Init schreibt die mysprite-Adresse in die SPR0POS/CTL-Daten; DisplayBitMap ohne x,y speichert d2/d3 des Aufrufers in x0/x1; DBF/Wortarithmetik, CMPA.l vorzeichenbehaftet; angehängte Sprites mit ungerader Kanalzahl überschreiben Speicher; CustomSprites lädt per `!libs{#spriteslib,$1181}` das Sprite-Objekt mit der Nummer aus d1 (= CCOffset) – der Compiler verschiebt d1 dabei um den Objekt-Shift (×16), CCOffset wirkt 16-fach und muss eine gültige Sprite-Nummer sein (Maximumprüfung); ohne Debugger führt der Überlauf über den Sprung in den Fehlerprüfbereich irgendwohin (im Test gefunden: Original lief zufällig weiter, C hing); a3 bleibt auf bp1 bzw. der letzten CopList. DIFF: a2 unverändert, Tabellenindizes jenseits der Daten (Original liest seinen eigenen Code). Test: t164 (alle Befehle außer CreateDisplay), t167, t169. Nicht getestet: CreateDisplay (Blitz-Modus).

## Nebenbei

* `tests/systest_system.c`: der Log-Eintrag "trap N handler" enthielt das
  obere Wort einer absoluten Adresse in der Lib-Kopie und hing damit vom
  Speicherlayout ab (fiel nach Hinzufügen von `systest_cmdlibs.c` um);
  jetzt nur noch der Opcode.
